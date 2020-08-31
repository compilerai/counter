#!/usr/bin/perl -w
use strict;
use warnings;

system("rm -f i386_i386/dst_sym_exec_db");
system("rm -f i386_i386/dst.insn.usedef.preprocessed");
system("rm -f i386_i386/dst.insn.types.preprocessed");
system("rm -f etfg_i386/dst_sym_exec_db");
system("rm -f etfg_i386/dst.insn.usedef.preprocessed");
system("rm -f etfg_i386/dst.insn.types.preprocessed");

system("ln -s src_sym_exec_db i386_i386/dst_sym_exec_db");
system("ln -s src.insn.usedef.preprocessed i386_i386/dst.insn.usedef.preprocessed");
system("ln -s src.insn.types.preprocessed i386_i386/dst.insn.types.preprocessed");

system("ln -s ../i386_i386/src_sym_exec_db etfg_i386/dst_sym_exec_db");
system("ln -s ../i386_i386/src.insn.usedef.preprocessed etfg_i386/dst.insn.usedef.preprocessed");
system("ln -s ../i386_i386/src.insn.types.preprocessed etfg_i386/dst.insn.types.preprocessed");
