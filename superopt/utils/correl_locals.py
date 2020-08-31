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

class job:
  def __init__(self, cmd, index, total_jobs, name, tmp_dir, is_job_to_run):
    self.cmd = cmd
    self.index = index
    self.total_jobs = total_jobs
    self.name = name
    self.tmp_dir = tmp_dir
    self.is_job_to_run = is_job_to_run

  def get_result(self, f):
    with open(f + ".log", 'r') as fh:
      text = fh.read()
    configs = ["", "assume_correct", "assume_incorrect", "try_all_possibilities", "consider_all_local_variables"]
    time = {}
    result = {}
    num_locals = {}
    num_sprel_exprs = {}
    for config in configs:
      ms = re.findall('\ntotal-equiv.' + config + ': (.+?)s.*$', text, re.M)
      if len(ms) > 0:
        time[config] = round(float(ms[len(ms) - 1]), 2)
      else:
        time[config] = 0
      ms = re.findall('\neq.found.' + config + ':', text, re.M);
      if len(ms) > 0:
        result[config] = True
      else:
        result[config] = False
      ms = re.findall('\ncorrel_locals.num_locals.' + config + ': (.+?)$', text, re.M)
      if len(ms) > 0:
        num_locals[config] = int(ms[len(ms) - 1])
      else:
        num_locals[config] = -1
      ms = re.findall('\ncorrel_locals.num_sprel_exprs.' + config + ': (.+?)$', text, re.M)
      if len(ms) > 0:
        num_sprel_exprs[config] = int(ms[len(ms) - 1])
      else:
        num_sprel_exprs[config] = -1
    s = "[ "
    for config in configs:
      s = s + utils.formatted(str(num_locals[config]) + ':' + str(num_sprel_exprs[config]) + ':' + str(time[config]), result[config], True) + ", "
    s = s + " ]"
    return s

def run_job(j):
  if not j.is_job_to_run(j.name):
    print('Ignoring: {}'.format(j.name))
    return
  cmd = j.cmd
  f = j.name
  st = datetime.datetime.now().strftime('--%Y-%m-%d-%H-%M-%S')
  tmp_name = j.tmp_dir +'/'+ eq_cache.path_to_cache_id(f) + st
  logfile = utils.add_ext(tmp_name, 'log')
  fi = open(logfile, 'w')
  sys.stdout.write('Running {}/{} {} '.format(j.index, j.total_jobs, f))
  sys.stdout.flush();
  p = subprocess.Popen(cmd, stdout=fi, stderr=fi, shell=True)
  p.communicate()
  fi.close()
  status = j.get_result(tmp_name)
  eq_cache.add_log(f, tmp_name)
  #print j.name + ' ' + status
  print(' ' + status)

def run_jobs(jobs, cores):
  if (len(jobs) == 1):
    run_job(jobs[0])
  else:
    multiprocessing.Pool(cores).map(run_job, jobs, chunksize=1)

def make_jobs(files, tmp_dir, is_job_to_run, global_timeout, query_timeout):
  jobs = []
  llvm2tfg = os.path.expanduser('~') + '/llvm2tfg-build/bin/llvm2tfg'
  superopt_dir = os.path.expanduser('~') + '/superopt'
  eqgen = os.path.expanduser('~') + '/superopt/build/eqgen'
  eq = os.path.expanduser('~') + '/superopt/build/eq'
  index = 1
  max_address_space_size = 64*1024*1024*1024 #64GB
  for f in files:
    #print f + "\n"
    regex = re.compile(r'peeptabs-([^\/]+)/([^\/]+)/([^\/]+)-[AC]')
    prog = regex.search(f).group(1)
    compiler = regex.search(f).group(2)
    func_name = regex.search(f).group(3)
    if prog == 'ctests':
      llvm_exec = os.path.expanduser('~') + '/smpbench-build/cint/' + prog + '.bc.O0'
    else:
      llvm_exec = os.path.expanduser('~') + '/spec2000/llvm-bc-O0/' + prog
    ptab_file = utils.add_ext(f, 'ptab')
    f_eq = f.replace("peeptabs-", "eqfiles-")
    eq_file = utils.add_ext(f_eq, 'eq')
    eqllvm_file = utils.add_ext(f_eq, 'eq.llvm')
    proof_file = utils.add_ext(f_eq, 'proof')
    #cmd.append('--proof')
    #cmd.append(proof_file)
    llvm2tfg_cmd = llvm2tfg + " -f " + func_name + " " + llvm_exec + " -o " + eqllvm_file
    eqgen_cmd = eqgen + " -tfg_llvm " + eqllvm_file + " " + ptab_file
    eq_cmd_common = eq + " " + eq_file + " --global-timeout " + str(global_timeout) + " --query-timeout " + str(query_timeout) + " --max-address-space-size " + str(max_address_space_size) + " --disable_remove_fcall_side_effects"
    eq_cmd_plain = eq_cmd_common + " --proof " + proof_file 
    eq_cmd_correct_assumes = eq_cmd_common + " --proof " + proof_file + ".assume.correct" + " --assume " + superopt_dir + "/utils/correl_locals.assumes.correct." + func_name + " --disable_correl_locals --result_suffix assume_correct"
    eq_cmd_incorrect_assumes = eq_cmd_common + " --proof " + proof_file + ".assume.incorrect" + " --disable_correl_locals --result_suffix assume_incorrect"
    eq_cmd_try_all_possibilities = eq_cmd_common + " --proof " + proof_file + ".try_all_possibilities" + " --correl_locals_try_all_possibilities --result_suffix try_all_possibilities"
    eq_cmd_consider_all_local_variables = eq_cmd_common + " --proof " + proof_file + ".consider_all_local_variables" + " --correl_locals_try_all_possibilities --correl_locals_consider_all_local_variables --result_suffix consider_all_local_variables"
    eq_cmd = eq_cmd_plain + " && " + eq_cmd_correct_assumes + " && " + eq_cmd_incorrect_assumes + " && " + eq_cmd_try_all_possibilities + " && " + eq_cmd_consider_all_local_variables
    cmd = llvm2tfg_cmd + " && " + eqgen_cmd + " && " + eq_cmd
    jobs.append(job(cmd, index, len(files), f, tmp_dir, is_job_to_run))
    index += 1
  return jobs

def peeptab_name(bench, function):
  return "peeptabs-" + bench + "/gcc48-O2-i386/" + function

def is_job_to_run(f):
  return True

# this function is used in eqchech_test as well
# either use optional args or if signature of this function is changed then fix the calls at both places
def make_jobs_and_run(files, tmp_dir, cores, global_timeout, query_timeout):
  signal.signal(signal.SIGTERM, kill_pool)
  signal.signal(signal.SIGINT, kill_pool)
  signal.signal(signal.SIGQUIT, kill_pool)

  #is_job_to_run_arg = partial(is_job_to_run)
  jobs = make_jobs(files, tmp_dir, is_job_to_run, global_timeout, query_timeout)
  run_jobs(jobs, cores)

def cmp_eq_files(j1, j2):
  #return cmp(os.path.getsize(j1 + '.eq'), os.path.getsize(j2 + '.eq'))
  #p1 = j1.replace("eqfiles", "peeptabs")
  #p2 = j2.replace("eqfiles", "peeptabs")
  #f1 = utils.change_ext(p1, 'ptab')
  #f2 = utils.change_ext(p2, 'ptab')
  #return cmp(get_loc(f1), get_loc(f2))
  return cmp(get_loc(j1 + '.ptab'), get_loc(j2 + '.ptab'))

def kill_pool(signum, frame):
  #sys.exit(0)
  assert(False)

def main():
  utils.make_dir(eq_cache.cache_dir())

  parser = argparse.ArgumentParser()
  parser.add_argument("files", help = 'specify files as glob', nargs="+")
  parser.add_argument("--num-jobs", help='num of jobs for multiprocessing, default is max-cores avaiulable', type=int, default=multiprocessing.cpu_count())
  parser.add_argument("--global-timeout", help='global timeout in seconds', type=int, default=3600)
  parser.add_argument("--query-timeout", help='query timeout in seconds', type=int, default=600)
  parser.add_argument("--tmp-dir", help='tmp-dir for log files before moving to cahce', type=str, default='tmp')

  args = parser.parse_args()

  files = utils.merge_multiple_globs(args.files)
  files = utils.filter_files_with_ext(files, 'ptab')
  files = utils.remove_ext_in_list(files)

  files.sort(cmp_eq_files)
  files2 = []
  for f in files:
    if is_job_to_run(f):
      files2.append(f)
  print('Ignored: {} files'.format(len(files)-len(files2))) 
  files = files2

  utils.make_dir(args.tmp_dir)
  make_jobs_and_run(files, args.tmp_dir, args.num_jobs, args.global_timeout, args.query_timeout)

if __name__ == "__main__":
  main()
