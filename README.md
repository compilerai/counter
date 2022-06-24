# StaticPersist -- Compiler Support for PMEM Programming

# Table of contents
1. [Getting started](#getting-started) 
   1. [Ideal machine configuration](#machine-config)
   2. [Setup](#setup)
   3. [Running the compile-time reachability tool on the tests](#run)


# Getting started <a name="getting-started"></a>

## Ideal machine configuration <a name="machine-config"></a>

An ideal machine for running this artifact would have at least:

 * 8 physical CPUs
 * 32 GiB of RAM
 * 60 GiB disk space
 * Broadband connection

## Setup <a name="setup"></a>

1. Clone the repository to a directory with the path `/home/sbansal/counter`
2. Checkout the static-persist branch
   ```
   git checkout static-persist
   ```
3. Build
   ```
   make
   ```

## Run <a name="run"></a>
1. Run the compile-time analysis tool on a C or C++ file while supplying a tunable `call-context-depth` parameter
   ```
   /home/sbansal/counter/superopt/build/etfg_x64/identify_durables --call-context-depth 2 /home/sbansal/couunter/tests/c/linked_list.c
   ```
