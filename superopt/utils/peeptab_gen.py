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

def do_harvest(exe, fun, is_O0_spec, harvest, log):
  dd = exe + '.dd'
  cmd = ['perl', '-I' + srcdir + '/utils', srcdir + '/utils/gen_dwarfdump.pl', exe]
  subprocess.call(cmd)

  print('calling harvest for function ' + fun)
  sys.stdout.flush()
  cmd = [build_dir + '/harvest', '-functions_only', '-live_callee_save',
  '-allow_unsupported', '-no_canonicalize_imms', '-no_eliminate_unreachable_bbls',
  '-function_name', fun,
  '-add_transmap_identity',
  #'-ddsum', ddsum,
  #'-ddtypes', ddtypes,
  '-o', harvest, '-l', log]
  if is_O0_spec:
    cmd.extend(['-annotate_locals_using_dwarf', dd])
  cmd.extend([exe])
  print(cmd)
  sys.stdout.flush()
  subprocess.call(cmd)

def harvest_exe(exe, fun, reuse_harvest_files, is_O0_spec):
  #ddsum = exe + '.ddsum'
  #print "ddsum1="+ddsum1+", ddsum="+ddsum+"\n"
  harvest = exe + '.eqchecker_harvest' + '.' + fun
  log = exe + '.eqchecker_log' + '.' + fun
  print("reuse_harvest_files " + str(reuse_harvest_files))
  if not (reuse_harvest_files and os.path.exists(harvest) and os.path.exists(log)):
    do_harvest(exe, fun, is_O0_spec, harvest, log)
  else:
    print("os.path.exists(" + harvest + ") = " + str(os.path.exists(harvest)))
    print("os.path.exists(" + log + ") = " + str(os.path.exists(log)))
  return (harvest, log, gen_loop_info(log, fun))

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

#harvest_cache = {}
#def harvest_exe_cache(exe, force_harvest, fun, ddsum, ddtypes):
#  global harvest_cache
#  if (exe, fun) not in harvest_cache:
#    (harvest, log) = harvest_exe(exe, force_harvest, fun, ddsum, ddtypes)
#    harvest_cache[(exe, fun)] = (harvest, log, gen_loop_info(log, fun))
#  return harvest_cache[(exe, fun)]

def get_exe_name(benchname, compiler, opt, arch, which_spec):
  if which_spec == 'spec2000':
    return srcdir + "/../specrun-build/spec2000/"+ compiler + "-" + arch + "-" + opt + "/" + benchname
  elif which_spec == 'spec2006':
    return srcdir + "/../specrun-build/spec2006/"+ compiler + "-" + arch + "-" + opt + "/" + benchname
  elif which_spec == "smpbench":
    #opt_mod = re.sub('-soft-float', '', opt)
    return srcdir + "/../smpbench-build/cint/"+ benchname + "." + compiler + "." + opt + "." + arch
  else:
    assert(False)

def gen_peeptab_for_spec(benchname, which_spec, xxx_todo_changeme, fun, run_eq, reuse_harvest_files, acyclic_only):
  #suffix = '%s-%s-%s-%s-%s-%s' % (compiler1, opt1, arch1, compiler2, opt2, arch2)
  (compiler2, opt2, arch2) = xxx_todo_changeme
  suffix = '%s-%s-%s' % (compiler2, opt2, arch2)
  print('Doing: ' + suffix)

  #exe1 = get_exe_name(benchname, compiler1, opt1, arch1, which_spec)
  exe2 = get_exe_name(benchname, compiler2, opt2, arch2, which_spec)
  #(ddsum1, ddtypes1) = gen_ddsum_for_exe(exe1)
  #(ddsum2, ddtypes2) = gen_ddsum_for_exe(exe2)

  #suffix = suffix.replace('-soft-float', '')
  #if which_spec == 'smpbench':
  #  suffix = suffix.replace('-soft-float', '')
  #else:
  #  suffix = suffix.replace('soft-float', 'sf')

  #(harvest1, log1, loop_info1) = harvest_exe(exe1, fun, ddsum1, ddtypes1, reuse_harvest_files, True)
  (harvest2, log2, loop_info2) = harvest_exe(exe2, fun, reuse_harvest_files, False)
  if (acyclic_only and (loop_info2 == 'C')):
    return

  out_dir = srcdir + '/../peeptabs-' + benchname
  if not os.path.exists(out_dir):
    os.mkdir(out_dir)

  out_dir = out_dir + '/' + suffix

  if not os.path.exists(out_dir):
    os.mkdir(out_dir)

  #out_filename = out_dir + "/" + fun + '-' + loop_info1 + loop_info2 + ".ptab"
  out_filename = out_dir + "/" + fun + '-' + loop_info2 + ".ptab"
  #eq_filename = out_dir + "/" + fun + '-' + loop_info1 + loop_info2 + ".eq"
  eq_filename = out_dir + "/" + fun + '-' + loop_info2 + ".eq"
  print("calling h2p")
  sys.stdout.flush()
  #subprocess.call([build_dir + '/h2p', '-o',
  #    out_filename,
  #    '-function_name', fun,
  #    '-log1', log1, '-log2', log2, harvest1, harvest2])
  subprocess.call([build_dir + '/h2p', '-o',
      out_filename,
      '-function_name', fun,
      '-log2', log2, harvest2])

  if run_eq:
    print("calling eqgen")
    sys.stdout.flush()
    subprocess.call(['../superopt/utils/eq_run.py', out_filename, '--gen-eq-file', '--num-jobs', '1'])
    print("calling eq")
    sys.stdout.flush()
    subprocess.call(['../superopt/utils/eq_run.py', eq_filename, '--num-jobs', '1'])

if __name__ == "__main__":
  parser = argparse.ArgumentParser()
  parser.add_argument('-reuse-harvest-files', '--reuse-harvest-files', help = "reuse harvested files (_eqcheck_harvest._)", action = 'store_true')
  parser.add_argument("benchmark", help = "specify benchmark program", type = str)
  parser.add_argument("spec", help = "which benchmark", type = str, choices = ['smpbench', 'spec2000', 'spec2006'])
  #parser.add_argument("compiler1", help = "which compiler for first", type=str, choices = ['gcc48', 'clang36', 'ccomp', 'icc'])
  #parser.add_argument("opt1", help = "which optimization for first", type=str, choices = ['O0-soft-float', 'O2-soft-float', 'O3-soft-float'])
  parser.add_argument("compiler2", help = "which compiler for second", type=str, choices = ['gcc48', 'clang36', 'ccomp', 'icc'])
  parser.add_argument("opt2", help = "which optimization for second", type=str, choices = ['O0', 'O2', 'O3'])
  #parser.add_argument('ddsum1',  help = "ddsum1", type = str, action = 'store')
  #parser.add_argument('ddsum2',  help = "ddsum2", type = str, action = 'store')
  #parser.add_argument('ddtypes1',  help = "ddtypes1", type = str, action = 'store')
  #parser.add_argument('ddtypes2',  help = "ddtypes2", type = str, action = 'store')
  parser.add_argument('function_name', help = "function name to harvest", type = str, default = "", action = 'store')
  parser.add_argument('-eq', '--eq', help = "whether to run eq", action='store_true')
  parser.add_argument('-acyclic-only', '--acyclic-only', help = "harvest only acyclic O0 functions", action='store_true')

  args = parser.parse_args()

  benchname = args.benchmark
  which_spec = args.spec
  #comp1 = args.compiler1
  #opt1 = args.opt1
  comp2 = args.compiler2
  opt2 = args.opt2
  function_name = args.function_name
  run_eq = args.eq
  reuse_harvest_files = args.reuse_harvest_files
  acyclic_only = args.acyclic_only

  #print ('run_eq = ' + str(run_eq))
  #gen_peeptab_for_spec(benchname, which_spec, (comp1, opt1, "i386"), (comp2, opt2, "i386"), (args.ddsum1, args.ddtypes1), (args.ddsum2, args.ddtypes2), function_name, run_eq, reuse_harvest_files, acyclic_only)
  gen_peeptab_for_spec(benchname, which_spec, (comp2, opt2, "i386"), function_name, run_eq, reuse_harvest_files, acyclic_only)
