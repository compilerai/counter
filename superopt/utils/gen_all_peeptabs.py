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

smpbench_benches = ('ctests',);
spec2000_benches = (
   "mcf", "bzip2", "gzip", "parser", "crafty", "twolf", "gap", "perlbmk", "vortex", "vpr"
)
#spec2006_benches = ("sjeng",)
opts = ("llvm-bc",)

#for b in smpbench_benches:
#  print "removing eqchecker_harvest files for " + b
#  subprocess.call("rm -f " + srcdir + "/../smpbench-build/cint/" + b + "*eqchecker_harvest*", shell=True)
#  print "removing eqchecker_log files for " + b
#  subprocess.call("rm -f " + srcdir + "/../smpbench-build/cint/" + b + "*eqchecker_log*", shell=True)
#for b in spec2000_benches:
#  print "removing eqchecker_harvest files for " + b
#  subprocess.call("rm -f " + srcdir + "/../specrun-build/spec2000/*/" + b + "*eqchecker_harvest*", shell=True)
#  print "removing eqchecker_log files for " + b
#  subprocess.call("rm -f " + srcdir + "/../specrun-build/spec2000/*/" + b + "*eqchecker_log*", shell=True)
#for b in spec2006_benches:
#  print "removing eqchecker_harvest files for " + b
#  subprocess.call("rm -f " + srcdir + "/../specrun-build/spec2006/*/" + b + "*eqchecker_harvest*", shell=True)
#  print "removing eqchecker_log files for " + b
#  subprocess.call("rm -f " + srcdir + "/../specrun-build/spec2006/*/" + b + "*eqchecker_log*", shell=True)


#subprocess.call("rm -rf " + srcdir + "/../peeptabs-*", shell=True)
#cmd = "python " + srcdir + "/utils/eqcheck_gen_function_list.py --reuse-harvest-files " # --acyclic-only
cmd = "python " + srcdir + "/utils/eqcheck_gen_function_list.py " # --acyclic-only
#prefix = "--prefix " + srcdir + "/../grid/gridsubmit.py "
prefix = ""
cmd_prefix = cmd + prefix
for opt in opts:
  for b in smpbench_benches:
    subprocess.call(cmd_prefix + b + " smpbench " + opt, shell=True)
  for b in spec2000_benches:
    subprocess.call(cmd_prefix + b + " spec2000 " + opt, shell=True)
  #for b in spec2006_benches:
  #  subprocess.call(cmd_prefix + b + " spec2006 " + opt, shell=True)
  subprocess.call("cat " + srcdir + "/../peeptabs-*/*.functions > " + srcdir + "/../" + opt + ".functions", shell=True)
#subprocess.call("bash " + srcdir + "/../all.functions", shell=True)
