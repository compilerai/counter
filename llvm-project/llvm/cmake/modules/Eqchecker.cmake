find_package(Boost REQUIRED iostreams serialization)

set(PROFILE_FLAG "")
set(DEBUG_FLAG "-g")
set(OPT "-O3")
set(LINK_FLAG "-no-pie")
#set(LINK_FLAG "")
set(DEFINES "-DARCH_SRC=ARCH_ETFG -DARCH_DST=ARCH_I386 -DSRC_DST=\\\"etfg_i386\\\" -DSUPEROPT_DIR=\"\\\"${SUPEROPT_DIR}\\\"\"")
set(DEFINES "${DEFINES} -DELFIO_HPP") #to disable including /usr/include/elf.h
set(SUPEROPT_DIR "${LLVM_BINARY_DIR}/../../superopt")
set(SUPEROPT_RELEVANT_ABUILD "etfg_i386")
set(EQ_BINARY_DIR ${SUPEROPT_DIR}/build/${SUPEROPT_RELEVANT_ABUILD})
set(THIRD_PARTY_DIR "${SUPEROPT_DIR}/build/third_party")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${DEBUG_FLAG} ${PROFILE_FLAG} ${OPT} ${LINK_FLAG} ${DEFINES}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${DEBUG_FLAG} ${PROFILE_FLAG} ${OPT} ${LINK_FLAG} ${DEFINES}")

set(binutils_ver binutils-2.21)
set(binutils_dir ${THIRD_PARTY_DIR}/${binutils_ver})
set(binutils_install_dir ${binutils_dir}-install)
set(binutils_lib ${binutils_install_dir}/lib/libbfd.a ${binutils_install_dir}/lib/libiberty.a)

#set(z3_install /home/sbansal/z3-4.8.4.d6df51951f4c-x64-ubuntu-16.04)
set(z3_install ${THIRD_PARTY_DIR}/z3)
set(z3_lib ${z3_install}/usr/lib)
link_directories(${z3_lib})
link_directories(${binutils_install_dir}/lib)
link_directories(/usr/local/lib)

INCLUDE_DIRECTORIES(${LLVM_BINARY_DIR}/../../superopt/include)
INCLUDE_DIRECTORIES(${LLVM_BINARY_DIR}/../../superopt/lib)
INCLUDE_DIRECTORIES(${LLVM_BINARY_DIR}/../../superopt/lib/eq)
INCLUDE_DIRECTORIES(${LLVM_BINARY_DIR}/../../superopt/lib/expr)
INCLUDE_DIRECTORIES(${LLVM_BINARY_DIR}/../../superopt/lib/support)
INCLUDE_DIRECTORIES(${LLVM_BINARY_DIR}/../../superopt/build/${SUPEROPT_RELEVANT_ABUILD})

LINK_DIRECTORIES(${z3_lib})
LINK_DIRECTORIES(${EQ_BINARY_DIR})
set(BINLIBS_DIR ${LLVM_BINARY_DIR}/../../binlibs/etfg_i386)
set(SUPEROPT_LIBS ${BINLIBS_DIR}/libsuperopt.a equiv ${BINLIBS_DIR}/libfbgen.a eqchecker ${BINLIBS_DIR}/libsym_exec.a ${BINLIBS_DIR}/librewrite_lib.a tfg graph gsupport ${BINLIBS_DIR}/libi386.a ${BINLIBS_DIR}/libtransmap.a exec ${BINLIBS_DIR}/libvaltag.a ${BINLIBS_DIR}/libexpr.a ${BINLIBS_DIR}/libgas.a support cutils fpu ${BINLIBS_DIR}/libparser.a z3 -lgmp -lgmpxx ${Boost_LIBRARIES} -ldl ${binutils_lib} -lm -lz -lmagic -lboost_filesystem -lboost_system -lssl -lcrypto)
