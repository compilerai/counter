#!/usr/bin/python

#Typical usage:
#
#source build.sh
#python superopt/utils/eq_run.py smpbench-build/cint/ctests.bc.O0 smpbench-build/cint/ctests.gcc48.O2.i386 all
#bash etfg_cmds && bash eq_cmds

import utils
import sys
import glob
import subprocess
import os
import datetime
import argparse
import shutil
import multiprocessing
import pprint
import random
import signal
import eq_cache
import re
from functools import partial
from eq_read_log import get_result_str, get_loc

def get_functions_size_etfg_map_from_llvm2tfg_log(f, etfg_name):
  fi = open(f, 'r')
  line = fi.readline()
  regex = re.compile(r'([^:]*) : (\d+)')
  ret = {}
  while line:
    fun = regex.search(line).group(1)
    sz = regex.search(line).group(2)
    #print fun + " : " + sz
    ret[fun] = (sz, etfg_name)
    line = fi.readline()
  fi.close()
  return ret

def get_bname_from_llvm_bc(llvm_bc):
  regex = re.compile(r'\/([^\/]+)$')
  filename = regex.search(llvm_bc).group(1)
  #print "filename = " + filename
  regex2 = re.compile(r'^([^\.]+)\.')
  match = regex2.search(filename)
  if match:
    bname = match.group(1)
  else:
    bname = filename
  #print "returning " + bname + " for " + llvm_bc + "\n"
  return bname

def get_compiler_opt_arch_from_x86_bin(x86_bin):
  gcc_regex = re.compile(r'gcc48.O(\d)')
  clang_regex = re.compile(r'clang36.O(\d)')
  found = gcc_regex.search(x86_bin)
  if found:
    opt_level = found.group(1)
    return "gcc" + opt_level
  found = clang_regex.search(x86_bin)
  if found:
    opt_level = found.group(1)
    return "clang" + opt_level
  gcc_regex = re.compile(r'gcc48-i386-O(\d)')
  clang_regex = re.compile(r'clang36-i386-O(\d)')
  found = gcc_regex.search(x86_bin)
  if found:
    opt_level = found.group(1)
    return "gcc" + opt_level
  found = clang_regex.search(x86_bin)
  if found:
    opt_level = found.group(1)
    return "clang" + opt_level
  raise Exception('could not determine compiler for ' + x86_bin)

def is_ignorable(bname, fname):
  expensive_smt_queries_list = [("ctests", "array_average"), ("ctests", "ht_hashcode"), ("ctests", "display_ratio"), ("perlbmk", "Perl_pp_i_modulo"), ("twolf", "Yacm_random"), ("twolf", "MTInvert"), ("twolf", "controlf"), ("twolf", "add_cell"), ("vpr", "is_cbox"), ("vpr", "get_closest_seg_start"), ("vpr", "get_seg_end"), ("vpr", "is_sbox"), ("vpr", "get_simple_switch_block_track"), ("gap", "ElmfRange"), ("vpr", "toggle_rr"), ("gap", "IsPossRange")]
  myfunctions_list = [("ctests", "myexit"), ("ctests", "myrand_char"), ("ctests", "myrand"), ("ctests", "mymemcpy"), ("ctests", "mymemset"), ("ctests", "my_char_inc"), ("ctests", "my_atoi"), ("ctests", "myfclose"), ("ctests", "myfree"), ("ctests", "myrealloc"), ("ctests", "mystrdup"), ("ctests", "mycalloc"), ("ctests", "mystrncmp"), ("ctests", "mymemcmp"), ("ctests", "myfreopen"), ("ctests", "mystrcmp"), ("ctests", "myfopen"), ("ctests", "myfdopen"), ("ctests", "mymalloc"), ("ctests", "myprint_char"), ("ctests", "myprint_int")]
  ignore_list = expensive_smt_queries_list + myfunctions_list
  if (bname, fname) in expensive_smt_queries_list:
    print("Ignoring (" + bname + ", " + fname + "): involves expensive smt queries")
    return True
  if (bname, fname) in myfunctions_list:
    print("Ignoring (" + bname + ", " + fname + "): it is a \"my\"function")
    return True
  return False

def main():
  utils.make_dir(eq_cache.cache_dir())
  FNULL=open(os.devnull, 'w')

  parser = argparse.ArgumentParser()
  parser.add_argument("files_and_functions", help = 'specify [llvm_bc_file x86_exec_file function_name] triples. Use function_name=all for all functions', nargs="+")
  #parser.add_argument("--no-eqgen", help='do not (re)generate eq files', action='store_true')
  parser.add_argument("--ignore-cache", help='ignore cache and make jobs for allfunctions', action='store_true')
  parser.add_argument("--make-jobs-only", help='make jobs only; do not run them; just print to stdout', action='store_true')
  parser.add_argument("--num-cpus", help='num of cpus for multiprocessing, default is max-cores avaiulable', type=int, default=multiprocessing.cpu_count())
  parser.add_argument("--resume", help='run what is not in cache', action='store_true')
  parser.add_argument("--pass-status-to-run", help='run with given pass-status', type=str, default='')
  parser.add_argument("--shuffle", help='shuffle the order of peeptabs before running', action='store_true')
  parser.add_argument("--global-timeout", help='global timeout in seconds', type=int, default=3600)
  parser.add_argument("--query-timeout", help='query timeout in seconds', type=int, default=600)
  parser.add_argument("--log-dir", help='dir for log files. default: cache-dir', type=str, default=eq_cache.cache_dir())
  #parser.add_argument("--test_acyclic", help='test acyclic sequences: collapse vs. nocollapse', action='store_true')
  #parser.add_argument("--force_acyclic", help='collapse tfgs so that acyclic sequences are represented by a single edge', action='store_true')

  args = parser.parse_args()
  force_run = args.ignore_cache
  resume = args.resume
  shuffle = args.shuffle
  pass_status_to_run = args.pass_status_to_run
  files_and_functions = args.files_and_functions
  #test_acyclic = args.test_acyclic
  #force_acyclic = args.force_acyclic

  #files = utils.merge_multiple_globs(args.files)
  #files = utils.filter_files_with_ext(files, 'ptab')
  #files = utils.remove_ext_in_list(files)

  file_function_map = {}
  for (llvm_bc_in, x86_bin_in, function_name) in zip(*[iter(files_and_functions)]*3):
    llvm_bc = os.path.abspath(llvm_bc_in)
    x86_bin = os.path.abspath(x86_bin_in)
    #print "llvm_bc = " + llvm_bc
    #print "x86_bin = " + x86_bin
    #print "function_name = " + function_name
    all_functions = set([function_name])
    if (llvm_bc, x86_bin) in file_function_map:
      cur_functions = file_function_map.get((llvm_bc, x86_bin))
    else:
      cur_functions = set()
    for f in all_functions:
      cur_functions.add(f)
    file_function_map[(llvm_bc, x86_bin)] = cur_functions

  llvm_bc_function_map = {}
  for (llvm_bc, x86_bin) in file_function_map:
    #print "llvm_bc = " + llvm_bc
    #print "x86_bin = " + x86_bin
    #for f in file_function_map.get((llvm_bc, x86_bin)):
    #  print "  f = " + f
    if llvm_bc in llvm_bc_function_map:
      cur_functions = llvm_bc_function_map.get(llvm_bc)
    else:
      cur_functions = set()
    for f in file_function_map.get((llvm_bc, x86_bin)):
      cur_functions.add(f)
    llvm_bc_function_map[llvm_bc] = cur_functions

  home = os.path.expanduser('~')
  etfg_cmds = []
  llvm_bc_functions_size_etfg_map = {}
  for llvm_bc in llvm_bc_function_map:
    cmd = home + "/llvm2tfg-build/bin/llvm2tfg " + llvm_bc
    bname = get_bname_from_llvm_bc(llvm_bc)
    if not ("all" in llvm_bc_function_map.get(llvm_bc)):
      cmd += " -f "
      for f in llvm_bc_function_map.get(llvm_bc):
        cmd += f + ","
      outfile = home + "/eqfiles/" + bname + ".some_functions.etfg"
    else:
      outfile = home + "/eqfiles/" + bname + ".etfg"
    dry_run_outfile = outfile + ".dry_run"
    dry_run_cmd = cmd + " -dry-run" + " -o " + dry_run_outfile
    cmd += " -o " + outfile
    st = datetime.datetime.now().strftime('--%Y-%m-%d-%H-%M-%S')
    #dry_run_log = home + "/superopt/build/llvm2tfg." + llvm_bc + ".dryrun.log." + st
    #fi = open(dry_run_log, 'w')
    p = subprocess.Popen(dry_run_cmd, stdout=FNULL, stderr=subprocess.STDOUT, shell=True)
    p.communicate()
    #fi.close()
    llvm_bc_functions_size_etfg_map[llvm_bc] = get_functions_size_etfg_map_from_llvm2tfg_log(dry_run_outfile, outfile)
    etfg_cmds.append(cmd)

  #for llvm_bc in llvm_bc_functions_size_etfg_map:
  #  print "hello llvm_bc = " + llvm_bc
  #  fmap = llvm_bc_functions_size_etfg_map.get(llvm_bc)
  #  for f in fmap:
  #    print "hello f = " + f

  etfg_cmds_fp = open('etfg_cmds', 'w')
  for cmd in etfg_cmds:
    etfg_cmds_fp.write(cmd + "\n")
  etfg_cmds_fp.close()

  file_functions = {}
  for (llvm_bc, x86_bin) in file_function_map:
    fs = file_function_map.get((llvm_bc, x86_bin))
    if "all" in fs:
      funs = set()
      fmap = llvm_bc_functions_size_etfg_map.get(llvm_bc)
      for fun in fmap:
        funs.add(fun)
      file_functions[(llvm_bc, x86_bin)] = funs
    else:
      file_functions[(llvm_bc, x86_bin)] = fs

  eq_cmds = []
  for (llvm_bc, x86_bin) in file_functions:
    functions_in_this_benchmark = file_functions.get((llvm_bc, x86_bin))
    for f in functions_in_this_benchmark:
      bname = get_bname_from_llvm_bc(llvm_bc)
      if (is_ignorable(bname, f)):
        continue
      harvest = home + '/superopt/build/i386_i386/harvest'
      eqgen = home + '/superopt/build/etfg_i386/eqgen'
      eq = home + '/superopt/build/etfg_i386/eq'
      chaperon = home + '/superopt/utils/chaperon.py'
      fmap = llvm_bc_functions_size_etfg_map.get(llvm_bc)
      compiler_opt_arch = get_compiler_opt_arch_from_x86_bin(x86_bin)
      #print "f = " + f
      (_, etfg_file) = fmap.get(f)
      harvest_file =  home + "/eqfiles/" + bname + "." + compiler_opt_arch + "." + f + ".harvest"
      harvest_logfile = harvest_file + ".log"
      base_f = home + "/eqfiles/" + bname + "." + compiler_opt_arch + "." + f
      tfg_file = base_f + ".tfg"
      proof_file = base_f + ".proof"
      harvest_cmd = harvest + " -functions_only -live_callee_save -allow_unsupported -no_canonicalize_imms -no_eliminate_unreachable_bbls -use_orig_regnames -function_name " + f + " -o " + harvest_file + " -l " + harvest_logfile + " " + x86_bin
      eqgen_cmd = eqgen + " -f " + f + " -tfg_llvm " + etfg_file + " -l " + harvest_logfile + " -o " + tfg_file + " -e " + x86_bin + " " + harvest_file
      eq_cmd = eq + " " + f + " " + etfg_file + " " + tfg_file + " --proof " + proof_file + " --cleanup-query-files"
      cmd = harvest_cmd + " && " + eqgen_cmd + " && " + eq_cmd
      chaperoned_cmd = chaperon + " \"" + cmd + "\" --logfile " + "eqfiles/" + bname + "." + compiler_opt_arch + "." + f + ".eqlog"
      eq_cmds.append((llvm_bc, x86_bin, f, chaperoned_cmd))

  #eq_cmds.sort(cmp_eq_cmds_based_on_llvm_bc_function_sizes)
  if shuffle:
    random.shuffle(eq_cmds)
  else:
    eq_cmds = sorted(eq_cmds, key=lambda cmd: int(llvm_bc_functions_size_etfg_map.get(cmd[0]).get(cmd[2])[0]))

  utils.make_dir(args.log_dir)
  eq_cmds_fp = open('eq_cmds', 'w')
  for (_, _, _, cmd) in eq_cmds:
    eq_cmds_fp.write(cmd + "\n")
  eq_cmds_fp.close()
  print("Output files: etfg_cmds and eq_cmds")
  #make_jobs_and_run(files, args.log_dir, args.num_jobs, force_run, resume, pass_status_to_run, args.force_acyclic, args.test_acyclic, args.no_eqgen, args.global_timeout, args.query_timeout, args.make_jobs_only)

if __name__ == "__main__":
  main()
