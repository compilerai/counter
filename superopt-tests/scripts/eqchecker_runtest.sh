#!/bin/bash

set -eu # fail on error

EQLOGS=${EQLOGS_DIR:-${PWD}/eqlogs}
EQLOGS=$(realpath ${EQLOGS}) # get absolute path
mkdir -p ${EQLOGS}

BC_O0_SUFFIX=${BC_O0_SUFFIX}.ll
ETFG_SUFFIX=${BC_O0_SUFFIX}.ALL.etfg
O3_SUFFIX=${GCC_O3_SUFFIX#gcc.}

gen_command_internal_concise()
{
  fn=${1}
  etfg_file=${2}
  tfg_pfx=${3}
  eqflags=${4}
  user_sfx=${5}

  eqrun_ident=${user_sfx}

  etfg_path=${PWD}/${etfg_file}
  tfg_path=${PWD}/${tfg_pfx}.ALL.tfg
#  proof_path=${PWD}/${eqrun_ident}.proof
  proof_path=${EQLOGS}/${eqrun_ident}.proof  # USE this if version control for proof files is required
  eqlog_path=${EQLOGS}/${eqrun_ident}.eqlog

  echo "python ${SUPEROPT_PROJECT_DIR}/superopt/utils/chaperon.py --logfile \"${eqlog_path}\" \"${SUPEROPT_PROJECT_DIR}/superopt/build/etfg_i386/eq -f ${fn} ${eqflags} --proof ${proof_path} ${etfg_path} ${tfg_path}\""
}

gen_command_internal()
{
  fn=${1}
  etfg_file=${2}
  tfg_pfx=${3}
  eqflags=${4}
  user_sfx=${5}

  eqrun_ident=${tfg_pfx}${user_sfx}

  etfg_path=${PWD}/${etfg_file}
  tfg_path=${PWD}/${tfg_pfx}.ALL.tfg
#  proof_path=${PWD}/${eqrun_ident}.proof
  proof_path=${EQLOGS}/${eqrun_ident}.proof  # USE this if version control for proof files is required
  eqlog_path=${EQLOGS}/${eqrun_ident}.eqlog

  echo "python ${SUPEROPT_PROJECT_DIR}/superopt/utils/chaperon.py --logfile \"${eqlog_path}\" \"${SUPEROPT_PROJECT_DIR}/superopt/build/etfg_i386/eq -f ${fn} ${eqflags} --proof ${proof_path} ${etfg_path} ${tfg_path}\""
}

gen_command_all_compilers()
{
  fn=$1
  etfg_file=$2
  tfg_comm_pfx=$3
  file_fn_pfx=$4
  user_sfx_opt=${5:-}

  eqflags_non_comp=${g_global_eqflags:-}
  eqflags_non_comp="${eqflags_non_comp} ${g_eqflags[$file_fn_pfx]:-}"

  for compiler in clang gcc icc;
  do
      eqflags_comp=${g_eqflags[${file_fn_pfx}.${compiler}]:-${eqflags_non_comp}}
      tfg_pfx=${tfg_comm_pfx}.${compiler}.${O3_SUFFIX}
      gen_command_internal ${fn} ${etfg_file} ${tfg_pfx} "${eqflags_comp}" ".${fn}${user_sfx_opt}"
  done
}

get_funcs_except_main_and_MYmy_from_etfg()
{
  file=$1
  grep '=FunctionName:' "${file}" | grep -v -e 'MYmy' -e 'main$' | cut -f2 -d' '
}

gen_for_src_dst()
{
  filename_pfx="$1"
  user_sfx_opt="${2:+.${2}}"

  etfg_file="${filename_pfx}_src.${ETFG_SUFFIX}"
  tfg_file_pfx="${filename_pfx}_dst"
  for fn in $(get_funcs_except_main_and_MYmy_from_etfg "${etfg_file}");
  do
    file_fn_pfx="${filename_pfx}.${fn}"
    gen_command_all_compilers ${fn} ${etfg_file} ${tfg_file_pfx} ${file_fn_pfx} "${user_sfx_opt}"
  done
}

gen_for_all()
{
  filename_pfx="$1"
  user_sfx_opt="${2:+.${2}}"

  etfg_file="${filename_pfx}.${ETFG_SUFFIX}"
  tfg_file_pfx="${filename_pfx}"

  for fn in $(get_funcs_except_main_and_MYmy_from_etfg "${etfg_file}");
  do
    file_fn_pfx="${filename_pfx}.${fn}"
    gen_command_all_compilers ${fn} ${etfg_file} ${tfg_file_pfx} ${file_fn_pfx} "${user_sfx_opt}"
  done
}

gen_for_ll_as()
{
  filename_pfx="$1"
  user_sfx_opt="${2:+.${2}}"

  etfg_file="${filename_pfx}.ll.${ETFG_SUFFIX}"
  tfg_file_pfx="${filename_pfx}.as.${O0_SUFFIX}"
  for fn in $(get_funcs_except_main_and_MYmy_from_etfg "${etfg_file}");
  do
    file_fn_pfx="${filename_pfx}.${fn}"
    eqflags_all=${g_global_eqflags:-}
    eqflags_all="${eqflags_all} ${g_eqflags[$file_fn_pfx]:-}"
    gen_command_internal ${fn} ${etfg_file} ${tfg_file_pfx} "${eqflags_all}" ".${fn}${user_sfx_opt}"
  done
}

gen_commands_from_file_src_dst()
{
  infile="$1"
  eq_opts="$2"
  user_sfx_opt="${3:+.${3}}"

  while read line;
  do
    [[ "${line}" == "" ]] && continue
    [[ "${line}" == '#'* ]] && continue
    arr=(${line})
    func=${arr[0]} # take out the first space separated word
    eqflags=${g_eqflags[$func]:-}
    eqflags_comp=${g_eqflags[${func}.${compiler}]:-${eqflags}}
    final_eq_opts="${eq_opts} ${g_global_eqflags:-} ${eqflags_comp}"

    etfg_file="${binary}_src.${ETFG_SUFFIX}"
    tfg_pfx="${binary}_dst.${compiler}.${O3_SUFFIX}"
    gen_command_internal_concise ${func} ${etfg_file} ${tfg_pfx} "${final_eq_opts}" "${func}${user_sfx_opt}"
  done < ${infile}
}
gen_commands_from_file()
{
  infile="$1"
  eq_opts="$2"
  user_sfx_opt="${3:+.${3}}"

  while read line;
  do
    [[ "${line}" == "" ]] && continue
    [[ "${line}" == '#'* ]] && continue
    arr=(${line})
    func=${arr[0]} # take out the first space separated word
    eqflags=${g_eqflags[$func]:-}
    eqflags_comp=${g_eqflags[${func}.${compiler}]:-${eqflags}}
    final_eq_opts="${eq_opts} ${g_global_eqflags:-} ${eqflags_comp}"

    etfg_file="${binary}.${ETFG_SUFFIX}"
    tfg_pfx="${binary}.${compiler}.${O3_SUFFIX}"
    gen_command_internal_concise ${func} ${etfg_file} ${tfg_pfx} "${final_eq_opts}" "${func}.${compiler}${user_sfx_opt}"
  done < ${infile}
}
