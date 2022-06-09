; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc < %s -mtriple=x86_64-unknown -mattr=+ssse3           | FileCheck %s --check-prefixes=SSSE3,SSSE3-SLOW
; RUN: llc < %s -mtriple=x86_64-unknown -mattr=+ssse3,fast-hops | FileCheck %s --check-prefixes=SSSE3,SSSE3-FAST
; RUN: llc < %s -mtriple=x86_64-unknown -mattr=+avx             | FileCheck %s --check-prefixes=AVX,AVX-SLOW,AVX1,AVX1-SLOW
; RUN: llc < %s -mtriple=x86_64-unknown -mattr=+avx,fast-hops   | FileCheck %s --check-prefixes=AVX,AVX-FAST,AVX1,AVX1-FAST
; RUN: llc < %s -mtriple=x86_64-unknown -mattr=+avx2            | FileCheck %s --check-prefixes=AVX,AVX-SLOW,AVX2,AVX2-SLOW
; RUN: llc < %s -mtriple=x86_64-unknown -mattr=+avx2,fast-hops  | FileCheck %s --check-prefixes=AVX,AVX-FAST,AVX2,AVX2-FAST
; RUN: llc < %s -mtriple=x86_64-unknown -mattr=+avx2,fast-variable-shuffle | FileCheck %s --check-prefixes=AVX,AVX2,AVX2-SHUF

define <8 x i16> @phaddw1(<8 x i16> %x, <8 x i16> %y) {
; SSSE3-LABEL: phaddw1:
; SSSE3:       # %bb.0:
; SSSE3-NEXT:    phaddw %xmm1, %xmm0
; SSSE3-NEXT:    retq
;
; AVX-LABEL: phaddw1:
; AVX:       # %bb.0:
; AVX-NEXT:    vphaddw %xmm1, %xmm0, %xmm0
; AVX-NEXT:    retq
  %a = shufflevector <8 x i16> %x, <8 x i16> %y, <8 x i32> <i32 0, i32 2, i32 4, i32 6, i32 8, i32 10, i32 12, i32 14>
  %b = shufflevector <8 x i16> %x, <8 x i16> %y, <8 x i32> <i32 1, i32 3, i32 5, i32 7, i32 9, i32 11, i32 13, i32 15>
  %r = add <8 x i16> %a, %b
  ret <8 x i16> %r
}

define <8 x i16> @phaddw2(<8 x i16> %x, <8 x i16> %y) {
; SSSE3-LABEL: phaddw2:
; SSSE3:       # %bb.0:
; SSSE3-NEXT:    phaddw %xmm1, %xmm0
; SSSE3-NEXT:    retq
;
; AVX-LABEL: phaddw2:
; AVX:       # %bb.0:
; AVX-NEXT:    vphaddw %xmm1, %xmm0, %xmm0
; AVX-NEXT:    retq
  %a = shufflevector <8 x i16> %x, <8 x i16> %y, <8 x i32> <i32 1, i32 2, i32 5, i32 6, i32 9, i32 10, i32 13, i32 14>
  %b = shufflevector <8 x i16> %y, <8 x i16> %x, <8 x i32> <i32 8, i32 11, i32 12, i32 15, i32 0, i32 3, i32 4, i32 7>
  %r = add <8 x i16> %a, %b
  ret <8 x i16> %r
}

define <4 x i32> @phaddd1(<4 x i32> %x, <4 x i32> %y) {
; SSSE3-LABEL: phaddd1:
; SSSE3:       # %bb.0:
; SSSE3-NEXT:    phaddd %xmm1, %xmm0
; SSSE3-NEXT:    retq
;
; AVX-LABEL: phaddd1:
; AVX:       # %bb.0:
; AVX-NEXT:    vphaddd %xmm1, %xmm0, %xmm0
; AVX-NEXT:    retq
  %a = shufflevector <4 x i32> %x, <4 x i32> %y, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %b = shufflevector <4 x i32> %x, <4 x i32> %y, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %r = add <4 x i32> %a, %b
  ret <4 x i32> %r
}

define <4 x i32> @phaddd2(<4 x i32> %x, <4 x i32> %y) {
; SSSE3-LABEL: phaddd2:
; SSSE3:       # %bb.0:
; SSSE3-NEXT:    phaddd %xmm1, %xmm0
; SSSE3-NEXT:    retq
;
; AVX-LABEL: phaddd2:
; AVX:       # %bb.0:
; AVX-NEXT:    vphaddd %xmm1, %xmm0, %xmm0
; AVX-NEXT:    retq
  %a = shufflevector <4 x i32> %x, <4 x i32> %y, <4 x i32> <i32 1, i32 2, i32 5, i32 6>
  %b = shufflevector <4 x i32> %y, <4 x i32> %x, <4 x i32> <i32 4, i32 7, i32 0, i32 3>
  %r = add <4 x i32> %a, %b
  ret <4 x i32> %r
}

define <4 x i32> @phaddd3(<4 x i32> %x) {
; SSSE3-LABEL: phaddd3:
; SSSE3:       # %bb.0:
; SSSE3-NEXT:    phaddd %xmm0, %xmm0
; SSSE3-NEXT:    retq
;
; AVX-LABEL: phaddd3:
; AVX:       # %bb.0:
; AVX-NEXT:    vphaddd %xmm0, %xmm0, %xmm0
; AVX-NEXT:    retq
  %a = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 undef, i32 2, i32 4, i32 6>
  %b = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 undef, i32 3, i32 5, i32 7>
  %r = add <4 x i32> %a, %b
  ret <4 x i32> %r
}

define <4 x i32> @phaddd4(<4 x i32> %x) {
; SSSE3-LABEL: phaddd4:
; SSSE3:       # %bb.0:
; SSSE3-NEXT:    phaddd %xmm0, %xmm0
; SSSE3-NEXT:    retq
;
; AVX-LABEL: phaddd4:
; AVX:       # %bb.0:
; AVX-NEXT:    vphaddd %xmm0, %xmm0, %xmm0
; AVX-NEXT:    retq
  %a = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 0, i32 2, i32 undef, i32 undef>
  %b = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 1, i32 3, i32 undef, i32 undef>
  %r = add <4 x i32> %a, %b
  ret <4 x i32> %r
}

define <4 x i32> @phaddd5(<4 x i32> %x) {
; SSSE3-LABEL: phaddd5:
; SSSE3:       # %bb.0:
; SSSE3-NEXT:    phaddd %xmm0, %xmm0
; SSSE3-NEXT:    retq
;
; AVX-LABEL: phaddd5:
; AVX:       # %bb.0:
; AVX-NEXT:    vphaddd %xmm0, %xmm0, %xmm0
; AVX-NEXT:    retq
  %a = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 0, i32 3, i32 undef, i32 undef>
  %b = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 1, i32 2, i32 undef, i32 undef>
  %r = add <4 x i32> %a, %b
  ret <4 x i32> %r
}

define <4 x i32> @phaddd6(<4 x i32> %x) {
; SSSE3-SLOW-LABEL: phaddd6:
; SSSE3-SLOW:       # %bb.0:
; SSSE3-SLOW-NEXT:    pshufd {{.*#+}} xmm1 = xmm0[1,1,1,1]
; SSSE3-SLOW-NEXT:    paddd %xmm1, %xmm0
; SSSE3-SLOW-NEXT:    retq
;
; SSSE3-FAST-LABEL: phaddd6:
; SSSE3-FAST:       # %bb.0:
; SSSE3-FAST-NEXT:    phaddd %xmm0, %xmm0
; SSSE3-FAST-NEXT:    retq
;
; AVX-SLOW-LABEL: phaddd6:
; AVX-SLOW:       # %bb.0:
; AVX-SLOW-NEXT:    vpshufd {{.*#+}} xmm1 = xmm0[1,1,1,1]
; AVX-SLOW-NEXT:    vpaddd %xmm1, %xmm0, %xmm0
; AVX-SLOW-NEXT:    retq
;
; AVX-FAST-LABEL: phaddd6:
; AVX-FAST:       # %bb.0:
; AVX-FAST-NEXT:    vphaddd %xmm0, %xmm0, %xmm0
; AVX-FAST-NEXT:    retq
;
; AVX2-SHUF-LABEL: phaddd6:
; AVX2-SHUF:       # %bb.0:
; AVX2-SHUF-NEXT:    vpshufd {{.*#+}} xmm1 = xmm0[1,1,1,1]
; AVX2-SHUF-NEXT:    vpaddd %xmm1, %xmm0, %xmm0
; AVX2-SHUF-NEXT:    retq
  %a = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 0, i32 undef, i32 undef, i32 undef>
  %b = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 1, i32 undef, i32 undef, i32 undef>
  %r = add <4 x i32> %a, %b
  ret <4 x i32> %r
}

define <4 x i32> @phaddd7(<4 x i32> %x) {
; SSSE3-LABEL: phaddd7:
; SSSE3:       # %bb.0:
; SSSE3-NEXT:    phaddd %xmm0, %xmm0
; SSSE3-NEXT:    retq
;
; AVX-LABEL: phaddd7:
; AVX:       # %bb.0:
; AVX-NEXT:    vphaddd %xmm0, %xmm0, %xmm0
; AVX-NEXT:    retq
  %a = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 undef, i32 3, i32 undef, i32 undef>
  %b = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 undef, i32 2, i32 undef, i32 undef>
  %r = add <4 x i32> %a, %b
  ret <4 x i32> %r
}

define <8 x i16> @phsubw1(<8 x i16> %x, <8 x i16> %y) {
; SSSE3-LABEL: phsubw1:
; SSSE3:       # %bb.0:
; SSSE3-NEXT:    phsubw %xmm1, %xmm0
; SSSE3-NEXT:    retq
;
; AVX-LABEL: phsubw1:
; AVX:       # %bb.0:
; AVX-NEXT:    vphsubw %xmm1, %xmm0, %xmm0
; AVX-NEXT:    retq
  %a = shufflevector <8 x i16> %x, <8 x i16> %y, <8 x i32> <i32 0, i32 2, i32 4, i32 6, i32 8, i32 10, i32 12, i32 14>
  %b = shufflevector <8 x i16> %x, <8 x i16> %y, <8 x i32> <i32 1, i32 3, i32 5, i32 7, i32 9, i32 11, i32 13, i32 15>
  %r = sub <8 x i16> %a, %b
  ret <8 x i16> %r
}

define <4 x i32> @phsubd1(<4 x i32> %x, <4 x i32> %y) {
; SSSE3-LABEL: phsubd1:
; SSSE3:       # %bb.0:
; SSSE3-NEXT:    phsubd %xmm1, %xmm0
; SSSE3-NEXT:    retq
;
; AVX-LABEL: phsubd1:
; AVX:       # %bb.0:
; AVX-NEXT:    vphsubd %xmm1, %xmm0, %xmm0
; AVX-NEXT:    retq
  %a = shufflevector <4 x i32> %x, <4 x i32> %y, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %b = shufflevector <4 x i32> %x, <4 x i32> %y, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %r = sub <4 x i32> %a, %b
  ret <4 x i32> %r
}

define <4 x i32> @phsubd2(<4 x i32> %x) {
; SSSE3-LABEL: phsubd2:
; SSSE3:       # %bb.0:
; SSSE3-NEXT:    phsubd %xmm0, %xmm0
; SSSE3-NEXT:    retq
;
; AVX-LABEL: phsubd2:
; AVX:       # %bb.0:
; AVX-NEXT:    vphsubd %xmm0, %xmm0, %xmm0
; AVX-NEXT:    retq
  %a = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 undef, i32 2, i32 4, i32 6>
  %b = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 undef, i32 3, i32 5, i32 7>
  %r = sub <4 x i32> %a, %b
  ret <4 x i32> %r
}

define <4 x i32> @phsubd3(<4 x i32> %x) {
; SSSE3-LABEL: phsubd3:
; SSSE3:       # %bb.0:
; SSSE3-NEXT:    phsubd %xmm0, %xmm0
; SSSE3-NEXT:    retq
;
; AVX-LABEL: phsubd3:
; AVX:       # %bb.0:
; AVX-NEXT:    vphsubd %xmm0, %xmm0, %xmm0
; AVX-NEXT:    retq
  %a = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 0, i32 2, i32 undef, i32 undef>
  %b = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 1, i32 3, i32 undef, i32 undef>
  %r = sub <4 x i32> %a, %b
  ret <4 x i32> %r
}

define <4 x i32> @phsubd4(<4 x i32> %x) {
; SSSE3-SLOW-LABEL: phsubd4:
; SSSE3-SLOW:       # %bb.0:
; SSSE3-SLOW-NEXT:    pshufd {{.*#+}} xmm1 = xmm0[1,1,1,1]
; SSSE3-SLOW-NEXT:    psubd %xmm1, %xmm0
; SSSE3-SLOW-NEXT:    retq
;
; SSSE3-FAST-LABEL: phsubd4:
; SSSE3-FAST:       # %bb.0:
; SSSE3-FAST-NEXT:    phsubd %xmm0, %xmm0
; SSSE3-FAST-NEXT:    retq
;
; AVX-SLOW-LABEL: phsubd4:
; AVX-SLOW:       # %bb.0:
; AVX-SLOW-NEXT:    vpshufd {{.*#+}} xmm1 = xmm0[1,1,1,1]
; AVX-SLOW-NEXT:    vpsubd %xmm1, %xmm0, %xmm0
; AVX-SLOW-NEXT:    retq
;
; AVX-FAST-LABEL: phsubd4:
; AVX-FAST:       # %bb.0:
; AVX-FAST-NEXT:    vphsubd %xmm0, %xmm0, %xmm0
; AVX-FAST-NEXT:    retq
;
; AVX2-SHUF-LABEL: phsubd4:
; AVX2-SHUF:       # %bb.0:
; AVX2-SHUF-NEXT:    vpshufd {{.*#+}} xmm1 = xmm0[1,1,1,1]
; AVX2-SHUF-NEXT:    vpsubd %xmm1, %xmm0, %xmm0
; AVX2-SHUF-NEXT:    retq
  %a = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 0, i32 undef, i32 undef, i32 undef>
  %b = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 1, i32 undef, i32 undef, i32 undef>
  %r = sub <4 x i32> %a, %b
  ret <4 x i32> %r
}

define <8 x i16> @phsubw1_reverse(<8 x i16> %x, <8 x i16> %y) {
; SSSE3-LABEL: phsubw1_reverse:
; SSSE3:       # %bb.0:
; SSSE3-NEXT:    movdqa {{.*#+}} xmm3 = [2,3,6,7,10,11,14,15,14,15,10,11,12,13,14,15]
; SSSE3-NEXT:    movdqa %xmm1, %xmm4
; SSSE3-NEXT:    pshufb %xmm3, %xmm4
; SSSE3-NEXT:    movdqa %xmm0, %xmm2
; SSSE3-NEXT:    pshufb %xmm3, %xmm2
; SSSE3-NEXT:    punpcklqdq {{.*#+}} xmm2 = xmm2[0],xmm4[0]
; SSSE3-NEXT:    movdqa {{.*#+}} xmm3 = [0,1,4,5,8,9,12,13,8,9,12,13,12,13,14,15]
; SSSE3-NEXT:    pshufb %xmm3, %xmm1
; SSSE3-NEXT:    pshufb %xmm3, %xmm0
; SSSE3-NEXT:    punpcklqdq {{.*#+}} xmm0 = xmm0[0],xmm1[0]
; SSSE3-NEXT:    psubw %xmm0, %xmm2
; SSSE3-NEXT:    movdqa %xmm2, %xmm0
; SSSE3-NEXT:    retq
;
; AVX-LABEL: phsubw1_reverse:
; AVX:       # %bb.0:
; AVX-NEXT:    vmovdqa {{.*#+}} xmm2 = [2,3,6,7,10,11,14,15,14,15,10,11,12,13,14,15]
; AVX-NEXT:    vpshufb %xmm2, %xmm1, %xmm3
; AVX-NEXT:    vpshufb %xmm2, %xmm0, %xmm2
; AVX-NEXT:    vpunpcklqdq {{.*#+}} xmm2 = xmm2[0],xmm3[0]
; AVX-NEXT:    vpxor %xmm3, %xmm3, %xmm3
; AVX-NEXT:    vpblendw {{.*#+}} xmm1 = xmm1[0],xmm3[1],xmm1[2],xmm3[3],xmm1[4],xmm3[5],xmm1[6],xmm3[7]
; AVX-NEXT:    vpblendw {{.*#+}} xmm0 = xmm0[0],xmm3[1],xmm0[2],xmm3[3],xmm0[4],xmm3[5],xmm0[6],xmm3[7]
; AVX-NEXT:    vpackusdw %xmm1, %xmm0, %xmm0
; AVX-NEXT:    vpsubw %xmm0, %xmm2, %xmm0
; AVX-NEXT:    retq
  %a = shufflevector <8 x i16> %x, <8 x i16> %y, <8 x i32> <i32 1, i32 3, i32 5, i32 7, i32 9, i32 11, i32 13, i32 15>
  %b = shufflevector <8 x i16> %x, <8 x i16> %y, <8 x i32> <i32 0, i32 2, i32 4, i32 6, i32 8, i32 10, i32 12, i32 14>
  %r = sub <8 x i16> %a, %b
  ret <8 x i16> %r
}

define <4 x i32> @phsubd1_reverse(<4 x i32> %x, <4 x i32> %y) {
; SSSE3-LABEL: phsubd1_reverse:
; SSSE3:       # %bb.0:
; SSSE3-NEXT:    movaps %xmm0, %xmm2
; SSSE3-NEXT:    shufps {{.*#+}} xmm2 = xmm2[1,3],xmm1[1,3]
; SSSE3-NEXT:    shufps {{.*#+}} xmm0 = xmm0[0,2],xmm1[0,2]
; SSSE3-NEXT:    psubd %xmm0, %xmm2
; SSSE3-NEXT:    movdqa %xmm2, %xmm0
; SSSE3-NEXT:    retq
;
; AVX-LABEL: phsubd1_reverse:
; AVX:       # %bb.0:
; AVX-NEXT:    vshufps {{.*#+}} xmm2 = xmm0[1,3],xmm1[1,3]
; AVX-NEXT:    vshufps {{.*#+}} xmm0 = xmm0[0,2],xmm1[0,2]
; AVX-NEXT:    vpsubd %xmm0, %xmm2, %xmm0
; AVX-NEXT:    retq
  %a = shufflevector <4 x i32> %x, <4 x i32> %y, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %b = shufflevector <4 x i32> %x, <4 x i32> %y, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %r = sub <4 x i32> %a, %b
  ret <4 x i32> %r
}

define <4 x i32> @phaddd_single_source1(<4 x i32> %x) {
; SSSE3-LABEL: phaddd_single_source1:
; SSSE3:       # %bb.0:
; SSSE3-NEXT:    phaddd %xmm0, %xmm0
; SSSE3-NEXT:    retq
;
; AVX-LABEL: phaddd_single_source1:
; AVX:       # %bb.0:
; AVX-NEXT:    vphaddd %xmm0, %xmm0, %xmm0
; AVX-NEXT:    retq
  %l = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 undef, i32 undef, i32 0, i32 2>
  %r = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 undef, i32 undef, i32 1, i32 3>
  %add = add <4 x i32> %l, %r
  ret <4 x i32> %add
}

define <4 x i32> @phaddd_single_source2(<4 x i32> %x) {
; SSSE3-LABEL: phaddd_single_source2:
; SSSE3:       # %bb.0:
; SSSE3-NEXT:    phaddd %xmm0, %xmm0
; SSSE3-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[3,2,2,3]
; SSSE3-NEXT:    retq
;
; AVX-LABEL: phaddd_single_source2:
; AVX:       # %bb.0:
; AVX-NEXT:    vphaddd %xmm0, %xmm0, %xmm0
; AVX-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[3,2,2,3]
; AVX-NEXT:    retq
  %l = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 undef, i32 undef, i32 0, i32 2>
  %r = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 undef, i32 undef, i32 1, i32 3>
  %add = add <4 x i32> %l, %r
  %shuffle2 = shufflevector <4 x i32> %add, <4 x i32> undef, <4 x i32> <i32 3, i32 2, i32 undef, i32 undef>
  ret <4 x i32> %shuffle2
}

define <4 x i32> @phaddd_single_source3(<4 x i32> %x) {
; SSSE3-LABEL: phaddd_single_source3:
; SSSE3:       # %bb.0:
; SSSE3-NEXT:    phaddd %xmm0, %xmm0
; SSSE3-NEXT:    retq
;
; AVX-LABEL: phaddd_single_source3:
; AVX:       # %bb.0:
; AVX-NEXT:    vphaddd %xmm0, %xmm0, %xmm0
; AVX-NEXT:    retq
  %l = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 undef, i32 undef, i32 0, i32 undef>
  %r = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 undef, i32 undef, i32 1, i32 undef>
  %add = add <4 x i32> %l, %r
  ret <4 x i32> %add
}

define <4 x i32> @phaddd_single_source4(<4 x i32> %x) {
; SSSE3-SLOW-LABEL: phaddd_single_source4:
; SSSE3-SLOW:       # %bb.0:
; SSSE3-SLOW-NEXT:    pshufd {{.*#+}} xmm1 = xmm0[2,2,2,2]
; SSSE3-SLOW-NEXT:    paddd %xmm1, %xmm0
; SSSE3-SLOW-NEXT:    retq
;
; SSSE3-FAST-LABEL: phaddd_single_source4:
; SSSE3-FAST:       # %bb.0:
; SSSE3-FAST-NEXT:    phaddd %xmm0, %xmm0
; SSSE3-FAST-NEXT:    retq
;
; AVX-SLOW-LABEL: phaddd_single_source4:
; AVX-SLOW:       # %bb.0:
; AVX-SLOW-NEXT:    vpshufd {{.*#+}} xmm1 = xmm0[2,2,2,2]
; AVX-SLOW-NEXT:    vpaddd %xmm0, %xmm1, %xmm0
; AVX-SLOW-NEXT:    retq
;
; AVX-FAST-LABEL: phaddd_single_source4:
; AVX-FAST:       # %bb.0:
; AVX-FAST-NEXT:    vphaddd %xmm0, %xmm0, %xmm0
; AVX-FAST-NEXT:    retq
;
; AVX2-SHUF-LABEL: phaddd_single_source4:
; AVX2-SHUF:       # %bb.0:
; AVX2-SHUF-NEXT:    vpshufd {{.*#+}} xmm1 = xmm0[2,2,2,2]
; AVX2-SHUF-NEXT:    vpaddd %xmm0, %xmm1, %xmm0
; AVX2-SHUF-NEXT:    retq
  %l = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 undef, i32 undef, i32 undef, i32 2>
  %add = add <4 x i32> %l, %x
  ret <4 x i32> %add
}

define <4 x i32> @phaddd_single_source5(<4 x i32> %x) {
; SSSE3-SLOW-LABEL: phaddd_single_source5:
; SSSE3-SLOW:       # %bb.0:
; SSSE3-SLOW-NEXT:    pshufd {{.*#+}} xmm1 = xmm0[2,2,2,2]
; SSSE3-SLOW-NEXT:    paddd %xmm0, %xmm1
; SSSE3-SLOW-NEXT:    pshufd {{.*#+}} xmm0 = xmm1[3,3,3,3]
; SSSE3-SLOW-NEXT:    retq
;
; SSSE3-FAST-LABEL: phaddd_single_source5:
; SSSE3-FAST:       # %bb.0:
; SSSE3-FAST-NEXT:    phaddd %xmm0, %xmm0
; SSSE3-FAST-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[3,3,3,3]
; SSSE3-FAST-NEXT:    retq
;
; AVX-SLOW-LABEL: phaddd_single_source5:
; AVX-SLOW:       # %bb.0:
; AVX-SLOW-NEXT:    vpshufd {{.*#+}} xmm1 = xmm0[2,2,2,2]
; AVX-SLOW-NEXT:    vpaddd %xmm0, %xmm1, %xmm0
; AVX-SLOW-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[3,3,3,3]
; AVX-SLOW-NEXT:    retq
;
; AVX-FAST-LABEL: phaddd_single_source5:
; AVX-FAST:       # %bb.0:
; AVX-FAST-NEXT:    vphaddd %xmm0, %xmm0, %xmm0
; AVX-FAST-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[3,3,3,3]
; AVX-FAST-NEXT:    retq
;
; AVX2-SHUF-LABEL: phaddd_single_source5:
; AVX2-SHUF:       # %bb.0:
; AVX2-SHUF-NEXT:    vpshufd {{.*#+}} xmm1 = xmm0[2,2,2,2]
; AVX2-SHUF-NEXT:    vpaddd %xmm0, %xmm1, %xmm0
; AVX2-SHUF-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[3,3,3,3]
; AVX2-SHUF-NEXT:    retq
  %l = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 undef, i32 undef, i32 undef, i32 2>
  %add = add <4 x i32> %l, %x
  %shuffle2 = shufflevector <4 x i32> %add, <4 x i32> undef, <4 x i32> <i32 3, i32 undef, i32 undef, i32 undef>
  ret <4 x i32> %shuffle2
}

define <4 x i32> @phaddd_single_source6(<4 x i32> %x) {
; SSSE3-LABEL: phaddd_single_source6:
; SSSE3:       # %bb.0:
; SSSE3-NEXT:    phaddd %xmm0, %xmm0
; SSSE3-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[2,2,3,3]
; SSSE3-NEXT:    retq
;
; AVX-LABEL: phaddd_single_source6:
; AVX:       # %bb.0:
; AVX-NEXT:    vphaddd %xmm0, %xmm0, %xmm0
; AVX-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[2,2,3,3]
; AVX-NEXT:    retq
  %l = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 undef, i32 undef, i32 0, i32 undef>
  %r = shufflevector <4 x i32> %x, <4 x i32> undef, <4 x i32> <i32 undef, i32 undef, i32 1, i32 undef>
  %add = add <4 x i32> %l, %r
  %shuffle2 = shufflevector <4 x i32> %add, <4 x i32> undef, <4 x i32> <i32 undef, i32 2, i32 undef, i32 undef>
  ret <4 x i32> %shuffle2
}

define <8 x i16> @phaddw_single_source1(<8 x i16> %x) {
; SSSE3-LABEL: phaddw_single_source1:
; SSSE3:       # %bb.0:
; SSSE3-NEXT:    phaddw %xmm0, %xmm0
; SSSE3-NEXT:    retq
;
; AVX-LABEL: phaddw_single_source1:
; AVX:       # %bb.0:
; AVX-NEXT:    vphaddw %xmm0, %xmm0, %xmm0
; AVX-NEXT:    retq
  %l = shufflevector <8 x i16> %x, <8 x i16> undef, <8 x i32> <i32 undef, i32 undef, i32 undef, i32 undef, i32 0, i32 2, i32 4, i32 6>
  %r = shufflevector <8 x i16> %x, <8 x i16> undef, <8 x i32> <i32 undef, i32 undef, i32 undef, i32 undef, i32 1, i32 3, i32 5, i32 7>
  %add = add <8 x i16> %l, %r
  ret <8 x i16> %add
}

define <8 x i16> @phaddw_single_source2(<8 x i16> %x) {
; SSSE3-LABEL: phaddw_single_source2:
; SSSE3:       # %bb.0:
; SSSE3-NEXT:    phaddw %xmm0, %xmm0
; SSSE3-NEXT:    pshufhw {{.*#+}} xmm0 = xmm0[0,1,2,3,5,4,6,7]
; SSSE3-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[2,2,2,2]
; SSSE3-NEXT:    retq
;
; AVX-SLOW-LABEL: phaddw_single_source2:
; AVX-SLOW:       # %bb.0:
; AVX-SLOW-NEXT:    vphaddw %xmm0, %xmm0, %xmm0
; AVX-SLOW-NEXT:    vpshufhw {{.*#+}} xmm0 = xmm0[0,1,2,3,5,4,6,7]
; AVX-SLOW-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[2,2,2,2]
; AVX-SLOW-NEXT:    retq
;
; AVX-FAST-LABEL: phaddw_single_source2:
; AVX-FAST:       # %bb.0:
; AVX-FAST-NEXT:    vphaddw %xmm0, %xmm0, %xmm0
; AVX-FAST-NEXT:    vpshufhw {{.*#+}} xmm0 = xmm0[0,1,2,3,5,4,6,7]
; AVX-FAST-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[2,2,2,2]
; AVX-FAST-NEXT:    retq
;
; AVX2-SHUF-LABEL: phaddw_single_source2:
; AVX2-SHUF:       # %bb.0:
; AVX2-SHUF-NEXT:    vphaddw %xmm0, %xmm0, %xmm0
; AVX2-SHUF-NEXT:    vpshufb {{.*#+}} xmm0 = xmm0[10,11,8,9,10,11,8,9,10,11,8,9,10,11,8,9]
; AVX2-SHUF-NEXT:    retq
  %l = shufflevector <8 x i16> %x, <8 x i16> undef, <8 x i32> <i32 undef, i32 undef, i32 undef, i32 undef, i32 0, i32 2, i32 4, i32 6>
  %r = shufflevector <8 x i16> %x, <8 x i16> undef, <8 x i32> <i32 undef, i32 undef, i32 undef, i32 undef, i32 1, i32 3, i32 5, i32 7>
  %add = add <8 x i16> %l, %r
  %shuffle2 = shufflevector <8 x i16> %add, <8 x i16> undef, <8 x i32> <i32 5, i32 4, i32 3, i32 2, i32 undef, i32 undef, i32 undef, i32 undef>
  ret <8 x i16> %shuffle2
}

define <8 x i16> @phaddw_single_source3(<8 x i16> %x) {
; SSSE3-LABEL: phaddw_single_source3:
; SSSE3:       # %bb.0:
; SSSE3-NEXT:    phaddw %xmm0, %xmm0
; SSSE3-NEXT:    retq
;
; AVX-LABEL: phaddw_single_source3:
; AVX:       # %bb.0:
; AVX-NEXT:    vphaddw %xmm0, %xmm0, %xmm0
; AVX-NEXT:    retq
  %l = shufflevector <8 x i16> %x, <8 x i16> undef, <8 x i32> <i32 undef, i32 undef, i32 undef, i32 undef, i32 0, i32 2, i32 undef, i32 undef>
  %r = shufflevector <8 x i16> %x, <8 x i16> undef, <8 x i32> <i32 undef, i32 undef, i32 undef, i32 undef, i32 1, i32 3, i32 undef, i32 undef>
  %add = add <8 x i16> %l, %r
  ret <8 x i16> %add
}

define <8 x i16> @phaddw_single_source4(<8 x i16> %x) {
; SSSE3-SLOW-LABEL: phaddw_single_source4:
; SSSE3-SLOW:       # %bb.0:
; SSSE3-SLOW-NEXT:    movdqa %xmm0, %xmm1
; SSSE3-SLOW-NEXT:    pslld $16, %xmm1
; SSSE3-SLOW-NEXT:    paddw %xmm0, %xmm1
; SSSE3-SLOW-NEXT:    movdqa %xmm1, %xmm0
; SSSE3-SLOW-NEXT:    retq
;
; SSSE3-FAST-LABEL: phaddw_single_source4:
; SSSE3-FAST:       # %bb.0:
; SSSE3-FAST-NEXT:    phaddw %xmm0, %xmm0
; SSSE3-FAST-NEXT:    retq
;
; AVX-SLOW-LABEL: phaddw_single_source4:
; AVX-SLOW:       # %bb.0:
; AVX-SLOW-NEXT:    vpslld $16, %xmm0, %xmm1
; AVX-SLOW-NEXT:    vpaddw %xmm0, %xmm1, %xmm0
; AVX-SLOW-NEXT:    retq
;
; AVX-FAST-LABEL: phaddw_single_source4:
; AVX-FAST:       # %bb.0:
; AVX-FAST-NEXT:    vphaddw %xmm0, %xmm0, %xmm0
; AVX-FAST-NEXT:    retq
;
; AVX2-SHUF-LABEL: phaddw_single_source4:
; AVX2-SHUF:       # %bb.0:
; AVX2-SHUF-NEXT:    vpslld $16, %xmm0, %xmm1
; AVX2-SHUF-NEXT:    vpaddw %xmm0, %xmm1, %xmm0
; AVX2-SHUF-NEXT:    retq
  %l = shufflevector <8 x i16> %x, <8 x i16> undef, <8 x i32> <i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 6>
  %add = add <8 x i16> %l, %x
  ret <8 x i16> %add
}

define <8 x i16> @phaddw_single_source6(<8 x i16> %x) {
; SSSE3-LABEL: phaddw_single_source6:
; SSSE3:       # %bb.0:
; SSSE3-NEXT:    phaddw %xmm0, %xmm0
; SSSE3-NEXT:    psrldq {{.*#+}} xmm0 = xmm0[6,7,8,9,10,11,12,13,14,15],zero,zero,zero,zero,zero,zero
; SSSE3-NEXT:    retq
;
; AVX-LABEL: phaddw_single_source6:
; AVX:       # %bb.0:
; AVX-NEXT:    vphaddw %xmm0, %xmm0, %xmm0
; AVX-NEXT:    vpsrldq {{.*#+}} xmm0 = xmm0[6,7,8,9,10,11,12,13,14,15],zero,zero,zero,zero,zero,zero
; AVX-NEXT:    retq
  %l = shufflevector <8 x i16> %x, <8 x i16> undef, <8 x i32> <i32 undef, i32 undef, i32 undef, i32 undef, i32 0, i32 undef, i32 undef, i32 undef>
  %r = shufflevector <8 x i16> %x, <8 x i16> undef, <8 x i32> <i32 undef, i32 undef, i32 undef, i32 undef, i32 1, i32 undef, i32 undef, i32 undef>
  %add = add <8 x i16> %l, %r
  %shuffle2 = shufflevector <8 x i16> %add, <8 x i16> undef, <8 x i32> <i32 undef, i32 4, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>
  ret <8 x i16> %shuffle2
}

; PR39921 + PR39936
define i32 @PR39936_v8i32(<8 x i32>) {
; SSSE3-SLOW-LABEL: PR39936_v8i32:
; SSSE3-SLOW:       # %bb.0:
; SSSE3-SLOW-NEXT:    phaddd %xmm1, %xmm0
; SSSE3-SLOW-NEXT:    phaddd %xmm0, %xmm0
; SSSE3-SLOW-NEXT:    pshufd {{.*#+}} xmm1 = xmm0[1,1,1,1]
; SSSE3-SLOW-NEXT:    paddd %xmm0, %xmm1
; SSSE3-SLOW-NEXT:    movd %xmm1, %eax
; SSSE3-SLOW-NEXT:    retq
;
; SSSE3-FAST-LABEL: PR39936_v8i32:
; SSSE3-FAST:       # %bb.0:
; SSSE3-FAST-NEXT:    phaddd %xmm1, %xmm0
; SSSE3-FAST-NEXT:    phaddd %xmm0, %xmm0
; SSSE3-FAST-NEXT:    phaddd %xmm0, %xmm0
; SSSE3-FAST-NEXT:    movd %xmm0, %eax
; SSSE3-FAST-NEXT:    retq
;
; AVX1-SLOW-LABEL: PR39936_v8i32:
; AVX1-SLOW:       # %bb.0:
; AVX1-SLOW-NEXT:    vextractf128 $1, %ymm0, %xmm1
; AVX1-SLOW-NEXT:    vphaddd %xmm1, %xmm0, %xmm0
; AVX1-SLOW-NEXT:    vphaddd %xmm0, %xmm0, %xmm0
; AVX1-SLOW-NEXT:    vpshufd {{.*#+}} xmm1 = xmm0[1,1,1,1]
; AVX1-SLOW-NEXT:    vpaddd %xmm0, %xmm1, %xmm0
; AVX1-SLOW-NEXT:    vmovd %xmm0, %eax
; AVX1-SLOW-NEXT:    vzeroupper
; AVX1-SLOW-NEXT:    retq
;
; AVX1-FAST-LABEL: PR39936_v8i32:
; AVX1-FAST:       # %bb.0:
; AVX1-FAST-NEXT:    vextractf128 $1, %ymm0, %xmm1
; AVX1-FAST-NEXT:    vphaddd %xmm1, %xmm0, %xmm0
; AVX1-FAST-NEXT:    vphaddd %xmm0, %xmm0, %xmm0
; AVX1-FAST-NEXT:    vphaddd %xmm0, %xmm0, %xmm0
; AVX1-FAST-NEXT:    vmovd %xmm0, %eax
; AVX1-FAST-NEXT:    vzeroupper
; AVX1-FAST-NEXT:    retq
;
; AVX2-SLOW-LABEL: PR39936_v8i32:
; AVX2-SLOW:       # %bb.0:
; AVX2-SLOW-NEXT:    vextracti128 $1, %ymm0, %xmm1
; AVX2-SLOW-NEXT:    vphaddd %xmm1, %xmm0, %xmm0
; AVX2-SLOW-NEXT:    vphaddd %xmm0, %xmm0, %xmm0
; AVX2-SLOW-NEXT:    vpshufd {{.*#+}} xmm1 = xmm0[1,1,1,1]
; AVX2-SLOW-NEXT:    vpaddd %xmm0, %xmm1, %xmm0
; AVX2-SLOW-NEXT:    vmovd %xmm0, %eax
; AVX2-SLOW-NEXT:    vzeroupper
; AVX2-SLOW-NEXT:    retq
;
; AVX2-FAST-LABEL: PR39936_v8i32:
; AVX2-FAST:       # %bb.0:
; AVX2-FAST-NEXT:    vextracti128 $1, %ymm0, %xmm1
; AVX2-FAST-NEXT:    vphaddd %xmm1, %xmm0, %xmm0
; AVX2-FAST-NEXT:    vphaddd %xmm0, %xmm0, %xmm0
; AVX2-FAST-NEXT:    vphaddd %xmm0, %xmm0, %xmm0
; AVX2-FAST-NEXT:    vmovd %xmm0, %eax
; AVX2-FAST-NEXT:    vzeroupper
; AVX2-FAST-NEXT:    retq
;
; AVX2-SHUF-LABEL: PR39936_v8i32:
; AVX2-SHUF:       # %bb.0:
; AVX2-SHUF-NEXT:    vextracti128 $1, %ymm0, %xmm1
; AVX2-SHUF-NEXT:    vphaddd %xmm1, %xmm0, %xmm0
; AVX2-SHUF-NEXT:    vphaddd %xmm0, %xmm0, %xmm0
; AVX2-SHUF-NEXT:    vpshufd {{.*#+}} xmm1 = xmm0[1,1,1,1]
; AVX2-SHUF-NEXT:    vpaddd %xmm0, %xmm1, %xmm0
; AVX2-SHUF-NEXT:    vmovd %xmm0, %eax
; AVX2-SHUF-NEXT:    vzeroupper
; AVX2-SHUF-NEXT:    retq
  %2 = shufflevector <8 x i32> %0, <8 x i32> undef, <8 x i32> <i32 0, i32 2, i32 4, i32 6, i32 undef, i32 undef, i32 undef, i32 undef>
  %3 = shufflevector <8 x i32> %0, <8 x i32> undef, <8 x i32> <i32 1, i32 3, i32 5, i32 7, i32 undef, i32 undef, i32 undef, i32 undef>
  %4 = add <8 x i32> %2, %3
  %5 = shufflevector <8 x i32> %4, <8 x i32> undef, <8 x i32> <i32 0, i32 2, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>
  %6 = shufflevector <8 x i32> %4, <8 x i32> undef, <8 x i32> <i32 1, i32 3, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>
  %7 = add <8 x i32> %5, %6
  %8 = shufflevector <8 x i32> %7, <8 x i32> undef, <8 x i32> <i32 1, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef, i32 undef>
  %9 = add <8 x i32> %8, %7
  %10 = extractelement <8 x i32> %9, i32 0
  ret i32 %10
}
