#!/usr/bin/python
import sys
import subprocess
import re
import os
import os.path
import argparse
import shutil
import glob
import random
from os.path import getsize

def exists_eqfile_for_ptab(ptab):
  eqfile = ptab
  eqfile = eqfile.replace("peeptabs", "eqfiles")
  eqfile = eqfile.replace("ptab", "eq")
  return os.path.isfile(eqfile)

missing_eqfiles = []

if __name__ == "__main__":
  for ptab in sys.argv[1:]:
    if not exists_eqfile_for_ptab(ptab):
      missing_eqfiles.append(ptab)

missing_eqfiles_sorted = sorted(missing_eqfiles, key=getsize)
for ptab in missing_eqfiles_sorted:
  print(ptab)
