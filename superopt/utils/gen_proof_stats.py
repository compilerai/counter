#!/usr/bin/python

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
#from eq_read_log import get_result_str, get_loc
from eq_read_log import get_loc

def get_aloc(f):
  ret = 0
  with open(f, 'r') as fh:
    for line in fh:
      ms = re.findall('^\.i', line, re.M)
      if len(ms) > 0:
        ret = ret + 1
  return ret - 8



def get_num_locals(f):
  ret = 0
  with open(f, 'r') as fh:
    for line in fh:
      ms = re.findall('^C_LOCAL', line, re.M)
      if len(ms) > 0:
        ret = ret + 1
  return ret

def get_num_stack_slots(f):
  ret = 0
  with open(f, 'r') as fh:
    for line in fh:
      ms = re.findall('^memlabel-mem-esp', line, re.M)
      if len(ms) > 0:
        ret = ret + 1
  return ret - 1

def main():
  utils.make_dir(eq_cache.cache_dir())

  parser = argparse.ArgumentParser()
  parser.add_argument("files", help = 'specify files as glob', nargs="+")

  args = parser.parse_args()

  files = args.files
  superopt_dir = os.path.expanduser('~') + '/superopt'

  for f in files:
    loc = get_aloc(f.replace("eqfiles-", "peeptabs-") + '.ptab')
    num_locals = get_num_locals(f + '.eq.llvm')
    num_stack_slots = get_num_stack_slots(f + '.eq')
    cmd = superopt_dir + '/build/eq-check-proof ' + f + '.eq ' + f + '.proof'
    print("eq_file: " + f + "\nALOC: " + str(loc) + "\nnum_locals: " + str(num_locals) + "\nnum_stack_slots: " + str(num_stack_slots))
    sys.stdout.flush()
    p = subprocess.Popen(cmd, shell=True)
    p.communicate()

if __name__ == "__main__":
  main()
