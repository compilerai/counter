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

def eqfile_exists(ptab):
  ptab = os.path.abspath(ptab)
  eqfile = ptab
  eqfile = eqfile.replace(".ptab", ".eq")
  eqfile = eqfile.replace("peeptabs", "eqfiles")
  return os.path.exists(eqfile)

def identify_peeptabs_with_missing_eqfiles(peeptabs):
  ret = []
  for p in peeptabs:
    if not eqfile_exists(p):
      ret.append(p)
  return ret

def peeptab_get_function_and_bitcode_file_names(peeptab_path):
  peeptab_filename = os.path.basename(peeptab_path)
  m = re.search('(^[^-]+)', peeptab_filename)
  function_name = m.group(0)
  #print "function_name = " + function_name + ".\n"
  peeptab_dirname = os.path.dirname(os.path.dirname(peeptab_path))
  #print "peeptab_dirname = " + peeptab_dirname + ".\n"
  m = re.search('([^-]+$)', peeptab_dirname)
  execname = m.group(0)
  #print "execname = " + execname + ".\n"
  if execname == 'ctests':
    bitcode_filename = os.path.dirname(peeptab_dirname) + '/smpbench-build/cint/ctests.bc.O0'
  elif execname == 'sjeng':
    bitcode_filename = os.path.dirname(peeptab_dirname) + '/specrun-build/spec2006/llvm-bc-O0/' + execname
  else:
    bitcode_filename = os.path.dirname(peeptab_dirname) + '/specrun-build/spec2000/llvm-bc-O0/' + execname
  #print "bitcode_filename = " + bitcode_filename + ".\n"
  return (function_name, bitcode_filename)

def ignore(function_name):
  ignore_list = ['my_atoi', 'myexit', 'mymalloc', 'mymemcpy', 'mymemset', 'myalloca', 'myabort', 'mymemcmp', 'mystrcmp']
  if ignore_list.count(function_name) == 1:
    return True
  else:
    return False

if __name__ == "__main__":
  parser = argparse.ArgumentParser()
  #parser.add_argument('--prefix', help = "prefix to be prepended to each output command", type = str, default = 'python ' + srcdir + "/../grid/gridsubmit.py ", action = 'store')
  #parser.add_argument('--batchsize', help = "number of eqfile generations per command (default 1)", type = int, default = 1, action = 'store')
  #parser.add_argument('-eq', '--eq', help = "whether to run eq", action='store_true')
  parser.add_argument('-missing_only', '--missing_only', help = "generate missing eqfiles only", action = 'store_true')

  (args, peeptabs) = parser.parse_known_args()
  #prefix = args.prefix
  batchsize = 1 #args.batchsize

  i = 0
  #if not (prefix == ""):
  #  sys.stdout.write(prefix + " \"")
  if (args.missing_only):
    peeptabs_with_missing_eqfiles = identify_peeptabs_with_missing_eqfiles(peeptabs)
  else:
    peeptabs_with_missing_eqfiles = peeptabs
  for p in peeptabs_with_missing_eqfiles:
    i += 1
    p = os.path.abspath(p)
    eqfile_template = p
    eqfile_template = eqfile_template.replace("peeptabs", "eqfiles")
    elog = eqfile_template.replace(".ptab", ".eqlog")
    llvm_eqfile = eqfile_template.replace(".ptab", ".eq.llvm")
    llvm_eqfilelog = eqfile_template.replace(".ptab", ".eq.llvm.log")
    if not os.path.exists(os.path.dirname(elog)):
      subprocess.call("mkdir -p " + os.path.dirname(elog), shell=True)
    (function_name, bitcode_filename) = peeptab_get_function_and_bitcode_file_names(p);
    if ignore(function_name):
      continue
    sys.stdout.write(build_dir + "/../../llvm2tfg-build/bin/llvm2tfg -f " + function_name + " " + bitcode_filename + " -o " + llvm_eqfile + " > " + llvm_eqfilelog + " 2>&1")
    sys.stdout.write(" && " + build_dir + "/eqgen -tfg_llvm " + llvm_eqfile + " " + p + " > " + elog + " 2>&1")
    if i % batchsize == 0:
      #if prefix == "":
      sys.stdout.write("\n")
      #else:
      #  sys.stdout.write("\"\n" + prefix + "\"")
    else:
      print(" && ", end=' ')
  if not (i % batchsize == 0):
    #if prefix == "":
    sys.stdout.write("/bin/true\n")
    #else:
    #  sys.stdout.write("/bin/true\"\n")
