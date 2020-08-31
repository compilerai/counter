#define INSN1r(opc) opc ##l %ebx; opc ##w %bx; opc ##b %bl; opc ##b %bh
#define INSN2rr(opc) opc ##l %ecx, %edx; opc ##w %cx, %dx; opc ##b %cl, %dl; opc ##b %cl, %dh; opc ##b %ch, %dl; opc ##b %ch, %dh
#define INSN2rx16(opc) opc ##w %cx, %dx; INSN2mx_suf(opc, %bx, w 1)
#define INSN2rx32(opc) opc ##l %ecx, %edx; INSN2mx_suf(opc, %ebx, l 1)
#define INSN2ir(opc) opc ##l $123, %edx; opc ##w $123, %dx; opc ##b $123, %dl; opc ##b $123, %dh
#define INSN2xm(opc, x) opc ## x ##, 456; opc ## x ##, 456(%edx); opc ## x ##, 456(%edx, %eax, 1); opc ## x ##, 456(%edx, %eax, 2); opc ## x ##, 456(%edx, %eax, 4); opc ## x ##, 456(%edx, %eax, 8); opc ## x ##, 456(, %eax, 1); opc ## x ##, 456(, %eax, 2); opc ## x ##, 456(, %eax, 4); opc ## x ##, 456(, %eax, 8)
#define INSN3ixm(opc, i, x) opc ## i ##, x ##, 456; opc ## i ##, x ##, 456(%edx); opc ## i ##, x ##, 456(%edx, %eax, 1); opc ## i ##, x ##, 456(%edx, %eax, 2); opc ## i ##, x ##, 456(%edx, %eax, 4); opc ## i ##, x ##, 456(%edx, %eax, 8); opc ## i ##, x ##, 456(, %eax, 1); opc ## i ##, x ##, 456(, %eax, 2); opc ## i ##, x ##, 456(, %eax, 4); opc ## i ##, x ##, 456(, %eax, 8)
//#define INSN2rm(opc) INSN2xm(opc, l %ebx); INSN2xm(lock opc, l %ebx); INSN2xm(opc, w %bx); INSN2xm(lock opc, w %bx); INSN2xm(opc, b %bl); INSN2xm(lock opc, b %bl); INSN2xm(opc, b %bh); INSN2xm(lock opc, b %bh)
#define INSN2rm(opc) INSN2xm(opc, l %ebx); INSN2xm(opc, w %bx); INSN2xm(opc, b %bl); INSN2xm(opc, b %bh)
#define INSN2cm(opc) INSN2xm(opc, l %cl); INSN2xm(opc, w %cl); INSN2xm(opc, b %cl)
#define INSN2cr(opc) opc ##l %cl, ## %ebx; opc ##w %cl, ## %bx; opc ## b %cl, ## %bl; opc ## b %cl, ## %bh
#define INSN2im(opc) INSN2xm(opc, l $123); INSN2xm(opc, w $123); INSN2xm(opc, b $123)
#define INSN2mx_suf(opc, x, suf) opc ## suf ## 56, ## x; opc ## suf ## 56(%ecx), ## x; opc ## suf ## 56(%ecx, %edx, 1), ## x; opc ## suf ## 56(%ecx, %edx, 2), ## x; opc ## suf ## 56(%ecx, %edx, 4), ## x; opc ## suf ## 56(%ecx, %edx, 8), ## x; opc ## suf ## 56(, %edx, 1), ## x; opc ## suf ## 56(, %edx, 2), ## x; opc ## suf ## 56(, %edx, 4), ## x; opc ## suf ## 56(, %edx, 8), ## x
#define INSN3imx(opc, i, x) opc i, 456, ## x; opc i, 456(%ecx), ## x; opc i, 456(%ecx, %edx, 1), ## x; opc i, 456(%ecx, %edx, 2), ## x; opc i, 456(%ecx, %edx, 4), ## x; opc i, 456(%ecx, %edx, 8), ## x; opc i, 456(, %edx, 1), ## x; opc i, 456(, %edx, 2), ## x; opc i, 456(, %edx, 4), ## x; opc i, 456(, %edx, 8), ## x
#define INSN3yxm_suf(opc, x, suf) opc ## suf, x, 456; opc ## suf, x, 456(%eax); opc ## suf, x, 456(%eax, %edx, 1); opc ## suf, x, 456(%eax, %edx, 2); opc ## suf, x, 456(%eax, %edx, 4); opc ## suf, x, 456(%eax, %edx, 8); opc ## suf, x, 456(, %edx, 1); opc ## suf, x, 456(, %edx, 2); opc ## suf, x, 456(, %edx, 4); opc ## suf, x, 456(, %edx, 8)
#define INSN2mr(opc) INSN2mx_suf(opc, %ebx, l 1); INSN2mx_suf(opc, %bx, w 1); INSN2mx_suf(opc, %bl, b 1); INSN2mx_suf(opc, %bh, b 1)
#define INSN1m_suf(opc, suf) opc ## suf ## 23; opc ## suf ## 23(%ecx); opc ## suf ## 23(%ecx, %ebx, 1); opc ## suf ## 23(%ecx, %ebx, 2); opc ## suf ## 23(%ecx, %ebx, 4); opc ## suf ## 23(%ecx, %ebx, 8); opc ## suf ## 23(, %ebx, 1); opc ## suf ## 23(, %ebx, 2); opc ## suf ## 23(, %ebx, 4); opc ## suf ## 23(, %ebx, 8)
#define INSN1m(opc) INSN1m_suf(opc, l 1); INSN1m_suf(opc, w 1); INSN1m_suf(opc, b 1)
#define INSN1(opc)  INSN1r(opc); INSN1m(opc)
#define INSN2i(opc) INSN2ir(opc); INSN2im(opc)
#define INSN2c(opc) INSN2cr(opc); INSN2cm(opc)
#define INSN2(opc)  INSN2rr(opc); INSN2rm(opc); INSN2mr(opc); INSN2i(opc)
#define CFINSN_direct(opc) jne 1f; opc _start; 1:
#define CFINSN_it(opc, target) jne 1f; opc target; 1:
#define CFINSN_indirect(opc) CFINSN_it(opc, *%ebx); CFINSN_it(opc, *123); CFINSN_it(opc, *123(%ebx)); CFINSN_it(opc, *123(%ebx, %esi, 1)); CFINSN_it(opc, *123(%ebx, %esi, 2)); CFINSN_it(opc, *123(%ebx, %esi, 4)); CFINSN_it(opc, *123(%ebx, %esi, 8)); CFINSN_it(opc, *123(, %ebx, 1)); CFINSN_it(opc, *123(, %ebx, 2)); CFINSN_it(opc, *123(, %ebx, 4)); CFINSN_it(opc, *123(, %ebx, 8))
#define STRING_INSN(opc) opc ## b; opc ## w; opc ## l; rep opc ## b; rep opc ## w; rep opc ## l; repz opc ## b; repz opc ## w; repz opc ## l; repnz opc ## b; repnz opc ## w; repnz opc ## l
#define CMPSS_CMPSD_INSN(opc) opc $0,%xmm0,%xmm1; opc $1,%xmm0,%xmm1; opc $2,%xmm0,%xmm1; opc $3,%xmm0,%xmm1; opc $4,%xmm0,%xmm1; opc $5,%xmm0,%xmm1; opc $6,%xmm0,%xmm1; opc $7,%xmm0,%xmm1; INSN3imx(opc, $0, %xmm0); INSN3imx(opc, $1, %xmm0); INSN3imx(opc, $2, %xmm0); INSN3imx(opc, $3, %xmm0); INSN3imx(opc, $4, %xmm0); INSN3imx(opc, $5, %xmm0); INSN3imx(opc, $6, %xmm0); INSN3imx(opc, $7, %xmm0)
#define FLAG_INSN(flag) CFINSN_direct(j ## flag); INSN2rx32(cmov ## flag); INSN2rx16(cmov ## flag); set ## flag %bl; set ## flag %bh; INSN1m_suf(set ## flag, b 1)
//#define BT_INSN(opc) INSN2xm(opc, l %ebx); INSN2xm(opc, w %bx); INSN2xm(opc, l $12); INSN2xm(opc, w $12); opc %ecx,%edx; opc $12,%edx
#define BT_INSN(opc) INSN2xm(opc, l $12); INSN2xm(opc, w $12); opc %ecx,%edx; opc %cx,%dx; opc $12,%edx; opc $12,%dx
#define BS_INSN(opc) INSN2mx_suf(opc, %ebx, l 1); INSN2mx_suf(opc, %bx, w 1); opc ##l %ecx, %edx; opc ##w %cx, %dx
//#define BS_INSN(opc) opc ##l %ecx, %edx; opc ##w %cx, %dx
#define SHXD_INSN(opc) INSN3yxm_suf(opc, %ebx, l $123); INSN3yxm_suf(opc, %bx, w $123); INSN3yxm_suf(opc, %ebx, l %cl); INSN3yxm_suf(opc, %bx, w %cl); opc $12, %ebx, %edx; opc $12, %bx, %dx; opc %cl, %ebx, %edx; opc %cl, %bx, %dx
#define INSN1st(opc) opc %st(0); opc %st(1); opc %st(2); opc %st(3); opc %st(4); opc %st(5); opc %st(6); opc %st(7)
#define INSN2st_to_zero(opc) opc %st(1),%st(0); opc %st(2),%st(0); opc %st(3),%st(0); opc %st(4),%st(0); opc %st(5),%st(0); opc %st(6),%st(0); opc %st(7),%st(0)
#define BMI3m0(opc) opc %eax,%ecx,%ebx; opc 456,%eax,%ecx; opc 456(%ebx),%eax,%ecx; opc 456(%esi,%ebx,1),%eax,%ecx; opc 456(%esi,%ebx,2),%eax,%ecx; opc 456(%esi,%ebx,4),%eax,%ecx; opc 456(%esi,%ebx,8),%eax,%ecx; opc 456(,%ebx,1),%eax,%ecx; opc 456(,%ebx,2),%eax,%ecx; opc 456(,%ebx,4),%eax,%ecx; opc 456(,%ebx,8),%eax,%ecx
#define BMI3m1(opc) opc %eax,%ecx,%edx; opc %eax,456,%ecx; opc %eax,456(%edx),%ecx; opc %eax,456(%edx,%ebx,1),%ecx; opc %eax,456(%edx,%ebx,2),%ecx; opc %eax,456(%edx,%ebx,4),%ecx; opc %eax,456(%edx,%ebx,8),%ecx; opc %eax,456(,%ebx,1),%ecx; opc %eax,456(,%ebx,2),%ecx; opc %eax,456(,%ebx,4),%ecx; opc %eax,456(,%ebx,8),%ecx
#define BMI2(opc,suf) opc ##suf %ecx, %edx; INSN2mx_suf(opc, %ebx, suf 1)
#define BMI2w(opc,suf) opc ##suf %cx, %dx; INSN2mx_suf(opc, %bx, suf 1)
#define BMI3i0m1(opc) opc $13,%ecx,%edx; opc $13,456,%ecx; opc $13,456(%edx),%ecx; opc $13,456(%edx,%ebx,1),%ecx; opc $13,456(%edx,%ebx,2),%ecx; opc $13,456(%edx,%ebx,4),%ecx; opc $13,456(%edx,%ebx,8),%ecx; opc $13,456(,%ebx,1),%ecx; opc $13,456(,%ebx,2),%ecx; opc $13,456(,%ebx,4),%ecx; opc $13,456(,%ebx,8),%ecx
#define PSHUFINSN_REG(opc,reg0,reg1) \
  opc $0,reg0,reg1; \
  opc $1,reg0,reg1; \
  opc $2,reg0,reg1; \
  opc $3,reg0,reg1; \
  opc $4,reg0,reg1; \
  opc $5,reg0,reg1; \
  opc $6,reg0,reg1; \
  opc $7,reg0,reg1; \
  opc $8,reg0,reg1; \
  opc $9,reg0,reg1; \
  opc $10,reg0,reg1; \
  opc $11,reg0,reg1; \
  opc $12,reg0,reg1; \
  opc $13,reg0,reg1; \
  opc $14,reg0,reg1; \
  opc $15,reg0,reg1; \
  opc $16,reg0,reg1; \
  opc $17,reg0,reg1; \
  opc $18,reg0,reg1; \
  opc $19,reg0,reg1; \
  opc $20,reg0,reg1; \
  opc $21,reg0,reg1; \
  opc $22,reg0,reg1; \
  opc $23,reg0,reg1; \
  opc $24,reg0,reg1; \
  opc $25,reg0,reg1; \
  opc $26,reg0,reg1; \
  opc $27,reg0,reg1; \
  opc $28,reg0,reg1; \
  opc $29,reg0,reg1; \
  opc $30,reg0,reg1; \
  opc $31,reg0,reg1; \
  opc $32,reg0,reg1; \
  opc $33,reg0,reg1; \
  opc $34,reg0,reg1; \
  opc $35,reg0,reg1; \
  opc $36,reg0,reg1; \
  opc $37,reg0,reg1; \
  opc $38,reg0,reg1; \
  opc $39,reg0,reg1; \
  opc $40,reg0,reg1; \
  opc $41,reg0,reg1; \
  opc $42,reg0,reg1; \
  opc $43,reg0,reg1; \
  opc $44,reg0,reg1; \
  opc $45,reg0,reg1; \
  opc $46,reg0,reg1; \
  opc $47,reg0,reg1; \
  opc $48,reg0,reg1; \
  opc $49,reg0,reg1; \
  opc $50,reg0,reg1; \
  opc $51,reg0,reg1; \
  opc $52,reg0,reg1; \
  opc $53,reg0,reg1; \
  opc $54,reg0,reg1; \
  opc $55,reg0,reg1; \
  opc $56,reg0,reg1; \
  opc $57,reg0,reg1; \
  opc $58,reg0,reg1; \
  opc $59,reg0,reg1; \
  opc $60,reg0,reg1; \
  opc $61,reg0,reg1; \
  opc $62,reg0,reg1; \
  opc $63,reg0,reg1; \
  opc $64,reg0,reg1; \
  opc $65,reg0,reg1; \
  opc $66,reg0,reg1; \
  opc $67,reg0,reg1; \
  opc $68,reg0,reg1; \
  opc $69,reg0,reg1; \
  opc $70,reg0,reg1; \
  opc $71,reg0,reg1; \
  opc $72,reg0,reg1; \
  opc $73,reg0,reg1; \
  opc $74,reg0,reg1; \
  opc $75,reg0,reg1; \
  opc $76,reg0,reg1; \
  opc $77,reg0,reg1; \
  opc $78,reg0,reg1; \
  opc $79,reg0,reg1; \
  opc $80,reg0,reg1; \
  opc $81,reg0,reg1; \
  opc $82,reg0,reg1; \
  opc $83,reg0,reg1; \
  opc $84,reg0,reg1; \
  opc $85,reg0,reg1; \
  opc $86,reg0,reg1; \
  opc $87,reg0,reg1; \
  opc $88,reg0,reg1; \
  opc $89,reg0,reg1; \
  opc $90,reg0,reg1; \
  opc $91,reg0,reg1; \
  opc $92,reg0,reg1; \
  opc $93,reg0,reg1; \
  opc $94,reg0,reg1; \
  opc $95,reg0,reg1; \
  opc $96,reg0,reg1; \
  opc $97,reg0,reg1; \
  opc $98,reg0,reg1; \
  opc $99,reg0,reg1; \
  opc $100,reg0,reg1; \
  opc $101,reg0,reg1; \
  opc $102,reg0,reg1; \
  opc $103,reg0,reg1; \
  opc $104,reg0,reg1; \
  opc $105,reg0,reg1; \
  opc $106,reg0,reg1; \
  opc $107,reg0,reg1; \
  opc $108,reg0,reg1; \
  opc $109,reg0,reg1; \
  opc $110,reg0,reg1; \
  opc $111,reg0,reg1; \
  opc $112,reg0,reg1; \
  opc $113,reg0,reg1; \
  opc $114,reg0,reg1; \
  opc $115,reg0,reg1; \
  opc $116,reg0,reg1; \
  opc $117,reg0,reg1; \
  opc $118,reg0,reg1; \
  opc $119,reg0,reg1; \
  opc $120,reg0,reg1; \
  opc $121,reg0,reg1; \
  opc $122,reg0,reg1; \
  opc $123,reg0,reg1; \
  opc $124,reg0,reg1; \
  opc $125,reg0,reg1; \
  opc $126,reg0,reg1; \
  opc $127,reg0,reg1; \
  opc $128,reg0,reg1; \
  opc $129,reg0,reg1; \
  opc $130,reg0,reg1; \
  opc $131,reg0,reg1; \
  opc $132,reg0,reg1; \
  opc $133,reg0,reg1; \
  opc $134,reg0,reg1; \
  opc $135,reg0,reg1; \
  opc $136,reg0,reg1; \
  opc $137,reg0,reg1; \
  opc $138,reg0,reg1; \
  opc $139,reg0,reg1; \
  opc $140,reg0,reg1; \
  opc $141,reg0,reg1; \
  opc $142,reg0,reg1; \
  opc $143,reg0,reg1; \
  opc $144,reg0,reg1; \
  opc $145,reg0,reg1; \
  opc $146,reg0,reg1; \
  opc $147,reg0,reg1; \
  opc $148,reg0,reg1; \
  opc $149,reg0,reg1; \
  opc $150,reg0,reg1; \
  opc $151,reg0,reg1; \
  opc $152,reg0,reg1; \
  opc $153,reg0,reg1; \
  opc $154,reg0,reg1; \
  opc $155,reg0,reg1; \
  opc $156,reg0,reg1; \
  opc $157,reg0,reg1; \
  opc $158,reg0,reg1; \
  opc $159,reg0,reg1; \
  opc $160,reg0,reg1; \
  opc $161,reg0,reg1; \
  opc $162,reg0,reg1; \
  opc $163,reg0,reg1; \
  opc $164,reg0,reg1; \
  opc $165,reg0,reg1; \
  opc $166,reg0,reg1; \
  opc $167,reg0,reg1; \
  opc $168,reg0,reg1; \
  opc $169,reg0,reg1; \
  opc $170,reg0,reg1; \
  opc $171,reg0,reg1; \
  opc $172,reg0,reg1; \
  opc $173,reg0,reg1; \
  opc $174,reg0,reg1; \
  opc $175,reg0,reg1; \
  opc $176,reg0,reg1; \
  opc $177,reg0,reg1; \
  opc $178,reg0,reg1; \
  opc $179,reg0,reg1; \
  opc $180,reg0,reg1; \
  opc $181,reg0,reg1; \
  opc $182,reg0,reg1; \
  opc $183,reg0,reg1; \
  opc $184,reg0,reg1; \
  opc $185,reg0,reg1; \
  opc $186,reg0,reg1; \
  opc $187,reg0,reg1; \
  opc $188,reg0,reg1; \
  opc $189,reg0,reg1; \
  opc $190,reg0,reg1; \
  opc $191,reg0,reg1; \
  opc $192,reg0,reg1; \
  opc $193,reg0,reg1; \
  opc $194,reg0,reg1; \
  opc $195,reg0,reg1; \
  opc $196,reg0,reg1; \
  opc $197,reg0,reg1; \
  opc $198,reg0,reg1; \
  opc $199,reg0,reg1; \
  opc $200,reg0,reg1; \
  opc $201,reg0,reg1; \
  opc $202,reg0,reg1; \
  opc $203,reg0,reg1; \
  opc $204,reg0,reg1; \
  opc $205,reg0,reg1; \
  opc $206,reg0,reg1; \
  opc $207,reg0,reg1; \
  opc $208,reg0,reg1; \
  opc $209,reg0,reg1; \
  opc $210,reg0,reg1; \
  opc $211,reg0,reg1; \
  opc $212,reg0,reg1; \
  opc $213,reg0,reg1; \
  opc $214,reg0,reg1; \
  opc $215,reg0,reg1; \
  opc $216,reg0,reg1; \
  opc $217,reg0,reg1; \
  opc $218,reg0,reg1; \
  opc $219,reg0,reg1; \
  opc $220,reg0,reg1; \
  opc $221,reg0,reg1; \
  opc $222,reg0,reg1; \
  opc $223,reg0,reg1; \
  opc $224,reg0,reg1; \
  opc $225,reg0,reg1; \
  opc $226,reg0,reg1; \
  opc $227,reg0,reg1; \
  opc $228,reg0,reg1; \
  opc $229,reg0,reg1; \
  opc $230,reg0,reg1; \
  opc $231,reg0,reg1; \
  opc $232,reg0,reg1; \
  opc $233,reg0,reg1; \
  opc $234,reg0,reg1; \
  opc $235,reg0,reg1; \
  opc $236,reg0,reg1; \
  opc $237,reg0,reg1; \
  opc $238,reg0,reg1; \
  opc $239,reg0,reg1; \
  opc $240,reg0,reg1; \
  opc $241,reg0,reg1; \
  opc $242,reg0,reg1; \
  opc $243,reg0,reg1; \
  opc $244,reg0,reg1; \
  opc $245,reg0,reg1; \
  opc $246,reg0,reg1; \
  opc $247,reg0,reg1; \
  opc $248,reg0,reg1; \
  opc $249,reg0,reg1; \
  opc $250,reg0,reg1; \
  opc $251,reg0,reg1; \
  opc $252,reg0,reg1; \
  opc $253,reg0,reg1; \
  opc $254,reg0,reg1; \
  opc $255,reg0,reg1

#define PSHUFINSN_MEM(opc,reg0) \
  INSN3imx(opc, $0, reg0); \
  INSN3imx(opc, $1, reg0); \
  INSN3imx(opc, $2, reg0); \
  INSN3imx(opc, $3, reg0); \
  INSN3imx(opc, $4, reg0); \
  INSN3imx(opc, $5, reg0); \
  INSN3imx(opc, $6, reg0); \
  INSN3imx(opc, $7, reg0); \
  INSN3imx(opc, $8, reg0); \
  INSN3imx(opc, $9, reg0); \
  INSN3imx(opc, $10, reg0); \
  INSN3imx(opc, $11, reg0); \
  INSN3imx(opc, $12, reg0); \
  INSN3imx(opc, $13, reg0); \
  INSN3imx(opc, $14, reg0); \
  INSN3imx(opc, $15, reg0); \
  INSN3imx(opc, $16, reg0); \
  INSN3imx(opc, $17, reg0); \
  INSN3imx(opc, $18, reg0); \
  INSN3imx(opc, $19, reg0); \
  INSN3imx(opc, $20, reg0); \
  INSN3imx(opc, $21, reg0); \
  INSN3imx(opc, $22, reg0); \
  INSN3imx(opc, $23, reg0); \
  INSN3imx(opc, $24, reg0); \
  INSN3imx(opc, $25, reg0); \
  INSN3imx(opc, $26, reg0); \
  INSN3imx(opc, $27, reg0); \
  INSN3imx(opc, $28, reg0); \
  INSN3imx(opc, $29, reg0); \
  INSN3imx(opc, $30, reg0); \
  INSN3imx(opc, $31, reg0); \
  INSN3imx(opc, $32, reg0); \
  INSN3imx(opc, $33, reg0); \
  INSN3imx(opc, $34, reg0); \
  INSN3imx(opc, $35, reg0); \
  INSN3imx(opc, $36, reg0); \
  INSN3imx(opc, $37, reg0); \
  INSN3imx(opc, $38, reg0); \
  INSN3imx(opc, $39, reg0); \
  INSN3imx(opc, $40, reg0); \
  INSN3imx(opc, $41, reg0); \
  INSN3imx(opc, $42, reg0); \
  INSN3imx(opc, $43, reg0); \
  INSN3imx(opc, $44, reg0); \
  INSN3imx(opc, $45, reg0); \
  INSN3imx(opc, $46, reg0); \
  INSN3imx(opc, $47, reg0); \
  INSN3imx(opc, $48, reg0); \
  INSN3imx(opc, $49, reg0); \
  INSN3imx(opc, $50, reg0); \
  INSN3imx(opc, $51, reg0); \
  INSN3imx(opc, $52, reg0); \
  INSN3imx(opc, $53, reg0); \
  INSN3imx(opc, $54, reg0); \
  INSN3imx(opc, $55, reg0); \
  INSN3imx(opc, $56, reg0); \
  INSN3imx(opc, $57, reg0); \
  INSN3imx(opc, $58, reg0); \
  INSN3imx(opc, $59, reg0); \
  INSN3imx(opc, $60, reg0); \
  INSN3imx(opc, $61, reg0); \
  INSN3imx(opc, $62, reg0); \
  INSN3imx(opc, $63, reg0); \
  INSN3imx(opc, $64, reg0); \
  INSN3imx(opc, $65, reg0); \
  INSN3imx(opc, $66, reg0); \
  INSN3imx(opc, $67, reg0); \
  INSN3imx(opc, $68, reg0); \
  INSN3imx(opc, $69, reg0); \
  INSN3imx(opc, $70, reg0); \
  INSN3imx(opc, $71, reg0); \
  INSN3imx(opc, $72, reg0); \
  INSN3imx(opc, $73, reg0); \
  INSN3imx(opc, $74, reg0); \
  INSN3imx(opc, $75, reg0); \
  INSN3imx(opc, $76, reg0); \
  INSN3imx(opc, $77, reg0); \
  INSN3imx(opc, $78, reg0); \
  INSN3imx(opc, $79, reg0); \
  INSN3imx(opc, $80, reg0); \
  INSN3imx(opc, $81, reg0); \
  INSN3imx(opc, $82, reg0); \
  INSN3imx(opc, $83, reg0); \
  INSN3imx(opc, $84, reg0); \
  INSN3imx(opc, $85, reg0); \
  INSN3imx(opc, $86, reg0); \
  INSN3imx(opc, $87, reg0); \
  INSN3imx(opc, $88, reg0); \
  INSN3imx(opc, $89, reg0); \
  INSN3imx(opc, $90, reg0); \
  INSN3imx(opc, $91, reg0); \
  INSN3imx(opc, $92, reg0); \
  INSN3imx(opc, $93, reg0); \
  INSN3imx(opc, $94, reg0); \
  INSN3imx(opc, $95, reg0); \
  INSN3imx(opc, $96, reg0); \
  INSN3imx(opc, $97, reg0); \
  INSN3imx(opc, $98, reg0); \
  INSN3imx(opc, $99, reg0); \
  INSN3imx(opc, $100, reg0); \
  INSN3imx(opc, $101, reg0); \
  INSN3imx(opc, $102, reg0); \
  INSN3imx(opc, $103, reg0); \
  INSN3imx(opc, $104, reg0); \
  INSN3imx(opc, $105, reg0); \
  INSN3imx(opc, $106, reg0); \
  INSN3imx(opc, $107, reg0); \
  INSN3imx(opc, $108, reg0); \
  INSN3imx(opc, $109, reg0); \
  INSN3imx(opc, $110, reg0); \
  INSN3imx(opc, $111, reg0); \
  INSN3imx(opc, $112, reg0); \
  INSN3imx(opc, $113, reg0); \
  INSN3imx(opc, $114, reg0); \
  INSN3imx(opc, $115, reg0); \
  INSN3imx(opc, $116, reg0); \
  INSN3imx(opc, $117, reg0); \
  INSN3imx(opc, $118, reg0); \
  INSN3imx(opc, $119, reg0); \
  INSN3imx(opc, $120, reg0); \
  INSN3imx(opc, $121, reg0); \
  INSN3imx(opc, $122, reg0); \
  INSN3imx(opc, $123, reg0); \
  INSN3imx(opc, $124, reg0); \
  INSN3imx(opc, $125, reg0); \
  INSN3imx(opc, $126, reg0); \
  INSN3imx(opc, $127, reg0); \
  INSN3imx(opc, $128, reg0); \
  INSN3imx(opc, $129, reg0); \
  INSN3imx(opc, $130, reg0); \
  INSN3imx(opc, $131, reg0); \
  INSN3imx(opc, $132, reg0); \
  INSN3imx(opc, $133, reg0); \
  INSN3imx(opc, $134, reg0); \
  INSN3imx(opc, $135, reg0); \
  INSN3imx(opc, $136, reg0); \
  INSN3imx(opc, $137, reg0); \
  INSN3imx(opc, $138, reg0); \
  INSN3imx(opc, $139, reg0); \
  INSN3imx(opc, $140, reg0); \
  INSN3imx(opc, $141, reg0); \
  INSN3imx(opc, $142, reg0); \
  INSN3imx(opc, $143, reg0); \
  INSN3imx(opc, $144, reg0); \
  INSN3imx(opc, $145, reg0); \
  INSN3imx(opc, $146, reg0); \
  INSN3imx(opc, $147, reg0); \
  INSN3imx(opc, $148, reg0); \
  INSN3imx(opc, $149, reg0); \
  INSN3imx(opc, $150, reg0); \
  INSN3imx(opc, $151, reg0); \
  INSN3imx(opc, $152, reg0); \
  INSN3imx(opc, $153, reg0); \
  INSN3imx(opc, $154, reg0); \
  INSN3imx(opc, $155, reg0); \
  INSN3imx(opc, $156, reg0); \
  INSN3imx(opc, $157, reg0); \
  INSN3imx(opc, $158, reg0); \
  INSN3imx(opc, $159, reg0); \
  INSN3imx(opc, $160, reg0); \
  INSN3imx(opc, $161, reg0); \
  INSN3imx(opc, $162, reg0); \
  INSN3imx(opc, $163, reg0); \
  INSN3imx(opc, $164, reg0); \
  INSN3imx(opc, $165, reg0); \
  INSN3imx(opc, $166, reg0); \
  INSN3imx(opc, $167, reg0); \
  INSN3imx(opc, $168, reg0); \
  INSN3imx(opc, $169, reg0); \
  INSN3imx(opc, $170, reg0); \
  INSN3imx(opc, $171, reg0); \
  INSN3imx(opc, $172, reg0); \
  INSN3imx(opc, $173, reg0); \
  INSN3imx(opc, $174, reg0); \
  INSN3imx(opc, $175, reg0); \
  INSN3imx(opc, $176, reg0); \
  INSN3imx(opc, $177, reg0); \
  INSN3imx(opc, $178, reg0); \
  INSN3imx(opc, $179, reg0); \
  INSN3imx(opc, $180, reg0); \
  INSN3imx(opc, $181, reg0); \
  INSN3imx(opc, $182, reg0); \
  INSN3imx(opc, $183, reg0); \
  INSN3imx(opc, $184, reg0); \
  INSN3imx(opc, $185, reg0); \
  INSN3imx(opc, $186, reg0); \
  INSN3imx(opc, $187, reg0); \
  INSN3imx(opc, $188, reg0); \
  INSN3imx(opc, $189, reg0); \
  INSN3imx(opc, $190, reg0); \
  INSN3imx(opc, $191, reg0); \
  INSN3imx(opc, $192, reg0); \
  INSN3imx(opc, $193, reg0); \
  INSN3imx(opc, $194, reg0); \
  INSN3imx(opc, $195, reg0); \
  INSN3imx(opc, $196, reg0); \
  INSN3imx(opc, $197, reg0); \
  INSN3imx(opc, $198, reg0); \
  INSN3imx(opc, $199, reg0); \
  INSN3imx(opc, $200, reg0); \
  INSN3imx(opc, $201, reg0); \
  INSN3imx(opc, $202, reg0); \
  INSN3imx(opc, $203, reg0); \
  INSN3imx(opc, $204, reg0); \
  INSN3imx(opc, $205, reg0); \
  INSN3imx(opc, $206, reg0); \
  INSN3imx(opc, $207, reg0); \
  INSN3imx(opc, $208, reg0); \
  INSN3imx(opc, $209, reg0); \
  INSN3imx(opc, $210, reg0); \
  INSN3imx(opc, $211, reg0); \
  INSN3imx(opc, $212, reg0); \
  INSN3imx(opc, $213, reg0); \
  INSN3imx(opc, $214, reg0); \
  INSN3imx(opc, $215, reg0); \
  INSN3imx(opc, $216, reg0); \
  INSN3imx(opc, $217, reg0); \
  INSN3imx(opc, $218, reg0); \
  INSN3imx(opc, $219, reg0); \
  INSN3imx(opc, $220, reg0); \
  INSN3imx(opc, $221, reg0); \
  INSN3imx(opc, $222, reg0); \
  INSN3imx(opc, $223, reg0); \
  INSN3imx(opc, $224, reg0); \
  INSN3imx(opc, $225, reg0); \
  INSN3imx(opc, $226, reg0); \
  INSN3imx(opc, $227, reg0); \
  INSN3imx(opc, $228, reg0); \
  INSN3imx(opc, $229, reg0); \
  INSN3imx(opc, $230, reg0); \
  INSN3imx(opc, $231, reg0); \
  INSN3imx(opc, $232, reg0); \
  INSN3imx(opc, $233, reg0); \
  INSN3imx(opc, $234, reg0); \
  INSN3imx(opc, $235, reg0); \
  INSN3imx(opc, $236, reg0); \
  INSN3imx(opc, $237, reg0); \
  INSN3imx(opc, $238, reg0); \
  INSN3imx(opc, $239, reg0); \
  INSN3imx(opc, $240, reg0); \
  INSN3imx(opc, $241, reg0); \
  INSN3imx(opc, $242, reg0); \
  INSN3imx(opc, $243, reg0); \
  INSN3imx(opc, $244, reg0); \
  INSN3imx(opc, $245, reg0); \
  INSN3imx(opc, $246, reg0); \
  INSN3imx(opc, $247, reg0); \
  INSN3imx(opc, $248, reg0); \
  INSN3imx(opc, $249, reg0); \
  INSN3imx(opc, $250, reg0); \
  INSN3imx(opc, $251, reg0); \
  INSN3imx(opc, $252, reg0); \
  INSN3imx(opc, $253, reg0); \
  INSN3imx(opc, $254, reg0); \
  INSN3imx(opc, $255, reg0)

#define PSHUFINSN_xMEM(opc,reg0) \
  INSN3ixm(opc, $0, reg0); \
  INSN3ixm(opc, $1, reg0); \
  INSN3ixm(opc, $2, reg0); \
  INSN3ixm(opc, $3, reg0); \
  INSN3ixm(opc, $4, reg0); \
  INSN3ixm(opc, $5, reg0); \
  INSN3ixm(opc, $6, reg0); \
  INSN3ixm(opc, $7, reg0); \
  INSN3ixm(opc, $8, reg0); \
  INSN3ixm(opc, $9, reg0); \
  INSN3ixm(opc, $10, reg0); \
  INSN3ixm(opc, $11, reg0); \
  INSN3ixm(opc, $12, reg0); \
  INSN3ixm(opc, $13, reg0); \
  INSN3ixm(opc, $14, reg0); \
  INSN3ixm(opc, $15, reg0); \
  INSN3ixm(opc, $16, reg0); \
  INSN3ixm(opc, $17, reg0); \
  INSN3ixm(opc, $18, reg0); \
  INSN3ixm(opc, $19, reg0); \
  INSN3ixm(opc, $20, reg0); \
  INSN3ixm(opc, $21, reg0); \
  INSN3ixm(opc, $22, reg0); \
  INSN3ixm(opc, $23, reg0); \
  INSN3ixm(opc, $24, reg0); \
  INSN3ixm(opc, $25, reg0); \
  INSN3ixm(opc, $26, reg0); \
  INSN3ixm(opc, $27, reg0); \
  INSN3ixm(opc, $28, reg0); \
  INSN3ixm(opc, $29, reg0); \
  INSN3ixm(opc, $30, reg0); \
  INSN3ixm(opc, $31, reg0); \
  INSN3ixm(opc, $32, reg0); \
  INSN3ixm(opc, $33, reg0); \
  INSN3ixm(opc, $34, reg0); \
  INSN3ixm(opc, $35, reg0); \
  INSN3ixm(opc, $36, reg0); \
  INSN3ixm(opc, $37, reg0); \
  INSN3ixm(opc, $38, reg0); \
  INSN3ixm(opc, $39, reg0); \
  INSN3ixm(opc, $40, reg0); \
  INSN3ixm(opc, $41, reg0); \
  INSN3ixm(opc, $42, reg0); \
  INSN3ixm(opc, $43, reg0); \
  INSN3ixm(opc, $44, reg0); \
  INSN3ixm(opc, $45, reg0); \
  INSN3ixm(opc, $46, reg0); \
  INSN3ixm(opc, $47, reg0); \
  INSN3ixm(opc, $48, reg0); \
  INSN3ixm(opc, $49, reg0); \
  INSN3ixm(opc, $50, reg0); \
  INSN3ixm(opc, $51, reg0); \
  INSN3ixm(opc, $52, reg0); \
  INSN3ixm(opc, $53, reg0); \
  INSN3ixm(opc, $54, reg0); \
  INSN3ixm(opc, $55, reg0); \
  INSN3ixm(opc, $56, reg0); \
  INSN3ixm(opc, $57, reg0); \
  INSN3ixm(opc, $58, reg0); \
  INSN3ixm(opc, $59, reg0); \
  INSN3ixm(opc, $60, reg0); \
  INSN3ixm(opc, $61, reg0); \
  INSN3ixm(opc, $62, reg0); \
  INSN3ixm(opc, $63, reg0); \
  INSN3ixm(opc, $64, reg0); \
  INSN3ixm(opc, $65, reg0); \
  INSN3ixm(opc, $66, reg0); \
  INSN3ixm(opc, $67, reg0); \
  INSN3ixm(opc, $68, reg0); \
  INSN3ixm(opc, $69, reg0); \
  INSN3ixm(opc, $70, reg0); \
  INSN3ixm(opc, $71, reg0); \
  INSN3ixm(opc, $72, reg0); \
  INSN3ixm(opc, $73, reg0); \
  INSN3ixm(opc, $74, reg0); \
  INSN3ixm(opc, $75, reg0); \
  INSN3ixm(opc, $76, reg0); \
  INSN3ixm(opc, $77, reg0); \
  INSN3ixm(opc, $78, reg0); \
  INSN3ixm(opc, $79, reg0); \
  INSN3ixm(opc, $80, reg0); \
  INSN3ixm(opc, $81, reg0); \
  INSN3ixm(opc, $82, reg0); \
  INSN3ixm(opc, $83, reg0); \
  INSN3ixm(opc, $84, reg0); \
  INSN3ixm(opc, $85, reg0); \
  INSN3ixm(opc, $86, reg0); \
  INSN3ixm(opc, $87, reg0); \
  INSN3ixm(opc, $88, reg0); \
  INSN3ixm(opc, $89, reg0); \
  INSN3ixm(opc, $90, reg0); \
  INSN3ixm(opc, $91, reg0); \
  INSN3ixm(opc, $92, reg0); \
  INSN3ixm(opc, $93, reg0); \
  INSN3ixm(opc, $94, reg0); \
  INSN3ixm(opc, $95, reg0); \
  INSN3ixm(opc, $96, reg0); \
  INSN3ixm(opc, $97, reg0); \
  INSN3ixm(opc, $98, reg0); \
  INSN3ixm(opc, $99, reg0); \
  INSN3ixm(opc, $100, reg0); \
  INSN3ixm(opc, $101, reg0); \
  INSN3ixm(opc, $102, reg0); \
  INSN3ixm(opc, $103, reg0); \
  INSN3ixm(opc, $104, reg0); \
  INSN3ixm(opc, $105, reg0); \
  INSN3ixm(opc, $106, reg0); \
  INSN3ixm(opc, $107, reg0); \
  INSN3ixm(opc, $108, reg0); \
  INSN3ixm(opc, $109, reg0); \
  INSN3ixm(opc, $110, reg0); \
  INSN3ixm(opc, $111, reg0); \
  INSN3ixm(opc, $112, reg0); \
  INSN3ixm(opc, $113, reg0); \
  INSN3ixm(opc, $114, reg0); \
  INSN3ixm(opc, $115, reg0); \
  INSN3ixm(opc, $116, reg0); \
  INSN3ixm(opc, $117, reg0); \
  INSN3ixm(opc, $118, reg0); \
  INSN3ixm(opc, $119, reg0); \
  INSN3ixm(opc, $120, reg0); \
  INSN3ixm(opc, $121, reg0); \
  INSN3ixm(opc, $122, reg0); \
  INSN3ixm(opc, $123, reg0); \
  INSN3ixm(opc, $124, reg0); \
  INSN3ixm(opc, $125, reg0); \
  INSN3ixm(opc, $126, reg0); \
  INSN3ixm(opc, $127, reg0); \
  INSN3ixm(opc, $128, reg0); \
  INSN3ixm(opc, $129, reg0); \
  INSN3ixm(opc, $130, reg0); \
  INSN3ixm(opc, $131, reg0); \
  INSN3ixm(opc, $132, reg0); \
  INSN3ixm(opc, $133, reg0); \
  INSN3ixm(opc, $134, reg0); \
  INSN3ixm(opc, $135, reg0); \
  INSN3ixm(opc, $136, reg0); \
  INSN3ixm(opc, $137, reg0); \
  INSN3ixm(opc, $138, reg0); \
  INSN3ixm(opc, $139, reg0); \
  INSN3ixm(opc, $140, reg0); \
  INSN3ixm(opc, $141, reg0); \
  INSN3ixm(opc, $142, reg0); \
  INSN3ixm(opc, $143, reg0); \
  INSN3ixm(opc, $144, reg0); \
  INSN3ixm(opc, $145, reg0); \
  INSN3ixm(opc, $146, reg0); \
  INSN3ixm(opc, $147, reg0); \
  INSN3ixm(opc, $148, reg0); \
  INSN3ixm(opc, $149, reg0); \
  INSN3ixm(opc, $150, reg0); \
  INSN3ixm(opc, $151, reg0); \
  INSN3ixm(opc, $152, reg0); \
  INSN3ixm(opc, $153, reg0); \
  INSN3ixm(opc, $154, reg0); \
  INSN3ixm(opc, $155, reg0); \
  INSN3ixm(opc, $156, reg0); \
  INSN3ixm(opc, $157, reg0); \
  INSN3ixm(opc, $158, reg0); \
  INSN3ixm(opc, $159, reg0); \
  INSN3ixm(opc, $160, reg0); \
  INSN3ixm(opc, $161, reg0); \
  INSN3ixm(opc, $162, reg0); \
  INSN3ixm(opc, $163, reg0); \
  INSN3ixm(opc, $164, reg0); \
  INSN3ixm(opc, $165, reg0); \
  INSN3ixm(opc, $166, reg0); \
  INSN3ixm(opc, $167, reg0); \
  INSN3ixm(opc, $168, reg0); \
  INSN3ixm(opc, $169, reg0); \
  INSN3ixm(opc, $170, reg0); \
  INSN3ixm(opc, $171, reg0); \
  INSN3ixm(opc, $172, reg0); \
  INSN3ixm(opc, $173, reg0); \
  INSN3ixm(opc, $174, reg0); \
  INSN3ixm(opc, $175, reg0); \
  INSN3ixm(opc, $176, reg0); \
  INSN3ixm(opc, $177, reg0); \
  INSN3ixm(opc, $178, reg0); \
  INSN3ixm(opc, $179, reg0); \
  INSN3ixm(opc, $180, reg0); \
  INSN3ixm(opc, $181, reg0); \
  INSN3ixm(opc, $182, reg0); \
  INSN3ixm(opc, $183, reg0); \
  INSN3ixm(opc, $184, reg0); \
  INSN3ixm(opc, $185, reg0); \
  INSN3ixm(opc, $186, reg0); \
  INSN3ixm(opc, $187, reg0); \
  INSN3ixm(opc, $188, reg0); \
  INSN3ixm(opc, $189, reg0); \
  INSN3ixm(opc, $190, reg0); \
  INSN3ixm(opc, $191, reg0); \
  INSN3ixm(opc, $192, reg0); \
  INSN3ixm(opc, $193, reg0); \
  INSN3ixm(opc, $194, reg0); \
  INSN3ixm(opc, $195, reg0); \
  INSN3ixm(opc, $196, reg0); \
  INSN3ixm(opc, $197, reg0); \
  INSN3ixm(opc, $198, reg0); \
  INSN3ixm(opc, $199, reg0); \
  INSN3ixm(opc, $200, reg0); \
  INSN3ixm(opc, $201, reg0); \
  INSN3ixm(opc, $202, reg0); \
  INSN3ixm(opc, $203, reg0); \
  INSN3ixm(opc, $204, reg0); \
  INSN3ixm(opc, $205, reg0); \
  INSN3ixm(opc, $206, reg0); \
  INSN3ixm(opc, $207, reg0); \
  INSN3ixm(opc, $208, reg0); \
  INSN3ixm(opc, $209, reg0); \
  INSN3ixm(opc, $210, reg0); \
  INSN3ixm(opc, $211, reg0); \
  INSN3ixm(opc, $212, reg0); \
  INSN3ixm(opc, $213, reg0); \
  INSN3ixm(opc, $214, reg0); \
  INSN3ixm(opc, $215, reg0); \
  INSN3ixm(opc, $216, reg0); \
  INSN3ixm(opc, $217, reg0); \
  INSN3ixm(opc, $218, reg0); \
  INSN3ixm(opc, $219, reg0); \
  INSN3ixm(opc, $220, reg0); \
  INSN3ixm(opc, $221, reg0); \
  INSN3ixm(opc, $222, reg0); \
  INSN3ixm(opc, $223, reg0); \
  INSN3ixm(opc, $224, reg0); \
  INSN3ixm(opc, $225, reg0); \
  INSN3ixm(opc, $226, reg0); \
  INSN3ixm(opc, $227, reg0); \
  INSN3ixm(opc, $228, reg0); \
  INSN3ixm(opc, $229, reg0); \
  INSN3ixm(opc, $230, reg0); \
  INSN3ixm(opc, $231, reg0); \
  INSN3ixm(opc, $232, reg0); \
  INSN3ixm(opc, $233, reg0); \
  INSN3ixm(opc, $234, reg0); \
  INSN3ixm(opc, $235, reg0); \
  INSN3ixm(opc, $236, reg0); \
  INSN3ixm(opc, $237, reg0); \
  INSN3ixm(opc, $238, reg0); \
  INSN3ixm(opc, $239, reg0); \
  INSN3ixm(opc, $240, reg0); \
  INSN3ixm(opc, $241, reg0); \
  INSN3ixm(opc, $242, reg0); \
  INSN3ixm(opc, $243, reg0); \
  INSN3ixm(opc, $244, reg0); \
  INSN3ixm(opc, $245, reg0); \
  INSN3ixm(opc, $246, reg0); \
  INSN3ixm(opc, $247, reg0); \
  INSN3ixm(opc, $248, reg0); \
  INSN3ixm(opc, $249, reg0); \
  INSN3ixm(opc, $250, reg0); \
  INSN3ixm(opc, $251, reg0); \
  INSN3ixm(opc, $252, reg0); \
  INSN3ixm(opc, $253, reg0); \
  INSN3ixm(opc, $254, reg0); \
  INSN3ixm(opc, $255, reg0)


.global _start
_start:
CFINSN_indirect(jmp)
CFINSN_indirect(call)
CFINSN_direct(call)
call 1f
1:
ud2
INSN2mx_suf(lds, %ebx, l 1)
INSN2mx_suf(lds, %bx, w 1)
INSN2mx_suf(lss, %ebx, l 1)
INSN2mx_suf(lss, %bx, w 1)
INSN2mx_suf(les, %ebx, l 1)
INSN2mx_suf(les, %bx, w 1)
INSN2mx_suf(lfs, %ebx, l 1)
INSN2mx_suf(lfs, %bx, w 1)
INSN2mx_suf(lgs, %ebx, l 1)
INSN2mx_suf(lgs, %bx, w 1)
enter $1234,$0xfc
inb %dx,%al
inw %dx,%ax
inl %dx,%eax
inb $123,%al
inw $123,%ax
inl $123,%eax
outb %al,%dx
outw %ax,%dx
outl %eax,%dx
outb %al,$123
outw %ax,$123
outl %eax,$123
int3
into
INSN1m_suf(fxsav, e 1)
INSN1m_suf(fxrsto, r 1)
enter $1234,$0
popal
pushal
INSN1m_suf(nop, l 1)
INSN1m_suf(nop, w 1)
comisd %xmm1,%xmm2
INSN2mx_suf(comis, %xmm0, d 1)
comiss %xmm1,%xmm2
INSN2mx_suf(comis, %xmm0, s 1)
ucomisd %xmm1,%xmm2
INSN2mx_suf(ucomis, %xmm0, d 1)
ucomiss %xmm1,%xmm2
INSN2mx_suf(ucomis, %xmm0, s 1)
pushf
pushl %es
pushl %ecx
pushw %cx
pushl $123
pushw $123
INSN1m_suf(push, l 1)
INSN1m_suf(push, w 1)
popl %ecx
popw %cx
INSN1m_suf(pop, l 1)
INSN1m_suf(pop, w 1)
movl $123, %ecx
int $0x80

FLAG_INSN(ne)
FLAG_INSN(e)
FLAG_INSN(g)
FLAG_INSN(le)
FLAG_INSN(l)
FLAG_INSN(ge)
FLAG_INSN(a)
FLAG_INSN(be)
FLAG_INSN(b)
FLAG_INSN(ae)
FLAG_INSN(o)
FLAG_INSN(no)
FLAG_INSN(s)
FLAG_INSN(ns)
FLAG_INSN(p)
FLAG_INSN(np)

jecxz 1f
nop
1:
INSN2xm(bound, l %ebx)
loopnel 1f
nop
1:
loopel 1f
nop
1:
CFINSN_it(ret,)
CFINSN_it(retl, $123)
CFINSN_it(lretl,)
CFINSN_it(iretl,)
arpl %sp,%ax
INSN1(inc)
INSN1(dec)
INSN1(neg)
INSN1(not)
INSN2(mov)
INSN2(add)
INSN2(adc)
INSN2(sub)
INSN2(sbb)
INSN2(cmp)
INSN2(and)
INSN2(or)
INSN2(xor)
INSN2i(shl)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
INSN2c(shl)
INSN2i(shr)
INSN2i(sar)
INSN2c(shr)
INSN2c(sar)
INSN2i(rol)
INSN2c(rol)
INSN2i(rcl)
INSN2c(rcl)
INSN2i(ror)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
INSN2c(ror)
INSN2i(rcr)
INSN2c(rcr)
INSN2mx_suf(lea, %ebx, l 1)
#INSN2mx_suf(crc32, %ebx, l 1)
#INSN2mx_suf(crc32, %ebx, w 1)
#INSN2mx_suf(crc32, %ebx, b 1)
#crc32l %ecx, %ebx
#crc32w %cx, %ebx
#crc32b %cl, %ebx
#crc32b %ch, %ebx
INSN2rr(xchg)
INSN2rm(xchg)
INSN2rr(xadd)
INSN2rm(xadd)
#xlatb
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
INSN2rr(cmpxchg)
INSN2rm(cmpxchg)
INSN2xm(movbe, l %ebx)
INSN2xm(movbe, w %bx)
INSN2mx_suf(movbe, %ebx, l 1)
INSN2mx_suf(movbe, %bx, w 1)
INSN2(test)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
INSN2mx_suf(imul, %ebx, l 1)
imull %ecx, %ebx
imull $123, %ecx, %ebx
INSN3imx(imull, $123, %ebx)
INSN1(imul)
INSN1(mul)
INSN1(idiv)
INSN1(div)

CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
movzbl %bl, %edx
movzbl %bh, %edx
INSN2mx_suf(movzb, %ebx, l 1)
movzwl %bx, %edx
INSN2mx_suf(movzw, %ebx, l 1)
movsbl %bl, %edx
movsbl %bh, %edx
INSN2mx_suf(movsb, %ebx, l 1)
movswl %bx, %edx
INSN2mx_suf(movsw, %ebx, l 1)

CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
movzbw %bl, %dx
movzbw %bh, %dx
INSN2mx_suf(movzb, %bx, w 1)
movsbw %bl, %dx
movsbw %bh, %dx
INSN2mx_suf(movsb, %bx, w 1)

SHXD_INSN(shrd)
SHXD_INSN(shld)

CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
leave
hlt
daa
das
aaa
aad
aam
aas
cld
std
clc
stc
cmc
cli
sti
cbtw
cltd
cwtl
cdq
cwd
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
bswap %ecx
movw %ds, %ax
INSN2xm(mov, %ds)
movl %ds, %eax
movw %ax, %ds
movl %eax, %ss
movl %eax, %es
movl %eax, %fs
movl %eax, %gs
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
BS_INSN(bsf)
BS_INSN(bsr)
BT_INSN(bt)
BT_INSN(btc)
BT_INSN(btr)
BT_INSN(bts)
STRING_INSN(movs)
STRING_INSN(stos)
STRING_INSN(scas)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
STRING_INSN(lods)
STRING_INSN(cmps)
CMPSS_CMPSD_INSN(cmpss)
CMPSS_CMPSD_INSN(cmpsd)
cpuid
pushf
popf
rdtsc
rdpmc
sahf
lahf

CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
psubb %mm0, %mm0
INSN2mx_suf(psadb, %mm0, w 1)
movd %edx, %mm0
INSN2mx_suf(mov, %mm0, d 1)
movd %mm0, %edx
INSN2xm(movd, %mm0)
pxor %mm0,%mm1
pxor %mm0,%mm0
INSN2mx_suf(pxo,%mm0,r 1)
pand %mm0,%mm1
INSN2mx_suf(pan,%mm0,d 1)
por %mm0,%mm1
INSN2mx_suf(po,%mm0,r 1)
pandn %mm0,%mm1
INSN2mx_suf(pand,%mm0,n 1)
pcmpeqb %mm0,%mm1
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
INSN2mx_suf(pcmpeq,%mm0,b 1)
pcmpeqw %mm0,%mm1
INSN2mx_suf(pcmpeq,%mm0,w 1)
pcmpeqd %mm0,%mm1
INSN2mx_suf(pcmpeq,%mm0,d 1)
pcmpgtb %mm0,%mm1
INSN2mx_suf(pcmpgt,%mm0,b 1)
pcmpgtw %mm0,%mm1
INSN2mx_suf(pcmpgt,%mm0,w 1)
pcmpgtd %mm0,%mm1
INSN2mx_suf(pcmpgt,%mm0,d 1)

CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
por %mm0,%mm1
INSN2mx_suf(po,%mm0,r 1)
pandn %mm0,%mm1
INSN2mx_suf(pand,%mm0,n 1)
psubb %mm0,%mm1
INSN2mx_suf(psub,%mm0,b 1)
pavgb %xmm0,%xmm1
INSN2mx_suf(pavg,%xmm0,b 1)
pavgw %xmm0,%xmm1
INSN2mx_suf(pavg,%xmm0,w 1)
pmovmskb %mm0,%edx

CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
PSHUFINSN_REG(pshufd,%xmm0,%xmm1)
PSHUFINSN_MEM(pshufd,%xmm0)
PSHUFINSN_REG(pshufw,%mm0,%mm1)
PSHUFINSN_MEM(pshufw,%mm0)
psubb %mm0,%mm1
INSN2mx_suf(psub,%mm0,b 1)
pavgb %xmm0,%xmm1
INSN2mx_suf(pavg,%xmm0,b 1)
pavgw %xmm0,%xmm1
INSN2mx_suf(pavg,%xmm0,w 1)
psllw %mm0,%mm1
psllw $12,%mm0
INSN2mx_suf(psll,%mm0,w 1)
pslld %mm0,%mm1
pslld $12,%mm0
INSN2mx_suf(psll,%mm0,d 1)
psllq %mm0,%mm1
psllq $12,%mm0
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
INSN2mx_suf(psll,%mm0,q 1)
psraw $12,%mm0
psraw %mm0,%mm0
INSN2mx_suf(psra,%mm0,w 1)
psrad $12,%mm0
psrad %mm0,%mm1
INSN2mx_suf(psra,%mm0,d 1)
psrlw $12,%mm0
psrlw %mm0,%mm1
INSN2mx_suf(psrl,%mm0,w 1)
psrld $12,%mm0
psrld %mm0,%mm1
INSN2mx_suf(psrl,%mm0,d 1)
psrlq $12,%mm0
psrlq %mm0,%mm1
INSN2mx_suf(psrl,%mm0,q 1)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
palignr $12,%mm0,%mm1
INSN3imx(palignr, $12, %mm0)
pshufb %xmm0,%xmm1
PSHUFINSN_REG(shufps,%xmm0,%xmm1)
PSHUFINSN_MEM(shufps,%xmm0)
PSHUFINSN_REG(shufpd,%xmm0,%xmm1)
PSHUFINSN_MEM(shufpd,%xmm0)
pmulhrsw %mm0,%mm1
INSN2mx_suf(pmulhrs,%mm0,w 1)
pmulhuw %mm0,%mm1
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
INSN2mx_suf(pmulhu,%mm0,w 1)
pmulhw %mm0,%mm1
INSN2mx_suf(pmulh,%mm0,w 1)
pmullw %mm0,%mm1
INSN2mx_suf(pmull,%mm0,w 1)
pmuludq %mm0,%mm1
INSN2mx_suf(pmulud,%mm0,q 1)

CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
pmulhrsw %xmm0,%xmm1
INSN2mx_suf(pmulhrs,%xmm0,w 1)
pmulhuw %xmm0,%xmm1
INSN2mx_suf(pmulhu,%xmm0,w 1)
pmulhw %xmm0,%xmm1
INSN2mx_suf(pmulh,%xmm0,w 1)
pmulld %xmm0,%xmm1
INSN2mx_suf(pmull,%xmm0,d 1)
pmullw %xmm0,%xmm1
INSN2mx_suf(pmull,%xmm0,w 1)
pmuludq %xmm0,%xmm1
INSN2mx_suf(pmulud,%xmm0,q 1)

CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
movd %xmm0,%edi
INSN2xm(movd, %xmm0)
movd %edi,%xmm0
INSN2mx_suf(mov, %xmm0, d 1)

//pshufd $0,%xmm0,%xmm1
//INSN3imx(pshufd, $0, %xmm0)
//pshuflw $0,%xmm0,%xmm1
//INSN3imx(pshuflw, $0, %xmm0)
//pshufhw $0,%xmm0,%xmm1
//INSN3imx(pshufhw, $0, %xmm0)
PSHUFINSN_REG(pshuflw,%xmm0,%xmm1)
PSHUFINSN_MEM(pshuflw,%xmm0)
PSHUFINSN_REG(pshufhw,%xmm0,%xmm1)
PSHUFINSN_MEM(pshufhw,%xmm0)


CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
movdqu %xmm0,%xmm1
INSN2xm(movdqu, %xmm0)
INSN2mx_suf(movdq,%xmm0,u 1)

movdqa %xmm0,%xmm1
INSN2xm(movdqa, %xmm0)
INSN2mx_suf(movdq, %xmm0,a 1)

INSN2xm(movntdq, %xmm0)
INSN2xm(movntpd, %xmm0)
INSN2xm(movntps, %xmm0)

movaps %xmm0,%xmm1
INSN2xm(movaps, %xmm0)
INSN2mx_suf(movap, %xmm0,s 1)

CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
palignr $12,%xmm0,%xmm1
INSN3imx(palignr, $12, %xmm0)
pxor %xmm0,%xmm1
INSN2mx_suf(pxo, %xmm0,r 1)
pxor %xmm0,%xmm0
pand %xmm0,%xmm1
INSN2mx_suf(pan, %xmm0,d 1)
ptest %xmm0,%xmm1
INSN2mx_suf(ptes, %xmm0,t 1)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
por %xmm0,%xmm1
INSN2mx_suf(po, %xmm0,r 1)
pandn %xmm0,%xmm1
INSN2mx_suf(pand, %xmm0,n 1)
pavgb %xmm0,%xmm1
INSN2mx_suf(pavg, %xmm0,b 1)
pavgw %xmm0,%xmm1
INSN2mx_suf(pavg, %xmm0,w 1)

CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
xorpd %xmm0,%xmm1
INSN2mx_suf(xorp, %xmm0,d 1)
xorpd %xmm0,%xmm0
xorps %xmm0,%xmm1
INSN2mx_suf(xorp, %xmm0,s 1)
xorps %xmm0,%xmm0
orpd %xmm0,%xmm1
INSN2mx_suf(orp, %xmm0,d 1)
orps %xmm0,%xmm1
INSN2mx_suf(orp, %xmm0,s 1)
andpd %xmm0,%xmm1
INSN2mx_suf(andp, %xmm0,d 1)
andps %xmm0,%xmm1
INSN2mx_suf(andp, %xmm0,s 1)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
andnpd %xmm0,%xmm1
INSN2mx_suf(andnp, %xmm0,d 1)
andnps %xmm0,%xmm1
INSN2mx_suf(andnp, %xmm0,s 1)
pcmpeqb %xmm0,%xmm1
INSN2mx_suf(pcmpeq, %xmm0,b 1)
pcmpeqw %xmm0,%xmm1
INSN2mx_suf(pcmpeq, %xmm0,w 1)
pcmpeqd %xmm0,%xmm1
INSN2mx_suf(pcmpeq, %xmm0,d 1)
pcmpeqq %xmm0,%xmm1
INSN2mx_suf(pcmpeq, %xmm0,q 1)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
pcmpgtb %xmm0,%xmm1
INSN2mx_suf(pcmpgt, %xmm0,b 1)
pcmpgtw %xmm0,%xmm1
INSN2mx_suf(pcmpgt, %xmm0,w 1)
pcmpgtd %xmm0,%xmm1
INSN2mx_suf(pcmpgt, %xmm0,d 1)
pcmpgtq %xmm0,%xmm1
INSN2mx_suf(pcmpgt, %xmm0,q 1)
addps %xmm0,%xmm1
INSN2mx_suf(addp,%xmm0,s 1)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
addpd %xmm0,%xmm1
INSN2mx_suf(addp,%xmm0,d 1)
addss %xmm0,%xmm1
INSN2mx_suf(adds,%xmm0,s 1)
addsd %xmm0,%xmm1
INSN2mx_suf(adds,%xmm0,d 1)
subps %xmm0,%xmm1
INSN2mx_suf(subp,%xmm0,s 1)
subpd %xmm0,%xmm1
INSN2mx_suf(subp,%xmm0,d 1)
subss %xmm0,%xmm1
INSN2mx_suf(subs,%xmm0,s 1)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
subsd %xmm0,%xmm1
INSN2mx_suf(subs,%xmm0,d 1)
mulps %xmm0,%xmm1
INSN2mx_suf(mulp,%xmm0,s 1)
mulpd %xmm0,%xmm1
INSN2mx_suf(mulp,%xmm0,d 1)
mulss %xmm0,%xmm1
INSN2mx_suf(muls,%xmm0,s 1)
mulsd %xmm0,%xmm1
INSN2mx_suf(muls,%xmm0,d 1)
divps %xmm0,%xmm1
INSN2mx_suf(divp,%xmm0,s 1)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
divpd %xmm0,%xmm1
INSN2mx_suf(divp,%xmm0,d 1)
divss %xmm0,%xmm1
INSN2mx_suf(divs,%xmm0,s 1)
divsd %xmm0,%xmm1
INSN2mx_suf(divs,%xmm0,d 1)
movss %xmm0,%xmm1
INSN2mx_suf(movs,%xmm0,s 1)
INSN2xm(movss, %xmm0)
movupd %xmm0,%xmm1
INSN2mx_suf(movup,%xmm0,d 1)
INSN2xm(movupd, %xmm0)
movups %xmm0,%xmm1
INSN2mx_suf(movup,%xmm0,s 1)
INSN2xm(movups, %xmm0)

CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
paddb %mm0,%mm1
INSN2mx_suf(padd,%mm0,b 1)
paddw %mm0,%mm1
INSN2mx_suf(padd,%mm0,w 1)
paddd %mm0,%mm1
INSN2mx_suf(padd,%mm0,d 1)
paddb %xmm0,%xmm1
INSN2mx_suf(padd,%xmm0,b 1)
paddw %xmm0,%xmm1
INSN2mx_suf(padd,%xmm0,w 1)
paddd %xmm0,%xmm1
INSN2mx_suf(padd,%xmm0,d 1)
paddq %xmm0,%xmm1
INSN2mx_suf(padd,%xmm0,q 1)

CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
psubb %mm0,%mm1
INSN2mx_suf(psub,%mm0,b 1)
psubw %mm0,%mm1
INSN2mx_suf(psub,%mm0,w 1)
psubd %mm0,%mm1
INSN2mx_suf(psub,%mm0,d 1)
psubb %xmm0,%xmm1
INSN2mx_suf(psub,%xmm0,b 1)
psubw %xmm0,%xmm1
INSN2mx_suf(psub,%xmm0,w 1)
psubd %xmm0,%xmm1
INSN2mx_suf(psub,%xmm0,d 1)
psubq %xmm0,%xmm1
INSN2mx_suf(psub,%xmm0,q 1)

CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
psubsb %mm0,%mm1
INSN2mx_suf(psubs,%mm0,b 1)
psubsb %xmm0,%xmm1
INSN2mx_suf(psubs,%xmm0,b 1)
psubsw %mm0,%mm1
INSN2mx_suf(psubs,%mm0,w 1)
psubsw %xmm0,%xmm1
INSN2mx_suf(psubs,%xmm0,w 1)
psubusb %mm0,%mm1
INSN2mx_suf(psubus,%mm0,b 1)
psubusb %xmm0,%xmm1
INSN2mx_suf(psubus,%xmm0,b 1)
psubusw %mm0,%mm1
INSN2mx_suf(psubus,%mm0,w 1)
psubusw %xmm0,%xmm1
INSN2mx_suf(psubus,%xmm0,w 1)

CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
pmovmskb %xmm0,%edx
psubb %xmm0,%xmm1
INSN2mx_suf(psub, %xmm0,b 1)
psllw %xmm0,%xmm1
psllw $12,%xmm0
INSN2mx_suf(psll, %xmm0,w 1)
pslld %xmm0,%xmm1
pslld $12,%xmm0
INSN2mx_suf(psll, %xmm0,d 1)
psllq %xmm0,%xmm1
psllq $12,%xmm0
INSN2mx_suf(psll, %xmm0,q 1)
psraw $12,%xmm0
psraw %xmm0,%xmm1
INSN2mx_suf(psra, %xmm0,w 1)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
psrad $12,%xmm0
psrad %xmm0,%xmm1
INSN2mx_suf(psra, %xmm0,d 1)
psrlw $12,%xmm0
psrlw %xmm0,%xmm1
INSN2mx_suf(psrl, %xmm0,w 1)
psrld $12,%xmm0
psrld %xmm0,%xmm1
INSN2mx_suf(psrl, %xmm0,d 1)
psrlq $12,%xmm0
psrlq %xmm0,%xmm1
INSN2mx_suf(psrl, %xmm0,q 1)
pslldq $12,%xmm0
psrldq $12,%xmm0

CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
pcmpistri $0x1a,%xmm0,%xmm1
pcmpistri $0x3a,%xmm0,%xmm1
pcmpistri $0x2,%xmm0,%xmm1
pcmpistri $0x12,%xmm0,%xmm1

CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
movlhps %xmm0,%xmm1
movhlps %xmm0,%xmm1
INSN2mx_suf(movlp, %xmm0,s 1)
INSN2xm(movlps, %xmm0)
INSN2mx_suf(movhp, %xmm0,s 1)
INSN2xm(movhps, %xmm0)
INSN2mx_suf(movlp, %xmm0,d 1)
INSN2xm(movlpd, %xmm0)
INSN2mx_suf(movhp, %xmm0,d 1)
INSN2xm(movhpd, %xmm0)
movsd %xmm0,%xmm1
INSN2xm(movsd, %xmm0)
movq %xmm0,%xmm1
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
INSN2xm(movq, %xmm0)
INSN2mx_suf(movs, %xmm0, d 1)
INSN2mx_suf(mov, %xmm0, q 1)
INSN2mx_suf(mov, %mm0, q 1)
movq %mm0,%mm1
INSN2xm(movq, %mm0)
pminsw %mm0,%mm1
INSN2mx_suf(pmins,%mm0,w 1)
pminsb %xmm0,%xmm1
INSN2mx_suf(pmins, %xmm0,b 1)
pminsw %xmm0,%xmm1
INSN2mx_suf(pmins, %xmm0,w 1)
pminsd %xmm0,%xmm1
INSN2mx_suf(pmins, %xmm0,d 1)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
pminub %mm0,%mm1
INSN2mx_suf(pminu,%mm0,b 1)
pminub %xmm0,%xmm1
INSN2mx_suf(pminu, %xmm0,b 1)
pminuw %xmm0,%xmm1
INSN2mx_suf(pminu, %xmm0,w 1)
pminud %xmm0,%xmm1
INSN2mx_suf(pminu, %xmm0,d 1)

CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
maxpd %xmm0,%xmm1
INSN2mx_suf(maxp,%xmm0,d 1)
maxps %xmm0,%xmm1
INSN2mx_suf(maxp,%xmm0,s 1)
maxsd %xmm0,%xmm1
INSN2mx_suf(maxs,%xmm0,d 1)
maxss %xmm0,%xmm1
INSN2mx_suf(maxs,%xmm0,s 1)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
minpd %xmm0,%xmm1
INSN2mx_suf(minp,%xmm0,d 1)
minps %xmm0,%xmm1
INSN2mx_suf(minp,%xmm0,s 1)
minsd %xmm0,%xmm1
INSN2mx_suf(mins,%xmm0,d 1)
minss %xmm0,%xmm1
INSN2mx_suf(mins,%xmm0,s 1)

CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
pmaxsw %mm0,%mm1
INSN2mx_suf(pmaxs,%mm0,w 1)
pmaxsb %xmm0,%xmm1
INSN2mx_suf(pmaxs, %xmm0,b 1)
pmaxsw %xmm0,%xmm1
INSN2mx_suf(pmaxs, %xmm0,w 1)
pmaxsd %xmm0,%xmm1
INSN2mx_suf(pmaxs, %xmm0,d 1)
pmaxub %mm0,%mm1
INSN2mx_suf(pmaxu,%mm0,b 1)
pmaxub %xmm0,%xmm1
INSN2mx_suf(pmaxu, %xmm0,b 1)
pmaxuw %xmm0,%xmm1
INSN2mx_suf(pmaxu, %xmm0,w 1)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
pmaxud %xmm0,%xmm1
INSN2mx_suf(pmaxu, %xmm0,d 1)
punpckhbw %xmm0,%xmm1
INSN2mx_suf(punpckhb, %xmm0,w 1)
punpckhwd %xmm0,%xmm1
INSN2mx_suf(punpckhw, %xmm0,d 1)
punpckhdq %xmm0,%xmm1
INSN2mx_suf(punpckhd, %xmm0,q 1)
punpckhqdq %xmm0,%xmm1
INSN2mx_suf(punpckhq, %xmm0,dq 1)

CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
packuswb %xmm0,%xmm1
punpcklbw %xmm0,%xmm1
INSN2mx_suf(punpcklb, %xmm0,w 1)
punpcklwd %xmm0,%xmm1
INSN2mx_suf(punpcklw, %xmm0,d 1)
punpckldq %xmm0,%xmm1
INSN2mx_suf(punpckld, %xmm0,q 1)
punpcklqdq %xmm0,%xmm1
INSN2mx_suf(punpcklq, %xmm0,dq 1)
packuswb %xmm0,%xmm1
INSN2mx_suf(packusw, %xmm0,b 1)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
packusdw %xmm0,%xmm1
INSN2mx_suf(packusd, %xmm0,w 1)
packsswb %xmm0,%xmm1
INSN2mx_suf(packssw, %xmm0,b 1)
packssdw %xmm0,%xmm1
INSN2mx_suf(packssd, %xmm0,w 1)
unpckhpd %xmm0,%xmm1
INSN2mx_suf(unpckhp, %xmm0,d 1)
unpckhps %xmm0,%xmm1
INSN2mx_suf(unpckhp, %xmm0,s 1)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
unpcklpd %xmm0,%xmm1
INSN2mx_suf(unpcklp, %xmm0,d 1)
unpcklps %xmm0,%xmm1
INSN2mx_suf(unpcklp, %xmm0,s 1)

CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
PSHUFINSN_REG(pinsrb,%edx,%xmm0)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
PSHUFINSN_REG(pinsrw,%edx,%xmm0)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
PSHUFINSN_REG(pinsrw,%edx,%mm0)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
PSHUFINSN_REG(pinsrd,%edx,%xmm0)
#PSHUFINSN_REG(pinsrq,%edx,%xmm0)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
PSHUFINSN_MEM(pinsrb,%xmm0)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
PSHUFINSN_MEM(pinsrw,%xmm0)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
PSHUFINSN_MEM(pinsrw,%mm0)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
PSHUFINSN_MEM(pinsrd,%xmm0)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
#PSHUFINSN_MEM(pinsrq,%xmm0)

CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
PSHUFINSN_REG(pextrb,%xmm0,%edx)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
PSHUFINSN_REG(pextrw,%xmm0,%edx)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
PSHUFINSN_REG(pextrw,%mm0,%edx)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
PSHUFINSN_REG(pextrd,%xmm0,%edx)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
#PSHUFINSN_REG(pextrq,%edx,%xmm0)
PSHUFINSN_xMEM(pextrb,%xmm0)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
PSHUFINSN_xMEM(pextrw,%xmm0)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
PSHUFINSN_xMEM(pextrd,%xmm0)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
#PSHUFINSN_MEM(pextrq,%xmm0)


CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
xgetbv
xsetbv


CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
/* bmi */
BMI3m0(andn)
BMI3m1(bextr)
BMI2(bls,i)
BMI2(bls,msk)
BMI2(bls,r)
BMI3m1(bzhi)
BMI2(lz,cnt)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
BMI2w(lz,cnt)
BMI3m0(mulxl)
#BMI3m0(pdep)
#BMI3m0(pext)
BMI3i0m1(rorx)
BMI3m1(sarx)
BMI3m1(shlx)
BMI3m1(shrx)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
BMI2(tz,cnt)
BMI2w(tz,cnt)
BMI2(popcnt,l)
BMI2w(popcnt,w)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
lfence
sfence
INSN1m_suf(prefetcht, 0 1)
INSN1m_suf(prefetchnt, a 1)

CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
cvtdq2pd %xmm0,%xmm1
INSN2mx_suf(cvtdq2p,%xmm0,d 1)
cvtdq2ps %xmm0,%xmm1
INSN2mx_suf(cvtdq2p,%xmm0,s 1)
cvtpd2dq %xmm0,%xmm1
INSN2mx_suf(cvtpd2d,%xmm0,q 1)
cvtpd2pi %xmm0,%mm0
INSN2mx_suf(cvtpd2p,%mm0,i 1)
cvtpd2ps %xmm0,%xmm1
INSN2mx_suf(cvtpd2p,%xmm0,s 1)
cvtps2pd %xmm0,%xmm1
INSN2mx_suf(cvtps2p,%xmm0,d 1)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
cvtps2pi %xmm0,%mm0
INSN2mx_suf(cvtps2p,%mm0,i 1)
cvtsd2si %xmm0,%edx
INSN2mx_suf(cvtsd2s,%edx,i 1)
cvtsd2ss %xmm0,%xmm1
INSN2mx_suf(cvtsd2s,%xmm0,s 1)
cvtsi2sd %edx,%xmm1
INSN2mx_suf(cvtsi2s,%xmm0,d 1)
cvtsi2ss %edx,%xmm1
INSN2mx_suf(cvtsi2s,%xmm0,s 1)
cvtss2si %xmm0,%edx
INSN2mx_suf(cvtss2s,%edx,i 1)
cvtss2sd %xmm0,%xmm1
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
INSN2mx_suf(cvtss2s,%xmm0,d 1)
cvtpd2dq %xmm0,%xmm1
INSN2mx_suf(cvtpd2d,%xmm0,q 1)
cvtps2dq %xmm0,%xmm1
INSN2mx_suf(cvtps2d,%xmm0,q 1)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
cvttpd2dq %xmm0,%xmm1
INSN2mx_suf(cvttpd2d,%xmm0,q 1)
cvttpd2pi %xmm0,%mm0
INSN2mx_suf(cvttpd2p,%mm0,i 1)
cvttps2dq %xmm0,%xmm1
INSN2mx_suf(cvttps2d,%xmm0,q 1)
cvttps2pi %xmm0,%mm0
INSN2mx_suf(cvttps2p,%mm0,i 1)
cvttsd2si %xmm0,%ebx
INSN2mx_suf(cvttsd2s,%ebx,i 1)
cvttss2si %xmm0,%ebx
INSN2mx_suf(cvttss2s,%ebx,i 1)



CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
/* floating point */
INSN1m_suf(fstcw, w 1)
INSN1m_suf(fnstcw, w 1)

INSN1m_suf(fstsw, w 1)
INSN1m_suf(fnstsw, w 1)
fstsw %ax
fnstsw %ax

INSN1m_suf(stmxcs, r 1)
//INSN1m_suf(vstmxcs, r 1)
INSN1m_suf(ldmxcs, r 1)
//INSN1m_suf(vldmxcs, r 1)

INSN1m_suf(fldcw, w 1)
INSN1m_suf(fld, s 1) //32-bit
INSN1m_suf(fld, l 1) //64-bit
INSN1m_suf(fld, t 1) //80-bit

CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
INSN1m_suf(fistpl, l 1)
INSN1m_suf(fistp, s 1)
INSN1m_suf(fist, s 1)
INSN1m_suf(fistp, l 1)
INSN1m_suf(fist, l 1)

INSN1m_suf(fild, l 1)
INSN1m_suf(fildl, l 1)
INSN1m_suf(fil, d 1)

CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
INSN1m_suf(fmul, l 1)
INSN1m_suf(fmul, s 1)
INSN1m_suf(fidiv, l 1)
INSN1m_suf(fidiv, s 1)
INSN1m_suf(fdiv, l 1)
INSN1m_suf(fdiv, s 1)
fscale
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
fmul %st(0),%st(7)
fmul %st(0),%st(6)
fmul %st(0),%st(5)
fmul %st(0),%st(4)
fmul %st(0),%st(3)
fmul %st(0),%st(2)
fmul %st(0),%st(1)
fmul %st(0),%st
fmul %st(1),%st
fmul %st(2),%st(0)
fmul %st(3),%st(0)
fmul %st(4),%st(0)
fmul %st(5),%st(0)
fmul %st(6),%st(0)
fmul %st(7),%st(0)
fdiv %st(7),%st(0)
fdiv %st(6),%st(0)
fdiv %st(5),%st(0)
fdiv %st(4),%st(0)
fdiv %st(3),%st(0)
fdiv %st(2),%st
fdiv %st(1),%st(0)
fdiv %st(0),%st(1)
fdiv %st(0),%st(2)
fdiv %st(0),%st(3)
fdiv %st(0),%st(4)
fdiv %st(0),%st(5)
fdiv %st(0),%st(6)
fdiv %st(0),%st(7)
fdivp %st(0),%st
fdivp %st(0),%st(1)
fdivp %st(0),%st(2)
fdivp %st(0),%st(3)
fdivp %st(0),%st(4)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
fdivp %st(0),%st(5)
fdivp %st(0),%st(6)
fdivp %st(0),%st(7)
fdivr %st(0),%st(7)
fdivr %st(0),%st(6)
fdivr %st(0),%st(5)
fdivr %st(0),%st(4)
fdivr %st(0),%st(3)
fdivr %st(0),%st(2)
fdivr %st(0),%st(1)
fdivr %st(7),%st(0)
fdivr %st(6),%st(0)
fdivr %st(5),%st(0)
fdivr %st(4),%st(0)
fdivr %st(3),%st(0)
fdivr %st(2),%st(0)
fdivr %st(1),%st(0)
INSN1m_suf(fdivr, l 1)
INSN1m_suf(fdivr, s 1)

CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
fadd %st(0),%st(0)
fadd %st(0),%st(1)
fadd %st(0),%st(2)
fadd %st(0),%st(3)
fadd %st(0),%st(4)
fadd %st(0),%st(5)
fadd %st(0),%st(6)
fadd %st(0),%st(7)
fadd %st(1),%st(0)
fadd %st(2),%st(0)
fadd %st(3),%st(0)
fadd %st(4),%st(0)
fadd %st(5),%st(0)
fadd %st(6),%st(0)
fadd %st(7),%st(0)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
INSN1m_suf(fadd, l 1)
INSN1m_suf(fadd, s 1)
INSN1m_suf(fiad, d 1)
INSN1m_suf(fiadd, l 1)
INSN1m_suf(fimul, l 1)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
faddp %st(0),%st(0)
faddp %st(0),%st(1)
faddp %st(0),%st(2)
faddp %st(0),%st(3)
faddp %st(0),%st(4)
faddp %st(0),%st(5)
faddp %st(0),%st(6)
faddp %st(0),%st(7)
fsub %st(0),%st(0)
fsub %st(0),%st(1)
fsub %st(0),%st(2)
fsub %st(0),%st(3)
fsub %st(0),%st(4)
fsub %st(0),%st(5)
fsub %st(0),%st(6)
fsub %st(0),%st(7)
fsub %st(1),%st(0)
fsub %st(2),%st(0)
fsub %st(3),%st(0)
fsub %st(4),%st(0)
fsub %st(5),%st(0)
fsub %st(6),%st(0)
fsub %st(7),%st(0)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
INSN1m_suf(fsub, l 1)
INSN1m_suf(fsub, s 1)
INSN1m_suf(fisu, b 1)
fsubp %st(0),%st(1)
fsubp %st(0),%st(1)
fsubp %st(0),%st(2)
fsubp %st(0),%st(3)
fsubp %st(0),%st(4)
fsubp %st(0),%st(5)
fsubp %st(0),%st(6)
fsubp %st(0),%st(7)
fsubr %st(0),%st(0)
fsubr %st(0),%st(1)
fsubr %st(0),%st(2)
fsubr %st(0),%st(3)
fsubr %st(0),%st(4)
fsubr %st(0),%st(5)
fsubr %st(0),%st(6)
fsubr %st(0),%st(7)
fsubr %st(0),%st(0)
fsubr %st(1),%st(0)
fsubr %st(2),%st(0)
fsubr %st(3),%st(0)
fsubr %st(4),%st(0)
fsubr %st(5),%st(0)
fsubr %st(6),%st(0)
fsubr %st(7),%st(0)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
INSN1m_suf(fisub, l 1)
INSN1m_suf(fisub, r 1)
INSN1m_suf(fsubr, s 1)
INSN1m_suf(fsubr, l 1)
CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
fsubrp %st(0),%st(0)
fsubrp %st(0),%st(1)
fsubrp %st(0),%st(2)
fsubrp %st(0),%st(3)
fsubrp %st(0),%st(4)
fsubrp %st(0),%st(5)
fsubrp %st(0),%st(6)
fsubrp %st(0),%st(7)
fmulp %st(0),%st(7)
fmulp %st(0),%st(6)
fmulp %st(0),%st(5)
fmulp %st(0),%st(4)
fmulp %st(0),%st(3)
fmulp %st(0),%st(2)
fmulp %st(0),%st(1)
fmulp %st(0),%st(0)


CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
fdiv %st(0),%st
fdivrp %st(0),%st
fdivrp %st(0),%st(1)
fdivrp %st(0),%st(2)
fdivrp %st(0),%st(3)
fdivrp %st(0),%st(4)
fdivrp %st(0),%st(5)
fdivrp %st(0),%st(6)
fdivrp %st(0),%st(7)
fsubp %st(0),%st(0)

CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
INSN2st_to_zero(fcmovb)
INSN2st_to_zero(fcmove)
INSN2st_to_zero(fcmovbe)
INSN2st_to_zero(fcmovu)
INSN2st_to_zero(fcmovnb)
INSN2st_to_zero(fcmovne)
INSN2st_to_zero(fcmovnbe)
INSN2st_to_zero(fcmovnu)



CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
INSN1st(fld)
INSN1st(fst)
INSN1st(fstp)
INSN1st(fxch)
INSN1st(fucom)
INSN1st(fucomp)
INSN1st(fcomp)
INSN1st(fcom)
INSN1st(fcomi)
INSN1st(fcomip)
INSN1st(fucomi)
INSN1st(fucomip)
fcompp
INSN1m_suf(fcom, s 1)
INSN1m_suf(fcom, l 1)
INSN1m_suf(fcomp, s 1)
INSN1m_suf(fcomp, l 1)

CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
INSN1m_suf(fst, s 1) //32-bit
INSN1m_suf(fst, l 1) //64-bit ; 80-bit variant of fst does not exist
INSN1m_suf(fstp, s 1) //32-bit
INSN1m_suf(fstp, l 1) //64-bit
INSN1m_suf(fstp, t 1) //80-bit

CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
fld1
fldl2t
fldl2e
fldpi
fldlg2
fldln2
fldz

CFINSN_it(jmp, *%eax)   //break to avoid bbl size becoming too big
fucompp
fnop
fchs
fabs

CFINSN_direct(jmp)
_end:
