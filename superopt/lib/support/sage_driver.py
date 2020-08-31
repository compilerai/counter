#!/usr/bin/env python3

import os
import sys
import struct
import pexpect.replwrap

######### GLOBALS #########

# TODO read config from config-in

RECV_SIZE = 1024 # SAGE_HELPER_PROCESS_MSG_SIZE
OK = 0
OK_EMPTY = 1
TIMEOUT = 2
ERROR = 3

def new_sage_instance():
    # spawn sage -- unfortunately there seems to be no better way for killing stderr than starting a shell and redirecting it
    sage = pexpect.replwrap.REPLWrapper('/bin/bash -c "sage -python 2>/dev/null"', u">>> ", None)
    # invoke initial code
    header = "execfile('${CMAKE_BINARY_DIR}/bv_solve_template.py')"
    assert sage.run_command(header) == ""
    return sage

######### MAIN #########

assert len(sys.argv) == 3
input_fd = int(sys.argv[1])
output_fd = int(sys.argv[2])

sage = new_sage_instance()

# main loop -- read, evaluate, write back (and repeat)
while True:
    # READ -- read input message from caller

    # read message of RECV_SIZE from input_fd
    while True:
        try:
            msg = os.read(input_fd, RECV_SIZE)
            if len(msg) == 0: # EOF
                sys.exit(1)
            break
        except InterruptedError:
            pass

    # PREPARE -- prepare input to be passed to solver

    # timeout is encoded as last 4 bytes in msg
    timeout_secs = struct.unpack("<L", msg[-4:])[0]
    # TODO pass the full cmdline in msg only?
    # filename is terminated by NUL byte
    try:
        fname = msg[:-4].decode('utf-8').rstrip("\0")
    except Exception as e:
        sys.stderr.write("Exception: " + str(e))
        sys.stderr.write("msg: " + repr(msg))
        sys.exit(1)

    cmdline = ""
    with open(fname, "r") as fin:
        # Expected format: `points` in one line followed by `m`.
        #      [ [ <number> ... ], [ ... ], ... ]
        #      <number>
        cmdline = (",".join(fin.readlines())).replace("\n", "")

    if len(cmdline) == 0:
        sys.stderr.write("WARNING: Empty file")
        continue

    # EVALUATE -- pass to solver

    cmdline = "bv_solve_wrapper(" + cmdline + ")"
    # print("Sending cmdline " + cmdline + " with timeout " + str(timeout_secs))
    try:
        output = sage.run_command(cmdline, timeout=timeout_secs)
    except pexpect.exceptions.TIMEOUT as exp:
        res = TIMEOUT
        sys.stderr.write(repr(exp))
    else:
        output = output.strip()
        if output == "[]":
            res = OK_EMPTY
        elif len(output) and output[0] == "[":
            res = OK
        else:
            # cannot detect python exceptions/errors as stderr is being redirected
            res = ERROR
            sys.stderr.write("ERROR: received unknown output from sage: `"+ output +"'\n")

    if res == TIMEOUT or res == ERROR:
        # kill this instance and restart sage
        del sage
        sage = new_sage_instance()


    # WRITEBACK -- return result to caller

    if res == OK:
        assert os.write(output_fd, b"result\0\0") == 8      # send 8-byte header
        count = struct.pack("@L", len(output))
        assert os.write(output_fd, count) == len(count)
        assert os.write(output_fd, output.encode('utf-8')) == len(output)
    elif res == OK_EMPTY:
        assert os.write(output_fd, b"none\0\0\0\0") == 8
    elif res == TIMEOUT:
        assert os.write(output_fd, b"timeout\0") == 8
    elif res == ERROR:
        assert os.write(output_fd, b"error\0\0\0") == 8

