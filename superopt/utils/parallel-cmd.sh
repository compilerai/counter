#!/bin/bash

ROOT=${SUPEROPT_ROOT:-${HOME}}
LAST_LINK_NAME="last_result_cache"
RUNNER_SCRIPT=${RUNNER_SCRIPT:-./run_ctests.sh}

usage() {
	echo
	echo "Usage: $0 [-O OPTIONS ] [-m MSG ] [ -c CORES ] EQ_FUNCS_FNAME"
	echo
	echo "           -O options to be passed to ${RUNNER_SCRIPT}"
	echo '           -m specify message to be put as header of result_cache file'
	echo '           -c, CORES # of parallel eq commands to be run'
  echo
	echo "E.g. : $0 -m 'test run' test_funcs"
	echo
	exit 1
}

# process arguments
while getopts ':m:O:c:' opt; do
  case $opt in
    m)
      echo "-m was triggered, Parameter: $OPTARG" >&2
      MSG=$OPTARG
      ;;
    O)
      echo "-O was triggered, Parameter: $OPTARG" >&2
      RUNCTESTS_OPTIONS=$OPTARG
      ;;
    c)
      echo "-c was triggered, Parameter: $OPTARG" >&2
      CORES=$OPTARG
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

CORES=${CORES:-2}

shift $((OPTIND-1))
if [ -z $1 ];
then
  echo "Assuming eq_funcs as input file name"
  INPUT_FNAME="eq_funcs"
else
  INPUT_FNAME=$1
fi

COMMIT_ID=$([[ -d ${ROOT}/superopt/.git ]] && (pushd ${ROOT}/superopt/ >/dev/null 2>&1; git rev-parse --short HEAD; popd >/dev/null 2>&1) || cat ${ROOT}/superopt/git-commit)
RUN_ID="${INPUT_FNAME}_$(date +"%F_%T")_${COMMIT_ID}"
OUTPUT_FNAME="${RUN_ID}_result_cache"
OUTPUT_DIRNAME="eqlogs_${RUN_ID}"

mkdir -p "${OUTPUT_DIRNAME}"

echo "Input file name: $INPUT_FNAME"
echo "Output file name: $OUTPUT_FNAME"
echo "Output dir name: $OUTPUT_DIRNAME"
echo

true > "${OUTPUT_FNAME}"
if [[ ! -z ${RUNCTESTS_OPTIONS} ]]; then
  echo "# ${RUNNER_SCRIPT} options: ${RUNCTESTS_OPTIONS}" >> "${OUTPUT_FNAME}"
fi
if [[ ! -z ${MSG} ]]; then
  echo "# ${MSG}" >> "${OUTPUT_FNAME}"
fi

# handle C-c
trap 'kill -TERM ${PID}' TERM INT

grep -v -e '^#' -e '^[ ]*$' "${INPUT_FNAME}" | awk '{ print $1 }' | time parallel -P${CORES} ${RUNNER_SCRIPT} -D ${OUTPUT_DIRNAME} ${RUNCTESTS_OPTIONS} {} | tee -a "${OUTPUT_FNAME}" &

# wait for parallel to finish
PID=$!
wait ${PID}

# remove output file if empty
[ -f "${OUTPUT_FNAME}" ] && [ ! -s "${OUTPUT_FNAME}" ] && rm "${OUTPUT_FNAME}"

# create soft link for `last_result_cache'
ln -sf "${OUTPUT_FNAME}" ${LAST_LINK_NAME}
