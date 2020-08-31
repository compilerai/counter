#define CBRANCH_cond(cond, suf, dst) \
  1: b ## cond ## suf dst; \
  1: b ## cond ## suf ## l dst

#define CBRANCH_cond2(cond, bit, dst) \
  1: b ## cond bit, dst; \
  1: b ## cond ## l bit, dst

#define CBRANCH(suf, dst) \
  CBRANCH_cond(gt, suf, dst); \
  CBRANCH_cond(gt, suf, dst); \
  CBRANCH_cond(ge, suf, dst); \
  CBRANCH_cond(lt, suf, dst); \
  CBRANCH_cond(un, suf, dst); \
  CBRANCH_cond(nl, suf, dst); \
  CBRANCH_cond(so, suf, dst); \
  CBRANCH_cond(eq, suf, dst); \
  CBRANCH_cond(ne, suf, dst); \
  CBRANCH_cond(ns, suf, dst); \
  CBRANCH_cond(ng, suf, dst); \
  CBRANCH_cond(nu, suf, dst); \
  CBRANCH_cond(le, suf, dst); 1:
#define CBRANCH2(suf) \
  CBRANCH_cond(dz, suf,); \
  CBRANCH_cond(dnz, suf,); \
  CBRANCH_cond(dzt, suf, 4*cr0+eq); \
  CBRANCH_cond(dzf, suf, 4*cr0+eq); \
  CBRANCH_cond(dnzt, suf, 4*cr0+eq); \
  CBRANCH_cond(dnzf, suf, 4*cr0+eq);\
  CBRANCH_cond(dzt, suf, 4*cr0+lt); \
  CBRANCH_cond(dzf, suf, 4*cr0+lt); \
  CBRANCH_cond(dnzt, suf, 4*cr0+lt); \
  CBRANCH_cond(dnzf, suf, 4*cr0+lt); \
  CBRANCH_cond(dzt, suf, 4*cr0+gt); \
  CBRANCH_cond(dzf, suf, 4*cr0+gt); \
  CBRANCH_cond(dnzt, suf, 4*cr0+gt); \
  CBRANCH_cond(dnzf, suf, 4*cr0+gt); \
  CBRANCH_cond(dzt, suf, 4*cr0+so); \
  CBRANCH_cond(dzf, suf, 4*cr0+so); \
  CBRANCH_cond(dnzt, suf, 4*cr0+so); \
  CBRANCH_cond(dnzf, suf, 4*cr0+so); 1:
#define CBRANCH3(dst) \
  CBRANCH_cond(dz,,dst); \
  CBRANCH_cond(dnz,,dst); \
  CBRANCH_cond2(dzt, 4*cr0+eq, dst); \
  CBRANCH_cond2(dzf, 4*cr0+eq, dst); \
  CBRANCH_cond2(dnzt, 4*cr0+eq, dst); \
  CBRANCH_cond2(dnzf, 4*cr0+eq, dst);\
  CBRANCH_cond2(dzt, 4*cr0+lt, dst); \
  CBRANCH_cond2(dzf, 4*cr0+lt, dst); \
  CBRANCH_cond2(dnzt, 4*cr0+lt, dst); \
  CBRANCH_cond2(dnzf, 4*cr0+lt, dst); \
  CBRANCH_cond2(dzt, 4*cr0+gt, dst); \
  CBRANCH_cond2(dzf, 4*cr0+gt, dst); \
  CBRANCH_cond2(dnzt, 4*cr0+gt, dst); \
  CBRANCH_cond2(dnzf, 4*cr0+gt, dst); \
  CBRANCH_cond2(dzt, 4*cr0+so, dst); \
  CBRANCH_cond2(dzf, 4*cr0+so, dst); \
  CBRANCH_cond2(dnzt, 4*cr0+so, dst); \
  CBRANCH_cond2(dnzf, 4*cr0+so, dst); 1:

#define BRANCH(suf, dst) \
  1: bne 2f; \
  1: b ## suf dst; \
  2: b ## suf ## l dst; \
  1:

#define CBRANCH20(op, dst) \
  bne 2f; \
  1: bc 20, op, dst; \
  2: bc 20, op, 1f; \
  1: bcl 20, op, dst; \
  1: bcl 20, op, 1f; \
  1:

#define CRBOOL(opc) \
  opc 4*cr1+lt,4*cr2+lt,4*cr3+lt; \
  opc 4*cr1+lt,4*cr2+lt,4*cr3+gt; \
  opc 4*cr1+lt,4*cr2+lt,4*cr3+eq; \
  opc 4*cr1+lt,4*cr2+lt,4*cr3+so; \
  opc 4*cr1+lt,4*cr2+gt,4*cr3+lt; \
  opc 4*cr1+lt,4*cr2+gt,4*cr3+gt; \
  opc 4*cr1+lt,4*cr2+gt,4*cr3+eq; \
  opc 4*cr1+lt,4*cr2+gt,4*cr3+so; \
  opc 4*cr1+lt,4*cr2+eq,4*cr3+lt; \
  opc 4*cr1+lt,4*cr2+eq,4*cr3+gt; \
  opc 4*cr1+lt,4*cr2+eq,4*cr3+eq; \
  opc 4*cr1+lt,4*cr2+eq,4*cr3+so; \
  opc 4*cr1+lt,4*cr2+so,4*cr3+lt; \
  opc 4*cr1+lt,4*cr2+so,4*cr3+gt; \
  opc 4*cr1+lt,4*cr2+so,4*cr3+eq; \
  opc 4*cr1+lt,4*cr2+so,4*cr3+so; \
  opc 4*cr1+gt,4*cr2+lt,4*cr3+lt; \
  opc 4*cr1+gt,4*cr2+lt,4*cr3+gt; \
  opc 4*cr1+gt,4*cr2+lt,4*cr3+eq; \
  opc 4*cr1+gt,4*cr2+lt,4*cr3+so; \
  opc 4*cr1+gt,4*cr2+gt,4*cr3+lt; \
  opc 4*cr1+gt,4*cr2+gt,4*cr3+gt; \
  opc 4*cr1+gt,4*cr2+gt,4*cr3+eq; \
  opc 4*cr1+gt,4*cr2+gt,4*cr3+so; \
  opc 4*cr1+gt,4*cr2+eq,4*cr3+lt; \
  opc 4*cr1+gt,4*cr2+eq,4*cr3+gt; \
  opc 4*cr1+gt,4*cr2+eq,4*cr3+eq; \
  opc 4*cr1+gt,4*cr2+eq,4*cr3+so; \
  opc 4*cr1+gt,4*cr2+so,4*cr3+lt; \
  opc 4*cr1+gt,4*cr2+so,4*cr3+gt; \
  opc 4*cr1+gt,4*cr2+so,4*cr3+eq; \
  opc 4*cr1+gt,4*cr2+so,4*cr3+so; \
  opc 4*cr1+eq,4*cr2+lt,4*cr3+lt; \
  opc 4*cr1+eq,4*cr2+lt,4*cr3+gt; \
  opc 4*cr1+eq,4*cr2+lt,4*cr3+eq; \
  opc 4*cr1+eq,4*cr2+lt,4*cr3+so; \
  opc 4*cr1+eq,4*cr2+gt,4*cr3+lt; \
  opc 4*cr1+eq,4*cr2+gt,4*cr3+gt; \
  opc 4*cr1+eq,4*cr2+gt,4*cr3+eq; \
  opc 4*cr1+eq,4*cr2+gt,4*cr3+so; \
  opc 4*cr1+eq,4*cr2+eq,4*cr3+lt; \
  opc 4*cr1+eq,4*cr2+eq,4*cr3+gt; \
  opc 4*cr1+eq,4*cr2+eq,4*cr3+eq; \
  opc 4*cr1+eq,4*cr2+eq,4*cr3+so; \
  opc 4*cr1+eq,4*cr2+so,4*cr3+lt; \
  opc 4*cr1+eq,4*cr2+so,4*cr3+gt; \
  opc 4*cr1+eq,4*cr2+so,4*cr3+eq; \
  opc 4*cr1+eq,4*cr2+so,4*cr3+so; \
  opc 4*cr1+so,4*cr2+lt,4*cr3+lt; \
  opc 4*cr1+so,4*cr2+lt,4*cr3+gt; \
  opc 4*cr1+so,4*cr2+lt,4*cr3+eq; \
  opc 4*cr1+so,4*cr2+lt,4*cr3+so; \
  opc 4*cr1+so,4*cr2+gt,4*cr3+lt; \
  opc 4*cr1+so,4*cr2+gt,4*cr3+gt; \
  opc 4*cr1+so,4*cr2+gt,4*cr3+eq; \
  opc 4*cr1+so,4*cr2+gt,4*cr3+so; \
  opc 4*cr1+so,4*cr2+eq,4*cr3+lt; \
  opc 4*cr1+so,4*cr2+eq,4*cr3+gt; \
  opc 4*cr1+so,4*cr2+eq,4*cr3+eq; \
  opc 4*cr1+so,4*cr2+eq,4*cr3+so; \
  opc 4*cr1+so,4*cr2+so,4*cr3+lt; \
  opc 4*cr1+so,4*cr2+so,4*cr3+gt; \
  opc 4*cr1+so,4*cr2+so,4*cr3+eq; \
  opc 4*cr1+so,4*cr2+so,4*cr3+so

.global _start
_start:
dcbi 1,2
nop
#li 9,100
#lis 9,100
#
#addi 9,10,100
#addis 9,10,100
#addic 9,10,100
#addic. 9,10,100
#
#addi 9,0,100
#addis 9,0,100
#addic 9,0,100
#addic. 9,0,100
#
#subi 9,10,100
#subo 9,10,11
#subo. 9,10,11
#subfo 9,10,11
#subfo. 9,10,11
#subfc 9,10,11
#subfc. 9,10,11
#subfco 9,10,11
#subfco. 9,10,11
#subis 9,10,100
#subfic 9,10,100
#
#subfe 4,5,6
#subfe. 4,5,6
#subfeo 4,5,6
#subfeo. 4,5,6
#subfze 9,10
#subfze. 9,10
#subfme 9,10
#subfme. 9,10
#subfzeo 9,10
#subfzeo. 9,10
#subfmeo 9,10
#subfmeo. 9,10
#
#add 11,7,8
#add. 11,7,8
#addc 11,7,8
#addc. 11,7,8
#addco 11,7,8
#addco. 11,7,8
#adde 11,7,8
#adde. 11,7,8
#addeo 11,7,8
#addeo. 11,7,8
#addme 11,7
#addme. 11,7
#addmeo 11,7
#addo 11,7,8
#addo. 11,7,8
#addze 11,7
#addze. 11,7
#addzeo 11,7
#addzeo. 11,7
#
#and 11,7,8
#and. 11,7,8
#andc 11,7,8
#andc. 11,7,8
#andi. 11,7,199
#andis. 11,7,199
#
#
#sub 11,7,8
#sub. 11,7,8
#subf 11,7,8
#subf. 11,7,8
#mr 7,8
#mr. 7,8
#
#sc
#
#cmp 1,9,10
#cmpi 1,9,100
#cmpw 1,9,10
#cmpwi 1,9,100
#cmpl 1,9,10
#cmpli 1,9,100
#
#CRBOOL(crxor)
#CRBOOL(cror)
#CRBOOL(crnor)
#CRBOOL(creqv)
#CRBOOL(crand)
#
#lbz 18,23(4)
#lbzx 18,4,5
#
#lbz 18,23(0)
#lbzx 18,0,5
#
#lbzu 18,23(4)
#lbzux 18,4,5
#
#lwz 18,23(4)
#lwzx 18,4,5
#
#lwz 18,23(0)
#lwzx 18,0,5
#
#lwzu 18,23(4)
#lwzux 18,4,5
#
#lhz 18,23(4)
#lhzx 18,4,5
#
#lhz 18,23(0)
#lhzx 18,0,5
#
#lhzu 18,23(4)
#lhzux 18,4,5
#
#lha 18,23(4)
#lhax 18,4,5
#
#lha 18,23(0)
#lhax 18,0,5
#
#lhau 18,23(4)
#lhaux 18,4,5
#
#stbu 18,23(4)
#
#stb 18,23(4)
#stbx 18,4,5
#
#stb 18,23(0)
#stbx 18,0,5
#
#stbux 18,4,5
#
#stw 18,23(4)
#stwx 18,4,5
#
#stw 18,23(0)
#stwx 18,0,5
#
#stwu 18,23(4)
#stwux 18,4,5
#
#sth 18,23(4)
#sthx 18,4,5
#
#sth 18,23(0)
#sthx 18,0,5
#
#sthu 18,23(4)
#sthux 18,4,5
#
#lhbrx 1,2,3
#lwbrx 1,2,3
#sthbrx 1,2,3
#stwbrx 1,2,3
#
#lhbrx 1,0,3
#lwbrx 1,0,3
#sthbrx 1,0,3
#stwbrx 1,0,3
#
##lmw 4,100(5)
##stmw 4,100(5)
#
#rlwinm 1,2,3,5,7
#rlwinm. 1,2,3,5,7
#rlwnm 1,2,3,5,7
#rlwnm. 1,2,3,5,7
#
#slw 3,4,5
#slw. 3,4,5
#srw 3,4,5
#srw. 3,4,5
#srawi 3,4,5
#srawi. 3,4,5
#sraw 3,4,5
#sraw. 3,4,5
#
##rlwimi opcodes are handled specially in ppc/peep.trans.tab.base as
##our fbqemu algorithm is not able to represent a zero mask using 'masktill'
##modifier.
##rlwimi 1,2,3,5,7
##rlwimi. 1,2,3,5,7
#
#andi. 3,1,100
#andis. 3,1,100
#ori 3,1,100
#oris 3,1,100
#xori 3,1,100
#xoris 3,1,100
#
#and 3,1,2
#and. 3,1,2
#andc 3,1,2
#andc. 3,1,2
#or 3,1,2
#or. 3,1,2
#orc 3,1,2
#orc. 3,1,2
#nor 3,1,2
#nor. 3,1,2
#nand 3,1,2
#nand. 3,1,2
#xor 3,1,2
#xor. 3,1,2
#eqv 3,1,2
#eqv. 3,1,2
#
#BRANCH(lr,)
#CBRANCH(lr,)
#CBRANCH2(lr)
#BRANCH(ctr,)
#CBRANCH(ctr,)
#BRANCH(,_end)
#CBRANCH(,_end)
#CBRANCH3(_end)
#CBRANCH(,1f)
#CBRANCH3(1f)
#1: bl 1f
#1: CBRANCH20(4*cr7+so, _end)
#1:
##clrlslwi
##clrlslwi.
#
#mflr 1
#mtlr 1
#
#mfctr 1
#mtctr 1
#
#mfcr 1
#mcrf 1,2
#mcrxr 1
#
#mtcrf 0,12
#mtcrf 1,12
#mtcrf 2,12
#mtcrf 3,12
#mtcrf 4,12
#mtcrf 5,12
#mtcrf 6,12
#mtcrf 7,12
#mtcrf 8,12
#mtcrf 9,12
#mtcrf 10,12
#mtcrf 11,12
#mtcrf 12,12
#mtcrf 13,12
#mtcrf 14,12
#mtcrf 15,12
#mtcrf 16,12
#mtcrf 17,12
#mtcrf 18,12
#mtcrf 19,12
#mtcrf 20,12
#mtcrf 21,12
#mtcrf 22,12
#mtcrf 23,12
#mtcrf 24,12
#mtcrf 25,12
#mtcrf 26,12
#mtcrf 27,12
#mtcrf 28,12
#mtcrf 29,12
#mtcrf 30,12
#mtcrf 31,12
#mtcrf 32,12
#mtcrf 33,12
#mtcrf 34,12
#mtcrf 35,12
#mtcrf 36,12
#mtcrf 37,12
#mtcrf 38,12
#mtcrf 39,12
#mtcrf 40,12
#mtcrf 41,12
#mtcrf 42,12
#mtcrf 43,12
#mtcrf 44,12
#mtcrf 45,12
#mtcrf 46,12
#mtcrf 47,12
#mtcrf 48,12
#mtcrf 49,12
#mtcrf 50,12
#mtcrf 51,12
#mtcrf 52,12
#mtcrf 53,12
#mtcrf 54,12
#mtcrf 55,12
#mtcrf 56,12
#mtcrf 57,12
#mtcrf 58,12
#mtcrf 59,12
#mtcrf 60,12
#mtcrf 61,12
#mtcrf 62,12
#mtcrf 63,12
#mtcrf 64,12
#mtcrf 65,12
#mtcrf 66,12
#mtcrf 67,12
#mtcrf 68,12
#mtcrf 69,12
#mtcrf 70,12
#mtcrf 71,12
#mtcrf 72,12
#mtcrf 73,12
#mtcrf 74,12
#mtcrf 75,12
#mtcrf 76,12
#mtcrf 77,12
#mtcrf 78,12
#mtcrf 79,12
#mtcrf 80,12
#mtcrf 81,12
#mtcrf 82,12
#mtcrf 83,12
#mtcrf 84,12
#mtcrf 85,12
#mtcrf 86,12
#mtcrf 87,12
#mtcrf 88,12
#mtcrf 89,12
#mtcrf 90,12
#mtcrf 91,12
#mtcrf 92,12
#mtcrf 93,12
#mtcrf 94,12
#mtcrf 95,12
#mtcrf 96,12
#mtcrf 97,12
#mtcrf 98,12
#mtcrf 99,12
#mtcrf 101,12
#mtcrf 102,12
#mtcrf 103,12
#mtcrf 104,12
#mtcrf 105,12
#mtcrf 106,12
#mtcrf 107,12
#mtcrf 108,12
#mtcrf 109,12
#mtcrf 110,12
#mtcrf 111,12
#mtcrf 112,12
#mtcrf 113,12
#mtcrf 114,12
#mtcrf 115,12
#mtcrf 116,12
#mtcrf 117,12
#mtcrf 118,12
#mtcrf 119,12
#mtcrf 120,12
#mtcrf 121,12
#mtcrf 122,12
#mtcrf 123,12
#mtcrf 124,12
#mtcrf 125,12
#mtcrf 126,12
#mtcrf 127,12
#mtcrf 128,12
#mtcrf 129,12
#mtcrf 130,12
#mtcrf 131,12
#mtcrf 132,12
#mtcrf 133,12
#mtcrf 134,12
#mtcrf 135,12
#mtcrf 136,12
#mtcrf 137,12
#mtcrf 138,12
#mtcrf 139,12
#mtcrf 140,12
#mtcrf 141,12
#mtcrf 142,12
#mtcrf 143,12
#mtcrf 144,12
#mtcrf 145,12
#mtcrf 146,12
#mtcrf 147,12
#mtcrf 148,12
#mtcrf 149,12
#mtcrf 150,12
#mtcrf 151,12
#mtcrf 152,12
#mtcrf 153,12
#mtcrf 154,12
#mtcrf 155,12
#mtcrf 156,12
#mtcrf 157,12
#mtcrf 158,12
#mtcrf 159,12
#mtcrf 160,12
#mtcrf 161,12
#mtcrf 162,12
#mtcrf 163,12
#mtcrf 164,12
#mtcrf 165,12
#mtcrf 166,12
#mtcrf 167,12
#mtcrf 168,12
#mtcrf 169,12
#mtcrf 170,12
#mtcrf 171,12
#mtcrf 172,12
#mtcrf 173,12
#mtcrf 174,12
#mtcrf 175,12
#mtcrf 176,12
#mtcrf 177,12
#mtcrf 178,12
#mtcrf 179,12
#mtcrf 180,12
#mtcrf 181,12
#mtcrf 182,12
#mtcrf 183,12
#mtcrf 184,12
#mtcrf 185,12
#mtcrf 186,12
#mtcrf 187,12
#mtcrf 188,12
#mtcrf 189,12
#mtcrf 190,12
#mtcrf 191,12
#mtcrf 192,12
#mtcrf 193,12
#mtcrf 194,12
#mtcrf 195,12
#mtcrf 196,12
#mtcrf 197,12
#mtcrf 198,12
#mtcrf 199,12
#mtcrf 200,12
#mtcrf 201,12
#mtcrf 202,12
#mtcrf 203,12
#mtcrf 204,12
#mtcrf 205,12
#mtcrf 206,12
#mtcrf 207,12
#mtcrf 208,12
#mtcrf 209,12
#mtcrf 210,12
#mtcrf 211,12
#mtcrf 212,12
#mtcrf 213,12
#mtcrf 214,12
#mtcrf 215,12
#mtcrf 216,12
#mtcrf 217,12
#mtcrf 218,12
#mtcrf 219,12
#mtcrf 220,12
#mtcrf 221,12
#mtcrf 222,12
#mtcrf 223,12
#mtcrf 224,12
#mtcrf 225,12
#mtcrf 226,12
#mtcrf 227,12
#mtcrf 228,12
#mtcrf 229,12
#mtcrf 230,12
#mtcrf 231,12
#mtcrf 232,12
#mtcrf 233,12
#mtcrf 234,12
#mtcrf 235,12
#mtcrf 236,12
#mtcrf 237,12
#mtcrf 238,12
#mtcrf 239,12
#mtcrf 240,12
#mtcrf 241,12
#mtcrf 242,12
#mtcrf 243,12
#mtcrf 244,12
#mtcrf 245,12
#mtcrf 246,12
#mtcrf 247,12
#mtcrf 248,12
#mtcrf 249,12
#mtcrf 250,12
#mtcrf 251,12
#mtcrf 252,12
#mtcrf 253,12
#mtcrf 254,12
#mtcrf 255,12
#
#mfspr 2,1
#mtspr 1,2
#
#eieio
#isync
#sync
#
#lwarx 1,2,3
#stwcx. 1,2,3
#
#lwarx 1,0,3
#stwcx. 1,0,3
#
##dcba 1,2
#dcbi 1,2
#dcbf 1,2
#dcbst 1,2
#dcbt 1,2
#dcbtst 1,2
#dcbz 1,2
##icbi 1,2
#
##dcba 0,2
#dcbf 0,2
#dcbst 0,2
#dcbt 0,2
#dcbtst 0,2
#dcbz 0,2
##icbi 0,2
#
##icbt 1,2
#
#
#extsb 3,4
#extsb. 3,4
#extsh 3,4
#extsh. 3,4
#
#cntlzw 3,4
#cntlzw. 3,4
#
#neg 4, 5
#nego 4, 5
#neg. 4, 5
#nego. 4, 5
#
#mulli 4,5,100
#
#mullw 4,5,6
#mullw. 4,5,6
#mullwo 4,5,6
#mullwo. 4,5,6
#mulhw 4,5,6
#mulhw. 4,5,6
#mulhwu 4,5,6
#mulhwu. 4,5,6
#divw 4,5,6
#divw. 4,5,6
#divwo 4,5,6
#divwo. 4,5,6
#divwu 4,5,6
#divwu. 4,5,6
#divwuo 4,5,6
#divwuo. 4,5,6
#
#stfd 14,23(11)
#lfd 14,23(11)
#
##macchw 3,4,5
##macchw. 3,4,5
##macchwo 3,4,5
##macchwo. 3,4,5
##macchws 3,4,5
##macchws. 3,4,5
##macchwso 3,4,5
##macchwso. 3,4,5
##macchwsu 3,4,5
##macchwsu. 3,4,5
#
#nop
#_end:
#nop
