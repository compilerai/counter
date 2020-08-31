#!/usr/bin/python3

# parse-proof-file.py -- parse .proof file and extract useful info.
#
# Input: .proof files
# Output: Self-evident

import sys
import traceback
from collections import namedtuple, defaultdict

from eqlog_utils import *

ProofStats = namedtuple('ProofStats', ['name', 'nodes', 'edges', 'invariants_per_node'])

def read_edges(fin, stop_pfx):
  correlated_edges = 0
  for l in fin:
     if l.startswith(stop_pfx):
       return l,correlated_edges
     correlated_edges += 1
  else:
    raise ValueError("Ill-formed edges list")

def read_invariants_per_node(fin):
  correlations_per_node = []
  for l in fin:
    if l.startswith("=Node"):
      pred_count = int(l.split(':')[1].lstrip().split(' ')[0])
      correlations_per_node.append(pred_count)
  return "", correlations_per_node

def read_proof_file_stats_for_function(fin):
  line = find_line_starting_with(fin, "=FunctionName:", "function name")
  fnName = line.split(' ')[1].strip()

  # check and skip result boolean
  line = find_line_starting_with(fin, "=result", "=result")
  result_val = int(line.split(":")[1])
  if result_val == 0:
      return ProofStats(fnName, nodes=[], edges=[], invariants_per_node=[])

  # check and skip src_tfg
  find_line_starting_with(fin, "=src_tfg", "SRC TFG")
  find_line_starting_with(fin, "=TFGdone", "SRC TFG done")

  # check and skip DstTfg
  find_line_starting_with(fin, "=dst_tfg", "DST TFG")
  find_line_starting_with(fin, "=TFGdone", "DST TFG done")

  # find beginning of nodes list
  line = find_line_starting_with(fin, "=Nodes:", "CG nodes")
  # count all nodes
  correlated_nodes = len(line.split(" "))-1
  assert correlated_nodes > 0

  # find beginning of edges list
  line = find_line_starting_with(fin, "=Edges:", "CG edges")
  _,correlated_edges = read_edges(fin, stop_pfx="=graph done")
  assert correlated_edges > 0

  #line,correlations_per_node = read_invariants_per_node(fin)
  #assert len(correlations_per_node) == correlated_nodes
  #assert line == "" # finished
  correlations_per_node = [] # FIXME

  return ProofStats(name=fnName, nodes=correlated_nodes, edges=correlated_edges, invariants_per_node=correlations_per_node)

#if __name__ == '__main__':
#  for a in sys.argv[1:]:
#    with open(a, "r") as fin:
#      # magic string heuristic
#      try:
#        proofStats = read_proof_file_stats_for_function(fin)
#      except:
#        print("{}: {}".format(a, traceback.format_exc()))
#        continue
#    
#      #print "{}: {} {}".format(fnName, correlated_nodes, sum(correlations_per_node))
#      print("""{}:
#  nodes =           {}
#  edges =           {}
#  invariants =      {}
#  invariants/node = {:.1f}
#  """.format(proofStats.name, proofStats.nodes, proofStats.edges, sum(proofStats.invariants_per_node), avg(proofStats.invariants_per_node)))
