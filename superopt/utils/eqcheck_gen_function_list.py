#!/usr/bin/python
from config_host import *
import sys
import subprocess
import re
import os
import argparse
import shutil
import utils
import glob
import random

infinity=99999999

def uniq(input):
  output = []
  for x in input:
    if x not in output:
      output.append(x)
  return output

def gen_ddsum_for_exe_no_gcc(exe):
  ddsum = exe + '.ddsum'
  ddtypes = exe + '.ddtypes'
  if (not os.path.exists(ddsum)):
    cmd = ['perl', '-I' + srcdir + '/utils', srcdir + '/utils/gen_dwarfdump.pl', exe]
    subprocess.call(cmd)
    cmd = ['perl', '-I' + srcdir + '/utils', srcdir + '/utils/parse_dwarfdump.pl', exe]
    subprocess.call(cmd)
  return (ddsum, ddtypes)

def get_functions(exe):
  ret = []
  exe = re.sub('ccomp', 'gcc48', exe)
  (ddsum,ddtypes) = gen_ddsum_for_exe_no_gcc(exe) 
  print(("ddsum = " + ddsum))
  with open(ddsum, 'r') as fh:
    for line in fh:
      m = re.search('FUNCTION(\d*): (\S*)', line)
      if m:
        fun = m.group(2)
        if fun != "Option": #the Option function is too big in crafty (causes memory scalability issue); ignore it
          #print ("found function " + fun + " at index " + str(len(ret)))
          ret.append(fun)
        else:
          print(("IGNORING function " + fun + " (as hard-coded in eqcheck_gen_function_list.py)"))
  #random.shuffle(ret)
  ret = uniq(ret)
  print(("number of functions = " + str(len(ret))))
  sys.stdout.flush()
  return ret

def get_harvested_string(filename, function_name):
  fh = open(filename, 'r')
  ret = ''
  print(("function_name " + function_name))
  for line in fh:
    ret += line
    m = re.search('^== \S+ \S+ (\S+):', line)
    print(("line = " + line))
    if m:
      fun = m.group(1)
      print(("fun = " + fun))
      if fun == function_name:
        fh.close()
        return ret
      else:
        ret = ''
  fh.close()
  return ''


def gen_ddsum_for_exe(exe):
  # the following three lines ensure that only gcc48-O2's ddsum is used : hack
  # this also means that all those dd/ddsum files should be already present
  exe_gcc = exe
  exe_gcc = re.sub('ccomp', 'gcc48', exe_gcc)
  exe_gcc = re.sub('-O2-', '-O0-', exe_gcc)
  ddsum = exe + '.ddsum'
  ddtypes = exe + '.ddtypes'
  if (exe == exe_gcc):
    if (os.path.exists(ddsum)):
      subprocess.call(['rm', ddsum])
    if (os.path.exists(ddtypes)):
      subprocess.call(['rm', ddtypes])
  return gen_ddsum_for_exe_no_gcc(exe_gcc)

def harvest_exe(exe, force_harvest, fun, ddsum, ddtypes):
  dd = exe + '.dd'
  #ddsum = exe + '.ddsum'
  #print "ddsum1="+ddsum1+", ddsum="+ddsum+"\n"
  harvest = exe + '.eqchecker_harvest' + '.' + fun
  log = exe + '.eqchecker_log' + '.' + fun
  #symbols_filename = exe + '.symbols' + '.' + fun
  if not (os.path.exists(harvest) and os.path.exists(log)) or force_harvest:
    if os.path.exists(dd):
      subprocess.call(['rm', dd])
    if os.path.exists(harvest):
      subprocess.call(['rm', harvest])
    if os.path.exists(log):
      subprocess.call(['rm', log])
    #if os.path.exists(symbols_filename):
    #  subprocess.call(['rm', symbols_filename])
    
    cmd = ['perl', '-I' + srcdir + '/utils', srcdir + '/utils/gen_dwarfdump.pl', exe]
    subprocess.call(cmd)

    print('calling harvest for function ' + fun)
    sys.stdout.flush()
    cmd = [build_dir + '/i386-i386/harvest', '-functions_only', '-live_callee_save',
    '-allow_unsupported', '-no_canonicalize_imms', '-no_eliminate_unreachable_bbls',
    '-function_name', fun,
    '-add_transmap_identity', '-ddsum', ddsum, '-ddtypes', ddtypes,
    '-annotate_locals_using_dwarf', dd,
    #symbols_filename_str, symbols_filename,
    '-o', harvest, '-l', log, exe]
    print(cmd)
    sys.stdout.flush()
    subprocess.call(cmd)
  else:
    print('Harvest and log file found, not harvesting')
  return (harvest, log)

def gen_loop_info(log, fun):
  ret = 'X'
  with open(log, 'r') as fh:
    curfun = ''
    for line in fh:
      m = re.search('bbl \[(.*?)\]:', line)
      if m:
        curfun = m.group(1)
        if fun == curfun:
          ret = 'A'
      if (fun == curfun) and 'loop:' in line:
        ret = 'C'
        break
  return ret

harvest_cache = {}
def harvest_exe_cache(exe, force_harvest, fun, ddsum, ddtypes):
  global harvest_cache
  if (exe, fun) not in harvest_cache:
    (harvest, log) = harvest_exe(exe, force_harvest, fun, ddsum, ddtypes)
    harvest_cache[(exe, fun)] = (harvest, log, gen_loop_info(log, fun))
  return harvest_cache[(exe, fun)]

def get_exe_name(benchname, compiler, opt, arch, which_spec):
  if which_spec == 'spec2000':
    return srcdir + "/../specrun-build/spec2000/"+ compiler + "-" + arch + "-" + opt + "/" + benchname
  elif which_spec == 'spec2006':
    return srcdir + "/../specrun-build/spec2006/"+ compiler + "-" + arch + "-" + opt + "/" + benchname
  elif which_spec == "smpbench":
    opt_mod = re.sub('-soft-float', '', opt)
    return srcdir + "/../smpbench-build/cint/"+ benchname + "." + compiler + "." + opt_mod + "." + arch
  else:
    assert(False)

def get_llvm_bc_name(benchname, which_spec):
  if which_spec == 'spec2000':
    return srcdir + "/../specrun-build/spec2000/llvm-bc-O0/" + benchname
  elif which_spec == 'spec2006':
    return srcdir + "/../specrun-build/spec2006/llvm-bc-O0/" + benchname
  elif which_spec == "smpbench":
    return srcdir + "/../smpbench-build/cint/"+ benchname + ".bc.O0"
  else:
    assert(False)

#def belongs_to_ignore_peeptabs(ignore_peeptabs, benchname, (compiler1, opt1, arch1), (compiler2, opt2, arch2), function_name):
#  opt1s = re.sub('soft-float', 'sf', opt1)
#  opt2s = re.sub('soft-float', 'sf', opt2)
#  config_pair = compiler1 + "-" + opt1s + "-" + arch1 + "-" + compiler2 + "-" + opt2s + "-" + arch2
#  if (benchname, config_pair, function_name) in ignore_peeptabs:
#    #print "belongs_to_ignore_peeptabs: returning true for " + benchname + ", " + config_pair + ", " + function_name + "."
#    return True
#  #print "belongs_to_ignore_peeptabs: returning false for " + benchname + ", " + config_pair + ", " + function_name
#  return False

def filter_and_sort_using_results_file(functions, results_file, benchname, xxx_todo_changeme):
  (compiler2, opt2, arch2) = xxx_todo_changeme
  if results_file == '':
    return functions
  out = []
  already_passed = []
  opt1s = re.sub('soft-float', 'sf', opt1)
  opt2s = re.sub('soft-float', 'sf', opt2)
  #config_pair_in = compiler1 + "-" + opt1s + "-" + arch1 + "-" + compiler2 + "-" + opt2s + "-" + arch2
  config_pair_in = compiler2 + "-" + opt2s + "-" + arch2
  fh = open(results_file, 'r')
  for line in fh:
    f = re.search('^peeptabs-(\S*)--(\S*)--(\S*)-[CAX][CAX]  ', line)
    if f:
      prog = f.group(1)
      config_pair = f.group(2)
      function_name = f.group(3)
      if (function_name in functions) and (prog == benchname) and (config_pair == config_pair_in):
        m = re.search('^peeptabs-\S*--\S*--\S*-[CAX][CAX]  FAILED: (.*)  loc', line)
        if m:
          failure_reason = m.group(1)
          if (    (failure_reason.find("float") == -1)
              and (failure_reason.find("unsupported") == -1)):
            #print ("appending " + prog + " config " + config_pair + " function " + function_name + " to out non-float non-unsupported failure " + failure_reason)
            out.append(function_name)
        n = re.search('^peeptabs-\S*--\S*--\S*-[CAX][CAX]  passed', line)
        if n:
          already_passed.append(function_name)
  fh.close()
  for f in functions:
    if (not (f in out)) and (not (f in already_passed)):
      #print ("appending " + prog + " config " + config_pair + " function " + function_name + " to out not already-passed")
      out.append(f)
  print("Sorted functions:")
  for f in out:
    print((str(out.index(f)) + ". " + f))
  return out 

def gen_function_list_for_spec(benchname, xxx_todo_changeme1, which_spec, out_dir, results_file, prefix, reuse_harvest_files, acyclic_only):
  (compiler2, opt2, arch2) = xxx_todo_changeme1
  suffix = '%s-%s-%s' % (compiler2, opt2, arch2)
  print('Doing: ' + suffix)

  #exe1 = get_exe_name(benchname, compiler1, opt1, arch1, which_spec)
  exe2 = get_exe_name(benchname, compiler2, opt2, arch2, which_spec)
  #(ddsum1, ddtypes1) = gen_ddsum_for_exe(exe1)
  #(ddsum2, ddtypes2) = gen_ddsum_for_exe(exe2)

  if which_spec == 'smpbench':
    suffix = suffix.replace('-soft-float', '')
  else:
    suffix = suffix.replace('soft-float', 'sf')

  print("Calling get_functions(" + exe2 + ")\n")
  functions = get_functions(exe2)

  out_fn = out_dir + '/' + suffix + '.functions'

  sorted_functions = filter_and_sort_using_results_file(functions, results_file, benchname, (compiler2, opt2, arch2))

  out_fp = open(out_fn, "w")
  for fun in sorted_functions:
    if reuse_harvest_files:
      reuse_harvest_files_opt = "--reuse-harvest-files "
    else:
      reuse_harvest_files_opt = ""
    if acyclic_only:
      acyclic_only_opt = "--acyclic-only "
    else:
      acyclic_only_opt = ""
    command = srcdir + "/utils/peeptab_gen.py " + reuse_harvest_files_opt + acyclic_only_opt + benchname + " " + which_spec + " " + compiler2 + " " + opt2 + " " + fun
    print(command, file=out_fp)
  out_fp.close()

#def gen_function_list_for_spec_all_configs(benchname, which_spec, which_comp, out_dir, results_file, prefix, reuse_harvest_files, acyclic_only):
  ##gcc48_0_sf_i386 = ("gcc48", "O0-soft-float", "i386")
  #gcc48_2_sf_i386 = ("gcc48", "O2-soft-float", "i386")
  #gcc48_3_sf_i386 = ("gcc48", "O3-soft-float", "i386")
  ##clang36_0_sf_i386 = ("clang36", "O0-soft-float", "i386")
  #clang36_2_sf_i386 = ("clang36", "O2-soft-float", "i386")
  #clang36_3_sf_i386 = ("clang36", "O3-soft-float", "i386")
  ##ccomp_0_sf_i386 = ("ccomp", "O0-soft-float", "i386")
  ##ccomp_2_sf_i386 = ("ccomp", "O2-soft-float", "i386")
  ##icc_0_i386 = ("icc", "O0-soft-float", "i386")
  #icc_2_i386 = ("icc", "O2-soft-float", "i386")
  #icc_3_i386 = ("icc", "O3-soft-float", "i386")
  #if which_comp == 'all' or which_comp == 'O2' or which_comp == 'gccO2':
  #  gen_function_list_for_spec(benchname, gcc48_2_sf_i386, which_spec, out_dir, results_file, prefix, reuse_harvest_files, acyclic_only)
  #if which_comp == 'all' or which_comp == 'O3' or which_comp == 'gccO3':
  #  gen_function_list_for_spec(benchname, gcc48_3_sf_i386, which_spec, out_dir, results_file, prefix, reuse_harvest_files, acyclic_only)
  #if which_comp == 'all' or which_comp == 'O2' or which_comp == 'clangO2':
  #  gen_function_list_for_spec(benchname, clang36_2_sf_i386, which_spec, out_dir, results_file, prefix, reuse_harvest_files, acyclic_only)
  #if which_comp == 'all' or which_comp == 'O3' or which_comp == 'clangO3':
  #  gen_function_list_for_spec(benchname, clang36_3_sf_i386, which_spec, out_dir, results_file, prefix, reuse_harvest_files, acyclic_only)
  #if which_comp == 'all' or which_comp == 'O2' or which_comp == 'iccO2':
  #  gen_function_list_for_spec(benchname, icc_2_i386, which_spec, out_dir, results_file, prefix, reuse_harvest_files, acyclic_only)
  #if which_comp == 'all' or which_comp == 'O3' or which_comp == 'iccO3':
  #  gen_function_list_for_spec(benchname, icc_3_i386, which_spec, out_dir, results_file, prefix, reuse_harvest_files, acyclic_only)

if __name__ == "__main__":
  parser = argparse.ArgumentParser()
  parser.add_argument("benchmark", help = "specify benchmark", type = str)
  parser.add_argument("benchmarktype", help = "which benchmark", type = str, choices = ['smpbench', 'spec2000', 'spec2006'])
  #parser.add_argument("compiler_opts", help = "which compiler-optimization pairs", type = str, choices = ['all', 'O2', 'O3', 'gccO2', 'gccO3', 'clangO2', 'clangO3', 'iccO2', 'iccO3', 'ccompO2', 'cg02', 'cg03'])
  parser.add_argument("compiler_opts", help = "which compiler-optimization pairs", type = str, choices = ['llvm-bc'])
  #parser.add_argument('-f', '--function_name', dest = 'function_name', help = "function name to harvest", type = str, default = "", action = 'store')
  #parser.add_argument('-fstart', '--function_start_index', dest = 'function_start_index', help = "start index of the functions to harvest", type = int, default = 0, action = 'store')
  #parser.add_argument('-fstop', '--function_stop_index', dest = 'function_stop_index', help = "stop index of the functions to harvest", type = int, default = infinity, action = 'store')
  parser.add_argument('-results', '--results_file', dest = 'results_file', help = "filename containing the results so far", type = str, default = "", action = 'store')
  #parser.add_argument('--prefix', help = "prefix to be prepended to each output command", type = str, default = 'python ' + srcdir + "/../grid/gridsubmit.py ", action = 'store')
  parser.add_argument('-reuse-harvest-files', '--reuse-harvest-files', help = "reuse harvested files (_eqcheck_harvest._)", action = 'store_true')
  parser.add_argument('-acyclic-only', '--acyclic-only', help = "harvest only acyclic functions", action = 'store_true')
  #parser.add_argument('-eq', '--eq', help = "whether to run eq", action='store_true')

  args = parser.parse_args()

  benchname = args.benchmark
  which_spec = args.benchmarktype
  which_comp = args.compiler_opts
  #function_name = args.function_name
  #fstart_index = args.function_start_index
  #fstop_index = args.function_stop_index
  results_file = args.results_file
  #run_eq = args.eq
  reuse_harvest_files = args.reuse_harvest_files
  acyclic_only = args.acyclic_only

  #print "prefix : " + args.prefix

  #force_harvest = True
  out_dir = srcdir + '/../peeptabs-' + benchname
  if not os.path.exists(out_dir):
    os.mkdir(out_dir)
  #gen_function_list_for_spec_all_configs(benchname, which_spec, which_comp, out_dir, results_file, "", reuse_harvest_files, acyclic_only)
  bc = get_llvm_bc_name(benchname, which_spec)
  bc_asm = bc + ".s"
  cmd = "llvm-dis-3.6 < " + bc + " > " + bc_asm
  subprocess.call(cmd, shell=True)
  ret = []
  out_fn = out_dir + '/gcc48.O2.functions'
  out_fp = open(out_fn, "w")
  with open(bc_asm) as f:
    for line in f:
      m = re.match("^define .* \@([^\(]*)\(.*\).*\{", line)
      if m:
        fun_name = m.groups(0)[0]
        #print "benchname " + benchname + ", fun = " + fun_name
        command = srcdir + "/utils/peeptab_gen.py " + benchname + " " + which_spec + " gcc48 O2 " + fun_name + " > " + benchname + "." + "peeptab_gen" + ".log 2>&1"
        print(command, file=out_fp)
        #ret.append(fun_name)
  out_fp.close()
  #return ret

