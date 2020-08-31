python superopt/utils/gen_all_peeptabs.py
parallel -j 12 :::: llvm-bc.functions
python superopt/utils/eqcheck_gen_eqfiles.py --missing_only peeptabs-*/*/*.ptab > all.eqfiles
parallel -j 12 :::: all.eqfiles
