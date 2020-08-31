all: etfg_i386/src_sym_exec_db etfg_i386/src.insn.usedef.preprocessed etfg_i386/src.insn.types.preprocessed etfg_i386/fb.trans.tab \
     i386_i386/src_sym_exec_db i386_i386/src.insn.usedef.preprocessed i386_i386/src.insn.types.preprocessed i386_i386/fb.trans.tab \
     consts_db

etfg_i386/src_sym_exec_db: etfg_i386/src_sym_exec_db.bz2
	bunzip2 -k -f etfg_i386/src_sym_exec_db.bz2
	touch $@

etfg_i386/src.insn.usedef.preprocessed: etfg_i386/src.insn.usedef.preprocessed.bz2
	bunzip2 -k -f etfg_i386/src.insn.usedef.preprocessed.bz2
	touch $@

etfg_i386/src.insn.types.preprocessed: etfg_i386/src.insn.types.preprocessed.bz2
	bunzip2 -k -f etfg_i386/src.insn.types.preprocessed.bz2
	touch $@

etfg_i386/fb.trans.tab: etfg_i386/fb.trans.tab.bz2
	bunzip2 -k -f etfg_i386/fb.trans.tab.bz2
	touch $@

i386_i386/src_sym_exec_db: i386_i386/src_sym_exec_db.bz2
	bunzip2 -k -f i386_i386/src_sym_exec_db.bz2
	touch $@

i386_i386/src.insn.usedef.preprocessed: i386_i386/src.insn.usedef.preprocessed.bz2
	bunzip2 -k -f i386_i386/src.insn.usedef.preprocessed.bz2
	touch $@

i386_i386/src.insn.types.preprocessed: i386_i386/src.insn.types.preprocessed.bz2
	bunzip2 -k -f i386_i386/src.insn.types.preprocessed.bz2
	touch $@

i386_i386/fb.trans.tab: i386_i386/fb.trans.tab.bz2
	bunzip2 -k -f i386_i386/fb.trans.tab.bz2
	touch $@

consts_db: consts_db.bz2
	bunzip2 -k -f consts_db.bz2
	touch $@

clean:
	rm -f etfg_i386/src_sym_exec_db etfg_i386/src.insn.usedef.preprocessed etfg_i386/src.insn.types.preprocessed etfg_i386/fb.trans.tab \
        i386_i386/src_sym_exec_db i386_i386/src.insn.usedef.preprocessed i386_i386/src.insn.types.preprocessed i386_i386/fb.trans.tab \
        consts_db
