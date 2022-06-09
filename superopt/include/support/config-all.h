#ifndef CONFIG_ALL_H
#define CONFIG_ALL_H

#define COMMON_SRC 1
#define COMMON_DST 2

#define HARVEST_LOOPS 1

#define ARCH_I386 1
#define ARCH_PPC  2
#define ARCH_ARM  3
#define ARCH_ETFG 4
#define ARCH_X64  5

#define FALLBACK_ID 1
#define FALLBACK_QEMU 2
#define FALLBACK_ETFG 3

#define CALLRET_ID 1
#define CALLRET_TX 2

#define ENDIANNESS_BIG 1
#define ENDIANNESS_LITTLE 2

#define SRC_PCREL_START 1
#define SRC_PCREL_END 2

#define JUMPTABLE_ADD_NONE 1
#define JUMPTABLE_ADD_UNREACHABLE 2
#define JUMPTABLE_ADD_ALL 3

#if ARCH_SRC == ARCH_DST || ARCH_SRC == ARCH_ETFG

#define CALLRET CALLRET_ID

#else

#define CALLRET CALLRET_TX

#endif

#if ARCH_SRC == ARCH_DST
#define SRC_FALLBACK FALLBACK_ID
#elif ARCH_SRC == ARCH_ETFG
#define SRC_FALLBACK FALLBACK_ETFG
#else
#define SRC_FALLBACK FALLBACK_QEMU
#endif

#if ARCH_SRC == ARCH_I386

#define SRC_CROSSTOOL_PATH I386_CROSSTOOL_PATH
#define SRC_LD_EXEC I386_LD_EXEC
#define SRC_AS_EXEC I386_AS_EXEC
#define SRC_PCREL_TYPE SRC_PCREL_END
#define SRC_ENDIANNESS ENDIANNESS_LITTLE
#define SRC_INSN_ALIGN 1
#define JUMPTABLE_ADD JUMPTABLE_ADD_NONE
//#define JUMPTABLE_ADD JUMPTABLE_ADD_UNREACHABLE

#elif ARCH_SRC == ARCH_X64

#define SRC_CROSSTOOL_PATH I386_CROSSTOOL_PATH
#define SRC_LD_EXEC I386_LD_EXEC
#define SRC_AS_EXEC I386_AS_EXEC
#define SRC_PCREL_TYPE SRC_PCREL_END
#define SRC_ENDIANNESS ENDIANNESS_LITTLE
#define SRC_INSN_ALIGN 1
#define JUMPTABLE_ADD JUMPTABLE_ADD_NONE

#elif ARCH_SRC == ARCH_PPC

#define SRC_CROSSTOOL_PATH PPC_CROSSTOOL_PATH
#define SRC_LD_EXEC PPC_LD_EXEC
#define SRC_AS_EXEC PPC_AS_EXEC
#define SRC_PCREL_TYPE SRC_PCREL_START
#define SRC_ENDIANNESS ENDIANNESS_BIG
#define SRC_INSN_ALIGN 4
#define JUMPTABLE_ADD JUMPTABLE_ADD_UNREACHABLE
//#define JUMPTABLE_ADD JUMPTABLE_ADD_NONE

#elif ARCH_SRC == ARCH_ETFG

#define ERROR_STR "etfg-exec-not-used"
#define SRC_CROSSTOOL_PATH ERROR_STR
#define SRC_LD_EXEC ERROR_STR
#define SRC_AS_EXEC ERROR_STR
#define SRC_PCREL_TYPE SRC_PCREL_END
#define SRC_ENDIANNESS ENDIANNESS_LITTLE
#define SRC_INSN_ALIGN 1
#define JUMPTABLE_ADD JUMPTABLE_ADD_NONE
//#define JUMPTABLE_ADD JUMPTABLE_ADD_NONE

#endif

#if ARCH_DST == ARCH_I386

//#define DST_FALLBACK FALLBACK_ID
#define DST_ENDIANNESS ENDIANNESS_LITTLE

#elif ARCH_DST == ARCH_X64

#define DST_ENDIANNESS ENDIANNESS_LITTLE

#elif ARCH_DST == ARCH_PPC

//#define DST_FALLBACK FALLBACK_QEMU
#define DST_ENDIANNESS ENDIANNESS_BIG

#endif

#define FBGEN_BUILD_DIR BUILD_DIR "/../../../fbgen-build"
#define QEMUFB_INSTALL_PATH FBGEN_BUILD_DIR "/installs/" QEMUFB ".fb-install"
#define QEMUFB_PATH FBGEN_BUILD_DIR "/installs/" QEMUFB ".fb"
#define QEMUFB_EXEC QEMUFB_INSTALL_PATH "/bin/qemu-" SRC_NAME
#define GREP_EXEC "/bin/grep"

#define SMPBENCH_BUILD_DIR BUILD_DIR "/../../../smpbench-build"

#define I386_CROSSTOOL_PATH SMPBENCH_BUILD_DIR "/installs/crosstool-0.43-install/gcc-4.1.0-glibc-2.3.6.mod.i386/i686-unknown-linux-gnu"
//#define I386_LD_EXEC BUILD_DIR "/installs/binutils-2.21-install/bin/ld"
#define I386_LD_EXEC "/usr/bin/ld"
#define I386_AS_EXEC BUILD_DIR "/installs/binutils-2.21-install/bin/as"

#define PPC_CROSSTOOL_PATH SMPBENCH_BUILD_DIR "/installs/crosstool-0.43-install/gcc-4.1.0-glibc-2.3.6.mod.ppc/powerpc-405-linux-gnu"
#define PPC_LD_EXEC PPC_CROSSTOOL_PATH "/bin/powerpc-405-linux-gnu-ld"
#define PPC_AS_EXEC PPC_CROSSTOOL_PATH "/bin/powerpc-405-linux-gnu-as"

#if SRC_ENDIANNESS != ENDIANNESS_LITTLE
#define tswap16 bswap16
#define tswap32 bswap32
#define tswap64 bswap64
#define tswapl  bswap32
#define tswapq  bswap64
#else
#define tswap16(x) (x) 
#define tswap32(x) (x)
#define tswap64(x) (x)
#define tswapl(x)  (x)
#define tswapq(x)  (x)
#endif

#define CDECL 1

/*#define HBYTE_LEN 4
#define BYTE_LEN 8
#define WORD_LEN 16
#define DWORD_LEN 32
#define QWORD_LEN 64
#define DQWORD_LEN 128
#define QQWORD_LEN 256

#define BIT_LEN 1*/

#endif /* config-all.h */
