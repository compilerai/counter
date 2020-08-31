cmake_minimum_required(VERSION 2.8)

project(superopt)

if(CMAKE_CONFIGURATION_TYPES)
  list(APPEND CMAKE_CONFIGURATION_TYPES eetfg_i386)
  list(APPEND CMAKE_CONFIGURATION_TYPES i386_i386)
  list(APPEND CMAKE_CONFIGURATION_TYPES ppc_i386)
  list(REMOVE_DUPLICATES CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES}" CACHE STRING
    "Add the configurations that we need"
    FORCE)
endif()

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

find_program(DISTCC_FOUND distcc)
if(DISTCC_FOUND)
  execute_process(COMMAND distcc --show-hosts OUTPUT_VARIABLE DISTCC_HOSTS)
  if(DISTCC_HOSTS)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE distcc)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK distcc)
  endif(DISTCC_HOSTS)
endif(DISTCC_FOUND)

# emit compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#message(FATAL_ERROR config = $<CONFIG>)
#if (${CONFIG} EQUAL etfg_i386)
#  set(SRC_ARCH etfg)
#elseif (${CONFIG} EQUAL ppc_i386)
#  set(SRC_ARCH ppc)
#elseif (${CONFIG} EQUAL i386_i386)
#  set(SRC_ARCH i386)
#else()
#  message(FATAL_ERROR "Unsupported configuration ${CONFIG}")
#endif()
#
#set(DST_ARCH i386)
#set(SRC_DST ${SRC_ARCH}_${DST_ARCH})
execute_process(COMMAND echo ${CMAKE_BUILD_TYPE} COMMAND grep -Eo "^[^_]+" COMMAND tr -d '[:space:]' OUTPUT_VARIABLE SRC_ARCH)
execute_process(COMMAND echo ${CMAKE_BUILD_TYPE} COMMAND grep -Eo "[^_]+$" COMMAND tr -d '[:space:]' OUTPUT_VARIABLE DST_ARCH)
set(SRC_DST_HYPHEN ${SRC_ARCH}-${DST_ARCH})

find_package(Boost REQUIRED COMPONENTS system iostreams serialization)
find_program(GIT git)

# sagemath linear algebra solver
#find_program(sagemath_exe sage)
#if(NOT sagemath_exe)
#    message(FATAL_ERROR "sagemath not found!")
#endif()

get_filename_component(SUPEROPT_PROJECT_DIR ${CMAKE_SOURCE_DIR}/.. ABSOLUTE)

set(BINLIBS_DIR ${SUPEROPT_PROJECT_DIR}/binlibs/${CMAKE_BUILD_TYPE})


if(DEFINED ENV{SUPEROPT_INSTALL_DIR})
  set(SUPEROPT_INSTALL_DIR $ENV{SUPEROPT_INSTALL_DIR})
else()
  set(SUPEROPT_INSTALL_DIR ${SUPEROPT_PROJECT_DIR}/usr/local)
endif()

if(DEFINED ENV{SUPEROPT_TARS_DIR})
  set(TARS_DIR $ENV{SUPEROPT_TARS_DIR})
else()
  get_filename_component(TARS_DIR ${SUPEROPT_PROJECT_DIR}/../tars ABSOLUTE)
endif()

set(CROSSTOOL crosstool-0.43)
set(SMPBENCH_BUILD_DIR ${SUPEROPT_PROJECT_DIR}/../smpbench-build)
#set(SUPEROPT_DIR "${CMAKE_SOURCE_DIR}")
set(CROSSTOOL_DIR ${SMPBENCH_BUILD_DIR}/installs/${CROSSTOOL})
set(CROSSTOOL_INSTALL ${CROSSTOOL_DIR}-install)
set(CROSSTOOL_PPC_BIN ${CROSSTOOL_INSTALL}/gcc-4.1.0-glibc-2.3.6.mod.ppc/powerpc-405-linux-gnu/bin)
set(CROSSTOOL_I386_BIN ${CROSSTOOL_INSTALL}/gcc-4.1.0-glibc-2.3.6.mod.i386/i686-unknown-linux-gnu/bin)
set(CROSSTOOL_I386_LIB ${CROSSTOOL_INSTALL}/gcc-4.1.0-glibc-2.3.6.mod.i386/i686-unknown-linux-gnu/lib)

set(OPT_LEVEL "-O3")
set(DEBUG "-g")
#set(DEFINES  "${DEFINES} -DSUPEROPT_DIR=\"\\\"${SUPEROPT_DIR}\\\"\"")

#set(PROFILE_FLAGS "-pg")
set(PROFILE_FLAGS "")
set(WARNING_FLAGS "-Wall -Wno-unused-function -Wno-unused-variable -Wno-unused-label -Wno-unused-parameter -Wno-unused-value -Wno-unused-but-set-variable -Wno-parentheses -Wno-unused-result -Wno-sign-compare -Wno-narrowing -Wno-unknown-warning-option -Wno-mismatched-tags -Wno-undefined-var-template -Wno-null-conversion")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARNING_FLAGS} -Wno-pointer-sign")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${PROFILE_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${DEFINES}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-omit-frame-pointer")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OPT_LEVEL}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${DEBUG}")
#if (CMAKE_C_COMPILER MATCHES "clang")
#  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstandalone-debug")
#endif()
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined")

message ("> cmake_cxx_compiler: ${CMAKE_CXX_COMPILER}.")
message ("> cmake_cxx_compiler_id: ${CMAKE_CXX_COMPILER_ID}.")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_USE_CXX11_ABI=0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${PROFILE_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${DEFINES}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OPT_LEVEL}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${DEBUG}")
#if (CMAKE_CXX_COMPILER MATCHES "clang")
#  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstandalone-debug")
#endif()
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always")
set(CMAKE_VERBOSE_MAKEFILE ON)

set(ETFG_I386_FLAGS "-DARCH_SRC=ARCH_ETFG -DARCH_DST=ARCH_I386 -DSRC_DST=\\\"etfg_i386\\\"")
set(I386_I386_FLAGS "-DARCH_SRC=ARCH_I386 -DARCH_DST=ARCH_I386 -DSRC_DST=\\\"i386_i386\\\"")
set(PPC_I386_FLAGS "-DARCH_SRC=ARCH_PPC -DARCH_DST=ARCH_I386 -DSRC_DST=\\\"ppc_i386\\\"")

set(CMAKE_C_FLAGS_ETFG_I386 "${CMAKE_C_FLAGS} ${ETFG_I386_FLAGS}")
set(CMAKE_C_FLAGS_I386_I386 "${CMAKE_C_FLAGS} ${I386_I386_FLAGS}")
set(CMAKE_C_FLAGS_PPC_I386 "${CMAKE_C_FLAGS} ${PPC_I386_FLAGS}")
set(CMAKE_CXX_FLAGS_ETFG_I386 "${CMAKE_CXX_FLAGS} ${ETFG_I386_FLAGS}")
set(CMAKE_CXX_FLAGS_I386_I386 "${CMAKE_CXX_FLAGS} ${I386_I386_FLAGS}")
set(CMAKE_CXX_FLAGS_PPC_I386 "${CMAKE_CXX_FLAGS} ${PPC_I386_FLAGS}")

set(LINKIN_OP_CFLAGS -O2 -m32 -fno-strict-aliasing -mpreferred-stack-boundary=2)
set(LINKIN_OP_CFLAGS ${LINKIN_OP_CFLAGS} -falign-functions=0 -fno-gcse -fno-stack-protector)
set(LINKIN_OP_CFLAGS ${LINKIN_OP_CFLAGS} -fno-reorder-blocks -fno-optimize-sibling-calls)


include(ProcessorCount)
ProcessorCount(cores)
set(third_party_dir ${CMAKE_BINARY_DIR}/../third_party)
file(MAKE_DIRECTORY ${third_party_dir})

#find_program(z3_exe z3)
#if(NOT z3_exe)
#    message(FATAL_ERROR "z3 not found")
#endif()
#
#set(z3_install /usr)
#set(yices_ver yices-2.5.4)

set(z3_ver z3-4.8.7)
set(z3_tarfile ${z3_ver}-x86_64.pkg.tar.xz)
set(z3_dir ${third_party_dir}/z3)
set(z3_exe ${z3_dir}/usr/bin/z3)
set(z3_lib ${z3_dir}/usr/lib)
set(z3_include ${z3_dir}/usr/include)

set(yices_ver yices2-45e38fc3ca40708453b8b8ea9e737fa09dd84ec1)
set(yices_dir ${third_party_dir}/${yices_ver})
set(yices_build_exe ${yices_dir}/build/x86_64-pc-linux-gnu-release/bin/yices_smt2)
set(yices_exe ${third_party_dir}/yices_smt2)

set(cvc4_ver cvc4-1.7)
set(cvc4_dir ${third_party_dir}/${cvc4_ver})
set(cvc4_build_exe ${cvc4_dir}/cvc4-1.7-x86_64-linux-opt)
set(cvc4_exe ${third_party_dir}/cvc4)

set(boolector_ver boolector-3.1.0)
set(boolector_dir ${third_party_dir}/${boolector_ver})
set(boolector_build_exe ${boolector_dir}/build/bin/boolector)
set(boolector_exe ${third_party_dir}/boolector)

add_custom_target(solvers ALL DEPENDS ${yices_exe} ${cvc4_exe} ${z3_exe})

set(elfio_ver elfio-3.2)
set(elfio_dir ${third_party_dir}/${elfio_ver})
set(elfio_hdr ${elfio_dir}/elfio/elfio.hpp)

set(binutils_ver binutils-2.21)
set(binutils_dir ${third_party_dir}/${binutils_ver})
set(binutils_install_dir ${binutils_dir}-install)
# we will generate static libs using a custom target
# let's define them first; later we will use custom command for generating them
set(binutils_libbfd_file ${binutils_install_dir}/lib/libbfd.a)
set(binutils_libiberty_file ${binutils_install_dir}/lib/libiberty.a)
# our custom target; depends on the files thus creating them when required
add_custom_target(binutils_target DEPENDS ${binutils_libbfd_file} ${binutils_libiberty_file})
# now declare the imported libs
add_library(binutils_libbfd STATIC IMPORTED GLOBAL)
set_target_properties(binutils_libbfd PROPERTIES IMPORTED_LOCATION ${binutils_libbfd_file})
add_library(binutils_libiberty STATIC IMPORTED GLOBAL)
set_target_properties(binutils_libiberty PROPERTIES IMPORTED_LOCATION ${binutils_libiberty_file})
# these will depend on our custom target
# Note that this is required only for forcing ordering
add_dependencies(binutils_libbfd binutils_target)
add_dependencies(binutils_libiberty binutils_target)

#add_dependencies(lib/expr/z3_solver.h ${z3_header})

#add_custom_command(${z3_install}/include/z++.h DEPENDS ${z3_exe_custom})
#add_dependencies(${z3_install}/include/z++.h ${z3_exe_custom})

set(libdwarf_ver libdwarf-code-561de2eef0667fc4d45)
set(libdwarf_dir ${third_party_dir}/${libdwarf_ver})
set(libdwarf_exe ${libdwarf_dir}/dwarfdump/dwarfdump)

set(freetds_ver freetds-1.00.108)
set(freetds_dir ${third_party_dir}/${freetds_ver})
set(freetds_install ${freetds_dir}-install)
set(freetds_lib ${freetds_install}/lib/libsybdb.so)

#set(tdspp_ver tdspp)
#set(tdspp_dir ${CMAKE_BINARY_DIR}/../third_party/${tdspp_ver})
#set(tdspp_install ${tdspp_dir}-install)
#set(tdspp_lib ${tdspp_install}/lib/libtds++.so)

#set(LINK_DIRECTORIES ${binutils_install_dir}/lib)
#set(TARGET_LINK_LIBRARIES bfd)

#set(ocaml_ver ocaml-4.02.3)
#set(ocaml_dir ${CMAKE_BINARY_DIR}/third_party/${ocaml_ver})
#set(ocaml_install_dir ${ocaml_dir}-install)
#set(ocamlc ${ocaml_install_dir}/bin/ocamlc)
#set(ocamlopt ${ocaml_install_dir}/bin/ocamlopt)
#set(ocamlopt_flags -g -p)
#set(ocaml_libs nums.cmxa unix.cmxa ${z3_dir}/build/api/ml/z3.cmxa)
#set(ocaml_ldflags -p -cc "${CPP}" -cclib -lm -cclib -lbfd -cclib -liberty -ccopt -pg -cclib -L${z3_dir}/build -cclib -lz3 -cclib -L${yices_mod_dir}/build/x86_64-unknown-linux-gnu-release/lib -cclib -lyices -cclib -lgmp -cclib -lstdc++ -cclib -lcamlidl -cclib -lboost_iostreams)

#execute_process(COMMAND
#  "${GIT}" describe --match=NeVeRmAtCh --always --abbrev=40 --dirty
#  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
#  OUTPUT_VARIABLE GIT_SHA1
#  ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

#execute_process(COMMAND
#  "${GIT}" log -1 --format=%ad --date=local
#  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
#  OUTPUT_VARIABLE GIT_DATE
#  ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

#execute_process(COMMAND
#  "${GIT}" rev-parse --abbrev-ref HEAD
#  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
#  OUTPUT_VARIABLE GIT_BRANCH
#  ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)

# copy these files to binary directory
configure_file(${CMAKE_BINARY_DIR}/../config-host.h.in ${CMAKE_BINARY_DIR}/config-host.h)
configure_file(lib/support/bv_solve_template.py bv_solve_template.py COPYONLY)
configure_file(lib/support/sage_driver.py sage_driver.py)

link_directories(${z3_dir}/usr/lib)
link_directories(/usr/lib)
link_directories(/usr/local/lib)
#link_directories(${yices_mod_dir}/build/x86_64-unknown-linux-gnu-release/lib)
#link_directories(${z3_dir}/build)

#link_directories(${binutils_install_dir}/lib)
include_directories(${binutils_install_dir}/include)
include_directories(${z3_dir}/usr/include)
#include_directories(${z3_dir}/src/api/c++)
include_directories(${elfio_dir})
include_directories(${CMAKE_BINARY_DIR})
include_directories(${freetds_install}/include)
include_directories(${z3_include})
#include_directories(${tdspp_install}/include)
link_directories(${freetds_install}/lib)
link_directories(${z3_lib})
#link_directories(${tdspp_install}/lib)

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

#BISON_TARGET(expr_parse_y lib/expr/expr_parse.ypp ${CMAKE_CURRENT_BINARY_DIR}/expr_parse_y.cpp COMPILE_FLAGS -d)
#FLEX_TARGET(expr_parse_l lib/expr/expr_parse.l  ${CMAKE_CURRENT_BINARY_DIR}/expr_parse_l.cpp COMPILE_FLAGS -Plts)
#ADD_FLEX_BISON_DEPENDENCY(expr_parse_l expr_parse_y)

#BISON_TARGET(expr_parse_y lib/expr/expr_parse_new.ypp ${CMAKE_CURRENT_BINARY_DIR}/expr_parse_y.cpp COMPILE_FLAGS -d)
#FLEX_TARGET(expr_parse_l lib/expr/expr_parse_new.l ${CMAKE_CURRENT_BINARY_DIR}/expr_parse_l.cpp COMPILE_FLAGS)
#ADD_FLEX_BISON_DEPENDENCY(expr_parse_l expr_parse_y)

#BISON_TARGET(yices_model_y lib/parser/yices_model.ypp ${CMAKE_CURRENT_BINARY_DIR}/yices_model_y.cpp COMPILE_FLAGS -d)
#FLEX_TARGET(yices_model_l lib/parser/yices_model.l  ${CMAKE_CURRENT_BINARY_DIR}/yices_model_l.cpp COMPILE_FLAGS -Pyc)
#ADD_FLEX_BISON_DEPENDENCY(yices_model_l yices_model_y)

#BISON_TARGET(z3_model_y lib/parser/z3_model.ypp ${CMAKE_CURRENT_BINARY_DIR}/z3_model_y.cpp COMPILE_FLAGS -d)
#FLEX_TARGET(z3_model_l lib/parser/z3_model.l ${CMAKE_CURRENT_BINARY_DIR}/z3_model_l.cpp COMPILE_FLAGS)
#ADD_FLEX_BISON_DEPENDENCY(z3_model_l z3_model_y)

#BISON_TARGET(cvc4_model_y lib/parser/cvc4_model.ypp ${CMAKE_CURRENT_BINARY_DIR}/cvc4_model_y.cpp)
#FLEX_TARGET(cvc4_model_l lib/parser/cvc4_model.l ${CMAKE_CURRENT_BINARY_DIR}/cvc4_model_l.cpp COMPILE_FLAGS)
#ADD_FLEX_BISON_DEPENDENCY(cvc4_model_l cvc4_model_y)

#BISON_TARGET(boolector_model_y lib/parser/boolector_model.ypp ${CMAKE_CURRENT_BINARY_DIR}/boolector_model_y.cpp)
#FLEX_TARGET(boolector_model_l lib/parser/boolector_model.l ${CMAKE_CURRENT_BINARY_DIR}/boolector_model_l.cpp COMPILE_FLAGS)
#ADD_FLEX_BISON_DEPENDENCY(boolector_model_l boolector_model_y)

BISON_TARGET(edge_guard_y lib/parser/edge_guard.ypp ${CMAKE_CURRENT_BINARY_DIR}/edge_guard_y.cpp COMPILE_FLAGS -d)
FLEX_TARGET(edge_guard_l lib/parser/edge_guard.l  ${CMAKE_CURRENT_BINARY_DIR}/edge_guard_l.cpp COMPILE_FLAGS -Peg)
ADD_FLEX_BISON_DEPENDENCY(edge_guard_l edge_guard_y)

BISON_TARGET(cg_edge_composition_y lib/eq/cg_edge_composition.ypp ${CMAKE_CURRENT_BINARY_DIR}/cg_edge_composition_y.cpp COMPILE_FLAGS -d)
FLEX_TARGET(cg_edge_composition_l lib/eq/cg_edge_composition.l  ${CMAKE_CURRENT_BINARY_DIR}/cg_edge_composition_l.cpp COMPILE_FLAGS -Pcg)
ADD_FLEX_BISON_DEPENDENCY(cg_edge_composition_l cg_edge_composition_y)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_SOURCE_DIR}/lib)
include_directories(${CMAKE_SOURCE_DIR}/include)

add_library(fpu STATIC
              lib/fpu/softfloat-native.c
)

add_library(cutils STATIC
                 lib/cutils/addr_range.cpp
                 lib/cutils/graph_node.cpp
                 lib/cutils/imap_elem.cpp
                 lib/cutils/interval_set.cpp
                 lib/cutils/pc_elem.cpp
                 lib/cutils/string_elem.cpp
                 lib/cutils/chash.cpp
                 lib/cutils/clist.cpp
                 lib/cutils/rbtree.cpp
                 lib/cutils/large_int.cpp
                 lib/cutils/enumerator.cpp
)

add_library(fbgen STATIC
                  lib/fbgen/ocaml_output_parser.cpp
)


add_library(eqchecker STATIC
                  lib/eqgen/dst_tfg.cpp
                  lib/eq/corr_graph.cpp
                  lib/eq/correlate.cpp
                  lib/eq/function_cg_map.cpp
                  lib/eq/cg_with_inductive_preds.cpp
                  lib/eq/cg_with_relocatable_memlabels.cpp
                  lib/eq/cg_with_safety.cpp
                  lib/eq/cg_with_asm_annotation.cpp
                  lib/eq/cg_with_dst_ml_check.cpp
                  lib/eq/unsafe_cond.cpp
                  lib/eq/eqcheck.cpp
                  lib/eq/quota.cpp
                  lib/eq/cg_suffixpath_dfa.cpp
                  lib/ptfg/llptfg.cpp
                  lib/ptfg/function_signature.cpp
)

add_library(gsupport STATIC
                  lib/gsupport/corr_graph_edge.cpp
                  lib/gsupport/edge_with_cond.cpp
                  lib/gsupport/tfg_edge.cpp
                  lib/gsupport/itfg_edge.cpp
                  lib/gsupport/itfg_ec.cpp
                  lib/gsupport/suffixpath.cpp
                  lib/gsupport/rodata_map.cpp
                  lib/gsupport/parse_edge_composition.cpp
                  ${BISON_edge_guard_y_OUTPUTS}
                  ${FLEX_edge_guard_l_OUTPUTS}
                  ${BISON_cg_edge_composition_y_OUTPUTS}
                  ${FLEX_cg_edge_composition_l_OUTPUTS}
)

add_library(tfg STATIC
                  lib/rewrite/nextpc_function_name_map.cpp
                  lib/rewrite/symbol_or_section_id.cpp
                  lib/tfg/tfg.cpp
                  lib/tfg/tfg_llvm.cpp
                  lib/tfg/tfg_asm.cpp
                  lib/tfg/tfg_rewrite.cpp
                  lib/tfg/precond.cpp
                  lib/tfg/predicate.cpp
                  lib/tfg/predicate_set.cpp
                  lib/tfg/pred_avail_exprs_dfa.cpp
                  lib/tfg/avail_exprs.cpp
                  lib/tfg/avail_exprs_dfa.cpp
                  lib/tfg/edge_guard.cpp
                  lib/tfg/suffixpath_dfa.cpp
                  lib/tfg/parse_input_eq_file.cpp
)

add_library(graph STATIC
                  lib/gsupport/gsupport_cache.cpp
                  lib/graph/graph_with_predicates.cpp
                  lib/graph/graph_with_simplified_assets.cpp
                  lib/graph/graph_with_locs.cpp
                  lib/graph/graph_loc_id.cpp
                  lib/graph/graph_with_aliasing.cpp
                  lib/graph/graph_with_execution.cpp
                  lib/graph/graph_with_proofs.cpp
                  lib/graph/graph_with_ce.cpp
                  lib/graph/pc.cpp
                  lib/graph/prove.cpp
                  lib/graph/query_decomposition.cpp
                  lib/graph/graph_symbol.cpp
                  lib/graph/graph_cp_location.cpp
                  lib/graph/lr_map.cpp
                  lib/graph/lr_status.cpp
                  lib/graph/delta.cpp
                  lib/graph/callee_summary.cpp
                  lib/graph/point_set_helper.cpp
                  lib/graph/expr_group.cpp
                  lib/graph/point.cpp
                  lib/graph/eqclasses.cpp
                  lib/graph/locset.cpp
                  lib/graph/nodece_set.cpp
                  lib/graph/invariant_state.cpp
                  lib/graph/invariant_state_eqclass.cpp
                  lib/graph/invariant_state_eqclass_arr.cpp
                  lib/graph/memlabel_assertions.cpp
)

add_library(equiv STATIC
                  ${CMAKE_BINARY_DIR}/superoptdb_rpc.h
                  lib/equiv/equiv.cpp
                  lib/equiv/bequiv.cpp
                  lib/equiv/jtable.cpp
                  lib/equiv/jtable_gencode.cpp
)

add_library(exec STATIC
                  lib/exec/state.cpp
)

add_library(LockstepDbg STATIC
    lib/lockstep/instrumented_functions.cpp
    lib/lockstep/Lockstep.cpp
    lib/lockstep/Lockstep_helper.cpp
    lib/graph/pc.cpp
    lib/support/debug.cpp
    lib/support/dyn_debug.cpp
    lib/support/utils32.cpp
)

add_library(mymalloc STATIC
    lib/mymalloc/my_malloc.cpp
    lib/mymalloc/memlib.cpp
    lib/mymalloc/myutils.cpp
)

SET_TARGET_PROPERTIES(mymalloc PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32 -lssl")
SET_TARGET_PROPERTIES(LockstepDbg PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32 -lssl")


add_library(support STATIC
                  lib/support/timers.cpp
                  lib/support/log.cpp
                  lib/support/mytimer.cpp
                  lib/support/sig_handling.cpp
                  lib/support/cl.cpp
                  lib/support/utils32.cpp
                  lib/support/utils.cpp
                  lib/support/crypto.cpp
                  lib/support/mybitset.cpp
                  lib/support/command.cpp
                  lib/support/debug.cpp
                  lib/support/dyn_debug.cpp
                  lib/support/stopwatch.cpp
                  lib/support/c_utils.cpp
                  lib/support/stacktrace.cpp
                  lib/support/cmd_helper.cpp
                  lib/support/bv_const.cpp
                  lib/support/bv_const_expr.cpp
                  lib/support/linear_solver.cpp
                  lib/support/bv_solve.cpp
                  lib/support/sage_interface.cpp
                  lib/support/query_comment.cpp
                  lib/support/scatter_gather.cpp
                  lib/support/backtracker.cpp
                  lib/support/string_ref.cpp
                  lib/support/serialize.cpp
                  lib/support/bv_const_ref.cpp
)

#set(ml_srcs0 utils.ml log.ml)
#set(ml_srcs1 smt.ml smtsolver.ml readarg.ml transmap.ml regset.ml common.ml i386.ml ppc.ml)
#set(ml_srcs2 equiv.ml)
#string(REGEX REPLACE "([^;]+)(;|$)" "${CMAKE_BINARY_DIR}/ml/\\1\\2" ml_srcs0_fp "${ml_srcs0}")
#string(REGEX REPLACE "([^;]+)(;|$)" "${CMAKE_BINARY_DIR}/ml/\\1\\2" ml_srcs1_fp "${ml_srcs1}")
#string(REGEX REPLACE "([^;]+)(;|$)" "${CMAKE_BINARY_DIR}/ml/\\1\\2" ml_srcs2_fp "${ml_srcs2}")
#set(ml_srcs_all ${ml_srcs0_fp} ${CMAKE_BINARY_DIR}/ml/consts.ml ${CMAKE_BINARY_DIR}/ml/libsmt.ml ${ml_srcs1_fp} ${CMAKE_BINARY_DIR}/ml/src.ml ${ml_srcs2_fp}
#set(ml_includes -I${ocaml_install}/lib/ocaml -I${z3_dir}/build/api/ml -I${CMAKE_BINARY_DIR}/ml)
#
##string(REGEX REPLACE "(.+).ml(;|$)" "\\1.cmi" ml_cmis "${ml_srcs_all}")
##string(REGEX REPLACE "(.+).ml(;|$)" "\\1.cmx" ml_objs "${ml_srcs_all}")
#
#set(ml_cmis)
#set(ml_cmxs)
#foreach(ml_src, ${ml_srcs_all})
#  set(cmi_filename "${ml_src}.cmi")
#  add_custom_command(
#    output "${cmi_filename}"
#    command ${ocamlopt} ${ocamlopt_flags} ${ml_includes} -c ${ml_src}
#    DEPENDS ${ml_src}
#  )
#  set(ml_cmis ${ml_cmis} ${cmi_filename})
#endforeach(ml_src)


set(dblibs ${BINLIBS_DIR}/libdb.a sybdb)
set(libs ${BINLIBS_DIR}/libsuperopt.a equiv ${BINLIBS_DIR}/libfbgen.a eqchecker ${BINLIBS_DIR}/libsym_exec.a ${BINLIBS_DIR}/librewrite_lib.a tfg graph gsupport ${BINLIBS_DIR}/libi386.a ${BINLIBS_DIR}/libtransmap.a exec ${BINLIBS_DIR}/libvaltag.a ${BINLIBS_DIR}/libexpr.a ${BINLIBS_DIR}/libgas.a ${dblibs} support cutils fpu ${BINLIBS_DIR}/libparser.a z3 -lgmp -lgmpxx ${Boost_LIBRARIES} -ldl binutils_libbfd binutils_libiberty -lm -lpthread -lz -lmagic -lboost_filesystem -lssl -lcrypto)

add_executable(eq tools/eq_main.cpp)
target_link_libraries(eq ${libs})

add_executable(qcc-codegen tools/qcc-codegen.cpp)
target_link_libraries(qcc-codegen ${libs})

#add_executable(cg_check_safety tools/cg_check_safety.cpp)
#target_link_libraries(cg_check_safety ${libs})

add_executable(dst_insn_disassemble tools/dst_insn_disassemble.cpp)
target_link_libraries(dst_insn_disassemble ${libs})

add_executable(gen_unified_harvest_log tools/gen_unified_harvest_log.cpp)
target_link_libraries(gen_unified_harvest_log ${libs})

add_executable(src_iseq_get_usedef tools/src_iseq_get_usedef.cpp)
target_link_libraries(src_iseq_get_usedef ${libs})

add_executable(dst_insn_get_usedef tools/dst_insn_get_usedef.cpp)
target_link_libraries(dst_insn_get_usedef ${libs})

add_executable(dst_infer_regcons_from_assembly tools/dst_infer_regcons_from_assembly.cpp)
target_link_libraries(dst_infer_regcons_from_assembly ${libs})

add_executable(src_iseq_compute_typestate tools/src_iseq_compute_typestate.cpp)
target_link_libraries(src_iseq_compute_typestate ${libs})

add_executable(src_iseq_compute_fingerprint tools/src_iseq_compute_fingerprint.cpp)
target_link_libraries(src_iseq_compute_fingerprint ${libs})

add_executable(enum_candidate_make_itables tools/enum_candidate_make_itables.cpp)
target_link_libraries(enum_candidate_make_itables ${libs})

add_executable(src_iseq_gen_rand_states tools/src_iseq_gen_rand_states.cpp)
target_link_libraries(src_iseq_gen_rand_states ${libs})

add_executable(itables_prune_using_typestate tools/itables_prune_using_typestate.cpp)
target_link_libraries(itables_prune_using_typestate ${libs})

#add_executable(tfg_get_type_constraints tools/tfg_get_type_constraints.cpp)
#target_link_libraries(tfg_get_type_constraints ${libs})

#add_executable(tfg_get_usedef tools/tfg_get_usedef.cpp)
#target_link_libraries(tfg_get_usedef ${libs})

#add_executable(src_insn_get_tfg tools/src_insn_get_tfg.cpp)
#target_link_libraries(src_insn_get_tfg ${libs})

add_executable(peep_enumerate_transmaps tools/peep_enumerate_transmaps.cpp)
target_link_libraries(peep_enumerate_transmaps ${libs})

add_executable(peep_translate tools/peep_translate.cpp)
target_link_libraries(peep_translate ${libs})

#add_executable(eq-check-proof tools/eq_check_proof.cpp)
#target_link_libraries(eq-check-proof ${libs})

add_executable(prove tools/prove_main.cpp)
target_link_libraries(prove ${libs})

add_executable(simplify tools/simplify_main.cpp)
target_link_libraries(simplify ${libs})

#add_executable(simplify_using_sprel_pair_and_memlabel_maps tools/simplify_using_sprel_pair_and_memlabel_maps_main.cpp)
#target_link_libraries(simplify_using_sprel_pair_and_memlabel_maps ${libs})

#add_executable(pred_canonicalizer tools/pred_canonicalizer.cpp)
#target_link_libraries(pred_canonicalizer ${libs})

add_executable(bv_solver tools/bv_solver.cpp)
target_link_libraries(bv_solver ${libs})

add_executable(mybitset_tester tools/mybitset_tester.cpp)
target_link_libraries(mybitset_tester ${libs})

add_executable(scc_tester tools/scc_tester.cpp)
target_link_libraries(scc_tester ${libs})

#add_executable(is_expr_equal_using_lhs_set tools/is_expr_equal_using_lhs_set_main.cpp)
#target_link_libraries(is_expr_equal_using_lhs_set ${libs})

add_executable(decide_hoare_triple tools/decide_hoare_triple.cpp)
target_link_libraries(decide_hoare_triple ${libs})

add_executable(update_invariant_state_over_edge tools/update_invariant_state_over_edge.cpp)
target_link_libraries(update_invariant_state_over_edge ${libs})

add_executable(prove_using_local_sprel_expr_guesses tools/prove_using_local_sprel_expr_guesses_main.cpp)
target_link_libraries(prove_using_local_sprel_expr_guesses ${libs})

add_executable(ptfg2llvm tools/ptfg2llvm.cpp)
target_link_libraries(ptfg2llvm ${libs})

add_executable(transmap_translate tools/transmap_translate.cpp)
target_link_libraries(transmap_translate ${libs})

add_executable(smt_helper_process lib/support/smt_helper_process.cpp)
target_link_libraries(smt_helper_process ${libs})

add_executable(cmd_helper_process lib/support/cmd_helper_process.cpp)
target_link_libraries(cmd_helper_process ${libs})

add_executable(harvest tools/harvest_main.cpp)
target_link_libraries(harvest ${libs})

add_executable(rewrite tools/rewrite_main.cpp)
target_link_libraries(rewrite ${libs})

add_executable(codegen tools/codegen_main.cpp etfg-i386.ptab)
target_link_libraries(codegen ${libs})
add_dependencies(codegen mymalloc)

add_executable(debug_gen tools/debug_gen.cpp)
target_link_libraries(debug_gen ${libs})
add_dependencies(debug_gen LockstepDbg)
add_dependencies(debug_gen mymalloc)

#add_executable(h2p tools/h2p_main.c)
#target_link_libraries(h2p ${libs} ${binutils_lib})

add_executable(eqgen tools/eqgen_main.cpp)
target_link_libraries(eqgen ${libs} ${binutils_lib})

add_executable(peeptab_verify tools/peeptab_verify.cpp)
target_link_libraries(peeptab_verify ${libs} ${binutils_lib})

add_executable(typecheck tools/typecheck.cpp)
target_link_libraries(typecheck ${libs} ${binutils_lib})

#add_executable(peepgen tools/peepgen_main.cpp)
#target_link_libraries(peepgen ${libs} ${binutils_lib})

add_executable(client tools/client.cpp)
target_link_libraries(client ${libs} ${binutils_lib})

add_executable(superoptdb_server lib/server/superoptdb_server.cpp ${CMAKE_BINARY_DIR}/superoptdb_rpc_svc.cpp)
target_link_libraries(superoptdb_server ${libs} ${binutils_lib})

add_executable(usedef_init tools/usedef_init.cpp)
target_link_libraries(usedef_init ${libs} ${binutils_lib})

add_executable(enumerate_for_src_iseq_using_itable tools/enumerate_for_src_iseq_using_itable.cpp)
target_link_libraries(enumerate_for_src_iseq_using_itable ${libs} ${binutils_lib})

add_executable(typestates_make_non_poly_copyable tools/typestates_make_non_poly_copyable.cpp)
target_link_libraries(typestates_make_non_poly_copyable ${libs} ${binutils_lib})

add_executable(dst_iseq_assemble tools/dst_iseq_assemble.cpp)
target_link_libraries(dst_iseq_assemble ${libs} ${binutils_lib})

add_executable(jtable_from_itable tools/jtable_from_itable.cpp)
target_link_libraries(jtable_from_itable ${libs} ${binutils_lib})

add_executable(yices_parse tools/yices_parser.cpp)
target_link_libraries(yices_parse ${libs} ${binutils_lib})

add_executable(z3_parse tools/z3_parser.cpp)
target_link_libraries(z3_parse ${libs} ${binutils_lib})

add_executable(cvc4_parse tools/cvc4_parser.cpp)
target_link_libraries(cvc4_parse ${libs} ${binutils_lib})

add_executable(boolector_parse tools/boolector_parser.cpp)
target_link_libraries(boolector_parse ${libs} ${binutils_lib})

add_executable(expr_parse tools/expr_parser.cpp)
target_link_libraries(expr_parse ${libs} ${binutils_lib})

add_executable(graph_edge_composition_parser tools/graph_edge_composition_parser.cpp)
target_link_libraries(graph_edge_composition_parser ${libs} ${binutils_lib})

add_executable(dbclear tools/dbclear.cpp)
target_link_libraries(dbclear ${libs})

add_executable(db_pick_itable_for_enum_candidate tools/db_pick_itable_for_enum_candidate.cpp)
target_link_libraries(db_pick_itable_for_enum_candidate ${libs})

add_executable(db_create_schema tools/db_create_schema.cpp)
target_link_libraries(db_create_schema ${libs})

add_executable(db_populate_enum_candidates tools/db_populate_enum_candidates.cpp)
target_link_libraries(db_populate_enum_candidates ${libs})

add_executable(db_download_peeptab tools/db_download_peeptab.cpp)
target_link_libraries(db_download_peeptab ${libs})

add_executable(db_add_peep_entry tools/db_add_peep_entry.cpp)
target_link_libraries(db_add_peep_entry ${libs})

add_executable(db_update_itable_enum_state_for_enum_candidate tools/db_update_itable_enum_state_for_enum_candidate.cpp)
target_link_libraries(db_update_itable_enum_state_for_enum_candidate ${libs})

add_executable(db_populate_itables_for_enum_candidate tools/db_populate_itables_for_enum_candidate.cpp)
target_link_libraries(db_populate_itables_for_enum_candidate ${libs})


add_executable(db_pick_enum_candidate tools/db_pick_enum_candidate.cpp)
target_link_libraries(db_pick_enum_candidate ${libs})

add_executable(gen_enum_candidates_from_harvest_out tools/gen_enum_candidates_from_harvest_out.cpp)
target_link_libraries(gen_enum_candidates_from_harvest_out ${libs})

add_executable(etfg_rewrite tools/etfg_rewrite.cpp)
target_link_libraries(etfg_rewrite ${libs})

add_executable(alloc_constraints_solve tools/alloc_constraints_solve.cpp)
target_link_libraries(alloc_constraints_solve ${libs})

add_executable(tfg2dot tools/tfg2dot.cpp)
target_link_libraries(tfg2dot ${libs})

add_executable(etfg2dot tools/etfg2dot.cpp)
target_link_libraries(etfg2dot ${libs})

add_executable(tfg_split_memory_and_propagate_sprels tools/tfg_split_memory_and_propagate_sprels.cpp)
target_link_libraries(tfg_split_memory_and_propagate_sprels ${libs})

add_executable(etfg_split_memory_and_propagate_sprels tools/etfg_split_memory_and_propagate_sprels.cpp)
target_link_libraries(etfg_split_memory_and_propagate_sprels ${libs})

add_executable(annotate_assembly_using_proof_file tools/annotate_assembly_using_proof_file.cpp)
target_link_libraries(annotate_assembly_using_proof_file ${libs})

add_executable(prove_queries_diff tools/prove_queries_diff_main.cpp)
target_link_libraries(prove_queries_diff ${libs})

add_executable(parse_edge_composition tools/parse_edge_composition_main.cpp)
target_link_libraries(parse_edge_composition ${libs})

add_custom_command(
	#OUTPUT ${z3_exe_custom}
  OUTPUT ${z3_exe}
  COMMAND [ ! -s ${z3_exe} ] && (mkdir -p ${z3_dir} && cd ${z3_dir} && tar xf ${TARS_DIR}/${z3_tarfile}) || /bin/true
  DEPENDS ${TARS_DIR}/${z3_tarfile}
  )

add_custom_command(
  OUTPUT etfg-i386.ptab
  COMMAND perl -I ${CMAKE_SOURCE_DIR}/utils ${CMAKE_SOURCE_DIR}/utils/mk_etfg_i386_ptab.pl
  DEPENDS ${CMAKE_SOURCE_DIR}/utils/mk_etfg_i386_ptab.pl ${CMAKE_SOURCE_DIR}/utils/etfg-i386.cmpl_insns.ptab ${CMAKE_SOURCE_DIR}/utils/etfg-i386.bconds.ptab  ${CMAKE_SOURCE_DIR}/utils/etfg-i386.inverted_bconds.ptab ${CMAKE_SOURCE_DIR}/utils/etfg-i386.other.ptab ${CMAKE_SOURCE_DIR}/utils/etfg-i386.fcalls.ptab
)

add_custom_command(
  OUTPUT ${yices_build_exe}
  #COMMAND rm -rf ${yices_dir}
  COMMAND cd ${third_party_dir} && unzip -o ${TARS_DIR}/${yices_ver}.zip
  COMMAND cd ${yices_dir} && autoconf && ./configure && make -j${cores}
  DEPENDS ${TARS_DIR}/${yices_ver}.zip
)

add_custom_command(
  OUTPUT ${yices_exe}
  #COMMAND rm -rf ${yices_dir}
  COMMAND cp ${yices_build_exe} ${yices_exe}
  DEPENDS ${yices_build_exe}
)

add_custom_command(
  OUTPUT ${cvc4_build_exe}
  #COMMAND rm -rf ${cvc4_dir}
  COMMAND cd ${third_party_dir} && mkdir -p ${cvc4_dir} && tar xzf ${TARS_DIR}/${cvc4_ver}.tgz -C ${cvc4_dir}
  DEPENDS ${TARS_DIR}/${cvc4_ver}.tgz
)

add_custom_command(
  OUTPUT ${cvc4_exe}
  COMMAND cp ${cvc4_build_exe} ${cvc4_exe}
  DEPENDS ${cvc4_build_exe}
)

add_custom_command(
  OUTPUT ${boolector_build_exe}
  #COMMAND rm -rf ${boolector_dir}
  COMMAND cd ${third_party_dir} && tar xzf ${TARS_DIR}/${boolector_ver}.tar.gz
  COMMAND cd ${boolector_dir} && ./contrib/setup-lingeling.sh
  COMMAND cd ${boolector_dir} && ./contrib/setup-btor2tools.sh
  COMMAND cd ${boolector_dir} && ./configure.sh && cd build && make -j${cores}
  DEPENDS ${TARS_DIR}/${boolector_ver}.tar.gz
)

add_custom_command(
  OUTPUT ${boolector_exe}
  COMMAND cp ${boolector_build_exe} ${boolector_exe}
  DEPENDS ${boolector_build_exe}
)

add_custom_command(
  OUTPUT ${elfio_hdr}
  COMMAND rm -rf ${elfio_dir}
  #COMMAND cd ${third_party_dir}  && tar xzf ${CMAKE_SOURCE_DIR}/../tars/${elfio_ver}.tar.gz
  COMMAND cd ${third_party_dir}  && unzip ${TARS_DIR}/${elfio_ver}.zip
  COMMAND cd ${third_party_dir}/${elfio_ver} && export CC="gcc" && export CXX="g++" && export AS="as" && ./configure && make -j${cores} && touch ${elfio_hdr}
  DEPENDS ${TARS_DIR}/${elfio_ver}.zip
)

add_custom_command(
  OUTPUT ${binutils_libbfd_file} ${binutils_libiberty_file}
  COMMAND rm -rf ${binutils_dir} && cd ${third_party_dir} && tar xjf ${TARS_DIR}/${binutils_ver}.tar.bz2 && cd ${binutils_dir} && export CC="gcc" && export CXX="g++" && export AS="as" && ./configure --prefix=${binutils_install_dir} --build=i386-pc-gnu && make CFLAGS='-Wno-error -g' -j${cores} && make install
  DEPENDS ${TARS_DIR}/${binutils_ver}.tar.bz2
)

add_custom_command(
  OUTPUT ${libdwarf_exe}
  COMMAND rm -rf ${libdwarf_dir} && cd ${third_party_dir} && tar xjf ${TARS_DIR}/${libdwarf_ver}.tbz2 && cd ${libdwarf_dir} && ./configure && make -j${cores}
)

add_custom_command(
  OUTPUT ${freetds_lib}
  COMMAND rm -rf ${freetds_dir} && cd ${third_party_dir} && tar xf ${TARS_DIR}/${freetds_ver}.tar.gz && cd ${freetds_dir} && ./configure --prefix ${freetds_install} && make -j${cores} && make install
  DEPENDS ${TARS_DIR}/${freetds_ver}.tar.gz
)

#add_custom_command(
#  OUTPUT ${tdspp_lib}
#  COMMAND rm -rf ${tdspp_lib} && cd ${third_party_dir} && tar xf ${CMAKE_SOURCE_DIR}/../tars/${tdspp_ver}.tbz2 && cd ${tdspp_dir} && make INCS="-I${freetds_install}/include/" LIBS='-L${freetds_install}/lib -lct -shared -ldl' && make install PREFIX='${tdspp_install}'
#  DEPENDS ${freetds_lib}
#)

function(from_hex_to_dec in out)
  set(temp_dir "${CMAKE_BINARY_DIR}/generated")
  set(temp_file "${temp_dir}/hex_to_dec.cpp")
  file(
      WRITE
      "${temp_file}"
      "#include <iostream>\n"
      "int main() {"
      "  std::cout << int(VALUE);"
      "}"
  )

  try_run(
      run_result
      compile_result
      "${temp_dir}"
      "${temp_file}"
      COMPILE_DEFINITIONS
      "-DVALUE=0x${in}"
      RUN_OUTPUT_VARIABLE run_output
  )

  if(NOT compile_result)
    message(FATAL_ERROR "Compilation failed: in = ${in}; output:\n${compile_output}")
  endif()

  if(NOT run_result EQUAL 0)
    message(FATAL_ERROR "Run failed")
  endif()

  set("${out}" "${run_output}" PARENT_SCOPE)
endfunction()

execute_process(COMMAND ${CMAKE_COMMAND} -E echo ${SRC_DST_HYPHEN} COMMAND md5sum COMMAND grep -Eo "^...." OUTPUT_VARIABLE PSEUDORAND1)
string(STRIP ${PSEUDORAND1} PSEUDORAND2)
execute_process(COMMAND perl -le "print hex(\"2000${PSEUDORAND2}\");" OUTPUT_VARIABLE PSEUDORAND)
from_hex_to_dec("${PSEUDORAND2}" PSEUDORAND)

add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/superoptdb_rpc.h ${CMAKE_BINARY_DIR}/superoptdb_rpc_xdr.c ${CMAKE_BINARY_DIR}/superoptdb_rpc_svc.cpp ${CMAKE_BINARY_DIR}/superoptdb_rpc_clnt.c
  COMMAND echo "pseudorand = " ${PSEUDORAND}
  COMMAND echo "pseudorand2 = " ${PSEUDORAND2}
  COMMAND sed "s/XXX_PROGNUM_XXX/${PSEUDORAND}/g" ${CMAKE_SOURCE_DIR}/lib/superopt/superoptdb_rpc.x > ${CMAKE_BINARY_DIR}/superoptdb_rpc.x.S
  COMMAND gcc -P -E ${CMAKE_BINARY_DIR}/superoptdb_rpc.x.S > ${CMAKE_BINARY_DIR}/superoptdb_rpc.x
  COMMAND rpcgen ${CMAKE_BINARY_DIR}/superoptdb_rpc.x
  COMMAND mv ${CMAKE_BINARY_DIR}/superoptdb_rpc_svc.c ${CMAKE_BINARY_DIR}/superoptdb_rpc_svc.cpp
)

add_custom_target(
  cscope.files ALL
  COMMAND find ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/tools -name \"*.c\" > ${CMAKE_SOURCE_DIR}/cscope.files
  COMMAND find ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/tools -name \"*.h\" >> ${CMAKE_SOURCE_DIR}/cscope.files
  COMMAND find ${CMAKE_SOURCE_DIR}/lib ${CMAKE_SOURCE_DIR}/tools -name \"*.cpp\" >> ${CMAKE_SOURCE_DIR}/cscope.files
)

#add_custom_target(
#	linkin_main.o ALL
#	COMMAND ${CROSSTOOL_I386_BIN}/i686-unknown-linux-gnu-gcc -m32 ${LINKIN_OP_CFLAGS} -I${CMAKE_CURRENT_BINARY_DIR} -DSRC=ARCH_PPC -DDST=ARCH_I386 -I${CMAKE_SOURCE_DIR}/lib -c -o linkin_main.o ${CMAKE_SOURCE_DIR}/lib/linker/linkin_main.c
#)
#
#add_custom_target(
#	linkin_syscall.o ALL
#	COMMAND ${CROSSTOOL_I386_BIN}/i686-unknown-linux-gnu-gcc -m32 ${LINKIN_OP_CFLAGS} -I${CMAKE_CURRENT_BINARY_DIR} -DSRC=ARCH_PPC -DDST=ARCH_I386 -I${CMAKE_SOURCE_DIR}/lib -c -o linkin_syscall.o ${CMAKE_SOURCE_DIR}/lib/linker/linkin_syscall.c
#)
#
#add_custom_target(
#	dbg_functions.o ALL
#	COMMAND ${CROSSTOOL_I386_BIN}/i686-unknown-linux-gnu-gcc -m32 ${LINKIN_OP_CFLAGS} -I${CMAKE_CURRENT_BINARY_DIR} -DSRC=ARCH_PPC -DDST=ARCH_I386 -I${CMAKE_SOURCE_DIR}/lib -c -o dbg_functions.o ${CMAKE_SOURCE_DIR}/lib/linker/dbg_functions.c
#)
#
#add_custom_target(
#	linux_syscall.o ALL
#	COMMAND ${CROSSTOOL_I386_BIN}/i686-unknown-linux-gnu-gcc -m32 ${LINKIN_OP_CFLAGS} -I${CMAKE_CURRENT_BINARY_DIR} -DSRC=ARCH_PPC -DDST=ARCH_I386 -I${CMAKE_SOURCE_DIR}/lib -c -o linux_syscall.o ${CMAKE_SOURCE_DIR}/lib/linker/linux/syscall.c
#)
#
#add_custom_target(
#	linux_thunk.o ALL
#	COMMAND ${CROSSTOOL_I386_BIN}/i686-unknown-linux-gnu-gcc -m32 ${LINKIN_OP_CFLAGS} -I${CMAKE_CURRENT_BINARY_DIR} -DSRC=ARCH_PPC -DDST=ARCH_I386 -I${CMAKE_SOURCE_DIR}/lib -c -o linux_thunk.o ${CMAKE_SOURCE_DIR}/lib/linker/linux/thunk.c
#)
#
#add_custom_target(
#	linux_signal.o ALL
#	COMMAND ${CROSSTOOL_I386_BIN}/i686-unknown-linux-gnu-gcc -m32 ${LINKIN_OP_CFLAGS} -I${CMAKE_CURRENT_BINARY_DIR} -DSRC=ARCH_PPC -DDST=ARCH_I386 -I${CMAKE_SOURCE_DIR}/lib -c -o linux_signal.o ${CMAKE_SOURCE_DIR}/lib/linker/linux/signal.c
#)

#add_custom_target(
#	consts_db.in ALL
#	COMMAND [ ! -s ${CMAKE_SOURCE_DIR}/consts_db.in ] && wget 'http://www.cse.iitd.ernet.in/~sbansal/eqchecker/consts_db.in' -O ${CMAKE_SOURCE_DIR}/consts_db.in || true
#)

#add_custom_target(
#src.insn.usedef.preprocessed ALL
#COMMAND [ ! -s ${CMAKE_BINARY_DIR}/src.insn.usedef.preprocessed ] && wget 'http://www.cse.iitd.ernet.in/~sbansal/eqchecker/${SRC_ARCH}.insn.usedef.preprocessed' -O ${CMAKE_BINARY_DIR}/src.insn.usedef.preprocessed || true
#)

#add_custom_target(
#dst.insn.usedef.preprocessed ALL
#COMMAND [ ! -s ${CMAKE_BINARY_DIR}/dst.insn.usedef.preprocessed ] && echo "Fetching ${DST_ARCH}.insn.usedef.preprocessed" && wget 'http://www.cse.iitd.ernet.in/~sbansal/eqchecker/${DST_ARCH}.insn.usedef.preprocessed' -O ${CMAKE_BINARY_DIR}/dst.insn.usedef.preprocessed || true
#)

#add_custom_target(
#fb.trans.tab.in ALL
#COMMAND [ ! -s ${CMAKE_BINARY_DIR}/fb.trans.tab.in ] && echo "Fetching ${SRC_ARCH}_${DST_ARCH}.fb.trans.tab" && wget 'http://www.cse.iitd.ernet.in/~sbansal/eqchecker/${SRC_ARCH}_${DST_ARCH}.fb.trans.tab' -O ${CMAKE_BINARY_DIR}/fb.trans.tab.in || true
#)

#add_custom_target(
#src_sym_exec_db ALL
#COMMAND [ ! -s ${CMAKE_BINARY_DIR}/src_sym_exec_db ] && echo "Fetching ${SRC_ARCH}_sym_exec_db" && wget 'http://www.cse.iitd.ernet.in/~sbansal/eqchecker/${SRC_ARCH}_sym_exec_db' -O ${CMAKE_BINARY_DIR}/src_sym_exec_db || true
#)

#add_custom_target(
#dst_sym_exec_db ALL
#COMMAND [ ! -s ${CMAKE_BINARY_DIR}/dst_sym_exec_db ] && echo "Fetching ${DST_ARCH}_sym_exec_db" && wget 'http://www.cse.iitd.ernet.in/~sbansal/eqchecker/${DST_ARCH}_sym_exec_db' -O ${CMAKE_BINARY_DIR}/dst_sym_exec_db || true
#)

add_custom_target(
clean_dbs
COMMAND rm -f dst_sym_exec_db src_sym_exec_db fb.trans.tab.in dst.insn.usedef.preprocessed src.insn.usedef.preprocessed dst.insn.types.preprocessed src.insn.types.preprocessed consts_db.in
)

#add_custom_target(
#push_dbs
#COMMAND scp ${CMAKE_SOURCE_DIR}/build/etfg_i386/*sym_exec_db ${CMAKE_SOURCE_DIR}/build/etfg_i386/*.insn.usedef.preprocessed ${CMAKE_SOURCE_DIR}/build/etfg_i386/*.insn.types.preprocessed ${CMAKE_SOURCE_DIR}/build/etfg_i386/fb.trans.tab.in  ${CMAKE_SOURCE_DIR}/build/etfg_i386/fb.harvest.out ${CMAKE_SOURCE_DIR}/build/etfg_i386/src_fbtest ${CMAKE_SOURCE_DIR}/build/etfg_i386/dst_fbtest sbansal@sri.cse.iitd.ac.in:"public_html/superopt_dbs/etfg_i386"
#COMMAND scp ${CMAKE_SOURCE_DIR}/build/i386_i386/*sym_exec_db ${CMAKE_SOURCE_DIR}/build/i386_i386/*.insn.usedef.preprocessed ${CMAKE_SOURCE_DIR}/build/i386_i386/*.insn.types.preprocessed ${CMAKE_SOURCE_DIR}/build/i386_i386/fb.trans.tab.in ${CMAKE_SOURCE_DIR}/build/i386_i386/fb.harvest.out ${CMAKE_SOURCE_DIR}/build/i386_i386/src_fbtest ${CMAKE_SOURCE_DIR}/build/i386_i386/dst_fbtest sbansal@sri.cse.iitd.ac.in:"public_html/superopt_dbs/i386_i386"
#COMMAND scp ${CMAKE_SOURCE_DIR}/consts_db.in sbansal@sri.cse.iitd.ac.in:"public_html/superopt_dbs"
#)

#add_custom_target(
#pull_dbs
#COMMAND cd ${CMAKE_SOURCE_DIR}/build/etfg_i386 && wget --no-proxy --backups=1 http://www.cse.iitd.ac.in/~sbansal/superopt_dbs/etfg_i386/src_sym_exec_db http://www.cse.iitd.ac.in/~sbansal/superopt_dbs/etfg_i386/dst_sym_exec_db http://www.cse.iitd.ac.in/~sbansal/superopt_dbs/etfg_i386/src.insn.usedef.preprocessed http://www.cse.iitd.ac.in/~sbansal/superopt_dbs/etfg_i386/dst.insn.usedef.preprocessed http://www.cse.iitd.ac.in/~sbansal/superopt_dbs/etfg_i386/src.insn.types.preprocessed http://www.cse.iitd.ac.in/~sbansal/superopt_dbs/etfg_i386/dst.insn.types.preprocessed http://www.cse.iitd.ac.in/~sbansal/superopt_dbs/etfg_i386/fb.trans.tab.in http://www.cse.iitd.ac.in/~sbansal/superopt_dbs/etfg_i386/fb.harvest.out http://www.cse.iitd.ac.in/~sbansal/superopt_dbs/etfg_i386/src_fbtest http://www.cse.iitd.ac.in/~sbansal/superopt_dbs/etfg_i386/dst_fbtest && cd ${CMAKE_SOURCE_DIR}
#COMMAND cd ${CMAKE_SOURCE_DIR}/build/i386_i386 && wget --no-proxy --backups=1 http://www.cse.iitd.ac.in/~sbansal/superopt_dbs/i386_i386/src_sym_exec_db http://www.cse.iitd.ac.in/~sbansal/superopt_dbs/i386_i386/dst_sym_exec_db http://www.cse.iitd.ac.in/~sbansal/superopt_dbs/i386_i386/src.insn.usedef.preprocessed http://www.cse.iitd.ac.in/~sbansal/superopt_dbs/i386_i386/dst.insn.usedef.preprocessed http://www.cse.iitd.ac.in/~sbansal/superopt_dbs/i386_i386/src.insn.types.preprocessed http://www.cse.iitd.ac.in/~sbansal/superopt_dbs/i386_i386/dst.insn.types.preprocessed http://www.cse.iitd.ac.in/~sbansal/superopt_dbs/i386_i386/fb.trans.tab.in http://www.cse.iitd.ac.in/~sbansal/superopt_dbs/i386_i386/fb.harvest.out http://www.cse.iitd.ac.in/~sbansal/superopt_dbs/i386_i386/src_fbtest http://www.cse.iitd.ac.in/~sbansal/superopt_dbs/i386_i386/dst_fbtest && cd ${CMAKE_SOURCE_DIR}
##COMMAND wget --no-proxy --backups=1 http://www.cse.iitd.ac.in/~sbansal/superopt_dbs/consts_db.in && mv consts_db.in ${CMAKE_SOURCE_DIR}
#COMMAND wget --no-proxy --backups=1 http://www.cse.iitd.ac.in/~sbansal/superopt_dbs/consts_db.in && cp consts_db.in ${CMAKE_SOURCE_DIR}
#)
