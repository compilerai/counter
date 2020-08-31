#pragma once
#include "support/types.h"

extern "C" {
void lockstep_register_fun(char const* fun_name, int64_t is_small_function);
void lockstep_register_bbl(char const* bbl_name, int64_t is_small_function);
void lockstep_register_fcall();
void lockstep_register_inst(char const* name, int64_t typID, int64_t nbits, uint64_t val, int64_t t, int64_t nargs, uint64_t const* args, uint64_t const* arg_nbits);
void lockstep_clear_fun(int64_t is_main, int64_t is_small_function);
}
