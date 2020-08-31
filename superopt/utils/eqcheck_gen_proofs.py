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

if __name__ == "__main__":
  parser = argparse.ArgumentParser()
  parser.add_argument('--prefix', help = "prefix to be prepended to each output command", type = str, default = 'python ' + srcdir + "/../grid/gridsubmit.py ", action = 'store')
  parser.add_argument('--batchsize', help = "number of eqfile generations per command (default 6)", type = int, default = 6, action = 'store')
  #parser.add_argument('-eq', '--eq', help = "whether to run eq", action='store_true')

  (args, eqfiles) = parser.parse_known_args()
  prefix = args.prefix
  batchsize = args.batchsize

  for i in range(0, len(eqfiles), batchsize):
    if prefix != "":
      sys.stdout.write(prefix + "\"")
    sys.stdout.write(srcdir + "/utils/eq_run.py "); # + p + " > " + elog + " 2>&1"
    cur_batchsize = min(len(eqfiles) - i, batchsize)
    for j in range(0, cur_batchsize):
      p = eqfiles[i + j]
      p = os.path.abspath(p)
      sys.stdout.write(p + " ")
    if prefix == "":
      sys.stdout.write("\n")
    else:
      sys.stdout.write("\"\n" + prefix + "\"")
