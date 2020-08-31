#ifndef __EXECUTE_H
#define __EXECUTE_H

//#define TESTVECTORS_FILENAME	"../generator/testvectors"

static void *tvloc;
static void *invcode_loc;
static void *dc_code_loc;
static int dummy;

#define M(i) (((i)<<6) | ((i)<<4)  | ((i)<<2) | ((i)))
#define S(i) (((i)<<6) | ((i)<<4)  | ((i)<<2) | ((i)))

#define JUMP1_INDICATOR_NOT_TAKEN_VALUE			0x12345678
#define JUMP1_INDICATOR_TAKEN_VALUE			0x87654321

#define SETUP_STACK(stateptr,tvptr)
#define SETUP_MEMORY(stateptr,tvptr)

#define SETUP_REGS(stateptr,tvptr) do {					\
  stateptr->eax = tvptr->eax;						\
  stateptr->ecx = tvptr->ecx;						\
  stateptr->edx = tvptr->edx;						\
  stateptr->ebx = tvptr->ebx;						\
  stateptr->esp = tvptr->esp;						\
  stateptr->ebp = tvptr->ebp;						\
  stateptr->esi = tvptr->esi;						\
  stateptr->edi = tvptr->edi;						\
  stateptr->eflags = tvptr->eflags;					\
} while (0)

#define SETUP_MMX_REGS(stateptr,tvptr) do {				\
  stateptr->mm0 = tvptr->mm0;						\
  stateptr->mm1 = tvptr->mm1;						\
  stateptr->mm2 = tvptr->mm2;						\
  stateptr->mm3 = tvptr->mm3;						\
  stateptr->mm4 = tvptr->mm4;						\
  stateptr->mm5 = tvptr->mm5;						\
  stateptr->mm6 = tvptr->mm6;						\
  stateptr->mm7 = tvptr->mm7;						\
} while (0)

#define FILL32(x) (0UL-(x))
#define FILL64(x) (0ULL-(x))
/*flag mask = 0x36402 (not needed)*/
#define FILL_EFLAGS(af, of, cf, zf, sf, pf) 				\
  ((of << 11) | (sf << 7) | (zf << 6) | (af << 4) | (pf << 2) | cf)
#define SETUP_DC_BITS(stateptr, live_regs) do {				\
  (stateptr)->dc_eax = FILL32(live_regs.eax);				\
  (stateptr)->dc_ecx = FILL32(live_regs.ecx);				\
  (stateptr)->dc_edx = FILL32(live_regs.edx);				\
  (stateptr)->dc_ebx = FILL32(live_regs.ebx);				\
  (stateptr)->dc_esp = FILL32(live_regs.esp);				\
  (stateptr)->dc_ebp = FILL32(live_regs.ebp);				\
  (stateptr)->dc_esi = FILL32(live_regs.esi);				\
  (stateptr)->dc_edi = FILL32(live_regs.edi);				\
  (stateptr)->dc_eflags = FILL_EFLAGS(live_regs.af, live_regs.of,	\
      live_regs.cf, live_regs.zf, live_regs.sf, live_regs.pf);		\
  (stateptr)->dc_mm0 = FILL64(live_regs.mm0);				\
  (stateptr)->dc_mm1 = FILL64(live_regs.mm1);				\
  (stateptr)->dc_mm2 = FILL64(live_regs.mm2);				\
  (stateptr)->dc_mm3 = FILL64(live_regs.mm3);				\
  (stateptr)->dc_mm4 = FILL64(live_regs.mm4);				\
  (stateptr)->dc_mm5 = FILL64(live_regs.mm5);				\
  (stateptr)->dc_mm6 = FILL64(live_regs.mm6);				\
  (stateptr)->dc_mm7 = FILL64(live_regs.mm7);				\
  									\
  for (int jj = 0; jj < MEM_ARRAY_SIZE; jj++)				\
  {									\
      (stateptr)->dc_memory[jj] = 0xffffffff;				\
  } 									\
} while (0)

#define SETUP_JUMP1_INDICATOR() do {					\
  *((uint32_t *)JUMP1_INDICATOR_LOCATION) =				\
  		JUMP1_INDICATOR_NOT_TAKEN_VALUE;			\
} while (0)

#define HASH_JUMP1_INDICATOR 						\
  ((*((uint32_t *)JUMP1_INDICATOR_LOCATION))^JUMP1_INDICATOR_NOT_TAKEN_VALUE)

#define EXECUTE_TESTVECTOR(i) do {					\
    static machine_state *stateptr, *retval, *tvptr;			\
    stateptr = &i386_seq::retstate[i];					\
    tvptr = &i386_seq::testvectors[i];					\
    SETUP_REGS(stateptr, tvptr);					\
    SETUP_MMX_REGS(stateptr, tvptr);					\
    SETUP_STACK(stateptr,tvptr);					\
    SETUP_MEMORY(stateptr,tvptr);					\
    SETUP_JUMP1_INDICATOR();						\
    tvloc = i386_seq::tv_locations[i];					\
    /*__asm__ volatile ("push %eax");*/					\
    __asm__ volatile ("push %1" : "=g" (dummy) : "m" (stateptr));	\
    __asm__ volatile ("call *%1"					\
	: "=g" (dummy)							\
	: "m" (tvloc));							\
    /*__asm__ volatile ("movl %%eax, %1" 				\
	: "=g" (dummy) : "m" (retval));*/				\
    __asm__ volatile ("pop %eax");					\
    /*__asm__ volatile ("pop %eax");*/					\
    /*if (retval!=stateptr)						\
	error_exit("TV execution failed.retval\n");*/			\
} while (0)

/* Do not need to setup the testvector. Just execute the instruction
   sequence. */
#define EXECUTE_ON_STATE(stateptr) do {					\
    /*machine_state *stateptr = &i386_seq::retstate[i];*/		\
    machine_state *retval;						\
    void *dummy;							\
    invcode_loc = i386_seq::invcode_location;				\
    __asm__ volatile ("push %1" : "=g" (dummy) : "m" (stateptr));	\
    __asm__ volatile ("call *%1"					\
	: "=g" (dummy)							\
	: "m" (invcode_loc));						\
    /*__asm__ volatile ("movl %%eax, %1" 				\
	: "=g" (dummy) : "m" (retval));*/				\
    __asm__ volatile ("pop %eax");					\
    /*if (retval!=stateptr)						\
	error_exit("Retstate inverse execution failed.retval\n");*/	\
} while (0)

#define EXECUTE_DC_ON_STATE(stateptr) do {				\
    /* machine_state *stateptr = &i386_seq::retstate[i]; */		\
    machine_state *retval;						\
    void *dummy;							\
    dc_code_loc = i386_seq::dc_code_location;				\
    __asm__ volatile ("push %1" : "=g" (dummy) : "m" (stateptr));	\
    __asm__ volatile ("call *%1"					\
	: "=g" (dummy)							\
	: "m" (dc_code_loc));						\
    /*__asm__ volatile ("movl %%eax, %1" 				\
	: "=g" (dummy) : "m" (retval));*/				\
    __asm__ volatile ("pop %eax");					\
    /*if (retval!=stateptr)						\
	error_exit("Retstate dc-code execution failed.retval\n");*/	\
} while (0)


#if FINGERPRINT_REGS
#if FINGERPRINT_STACK
#define HASH_REGS(state, live_regs)					\
({									\
  DBG (MACHINESTATE_FP, ("eax = %x, ecx = %x, edx = %x,"		\
	" ebx = %x, esp = %x, ebp = %x,"				\
	" esi = %x, edi = %x.\n", state->eax,				\
	state->ecx,	state->edx, state->ebx,				\
	state->esp, state->ebp, state->esi,				\
	state->edi));							\
 									\
  unsigned ret = 0x0;							\
  if (live_regs.eax)							\
    ret ^= state->eax;							\
  if (live_regs.ecx)							\
    ret ^= (state->ecx + 0x110011);					\
  if (live_regs.edx)							\
    ret ^= (state->edx + 0x220022);					\
  if (live_regs.ebx)							\
    ret ^= (state->ebx + 0x330033);					\
  if (live_regs.esi)							\
    ret ^= (state->esi + 0x660066);					\
  if (live_regs.edi)							\
    ret ^= (state->edi + 0x770077);					\
  ret;									\
})
#else
#define HASH_REGS(state, live_regs)					\
({									\
  DBG (MACHINESTATE_FP, ("eax = %x, ecx = %x, edx = %x,"		\
	" ebx = %x, esp = %x, ebp = %x,"				\
	" esi = %x, edi = %x.\n", state->eax,				\
	state->ecx,	state->edx, state->ebx,				\
	state->esp, state->ebp, state->esi,				\
	state->edi));							\
  									\
  unsigned ret = 0x0;							\
  if (live_regs.eax)							\
    ret ^= state->eax;							\
  if (live_regs.ecx)							\
    ret ^= (state->ecx + 0x110011);					\
  if (live_regs.edx)							\
    ret ^= (state->edx + 0x220022);					\
  if (live_regs.ebx)							\
    ret ^= (state->ebx + 0x330033);					\
  if (live_regs.esp)							\
    ret ^= (state->esp + 0x440044);					\
  if (live_regs.ebp)							\
    ret ^= (state->ebp + 0x550055);					\
  if (live_regs.esi)							\
    ret ^= (state->esi + 0x660066);					\
  if (live_regs.edi)							\
    ret ^= (state->edi + 0x770077);					\
  ret;									\
})
#endif
#else
#define HASH_REGS(state, live_regs) 0
#endif

#define HASH_FLAGS(state, live_regs)					\
    ({									\
       unsigned ret;							\
       if (live_regs.af || live_regs.of || live_regs.cf ||		\
	   live_regs.zf || live_regs.sf || live_regs.pf)		\
       {								\
         ret = state->eflags;						\
       }								\
       else								\
       {								\
         ret = 0x0;							\
       }								\
       ret;								\
     })

#define HASH_MEM(state) 0

#if FINGERPRINT_STACK
#define HASH_STACK(state) ({						\
    unsigned long long ret ;						\
    ret = (state->esp+R(4))^(state->ebp+R(5)) ;				\
    DBG(MACHINESTATE_FP,("esp=%x,ebp=%x\n",state->esp,state->ebp)) ;	\
    for(int i=1;i<MAX_STACK_SIZE-1;i++) ret ^= (S(i)+state->stack[i]);	\
    ret; })
#else
#define HASH_STACK(state) 0
#endif

#if FINGERPRINT_MMX
#define HASH_MMX(state)							\
    LSB32(state->mm0)^LSB32(state->mm1)^LSB32(state->mm2)^		\
    LSB32(state->mm3)^LSB32(state->mm4)^LSB32(state->mm5)^		\
    LSB32(state->mm6)^LSB32(state->mm7)^				\
    MSB32(state->mm0)^MSB32(state->mm1)^MSB32(state->mm2)^		\
    MSB32(state->mm3)^MSB32(state->mm4)^MSB32(state->mm5)^		\
    MSB32(state->mm6)^MSB32(state->mm7)
#else
#define HASH_MMX(state) 0
#endif

#define hash_machine_state(state, live_regs)	({			\
    DBG (MACHINESTATE_FP,("HASH_REGS(state)=%x\n",			\
	HASH_REGS((state), (live_regs))));				\
    DBG (MACHINESTATE_FP,("HASH_FLAGS(state)=%x\n",			\
			 HASH_FLAGS((state), (live_regs))));		\
    DBG (MACHINESTATE_FP,("HASH_MEM(state)=%x\n",HASH_MEM((state)))) ;	\
    DBG (MACHINESTATE_FP,("HASH_STACK(state)=%x\n",HASH_STACK((state))));\
    DBG (MACHINESTATE_FP,("HASH_MMX(state)=%x\n",HASH_MMX((state)))) ;	\
    DBG (MACHINESTATE_FP,("HASH_STATE=%x\n",				\
			  (HASH_REGS((state), (live_regs))^		\
			   HASH_FLAGS((state), (live_regs))^		\
			   HASH_MEM((state))^HASH_STACK((state))^	\
			   HASH_MMX((state))))) ;			\
    (unsigned long long)						\
    (HASH_REGS((state), (live_regs))^HASH_FLAGS((state), (live_regs))^	\
     HASH_MEM((state))^HASH_STACK((state))^HASH_MMX((state))		\
     ^HASH_JUMP1_INDICATOR); })

#define JUMP_BACK_TO_TESTVECTOR(ibufp) do				\
{									\
  /* set it up to jump back to test vector */				\
  *ibufp++ = 0xff; *ibufp++ = 0x25;					\
  *(void **)ibufp = ((unsigned char *)ISEQ_SECTION+4);			\
  ibufp += sizeof (void **); *ibufp++ = '\0';				\
} while (0)


#endif
