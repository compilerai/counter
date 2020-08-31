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
from eq_read_log import get_result_str, get_loc

def main():
  parser = argparse.ArgumentParser()
  parser.add_argument("cmd", help = 'harvest/eqgen/eq command to chaperon', type=str)
  parser.add_argument("--logfile", help = 'filename of logfile', type=str, default="default.log")
  args = parser.parse_args()
  cmd = args.cmd
  #cache_id = args.cache_id
  #st = datetime.datetime.now().strftime('--%Y-%m-%d-%H-%M-%S')
  #tmp_name = tmp_dir +'/'+ eq_cache.path_to_cache_id(f) + st
  #tmp_name = cache_dir +'/'+ cache_id# + st
  logfile = args.logfile #utils.add_ext(tmp_name, 'log')
  logfp = open(logfile, 'w')
  p = subprocess.Popen(cmd, stdout=logfp, stderr=logfp, shell=True)
  p.communicate()
  logfp.close()
  print(logfile + " " + get_result_str(logfile, True, ''))

if __name__ == "__main__":
  main()
