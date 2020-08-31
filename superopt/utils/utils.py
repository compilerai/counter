import sys
import os
import glob

def formatted(string, passed, bold):
  attr = []
  if passed:
      # green
      attr.append('32')
  else:
      # red
      attr.append('31')
  if bold:
      attr.append('1')

  if sys.stdout.isatty():
    return '\x1b[%sm%s\x1b[0m' % (';'.join(attr), string)
  else:
    return string

def change_ext(f, ext):
  return os.path.splitext(f)[0] + '.' + ext

def get_ext(f):
  e = os.path.splitext(f)[1]
  if len(e) > 0:
    return e[1:]
  else:
    return ''

def add_ext(f, ext):
  n_e = os.path.splitext(f)
  assert(n_e[1] == '')
  return f + '.' + ext

def remove_ext(f):
  return os.path.splitext(f)[0]

def remove_ext_in_list(fs):
  ret = []
  for f in fs:
    ret.append(remove_ext(f))
  return ret

def filter_files_with_ext(fs, ext):
  ret = []
  for f in fs:
    if get_ext(f) == ext:
      ret.append(f)
  return ret

def de_dup(l):
  return list(set(l))

def make_dir(dir_path):
  if not os.path.exists(dir_path):
    os.makedirs(dir_path)

def path_to_filename(f):
  lst = f.split('/')
  return lst[len(lst)-1]

def merge_multiple_globs(gs):
  ret = []
  for g in gs:
    ret += glob.glob(g) 
  return de_dup(ret)
