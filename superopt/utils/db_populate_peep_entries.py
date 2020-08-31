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
import re
from functools import partial

def main():
  home = os.path.expanduser('~')
  db_pick_enum_candidate = home + '/superopt/build/etfg_i386/db_pick_enum_candidate'
  make_itables = home + '/superopt/build/etfg_i386/enum_candidate_make_itables'
  db_populate_itables_for_enum_candidate = home + '/superopt/build/etfg_i386/db_populate_itables_for_enum_candidate'
  db_pick_itable_for_enum_candidate = home + '/superopt/build/etfg_i386/db_pick_itable_for_enum_candidate'
  gen_rand_states = home + '/superopt/build/etfg_i386/src_iseq_gen_rand_states'
  compute_typestates = home + '/superopt/build/etfg_i386/src_iseq_compute_typestate'
  enumerate_for_src_iseq_using_itable = home + '/superopt/build/etfg_i386/enumerate_for_src_iseq_using_itable'
  db_update_itable_enum_state = home + '/superopt/build/etfg_i386/db_update_itable_enum_state_for_enum_candidate'
  db_add_peep_entry = home + '/superopt/build/etfg_i386/db_add_peep_entry'
  enum_candidate_file = home + "/out.enum_candidate"
  itables_file = home + "/out.itables"
  itable_file = home + "/out.itable"
  peep_entry_file = home + "/out.equiv"
  ipos_file = home + "/out.ipos"
  rand_states_file = home + "/out.rand_states"
  typestates_file = home + "/out.typestates"
  db_pick_enum_candidate_cmd = db_pick_enum_candidate + " > " + enum_candidate_file
  make_itables_cmd = make_itables + " " + enum_candidate_file + " > " + itables_file
  db_populate_itables_for_enum_candidate_cmd = db_populate_itables_for_enum_candidate + " " + enum_candidate_file + " " + itables_file
  db_pick_itable_for_enum_candidate_cmd = db_pick_itable_for_enum_candidate + " " + enum_candidate_file + " > " + itable_file
  gen_rand_states_cmd = gen_rand_states + " " + enum_candidate_file + " > " + rand_states_file
  compute_typestates_cmd = compute_typestates + " " + enum_candidate_file + " " + rand_states_file + " > " + typestates_file
  enumerate_for_src_iseq_using_itable_cmd = enumerate_for_src_iseq_using_itable + " " + enum_candidate_file + " " + rand_states_file + " " + typestates_file + " " + itable_file + " -o " + peep_entry_file + " -ipos_o " + ipos_file
  db_update_itable_enum_state_cmd = db_update_itable_enum_state + " " + enum_candidate_file + " " + itable_file + " " + ipos_file
  db_add_peep_entry_cmd = db_add_peep_entry + " " + peep_entry_file
  while (True):
    subprocess.check_output(db_pick_enum_candidate_cmd, shell=True)
    subprocess.check_output(make_itables_cmd, shell=True)
    subprocess.check_output(db_populate_itables_for_enum_candidate_cmd, shell=True)
    subprocess.check_output(db_pick_itable_for_enum_candidate_cmd, shell=True)
    subprocess.check_output(gen_rand_states_cmd, shell=True)
    subprocess.check_output(compute_typestates_cmd, shell=True)
    subprocess.check_output(enumerate_for_src_iseq_using_itable_cmd, shell=True)
    subprocess.check_output(db_update_itable_enum_state_cmd, shell=True)
    subprocess.check_output(db_add_peep_entry_cmd, shell=True)
    print("Finished one iteration of adding peep_entry\n");


if __name__ == "__main__":
  main()
