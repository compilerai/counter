#pragma once
#include <stdbool.h>

void yices_initialize(void);
bool yices_test_smt2_expression(char const *filename);
void yices_done(void);
