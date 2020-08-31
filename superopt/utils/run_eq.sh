#!/bin/bash

# globals
ROOT=${SUPEROPT_ROOT:-${HOME}}
BINARY_PATH_EQ=${ROOT}/superopt/build/etfg_i386/
BINARY_PATH_HARVEST=${ROOT}/superopt/build/i386_i386/
INPUT_FILES_DIR=${ROOT}/superopt-tests/
# BINARY_PATH="${ROOT}/superopt/build/"
export LD_LIBRARY_PATH=$ROOT/superopt/build/third_party/z3-z3-4.8.4/build
DEFAULT_EQ_PARAMS="--smt-query-timeout 200"
usage() {
	echo
	echo "Usage: $0 [-$] [-l] [-n] [-s] [-c COMPILER] [-o OPT_LVL] [-d DBG_TOOL] [ -O LOG_FILE ] [ -D LOG_FILE_DIR ] [ -P EQ_PARAMS ] [BINARY_NAME:]FUNC_NAME"
	echo
	echo '            BINARY_NAME is assumed in case of ctests.'
	echo
	echo '           -$ disables .etfg and .tfg caching'
	echo '           -l disables logging'
	echo '           -n do not run eq, just generate .etfg and .tfg files'
	echo '           -s save SMT solver stats to a file.  Filename will be made of .eqlog prefix with ".stats" suffix'
	echo '           -C, COMPILER can be {gcc, clang, icc}. Default is: gcc'
	echo '           -o, OPT_LVL can be {O2, O3}. Default is: O3'
	echo '           -d, DBG_TOOL can be {gdb, valgrind, rr, lldb, perf, time}. Default is: None'
	echo '           -O, path of output .eqlog file. Default is: "LOG_FILE_DIR/FUNC_NAME.COMPILER.OPT_LVL.eqlog".'
	echo '               NOTE: -l takes precedence over it and it takes precendence over LOG_FILE_DIR'
	echo '           -P, params to be passed directly to eq cmd.  Default is: ${DEFAULT_EQ_PARAMS}'
	echo '           -D, dir of output .eqlog file. Default is: "eqlogs"'
  echo
	echo "E.g. : $0 -c gcc -o O2 show_move "
	echo
	exit 1
}

PROG_NAME=$0

# process arguments
while getopts ':$lnsc:o:O:d:D:P:t:' opt; do
  case $opt in
    $)
      #echo "Disabling cache"
      NOCACHE=1
      ;;
    l)
      #echo "Disabling logging"
      NOLOGS=1
      ;;
    n)
      NOEQ=1
      ;;
    s)
      SOLVERSTATS=1
      ;;
    c)
      #echo "-c was triggered, Parameter: $OPTARG" >&2
      COMPILER=$OPTARG
      ;;
    o)
      #echo "-o was triggered, Parameter: $OPTARG" >&2
      OPT_LVL=$OPTARG
      ;;
    O)
      #echo "-O was triggered, Paramter: $OPTARG" >&2
      LOG_FILE=$OPTARG
      ;;
    d)
      #echo "-d was triggered, Parameter: $OPTARG" >&2
      DBG_TOOL=$OPTARG
      ;;
    D)
      #echo "-D was triggered, Paramter: $OPTARG" >&2
      LOG_FILE_DIR=$OPTARG
      ;;
    P)
      #echo "-P was triggered, Paramter: $OPTARG" >&2
      EQ_PARAMS=$OPTARG
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      usage
      ;;
    :)
      echo "Option -$OPTARG requires an argument." >&2
      usage
      ;;
  esac
done

# set defaults if arg not supplied
COMPILER=${COMPILER:-gcc}
OPT_LVL=${OPT_LVL:-O3}
DBG_TOOL=${DBG_TOOL:-}
LOG_FILE_DIR=${LOG_FILE_DIR:-eqlogs}
EQ_PARAMS=${EQ_PARAMS:-${DEFAULT_EQ_PARAMS}}

# validate arguments
declare -a COMPILERS=("gcc" "clang" "icc")
if [[ ! -n ${COMPILERS[${COMPILER}]} ]]; then
    echo "Invalid compiler \"$COMPILER\""
    usage
fi

declare -a OPT_LVLS=("O2" "O3")
if [[ ! -n ${OPT_LVLS[${OPT_LVL}]} ]]; then
    echo "Invalid opt-level \"$OPT_LVL\""
    usage
fi

declare -a DBG_TOOLS=("time" "gdb" "lldb" "valgrind" "rr" "perf" "")
if [[ ! -n ${DBG_TOOLS[${DBG_TOOL}]} ]]; then
    echo "Invalid debug tool \"$DBG_TOOL\""
    usage
fi

# get function name
shift $((OPTIND-1))
if [ -z $1 ];
then
	echo "Error: function name missing!"
	usage
fi

temp=(${1//:/ }) # convert : to space and make it array
if [[ ${#temp[@]} -eq 2 ]];
then
  BINARY_NAME=${temp[0]}
  FUNC_NAME=${temp[1]}
else
  FUNC_NAME=$1
fi

#### INPUT SPECIFIC SETUP ####

if [[ ${PROG_NAME} == *"run_ctests.sh"* ]];
then
  TEST_PREFIX=ctests
  TEST_SUFFIX=${COMPILER}.eqchecker.${OPT_LVL}
  LLVM_SRC=${INPUT_FILES_DIR}/cint/ctests.bc.O0
  X86_SRC=${INPUT_FILES_DIR}/cint/ctests.${TEST_SUFFIX}.i386
elif [[ ${PROG_NAME} == *"run_svcomp.sh"* ]];
then
  if [[ -v BINARY_NAME ]];
  then
    TEST_PREFIX=svcomp.${BINARY_NAME}
    TEST_SUFFIX=${COMPILER}.eqchecker.${OPT_LVL}
    LLVM_SRC=${INPUT_FILES_DIR}/svcomp/${BINARY_NAME}.bc.O0
    X86_SRC=${INPUT_FILES_DIR}/svcomp/${BINARY_NAME}.${TEST_SUFFIX}.i386
  else
    echo "Error! BINARY_NAME is required for svcomp."
    usage
  fi
elif [[ ${PROG_NAME} == *"run_bzip2.sh"* ]];
then
  if [[ -v BINARY_NAME ]];
  then
    TEST_PREFIX=cprog.${BINARY_NAME}
    TEST_SUFFIX=${COMPILER}.eqchecker.${OPT_LVL}
    LLVM_SRC=${INPUT_FILES_DIR}/bzip2/${BINARY_NAME}.bc.O0
    X86_SRC=${INPUT_FILES_DIR}/bzip2/${BINARY_NAME}.${TEST_SUFFIX}.i386
  else
    echo "Error! BINARY_NAME is required for cprog."
    usage
  fi
elif [[ ${PROG_NAME} == *"run_tsvc.sh"* ]];
then
  if [[ -v BINARY_NAME ]];
  then
    TEST_PREFIX=tsvc.${BINARY_NAME}
    TEST_SUFFIX=${COMPILER}.${OPT_LVL}
    LLVM_SRC=${INPUT_FILES_DIR}/tsvc/${BINARY_NAME}.bc.O0
    X86_SRC=${INPUT_FILES_DIR}/tsvc/${BINARY_NAME}.${TEST_SUFFIX}.i386
  else
    echo "Error! BINARY_NAME is required for tsvc."
    usage
  fi
elif [[ ${PROG_NAME} == *"run_spec2k.sh"* ]];
then
  if [[ -v BINARY_NAME ]];
  then
    TEST_PREFIX=spec2k.${BINARY_NAME}
    TEST_SUFFIX=${COMPILER}.eqchecker.${OPT_LVL}
    LLVM_SRC=$ROOT/spec2k/llvm-bc-O0/${BINARY_NAME}
    X86_SRC=$ROOT/spec2k/${COMPILER}-i386-${OPT_LVL}/${BINARY_NAME}
  else
    echo "Error! BINARY_NAME is required for spec2k."
    usage
  fi
elif [[ ${PROG_NAME} == *"run_llvm2ir.sh"* ]];
then
  if [[ -v BINARY_NAME ]];
  then
    TEST_PREFIX=cprog.${BINARY_NAME}
    # TODO use OPT_LVL?
    TEST_SUFFIX="llvm2ir"
    LLVM_SRC=${INPUT_FILES_DIR}/cint/${BINARY_NAME}.bc.O0
    LLVM2IR_SRC=${LLVM_SRC}
  else
    echo "Error! BINARY_NAME is required for cprog."
    usage
  fi
elif [[ ${PROG_NAME} == *"run_llvm2ir_spec2k.sh"* ]];
then
  if [[ -v BINARY_NAME ]];
  then
    TEST_PREFIX=spec2k.${BINARY_NAME}
    # TODO use OPT_LVL?
    TEST_SUFFIX="llvm2ir"
    LLVM_SRC=$ROOT/spec2k/llvm-bc-O0/${BINARY_NAME}
    LLVM2IR_SRC=${LLVM_SRC}
  else
    echo "Error! BINARY_NAME is required for spec2k."
    usage
  fi
else
  echo "Do not use run_eq.sh directly; use the links run_ctests.sh or run_spec2k.sh etc.!"
  exit 2
fi

#### COMMON PART ####

## setup proof filename

mkdir -p ${ROOT}/{eqfiles,eqlogs}

PROOF_FILE="${LOG_FILE_DIR}/${TEST_PREFIX}.${FUNC_NAME}.${TEST_SUFFIX}.proof"

## setup .eqlog filename

if [[ -v NOLOGS ]];
then
  LOG_FILE="/dev/null"
else
  LOG_FILE="${LOG_FILE:-${LOG_FILE_DIR}/${TEST_PREFIX}.${FUNC_NAME}.${TEST_SUFFIX}.eqlog}"
fi

## solver stats
if [[ -v SOLVERSTATS ]];
then
  SOLVER_STATS_FILE="${LOG_FILE_DIR}/${TEST_PREFIX}.${FUNC_NAME}.${TEST_SUFFIX}.stats"
fi

## setup llvm src .etfg filename -- arg1

## prefer global .etfg
#if [[ ! -v NOCACHE && -s $ROOT/eqfiles/${TEST_PREFIX}.etfg ]]; then
#  ETFG_FILENAME=$ROOT/eqfiles/${TEST_PREFIX}.etfg
#else
#  ETFG_FILENAME=$ROOT/eqfiles/${TEST_PREFIX}.${FUNC_NAME}.etfg
#fi

# generate only global .etfg since otherwise alias analysis would not be complete
ETFG_FILENAME=$ROOT/eqfiles/${TEST_PREFIX}.etfg

# run llvm2tfg to construct etfg
if [[ -v NOCACHE || ! -s ${ETFG_FILENAME} ]]; then
  LLVM2TFG_CMD="$ROOT/llvm-build/bin/llvm2tfg ${LLVM_SRC} -o ${ETFG_FILENAME} &&"
  #LLVM2TFG_CMD="$ROOT/llvm-build/bin/llvm2tfg -f ${FUNC_NAME} ${LLVM_SRC} -o ${ETFG_FILENAME} &&"
  ARG1_CMD=${LLVM2TFG_CMD}
fi
ARG1_FILENAME=${ETFG_FILENAME}

## setup arg2 filename

if [[ ! -v LLVM2IR_SRC ]]; then
  # prefer global .tfg
  if [[ ! -v NOCACHE && -s $ROOT/eqfiles/${TEST_PREFIX}.${TEST_SUFFIX}.tfg ]]; then
    TFG_FILENAME=$ROOT/eqfiles/${TEST_PREFIX}.${TEST_SUFFIX}.tfg
  else
    TFG_FILENAME=$ROOT/eqfiles/${TEST_PREFIX}.${FUNC_NAME}.${TEST_SUFFIX}.tfg
  fi
  # generate tfg
  if [[ -v NOCACHE || ! -s ${TFG_FILENAME} ]]; then
    HARVEST_FILENAME=$ROOT/eqfiles/${TEST_PREFIX}.${FUNC_NAME}.${TEST_SUFFIX}.harvest
    HARVEST_CMD="${BINARY_PATH_HARVEST}/harvest -functions_only -live_callee_save -allow_unsupported -no_canonicalize_imms -no_eliminate_unreachable_bbls -no_eliminate_duplicates -function_name ${FUNC_NAME} -o ${HARVEST_FILENAME} -l "${HARVEST_FILENAME}.log" ${X86_SRC} &&"
    EQGEN_CMD="${BINARY_PATH_EQ}/eqgen -f ${FUNC_NAME} -tfg_llvm ${ETFG_FILENAME} -l "${HARVEST_FILENAME}.log" -o ${TFG_FILENAME} -e ${X86_SRC} ${HARVEST_FILENAME} &&"
    #gdb --args ${BINARY_PATH_EQ}/eqgen -f ${FUNC_NAME} -tfg_llvm ${ETFG_FILENAME} -l "${HARVEST_FILENAME}.log" -o ${TFG_FILENAME} -e ${X86_SRC} ${HARVEST_FILENAME}
    ARG2_CMD=${HARVEST_CMD}${EQGEN_CMD}
  fi

  ARG2_FILENAME=${TFG_FILENAME}
else
  EQ_PARAMS="--llvm2ir ${EQ_PARAMS}"
  # TODO use LLVM2IR_SRC for building LLVM2IR_FILENAME file
  LLVM2IR_FILENAME=${ETFG_FILENAME}
  ARG2_FILENAME=${LLVM2IR_FILENAME}
fi

# no eq

if [[ -v NOEQ ]]; then
  FINAL_CMD=${ARG1_CMD}${ARG2_CMD}true
  bash -c "${FINAL_CMD}" > ${LOG_FILE} 2>&1
  exit
fi

if [[ -v SOLVER_STATS_FILE ]];
then
  SOLVER_STATS_ARG="--solver_stats ${SOLVER_STATS_FILE}"
fi
EQ_CMD="${BINARY_PATH_EQ}/eq ${EQ_PARAMS} --proof ${PROOF_FILE} ${SOLVER_STATS_ARG} -f ${FUNC_NAME} ${ARG1_FILENAME} ${ARG2_FILENAME}"

# run eq

case $DBG_TOOL in
  gdb)
    FINAL_CMD=${LLVM2TFG_CMD}${HARVEST_CMD}${EQGEN_CMD}true
    bash -c "${FINAL_CMD}"
    gdb -q --args $EQ_CMD
    ;;
  lldb)
    FINAL_CMD=${LLVM2TFG_CMD}${HARVEST_CMD}${EQGEN_CMD}true
    bash -c "${FINAL_CMD}"
    lldb -- $EQ_CMD
    ;;
  valgrind)
    FINAL_CMD=${LLVM2TFG_CMD}${HARVEST_CMD}${EQGEN_CMD}true
    bash -c "${FINAL_CMD}"
    valgrind -v --tool=memcheck --max-stackframe=4103256 $EQ_CMD
    ;;
  rr)
    FINAL_CMD=${LLVM2TFG_CMD}${HARVEST_CMD}${EQGEN_CMD}true
    bash -c "${FINAL_CMD}"
    rr record $EQ_CMD
    rr replay
    ;;
  perf)
    FINAL_CMD=${LLVM2TFG_CMD}${HARVEST_CMD}${EQGEN_CMD}true
    bash -c "${FINAL_CMD}"
    # Ref: https://stackoverflow.com/questions/7031210/linux-perf-how-to-interpret-and-find-hotspots
    # -F specifies sampling frequency per second, default is 4000
    perf record -F10 --call-graph dwarf -- $EQ_CMD > ${LOG_FILE} # XXX: HUGE result files, full call-graph
    #perf record -F 10 --call-graph lbr   -- $EQ_CMD > ${LOG_FILE} # smaller result files, better perf at the cost of limited call stack
    # TUI output
    perf report
    # flamegraph output
    #perf script | stackcollapse-perf.pl | flamegraph.pl  > eqflames.svg
    ;;
  time)
    FINAL_CMD=${LLVM2TFG_CMD}${HARVEST_CMD}${EQGEN_CMD}${EQ_CMD}
    time ${ROOT}/superopt/utils/chaperon.py "${FINAL_CMD}" --logfile ${LOG_FILE}
    ;;
  *)
    FINAL_CMD=${LLVM2TFG_CMD}${HARVEST_CMD}${EQGEN_CMD}${EQ_CMD}
    ${ROOT}/superopt/utils/chaperon.py "${FINAL_CMD}" --logfile ${LOG_FILE}
    ;;
esac

[[ -t 1 ]] && type say >/dev/null 2>&1 && say "Finished"
