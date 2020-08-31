include_directories(
  ${LLVM_MAIN_SRC_DIR}/lib/Target/X86
  ${LLVM_BINARY_DIR}/lib/Target/X86
  )

set(LLVM_LINK_COMPONENTS
  BitReader
  IRReader
  Core
  Support
  Analysis
  CodeGen
  )

find_package(Boost REQUIRED iostreams serialization)

#set(CMAKE_VERBOSE_MAKEFILE ON)
#set(SUPEROPT_DIR get_filename_component(ABSOLUTE, "${LLVM_BINARY_DIR}/../../superopt"))
#set(SUPEROPT_DIR "${LLVM_BINARY_DIR}/../../superopt")
#set(SUPEROPT_RELEVANT_ABUILD "etfg_i386")
#set(EQ_BINARY_DIR ${SUPEROPT_DIR}/build/${SUPEROPT_RELEVANT_ABUILD})
#set(THIRD_PARTY_DIR "${SUPEROPT_DIR}/build/third_party")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -no-pie -DARCH_SRC=ARCH_ETFG -DARCH_DST=ARCH_I386 -DSUPEROPT_DIR=\"\\\"${SUPEROPT_DIR}\\\"\"")
##set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -pg")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
#if (CMAKE_C_COMPILER MATCHES "clang")
#  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstandalone-debug")
#endif()
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -no-pie -DARCH_SRC=ARCH_ETFG -DARCH_DST=ARCH_I386 -DSUPEROPT_DIR=\"\\\"${SUPEROPT_DIR}\\\"\"")
##set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -pg")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
#if (CMAKE_CXX_COMPILER MATCHES "clang")
#  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstandalone-debug")
#endif()

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_USE_CXX11_ABI=0")

#set(z3_install /usr)
#link_directories(${z3_dir}/build)
#link_directories(${z3_install}/bin)
#link_directories(${binutils_install_dir}/lib)
#link_directories(/usr/local/lib)

#set(binutils_ver binutils-2.21)
#set(binutils_dir ${THIRD_PARTY_DIR}/${binutils_ver})
#set(binutils_install_dir ${binutils_dir}-install)
#set(binutils_lib ${binutils_install_dir}/lib/libbfd.a ${binutils_install_dir}/lib/libiberty.a)
#
#INCLUDE_DIRECTORIES(${LLVM_BINARY_DIR}/../../superopt/lib)
#INCLUDE_DIRECTORIES(${LLVM_BINARY_DIR}/../../superopt/lib/eq)
#INCLUDE_DIRECTORIES(${LLVM_BINARY_DIR}/../../superopt/lib/expr)
#INCLUDE_DIRECTORIES(${LLVM_BINARY_DIR}/../../superopt/lib/support)
#INCLUDE_DIRECTORIES(${LLVM_BINARY_DIR}/../../superopt/build/${SUPEROPT_RELEVANT_ABUILD})

INCLUDE_DIRECTORIES(../../lib/Analysis/Superopt)

#LINK_DIRECTORIES(${LLVM_BINARY_DIR}/../../superopt-build/installs/z3-0b6a836eb2695dae1834d7a0c3f4f86f20e17d4d/build/)
#LINK_DIRECTORIES(${z3_dir}/bin)
#LINK_DIRECTORIES(${z3_install}/bin)
#LINK_DIRECTORIES(${EQ_BINARY_DIR})
LINK_DIRECTORIES(${LLVM_BINARY_DIR}/lib)

add_llvm_tool(llvm2tfg
  main.cpp
  #sym_exec_llvm.cpp
  #state_llvm.cpp
  )

target_link_libraries(llvm2tfg PRIVATE ${SUPEROPT_LIBS})
#TARGET_LINK_LIBRARIES(llvm2tfg z3)
#TARGET_LINK_LIBRARIES(llvm2tfg ${Boost_LIBRARIES})
