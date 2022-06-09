cmake_minimum_required(VERSION 3.10)

project(superopt)

if(CMAKE_CONFIGURATION_TYPES)
  list(APPEND CMAKE_CONFIGURATION_TYPES etfg_i386)
  list(APPEND CMAKE_CONFIGURATION_TYPES i386_i386)
  list(APPEND CMAKE_CONFIGURATION_TYPES ppc_i386)
  list(APPEND CMAKE_CONFIGURATION_TYPES etfg_x64)
  list(APPEND CMAKE_CONFIGURATION_TYPES x64_x64)
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
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE) # always generate RPATH

execute_process(COMMAND echo ${CMAKE_BUILD_TYPE} COMMAND grep -Eo "^[^_]+" COMMAND tr -d '[:space:]' OUTPUT_VARIABLE SRC_ARCH)
execute_process(COMMAND echo ${CMAKE_BUILD_TYPE} COMMAND grep -Eo "[^_]+$" COMMAND tr -d '[:space:]' OUTPUT_VARIABLE DST_ARCH)
set(SRC_DST_HYPHEN ${SRC_ARCH}-${DST_ARCH})

find_package(Boost REQUIRED COMPONENTS system iostreams serialization)
find_package(yaml-cpp REQUIRED)
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

set(OPT_LEVEL "-Og")
set(DEBUG "-g")
#set(DEFINES  "${DEFINES} -DSUPEROPT_DIR=\"\\\"${SUPEROPT_DIR}\\\"\"")

#set(PROFILE_FLAGS "-pg")
set(PROFILE_FLAGS "")
set(WARNING_FLAGS "-Wall -Wno-unused-function -Wno-unused-variable -Wno-unused-label -Wno-unused-parameter -Wno-unused-value -Wno-unused-but-set-variable -Wno-parentheses -Wno-unused-result -Wno-sign-compare -Wno-narrowing -Wno-unknown-warning-option -Wno-mismatched-tags -Wno-undefined-var-template -Wno-null-conversion")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcommon")
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

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
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

set(ETFG_X64_FLAGS "-DARCH_SRC=ARCH_ETFG -DARCH_DST=ARCH_X64 -DSRC_DST=\\\"etfg_x64\\\"")
set(X64_X64_FLAGS "-DARCH_SRC=ARCH_X64 -DARCH_DST=ARCH_X64 -DSRC_DST=\\\"x64_x64\\\"")

set(CMAKE_C_FLAGS_ETFG_I386 "${CMAKE_C_FLAGS} ${ETFG_I386_FLAGS}")
set(CMAKE_C_FLAGS_I386_I386 "${CMAKE_C_FLAGS} ${I386_I386_FLAGS}")
set(CMAKE_C_FLAGS_PPC_I386 "${CMAKE_C_FLAGS} ${PPC_I386_FLAGS}")
set(CMAKE_C_FLAGS_ETFG_X64 "${CMAKE_C_FLAGS} ${ETFG_X64_FLAGS}")
set(CMAKE_C_FLAGS_X64_X64 "${CMAKE_C_FLAGS} ${X64_X64_FLAGS}")

set(CMAKE_CXX_FLAGS_ETFG_I386 "${CMAKE_CXX_FLAGS} ${ETFG_I386_FLAGS}")
set(CMAKE_CXX_FLAGS_I386_I386 "${CMAKE_CXX_FLAGS} ${I386_I386_FLAGS}")
set(CMAKE_CXX_FLAGS_PPC_I386 "${CMAKE_CXX_FLAGS} ${PPC_I386_FLAGS}")
set(CMAKE_CXX_FLAGS_ETFG_X64 "${CMAKE_CXX_FLAGS} ${ETFG_X64_FLAGS}")
set(CMAKE_CXX_FLAGS_X64_X64 "${CMAKE_CXX_FLAGS} ${X64_X64_FLAGS}")

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

#set(z3_ver z3-4.8.10)
set(z3_ver z3-4.8.14)
#set(z3_pkgname ${z3_ver}-x64-ubuntu-18.04)
set(z3_pkgname ${z3_ver}-x64-glibc-2.31)
#set(z3_tarfile ${z3_pkgname}.zip)
set(z3_tarfile ${z3_pkgname}.zip)
set(z3_dir ${third_party_dir}/z3)
#set(z3_path ${z3_dir}/${z3_pkgname})
set(z3_binpath ${z3_dir}/${z3_pkgname})
set(z3_exe ${z3_binpath}/bin/z3)
set(z3_lib ${z3_binpath}/bin)
set(z3_include ${z3_binpath}/include)

set(z3v487 z3-4.8.7)
set(z3v487_pkgname ${z3v487}-x86_64.pkg)
set(z3v487_tarfile ${z3v487_pkgname}.tar.xz)
set(z3v487_dir ${third_party_dir}/z3v487)
set(z3v487_binpath ${z3v487_dir}/usr)
set(z3v487_exe ${z3v487_binpath}/bin/z3)
set(z3v487_lib ${z3v487_binpath}/lib)
set(z3v487_include ${z3v487_binpath}/include)

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

add_custom_target(solvers ALL DEPENDS ${yices_exe} ${cvc4_exe} ${z3_exe} ${z3v487_exe})

set(elfio_ver elfio-3.2)
set(elfio_dir ${third_party_dir}/${elfio_ver})
set(elfio_hdr ${elfio_dir}/elfio/elfio.hpp)

set(jemalloc_ver jemalloc)
set(jemalloc_dir ${third_party_dir}/${jemalloc_ver})
set(jemalloc_install_dir ${third_party_dir}/${jemalloc_ver}-install)
set(libjemalloc_file ${jemalloc_install_dir}/lib/libjemalloc.a)

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

add_custom_target(jemalloc_target DEPENDS ${libjemalloc_file})
add_library(jemalloc_lib STATIC IMPORTED GLOBAL)
set_target_properties(jemalloc_lib PROPERTIES IMPORTED_LOCATION ${libjemalloc_file})

add_dependencies(jemalloc_lib jemalloc_target)

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
#configure_file(lib/support/bv_solve_template.py bv_solve_template.py COPYONLY)
#configure_file(lib/support/sage_driver.py sage_driver.py)

link_directories(${z3_lib})
link_directories(/usr/lib)
link_directories(/usr/local/lib)
#link_directories(${yices_mod_dir}/build/x86_64-unknown-linux-gnu-release/lib)
#link_directories(${binutils_install_dir}/lib)

include_directories(${binutils_install_dir}/include)
include_directories(${jemalloc_install_dir}/include)
include_directories(${z3_include})
include_directories(${elfio_dir})
include_directories(${CMAKE_BINARY_DIR})
include_directories(${freetds_install}/include)
include_directories(/usr/include/tirpc)
#include_directories(${tdspp_install}/include)

link_directories(${freetds_install}/lib)
#link_directories(${tdspp_install}/lib)

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

#BISON_TARGET(expr_parse_y lib/expr/expr_parse.ypp ${CMAKE_CURRENT_BINARY_DIR}/expr_parse_y.cpp COMPILE_FLAGS -d)
#FLEX_TARGET(expr_parse_l lib/expr/expr_parse.l  ${CMAKE_CURRENT_BINARY_DIR}/expr_parse_l.cpp COMPILE_FLAGS -Plts)
#ADD_FLEX_BISON_DEPENDENCY(expr_parse_l expr_parse_y)

BISON_TARGET(expr_parse_y lib/expr/expr_parse.ypp ${CMAKE_CURRENT_BINARY_DIR}/expr_parse_y.cpp COMPILE_FLAGS -d)
FLEX_TARGET(expr_parse_l lib/expr/expr_parse.l ${CMAKE_CURRENT_BINARY_DIR}/expr_parse_l.cpp COMPILE_FLAGS)
ADD_FLEX_BISON_DEPENDENCY(expr_parse_l expr_parse_y)

BISON_TARGET(scev_parse_y lib/gsupport/scev_parse.ypp ${CMAKE_CURRENT_BINARY_DIR}/scev_parse_y.cpp COMPILE_FLAGS -d)
FLEX_TARGET(scev_parse_l lib/gsupport/scev_parse.l ${CMAKE_CURRENT_BINARY_DIR}/scev_parse_l.cpp COMPILE_FLAGS)
ADD_FLEX_BISON_DEPENDENCY(scev_parse_l scev_parse_y)

BISON_TARGET(yices_model_y lib/parser/yices_model.ypp ${CMAKE_CURRENT_BINARY_DIR}/yices_model_y.cpp COMPILE_FLAGS -d)
FLEX_TARGET(yices_model_l lib/parser/yices_model.l ${CMAKE_CURRENT_BINARY_DIR}/yices_model_l.cpp COMPILE_FLAGS)
ADD_FLEX_BISON_DEPENDENCY(yices_model_l yices_model_y)

BISON_TARGET(z3_model_y lib/parser/z3_model.ypp ${CMAKE_CURRENT_BINARY_DIR}/z3_model_y.cpp COMPILE_FLAGS -d)
FLEX_TARGET(z3_model_l lib/parser/z3_model.l ${CMAKE_CURRENT_BINARY_DIR}/z3_model_l.cpp COMPILE_FLAGS)
ADD_FLEX_BISON_DEPENDENCY(z3_model_l z3_model_y)

BISON_TARGET(z3_neo_model_y lib/parser/z3_neo_model.ypp ${CMAKE_CURRENT_BINARY_DIR}/z3_neo_model_y.cpp COMPILE_FLAGS -d)
FLEX_TARGET(z3_neo_model_l lib/parser/z3_neo_model.l ${CMAKE_CURRENT_BINARY_DIR}/z3_neo_model_l.cpp COMPILE_FLAGS)
ADD_FLEX_BISON_DEPENDENCY(z3_neo_model_l z3_neo_model_y)

BISON_TARGET(cvc4_model_y lib/parser/cvc4_model.ypp ${CMAKE_CURRENT_BINARY_DIR}/cvc4_model_y.cpp)
FLEX_TARGET(cvc4_model_l lib/parser/cvc4_model.l ${CMAKE_CURRENT_BINARY_DIR}/cvc4_model_l.cpp COMPILE_FLAGS)
ADD_FLEX_BISON_DEPENDENCY(cvc4_model_l cvc4_model_y)

BISON_TARGET(boolector_model_y lib/parser/boolector_model.ypp ${CMAKE_CURRENT_BINARY_DIR}/boolector_model_y.cpp)
FLEX_TARGET(boolector_model_l lib/parser/boolector_model.l ${CMAKE_CURRENT_BINARY_DIR}/boolector_model_l.cpp COMPILE_FLAGS)
ADD_FLEX_BISON_DEPENDENCY(boolector_model_l boolector_model_y)

BISON_TARGET(edge_guard_y lib/gsupport/edge_guard.ypp ${CMAKE_CURRENT_BINARY_DIR}/edge_guard_y.cpp COMPILE_FLAGS -d)
FLEX_TARGET(edge_guard_l lib/gsupport/edge_guard.l  ${CMAKE_CURRENT_BINARY_DIR}/edge_guard_l.cpp COMPILE_FLAGS -Peg)
ADD_FLEX_BISON_DEPENDENCY(edge_guard_l edge_guard_y)

BISON_TARGET(cg_edge_composition_y lib/gsupport/cg_edge_composition.ypp ${CMAKE_CURRENT_BINARY_DIR}/cg_edge_composition_y.cpp COMPILE_FLAGS -d)
FLEX_TARGET(cg_edge_composition_l lib/gsupport/cg_edge_composition.l  ${CMAKE_CURRENT_BINARY_DIR}/cg_edge_composition_l.cpp COMPILE_FLAGS -Pcg)
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

add_library(db STATIC ${freetds_lib} #${tdspp_lib}
  lib/db/db.cpp
)

#add_library(fbgen STATIC
#                  lib/fbgen/ocaml_output_parser.cpp
#)


#add_library(expr STATIC ${z3_exe} ${z3v487_exe} ${yices_exe} ${yices_mod_exe}
#                  lib/expr/solver_interface.cpp
#                  lib/expr/graph_arg.cpp
#                  lib/expr/graph_arg_regs.cpp
#                  lib/expr/relevant_memlabels.cpp
#                  lib/expr/pc.cpp
#                  lib/expr/imm_to_expr.cpp
#                  lib/expr/allocsite.cpp
#                  lib/expr/allocstack.cpp
#                  lib/expr/sprel_map_pair.cpp
#                  lib/expr/sprel_map.cpp
#                  lib/expr/state.cpp
#                  lib/expr/expr_simplify.cpp
#                  lib/expr/local_sprel_expr_guesses.cpp
#                  lib/expr/expr_c_api.cpp
#                  lib/expr/expr.cpp
#                  lib/expr/expr_sort.cpp
#                  lib/expr/constant_value.cpp
#                  lib/expr/array_constant.cpp
#                  lib/expr/context.cpp
#                  lib/expr/context_cache.cpp
#                  lib/expr/expr_utils.cpp
#                  lib/expr/expr_to_z3_expr.cpp
#                  lib/expr/z3_solver.cpp
#                  lib/expr/counter_example.cpp
#                  lib/expr/alias_region.cpp
#                  lib/expr/memlabel.cpp
#                  lib/expr/memlabel_obj.cpp
#                  lib/expr/reachable_memlabels_map.cpp
#                  lib/expr/expr_eval_status.cpp
#                  lib/expr/comment.cpp
#                  lib/expr/langtype.cpp
#                  lib/expr/rounding_mode.cpp
#                  lib/expr/consts_struct.cpp
#                  lib/expr/aliasing_constraints.cpp
#                  lib/expr/range.cpp
#                  lib/expr/range_utils.cpp
#                  lib/expr/graph_local.cpp
#                  lib/expr/graph_locals_map.cpp
#                  lib/expr/eval.cpp
#                  lib/expr/sp_version.cpp
#                  lib/expr/ce_eval.cpp
#                  lib/expr/fsignature.cpp
#                  lib/expr/axpred_context.cpp
#                  lib/expr/axpred_parser.cpp
#                  lib/expr/axpred_desc.cpp
#                  lib/expr/callee_rw_memlabels.cpp
#                  lib/expr/call_context_entry.cpp
#                  lib/expr/call_context.cpp
#                  lib/parser/parsing_common.cpp
#                  lib/parser/ast_linearizer.cpp
#                  ${BISON_expr_parse_y_OUTPUTS}
#                  ${FLEX_expr_parse_l_OUTPUTS}
#                  ${BISON_z3_neo_model_y_OUTPUTS}
#                  ${FLEX_z3_neo_model_l_OUTPUTS}
#)

#add_library(valtag STATIC
#                  lib/valtag/ldst_input.cpp
#                  lib/valtag/pc_with_copy_id.cpp
#                  lib/valtag/imm_vt_map.cpp
#                  lib/valtag/valtag.cpp
#                  lib/valtag/memslot_set.cpp
#                  lib/valtag/memslot_map.cpp
#                  lib/valtag/memvt.cpp
#                  lib/valtag/nextpc_map.cpp
#                  lib/valtag/nextpc.cpp
#                  lib/valtag/memset.cpp
#                  lib/valtag/regmap.cpp
#                  lib/valtag/regcons.cpp
#                  lib/valtag/regset.cpp
#                  lib/valtag/regcount.cpp
#                  lib/valtag/symbol_set.cpp
#                  lib/valtag/state_elem_desc.cpp
#                  lib/valtag/reg_identifier.cpp
#                  lib/valtag/fixed_reg_mapping.cpp
#                  lib/valtag/transmap.cpp
#                  lib/valtag/transmap_loc.cpp
#)

#add_library(insn STATIC ${elfio_hdr}
#                  lib/insn/edge_table.cpp
#                  lib/insn/peep_preprocess.cpp
#                  lib/insn/live_ranges.cpp
#                  lib/insn/cfg.cpp
#                  lib/insn/jumptable.cpp
#                  lib/insn/fun_type_info.cpp
#                  lib/insn/insn_line_map.cpp
#                  lib/insn/input_file.cpp
#                  lib/insn/src-insn.cpp
#                  lib/insn/dst-insn.cpp
#                  lib/insn/usedef.cpp
#                  lib/insn/insn_list.cpp
#                  lib/insn/transmap_translation.cpp
#                  lib/insn/rdefs.cpp
#
#                  lib/i386/bbl.cpp
#                  lib/i386/insn.cpp
#                  lib/i386/costfn_core2.cpp
#                  lib/i386/costfn_haswell.cpp
#                  lib/i386/disas.c
#                  lib/i386/execute.cpp
#                  lib/i386/opctable.cpp
#
#                  lib/x64/insn.cpp
#
#                  lib/etfg/etfg_insn.cpp
#
#                  #lib/ppc/bbl.cpp
#                  #lib/ppc/disas.c
#                  #lib/ppc/execute.cpp
#                  #lib/ppc/insn-disas.cpp
#		              #lib/ppc/insn.cpp
#                  #lib/ppc/regset.cpp
#                  #lib/ppc/usedef.cpp
#)

#add_library(rewrite_lib STATIC ${elfio_hdr} ${libdwarf_exe}
#                  ${CMAKE_BINARY_DIR}/superoptdb_rpc.h
#                  lib/rewrite/symbol_map.cpp
#                  lib/rewrite/static_translate.cpp
#                  lib/rewrite/peephole.cpp
#                  lib/rewrite/regconst_map_possibilities.cpp
#                  lib/rewrite/nomatch_pairs.cpp
#                  lib/rewrite/transmap_set.cpp
#                  lib/rewrite/transmap_genset.cpp
#                  #lib/rewrite/usedef_sort.cpp
#                  lib/rewrite/function_pointers.cpp
#                  lib/rewrite/tagline.cpp
#                  lib/rewrite/dst_insn_folding_opts.cpp
#                  lib/rewrite/dst_insn_schedule.cpp
#                  #lib/rewrite/dwarf_map.cpp
#                  #lib/rewrite/locals_info.cpp
#                  #lib/rewrite/memlabel_map.c
#                  lib/rewrite/assembly_handling.cpp
#)

#add_library(eqchecker STATIC
#                  lib/eqgen/dst_tfg.cpp
#                  lib/eqgen/xml_print.cpp
#
#                  lib/eq/parse_input_eq_file.cpp
#                  lib/eq/corr_graph.cpp
#                  lib/eq/correlate.cpp
#                  lib/eq/correl_hints.cpp
#                  lib/eq/function_cg_map.cpp
#                  lib/eq/cg_with_inductive_preds.cpp
#                  lib/eq/cg_with_rank.cpp
#                  lib/eq/cg_with_backtracker.cpp
#                  lib/eq/cg_with_relocatable_memlabels.cpp
#                  lib/eq/cg_with_safety.cpp
#                  lib/eq/cg_with_asm_annotation.cpp
#                  lib/eq/cg_with_dst_ml_check.cpp
#                  lib/eq/cg_with_failcond.cpp
#                  lib/eq/unsafe_cond.cpp
#                  lib/eq/eqcheck.cpp
#                  lib/eq/quota.cpp
#                  lib/eq/cg_suffixpath_dfa.cpp
#                  lib/eq/failed_cg_set.cpp
#)

#add_library(ptfg STATIC
#                  lib/ptfg/llptfg.cpp
#                  lib/ptfg/function_signature.cpp
#                  lib/ptfg/ftmap.cpp
#                  lib/ptfg/ftmap_call_graph.cpp
#)

#add_library(gsupport STATIC
#                  lib/gsupport/expr_simplify_with_predicates.cpp
#                  lib/gsupport/alloc_dealloc.cpp
#                  lib/gsupport/src_dst_cg_path_tuple.cpp
#                  lib/gsupport/edge_wf_cond.cpp
#                  lib/gsupport/failcond.cpp
#                  lib/gsupport/graph_full_pathset.cpp
#                  lib/gsupport/edge_with_graph_ec.cpp
#                  lib/gsupport/node_with_graph_ec.cpp
#                  lib/gsupport/memlabel_assertions.cpp
#                  lib/gsupport/corr_graph_edge.cpp
#                  lib/gsupport/edge_with_cond.cpp
#                  lib/gsupport/tfg_edge.cpp
#                  lib/gsupport/tfg_edge_set.cpp
#                  lib/gsupport/itfg_edge.cpp
#                  lib/gsupport/itfg_ec.cpp
#                  lib/gsupport/suffixpath.cpp
#                  lib/gsupport/scev.cpp
#                  lib/gsupport/scev_toplevel.cpp
#                  lib/gsupport/rodata_map.cpp
#                  lib/gsupport/parse_edge_composition.cpp
#                  lib/gsupport/graph_ec.cpp
#                  lib/gsupport/graph_ec_with_attr.cpp
#                  lib/gsupport/tfg_ec.cpp
#                  lib/gsupport/graph_symbol.cpp
#                  #lib/gsupport/edge_guard.cpp
#                  lib/gsupport/predicate.cpp
#                  lib/gsupport/precond.cpp
#                  lib/gsupport/predicate_set.cpp
#                  lib/gsupport/gsupport_cache.cpp
#                  lib/gsupport/pc_local_sprel_expr_guesses.cpp
#                  lib/gsupport/pc_label.cpp
#                  ${BISON_edge_guard_y_OUTPUTS}
#                  ${FLEX_edge_guard_l_OUTPUTS}
#                  ${BISON_cg_edge_composition_y_OUTPUTS}
#                  ${FLEX_cg_edge_composition_l_OUTPUTS}
#                  ${BISON_scev_parse_y_OUTPUTS}
#                  ${FLEX_scev_parse_l_OUTPUTS}
#)

#add_library(tfg STATIC
#                  lib/tfg/nextpc_function_name_map.cpp
#                  lib/tfg/symbol_or_section_id.cpp
#                  lib/tfg/tfg.cpp
#                  #lib/tfg/tfg_ssa.cpp
#                  lib/tfg/tfg_llvm.cpp
#                  lib/tfg/tfg_asm.cpp
#                  lib/tfg/tfg_ssa.cpp
#                  lib/tfg/tfg_rewrite.cpp
#                  lib/tfg/suffixpath_dfa.cpp
#                  lib/tfg/sp_version_relations_dfa.cpp
#                  lib/tfg/ssa_analysis.cpp
#)

#add_library(sym_exec STATIC
#                  lib/sym_exec/sym_exec.cpp
#)

#add_library(graph STATIC ${elfio_hdr} ${libdwarf_exe}
#                  lib/graph/var_version.cpp
#                  lib/graph/prove_qd.cpp
#                  lib/graph/available_exprs_alias_analysis_combo.cpp
#                  lib/graph/graph_locs_map.cpp
#                  lib/graph/dfa.cpp
#                  lib/graph/dfa_over_paths.cpp
#                  lib/graph/alias_dfa.cpp
#                  lib/graph/graph_ec_unroll.cpp
#                  lib/graph/point_set.cpp
#                  lib/graph/avail_exprs_val.cpp
#                  lib/graph/avail_exprs.cpp
#                  lib/graph/avail_exprs_dfa.cpp
#                  lib/graph/graph.cpp
#                  lib/graph/graph_region.cpp
#                  lib/graph/graph_with_regions.cpp
#                  lib/graph/graph_with_var_versions.cpp
#                  lib/graph/graph_with_predicates.cpp
#                  lib/graph/graph_with_simplified_assets.cpp
#                  lib/graph/graph_with_locs.cpp
#                  lib/graph/graph_with_precondition.cpp
#                  lib/graph/graph_loc_id.cpp
#                  lib/graph/graph_with_aliasing.cpp
#                  lib/graph/graph_with_path_sensitive_locs.cpp
#                  lib/graph/graph_with_execution.cpp
#                  lib/graph/graph_with_proofs.cpp
#                  lib/graph/graph_with_ce.cpp
#                  lib/graph/prove.cpp
#                  lib/graph/query_decomposition.cpp
#                  lib/graph/graph_cp_location.cpp
#                  lib/graph/lr_map.cpp
#                  lib/graph/lr_status.cpp
#                  lib/graph/callee_summary.cpp
#                  lib/graph/point_set_helper.cpp
#                  lib/graph/expr_group.cpp
#                  lib/graph/point.cpp
#                  lib/graph/point_val.cpp
#                  lib/graph/eqclasses.cpp
#                  lib/graph/locset.cpp
#                  lib/graph/nodece.cpp
#                  lib/graph/graphce.cpp
#                  lib/graph/nodece_set.cpp
#                  lib/graph/invariant_state.cpp
#                  lib/graph/smallest_point_cover.cpp
#                  lib/graph/smallest_point_cover_arr.cpp
#                  lib/graph/smallest_point_cover_bv.cpp
#                  lib/graph/addr_that_may_differ.cpp
#                  lib/graph/graph_with_points.cpp
#                  lib/graph/graph_with_guessing.cpp
#                  lib/graph/graph_with_ssa_funcs.cpp
#                  lib/graph/graph_with_paths.cpp
#                  lib/graph/graph_with_correctness_covers.cpp
#                  lib/graph/bvsolve_cache.cpp
#)

#add_library(superopt STATIC
#                  lib/superopt/state.cpp
#                  lib/superopt/fingerprintdb_live_out.cpp
#                  lib/superopt/fallback.cpp
#                  lib/superopt/rand_states.cpp
#                  lib/superopt/fallback_qemu.cpp
#                  lib/superopt/sym_exec_db.cpp
#                  ${CMAKE_BINARY_DIR}/superoptdb_rpc.h
#                  ${CMAKE_BINARY_DIR}/superoptdb_rpc_xdr.c
#                  ${CMAKE_BINARY_DIR}/superoptdb_rpc_clnt.c
#                  lib/superopt/itable.cpp
#                  lib/superopt/itable_position.cpp
#                  lib/superopt/superopt.cpp
#                  lib/superopt/superoptdb.cpp
#                  lib/superopt/type_infer.cpp
#                  lib/superopt/typestate.cpp
#                  lib/superopt/typesystem.cpp
#                  lib/superopt/peep_type_cons.cpp
#                  lib/superopt/fingerprint.cpp
#                  lib/superopt/harvest.cpp
#                  lib/equiv/equiv.cpp
#                  lib/equiv/bequiv.cpp
#                  lib/equiv/jtable.cpp
#                  lib/equiv/jtable_gencode.cpp
#)


#add_library(exec STATIC
#                  lib/exec/state.cpp
#)

#add_library(LockstepDbg STATIC
#    lib/lockstep/instrumented_functions.cpp
#    lib/lockstep/Lockstep.cpp
#    lib/lockstep/Lockstep_helper.cpp
#    lib/expr/pc.cpp
#    lib/support/debug.cpp
#    lib/support/dyn_debug.cpp
#    lib/support/utils32.cpp
#)

#add_library(mymalloc STATIC
#    lib/mymalloc/my_malloc.cpp
#    lib/mymalloc/memlib.cpp
#    lib/mymalloc/myutils.cpp
#)

#SET_TARGET_PROPERTIES(mymalloc PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32 -lssl")
#SET_TARGET_PROPERTIES(LockstepDbg PROPERTIES COMPILE_FLAGS "-m32" LINK_FLAGS "-m32 -lssl")


#add_library(gas STATIC ${binutils_libbfd_file} ${binutils_libiberty_file}
#    lib/gas/common/input-scrub.c lib/gas/common/app.c lib/gas/common/macro.c
#    lib/gas/common/sb.c lib/gas/common/ehopt.c lib/gas/common/messages.c
#    lib/gas/common/input-file.c lib/gas/common/read.c lib/gas/common/atof-ieee.c
#    lib/gas/common/output-file.c lib/gas/common/write.c lib/gas/common/cond.c
#    lib/gas/common/hash.c lib/gas/common/listing.c lib/gas/common/frags.c
#    lib/gas/common/subsegs.c lib/gas/common/symbols.c lib/gas/common/expr.c
#    lib/gas/common/xmalloc.c lib/gas/common/dwarf2dbg.c lib/gas/common/as.c
#    lib/gas/common/common.c lib/gas/common/hex.c lib/gas/common/atof-generic.c
#    lib/gas/common/flonum-konst.c lib/gas/common/flonum-mult.c lib/gas/common/flonum-copy.c
#    lib/gas/universal.c lib/gas/disas-all.c lib/gas/tc-common.c
#    lib/gas/i386-opc.c lib/gas/tc-i386.c lib/gas/tc-ppc.c lib/gas/dummy.c
#)

#add_library(support STATIC
#                  lib/support/bvsolve_retval.cpp
#                  lib/support/free_id_list.cpp
#                  lib/support/dmalloc.cpp
#                  lib/support/read_source_files.cpp
#                  lib/support/timers.cpp
#                  lib/support/log.cpp
#                  lib/support/mytimer.cpp
#                  lib/support/sig_handling.cpp
#                  lib/support/cl.cpp
#                  lib/support/utils32.cpp
#                  lib/support/utils.cpp
#                  lib/support/crypto.cpp
#                  lib/support/mybitset.cpp
#                  lib/support/command.cpp
#                  lib/support/debug.cpp
#                  lib/support/dyn_debug.cpp
#                  lib/support/stopwatch.cpp
#                  lib/support/c_utils.cpp
#                  lib/support/stacktrace.cpp
#                  lib/support/cmd_helper.cpp
#                  lib/support/bv_const.cpp
#                  lib/support/bv_const_expr.cpp
#                  lib/support/linear_solver.cpp
#                  lib/support/bv_solve.cpp
#                  lib/support/sage_interface.cpp
#                  lib/support/query_comment.cpp
#                  lib/support/scatter_gather.cpp
#                  lib/support/backtracker.cpp
#                  lib/support/string_ref.cpp
#                  lib/support/serialize.cpp
#                  lib/support/bv_const_ref.cpp
#                  lib/support/globals_cpp.cpp
#)

add_library(parser STATIC
                  lib/parser/parse_tree_common.cpp
                  lib/parser/parse_tree_yices.cpp
                  ${BISON_yices_model_y_OUTPUTS}
                  ${FLEX_yices_model_l_OUTPUTS}
                  ${BISON_z3_model_y_OUTPUTS}
                  ${FLEX_z3_model_l_OUTPUTS}
                  ${BISON_cvc4_model_y_OUTPUTS}
                  ${FLEX_cvc4_model_l_OUTPUTS}
                  ${BISON_boolector_model_y_OUTPUTS}
                  ${FLEX_boolector_model_l_OUTPUTS}
                  ${BISON_yices_model_y_OUTPUTS}
                  ${FLEX_yices_model_l_OUTPUTS}
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


set(dblibs db sybdb)
set(libs ${BINLIBS_DIR}/libsuperopt.a ${BINLIBS_DIR}/libfbgen.a ${BINLIBS_DIR}/libeqchecker.a ${BINLIBS_DIR}/libsym_exec.a ${BINLIBS_DIR}/librewrite_lib.a ${BINLIBS_DIR}/libinsn.a ${BINLIBS_DIR}/libptfg.a ${BINLIBS_DIR}/libtfg.a ${BINLIBS_DIR}/libgraph.a ${BINLIBS_DIR}/libgsupport.a ${BINLIBS_DIR}/libexec.a ${BINLIBS_DIR}/libexpr.a ${BINLIBS_DIR}/libvaltag.a ${BINLIBS_DIR}/libgas.a ${dblibs} ${BINLIBS_DIR}/libsupport.a cutils fpu parser z3 jemalloc_lib -lgmp -lgmpxx ${Boost_LIBRARIES} -ldl binutils_libbfd binutils_libiberty -lm -lpthread -lz -lmagic -lboost_filesystem -lssl -lcrypto -lyaml-cpp -ltirpc)

add_executable(update_dst_edge_for_local_alloc_and_dealloc tools/update_dst_edge_for_local_alloc_and_dealloc.cpp)
target_link_libraries(update_dst_edge_for_local_alloc_and_dealloc ${libs})


#add_executable(ctypecheck tools/ctypecheck.cpp)
#target_link_libraries(ctypecheck ${libs})

#add_executable(ub_safety tools/tfg_check_safety.cpp)
#target_link_libraries(ub_safety ${libs})

#add_executable(eq tools/eq_main.cpp)
#target_link_libraries(eq ${libs})

#add_executable(tfg_preprocess_before_eqcheck tools/tfg_preprocess_before_eqcheck.cpp)
#target_link_libraries(tfg_preprocess_before_eqcheck ${libs})

#add_executable(tfg_ssa_construct_from_non_ssa_tfg tools/tfg_ssa_construct_from_non_ssa_tfg.cpp)
#target_link_libraries(tfg_ssa_construct_from_non_ssa_tfg ${libs})

add_executable(identify_durables tools/identify_durables.cpp)
target_link_libraries(identify_durables ${libs})


#add_executable(qcc-codegen tools/qcc-codegen.cpp)
#target_link_libraries(qcc-codegen ${libs})

#add_executable(cg_check_safety tools/cg_check_safety.cpp)
#target_link_libraries(cg_check_safety ${libs})

#add_executable(cg_compute_correctness_covers tools/cg_compute_correctness_covers.cpp)
#target_link_libraries(cg_compute_correctness_covers ${libs})

#add_executable(cg_check_relocatable_memlabels tools/cg_check_relocatable_memlabels.cpp)
#target_link_libraries(cg_check_relocatable_memlabels ${libs})

#add_executable(dst_insn_disassemble tools/dst_insn_disassemble.cpp)
#target_link_libraries(dst_insn_disassemble ${libs})

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

#add_executable(peep_enumerate_transmaps tools/peep_enumerate_transmaps.cpp)
#target_link_libraries(peep_enumerate_transmaps ${libs})

#add_executable(peep_translate tools/peep_translate.cpp)
#target_link_libraries(peep_translate ${libs})

#add_executable(eq-check-proof tools/eq_check_proof.cpp)
#target_link_libraries(eq-check-proof ${libs})

add_executable(expr_is_provable tools/expr_is_provable.cpp)
target_link_libraries(expr_is_provable ${libs})

add_executable(simplify tools/simplify_main.cpp)
target_link_libraries(simplify ${libs})

#add_executable(simplify_using_sprel_pair_and_memlabel_maps tools/simplify_using_sprel_pair_and_memlabel_maps_main.cpp)
#target_link_libraries(simplify_using_sprel_pair_and_memlabel_maps ${libs})

#add_executable(pred_canonicalizer tools/pred_canonicalizer.cpp)
#target_link_libraries(pred_canonicalizer ${libs})

add_executable(bv_solver tools/bv_solver.cpp)
target_link_libraries(bv_solver ${libs})

add_executable(ldr_decompose_and_linear_solve tools/ldr_decompose_and_linear_solve.cpp)
target_link_libraries(ldr_decompose_and_linear_solve ${libs})

add_executable(update_points_matrix_with_new_points_and_ldr_decompose tools/update_points_matrix_with_new_points_and_ldr_decompose.cpp)
target_link_libraries(update_points_matrix_with_new_points_and_ldr_decompose ${libs})

add_executable(mybitset_tester tools/mybitset_tester.cpp)
target_link_libraries(mybitset_tester ${libs})

add_executable(range_tester tools/range_tester.cpp)
target_link_libraries(range_tester ${libs})

add_executable(scc_tester tools/scc_tester.cpp)
target_link_libraries(scc_tester ${libs})

add_executable(is_expr_equal_using_lhs_set tools/is_expr_equal_using_lhs_set_main.cpp)
target_link_libraries(is_expr_equal_using_lhs_set ${libs})

add_executable(decide_hoare_triple tools/decide_hoare_triple.cpp)
target_link_libraries(decide_hoare_triple ${libs})

#add_executable(decide_hoare_triple_tfg tools/decide_hoare_triple_tfg.cpp)
#target_link_libraries(decide_hoare_triple_tfg ${libs})

add_executable(update_invariant_state_over_edge tools/update_invariant_state_over_edge.cpp)
target_link_libraries(update_invariant_state_over_edge ${libs})

add_executable(infer_invariants_xfer_over_path tools/infer_invariants_xfer_over_path_main.cpp)
target_link_libraries(infer_invariants_xfer_over_path ${libs})

#add_executable(check_monotonicity_of_invariants tools/check_monotonicity_of_invariants.cpp)
#target_link_libraries(check_monotonicity_of_invariants ${libs})

#add_executable(binsearch_test tools/binsearch_test.cpp)
#target_link_libraries(binsearch_test ${libs})

#add_executable(minimize_bv_matrix tools/minimize_bv_matrix.cpp)
#target_link_libraries(minimize_bv_matrix ${libs})

add_executable(check_wfconds_on_edge tools/check_wfconds_on_edge.cpp)
target_link_libraries(check_wfconds_on_edge ${libs})

add_executable(graph_propagate_CEs_across_new_edge tools/graph_propagate_CEs_across_new_edge.cpp)
target_link_libraries(graph_propagate_CEs_across_new_edge ${libs})

add_executable(prove tools/prove_main.cpp)
target_link_libraries(prove ${libs})

add_executable(ce_eval_prove tools/ce_eval_prove_main.cpp)
target_link_libraries(ce_eval_prove ${libs})

add_executable(ptfg2llvm tools/ptfg2llvm.cpp)
target_link_libraries(ptfg2llvm ${libs})

add_executable(transmap_translate tools/transmap_translate.cpp)
target_link_libraries(transmap_translate ${libs})

add_executable(ac_canonical tools/array_constant_canonicalizer.cpp)
target_link_libraries(ac_canonical ${libs})

add_executable(smt_helper_process lib/helpers/smt_helper_process.cpp)
target_link_libraries(smt_helper_process ${libs})

add_executable(qd_helper_process lib/helpers/qd_helper_process.cpp)
target_link_libraries(qd_helper_process ${libs})

#add_executable(cmd_helper_process lib/support/cmd_helper_process.cpp)
#target_link_libraries(cmd_helper_process ${libs})

add_executable(harvest tools/harvest_main.cpp)
target_link_libraries(harvest ${libs})

add_executable(rewrite tools/rewrite_main.cpp)
target_link_libraries(rewrite ${libs})

#add_executable(codegen tools/codegen_main.cpp etfg-i386.ptab)
#target_link_libraries(codegen ${libs})
#add_dependencies(codegen mymalloc)

#add_executable(debug_gen tools/debug_gen.cpp)
#target_link_libraries(debug_gen ${libs})
#add_dependencies(debug_gen LockstepDbg)
#add_dependencies(debug_gen mymalloc)

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

#add_executable(superoptdb_server lib/server/superoptdb_server.cpp ${CMAKE_BINARY_DIR}/superoptdb_rpc_svc.c)
#target_link_libraries(superoptdb_server ${libs} ${binutils_lib})

add_executable(usedef_init tools/usedef_init.cpp)
target_link_libraries(usedef_init ${libs} ${binutils_lib})

add_executable(enumerate_for_src_iseq_using_itable tools/enumerate_for_src_iseq_using_itable.cpp)
target_link_libraries(enumerate_for_src_iseq_using_itable ${libs} ${binutils_lib})

add_executable(typestates_make_non_poly_copyable tools/typestates_make_non_poly_copyable.cpp)
target_link_libraries(typestates_make_non_poly_copyable ${libs} ${binutils_lib})

#add_executable(dst_iseq_assemble tools/dst_iseq_assemble.cpp)
#target_link_libraries(dst_iseq_assemble ${libs} ${binutils_lib})

add_executable(jtable_from_itable tools/jtable_from_itable.cpp)
target_link_libraries(jtable_from_itable ${libs} ${binutils_lib})

add_executable(yices_parse tools/yices_parser.cpp)
target_link_libraries(yices_parse ${libs} ${binutils_lib})

add_executable(z3_parse tools/z3_parser.cpp)
target_link_libraries(z3_parse ${libs} ${binutils_lib})

add_executable(z3_neo_parse tools/z3_neo_parser.cpp)
target_link_libraries(z3_neo_parse ${libs} ${binutils_lib})

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

add_executable(tfg_split_memory_and_propagate_sprels tools/tfg_split_memory_and_propagate_sprels.cpp)
target_link_libraries(tfg_split_memory_and_propagate_sprels ${libs})

#add_executable(etfg_split_memory_and_propagate_sprels tools/etfg_split_memory_and_propagate_sprels.cpp)
#target_link_libraries(etfg_split_memory_and_propagate_sprels ${libs})

add_executable(annotate_assembly_using_proof_file tools/annotate_assembly_using_proof_file.cpp)
target_link_libraries(annotate_assembly_using_proof_file ${libs})

add_executable(prove_queries_diff tools/prove_queries_diff_main.cpp)
target_link_libraries(prove_queries_diff ${libs})

add_executable(prove_query_split tools/prove_query_split.cpp)
target_link_libraries(prove_query_split ${libs})

#add_executable(debug_soundness tools/debug_soundness.cpp)
#target_link_libraries(debug_soundness ${libs})

#add_executable(tfg_unroll_till_no_repeated_pcs tools/tfg_unroll_till_no_repeated_pcs.cpp)
#target_link_libraries(tfg_unroll_till_no_repeated_pcs ${libs})

add_custom_command(
	#OUTPUT ${z3_exe_custom}
  OUTPUT ${z3_exe}
  COMMAND [ ! -s ${z3_exe} ] && (mkdir -p ${z3_dir} && cd ${z3_dir} && unzip ${TARS_DIR}/${z3_tarfile}) || /bin/true
  #COMMAND [ ! -s ${z3_exe} ] && (mkdir -p ${z3_dir} && cd ${z3_dir} && tar xf ${TARS_DIR}/${z3_tarfile}) || /bin/true
  DEPENDS ${TARS_DIR}/${z3_tarfile}
  )

add_custom_command(
  OUTPUT ${z3v487_exe}
  COMMAND [ ! -s ${z3v487_exe} ] && (mkdir -p ${z3v487_dir} && cd ${z3v487_dir} && tar xf ${TARS_DIR}/${z3v487_tarfile}) || /bin/true
  DEPENDS ${TARS_DIR}/${z3v487_tarfile}
  )



add_custom_command(
  OUTPUT ${libjemalloc_file}
  COMMAND rm -rf ${jemalloc_dir} ${jemalloc_install_dir} && cd ${third_party_dir} && git clone https://github.com/compilerai/jemalloc ${jemalloc_ver} && cd ${jemalloc_dir} && ./autogen.sh && ./configure --prefix=${jemalloc_install_dir} --with-jemalloc-prefix=deterministic_  --disable-cxx && make && make install
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
  DEPENDS ${TARS_DIR}/${binutils_ver}.tar.bz2 ${elfio_hdr} ${libdwarf_exe}
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
  OUTPUT ${CMAKE_BINARY_DIR}/superoptdb_rpc.h ${CMAKE_BINARY_DIR}/superoptdb_rpc_xdr.c ${CMAKE_BINARY_DIR}/superoptdb_rpc_svc.c ${CMAKE_BINARY_DIR}/superoptdb_rpc_clnt.c
  COMMAND echo "pseudorand = " ${PSEUDORAND}
  COMMAND echo "pseudorand2 = " ${PSEUDORAND2}
  COMMAND sed "s/XXX_PROGNUM_XXX/${PSEUDORAND}/g" ${CMAKE_SOURCE_DIR}/lib/superopt/superoptdb_rpc.x > ${CMAKE_BINARY_DIR}/superoptdb_rpc.x.S
  COMMAND gcc -P -E ${CMAKE_BINARY_DIR}/superoptdb_rpc.x.S > ${CMAKE_BINARY_DIR}/superoptdb_rpc.x
  COMMAND rpcgen ${CMAKE_BINARY_DIR}/superoptdb_rpc.x
  #COMMAND mv ${CMAKE_BINARY_DIR}/superoptdb_rpc_svc.c ${CMAKE_BINARY_DIR}/superoptdb_rpc_svc.cpp
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
