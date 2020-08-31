#!/usr/bin/python

import argparse
import eq_read_log
import matplotlib.pyplot as plt
import numpy as np
from matplotlib import gridspec
import collections
import random
from operator import itemgetter
import pprint


def get_pass_fail_stats_vs_key(results_map, x_key):
  stats = collections.OrderedDict()
  stats['pass'] = {'data': [], 'label': 'Passed'}
  stats['fail-timeout'] = {'data': [], 'label': 'Timeouts'}
  #stats['fail-unsup-opc'] = {'data': [], 'label': 'Unsupported opcodes'}
  stats['fail-unknown'] = {'data': [], 'label': 'Unknown'}
  loc_ignored = 0
  dst_edge_ignored = 0
  res = {}
  for k in results_map:
    if x_key == 'dst-edges' and 'dst-edges' not in results_map[k]:
      continue
    x = results_map[k][x_key]
    pass_status = results_map[k]['pass_status']
    if x == 0 and x_key == 'loc':
      loc_ignored += 1
      continue
    if x == 0 or x == 2 and x_key == 'dst-edges':
      dst_edge_ignored += 1
      if pass_status not in res:
        res[pass_status] = 0
      res[pass_status] += 1
      continue
    if pass_status.startswith('passed'):
      stats['pass']['data'].append(x)
    elif 'TIMEOUT' in pass_status or 'FAILED_NOT_CHECKED' in pass_status:
      stats['fail-timeout']['data'].append(x)
    elif 'UNKNOWN' in pass_status:
      stats['fail-unknown']['data'].append(x)
    elif 'ERROR_src-loop-xor-dst-loop' in pass_status:
      stats['fail-unknown']['data'].append(x)
    elif 'Assert fail' in pass_status:
      stats['fail-unknown']['data'].append(x)
    elif 'Aborted' in pass_status:
      stats['fail-unknown']['data'].append(x)
    else:
      print(pass_status)
      assert(False)
  print('dst edge zeror ignored {}'.format(dst_edge_ignored))
  print('loc zero ignored {}'.format(loc_ignored))
  return stats

def print_count_till_loc(stats, loc_in):
  count = 0
  tot = 0
  for (loc, passed) in stats:
    if loc <= loc_in:
      tot += 1
      if passed:
        count += 1
  print('pass % till loc {} {}: {}%'.format(loc_in, count, compute_percent(count, tot)))

def get_pass_fail_loc_stats(results_map, x_key='loc'):
  stats = []
  for k in results_map:
    if x_key not in results_map[k]:
      continue
    x = results_map[k][x_key]
    pass_status = results_map[k]['pass_status'].startswith('passed')
    stats.append((x, pass_status))
  stats.sort()
  print('=='+x_key)
  if x_key == 'dst-edges':
    print_count_till_loc(stats, 3)
    print_count_till_loc(stats, 4)
    print_count_till_loc(stats, 5)
    print_count_till_loc(stats, 10)
    print_count_till_loc(stats, 12)
  print_count_till_loc(stats, 50)
  print_count_till_loc(stats, 50)
  print_count_till_loc(stats, 50)
  print_count_till_loc(stats, 100)
  print_count_till_loc(stats, 105)
  print_count_till_loc(stats, 200)
  print_count_till_loc(stats, 300)
  print_count_till_loc(stats, 345)
  print_count_till_loc(stats, 500)
  print_count_till_loc(stats, 600)
  print_count_till_loc(stats, 700)
  print_count_till_loc(stats, 800)
  print_count_till_loc(stats, 900)
  print_count_till_loc(stats, 1000)
  print_count_till_loc(stats, 1100)
  return stats

def get_solver_stats(results_map):
  stats = collections.OrderedDict()
  stats['yices'] = {'data': [], 'label': 'Yices'}
  stats['z3'] = {'data': [], 'label': 'Z3'}
  for k in results_map:
    z3 = results_map[k]['solver-stats']['z3']
    yices = results_map[k]['solver-stats']['yices']
    stats['z3']['data'] += z3
    stats['yices']['data'] += yices
  z3 = len(stats['z3']['data'])
  yices = len(stats['yices']['data'])
  print("z3 won: {}".format(len(stats['z3']['data'])))
  print("yices won: {}".format(len(stats['yices']['data'])))
  a = compute_percent(z3, z3+yices)
  print("z3 vs yices {}% : {}%".format(a, 100 - a))
  print("z3 max {}".format(max(stats['z3']['data'])))
  print("yices max {}".format(max(stats['yices']['data'])))
  return (stats, 'Time taken by query', 'Winner (count)')

def get_time_breakups(results_map):
  stats = collections.OrderedDict()
  stats['total-time'] = {'data': [], 'label': 'Total time'}
  stats['time-simplify'] = {'data': [], 'label': 'Simplifications'}
  stats['time-query'] = {'data': [], 'label': 'Simplifications + Solver'}
  arr = []
  for k in results_map:
    res = results_map[k]
    pass_status = res['pass_status']
    if 'solver-time' not in res or res['time'] < res['solver-time'] or res['time'] < res['time-simplify'] or res['time-simplify'] > res['solver-time'] or res['time-simplify'] == 0 or res['solver-time'] == 0:
      #print '{} {} {}'.format(k, res['solver-time'], pass_status)
      continue
    if 'TIMEOUT' in pass_status:
      continue
    arr.append((res['time'], res['time-simplify'], res['solver-time']))
  arr.sort(key=itemgetter(0)) 
  tot_time = 0
  for (time, time_simpli, time_solver) in arr:
    stats['time-simplify']['data'].append(time_simpli)
    stats['time-query']['data'].append(time_solver)
    stats['total-time']['data'].append(time)
    tot_time += time
  print('percent time by simpli {}'.format(compute_percent(sum(stats['time-simplify']['data']), tot_time)))
  print('percent time by solver+simpli {}'.format(compute_percent(sum(stats['time-simplify']['data']) + sum(stats['time-query']['data']), tot_time)))
  return stats

def get_time_breakups2(results_map):
  stats = collections.OrderedDict()
  stats['time-preprocess'] = {'data': [], 'label': 'Preprocessing'}
  stats['time-correlate'] = {'data': [], 'label': 'Prepocessing + Correlation'}
  stats['total-time'] = {'data': [], 'label': 'Preprocessing + Correlation +\nSimulation relation'}
  arr = []
  for k in results_map:
    res = results_map[k]
    pass_status = res['pass_status']
    if 'time-preprocess' not in res:# or 'TIMEOUT' in pass_status:
      continue
    arr.append((res['time'], res['time-preprocess'], res['time-correlate']))
  arr.sort(key=itemgetter(0)) 
  for (time, time_preprocess, time_correlate) in arr:
    stats['time-preprocess']['data'].append(time_preprocess)
    stats['time-correlate']['data'].append(time_correlate)
    stats['total-time']['data'].append(time)
  total_preprocess = sum(stats['time-preprocess']['data'])
  total_correlate = sum(stats['time-correlate']['data'])
  total = sum(stats['total-time']['data'])
  print('total preprocess time {}'.format(total_preprocess))
  print('total correlate time {}'.format(total_correlate))
  print('total time {}'.format(total))
  if total == 0:
    total = -1
  print('total preprocess % {}'.format(compute_percent(total_preprocess, total)))
  print('total correlate % {}'.format(compute_percent(total_correlate, total)))
  return stats

def get_pdt_vs_time(results_map):
  stats = collections.OrderedDict()
  stats = {'data-fail': [], 'data-pass': [], 'label_x': 'Number of product-graphs checked', 'label_y': 'Time (s)'}
  total_checks = 0
  total_eq_checks = 0
  for k in results_map:
      res = results_map[k]
      if 'nr-equality-check' not in res or 'nr-implication-check' not in res:
        continue
    #if 'CC' in k:
      pass_status = results_map[k]['pass_status']
      #if pass_status.startswith('passed') or 'TIMEOUT' not pass_status:
      eq_check = results_map[k]['nr-equality-check']
      impli_check = results_map[k]['nr-implication-check']
      time = results_map[k]['time']
      if 'TIMEOUT' in pass_status:
        time = 18000
      checks = impli_check + eq_check
      if pass_status.startswith('passed'):
        stats['data-pass'].append((checks, time))
      else:
        stats['data-fail'].append((checks, time))
      total_checks += checks
      total_eq_checks += eq_check
  a = compute_percent(total_eq_checks, total_checks)
  print('Equality checks vs implication check {}% : {}%'.format(a, 100 - a))
  return stats

def get_dst_edges_vs_time(results_map):
  stats = collections.OrderedDict()
  stats = {'data-fail': [], 'data-pass': [], 'label_x': 'Number of edges in collapsed dst-TFG', 'label_y': 'Time (s)'}
  total_checks = 0
  total_eq_checks = 0
  for k in results_map:
    #if 'CC' in k:
      if 'dst-edges' not in results_map[k] or results_map[k]['dst-edges'] == 2:
        continue
      pass_status = results_map[k]['pass_status']
      dst_edges = results_map[k]['dst-edges']
      time = results_map[k]['time']
      if 'TIMEOUT' in pass_status:
        time = 18000
      if pass_status.startswith('passed'):
        stats['data-pass'].append((dst_edges, time))
      else:
        stats['data-fail'].append((dst_edges, time))
  return stats

def get_loc_vs_time(results_map):
  stats = collections.OrderedDict()
  stats = {'data-fail': [], 'data-pass': [], 'label_x': 'ALOC', 'label_y': 'Time (s)'}
  total_checks = 0
  total_eq_checks = 0
  for k in results_map:
    #if 'CC' in k:
      pass_status = results_map[k]['pass_status']
      loc = results_map[k]['loc']
      time = results_map[k]['time']
      if 'TIMEOUT' in pass_status:
        time = 18000
      if pass_status.startswith('passed'):
        stats['data-pass'].append((loc, time))
      else:
        stats['data-fail'].append((loc, time))
  return stats

def avg_y_vals(data):
  ret = {}
  for x, y in data:
    if x not in ret:
      ret[x] = (y, 1)
    else:
      ret[x] = ((ret[x][0] * ret[x][1] + y)/(ret[x][1]+1), ret[x][1]+1)
  arr = []
  for a in ret:
    arr.append((a, ret[a][0]))
  return arr
def split_pair_to_arr(data):
  ret = [], []
  for d in data:
    ret[0].append(d[0])
    ret[1].append(d[1])
  return ret

def ignore_small_values(arr):
  return arr
  ret = []
  for (x, y) in arr:
    if y < 0.5:
      continue
    ret.append((x, y))
  return ret

def plot_dot_graph(stats, filename, xmin=0, ymin=0):
  plt.figure(figsize=(4, 3))
  plt.semilogx(basex=2)
  plt.semilogy(basey=2)
  data_pass = ignore_small_values(stats['data-pass'])
  data_fail = ignore_small_values(stats['data-fail'])
  #data_fail = avg_y_vals(data_fail)
  #data_pass = avg_y_vals(data_pass)
  data_fail.sort(key=itemgetter(0))
  data_pass.sort(key=itemgetter(0))
  X_pass ,Y_pass = split_pair_to_arr(data_pass)
  global g_colors
  plt.plot(X_pass, Y_pass, 'x', label='Pass', color = g_colors[0])
  X_fail ,Y_fail = split_pair_to_arr(data_fail)
  plt.plot(X_fail, Y_fail, '.', label='Fail', color = g_colors[3])
  if filename == 'time-with-loc.pdf':
    plt.plot([2**5.5,2**8.5], [2**0, 2**11.3])
    print('slope in time-with-loc.pdf is: {}'.format(11.3/3)) 
    plt.annotate('Slope:3.8', xy=(2**7.5, 2**7.4), xytext=(2**9, 2**4.4),  arrowprops=dict(arrowstyle="->", 
      connectionstyle="arc,angleA=0,armA=-15,angleB=-80,armB=15,rad=7"))
  plt.xlim(xmin=xmin)
  plt.ylim(ymin=ymin)
  plt.xlabel(stats['label_x'])
  plt.ylabel(stats['label_y'])
  plt.tight_layout()
  plt.legend(loc='lower right')
  plt.savefig(filename)

def plot_stacked_bar_graph_subplot_bar(cat, ax, x, ds_label, colors):
  ds = ds_label['data']
  label = ds_label['label']
  width = 0.8
  bars = []
  for (i, y) in enumerate(ds):
    bars.append(ax.bar(x, y, width, color=[colors[i]], bottom=sum(ds[:i]), label=cat))
  rect = bars[len(bars)-1]
  if False: # don't show function count
    ax.text(x+0.1, 100+4, str(label), rotation='vertical', ha='left', va='bottom', fontsize=8)
  return bars

def show_string(cat):
  if cat == 'compcert':
    return 'ctests'
  if cat == 'gcc48.O0':
    return 'gcc-O0'
  if cat == 'gcc48.O2':
    return 'gcc-O2'
  if cat == 'gcc48.O3':
    return 'gcc-O3'
  if cat == 'icc.O0':
    return 'icc-O0'
  if cat == 'icc.O2':
    return 'icc-O2'
  if cat == 'icc.O3':
    return 'icc-O3'
  if cat == 'ccomp.O0':
    return 'ccomp-O0'
  if cat == 'ccomp.O2':
    return 'ccomp-O2'
  if cat == 'clang36.O0':
    return 'clang-O0'
  if cat == 'clang36.O2':
    return 'clang-O2'
  if cat == 'clang36.O3':
    return 'clang-O3'
  if cat == 'gcc48-i386-O0-soft-float':
    return 'gcc-O0'
  if cat == 'gcc48-i386-O2-soft-float':
    return 'gcc-O2'
  if cat == 'gcc48-i386-O3-soft-float':
    return 'gcc-O3'
  if cat == 'clang36-i386-O0-soft-float':
    return 'clang-O0'
  if cat == 'clang36-i386-O2-soft-float':
    return 'clang-O2'
  if cat == 'clang36-i386-O3-soft-float':
    return 'clang-O3'
  if cat == 'icc-i386-O0-soft-float':
    return 'icc-O0'
  if cat == 'icc-i386-O2-soft-float':
    return 'icc-O2'
  if cat == 'icc-i386-O3-soft-float':
    return 'icc-O3'
  if cat == 'ccomp-i386-O0-soft-float':
    return 'ccomp-O0'
  if cat == 'ccomp-i386-O2-soft-float':
    return 'ccomp-O2'
  return cat

def plot_stacked_bar_graph_subplot(group, ax, ds, colors):
  width = 0.8
  num_cats = len(ds)
  ind = np.arange(num_cats)
  i = 0
  ax.set_xlabel(show_string(group))
  cats = []
  bars = []
  for cat in ds:
    #ax.set_title(show_string(group))
    bars = plot_stacked_bar_graph_subplot_bar(cat, ax, i, ds[cat], colors)
    i += 1
    cats.append(show_string(cat))
  #plt.sca(ax)
  ax.set_xticks(ind+width/2)
  ax.set_xticklabels(cats, rotation='vertical', fontsize=8.5)
  return bars

# group---category---data---[bar_data]
#                 |
#                  --label--int
#
def plot_stacked_bar_graph(ds, colors, ylabel):
  num_groups = len(ds)
  i = 0
  f, ax = plt.subplots(nrows=1, ncols=num_groups, sharey=True, figsize=(13, 3.3))
  f.subplots_adjust(top=0.6, wspace=0)
  ax[0].set_ylabel(ylabel)
  #width = 1
  bars = []
  for group in ds:
    num_cats = len(ds[group])
    ind = np.arange(num_cats)
    bars = plot_stacked_bar_graph_subplot(group, ax[i], ds[group], colors)
    ax[i].spines['top'].set_visible(False)
    ax[i].spines['right'].set_visible(False)
    if i != 0:
      ax[i].spines['bottom'].set_visible(False)
      ax[i].spines['left'].set_visible(False)
    ax[i].xaxis.set_ticks_position('bottom')
    ax[i].yaxis.set_ticks_position('left')
    ax[i].axhline(y=76, ls='--')
    ax[i].axhline(y=72, ls='-.', color = 'r')
    #ax[i].yaxis.grid(True)
    i += 1
  plt.ylim(ymax=107)
  plt.yticks([20, 40, 60, 80, 100])
  f.legend(bars, ['Passed cyclic', 'Passed acyclic', 'Failed cyclic', 'Failed acyclic'], 
    ncol=4, loc='upper right', fontsize=9)
  #plt.show()
  plt.tight_layout()
  plt.savefig('main.pdf', dpi=100)
  return

def get_bench_nr(bench):
  order_benches = [
    'specrand998', 
    'specrand999', 
    'specrand.998', 
    'specrand.999', 
    'mcf', 
    'bzip2', 
    'ctests', 
    'compcert', 
    'gzip', 
    'sjeng', 
    'astar',
    'parser', 
    'gap', 
    ]
  for (i, b) in enumerate(order_benches):
    if b == bench:
      return i
  #assert False
  return 100

def get_cat_nr_main(cat):
  order_cat = ['gcc2', 'clang2', 'icc2', 'ccomp2', 'gcc3', 'clang3', 'icc3']
  for (i, c) in enumerate(order_cat):
    if c == show_string(cat):
      return i
  #assert False
  return 100

def get_bench_cat_weight(xxx_todo_changeme):
  (b, c) = xxx_todo_changeme
  return 100*get_bench_nr(b) + get_cat_nr_main(c)

def compare_bench_cat(bc1, bc2):
  return cmp(get_bench_cat_weight(bc1), get_bench_cat_weight(bc2))

def get_group_cats(results):
  ret = []
  for (group, cat, loop) in results:
    if cat.startswith('cg'):
      continue
    if group == 'astar':
      continue
    if (group, cat) not in ret:
      ret.append((group, cat))
  ret.sort(compare_bench_cat)
  return ret

def get_cats(results):
  ret = set()
  for (group, cat, loop) in results:
    if cat.startswith('cg'):
      continue
    if group == 'astar':
      continue
    if (group, cat) not in ret:
      ret.add(cat)
  return ret

def compute_percent(a, total):
  return a*100.0/total

def plot_pass_percent(results_map):
  results = eq_read_log.merge_results_by_new_key(results_map, lambda bench_comp_func_loop : (bench_comp_func_loop[0], bench_comp_func_loop[1], bench_comp_func_loop[3]))
  group_cats = get_group_cats(results)
  #pp = pprint.PrettyPrinter()
  #pp.pprint(results)
  #pp.pprint(group_cats)
  ds = collections.OrderedDict()
  for (group, cat) in group_cats:
    if group not in ds:
      ds[group] = collections.OrderedDict()
    if cat not in ds[group]:
      ds[group][cat] = {}
      ds[group][cat]['data'] = [0, 0, 0, 0]
    total_aa = results[(group, cat, 'A')]['total']
    total_aa_pass = results[(group, cat, 'A')]['total-passed']
    total_aa_fail = total_aa - total_aa_pass
    total_cc = results[(group, cat, 'C')]['total']
    total_cc_pass = results[(group, cat, 'C')]['total-passed']
    total_cc_fail = total_cc - total_cc_pass
    total = total_aa + total_cc
    ds[group][cat]['data'][0] = compute_percent(total_cc_pass, total)
    ds[group][cat]['data'][1] = compute_percent(total_aa_pass, total)
    ds[group][cat]['data'][2] = compute_percent(total_cc_fail, total)
    ds[group][cat]['data'][3] = compute_percent(total_aa_fail, total)
    ds[group][cat]['label'] = total

  # fix missing ccomp2
  for b in ds:
    for comp in get_cats(results):
      if comp not in ds[b]:
        ds[b][comp] = {}
        ds[b][comp]['data'] = [0, 0, 0, 0]
        ds[b][comp]['label'] = 0

  global g_colors
  colors = g_colors
  plot_stacked_bar_graph(ds, colors, 'Equivalence stats (%)')

def get_colors(count):
  colors = ['#bae4b3','#74c476','#fb6a4a','#cb181d', '#1a9641', '#a6d96a', '#fdae61', '#d7191c']
  assert(count <= len(colors))
  return colors[0:count]

def get_color(index):
  colors = ['#bae4b3','#74c476','#fb6a4a','#cb181d', '#1a9641', '#a6d96a', '#fdae61', '#d7191c']
  assert(index < len(colors))
  return colors[index]

#g_colors = ['#1a9641', '#a6d96a', '#fdae61', '#d7191c']
g_colors =['#bae4b3','#74c476','#fb6a4a','#cb181d'] #['#edf8fb','#b2e2e2','#66c2a4','#238b45']
# http://colorbrewer2.org/
def pass_fail_loc(xxx_todo_changeme1, filename, log_x=False, log_y=False, cumulative=False, 
    bins=100, legend_loc='upper right', stacked=True, histtype='stepfilled', is_loc=False):
  (stats, xlabel, ylabel) = xxx_todo_changeme1
  global g_colors
  data = []
  labels = []
  plt.figure(figsize=(6, 4))
  if log_x:
    plt.semilogx(basex=2)
  for s in stats:
    data.append(stats[s]['data'])
    labels.append(stats[s]['label'])
  assert(len(data) == 3)
  assert(len(data[0]) > 0)
  assert(len(data[1]) > 0)
  assert(len(data[2]) > 0)
  rects = plt.hist(data, bins=bins, histtype=histtype, stacked=stacked, cumulative=cumulative, label=labels, log=log_y, color=get_colors(len(data)))
  if len(stats) > 1:
    plt.legend(loc = legend_loc)
  if is_loc:
    plt.annotate('ALOC: 105, 97% pass rate', xy=(105, 9700), xytext=(60, 2000),  arrowprops=dict(arrowstyle="->"))
    plt.annotate('ALOC: 345, 90% pass rate', xy=(345, 18200), xytext=(200,11135), arrowprops=dict(arrowstyle="->"))
  plt.xlabel(xlabel)
  plt.ylabel(ylabel)
  plt.tight_layout()
  plt.savefig(filename)

def plot_times(stats, log_x, log_y, filename):
  data = []
  labels = []
  plt.figure(figsize=(5, 3))
  if log_x:
    plt.semilogx(basex=2)
  if log_y:
    plt.semilogy(basey=2)
  ls = ['-', '--', ':', '-.']
  for i, s in enumerate(stats):
    plt.plot(np.arange(len(stats[s]['data'])), np.cumsum(stats[s]['data']), label=stats[s]['label'], ls=ls[i])
  plt.legend(loc = 'lower right')
  plt.xlabel('# of equivalence checks')
  plt.ylabel('Cumulative time (s)')
  plt.tight_layout()
  plt.savefig(filename)

def remove_out_of_range(x, y, max_x):
  retx = []
  rety = []
  for (i, data) in enumerate(x):
    if data < max_x:
      retx.append(data)
      rety.append(y[i])
  return retx, rety

def plot_x_y(x, y, xlabel, ylabel, filename, max_x=None, log_x=False):
  if max_x:
    x, y = remove_out_of_range(x, y, max_x)
  #y, x = remove_out_of_range(y, x, 8000)
  plt.figure(figsize=(8, 8))
  if log_x:
    plt.semilogx(basex=2)
  plt.plot(x, y, 'x')
  plt.title(filename)
  plt.xlabel(xlabel)
  plt.ylabel(ylabel)
  plt.tight_layout()
  plt.savefig(filename)

def time_loc(results_map, loop):
  time = []
  loc = []
  for (bench, comp, fun, aa_cc) in results_map:
    key = (bench, comp, fun, aa_cc)
    if results_map[key]['pass_status'].startswith('passed'):
      if results_map[key]['loc'] != 0:
        if loop == 'all' or loop == aa_cc:
          time.append(results_map[key]['time'])
          loc.append(results_map[key]['loc'])
  plot_x_y(loc, time, 'loc', 'time', 'time-loc-'+loop)

def plot_keys(results_map, xkey, ykey, loop, max_x=None, log_x=True):
  x = []
  y = []
  for (bench, comp, fun, aa_cc) in results_map:
    key = (bench, comp, fun, aa_cc)
    if results_map[key]['pass_status'].startswith('passed'):
      if results_map[key][xkey][0] != 0 and results_map[key][ykey] != 0:
        if loop == 'all' or loop == aa_cc:
          x.append(results_map[key][xkey][0])
          y.append(results_map[key][ykey])
  plot_x_y(x, y, xkey, ykey, '{}-{}-{}.pdf'.format(xkey, ykey, loop), max_x, log_x=log_x)


def ignore_bench(results_map, bench):
  ret = {}
  for k in results_map:
    if bench == k[0]:
      continue
    ret[k] = results_map[k]
  return ret

def ignore_comp(results_map, c):
  ret = {}
  for k in results_map:
    if c == k[1]:
      continue
    ret[k] = results_map[k]
  return ret

def ignore_unsup_ops(results_map):
  ret = {}
  for k in results_map:
    pass_status = results_map[k]['pass_status']
    if pass_status.startswith('passed'):
      ret[k] = results_map[k]
      continue
    if 'ERROR_contains_unsupported_op' in pass_status or 'ERROR_contains_float_op' in pass_status:
      continue
    ret[k] = results_map[k]
  return ret

def mean(arr):
  return sum(arr)/len(arr)

def percentile(arr, percent):
  return arr[len(arr)*percent/100]

def get_stats_verify(results_map):
  verify_time = []
  for k in results_map:
    res = results_map[k]
    pass_status = res['pass_status']
    if not pass_status.startswith('passed'):
      continue
    if res['loop'] != 'C':
      continue
    verify_time.append(res['verify-time'])
  print('verify-time only C: {}'.format(get_arr_stats(verify_time)))
  return {'data': verify_time, 'label': 'Verification time'}

def get_stats_timeouts(results_map, only_loops):
  time = []
  for k in results_map:
    res = results_map[k]
    pass_status = res['pass_status']
    if only_loops and 'C' not in k:
      continue
    if 'TIMEOUT' in pass_status:
      time.append(18000)
  print('timeout-time: {}'.format(get_arr_stats(time)))
  return {'data': time, 'label': 'Timeout'}

def get_stats_time_pass(results_map, only_loops):
  time = []
  for k in results_map:
    res = results_map[k]
    pass_status = res['pass_status']
    if not pass_status.startswith('passed'):
      continue
    if only_loops and 'C' not in k:
      continue
    time.append(res['time'])
  print('pass time stats: {}'.format(get_arr_stats(time)))
  return {'data': time, 'label': 'Passing time'}

def get_stats_time_fail(results_map, only_loops):
  time = []
  for k in results_map:
    res = results_map[k]
    pass_status = res['pass_status']
    if pass_status.startswith('passed'):
      continue
    if 'TIMEOUT' in pass_status:
      continue
    if only_loops and 'C' not in k:
      continue
    time.append(res['time'])
  print('fail time stats: {}'.format(get_arr_stats(time)))
  return {'data': time, 'label': 'Failing time'}

def plot_times_hist(loops):
  timeout_times = get_stats_timeouts(results_map, loops)
  pass_times = get_stats_time_pass(results_map, loops)
  fail_times = get_stats_time_fail(results_map, loops)
  stats = ({'timeouts': timeout_times, 'pass-time': pass_times, 'fail-time': fail_times}, 'Time (s)', 'cumulative count')
  filename = ''
  if loops:
    filename = 'times-hist-cc.pdf'
  else:
    filename = 'times-hist.pdf'
  fail_pass_loc_hist(stats, filename, cumulative=True, log_x=True, bins=50000, stacked=False, histtype='step', legend_loc='upper left')

def get_arr_stats(arr):
  ret = {} #collections.OrderedDict()
  arr.sort()
  ret['min'] = min(arr)
  ret['max'] = max(arr)
  ret['mean'] = mean(arr)
  ret['10%'] = percentile(arr, 10)
  ret['50%'] = percentile(arr, 50)
  ret['90%'] = percentile(arr, 90)
  ret['95%'] = percentile(arr, 95)
  ret['99%'] = percentile(arr, 99)
  ret['85%'] = percentile(arr, 85)
  ret['80%'] = percentile(arr, 80)
  ret['count'] = len(arr)
  ret['sum'] = sum(arr)
  return ret

def get_stat_array(results_map, key, passed_only=False, loop_only=False, ignore_zero=False, ignore_timeouts=False, ignore_missing_key=True):
  ret = []
  for k in results_map:
    if ignore_missing_key and key not in results_map[k]:
      continue
    res = results_map[k]
    pass_status = res['pass_status']
    passed = pass_status.startswith('passed')
    loop = 'C' in k
    if loop_only and not loop:
      continue
    if passed_only and not passed:
      continue
    if ignore_zero and res[key] == 0:
      continue
    if ignore_timeouts and 'TIMEOUT' in pass_status:
      continue
    if key=='time' and 'TIMEOUT' in pass_status:
      ret.append(18000)
    else:
      val = res[key]
      if key == 'max_quota':
        #if 'back_propagation: 1' in val:
        if 'guess_level: 1' in val:
          ret.append(1)
      else:
        ret.append(res[key])
  ret.sort()
  return ret

def get_pass_fail_stats(results_map):
  stats = {}
  for k in results_map:
    bench = k[0]
    if bench not in stats:
      stats[bench] = {'total_passed': 0, 'total': 0, 'total_cc': 0, 'total_passed_cc': 0}
    res = results_map[k]
    pass_status = res['pass_status']
    passed = pass_status.startswith('passed')
    loop = 'C' in k
    stats[bench]['total'] += 1
    if loop:
      stats[bench]['total_cc'] += 1
    if passed:
      stats[bench]['total_passed'] += 1
    if passed and loop:
      stats[bench]['total_passed_cc'] += 1
  total = 0
  total_passed = 0
  for s in stats:
    total_passed += stats[s]['total_passed']
    total += stats[s]['total']
    print(s+' pass % {}'.format(compute_percent(stats[s]['total_passed'], stats[s]['total'])))
  print('cumulative pass % {}'.format(compute_percent(total_passed, total)))
  #print stats

def cumsum(arr):
  ret = []
  x = 0
  for a in arr:
    x += a
    ret.append(x)
  return ret

def gen_stats(results_map):
  time_stats = get_arr_stats(get_stat_array(results_map, 'time'))
  print('time: {}'.format(time_stats))
  print('total time: {} months'.format(time_stats['sum']/(60*60*24*30.0)))
  time_stats_pass = get_arr_stats(get_stat_array(results_map, 'time', passed_only=True))
  print('time-pass: {}'.format(time_stats_pass))

  #print 'verify-time: {}'.format(get_arr_stats(get_stat_array(results_map, 'verify-time', passed_only=True, loop_only=True, ignore_zero=True)))
  print('ALOC: {}'.format(get_arr_stats(get_stat_array(results_map, 'loc', passed_only=False))))
  print('ALOC-pass: {}'.format(get_arr_stats(get_stat_array(results_map, 'loc', passed_only=True))))
  print('dst-edges: {}'.format(get_arr_stats(get_stat_array(results_map, 'dst-edges'))))
  print('dst-edges-pass: {}'.format(get_arr_stats(get_stat_array(results_map, 'dst-edges', passed_only=True))))
  print('guess-level-pass: {}'.format(get_arr_stats(get_stat_array(results_map, 'max_quota', passed_only=True))))
  #print 'proof size bytes: {}'.format(get_arr_stats(get_stat_array(results_map, 'vc-string-size', passed_only=True, loop_only=True, ignore_zero=True)))
  #print 'proof size zlib bytes: {}'.format(get_arr_stats(get_stat_array(results_map, 'vc-string-size-zlib', passed_only=True, loop_only=True, ignore_zero=True)))
  l1_total = get_arr_stats(get_stat_array(results_map, 'l1-cache-total'))['sum']
  l1_hits = get_arr_stats(get_stat_array(results_map, 'l1-cache-hits-false'))['sum'] + \
            get_arr_stats(get_stat_array(results_map, 'l1-cache-hits-true'))['sum']
  l2_total = get_arr_stats(get_stat_array(results_map, 'l2-cache-total'))['sum']
  l2_hits = get_arr_stats(get_stat_array(results_map, 'l2-cache-hits'))['sum']
  print('Cache hit rate: {}'.format(compute_percent(l1_hits+l2_hits, l1_total+l2_total)))
  print('\npass fail stats including floats')
  print('--------------------------------')
  get_pass_fail_stats(results_map)
  print('\npass fail stats excluding floats')
  print('--------------------------------')
  get_pass_fail_stats(ignore_float_ignore_harvest_error(results_map))
  time_stats_cum = cumsum(get_stat_array(results_map, 'time', ignore_timeouts=True))
  total = time_stats_cum[len(time_stats_cum)-1]
  time_stats_cum_percent = []
  for t in time_stats_cum:
    time_stats_cum_percent.append(compute_percent(t, total))
  print('time cumsum stats: {}'.format(get_arr_stats(time_stats_cum_percent)))

def get_cat_nr_perf(cat):
  order_cat = ['gcc-O0', 'llvm-O0', 'icc-O0', 'ccomp-O0', 'gcc-O2', 'llvm-O2', 'icc-O2', 'ccomp-O2']
  for (i, c) in enumerate(order_cat):
    if c == cat:
      return i
  return 100

def compare_bench(b1, b2):
  return cmp(get_bench_nr(b1), get_bench_nr(b2))

def compare_cat(c1, c2):
  return cmp(get_cat_nr_perf(c1), get_cat_nr_perf(c2))

def get_benches_comps(stats):
  benches = []
  comps = []
  for b in stats:
    benches.append(b)
    for c in stats[b]:
      if (c not in comps) and (not c.endswith('0')):
        comps.append(c)
  #for b in benches:
  #  print '{} {}'.format(b, get_bench_nr(b))
  benches.sort(compare_bench)
  comps.sort(compare_cat)
  return (benches, comps)

def parse_compcert_perf_results(stats):
  stats['ctests'] = {}
  for line in open('../superopt/compcert.out'):
    [comp, val] = line.strip().split(' ')
    comp = show_string(comp)
    val = float(val)
    if comp not in stats['ctests']:
      stats['ctests'][comp] = val
    if val < stats['ctests']:
      stats['ctests'][comp] = val

def parse_spec_perf_results(stats, filename):
  #stats['ctests'] = {}
  data = []
  for line in open(filename):
    lst = line.strip().split(':')
    if len(lst) != 5:
      continue
    [bench, arg, comp, count, val] = lst
    val = float(val)
    data.append((bench.strip(), arg.strip(), show_string(comp.strip()), count.strip(), val))
  # sum different args
  stats1 = {}
  for k in data:
    (bench, arg, comp, count, val) = k
    new_k = (bench, comp, count)
    if new_k not in stats1:
      stats1[new_k] = val
    else:
      stats1[new_k] += val
  for k in stats1:
    (bench, comp, count) = k
    val = stats1[k]
    if bench not in stats:
      stats[bench] = {}
    if comp not in stats[bench]:
      stats[bench][comp] = val
    if val < stats[bench][comp]:
      stats[bench][comp] = val

def scale_stats(stats, comp):
  for bench in stats:
    ref = stats[bench][comp]
#    print ref
    for c in stats[bench]:
      curr = stats[bench][c]
      stats[bench][c] = ref/curr

def is_valid_comp_option(b, c):
  if b in ['astar', 'vortex', 'twolf', 'vortex', 'perlbmk'] and c.startswith('ccomp'):
    return False
  else: return True

def get_comp_values(stats, comp, benches):
  ret = []
  for b in benches:
    if not is_valid_comp_option(b, comp):
      ret.append(0)
    else:
      ret.append(stats[b][comp])
  return ret

def compute_avg_speedup(stats, benches, comps):
  count = 0
  tot = 0
  for b in benches:
    for c in comps:
      if not is_valid_comp_option(b, c):
        continue
      count += 1
      tot += stats[b][c]
  return tot/count

def ignore_benches_perf(stats):
  ret = {}
  for k in stats:
    if k in ['mcf', 'bzip2', 'gzip', 'crafty', 'ctests', 'gap', 'vortex', 'sjeng', 'vpr', 'twolf', 'parser', 'perlbmk']:
      ret[k] = stats[k]
  return ret

def perf_plots():
  stats = {}
  parse_compcert_perf_results(stats)
  parse_spec_perf_results(stats, '../superopt/specrun2000.out')
  parse_spec_perf_results(stats, '../superopt/specrun2006.out')
  stats = ignore_benches_perf(stats)
  scale_stats(stats, 'icc-O0')
  #pp = pprint.PrettyPrinter()
  #pp.pprint(stats)
  (benches, comps) = get_benches_comps(stats)
  
  avg_speedup = compute_avg_speedup(stats, benches, comps)
  print('avg-speedup {}'.format(avg_speedup))

  n_benches = len(benches)
  n_comps = len(comps)
  ind = np.arange(n_benches)
  width = 1.0/(1.5+n_comps)

  fig = plt.figure(figsize=(11.5, 5))
  ax = fig.add_subplot(111)

  rectss = []
  for i,c in enumerate(comps):
    yvals = get_comp_values(stats, c, benches)
    rects = ax.bar(ind+width*i, yvals, width, color=get_color(i))
    rectss.append(rects)

  ax.axhline(y=avg_speedup, ls='--')
  plt.annotate('avg speedup: '+'{:.1f}'.format(avg_speedup), xy=(8.5, avg_speedup), xytext=(9, avg_speedup*1.5),  arrowprops=dict(arrowstyle="->"))
  ax.set_ylabel('Speedup relative to icc-O0')
  ax.set_xticks(ind+0.5-width/2.0)
  labels = list(map(show_string, benches))
  ax.set_xticklabels(labels, rotation=20)
  ax.legend(rectss, comps, ncol=2, loc='upper left')
  plt.tight_layout()
  plt.savefig('perf.pdf', dpi=100)

if __name__ == "__main__":
  parser = argparse.ArgumentParser()
  #parser.add_argument('--stats-only', help = "print only final stats", action='store_true')
  args = parser.parse_args()

  plt.rcParams['font.size'] = 11
  plt.rcParams['font.family'] = 'monospace'
  plt.rcParams['legend.fontsize'] = 9

  perf_plots()
  results_map = eval(open('res-all-map', 'r').read())
  results_map = ignore_comp(results_map, 'cg02')
  results_map = ignore_comp(results_map, 'cg03')
  results_map_orig = results_map
  results_map = ignore_unsup_ops(results_map)
  print('{} floats-etc ignored'.format(len(results_map_orig) - len(results_map)))
  results_map = ignore_bench(results_map, 'astar')

  get_pass_fail_loc_stats(results_map, 'loc')
  get_pass_fail_loc_stats(results_map, 'dst-edges')
  #assert(False)
  plot_pass_percent(results_map)
  pass_fail_stats_loc = get_pass_fail_stats_vs_key(results_map, 'loc')
  pass_fail_loc((pass_fail_stats_loc, 'Assembly lines of code (ALOC) (log scale)', 'Equivalence stats (count)'), 
    'pass-fail-loc.pdf', log_x=True, cumulative=True, bins=10000, legend_loc='upper left', is_loc=True)

  pass_fail_stats_dst_edges = get_pass_fail_stats_vs_key(results_map, 'dst-edges')
  pass_fail_loc((pass_fail_stats_dst_edges, 'Edges in collapsed dst-TFG', 'Equivalence stats (count)'), 'pass-fail-dst-edges.pdf', log_x=False, cumulative=True, bins=10000, legend_loc='lower right')

  plot_times(get_time_breakups(results_map), False, True, 'time-breakup-solver.pdf')
  plot_times(get_time_breakups2(results_map), False, True, 'time-breakup-steps.pdf')

  plot_dot_graph(get_pdt_vs_time(results_map), 'time-with-pdts.pdf', ymin=0.5)
  plot_dot_graph(get_dst_edges_vs_time(results_map), 'time-with-dst-edges.pdf', xmin=0.5, ymin=0.5)
  plot_dot_graph(get_loc_vs_time(results_map), 'time-with-loc.pdf', ymin=0.5)

  gen_stats(results_map)
#order gcc-llvm-icc-ccomp-cgg0

  #fail_pass_loc_hist(get_solver_stats(results_map), 'solver-hist.pdf', cumulative=False, bins=10, log_y=True)

  #plot_keys(results_map, 'dst-nodes', 'time', 'CC')
  #plot_keys(results_map, 'src-nodes', 'time', 'CC', 60)
  #plot_keys(results_map, 'dst-edges', 'time', 'CC')
  #plot_keys(results_map, 'src-edges', 'time', 'CC')
#  plot_keys(results_map, 'run-houdini', 'time', 'CC', 500)
  #plot_keys(results_map, 'is-valid-correlation', 'time', 'CC')
  #plot_keys(results_map, 'solver-time', 'time', 'all', 6000)
  #plot_keys(results_map, 'solver-queries', 'time', 'all', 30000)
  #plot_keys(results_map, 'loc', 'time', 'AA', log_x=True)
  #plot_keys(results_map, 'loc', 'time', 'CC', log_x=True)
  #plot_keys(results_map, 'loc', 'time', 'all', log_x=True)
  #plot_keys(results_map, 'loc', 'total-queries', 'CC')
  #plot_keys(results_map, 'src-edges', 'total-queries', 'CC')
  #plot_keys(results_map, 'num_constrains', 'time', 'CC', 1000)

#  plot_times_hist(True)
#  plot_times_hist(False)
