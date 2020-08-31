#!/usr/bin/python

import os
import subprocess
from subprocess import PIPE
from multiprocessing import Pool
import sys
import glob
import subprocess
import datetime
import argparse
import shutil
import multiprocessing
import pprint
import random
import signal
import utils

def run_command(cmd):
  #print str(cmd[1]) + 'start'
  proc = subprocess.Popen(cmd, stdout=PIPE, stderr=PIPE)
  out = proc.stdout.read()
  passed = 'eq-proof-valid:' in out
  if passed:
    print(cmd[1] + ' ' + utils.formatted('pass', passed, True))
  else:
    print(cmd[1] + ' ' + utils.formatted('fail', passed, True))

def run_commands(cmds):
  pool = Pool()
  Pool().map(run_command, cmds, chunksize=1)

def make_commands(files):
  cmds = []
  eq_check_proof = 'build/eq-check-proof'
  for f in files:
    ext = os.path.splitext(f)[1] 
    filename = os.path.splitext(f)[0] 
    proof_file = os.path.splitext(f)[0] + '.proof'
    if ext == '.eq' and proof_file in files:
      cmds.append([eq_check_proof, f, proof_file])
  print('total jobs: ' + str(len(cmds)))
  return cmds

def kill_pool(signum, frame):
  assert(False)

def main():
  signal.signal(signal.SIGTERM, kill_pool)
  signal.signal(signal.SIGINT, kill_pool)
  signal.signal(signal.SIGQUIT, kill_pool)

  parser = argparse.ArgumentParser()
  parser.add_argument("files", help = 'specify files as glob')
  args = parser.parse_args()

  files = glob.glob(args.files)

  run_commands(make_commands(files))

if __name__ == "__main__":
  main()

