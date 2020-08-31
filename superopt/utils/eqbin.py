#!/usr/bin/python

import utils
import sys
import glob
import subprocess
import os
import datetime
import argparse
import magic
import shutil
import multiprocessing
import pprint
import random
import signal
import eq_cache
import re
from functools import partial
from eq_read_log import get_result_str, get_loc

root = os.getenv("SUPEROPT_PROJECT_DIR", default=os.getenv("HOME"))

def run_llvm2tfg(function_name, bc, etfg):
  fun_name_opt = ''
  if function_name != 'ALL':
    fun_name_opt = '-f ' + function_name
  llvm2tfg_cmd = root + "/llvm-project/build/bin/llvm2tfg " + fun_name_opt + " " + bc + " -o " + etfg
  print("llvm2tfg_cmd: " + llvm2tfg_cmd)
  sys.stdout.flush()
  p = subprocess.Popen(llvm2tfg_cmd, shell=True)
  p.communicate()
  return p.returncode

def run_harvest(function_name, harvest, harvest_log, i386):
  harvest_cmd = root + "/superopt/build/i386_i386/harvest -f " + function_name + " -functions_only -live_callee_save -allow_unsupported -no_canonicalize_imms -no_eliminate_unreachable_bbls -no_eliminate_duplicates -use_orig_regnames -o " + harvest + " -l " + harvest_log + " " + i386
  print("harvest_cmd: " + harvest_cmd)
  p = subprocess.Popen(harvest_cmd, shell=True)
  p.communicate()
  return p.returncode

def run_eqgen(function_name, etfg, harvest_log, tfg, i386, harvest):
  eqgen_cmd = root + "/superopt/build/etfg_i386/eqgen -tfg_llvm " + etfg + " -l " + harvest_log + " -o " + tfg + " -e " + i386 + " -f " + function_name + " " + harvest
  print("eqgen_cmd: " + eqgen_cmd)
  sys.stdout.flush()
  p = subprocess.Popen(eqgen_cmd, shell=True)
  p.communicate()
  return p.returncode

def main():
  parser = argparse.ArgumentParser()
  parser.add_argument("-n", help = 'stop after generating .etfg and .tfg files', action='store_true', default=False)
  parser.add_argument("--noclobber", help = 'do not generate if files are already present', action='store_true', default=False)
  parser.add_argument("bc", help = 'bitcode filename', type=str)
  parser.add_argument("i386", help = 'i386 filename', nargs='+', type=str)
  parser.add_argument("-f", help = 'function name', type=str, default='ALL')

  args = parser.parse_args()

  function_name = args.f
  bc_orig = args.bc
  bc = os.path.realpath(bc_orig)
  etfg = bc + "." + function_name + ".etfg"
  if not args.noclobber or not os.path.exists(etfg) or os.path.getsize(etfg) == 0:
    if run_llvm2tfg(function_name, bc, etfg) != 0:
      return 1

  # TODO: parallelize this embarrassingly parallel loop?
  for i386_orig in args.i386:
    i386 = os.path.realpath(i386_orig)
    tfg = i386 + "." + function_name + ".tfg"
    i386_filetype = magic.from_file(i386, mime=True)
    if i386_filetype == 'application/x-executable' or i386_filetype == 'application/x-object':
      harvest = i386 + "." + function_name + ".harvest"
      harvest_log = harvest + ".log"
      if not args.noclobber or not os.path.exists(harvest) or os.path.getsize(harvest) == 0:
        if run_harvest(function_name, harvest, harvest_log, i386) != 0:
          return 2
      eqgen_cmd = ""
      if not args.noclobber or not os.path.exists(tfg) or os.path.getsize(tfg) == 0:
        if run_eqgen(function_name, etfg, harvest_log, tfg, i386, harvest) != 0:
          return 3
    else:
      bc2tfg_cmd = ""
      if not args.noclobber or not os.path.exists(tfg) or os.path.getsize(tfg) == 0:
        if run_llvm2tfg(function_name, i386, tfg) != 0:
          return 2

    if not args.n:
      proof = bc_orig + "__" + i386_orig + "." + function_name + ".proof"
      eq_cmd = ""
      eq_cmd += root + "/superopt/build/etfg_i386/eq "
      #eq_cmd += " --global-timeout 10 "
      if i386_filetype != 'application/x-executable':
        eq_cmd += ' --llvm2ir'
      eq_cmd += " --proof " + proof + " -f " + function_name + " " + etfg + " " + tfg
      print("eq_cmd: " + eq_cmd)
      p = subprocess.Popen(eq_cmd, shell=True)
      p.communicate()
      if p.returncode != 0:
        return 4

  return 0

if __name__ == "__main__":
  exit(main())
