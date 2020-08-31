#ifndef GAS_UNIVERSAL_H
#define GAS_UNIVERSAL_H

#include "config-host.h"

struct expressionS;
extern int gas_assemble_mode;

#define md_parse_name(name, exp, mode, c) universal_parse_name (name, exp, c)
int universal_parse_name (const char *name, struct expressionS *exp, char *nextcharP);

#endif
