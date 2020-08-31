#!/usr/bin/python3

import argparse
import os

def main():
  parser = argparse.ArgumentParser()
  parser.add_argument("harvest", help = 'harvest filename', type=str)
  args = parser.parse_args()

  harvest_orig = args.harvest
  harvest = os.path.realpath(harvest_orig)

  with open(harvest, mode='r') as fin:
    line = fin.readline()
    while line != '':
      assert line.startswith(".i0:") # start of new function
      ln = 1
      while True:
        line = fin.readline()
        if line.startswith("--"):
          break
        assert line.startswith(".i{}".format(ln))
        ln += 1
      line = fin.readline()
      assert line.startswith("live :")
      line = fin.readline()
      assert line.startswith("memlive :")
      line = fin.readline()
      assert line.startswith("== ")
      fn = line.split(':')[0].split(' ')[-1]
      print(("{}: {}".format(fn, ln)))
      line = fin.readline()
    
if __name__ == "__main__":
  main()
