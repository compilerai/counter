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
from functools import partial

from eq_read_log import get_result_str, get_pass_status_from_multiple_log_files, get_loc

def cache_dir():
  return "cache-results-osdi2018"

def path_to_cache_id(f):
  return f.replace('/', '--')

def add_log(f, logfile):
  base_f = os.path.splitext(f)[0]
  base_cache_f = path_to_cache_id(base_f)
  st = datetime.datetime.now().strftime('--%Y-%m-%d-%H-%M-%S')
  for ext in ['log', 'logpy', 'proof']:
    if os.path.exists(utils.change_ext(logfile, ext)):
      shutil.copyfile(utils.change_ext(logfile, ext), cache_dir() + '/' + base_cache_f + st + '.' + ext)
  for ext in ['.ptab', '.eq']:
    if os.path.exists(base_f + ext):
      shutil.copyfile(base_f + ext, cache_dir() + '/' + base_cache_f + st + ext)

def get_files_from_cache(f, ext):
  return glob.glob(cache_dir() + '/' + path_to_cache_id(utils.remove_ext(f)) + '--*.' + ext)

def get_bench_and_type(peeptab):
  lst = peeptab.split('/')
  if len(lst) == 3:
    bench = lst[0].split('-')[1]
    comp = lst[1]
    func = lst[2].split('-')[0]
    loop = lst[2].split('-')[1]
    return (bench, comp, func, loop)
  else:
    return ("", "", lst[len(lst)-1], "")

def get_pass_status(f):
  files = get_files_from_cache(f, '*')
  files = utils.remove_ext_in_list(files)
  files = utils.de_dup(files)
  assert(len(files) > 0)
  pass_status = get_pass_status_from_multiple_log_files(files)
  return pass_status

def is_cached(f):
  files = get_files_from_cache(f, '*')
  return len(files) > 0

def is_cached_and_passing(f):
  if not is_cached(f):
    return False
  pass_status = get_pass_status(f)
  #print "pass_status = " + pass_status
  return pass_status.startswith('passed') or pass_status.startswith('FAILED: ERROR_contains_float_op') or pass_status.startswith('FAILED: ERROR_contains_unsupported_op')

