#define GAS_COMMON_ARCH_PPC
#include "config-host.h"
#include <bfd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
//#include "gas/gas_common.h"
#include "gas/opcode-ppc.h"
#include "gas/common/common.h"
#include "gas/common/as.h"
#include "gas/common/expr.h"
#include "gas/common/safe-ctype.h"
#include "gas/tc-ppc.h"
#include "support/debug.h"
#include "gas/universal.h"

typedef bool boolean;
/* tc-ppc.c -- Assemble for the PowerPC or POWER (RS/6000)
   Copyright 1994, 1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002
   Free Software Foundation, Inc.
   Written by Ian Lance Taylor, Cygnus Support.

   This file is part of GAS, the GNU Assembler.

   GAS is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   GAS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GAS; see the file COPYING.  If not, write to the Free
   Software Foundation, 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */

#include <stdio.h>

/* This is the assembler for the PowerPC or POWER (RS/6000) chips.  */

/* Tell the main code what the endianness is.  */
extern int target_big_endian;

/* Whether or not, we've set target_big_endian.  */
static int set_target_endian = 0;

/* Whether to use user friendly register names.  */
#ifndef TARGET_REG_NAMES_P
#define TARGET_REG_NAMES_P false
#endif

/* Macros for calculating LO, HI, HA, HIGHER, HIGHERA, HIGHEST,
   HIGHESTA.  */

/* #lo(value) denotes the least significant 16 bits of the indicated.  */
#define PPC_LO(v) ((v) & 0xffff)

/* #hi(value) denotes bits 16 through 31 of the indicated value.  */
#define PPC_HI(v) (((v) >> 16) & 0xffff)

/* #ha(value) denotes the high adjusted value: bits 16 through 31 of
  the indicated value, compensating for #lo() being treated as a
  signed number.  */
#define PPC_HA(v) PPC_HI ((v) + 0x8000)

/* #higher(value) denotes bits 32 through 47 of the indicated value.  */
#define PPC_HIGHER(v) (((v) >> 16 >> 16) & 0xffff)

/* #highera(value) denotes bits 32 through 47 of the indicated value,
   compensating for #lo() being treated as a signed number.  */
#define PPC_HIGHERA(v) PPC_HIGHER ((v) + 0x8000)

/* #highest(value) denotes bits 48 through 63 of the indicated value.  */
#define PPC_HIGHEST(v) (((v) >> 24 >> 24) & 0xffff)

/* #highesta(value) denotes bits 48 through 63 of the indicated value,
   compensating for #lo being treated as a signed number.  */
#define PPC_HIGHESTA(v) PPC_HIGHEST ((v) + 0x8000)

#define SEX16(val) ((((val) & 0xffff) ^ 0x8000) - 0x8000)

static boolean reg_names_p = TARGET_REG_NAMES_P;

static boolean register_name PARAMS ((expressionS *));
static unsigned long ppc_insert_operand
  PARAMS ((unsigned long insn, const struct powerpc_operand *operand,
	   offsetT val, char *file, unsigned int line));
static void ppc_macro PARAMS ((char *str, const struct powerpc_macro *macro));
static void ppc_byte PARAMS ((int));



/* Characters which are used to indicate an exponent in a floating
   point number.  */
static const char EXP_CHARS[] = "eE";

/* Characters which mean that a number is a floating point constant,
   as in 0d1.0.  */
static const char FLT_CHARS[] = "dD";


const pseudo_typeS md_pseudo_table[] =
{
  /* Pseudo-ops which must be overridden.  */
  { "byte",	ppc_byte,	0 },

#ifdef OBJ_XCOFF
  /* Pseudo-ops specific to the RS/6000 XCOFF format.  Some of these
     legitimately belong in the obj-*.c file.  However, XCOFF is based
     on COFF, and is only implemented for the RS/6000.  We just use
     obj-coff.c, and add what we need here.  */
  { "comm",	ppc_comm,	0 },
  { "lcomm",	ppc_comm,	1 },
  { "bb",	ppc_bb,		0 },
  { "bc",	ppc_bc,		0 },
  { "bf",	ppc_bf,		0 },
  { "bi",	ppc_biei,	0 },
  { "bs",	ppc_bs,		0 },
  { "csect",	ppc_csect,	0 },
  { "data",	ppc_section,	'd' },
  { "eb",	ppc_eb,		0 },
  { "ec",	ppc_ec,		0 },
  { "ef",	ppc_ef,		0 },
  { "ei",	ppc_biei,	1 },
  { "es",	ppc_es,		0 },
  { "extern",	ppc_extern,	0 },
  { "function",	ppc_function,	0 },
  { "lglobl",	ppc_lglobl,	0 },
  { "rename",	ppc_rename,	0 },
  { "section",	ppc_named_section, 0 },
  { "stabx",	ppc_stabx,	0 },
  { "text",	ppc_section,	't' },
  { "toc",	ppc_toc,	0 },
  { "long",	ppc_xcoff_cons,	2 },
  { "llong",	ppc_xcoff_cons,	3 },
  { "word",	ppc_xcoff_cons,	1 },
  { "short",	ppc_xcoff_cons,	1 },
  { "vbyte",    ppc_vbyte,	0 },
#endif

#ifdef OBJ_ELF
  { "llong",	ppc_elf_cons,	8 },
  { "quad",	ppc_elf_cons,	8 },
  { "long",	ppc_elf_cons,	4 },
  { "word",	ppc_elf_cons,	2 },
  { "short",	ppc_elf_cons,	2 },
  { "rdata",	ppc_elf_rdata,	0 },
  { "rodata",	ppc_elf_rdata,	0 },
  { "lcomm",	ppc_elf_lcomm,	0 },
  { "file",	(void (*) PARAMS ((int))) dwarf2_directive_file, 0 },
  { "loc",	dwarf2_directive_loc, 0 },
#endif

#ifdef TE_PE
  /* Pseudo-ops specific to the Windows NT PowerPC PE (coff) format.  */
  { "previous", ppc_previous,   0 },
  { "pdata",    ppc_pdata,      0 },
  { "ydata",    ppc_ydata,      0 },
  { "reldata",  ppc_reldata,    0 },
  { "rdata",    ppc_rdata,      0 },
  { "ualong",   ppc_ualong,     0 },
  { "znop",     ppc_znop,       0 },
  { "comm",	ppc_pe_comm,	0 },
  { "lcomm",	ppc_pe_comm,	1 },
  { "section",  ppc_pe_section, 0 },
  { "function",	ppc_pe_function,0 },
  { "tocd",     ppc_pe_tocd,    0 },
#endif

#if defined (OBJ_XCOFF) || defined (OBJ_ELF)
  { "tc",	ppc_tc,		0 },
  { "machine",  ppc_machine,    0 },
#endif

  { NULL,	NULL,		0 }
};


/* Predefined register names if -mregnames (or default for Windows NT).
   In general, there are lots of them, in an attempt to be compatible
   with a number of other Windows NT assemblers.  */

/* Structure to hold information about predefined registers.  */
struct pd_reg
  {
    char *name;
    int value;
  };

/* List of registers that are pre-defined:

   Each general register has predefined names of the form:
   1. r<reg_num> which has the value <reg_num>.
   2. r.<reg_num> which has the value <reg_num>.

   Each floating point register has predefined names of the form:
   1. f<reg_num> which has the value <reg_num>.
   2. f.<reg_num> which has the value <reg_num>.

   Each vector unit register has predefined names of the form:
   1. v<reg_num> which has the value <reg_num>.
   2. v.<reg_num> which has the value <reg_num>.

   Each condition register has predefined names of the form:
   1. cr<reg_num> which has the value <reg_num>.
   2. cr.<reg_num> which has the value <reg_num>.

   There are individual registers as well:
   sp or r.sp     has the value 1
   rtoc or r.toc  has the value 2
   fpscr          has the value 0
   xer            has the value 1
   lr             has the value 8
   ctr            has the value 9
   pmr            has the value 0
   dar            has the value 19
   dsisr          has the value 18
   dec            has the value 22
   sdr1           has the value 25
   srr0           has the value 26
   srr1           has the value 27

   The table is sorted. Suitable for searching by a binary search.  */

static const struct pd_reg pre_defined_registers[] =
{
  { "cr.0", 0 },    /* Condition Registers */
  { "cr.1", 1 },
  { "cr.2", 2 },
  { "cr.3", 3 },
  { "cr.4", 4 },
  { "cr.5", 5 },
  { "cr.6", 6 },
  { "cr.7", 7 },

  { "cr0", 0 },
  { "cr1", 1 },
  { "cr2", 2 },
  { "cr3", 3 },
  { "cr4", 4 },
  { "cr5", 5 },
  { "cr6", 6 },
  { "cr7", 7 },

  { "ctr", 9 },

  { "dar", 19 },    /* Data Access Register */
  { "dec", 22 },    /* Decrementer */
  { "dsisr", 18 },  /* Data Storage Interrupt Status Register */

  { "f.0", 0 },     /* Floating point registers */
  { "f.1", 1 },
  { "f.10", 10 },
  { "f.11", 11 },
  { "f.12", 12 },
  { "f.13", 13 },
  { "f.14", 14 },
  { "f.15", 15 },
  { "f.16", 16 },
  { "f.17", 17 },
  { "f.18", 18 },
  { "f.19", 19 },
  { "f.2", 2 },
  { "f.20", 20 },
  { "f.21", 21 },
  { "f.22", 22 },
  { "f.23", 23 },
  { "f.24", 24 },
  { "f.25", 25 },
  { "f.26", 26 },
  { "f.27", 27 },
  { "f.28", 28 },
  { "f.29", 29 },
  { "f.3", 3 },
  { "f.30", 30 },
  { "f.31", 31 },
  { "f.4", 4 },
  { "f.5", 5 },
  { "f.6", 6 },
  { "f.7", 7 },
  { "f.8", 8 },
  { "f.9", 9 },

  { "f0", 0 },
  { "f1", 1 },
  { "f10", 10 },
  { "f11", 11 },
  { "f12", 12 },
  { "f13", 13 },
  { "f14", 14 },
  { "f15", 15 },
  { "f16", 16 },
  { "f17", 17 },
  { "f18", 18 },
  { "f19", 19 },
  { "f2", 2 },
  { "f20", 20 },
  { "f21", 21 },
  { "f22", 22 },
  { "f23", 23 },
  { "f24", 24 },
  { "f25", 25 },
  { "f26", 26 },
  { "f27", 27 },
  { "f28", 28 },
  { "f29", 29 },
  { "f3", 3 },
  { "f30", 30 },
  { "f31", 31 },
  { "f4", 4 },
  { "f5", 5 },
  { "f6", 6 },
  { "f7", 7 },
  { "f8", 8 },
  { "f9", 9 },

  { "fpscr", 0 },

  { "lr", 8 },     /* Link Register */

  { "pmr", 0 },

  { "r.0", 0 },    /* General Purpose Registers */
  { "r.1", 1 },
  { "r.10", 10 },
  { "r.11", 11 },
  { "r.12", 12 },
  { "r.13", 13 },
  { "r.14", 14 },
  { "r.15", 15 },
  { "r.16", 16 },
  { "r.17", 17 },
  { "r.18", 18 },
  { "r.19", 19 },
  { "r.2", 2 },
  { "r.20", 20 },
  { "r.21", 21 },
  { "r.22", 22 },
  { "r.23", 23 },
  { "r.24", 24 },
  { "r.25", 25 },
  { "r.26", 26 },
  { "r.27", 27 },
  { "r.28", 28 },
  { "r.29", 29 },
  { "r.3", 3 },
  { "r.30", 30 },
  { "r.31", 31 },
  { "r.4", 4 },
  { "r.5", 5 },
  { "r.6", 6 },
  { "r.7", 7 },
  { "r.8", 8 },
  { "r.9", 9 },

  { "r.sp", 1 },   /* Stack Pointer */

  { "r.toc", 2 },  /* Pointer to the table of contents */

  { "r0", 0 },     /* More general purpose registers */
  { "r1", 1 },
  { "r10", 10 },
  { "r11", 11 },
  { "r12", 12 },
  { "r13", 13 },
  { "r14", 14 },
  { "r15", 15 },
  { "r16", 16 },
  { "r17", 17 },
  { "r18", 18 },
  { "r19", 19 },
  { "r2", 2 },
  { "r20", 20 },
  { "r21", 21 },
  { "r22", 22 },
  { "r23", 23 },
  { "r24", 24 },
  { "r25", 25 },
  { "r26", 26 },
  { "r27", 27 },
  { "r28", 28 },
  { "r29", 29 },
  { "r3", 3 },
  { "r30", 30 },
  { "r31", 31 },
  { "r4", 4 },
  { "r5", 5 },
  { "r6", 6 },
  { "r7", 7 },
  { "r8", 8 },
  { "r9", 9 },

  { "rtoc", 2 },  /* Table of contents */

  { "sdr1", 25 }, /* Storage Description Register 1 */

  { "sp", 1 },

  { "srr0", 26 }, /* Machine Status Save/Restore Register 0 */
  { "srr1", 27 }, /* Machine Status Save/Restore Register 1 */

  { "v.0", 0 },     /* Vector registers */
  { "v.1", 1 },
  { "v.10", 10 },
  { "v.11", 11 },
  { "v.12", 12 },
  { "v.13", 13 },
  { "v.14", 14 },
  { "v.15", 15 },
  { "v.16", 16 },
  { "v.17", 17 },
  { "v.18", 18 },
  { "v.19", 19 },
  { "v.2", 2 },
  { "v.20", 20 },
  { "v.21", 21 },
  { "v.22", 22 },
  { "v.23", 23 },
  { "v.24", 24 },
  { "v.25", 25 },
  { "v.26", 26 },
  { "v.27", 27 },
  { "v.28", 28 },
  { "v.29", 29 },
  { "v.3", 3 },
  { "v.30", 30 },
  { "v.31", 31 },
  { "v.4", 4 },
  { "v.5", 5 },
  { "v.6", 6 },
  { "v.7", 7 },
  { "v.8", 8 },
  { "v.9", 9 },

  { "v0", 0 },
  { "v1", 1 },
  { "v10", 10 },
  { "v11", 11 },
  { "v12", 12 },
  { "v13", 13 },
  { "v14", 14 },
  { "v15", 15 },
  { "v16", 16 },
  { "v17", 17 },
  { "v18", 18 },
  { "v19", 19 },
  { "v2", 2 },
  { "v20", 20 },
  { "v21", 21 },
  { "v22", 22 },
  { "v23", 23 },
  { "v24", 24 },
  { "v25", 25 },
  { "v26", 26 },
  { "v27", 27 },
  { "v28", 28 },
  { "v29", 29 },
  { "v3", 3 },
  { "v30", 30 },
  { "v31", 31 },
  { "v4", 4 },
  { "v5", 5 },
  { "v6", 6 },
  { "v7", 7 },
  { "v8", 8 },
  { "v9", 9 },

  { "xer", 1 },

};

#define REG_NAME_CNT	(sizeof (pre_defined_registers) / sizeof (struct pd_reg))

/* Given NAME, find the register number associated with that name, return
   the integer value associated with the given name or -1 on failure.  */

static int reg_name_search
  PARAMS ((const struct pd_reg *, int, const char * name));

static int
reg_name_search (regs, regcount, name)
     const struct pd_reg *regs;
     int regcount;
     const char *name;
{
  int middle, low, high;
  int cmp;

  low = 0;
  high = regcount - 1;

  do
    {
      middle = (low + high) / 2;
      cmp = strcasecmp (name, regs[middle].name);
      DBG(MD_ASSEMBLE, "name = %s, regs[%d].name=%s, cmp=%d\n", name, middle, regs[middle].name, cmp);
      if (cmp < 0)
	high = middle - 1;
      else if (cmp > 0)
	low = middle + 1;
      else
	return regs[middle].value;
    }
  while (low <= high);

  return -1;
}

/*
 * Summary of register_name.
 *
 * in:	Input_line_pointer points to 1st char of operand.
 *
 * out:	A expressionS.
 *      The operand may have been a register: in this case, X_op == O_register,
 *      X_add_number is set to the register number, and truth is returned.
 *	Input_line_pointer->(next non-blank) char after operand, or is in its
 *      original state.
 */

static boolean
register_name (expressionP)
     expressionS *expressionP;
{
  int reg_number;
  char *name;
  char *start;
  char c;

  DBG (MD_ASSEMBLE, "input_line_pointer = %s.\n", input_line_pointer);
  /* Find the spelling of the operand.  */
  start = name = input_line_pointer;
  if (name[0] == '%' && ISALPHA (name[1]))
    name = ++input_line_pointer;

  else if (!reg_names_p || !ISALPHA (name[0])) {
    DBG(MD_ASSEMBLE, "reg_names_p=%p. name[0]=%c, ISALPHA(name[0])=%d\n", reg_names_p,
        name[0], ISALPHA(name[0]));
    return false;
  }

  DBG(MD_ASSEMBLE, "reg_names_p=%p, name=%s.\n", reg_names_p, name);

  c = get_symbol_end ();
  reg_number = reg_name_search (pre_defined_registers, REG_NAME_CNT, name);

  /* Put back the delimiting char.  */
  *input_line_pointer = c;

  /* Look to see if it's in the register table.  */
  if (reg_number >= 0)
    {
      expressionP->X_op = O_register;
      expressionP->X_add_number = reg_number;

      /* Make the rest nice.  */
      expressionP->X_add_symbol = NULL;
      expressionP->X_op_symbol = NULL;

      return true;
    }

  /* Reset the line as if we had not done anything.  */
  input_line_pointer = start;
  DBG(MD_ASSEMBLE, "input_line_pointer=%s. returning false\n", input_line_pointer);
  return false;
}

/* This function is called for each symbol seen in an expression.  It
   handles the special parsing which PowerPC assemblers are supposed
   to use for condition codes.  */

/* Whether to do the special parsing.  */
static boolean cr_operand;

/* Names to recognize in a condition code.  This table is sorted.  */
static const struct pd_reg cr_names[] =
{
  { "cr0", 0 },
  { "cr1", 1 },
  { "cr2", 2 },
  { "cr3", 3 },
  { "cr4", 4 },
  { "cr5", 5 },
  { "cr6", 6 },
  { "cr7", 7 },
  { "eq", 2 },
  { "gt", 1 },
  { "lt", 0 },
  { "so", 3 },
  { "un", 3 }
};

/* Parsing function.  This returns non-zero if it recognized an
   expression.  */

int
ppc_parse_name (name, expr)
     const char *name;
     expressionS *expr;
{
  int val;

  if (! cr_operand)
    return 0;

  val = reg_name_search (cr_names, sizeof cr_names / sizeof cr_names[0],
			 name);
  if (val < 0)
    return 0;

  expr->X_op = O_constant;
  expr->X_add_number = val;

  return 1;
}

/* Local variables.  */

/* The type of processor we are assembling for.  This is one or more
   of the PPC_OPCODE flags defined in opcode/ppc.h.  */
static unsigned long ppc_cpu = PPC_OPCODE_PPC | PPC_OPCODE_32;

/* Opcode hash table.  */
static struct hash_control *ppc_hash;

/* Macro hash table.  */
static struct hash_control *ppc_macro_hash;


/* This function is called when the assembler starts up.  It is called
   after the options have been parsed and the output file has been
   opened.  */

void
ppc_md_begin ()
{
  register const struct powerpc_opcode *op;
  const struct powerpc_opcode *op_end;
  const struct powerpc_macro *macro;
  const struct powerpc_macro *macro_end;
  boolean dup_insn = false;

  /* Insert the opcodes into a hash table.  */
  ppc_hash = gas_hash_new ();

  op_end = powerpc_opcodes + powerpc_num_opcodes;
  for (op = powerpc_opcodes; op < op_end; op++) {
    assert ((op->opcode & op->mask) == op->opcode);

    if ((op->flags & ppc_cpu & ~(PPC_OPCODE_32 | PPC_OPCODE_64)) != 0
        && ((op->flags & (PPC_OPCODE_32 | PPC_OPCODE_64)) == 0
          || ((op->flags & (PPC_OPCODE_32 | PPC_OPCODE_64))
            == (ppc_cpu & (PPC_OPCODE_32 | PPC_OPCODE_64))))) {
      const char *retval;

      retval = gas_hash_insert (ppc_hash, op->name, (PTR) op);
      if (retval != (const char *) NULL) {
        /* Ignore Power duplicates for -m601.  */
        if ((ppc_cpu & PPC_OPCODE_601) != 0
            && (op->flags & PPC_OPCODE_POWER) != 0) {
          continue;
        }
        as_bad (_("Internal assembler error for instruction %s"),
            op->name);
        dup_insn = true;
      }
    }
  }

  /* Insert the macros into a hash table.  */
  ppc_macro_hash = gas_hash_new ();

  macro_end = powerpc_macros + powerpc_num_macros;
  for (macro = powerpc_macros; macro < macro_end; macro++)
    {
      if ((macro->flags & ppc_cpu) != 0)
	{
	  const char *retval;

	  retval = gas_hash_insert (ppc_macro_hash, macro->name, (PTR) macro);
	  if (retval != (const char *) NULL)
	    {
	      as_bad (_("Internal assembler error for macro %s"), macro->name);
	      dup_insn = true;
	    }
	}
    }

  if (dup_insn)
    abort ();

  /* Tell the main code what the endianness is if it is not overidden
     by the user.  */
  if (!set_target_endian)
    {
      set_target_endian = 1;
      target_big_endian = PPC_BIG_ENDIAN;
    }
}

/* Insert an operand value into an instruction.  */

static unsigned long
ppc_insert_operand (insn, operand, val, file, line)
     unsigned long insn;
     const struct powerpc_operand *operand;
     offsetT val;
     char *file;
     unsigned int line;
{
  if (operand->bits != 32)
    {
      long min, max;
      offsetT test;

      if ((operand->flags & PPC_OPERAND_SIGNED) != 0)
	{
	  if ((operand->flags & PPC_OPERAND_SIGNOPT) != 0)
	    max = (1 << operand->bits) - 1;
	  else
	    max = (1 << (operand->bits - 1)) - 1;
	  min = - (1 << (operand->bits - 1));

	  if (true /*!ppc_obj64*/)
	    {
	      /* Some people write 32 bit hex constants with the sign
		 extension done by hand.  This shouldn't really be
		 valid, but, to permit this code to assemble on a 64
		 bit host, we sign extend the 32 bit value.  */
	      if (val > 0
		  && (val & (offsetT) 0x80000000) != 0
		  && (val & (offsetT) 0xffffffff) == val)
		{
		  val -= 0x80000000;
		  val -= 0x80000000;
		}
	    }
	}
      else
	{
	  max = (1 << operand->bits) - 1;
	  min = 0;
	}

      if ((operand->flags & PPC_OPERAND_NEGATIVE) != 0)
	test = - val;
      else
	test = val;

      if (test < (offsetT) min || test > (offsetT) max)
	{
	  /*
	  const char *err =
	    _("operand out of range (%s not between %ld and %ld)");
	  char buf[100];

	  sprint_value (buf, test);
	  as_bad_where (file, line, err, buf, min, max);
	  */
	  fprintf (stderr, "%s() %d: operand out of range (%lx not between %ld "
	      "and %ld)", __func__, __LINE__, test, min, max);
	  assert (0);
	}
    }

  if (operand->insert)
    {
      const char *errmsg;

      errmsg = NULL;
      insn = (*operand->insert) (insn, (long) val, &errmsg);
      if (errmsg != (const char *) NULL)
      {
	fprintf (stderr, "%s", errmsg);
	as_bad_where (file, line, errmsg);
      }
    }
  else
    insn |= (((long) val & ((1 << operand->bits) - 1))
	     << operand->shift);

  return insn;
}

#define MAX_INSN_FIXUPS (5)

/* This routine is called for each instruction to be assembled.  */

int
ppc_md_assemble (str, f)
     char *str;
     uint8_t *f;
{
  char *s;
  const struct powerpc_opcode *opcode;
  unsigned long insn;
  const unsigned char *opindex_ptr;
  int skip_optional;
  int need_paren;
  int next_opindex;
  int fc;
  //char *f;
  gas_assemble_mode = ARCH_PPC;

  /* Get the opcode.  */
  for (s = str; *s != '\0' && ! ISSPACE (*s); s++)
    ;
  if (*s != '\0')
    *s++ = '\0';

  /* Look up the opcode in the hash table.  */
  opcode = (const struct powerpc_opcode *) gas_hash_find (ppc_hash, str);
  if (opcode == (const struct powerpc_opcode *) NULL)
    {
      const struct powerpc_macro *macro;

      macro = (const struct powerpc_macro *) gas_hash_find (ppc_macro_hash, str);
      if (macro == (const struct powerpc_macro *) NULL)
	as_bad (_("Unrecognized opcode: `%s'"), str);
      else
	ppc_macro (s, macro);

      return -1;
    }

  insn = opcode->opcode;

  str = s;
  while (ISSPACE (*str))
    ++str;

  /* PowerPC operands are just expressions.  The only real issue is
     that a few operand types are optional.  All cases which might use
     an optional operand separate the operands only with commas (in
     some cases parentheses are used, as in ``lwz 1,0(1)'' but such
     cases never have optional operands).  There is never more than
     one optional operand for an instruction.  So, before we start
     seriously parsing the operands, we check to see if we have an
     optional operand, and, if we do, we count the number of commas to
     see whether the operand should be omitted.  */
  skip_optional = 0;
  for (opindex_ptr = opcode->operands; *opindex_ptr != 0; opindex_ptr++)
    {
      const struct powerpc_operand *operand;

      operand = &powerpc_operands[*opindex_ptr];
      if ((operand->flags & PPC_OPERAND_OPTIONAL) != 0)
	{
	  unsigned int opcount;
	  unsigned int num_operands_expected;
	  unsigned int i;

	  /* There is an optional operand.  Count the number of
	     commas in the input line.  */
	  if (*str == '\0')
	    opcount = 0;
	  else
	    {
	      opcount = 1;
	      s = str;
	      while ((s = strchr (s, ',')) != (char *) NULL)
		{
		  ++opcount;
		  ++s;
		}
	    }

	  /* Compute the number of expected operands.
	     Do not count fake operands.  */
	  for (num_operands_expected = 0, i = 0; opcode->operands[i]; i ++)
	    if ((powerpc_operands [opcode->operands[i]].flags & PPC_OPERAND_FAKE) == 0)
	      ++ num_operands_expected;

	  /* If there are fewer operands in the line then are called
	     for by the instruction, we want to skip the optional
	     operand.  */
	  if (opcount < num_operands_expected)
	    skip_optional = 1;

	  break;
	}
    }

  /* Gather the operands.  */
  need_paren = 0;
  next_opindex = 0;
  fc = 0;
  for (opindex_ptr = opcode->operands; *opindex_ptr != 0; opindex_ptr++)
    {
      const struct powerpc_operand *operand;
      const char *errmsg;
      char *hold;
      expressionS ex;
      char endc;

      DBG (MD_ASSEMBLE, "*opindex_ptr=%hhd\n", *opindex_ptr);

      if (next_opindex == 0)
	operand = &powerpc_operands[*opindex_ptr];
      else
	{
	  operand = &powerpc_operands[next_opindex];
	  next_opindex = 0;
	}

      errmsg = NULL;

      /* If this is a fake operand, then we do not expect anything
	 from the input.  */
      if ((operand->flags & PPC_OPERAND_FAKE) != 0)
	{
	  DBG (MD_ASSEMBLE, "%s", "fake operand\n");
	  insn = (*operand->insert) (insn, 0L, &errmsg);
	  if (errmsg != (const char *) NULL)
	    as_bad (errmsg);
	  continue;
	}

      /* If this is an optional operand, and we are skipping it, just
	 insert a zero.  */
      if ((operand->flags & PPC_OPERAND_OPTIONAL) != 0
	  && skip_optional)
	{
	  DBG (MD_ASSEMBLE, "%s", "optional operand\n");
	  if (operand->insert)
	    {
	      DBG (MD_ASSEMBLE, "%s", "inserting zero\n");
	      insn = (*operand->insert) (insn, 0L, &errmsg);
	      if (errmsg != (const char *) NULL)
		as_bad (errmsg);
	    }
	  if ((operand->flags & PPC_OPERAND_NEXT) != 0)
	    next_opindex = *opindex_ptr + 1;
	  continue;
	}

      /* Gather the operand.  */
      hold = input_line_pointer;
      input_line_pointer = str;

	{
	  if (! register_name (&ex))
	    {
	      DBG (MD_ASSEMBLE, "%s", "not register name\n");
	      if ((operand->flags & PPC_OPERAND_CR) != 0)
		cr_operand = true;
	      expression (&ex);
	      cr_operand = false;
	    }
	}

      str = input_line_pointer;
      input_line_pointer = hold;

      if (ex.X_op == O_illegal)
	as_bad (_("illegal operand"));
      else if (ex.X_op == O_absent)
	as_bad (_("missing operand"));
      else if (ex.X_op == O_register)
	{
	  DBG (MD_ASSEMBLE, "%s", "register operand\n");
	  insn = ppc_insert_operand (insn, operand, ex.X_add_number,
				     (char *) NULL, 0);
	}
      else if (ex.X_op == O_constant)
	{
	  DBG (MD_ASSEMBLE, "%s", "constant operand\n");
	  insn = ppc_insert_operand (insn, operand, ex.X_add_number,
				     (char *) NULL, 0);
	}
      else
	{
	  /* We need to generate a fixup for this expression.  */
	  /*
	  if (fc >= MAX_INSN_FIXUPS)
	    as_fatal (_("too many fixups"));
	  fixups[fc].exp = ex;
	  fixups[fc].opindex = *opindex_ptr;
	  fixups[fc].reloc = BFD_RELOC_UNUSED;
	  ++fc;
	  */
	}

      if (need_paren)
	{
	  endc = ')';
	  need_paren = 0;
	}
      else if ((operand->flags & PPC_OPERAND_PARENS) != 0)
	{
	  endc = '(';
	  need_paren = 1;
	}
      else
	endc = ',';

      /* The call to expression should have advanced str past any
	 whitespace.  */
      if (*str != endc
	  && (endc != ',' || *str != '\0'))
	{
	  printf("syntax error; found `%c' but expected `%c'\n", *str, endc);
    fflush(stdout);
	  ASSERT(0);
	  break;
	}

      if (*str != '\0')
	++str;
    }

  while (ISSPACE (*str))
    ++str;

  if (*str != '\0')
    as_bad (_("junk at end of line: `%s'"), str);

  /* Write out the instruction.  */
  //f = frag_more (4);
  md_number_to_chars (f, insn, 4);

  return 4;
}

/* Handle a macro.  Gather all the operands, transform them as
   described by the macro, and call md_assemble recursively.  All the
   operands are separated by commas; we don't accept parentheses
   around operands here.  */

static void
ppc_macro (str, macro)
     char *str;
     const struct powerpc_macro *macro;
{
  char *operands[10];
  unsigned int count;
  char *s;
  unsigned int len;
  const char *format;
  int arg;
  char *send;
  char *complete;

  /* Gather the users operands into the operands array.  */
  count = 0;
  s = str;
  while (1)
    {
      if (count >= sizeof operands / sizeof operands[0])
	break;
      operands[count++] = s;
      s = strchr (s, ',');
      if (s == (char *) NULL)
	break;
      *s++ = '\0';
    }

  if (count != macro->operands)
    {
      as_bad (_("wrong number of operands"));
      return;
    }

  /* Work out how large the string must be (the size is unbounded
     because it includes user input).  */
  len = 0;
  format = macro->format;
  while (*format != '\0')
    {
      if (*format != '%')
	{
	  ++len;
	  ++format;
	}
      else
	{
	  arg = strtol (format + 1, &send, 10);
	  assert (send != format && arg >= 0 && arg < count);
	  len += strlen (operands[arg]);
	  format = send;
	}
    }

  /* Put the string together.  */
  complete = s = (char *) alloca (len + 1);
  format = macro->format;
  while (*format != '\0')
    {
      if (*format != '%')
	*s++ = *format++;
      else
	{
	  arg = strtol (format + 1, &send, 10);
	  strcpy (s, operands[arg]);
	  s += strlen (s);
	  format = send;
	}
    }
  *s = '\0';

  /* Assemble the constructed instruction.  */
  //md_assemble (complete);		//XXX
}


/* Pseudo-op handling.  */

/* The .byte pseudo-op.  This is similar to the normal .byte
   pseudo-op, but it can also take a single ASCII string.  */

static void
ppc_byte (ignore)
     int ignore ATTRIBUTE_UNUSED;
{
  if (*input_line_pointer != '\"')
    {
      cons (1);
      return;
    }

  /* Gather characters.  A real double quote is doubled.  Unusual
     characters are not permitted.  */
  ++input_line_pointer;
  while (1)
    {
      char c;

      c = *input_line_pointer++;

      if (c == '\"')
	{
	  if (*input_line_pointer != '\"')
	    break;
	  ++input_line_pointer;
	}

      FRAG_APPEND_1_CHAR (c);
    }

  demand_empty_rest_of_line ();
}


/* Turn a string in input_line_pointer into a floating point constant
   of type TYPE, and store the appropriate bytes in *LITP.  The number
   of LITTLENUMS emitted is stored in *SIZEP.  An error message is
   returned, or NULL on OK.  */

char *
ppc_md_atof (type, litp, sizep)
     int type;
     char *litp;
     int *sizep;
{
  int prec;
  LITTLENUM_TYPE words[4];
  char *t;
  int i;

  switch (type)
    {
    case 'f':
      prec = 2;
      break;

    case 'd':
      prec = 4;
      break;

    default:
      *sizep = 0;
      return _("bad call to md_atof");
    }

  t = atof_ieee (input_line_pointer, type, words);
  if (t)
    input_line_pointer = t;

  *sizep = prec * 2;

  if (target_big_endian)
    {
      for (i = 0; i < prec; i++)
	{
	  md_number_to_chars (litp, (valueT) words[i], 2);
	  litp += 2;
	}
    }
  else
    {
      for (i = prec - 1; i >= 0; i--)
	{
	  md_number_to_chars (litp, (valueT) words[i], 2);
	  litp += 2;
	}
    }

  return NULL;
}

#if 0 //sorav
/* Align a section (I don't know why this is machine dependent).  */

valueT
md_section_align (seg, addr)
     asection *seg;
     valueT addr;
{
  int align = bfd_get_section_alignment (stdoutput, seg);

  return ((addr + (1 << align) - 1) & (-1 << align));
}

/* We don't have any form of relaxing.  */

int
md_estimate_size_before_relax (fragp, seg)
     fragS *fragp ATTRIBUTE_UNUSED;
     asection *seg ATTRIBUTE_UNUSED;
{
  abort ();
  return 0;
}

/* Convert a machine dependent frag.  We never generate these.  */

void
md_convert_frag (abfd, sec, fragp)
     bfd *abfd ATTRIBUTE_UNUSED;
     asection *sec ATTRIBUTE_UNUSED;
     fragS *fragp ATTRIBUTE_UNUSED;
{
  abort ();
}
#endif

/* We have no need to default values of symbols.  */

symbolS *
ppc_md_undefined_symbol (name)
     char *name ATTRIBUTE_UNUSED;
{
  return 0;
}

/* Functions concerning relocs.  */

#ifdef OBJ_XCOFF

/* This is called to see whether a fixup should be adjusted to use a
   section symbol.  We take the opportunity to change a fixup against
   a symbol in the TOC subsegment into a reloc against the
   corresponding .tc symbol.  */

int
ppc_fix_adjustable (fix)
     fixS *fix;
{
  valueT val;

  resolve_symbol_value (fix->fx_addsy);
  val = S_GET_VALUE (fix->fx_addsy);
  if (ppc_toc_csect != (symbolS *) NULL
      && fix->fx_addsy != (symbolS *) NULL
      && fix->fx_addsy != ppc_toc_csect
      && S_GET_SEGMENT (fix->fx_addsy) == data_section
      && val >= ppc_toc_frag->fr_address
      && (ppc_after_toc_frag == (fragS *) NULL
	  || val < ppc_after_toc_frag->fr_address))
    {
      symbolS *sy;

      for (sy = symbol_next (ppc_toc_csect);
	   sy != (symbolS *) NULL;
	   sy = symbol_next (sy))
	{
	  if (symbol_get_tc (sy)->class == XMC_TC0)
	    continue;
	  if (symbol_get_tc (sy)->class != XMC_TC)
	    break;
	  resolve_symbol_value (sy);
	  if (val == S_GET_VALUE (sy))
	    {
	      fix->fx_addsy = sy;
	      fix->fx_addnumber = val - ppc_toc_frag->fr_address;
	      return 0;
	    }
	}

      as_bad_where (fix->fx_file, fix->fx_line,
		    _("symbol in .toc does not match any .tc"));
    }

  /* Possibly adjust the reloc to be against the csect.  */
  if (fix->fx_addsy != (symbolS *) NULL
      && symbol_get_tc (fix->fx_addsy)->subseg == 0
      && symbol_get_tc (fix->fx_addsy)->class != XMC_TC0
      && symbol_get_tc (fix->fx_addsy)->class != XMC_TC
      && S_GET_SEGMENT (fix->fx_addsy) != bss_section
      /* Don't adjust if this is a reloc in the toc section.  */
      && (S_GET_SEGMENT (fix->fx_addsy) != data_section
	  || ppc_toc_csect == NULL
	  || val < ppc_toc_frag->fr_address
	  || (ppc_after_toc_frag != NULL
	      && val >= ppc_after_toc_frag->fr_address)))
    {
      symbolS *csect;

      if (S_GET_SEGMENT (fix->fx_addsy) == text_section)
	csect = ppc_text_csects;
      else if (S_GET_SEGMENT (fix->fx_addsy) == data_section)
	csect = ppc_data_csects;
      else
	abort ();

      /* Skip the initial dummy symbol.  */
      csect = symbol_get_tc (csect)->next;

      if (csect != (symbolS *) NULL)
	{
	  while (symbol_get_tc (csect)->next != (symbolS *) NULL
		 && (symbol_get_frag (symbol_get_tc (csect)->next)->fr_address
		     <= val))
	    {
	      /* If the csect address equals the symbol value, then we
		 have to look through the full symbol table to see
		 whether this is the csect we want.  Note that we will
		 only get here if the csect has zero length.  */
	      if ((symbol_get_frag (csect)->fr_address == val)
		  && S_GET_VALUE (csect) == S_GET_VALUE (fix->fx_addsy))
		{
		  symbolS *scan;

		  for (scan = symbol_next (csect);
		       scan != NULL;
		       scan = symbol_next (scan))
		    {
		      if (symbol_get_tc (scan)->subseg != 0)
			break;
		      if (scan == fix->fx_addsy)
			break;
		    }

		  /* If we found the symbol before the next csect
		     symbol, then this is the csect we want.  */
		  if (scan == fix->fx_addsy)
		    break;
		}

	      csect = symbol_get_tc (csect)->next;
	    }

	  fix->fx_offset += (S_GET_VALUE (fix->fx_addsy)
			     - symbol_get_frag (csect)->fr_address);
	  fix->fx_addsy = csect;
	}
    }

  /* Adjust a reloc against a .lcomm symbol to be against the base
     .lcomm.  */
  if (fix->fx_addsy != (symbolS *) NULL
      && S_GET_SEGMENT (fix->fx_addsy) == bss_section
      && ! S_IS_EXTERNAL (fix->fx_addsy))
    {
      resolve_symbol_value (symbol_get_frag (fix->fx_addsy)->fr_symbol);
      fix->fx_offset +=
	(S_GET_VALUE (fix->fx_addsy)
	 - S_GET_VALUE (symbol_get_frag (fix->fx_addsy)->fr_symbol));
      fix->fx_addsy = symbol_get_frag (fix->fx_addsy)->fr_symbol;
    }

  return 0;
}

/* A reloc from one csect to another must be kept.  The assembler
   will, of course, keep relocs between sections, and it will keep
   absolute relocs, but we need to force it to keep PC relative relocs
   between two csects in the same section.  */

int
ppc_force_relocation (fix)
     fixS *fix;
{
  /* At this point fix->fx_addsy should already have been converted to
     a csect symbol.  If the csect does not include the fragment, then
     we need to force the relocation.  */
  if (fix->fx_pcrel
      && fix->fx_addsy != NULL
      && symbol_get_tc (fix->fx_addsy)->subseg != 0
      && ((symbol_get_frag (fix->fx_addsy)->fr_address
	   > fix->fx_frag->fr_address)
	  || (symbol_get_tc (fix->fx_addsy)->next != NULL
	      && (symbol_get_frag (symbol_get_tc (fix->fx_addsy)->next)->fr_address
		  <= fix->fx_frag->fr_address))))
    return 1;

  return 0;
}

#endif /* OBJ_XCOFF */

#ifdef OBJ_ELF
int
ppc_fix_adjustable (fix)
     fixS *fix;
{
  return (fix->fx_r_type != BFD_RELOC_16_GOTOFF
	  && fix->fx_r_type != BFD_RELOC_LO16_GOTOFF
	  && fix->fx_r_type != BFD_RELOC_HI16_GOTOFF
	  && fix->fx_r_type != BFD_RELOC_HI16_S_GOTOFF
	  && fix->fx_r_type != BFD_RELOC_GPREL16
	  && fix->fx_r_type != BFD_RELOC_VTABLE_INHERIT
	  && fix->fx_r_type != BFD_RELOC_VTABLE_ENTRY
	  && ! S_IS_EXTERNAL (fix->fx_addsy)
	  && ! S_IS_WEAK (fix->fx_addsy)
	  && (fix->fx_pcrel
	      || (fix->fx_subsy != NULL
		  && (S_GET_SEGMENT (fix->fx_subsy)
		      == S_GET_SEGMENT (fix->fx_addsy)))
	      || S_IS_LOCAL (fix->fx_addsy)));
}
#endif

/* Apply a fixup to the object code.  This is called for all the
   fixups we generated by the call to fix_new_exp, above.  In the call
   above we used a reloc code which was the largest legal reloc code
   plus the operand index.  Here we undo that to recover the operand
   index.  At this point all symbol values should be fully resolved,
   and we attempt to completely resolve the reloc.  If we can not do
   that, we determine the correct reloc code and put it back in the
   fixup.  */

void
ppc_md_apply_fix3 (fixP, valP, seg)
     fixS *fixP;
     valueT * valP;
     segT seg ATTRIBUTE_UNUSED;
{
  valueT value = * valP;

#ifdef OBJ_ELF
  if (fixP->fx_addsy != NULL)
    {
      /* `*valuep' may contain the value of the symbol on which the reloc
	 will be based; we have to remove it.  */
      if (symbol_used_in_reloc_p (fixP->fx_addsy)
	  && S_GET_SEGMENT (fixP->fx_addsy) != absolute_section
	  && S_GET_SEGMENT (fixP->fx_addsy) != undefined_section
	  && ! bfd_is_com_section (S_GET_SEGMENT (fixP->fx_addsy)))
	value -= S_GET_VALUE (fixP->fx_addsy);

      /* FIXME: Why '+'?  Better yet, what exactly is '*valuep'
	 supposed to be?  I think this is related to various similar
	 FIXMEs in tc-i386.c and tc-sparc.c.  */
      if (fixP->fx_pcrel)
	value += fixP->fx_frag->fr_address + fixP->fx_where;
    }
  else
    fixP->fx_done = 1;
#else
  /* FIXME FIXME FIXME: The value we are passed in *valuep includes
     the symbol values.  Since we are using BFD_ASSEMBLER, if we are
     doing this relocation the code in write.c is going to call
     bfd_install_relocation, which is also going to use the symbol
     value.  That means that if the reloc is fully resolved we want to
     use *valuep since bfd_install_relocation is not being used.
     However, if the reloc is not fully resolved we do not want to use
     *valuep, and must use fx_offset instead.  However, if the reloc
     is PC relative, we do want to use *valuep since it includes the
     result of md_pcrel_from.  This is confusing.  */
  if (fixP->fx_addsy == (symbolS *) NULL)
    fixP->fx_done = 1;

  else if (fixP->fx_pcrel)
    ;

  else
    {
      value = fixP->fx_offset;
      if (fixP->fx_subsy != (symbolS *) NULL)
	{
	  if (S_GET_SEGMENT (fixP->fx_subsy) == absolute_section)
	    value -= S_GET_VALUE (fixP->fx_subsy);
	  else
	    {
	      /* We can't actually support subtracting a symbol.  */
	      as_bad_where (fixP->fx_file, fixP->fx_line,
			    _("expression too complex"));
	    }
	}
    }
#endif
}

//sorav
/* The location from which a PC relative jump should be calculated,
   given a PC relative reloc.  */
/*long
md_pcrel_from (fixS *fixp)
{
  return fixp->fx_frag->fr_address + fixp->fx_where;
}*/
