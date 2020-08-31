superopt-tests
==============

Collection of test suites for superopt.

## Building

```sh
# first time configuration
./configure

# for building all test suites
make

# Or, for a particular test suite
make <dirname>

# For example, for building bzip2 the following command would be used
make bzip2
```

## Generating testcases .etfg and .tfg files

```sh
# for all
make gentest

# Or, for a particular test suite
make -C build/<dirname> gentest

# For example, for generating bzip2 test files
make -C build/bzip2 gentest
```

## Running tests

```sh
# for all
make runtest

# Or, for a particular test suite
make -C build/<dirname> runtest

# For example, for running bzip2 test
make -C build/bzip2 runtest
```

## Cleaning-up

For restoring the pristine state use:
```sh
make distclean
```

For deleting only the compiled files, use:
```sh
make clean
```

## Editing paths

All paths are set in `Make.conf`.

In addition, some environment variables are used for some paths.  The exhaustive list of environment variables used are:

1. `SUPEROPT_PROJECT_DIR`: Directory where `superopt-project` is cloned.

2. `ICC_INSTALL_DIR`: Directory where ICC is installed.  `icc` is then located to `${ICC_INSTALL_DIR}/bin/icc`.

3. `COMPCERT_INSTALL_BIN`: Directory where CompCert is installed.  `ccomp` is then located to `${COMPCERT_INSTALL_BIN}/ccomp`

4. `COMPCERT_INSTALL_LIB`: Search path for CompCert libs.

