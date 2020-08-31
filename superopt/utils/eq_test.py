import glob
import sys
import argparse
import multiprocessing
import random

from eq_run import make_jobs_and_run
import eqcheck_test_db

def main():
  parser = argparse.ArgumentParser()
  parser.add_argument("fail_pass", help = "which benchmark", type = str, choices = ['passing', 'failing'])
  args = parser.parse_args()

  tests_pass = eqcheck_test_db.ctest_gcc_pass + eqcheck_test_db.ctest_clang_pass + \
               eqcheck_test_db.ctest_cg_pass + eqcheck_test_db.ctest_ccomp_pass + \
               eqcheck_test_db.ctest_icc_pass
  tests_fail = eqcheck_test_db.ctest_gcc_fail + eqcheck_test_db.ctest_clang_fail + \
               eqcheck_test_db.ctest_cg_fail + eqcheck_test_db.ctest_ccomp_fail + \
               eqcheck_test_db.ctest_icc_fail
  tests = []
  if args.fail_pass == 'passing':
    tests = tests_pass
  else:
    tests = tests_fail

  random.shuffle(tests)
  make_jobs_and_run(tests, 'log-files-test', multiprocessing.cpu_count(), True, False, '')

main()
