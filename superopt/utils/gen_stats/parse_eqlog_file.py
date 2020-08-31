#!/usr/bin/python3

# parse-eqlog-file.py -- parse .eqlog file and extract useful info.

import os
import glob
import sys
import traceback
import csv
import argparse
import re
from pprint import pprint
from collections import namedtuple, defaultdict

from eqlog_utils import *
from parse_proof_file import *

PathStats = namedtuple('PathStats', ['paths', 'edges', 'sop_edges', 'max_path_length'])
AllPathStats = namedtuple('AllPathStats', ['pruned', 'unpruned'])
CGPathStats = namedtuple('CGPathStats', ['src', 'dst'])
Timer = namedtuple('Timer', ['time', 'starts'])
FnStats = namedtuple('FunctionStats', ['name', 'status', 'timers', 'counters', 'path_stats', 'cg_path_stats'])

def empty_timers():
  return defaultdict(lambda: Timer(float('NaN'),float('NaN')))

def empty_counters():
  return defaultdict(lambda: 0)

def empty_all_path_stats():
  return AllPathStats(pruned=[], unpruned=[])

def empty_cg_path_stats():
  return CGPathStats(src=[], dst=[])

# returns a map 'name' -> ('time','starts') of timers
def read_timers(fin):
  ret = empty_timers()
  for l in fin:
    l = l.strip()
    if l == "":
      break
    key, val = l.split('.:')
    time, count = parse_time_num_starts(val)
    assert key not in ret
    ret[key] = Timer(time, count)
  else:
    raise ValueError("{}: timers are not well-formed".format(fin.name))
  return ret

# returns a dict 'name' -> 'value' of counters
def read_counters(fin, ret=None):
  if ret is None:
      ret = empty_counters()
  for l in fin:
    l = l.strip()
    if l == "":
      break
    key, val = l.split('.:')
    try:
      val = int(val)
    except:
      val = float(val)
    if key in ret:
        print("Multiple {} keys in counter".format(key))
        print("Counters: {}".format(ret))
    assert key not in ret
    ret[key] = val
  else:
    raise ValueError("{}: counters are not well-formed".format(fin.name))
  return ret

def parse_path_stats(s):
  # example input:
  # <fn>:stats; path_count = 1; edge_count = 1; sop_edge_count = 1; max_path_length = 1
  try:
    _, paths_str, edges_str, sop_edges_str, max_path_length_str = s.split(';')
    paths     = int(paths_str.split('=')[1].strip())
    edges     = int(edges_str.split('=')[1].strip())
    sop_edges = int(sop_edges_str.split('=')[1].strip())
    max_path_length = int(max_path_length_str.split('=')[1].strip())
  except:
    _, paths_str, edges_str, sop_edges_str, max_path_length_str, min_path_length_str = s.split(';')
    paths     = int(paths_str.split('=')[1].strip())
    edges     = int(edges_str.split('=')[1].strip())
    sop_edges = int(sop_edges_str.split('=')[1].strip())
    max_path_length = int(max_path_length_str.split('=')[1].strip())
    min_path_length = int(min_path_length_str.split('=')[1].strip()) # ignored lol
  return PathStats(paths, edges, sop_edges, max_path_length)

def read_cgpath_stats(fin, search_for_dump_start=True):
  # collect statistics for all CG edges

  # find beginning of dump
  if search_for_dump_start:
    try:
      find_line_starting_with(fin, "check_eq: Dumping CG path stats", "CG path stats")
    except:
      return empty_cg_path_stats()

  # we found the dump start reading it
  src_ec = True
  src_ec_stats = []
  dst_ec_stats = []
  for line in fin:
    if line.startswith("check_eq: Finished CG path stats dump"):
      break
    if line.startswith("check_eq:stats;"):
      pthStats = parse_path_stats(line)
      if src_ec:
        src_ec_stats.append(pthStats)
      else:
        dst_ec_stats.append(pthStats)
      src_ec = not src_ec
  else:
    raise ValueError("{}: CG path stats are not well-formed".format(fin.name))
  if len(src_ec_stats) != len(dst_ec_stats):
    raise ValueError("{}: CG path stats are not well-formed".format(fin.name))
  assert len(src_ec_stats) == len(dst_ec_stats)
  return CGPathStats(src=src_ec_stats, dst=dst_ec_stats)

def get_paths_stats(fin, stop_pfx, msg):
  # collect statistics for all src ec:
  # number of paths, number of edges in SP, number of edges in SOP, max path length
  # for both unpruned and pruned ECs
  pruned = False
  pruned_paths = []
  unpruned_paths = []
  for line in fin:
    if stop_pfx.match(line):
      ret_line = line
      break
    if line.startswith("semantically_simplify_tfg_edge_composition:stats;"):
      pthStats = parse_path_stats(line)
      if pruned:
        pruned_paths.append(pthStats)
      else:
        unpruned_paths.append(pthStats)
      pruned = not pruned
  else:
    raise ValueError("{}: {} not found".format(fin.name, msg))
  if len(unpruned_paths) != len(pruned_paths):
    raise ValueError("{}: path stats are not well-formed! pruned_paths = {}, pruned_paths = {}".format(fin.name,len(pruned_paths),len(unpruned_paths)))
  assert len(unpruned_paths) == len(pruned_paths)
  return ret_line,AllPathStats(unpruned=unpruned_paths, pruned=pruned_paths)

def read_eqlog_stats_for_function(fin):
  # try to get the function name
  line = find_line_starting_with(fin, "MSG: Computing equivalence for function:", "function name")
  fnName = line.split(':')[2].strip()

  try:
      line = find_line_starting_with_regex(fin, re.compile("(Constructed Product-CFG proves Bisimulation|check_eq \d*: correlation check failed)"), "End of eqcheck")
  except Exception as e:
      # incomplete log; either still running or terminated prematurely
      return FnStats(fnName, False, empty_timers(), empty_counters(), empty_all_path_stats(), empty_cg_path_stats())
  if "correlation check failed" in line:
      status = False
      cg_path_stats = empty_cg_path_stats()
  else:
      status = True
      cg_path_stats = empty_cg_path_stats() # not available :/

  #find_line_starting_with(fin, "timers:", "timers")
  #timers = read_timers(fin)
  find_line_starting_with(fin, "counters:", "counters")
  counters = read_counters(fin)
  find_line_starting_with(fin, "flags:", "flags")
  counters = read_counters(fin, counters)

  # look for total-equiv time
  line = find_line_starting_with(fin, "total-equiv.", "total-equiv time")
  total_equiv_time = float(line.split('.:')[1].strip().split(' ')[0][:-1])
  counters['total-equiv'] = total_equiv_time

  return FnStats(fnName, status, empty_timers(), counters, empty_all_path_stats(), cg_path_stats)

def get_CEs_per_node_from_counters(counters):
  ces_per_node = []
  for k,v in counters.items():
    if k.startswith('total-counter-examples-at-'):
      ces_per_node.append(v)
  return ces_per_node

def get_generated_CEs_per_node_from_counters(counters):
  gen_ces_per_node = []
  for k,v in counters.items():
    if k.startswith('generated-added-counter-examples-at-'):
      gen_ces_per_node.append(v)
  return gen_ces_per_node
