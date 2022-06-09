all: etfg_x64/consts_db
     #i386_i386/src.insn.types.preprocessed i386_i386/fb.trans.tab
     #etfg_i386/src.insn.types.preprocessed e

%/src_sym_exec_db: %/src_sym_exec_db.bz2
	bunzip2 -k -f $<
	touch $@

%/src.insn.usedef.preprocessed: %/src.insn.usedef.preprocessed.bz2
	bunzip2 -k -f $<
	touch $@

%/src.insn.types.preprocessed: %/src.insn.types.preprocessed.bz2
	bunzip2 -k -f $<
	touch $@

%/fb.trans.tab: %/fb.trans.tab.bz2
	bunzip2 -k -f $<
	touch $@

%/consts_db: %/consts_db.bz2
	bunzip2 -k -f $<
	touch $@

clean:
	rm -f etfg_i386/src_sym_exec_db etfg_i386/src.insn.usedef.preprocessed etfg_i386/src.insn.types.preprocessed etfg_i386/fb.trans.tab \
        i386_i386/consts_db i386_i386/src_sym_exec_db i386_i386/src.insn.usedef.preprocessed i386_i386/src.insn.types.preprocessed i386_i386/fb.trans.tab \
