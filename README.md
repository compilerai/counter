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

The artifact is packaged as a Docker application.  Installation of Docker is covered [here](https://docs.docker.com/engine/install/).
Follow these steps for building and running the equivalence checker based on CounterTV:

1. Clone the repository to a directory.
2. Checkout the static-persist branch
   ```
   git checkout static-persist
   ```
3. [Install Docker Engine](https://docs.docker.com/engine/install/) and set it up.  Make sure you are able to run the [hello-world example](https://docs.docker.com/get-started/#test-docker-installation).
4. Bild the Docker image.  Note that internet connectivity is required in this step.
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

## Run <a name="run"></a>

1. (Inside the container) Run the compile-time analysis tool on a C or C++ file while supplying a tunable `call-context-depth` parameter
   ```
   /home/sbansal/counter/superopt/build/etfg_x64/identify_durables --call-context-depth 2 /home/sbansal/couunter/tests/c/linked_list.c
   ```
