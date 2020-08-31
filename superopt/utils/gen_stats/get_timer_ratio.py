#!/usr/bin/python3

from os.path import basename
import argparse
import time

from parse_eqlog_file import *

def get_timer_value(timers, pfx):
  ret = None
  for k,v in timers.items():
      if k.startswith(pfx):
          if ret is not None:
              raise ValueError("Multiple matches for argument `{}'".format(pfx))
          ret = v.time
  if ret is None:
      raise ValueError("Timer `{}' not found in timers".format(pfx))
  return ret

def main():
  parser = argparse.ArgumentParser(description="Get time taken by two eqlog timers and compute their ratio")
  parser.add_argument("timer1", help = 'timer1', type=str)
  parser.add_argument("timer2", help = 'timer2', type=str, default="total-equiv")
  parser.add_argument("eqlog_file", help = '.eqlog filename', type=str, nargs='+')

  args = parser.parse_args()
  timer1 = args.timer1
  timer2 = args.timer2
  for eqlog_file in args.eqlog_file:
    fnStats = read_eqlog_stats_for_function(open(eqlog_file, "r"))

    t1 = get_timer_value(fnStats.timers, timer1)
    t2 = get_timer_value(fnStats.timers, timer2)
    
    print("{} {:.2f} %, {} / {}".format(basename(eqlog_file), t1*100.0/t2, t1, t2))

  return 0

if __name__ == '__main__':
  exit(main())
