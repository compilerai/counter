# StaticPersist -- Compiler Support for PMEM Programming

# Table of contents
1. [Getting started](#getting-started) 
   1. [Ideal machine configuration](#machine-config)
   2. [Setup](#setup)
   3. [Running equivalence checker for an example function](#run-paper-ex)
2. [Step-by-Step instructions for running the benchmarks](#running-instructions)
   1. [Directory Structure](#path-info)
   2. [Reproducing the Table 2 results](#table2-gen)
   3. [Running a particular benchmark category with either Best-First or Depth-First Strategy](#individual-results)
   4. [Reproducing the discovered bug in diet libc library](#run-dietlibc-ex)
   5. [Running equivalence checker on custom code](#run-custom-code)
3. [HOWTO: Interpret the input and output files](#howto-IO)
   1. [Interpreting the input CFG (.tfg and .etfg) files](#howto-interpret-tfg)
   2. [Interpreting the output product-CFG (.proof) file](#howto-interpret-proof)
   3. [Interpreting the output log (.eqlog) file](#howto-interpret-eqlog)
4. [CounterTV code walkthrough](#code-comments)
5. [Archived output (.eqlog and .proof) files](#archived-files)


# Getting started <a name="getting-started"></a>

## Ideal machine configuration <a name="machine-config"></a>

An ideal machine for running this artifact would have at least:

 * 8 physical CPUs
 * 32 GiB of RAM
 * 60 GiB disk space
 * Broadband connection

## Setup <a name="setup"></a>

1. Clone the repository to a directory with the path `/home/sbansal/counter`
2. Build
   ```
   make
   ```
3. Run the compile-time analysis tool on a C or C++ file while supplying a tunable `call-context-depth` parameter
   ```
   /home/sbansal/counter/superopt/build/etfg_x64/identify_durables --call-context-depth 2 /home/sbansal/couunter/tests/c/linked_list.c
   ```
