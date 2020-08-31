#!/usr/bin/python

import glob
import sys
import re
import subprocess
import os
import pprint
import argparse
import shutil
import utils
import eq_cache


def is_logpy_available(f):
  logpy = utils.add_ext(f, 'logpy')
  return os.path.exists(logpy)

def read_logpy(f):
  if not is_logpy_available(f):
    return
  logpy = utils.add_ext(f, 'logpy')
  with open(logpy, 'r') as fh:
    return eval(fh.read())

def read_logfile(f, ptab = ''):
  #logpy = utils.add_ext(f, 'logpy')
  #res = read_logpy(f)
  #if not res:
  res = read_logfile_actual(f, ptab)
  #with open(logpy, 'w') as fh:
  #  pprint.pprint(res, fh)
  return res
  
def read_logfile_actual(n, ptab = ''):
  f = n #utils.add_ext(n, 'log')
  result = {}
  text = ''
  with open(f, 'r') as fh:
    text = fh.read()
  pass_status = get_pass_status(text, "")
  result['pass_status'] = pass_status
  pass_status = get_pass_status(text, "assume_correct")
  result['pass_status_assume_correct'] = pass_status
  pass_status = get_pass_status(text, "assume_incorrect")
  result['pass_status_assume_incorrect'] = pass_status
  time = get_time(text, 'total-equiv.')
  if time == 0.0:
    time = get_time(text, 'total-equiv')
  result['time'] = time
  time = get_time(text, 'total-equiv.correct')
  result['time_assume_correct'] = time
  time = get_time(text, 'total-equiv.incorrect')
  result['time_assume_incorrect'] = time
  result['loc'] = 0
  result['log-file'] = utils.path_to_filename(n)
  #result['time'] = time
  #print "f = " + f
  #key = get_bench_and_type(f)
  #bench = key[0]
  #comp = key[1]
  #func = key[2]
  #loop = key[3]
  #if ptab == '':
  #  #ptab = utils.change_ext(f, 'ptab')
  #  ptab = "peeptabs-" + bench + "/" + comp + "/" + func + "-" + loop + ".ptab"
  ##print "ptab = " + ptab
  #if os.path.exists(ptab):
  #  result['loc'] = get_loc(ptab)
  #else:
  #  result['loc'] = 0

  #result['loop'] = loop
  result['solver-time'] = get_time(text, 'query:sat')
  result['run-houdini'] = get_num_starts(text, 'run_houdini')
  #result['uses-local-sprel-expr-mapping'] = proof_uses_local_sprel_expr_mapping(f)
  result['time-preprocess'] = get_num_starts(text, 'preprocess_tfgs')[1]
  result['time-simplify'] = get_num_starts(text, 'query:simplify')[1]
  result['time-simplify-z3'] = get_num_starts(text, 'query:simplify-z3')[1]
  result['time-correlate'] = get_num_starts(text, 'find_correlation')[1]
  add_cache_counts(text, result)
  add_more_counts(text, result)
  return result
  #if pass_status != 'passed':
  #  time = 0

  lines = text.splitlines()
  state_cons = {'state': 0, 'result' : 0}
  state_solver_data = {'yices': [], 'z3' : []}
  for line in lines:
    line_visitor_loc(line, state_loc)
    line_visitor_solver_times(line, state_solver_data)

  result['harvester-error'] = state_loc['result-harvester-error']
  result['float'] = state_loc['result-float']
  result['pass_status'] = add_err_info_to_pass_status(result['pass_status'], result['float'], 'FLOAT')
  result['pass_status'] = add_err_info_to_pass_status(result['pass_status'], result['harvester-error'], 'HARVESTER_ERROR')
  return result

def add_err_info_to_pass_status(pass_status, is_err, err_str):
  if is_err:
    if pass_status.startswith('passed'):
      return pass_status.replace('passed', 'passed ' + err_str)
    if pass_status.startswith('FAILED'):
      return 'FAILED: ' + err_str
  return pass_status

def search_last(text, rexp):
  ms = re.findall(rexp, text, re.M)
  if len(ms) > 0:
    return ms[len(ms)-1]
  else:
    return None

def get_counter(text, key):
  counter = '0'
  m = search_last(text, '\n' + key + ': (.+?)$')
  if m:
    counter = m
  return int(counter)

def get_counter_str(text, key):
  counter = ''
  m = search_last(text, '\n' + key + ': (.+?)$')
  if m:
    counter = m
  return counter

def get_time(text, key):
  time = '0'
  m = search_last(text, '\n' + key + ': (.+?)s.*$')
  #print "searching for key " + key
  if m:
    #print "found key " + key + ", m = " + m
    time = m
  return float(time)

def get_flag(text, key):
  flag = '0'
  m = search_last(text, '\n' + key + ': (.+?)')
  if m:
    flag = m
  return int(flag) == 1

def find_match_count(text, key):
  return len(re.findall(key, text))

# matches following:
# run_houdini: 3.30353s (num_starts 15)
def get_num_starts(text, key):
  counter = '0'
  time = '0'
  #m = re.search('\n' + key + ': (.*?)s \(num_starts (.*?)\)$', text, re.M)
  m = search_last(text, '\n' + key + ': (.*?)s \((?:num_starts|calls:) (.*?)\)$')
  if m:
    time = m[0]
    counter = m[1]
  return (int(counter), float(time))

def add_cache_counts(text, result):
  result['l1-cache-hits-true'] = get_counter(text, 'expr_list_lhs_prove_rhs.true.cache.hits')
  result['l1-cache-hits-false'] = get_counter(text, 'expr_list_lhs_prove_rhs.false.cache.hits')
  result['l1-cache-misses'] = get_counter(text, 'expr_list_lhs_prove_rhs.cache.misses')
  result['l1-cache-total'] = get_counter(text, 'expr_list_lhs_prove_rhs.queries.total')
  result['l2-cache-total'] = get_counter(text, 'sat-queries')
  result['l2-cache-hits'] = get_counter(text, 'sat-queries-cache-hits')
  solver_queries = result['l2-cache-total'] - result['l2-cache-hits']
  total_queries = result['l2-cache-total'] + result['l1-cache-total'] - result['l1-cache-misses']
  result['solver-queries'] = solver_queries
  result['total-queries'] = total_queries
  result['simplify-cache-total'] = get_counter(text, 'simplify')
  result['simplify-cache-hits'] = get_counter(text, 'simplify-cache-hit')

def add_more_counts(text, result):
  result['nr-equality-check'] = get_counter(text, 'nr-equality-check')
  result['nr-implication-check'] = get_counter(text, 'nr-implication-check')
  result['corr-bt'] = get_counter(text, 'corr-bt')
  result['corr-bt-full-corr-found'] = get_counter(text, 'corr-bt-full-corr-found')
  result['corr-bt-savings'] = get_counter(text, 'corr-bt-savings')
  result['src-nodes'] = get_counter(text, 'src-nodes')
  result['src-edges'] = get_counter(text, 'src-edges')
  result['dst-nodes'] = get_counter(text, 'dst-nodes')
  result['dst-edges'] = get_counter(text, 'dst-edges')
  result['timeout-occured'] = get_counter(text, 'timeout-occured')
  result['GIT_SHA1'] = get_counter_str(text, 'GIT_SHA1')
  result['GIT_DATE'] = get_counter_str(text, 'GIT_DATE')
  result['GIT_BRANCH'] = get_counter_str(text, 'GIT_BRANCH')
  result['max_quota'] = get_counter_str(text, 'max_quota')

def line_visitor_count(line, state_loc):
  state_loc['result'] = state_loc['result'] + 1

# STATE0  --  bef dst
# STATE1  --  '.i0: xchgl' dst started
# STATE2  --  '==' dst stop
def line_visitor_loc(line, state_loc):
  if state_loc['state'] == 2:
    return
  if line.startswith('.i0: xchgl'):
    assert(state_loc['state'] == 0)
    if state_loc['state'] == 0:
      state_loc['state'] = 1
  if line.startswith('=='):
    assert(state_loc['state'] == 1)
    if state_loc['state'] == 1:
      state_loc['state'] = 2
  if state_loc['state'] == 1:
    state_loc['result'] = 2
    state_loc['result-float'] = state_loc['result-float'] or is_float_insn(line)
    state_loc['result-harvester-error'] = state_loc['result-harvester-error'] or is_harvester_error(line)

def line_visitor_solver_times(line, state):
  m = re.search('yices_smt2 won at.*\[(.+?)\]', line)
  if m:
    time = float(m.group(1))
    state['yices'].append(time)
  m = re.search('z3 won at.*\[(.+?)\]', line)
  if m:
    time = float(m.group(1))
    state['z3'].append(time)

def get_opcode_from_line(line):
  lst = line.split(' ')
  assert len(lst) > 0
  if lst[0].startswith('.i'):
    return lst[1]
  else:
    return lst[0]

g_float_opcodes = []
def populate_float_opcodes():
  global g_float_opcodes
  for opc in open('utils/x86-floating-opcodes').read().splitlines():
    if opc not in g_float_opcodes:
      g_float_opcodes.append(opc)

def is_float_insn(line):
  global g_float_opcodes
  opc = get_opcode_from_line(line)
  for fopc in g_float_opcodes:
    if opc.startswith(fopc):
      return True
  return False

def is_harvester_error(line):
  return 'NEXTPC0x7f' in line

def line_visitor_nr_of_constrains(line, state_cons):
  if state_cons['state'] == 2:
    return
  if line.startswith("simulation relation:"):
    state_cons['state'] = 1
  if state_cons['state'] == 1 and line.startswith('Context stats:'):
    state_cons['state'] = 2
  if state_cons['state'] == 1 and line.startswith("condition#"):
    state_cons['result'] += 1

def line_visitor_nr_of_edges_nodes(src_dst, line, state_cons):
  if state_cons['state'] == 3:
    return
  if line.startswith("=={} TFG after preprocess==".format(src_dst)):
    state_cons['state'] = 1
    state_cons['nodes'] = -2
  if state_cons['state'] == 1:
    state_cons['nodes'] += 1
    if line.startswith("signature:"):
      state_cons['state'] = 2
  if state_cons['state'] == 2:
    if line.startswith('=EDGE:'):
      state_cons['edges'] += 1
    if line.startswith('=E0'):
      state_cons['state'] = 3

def get_pass_status(text, suffix):
  #if 'eq-found.' + suffix + ':' in text:
  if 'eq-found' in text:
    pass_status = 'passed'
  else:
    pass_status = 'UNKNOWN'
    if 'Invalid eq file' in text:
      pass_status = 'invalid-eq-file'
    elif 'eq-timeout:' in text or 'received signal 14' in text or 'Equivalence check timed out' in text:
      pass_status = 'TIMEOUT'
    elif 'Safety check failed on CG!' in text:
      pass_status = 'safety-check-failed'
    elif 'DST TFG memlabel check failed on CG!' in text:
      pass_status = 'dst-tfg-memlabel-check-failed'
    elif 'equivalence proof check with relocatable memlabels failed' in text:
      pass_status = 'relocatable-memlabels-check-failed'
    elif 'equivalence proof check failed' in text:
      pass_status = 'eq-proof-check-failed'
    elif 'WARNING : Solver timeout' in text or 'WARNING empty counter example' in text:
      pass_status = 'query-TIMEOUT'
    elif 'multiple possibilities for RODATA symbol' in text:
      pass_status = 'unresolved-RODATA'
    elif 'ml.memlabel_is_atomic() || !ml.memlabel_contains_stack()' in text:
      pass_status = 'non-atomic-stack-memlabel'
    elif 'rand_vals.is_empty()' in text:
      pass_status = 'rand_vals-non-empty'
    elif 'old_preds != m_preds' in text:
      pass_status = 'old_preds-neq-new-preds'
    elif 'Segmentation fault' in text:
      pass_status = 'segfault'
    else:
      m = search_last(text, 'ERROR_(.+?)\n')
      if m:
        pass_status = 'ERROR_' + m
      elif 'Assertion' in text:
        pass_status = 'Assert-fail'
      elif 'Aborted' in text:
        pass_status = 'Aborted'
    pass_status = "FAILED: " + pass_status
  return pass_status

def get_loc(f):
  state_loc = {'state': 0, 'result' : 0, 'result-float': False, 'result-harvester-error': False}
  with open(f, 'r') as fh:
    for line in fh:
      line_visitor_count(line, state_loc)
  #print "get_loc(" + f + ") = " + str(state_loc['result'])
  return state_loc['result']


def proof_uses_local_sprel_expr_mapping(logfile):
  (bench, comp, func, loop) = get_bench_and_type(logfile)
  proof_file = "eqfiles-" + bench + "/" + comp + "/" + func + "-" + loop + ".proof"
  if not os.path.exists(proof_file):
    return 0

  with open(proof_file, 'r') as fh:
    text = fh.read()
  ms = re.findall('\n=local', text, re.M)
  if len(ms) > 0:
    return 1
  else:
    return 0

def compare_file_length(f1, f2):
  count1 = get_loc(f1);
  count2 = get_loc(f2);
  return cmp(count1, count2)

def read_last_n_lines(filename, n):
  return subprocess.check_output(["tail", "-n", str(n), filename])

def read_first_n_lines(filename, n):
  return subprocess.check_output(["head", "-n", str(n), filename], stdout = subprocess.PIPE)

def format_time(time):
  lst = time.split(":")
  if len(lst) == 3:
    h = int(lst[0])
    m = int(lst[1])
    s = int(lst[2])
    secs = s + 60*m + 3600*h
    secs = secs - 7
    return str(secs) + "s"
  else:
    return "-"

def get_result_str(logfile, formatting, ptab = ''):
  res = read_logfile(logfile, ptab)
  pass_status = res['pass_status']
  #loop = res['loop']
  time = res['time']
  passed = pass_status.startswith('passed')
  if formatting:
    return utils.formatted(pass_status, passed, True) + " - " + " [" + str(round(float(time), 2)) + "]"
  else:
    return pass_status + " - " + " [" + str(0) + "]"

def get_bench_and_type(f):
  lst = f.split('/')
  filename = lst[len(lst)-1]
  lst = filename.split('--')
  #print "filename = " + filename
  bench = lst[0].split('-')[1]
  comp = lst[1]
  func = lst[2].split('-')[0]
  loop = lst[2].split('-')[1]
  return (bench, comp, func, loop)
  
def get_filename(xxx_todo_changeme):
  (bench, comp, func, loop) = xxx_todo_changeme
  return 'peeptabs-{}--{}--{}-{}'.format(bench, comp, func, loop)
  
def getfilename(f):
  lst = f.split('/')
  return lst[len(lst)-1]

def init_results():
    return {
            'total' : 0,
            'total-passed' : 0,
            'total-failed' : 0, 
            'time-passed' : 0,
            'time-passed-verify-simrel' : 0,
            'max-loc' : 0,
            'loc-passed' : 0,
            'loc-failed' : 0,
            'vc-string-size' : 0,
            'vc-string-size-zlib' : 0,
            'float-passed' : 0,
            'float-failed' : 0,
            }

def add_to_results(results, res):
  pass_status = res['pass_status']
  #is_float = res['float']
  time = res['time']
  passed = pass_status.startswith('passed')
  loc = res['loc']

  results['total'] += 1 

  if passed:
    #if is_float:
    #  results['float-passed'] += 1 
    results['total-passed'] += 1 
    results['loc-passed'] += loc
    #results['time-passed-verify-simrel'] += res['verify-time']
    #results['vc-string-size'] += res['vc-string-size']
    #results['vc-string-size-zlib'] += res['vc-string-size-zlib']
    if loc > results['max-loc']:
      results['max-loc'] += loc
    results['time-passed'] += time
  else:
    #if is_float:
    #  results['float-failed'] += 1 
    results['total-failed'] += 1 
    results['loc-failed'] += loc
  if pass_status not in results:
    results[pass_status] = 0
  results[pass_status] += 1
  float_count = 0
  if 'FAILED: ERROR_contains_float_op' in results:
    float_count += results['FAILED: ERROR_contains_float_op']
  if 'FAILED: ERROR_contains_unsupported_op' in results:
    float_count += results['FAILED: ERROR_contains_unsupported_op']
  tot = results['total'] - float_count
  tot_passed = results['total-passed']
  if tot == 0:
    results['% pass'] = -1
  else:
    results['% pass'] = (tot_passed)*100.0/tot
  return results

def init_if_not_done(results, bench, comp):
  if bench not in results:
    results[bench] = {}
  if comp not in results[bench]:
    results[bench][comp] = init_results()

def update_results_map(res, results_map, f):
  key = get_bench_and_type(f)
  if key not in results_map:
    results_map[key] = res
    return results_map
  existing_passed = 'passed' in results_map[key]['pass_status'] 
  curr_passed =  'passed' in res['pass_status']
  new_res = results_map[key]
  if curr_passed or not existing_passed: # use the latest pass result or curr-result if existing failed
    new_res = res
  if not curr_passed and not existing_passed:
    existing_loc = results_map[key]['loc'] 
    curr_loc =  res['loc']
    if curr_loc == 0 and existing_loc != 0:
      new_res = results_map[key]
  results_map[key] = new_res
  return results_map

def process_all_files(files, stats_only):
  files.sort()  # will sort in the order of increasing time 
  results_map = {}
  i = 0
  for f in files:
    res = read_logfile(f)
    #pprint.pprint(res)
    #if not stats_only:
    #  print f + ' ' + get_result_str(f, True)
    #sys.stderr.write("doing {}/{}\n".format(i, len(files)))
    results_map = update_results_map(res, results_map, f)
    i += 1
  return results_map

def get_pass_status_from_multiple_log_files(files):
  results_map = process_all_files(files, False)
  assert(len(results_map) == 1)
  for key in results_map:
    return results_map[key]['pass_status']
  return ''

def merge_results_by_new_key(results_map, new_key_fun):
  results = {}
  for (bench, comp, func, loop) in results_map:
    new_key = new_key_fun((bench, comp, func, loop))
    res = results_map[(bench, comp, func, loop)]
    if new_key not in results:
      results[new_key] = init_results()
    results[new_key] = add_to_results(results[new_key], res)
  return results

def remove_lib_files(files):
  files_ret = []
  lib_funs = open('utils/lib_functions_to_be_ignored', 'r').read()
  for f in files:
    (bench, comp, func, loop) = get_bench_and_type(f)
    if func not in lib_funs:
      files_ret.append(f)
  return files_ret

def compare_res_list(item1, item2):
  return cmp(item1[1]['loc'], item2[1]['loc'])

def print_results_map(results_map):
  res_list = list(results_map.items())
  res_list.sort(compare_res_list)
  for item in res_list:
    print(('{}  {}  loc={}'.format(get_filename(item[0]), item[1]['pass_status'], item[1]['loc'])))

def get_all_required_logfiles(results_map):
  log_files = []
  for key in results_map:
    log_files.append(results_map[key]['log-file'])
  return log_files

def gen_res_all_map_file(results_map):
  with open('res-all-map', 'w') as f:
    pprint.pprint(results_map, f)

def update_manual_results(results_map):
  with open('res-all-map-manual', 'r') as fh:
    results_map_manual = eval(fh.read())
    for k in results_map_manual:
      results_map[k]['pass_status'] = results_map_manual[k]['pass_status']

#def remove_old_log_files(log_files_to_keep, files):
#  for f in files:
#    if f not in log_files_to_keep:
#      f_new = f.replace('cache-results-after-pldi16', 'junk-cache-results-after-pldi16')
#      shutil.move(f, f_new)

def move_log_files(passing, files, prefix):
  for n in passing:
    fs = glob.glob("cache-results-pldi2018/" + n + '.*')
    for f in fs:
      f_new = prefix + '-' + f
      shutil.move(f, f_new)

def get_all_passing_failing_logfiles(results_map):
  log_files_passing = []
  log_files_failing = []
  for key in results_map:
    if results_map[key]['pass_status'].startswith("passed"):
      log_files_passing.append(results_map[key]['log-file'])
    else:
      log_files_failing.append(results_map[key]['log-file'])
  return (log_files_passing, log_files_failing)

#def get_ptabs():
#  ret = {}
#  for f in glob.glob("peeptabs-*/*/*.ptab"):
#    k = get_bench_and_type(eq_cache.path_to_cache_id(utils.remove_ext(f)))
#    ret[k] = {'pass_status': 'FAILED_NOT_CHECKED', 'loc': get_loc(f), 'time': 18000}
#  return ret

def get_eqfiles():
  ret = {}
  for f in glob.glob("eqfiles-*/*/*.eq"):
    k = get_bench_and_type(eq_cache.path_to_cache_id(utils.remove_ext(f)))
    ret[k] = {'pass_status': 'FAILED_NOT_CHECKED', 'loc': get_loc(f), 'time': 18000}
  return ret



def update_res_map_with_ptabs(results_map, ptabs):
  ret = results_map
  for k in ptabs:
    if k not in results_map:
      ret[k] = ptabs[k]
  return ret 

def make_uniform_names(xxx_todo_changeme9):
  (bench, comp, func, loop) = xxx_todo_changeme9
  bench_ret, comp_ret, func_ret, loop_ret = (bench, comp, func, loop)
  if('gcc48-O0' in comp and 'gcc48-O2' in comp):
    comp_ret = 'gcc2'
  elif('gcc48-O0' in comp and 'gcc48-O3' in comp):
    comp_ret = 'gcc3'
  elif('icc-O0' in comp and 'icc-O2' in comp):
    comp_ret = 'icc2'
  elif('icc-O0' in comp and 'icc-O3' in comp):
    comp_ret = 'icc3'
  elif('clang36-O0' in comp and 'clang36-O2' in comp):
    comp_ret = 'clang2'
  elif('clang36-O0' in comp and 'clang36-O3' in comp):
    comp_ret = 'clang3'
  elif('ccomp-O0' in comp and 'ccomp-O2' in comp):
    comp_ret = 'ccomp2'
  elif('ccomp-O0' in comp and 'gcc48-O2' in comp):
    comp_ret = 'cg02'
  elif('ccomp-O0' in comp and 'gcc48-O3' in comp):
    comp_ret = 'cg03'
  if loop_ret != 'A' and  loop_ret != 'C':
    loop_ret = 'A'
  return (bench_ret, comp_ret, func_ret, loop_ret)

def make_uniform_keys(results_map):
  ret = dict()
  for k in results_map:
    ret[make_uniform_names(k)] = results_map[k]
  return ret

if __name__ == "__main__":
  parser = argparse.ArgumentParser()
  parser.add_argument("files", help = "specify files in glob format", type = str)
  parser.add_argument('--stats-only', help = "print only final stats", action='store_true')
  parser.add_argument('--gen-res-all-map', help = "generate res-all-map file", action='store_true')
  parser.add_argument('--prune-log-files', help = "prune unnecessary log files", action='store_true')
  parser.add_argument('--split-log-files', help = "split log files into failed and passed", action='store_true')

  args = parser.parse_args()

  #populate_float_opcodes()
  files = glob.glob(args.files)
  files = utils.de_dup(utils.remove_ext_in_list(files))
  #files = remove_lib_files(files)
  results_map = process_all_files(files, args.stats_only)
  #print results_map
  #ptabs = get_ptabs()
  eqfiles = get_eqfiles()
  print_results_map(results_map)
  #results_map = update_res_map_with_ptabs(results_map, ptabs)
  results_map = update_res_map_with_ptabs(results_map, eqfiles)
  results_map = make_uniform_keys(results_map)
  ##update_manual_results(results_map)
  print_results_map(results_map)
  pp = pprint.PrettyPrinter()
  pp.pprint(merge_results_by_new_key(results_map, lambda bench_comp_func_loop : (bench_comp_func_loop[0], bench_comp_func_loop[1], bench_comp_func_loop[3])))
  pp.pprint(merge_results_by_new_key(results_map, lambda bench_comp_func_loop1 : (bench_comp_func_loop1[0], bench_comp_func_loop1[1])))
  pp.pprint(merge_results_by_new_key(results_map, lambda bench_comp_func_loop2 : (bench_comp_func_loop2[1])))
  pp.pprint(merge_results_by_new_key(results_map, lambda bench_comp_func_loop3 : (bench_comp_func_loop3[0])))
  pp.pprint(merge_results_by_new_key(results_map, lambda bench_comp_func_loop4 :  'ALL-O2-no-cg' if '2' in bench_comp_func_loop4[1] and 'cg' not in bench_comp_func_loop4[1] else 'junk'))
  pp.pprint(merge_results_by_new_key(results_map, lambda bench_comp_func_loop5 :  'ALL-O3-no-cg' if '3' in bench_comp_func_loop5[1] and 'cg' not in bench_comp_func_loop5[1] else 'junk'))
  pp.pprint(merge_results_by_new_key(results_map, lambda bench_comp_func_loop6 :  'ALL-no-cg' if 'cg' not in bench_comp_func_loop6[1] else 'junk'))
  pp.pprint(merge_results_by_new_key(results_map, lambda bench_comp_func_loop7 :  'ALL-O2' if '2' in bench_comp_func_loop7[1] else 'ALL-O3'))
  pp.pprint(merge_results_by_new_key(results_map, lambda bench_comp_func_loop8 : 'ALL'))
  if args.gen_res_all_map:
    gen_res_all_map_file(results_map)
  #if args.prune_log_files:
  #  log_files_to_keep = get_all_required_logfiles(results_map)
  #  remove_old_log_files(log_files_to_keep, files)
  if args.split_log_files:
    passing, failing = get_all_passing_failing_logfiles(results_map)
    move_log_files(passing, files, "passing")
    move_log_files(failing, files, "failing")

