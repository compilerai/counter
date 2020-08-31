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
  parser.add_argument("logfile", help = '.eqlog file', type=str)
  args = parser.parse_args()
  logfile = args.logfile
  print((logfile + " " + get_result_str(logfile, True, '')))

if __name__ == "__main__":
  main()
