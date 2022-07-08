# StaticPersist -- Compiler Support for PMEM Programming

# Table of contents
1. [Getting started](#getting-started) 
   1. [Ideal machine configuration](#machine-config)
   2. [Setup](#setup)
   3. [Running the compile-time reachability tool on the tests](#run)
   3. [Understanding the output of the tool ](#understand)


# Getting started <a name="getting-started"></a>

## Ideal machine configuration <a name="machine-config"></a>

An ideal machine for running this artifact would have at least:

 * 8 physical CPUs
 * 32 GiB of RAM
 * 60 GiB disk space
 * Broadband connection

## Setup <a name="setup"></a>

The artifact is packaged as a Docker application.  Installation of Docker is covered [here](https://docs.docker.com/engine/install/).
Follow these steps for building and running the equivalence checker based on CounterTV:

1. Clone the repository to a directory.
2. Checkout the static-persist branch
   ```
   git checkout static-persist
   ```

You have two options: either build inside a Docker container (in which case you will not have to worry about dependency issues, etc.) or build on a your own machine which has Ubuntu-20.04 installed on it.  In the latter case, you need to ensure that you have cloned this repository at `/home/sbansal/counter` (path of the repository).

### Build Using Docker
3. [Install Docker Engine](https://docs.docker.com/engine/install/) and set it up.  Make sure you are able to run the [hello-world example](https://docs.docker.com/get-started/#test-docker-installation).
4. Build the Docker image.  Note that internet connectivity is required in this step.
   ```
   docker build -t static-persist .
   ```
   This process can take a while depending upon your internet connection bandwidth.  
5. Run a container with the built image.
   ```
   docker run -it static-persist
   ```
6. (Inside the container) Build the artifact and install the equivalence checker.
   ```
   make
   ```

### Build on Ubuntu 20.04 in /home/sbansal/counter
3. Build the artifact and install the equivalence checker.
   ```
   make
   ```

## Run <a name="run"></a>

1. (Inside the container) Run the compile-time analysis tool on a C or C++ file while supplying a tunable `call-context-depth` parameter
   ```
   /home/sbansal/counter/superopt/build/etfg_x64/identify_durables --call-context-depth 2 /home/sbansal/counter/tests/c/linked_list.c
   ```

2. The tool should produce the following output:
   ```
   ...  <snipped> ...

   Durable regions:
   objects allocated at: {
     main : line 51
     insert2 : (line 32 at column 34)
   }
   objects allocated at: {
     main : line 51
     insert2 : (line 36 at column 13)
   }
   global variable 'head2'
   ```

The source code for the `linked_list.c` program can be found in `tests/c/linked_list.c`.

## Understanding the output of the tool <a name="understand"></a>

1. The programmer can annotate global variables as _durable roots_ by using the `pmem_` prefix in their names.  For example, the `pmem_root` global variable in `tests/c/binary_search_tree.c`
2. The programmer could also annotate heap allocation statements as _durable roots_ by using the `pmem_malloc()` function.  For example, the use of `pmem_malloc()` in the `insert2()` function in `tests/c/linked_list.c` indicates that all allocations made through that statement would be allocated in persistent memory.
3. Upon running the tool on the source code file, it outputs all the allocation statements (represented as bounded-depth call-stacks) that are reachable through the annotated durable roots.
4. The call-context-depth parameter controls the maximum depth of the call-stacks used to identify the allocation statements.  For example, a call-context-depth of 2 will produce bounded-depth call-stacks of depth 2 or smaller.  Deeper call-stacks usually produce more precise results.

Considering the example from previous section, the output of the tool indicates that there are three allocations which are reachable through durable roots:
1. The object allocated by following the call-chain `main()` line 51, `insert2()` line 32.
   This allocation is annotated using `pmem_malloc()` as a durable root and thus is a durable root itself.
2. The object allocated by following the call-chain `main()` line 51, `insert2()` line 36.
   This allocation is not annotated, however, it is reachable through `n` which is annotated as durable root.
3. The global variable `head2`.
   The call to `insert2()` in `main()` (line 51) is passing `head2` as a parameter.  The corresponding formal parameter `h` in `insert2()` can be assigned a durable root (line 39), hence it should be allocated in persistent memory.

It can be verified that no other allocations is reachable through durable roots in this program.
