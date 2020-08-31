import re # for re._pattern_type

def find_line_starting_with(fin, pfx, msg):
  for l in fin:
    if l.startswith(pfx):
      return l
  else:
      raise ValueError("{}: {} not found.\nCould not match expected prefix: {}\n".format(fin.name, msg, pfx))

def find_line_starting_with_regex(fin, pfx, msg):
  for l in fin:
    if pfx.match(l):
      return l
  else:
      raise ValueError("{}: {} not found.\nCould not match expected prefix: {}\n".format(fin.name, msg, pfx.pattern))

def get_values_with_key(dictlike, keypattern):
  if isinstance(keypattern, str):
    matches = lambda x: x.startswith(keypattern)
  elif isinstance(keypattern, re._pattern_type):
    matches = lambda x: keypattern.match(x)
  ret = []
  for k,v in dictlike.items():
    if matches(k):
      ret.append(v)
  return ret

def parse_time_num_starts(s):
  time_str, num_starts_str = s.strip().split(" (")
  time_str = time_str.strip()[:-1] # remove 's'
  num_starts_str = num_starts_str.split(" ")[1][:-1] # remove ")"
  return float(time_str), int(num_starts_str)

def avg(l):
  if len(l) == 0:
    return 0
  assert len(l) > 0
  return sum(l)/len(l)

def median(l):
  if len(l) == 0:
    return float('NaN')
  assert len(l) > 0
  return sorted(l)[(len(l)-1)//2] # lower median

def mmax(l):
  if len(l) == 0:
    return float('NaN')
  assert len(l) > 0
  return max(l)
