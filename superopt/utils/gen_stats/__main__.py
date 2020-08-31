#!/usr/bin/python3

import os
import glob
import sys
import traceback
import csv
import argparse
from pprint import pprint
from collections import namedtuple, defaultdict, OrderedDict

from eqlog_utils import *
from parse_proof_file import *
from parse_eqlog_file import *

def read_eqlogs(eqlogs, ignore_dups=False):
  all_fn_data = {}
  for a in eqlogs:
    with open(a, "r") as fin:
      try:
        fnStats = read_eqlog_stats_for_function(fin)
      except:
        print(traceback.format_exc())
        continue
      if fnStats.name in all_fn_data:
        print("Found mulitple .eqlog for '{}'".format(fnStats.name))
        if not ignore_dups:
            raise ValueError("Multiple eqlogs for function '{}' in {}".format(fnStats.name,eqlog_dir))
        else:
            print("--- Ignoring as asked ---")
            continue
      assert fnStats.name not in all_fn_data
      all_fn_data[fnStats.name] = fnStats
  return all_fn_data

def read_proofs(eqlog_dir):
  all_fn_data = {}
  for a in glob.glob(os.path.join(eqlog_dir, '*.proof')):
    with open(a, "r") as fin:
      # magic string heuristic
      try:
        proofStats = read_proof_file_stats_for_function(fin)
      except:
        print("{}: {}".format(a, traceback.format_exc()))
        continue
      if proofStats.name in all_fn_data:
        print("Found mulitple .proof for '{}'".format(proofStats.name))
        raise ValueError("Multiple proofs for function '{}' in {}".format(proofStats.name,eqlog_dir))
      assert proofStats.name not in all_fn_data
      all_fn_data[proofStats.name] = proofStats
  return all_fn_data

def get_relevant_fields_for_function(fnStats):
  fnName = fnStats.name
  status = fnStats.status
  counters = fnStats.counters
  timers = fnStats.timers
  path_stats = fnStats.path_stats
  cg_path_stats = fnStats.cg_path_stats

  ces_per_node = get_CEs_per_node_from_counters(counters)
  gen_ces_per_node = get_generated_CEs_per_node_from_counters(counters)

  ret = OrderedDict()

  ret['name'] = fnName
  ret['eq-passed'] = 1 if status else 0
  ret['dst-aloc'] = counters['dst-aloc']

  # CG
  ret['cg-edges'] = counters['# of edges in Product-CFG']
  ret['cg-pcs'] = counters['# of nodes in Product-CFG']
  # end-to-end time
  ret['total-equiv-secs'] = counters['total-equiv']
  # composite paths
  ret['paths-enumerated'] = counters['# of paths enumerated']
  ret['paths-expanded'] = counters['# of paths expanded']
  ret['paths-pruned'] = counters['# of paths enumerated'] - counters['# of Paths Prunned -- InvRelatesHeapAtEachNode'] - counters['# of Paths Prunned -- CEsSatisfyCorrelCriterion']

  ret['CE-generated'] = sum(get_values_with_key(counters, 'Counter-examples-Generated-at-'))
  ret['CE-total'] = sum(get_values_with_key(counters, 'Counter-examples-Total-at-'))

  # CE rejections
  #ret['CE-guided-implied-corr-reject'] = counters['CE-guided-implied-corr-reject']
  #ret['CE-guided-state-corr-reject'] = counters['CE-guided-state-corr-reject']

  #ret['removed-siblings'] = counters['removed-siblings']

  # correlations
  #ret['corr-attempted-implied-check'] = counters['corr-attempted-implied-check']

  # CEs
  #ret['median-total-CEs-per-node'] = median(ces_per_node)
  #ret['avg-total-CEs-per-node'] = avg(ces_per_node)
  #ret['max-total-CEs-per-node'] = mmax(ces_per_node)

  #ret['median-gen-CEs-per-node'] = median(gen_ces_per_node)
  #ret['avg-gen-CEs-per-node'] = avg(gen_ces_per_node)
  #ret['max-gen-CEs-per-node'] = mmax(gen_ces_per_node)

  #ret['find-correlation'] = timers['find_correlation'].time
  # smt solver
  #ret['decide_hoare_triple-time-secs'] = timers['decide_hoare_triple_helper'].time
  #ret['smt-query-time-secs'] = timers['query:smt'].time
  # linear solver
  ret['linear-solver-queries'] = counters['# of queries to linear solver']

  # solver calls
  ret['smt-queries'] = counters['# of smt-solver-queries']
  #ret['decide_hoare_triple-calls'] = timers['decide_hoare_triple_helper'].starts
  #ret['queries-decided-trivially'] = timers['prove_after_enumerating_local_sprel_expr_guesses'].starts - timers['prove_after_enumerating_local_sprel_expr_guesses.proof_query_not_trivial'].starts
  #ret['smt-query-time-secs'] = timers['query:smt-miss total'].time
  #ret['smt-query-calls'] = timers['query:smt-miss total'].starts

  ## checking time
  #ret['check-equivalence-proof'] = timers['check_equivalence_proof'].time
  #ret['check-safety'] = timers['check_safety'].time
  #ret['check-dst-mls'] = timers['check_dst_mls'].time

  #ret['time-taken-for-path-enumeration'] = timers['get_unrolled_paths_from'].time

  # BV_solve
  #ret['max-bv_solve-matrix-row'] = counters['max-bv_solve-matrix-row']
  #ret['max-bv_solve-matrix-col'] = counters['max-bv_solve-matrix-col']
  #ret['avg-bv_solve-matrix-rows'] = counters['avg-bv_solve-matrix-rows']
  #ret['avg-bv_solve-matrix-cols'] = counters['avg-bv_solve-matrix-cols'] # TODO fix this
  
  # path pruning
  #ret['time-taken-for-pruning-secs'] = timers['semantically_simplify_tfg_edge_composition'].time
  ##ret['solver-time-taken-for-pruning-secs'] = timers['path_represented_by_tfg_edge_composition_is_not_feasible'].time
  ### paths
  #ret['avg-number-of-paths-before-pruning'] = avg([ps.paths for ps in path_stats.unpruned])
  #ret['avg-number-of-paths-after-pruning'] = avg([ps.paths for ps in path_stats.pruned])
  ### edges and sop-edges
  #ret['avg-edges-before-pruning'] = avg([ps.edges for ps in path_stats.unpruned])
  #ret['avg-sop-edges-before-pruning'] = avg([ps.sop_edges for ps in path_stats.unpruned])

  #ret['avg-edges-after-pruning'] = avg([ps.edges for ps in path_stats.pruned])
  #ret['avg-sop-edges-after-pruning'] = avg([ps.sop_edges for ps in path_stats.pruned])

  #ret['median-edges-before-pruning'] = median([ps.edges for ps in path_stats.unpruned])
  #ret['median-sop-edges-before-pruning'] = median([ps.sop_edges for ps in path_stats.unpruned])

  #ret['median-edges-after-pruning'] = median([ps.edges for ps in path_stats.pruned])
  #ret['median-sop-edges-after-pruning'] = median([ps.sop_edges for ps in path_stats.pruned])

  #ret['max-edges-before-pruning'] = mmax([ps.edges for ps in path_stats.unpruned])
  #ret['max-sop-edges-before-pruning'] = mmax([ps.sop_edges for ps in path_stats.unpruned])

  ## compression ratio
  #ret['avg-composite-path-compression-ratio'] = avg([ps.sop_edges / ps.edges for ps in path_stats.unpruned])
  #ret['median-composite-path-compression-ratio'] = median([ps.sop_edges / ps.edges for ps in path_stats.unpruned])
  #ret['max-composite-path-compression-ratio'] = mmax([ps.sop_edges / ps.edges for ps in path_stats.unpruned])

  #ret['max-max-path-length-before-pruning'] = mmax([ps.max_path_length for ps in path_stats.unpruned])
  #ret['max-max-path-length-after-pruning'] = mmax([ps.max_path_length for ps in path_stats.pruned])
  #ret['avg-max-path-length-before-pruning'] = avg([ps.max_path_length for ps in path_stats.unpruned])
  #ret['avg-max-path-length-after-pruning'] = avg([ps.max_path_length for ps in path_stats.pruned])

  # timeouts
  #ret['timeouts'] = counters['query-decomposition-invoked']
  #ret['qdecomp-timed-out'] = counters['query-decomposition-timed-out']

  # CG characteristics
  #ret['avg-number-of-src-dst-sop-edges-in-cg-edge'] = avg([cge[0].sop_edges+cge[1].sop_edges for cge in zip(cg_path_stats.src,cg_path_stats.dst)])
  #ret['max-number-of-src-dst-sop-edges-in-cg-edges'] = mmax([cge[0].sop_edges+cge[1].sop_edges for cge in zip(cg_path_stats.src,cg_path_stats.dst)])
  #ret['median-number-of-src-dst-sop-edges-in-cg-edges'] = median([cge[0].sop_edges+cge[1].sop_edges for cge in zip(cg_path_stats.src,cg_path_stats.dst)])

  #ret['max-number-of-paths-in-cg-edge'] = mmax([max(cge[0].paths,cge[1].paths) for cge in zip(cg_path_stats.src,cg_path_stats.dst)])
  #ret['max-path-length-in-cg-edge'] = mmax([max(cge[0].max_path_length,cge[1].max_path_length) for cge in zip(cg_path_stats.src,cg_path_stats.dst)])

  #ret['query-paths'] = counters['query-paths']
  #ret['query-preds'] = counters['query-preds']

  return ret

def get_relevant_data_from_fnstats(all_fnstats):
  return { name : get_relevant_fields_for_function(fnStats) for name,fnStats in all_fnstats.items() }

def get_relevant_fields_for_proof(proofStats):
  ret = OrderedDict()
  ret['CG-nodes'] = proofStats.nodes
  ret['CG-edges'] = proofStats.edges
  ret['total-invariants'] = sum(proofStats.invariants_per_node)
  ret['avg-invariants_per_node'] = avg(proofStats.invariants_per_node)

  return ret

def get_relevant_data_from_proofstats(all_proofstats):
  return { name : get_relevant_fields_for_proof(proofStats) for name,proofStats in all_proofstats.items() }

def merge_fn_data(all_fn_data_eqlogs, all_fn_data_proofs):
  ret = {}
  for fnName, eqlog_data in all_fn_data_eqlogs.items():
    if fnName not in all_fn_data_proofs:
      print("Skipping {} as .proof not collected".format(fnName))
      continue
    assert fnName in all_fn_data_proofs
    proof_data = all_fn_data_proofs[fnName]
    assert set(eqlog_data.keys()).isdisjoint(set(proof_data.keys()))
    ret[fnName] = OrderedDict()
    for k,v in eqlog_data.items():
        ret[fnName][k] = v
    for k,v in proof_data.items():
        ret[fnName][k] = v
  return ret

def read_harvest_aloc_data(harvest_aloc):
  ret = {}
  with open(harvest_aloc, 'r') as fin:
    for l in fin:
      fnName, aloc = l.split(':')
      aloc_int = int(aloc)
      ret[fnName] = aloc_int
  return ret

def add_aloc_to_all_fn_data(all_fn_data, all_fn_aloc):
  for fnName, data in all_fn_data.items():
    if fnName not in all_fn_aloc:
      print("ALOC data not found for {}.  Setting to -1.".format(fnName))
      aloc = -1
    else:
      assert fnName in all_fn_aloc
      aloc = all_fn_aloc[fnName]
    data['aloc'] = aloc

def topKtimers(timers, k):
  ret = [name,timer in timers.items()]
  ret.sort(key=lambda n,t: t.time)
  return ret[:k]

def get_relevant_data_from_eqlog_files(eqlog_files, eqlog_dir=None, ignore_dups=False, read_proofs=False):
  all_fnstats = read_eqlogs(eqlog_files, ignore_dups=ignore_dups)
  all_fn_data_eqlogs = get_relevant_data_from_fnstats(all_fnstats)
  if read_proofs and eqlog_dir is not None:
    all_proofstats = read_proofs(eqlog_dir)
    all_fn_data_proofs = get_relevant_data_from_proofstats(all_proofstats)
    all_fn_data = merge_fn_data(all_fn_data_eqlogs, all_fn_data_proofs)
  else:
    all_fn_data = all_fn_data_eqlogs
  return [data for fnName,data in all_fn_data.items()]

if __name__ == '__main__':
  parser = argparse.ArgumentParser()
  parser.add_argument("-read_proofs", help = "Read data from .proof files", action='store_true')
  parser.add_argument("-ignore_dups", help = "ignore duplicates (only one is retained)", action='store_true')
  parser.add_argument("-o", "--output", help = 'output csv file name', type=str, default='out.csv')
  #parser.add_argument("-s", "--summary", help = 'generate summary of data', action='store_true')
  group = parser.add_mutually_exclusive_group(required=True)
  group.add_argument("-d", '--dir', help = 'directory where .proof, .eqlog and .stats files are stored', type=str)
  group.add_argument("-f", '--files',  help = '.eqlog files', nargs='+', type=str)

  args = parser.parse_args()

  eqlog_dir = None
  if args.dir is not None:
      eqlog_dir = os.path.realpath(args.dir)
      eqlog_files = glob.glob(os.path.join(eqlog_dir, '*.eqlog'))
  elif args.files is not None:
      eqlog_files = args.files
  else:
      assert(False)

  all_fn_data = get_relevant_data_from_eqlog_files(eqlog_files, eqlog_dir, ignore_dups=args.ignore_dups, read_proofs=args.read_proofs)
  if len(all_fn_data) > 0:
    if 'dst-aloc' in all_fn_data[0]:
       all_fn_data.sort(key=lambda row: row['dst-aloc']) # sort by ALOC
    filename = args.output
    with open(filename, 'w', newline='') as csvfile:
      fieldnames = all_fn_data[0].keys() # get keys of inner dict
      writer = csv.DictWriter(csvfile, fieldnames=fieldnames)

      writer.writeheader()
      for data in all_fn_data:
        writer.writerow(data)
 
    print("Finished! {} record(s) written to '{}'".format(len(all_fn_data), filename))
  else:
    print("Finished! 0 records found.")

