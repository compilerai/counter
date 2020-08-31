#!/usr/bin/python3
# coding: utf-8
import os
import sys
import csv
import glob

# NOTE: we assume CWD to be superopt-project root

# hacky imports
sys.path.extend(["superopt/utils", "superopt/utils/gen_stats"])
# from __main__ won't work; if gen_stats is not in path then imports in gen_stats/__main__ won't work
from gen_stats.__main__ import *

eqlogs_dirname = os.environ.get('EQLOGS_DIR') or 'eqlogs'
logs_dirname = os.environ.get('LOGS_DIR') or '/superopt-tests/build/'
suitefullpath = lambda suite: "{}/{}/{}/{}/".format(os.getcwd(), logs_dirname, suite, eqlogs_dirname)

# names must match corresponding dir names
suitenames = ['TSVC_prior_work-gcc', 'TSVC_prior_work-clang', 'TSVC_prior_work-icc', 'TSVC_new-gcc', 'TSVC_new-clang', 'TSVC_new-icc', 'LORE_mem_write-4uf', 'LORE_mem_write-8uf', 'LORE_no_mem_write-4uf', 'LORE_no_mem_write-8uf']

def summarize_data(bfs_data, dfs_data):
    #assert len(bfs_data) > 0 and len(dfs_data) > 0
    ret = OrderedDict()
    ret['total-fns'] = len(bfs_data)
    ret['failing-fns'] = ret['total-fns'] - sum(d['eq-passed'] for d in bfs_data)
    ret['avg-ALOC'] = round(avg([ d['dst-aloc'] for d in bfs_data if d['dst-aloc'] != 0]))
    ret['max-ALOC'] = max(d['dst-aloc'] for d in bfs_data)
    ret['avg-product-CFG-nodes'] = round(avg([d['cg-pcs'] for d in bfs_data]),1)
    ret['max-product-CFG-nodes'] = max(d['cg-pcs'] for d in bfs_data)
    ret['avg-product-CFG-edges'] = round(avg([d['cg-edges'] for d in bfs_data]),1)
    ret['max-product-CFG-edges'] = max(d['cg-edges'] for d in bfs_data)
    ret['avg-total-CEs-node'] = round(avg([d['CE-total']/(d['cg-pcs']-2) for d in bfs_data if d['eq-passed'] == 1]))
    #ret['max-total-CEs-node'] = round(max([d['CE-total']/(d['cg-pcs']-2) for d in bfs_data if d['eq-passed'] == 1]))
    ret['avg-gen-CEs-node'] =   round(avg([d['CE-generated']/(d['cg-pcs']-2) for d in bfs_data if d['eq-passed'] == 1]))
    #ret['max-gen-CEs-node'] =   round(max([d['CE-generated']/(d['cg-pcs']-2) for d in bfs_data if d['eq-passed'] == 1]))
    ret['BFS-avg-eqtime'] =  round(avg([d['total-equiv-secs'] for d in bfs_data]),1)
    ret['BFS-avg-paths-enum'] = round(avg([d['paths-enumerated'] for d in bfs_data]),1)
    ret['BFS-avg-paths-pruned'] = round(avg([d['paths-pruned'] for d in bfs_data]),1)
    ret['BFS-avg-paths-expanded'] = round(avg([d['paths-expanded'] for d in bfs_data]),1)
    ret['DFS-mem-timeout-reached'] = ret['total-fns'] - sum(d['eq-passed'] for d in dfs_data) - ret['failing-fns']
    ret['DFS-avg-paths-enum'] = round(avg([d['paths-enumerated'] for d in dfs_data]),1)
    ret['DFS-avg-paths-expanded'] = round(avg([d['paths-expanded'] for d in dfs_data]),1)
    ret['avg-paths-expanded-DFS-by-BFS'] = round(ret['DFS-avg-paths-expanded']/ret['BFS-avg-paths-expanded']) if ret['BFS-avg-paths-expanded'] != 0 else 0
    return ret

summaries = []
for s in suitenames:
    suitedir, config = s.split('-')
    sfp = suitefullpath(suitedir)
    #print("Looking at {}".format(sfp))
    print("Reading {} BFS eqlogs...".format(s))
    glbs = "{}/*{}.BFS.eqlog".format(sfp, config)
    print("Glob = {}".format(glbs))
    bfs_data = get_relevant_data_from_eqlog_files(glob.glob(glbs))
    print("Reading {} DFS eqlogs...".format(s))
    glbs = "{}/*{}.DFS.eqlog".format(sfp, config)
    print("Glob = {}".format(glbs))
    dfs_data = get_relevant_data_from_eqlog_files(glob.glob(glbs))
    print("BFS records = {}, DFS records = {}".format(len(bfs_data), len(dfs_data)))
    summary = summarize_data(bfs_data, dfs_data)
    summaries.append(summary)
    
if len(summaries) == 0:
    print("No data found.")
    exit(1)

with open('table2.csv', 'w') as csvfile:
    tablewriter = csv.writer(csvfile)
    tablewriter.writerow(['prop']+suitenames) # header

    props = summaries[0].keys()
    unpk = lambda k: (s[k] for s in summaries)
    for p in props:
        tablewriter.writerow([p, *unpk(p)])
