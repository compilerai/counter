#!/usr/bin/perl -w
use strict;
use warnings;

my @functions = (
"initialiseCRC",
"bsGetInt32",
"getGlobalCRC",
"cadvise",
"bsGetUChar",
"getFinalCRC",
"showFileNames",
"bsGetIntVS",
"setGlobalCRC",
"bsPutInt32",
"bsPutUChar",
"blockOverrun",
"badBGLengths",
"spec_initbufs",
"bitStreamEOF",
"badBlockHeader",
"mySignalCatcher",
"spec_rewind",
"ioError",
"compressedStreamEOF",
"bsPutIntVS",
"panic",
"uncompressOutOfMemory",
"spec_uncompress",
"compressOutOfMemory",
"bsSetStream",
"crcError",
"bsPutUInt32",
"spec_reset",
"mySIGSEGVorSIGBUScatcher",
"bsGetUInt32",
"bsFinishedWithStream",
"makeMaps",
"bsW",
"indexIntoF",
"cleanUpAndFail",
"spec_putc",
"vswap",
"med3",
"spec_getc",
"spec_write",
"spec_init",
"spec_read",
"hbAssignCodes",
"spec_ungetc",
"allocateCompressStructures",
"getRLEpair",
"randomiseBlock",
"moveToFrontCodeAndSend",
"spec_compress",
"loadAndRLEsource",
"fullGtU",
"hbCreateDecodeTables",
"bsR",
"setDecompressStructureSizes",
"spec_load",
"generateMTFValues",
"compressStream",
"uncompressStream",
"simpleSort",
"recvDecodingTables",
"testStream",
"qSort3",
"undoReversibleTransformation_fast",
"hbMakeCodeLengths",
"undoReversibleTransformation_small",
"getAndMoveToFrontDecode",
"sortIt",
"main",
"debug_time",
"doReversibleTransformation",
"sendMTFValues"
);

for my $function (@functions) {
  print "eqchecking $function\n";
  eqcheck($function);
}


sub eqcheck
{
  my $function = shift;
  my $path = "/home/sbansal/smpbench-build/cint";
  my $o0 = "$path/bzip2.bc.O0";
  my $o3 = "$path/bzip2.bc.O3";
  my $o0_etfg = "eqfiles/bzip2.$function.O0.etfg";
  my $o3_etfg = "eqfiles/bzip2.$function.O3.etfg";

  my $bindir = "/home/sbansal/bin";
  my $llvm2tfg = "$bindir/llvm2tfg";
  my $chaperon = "$bindir/chaperon.py";
  my $eq = "$bindir/eq";

  system("$llvm2tfg -f $function $o0 -o $o0_etfg > eqlogs/llvm2tfg.bzip2.o0.$function 2>&1");
  system("$llvm2tfg -f $function $o3 -o $o3_etfg > eqlogs/llvm2tfg.bzip2.o3.$function 2>&1");
  #system("$chaperon \"$eq $function $o0_etfg $o3_etfg --llvm2ir\" --logfile eqlogs/bzip2.llvm2ir.$function");
  system("$chaperon \"$eq $function $o0_etfg $o0_etfg --llvm2ir\" --logfile eqlogs/bzip2.llvm2ir.o0.$function");
  system("$chaperon \"$eq $function $o3_etfg $o3_etfg --llvm2ir\" --logfile eqlogs/bzip2.llvm2ir.o3.$function");
  system("$chaperon \"$eq $function $o0_etfg $o3_etfg --llvm2ir\" --logfile eqlogs/bzip2.llvm2ir.$function");
}
