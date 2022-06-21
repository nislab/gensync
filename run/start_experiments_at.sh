#!/usr/bin/env bash

echo "echo 'Spiteful Corgi Bites' | sleep_before_gensync=60 ./run_colosseum.sh -b sync-edge-033 sync-edge-034 sync-edge-037 1009 >iperf_1009.log 2>&1" | at 10:51pm

echo "echo 'Spiteful Corgi Bites' | sleep_before_gensync=60 ./run_colosseum.sh -b sync-edge-038 sync-edge-039 sync-edge-044 1009 latency >ping_1009.log 2>&1" | at 10:52pm

# ################ BEGIN PARAMETERS ################
# experiments=( # "sync-edge-033 sync-edge-034 sync-edge-037 sync-edge-038 1017 8:49pm"
#               # "sync-edge-039 sync-edge-041 sync-edge-043 sync-edge-044 1018 8:55pm"
#               # "sync-edge-045 sync-edge-046 sync-edge-047 sync-edge-048 1019 8:55pm"
#               # # Boston
#               # "sync-edge-033 sync-edge-034 sync-edge-037 sync-edge-038 1031 12:55am"
#               # "sync-edge-049 sync-edge-051 sync-edge-052 sync-edge-053 1033 12:55am"
#               # "sync-edge-039 sync-edge-041 sync-edge-043 sync-edge-044 1024 12:59am"
#               # # Utah
#               # "sync-edge-066 sync-edge-067 sync-edge-068 sync-edge-069 1025 1:40am"
#               # "sync-edge-070 sync-edge-071 sync-edge-072 sync-edge-073 1026 1:45am"
#     # "sync-edge-074 sync-edge-075 sync-edge-076 sync-edge-077 1027 1:47am"

#     "sync-edge-065 sync-edge-066 sync-edge-067 1026 10:16pm" )

# log_dir=start_experiments_at
# containers_pass='Spiteful Corgi Bites'
# ################ END PARAMETERS ################

# set -e
# trap "exit 1" TERM
# export TOP_PID=$$

# requirements=( at )

# err() {
#     echo "error: $1"
#     kill -s TERM $TOP_PID
#     exit
# }

# check_requirements() {
#     for r in ${requirements[@]}; do
#         if ! command -v $r 2>&1 >/dev/null; then
#             err "$r not installed."
#         fi
#     done
# }

# # Return experiment command
# # ${@:0:3} hosts
# # ${@: -1} scenario ID
# get_cmd() {
#     if ! [ "${#}" -eq 4 ]; then
#         err "get_cmd needs exactly 4 arguments, but passed ${#}."
#     fi

#     local all=( "$@" )
#     local scenario="${all[-1]}"
#     local hosts="${all[@]::3}"
#     local log_name="$(echo $scenario $hosts | sed 's/ /_/g')"
#     local log_path="$log_dir"/"$log_name"-"$(date +%N)".log

#     echo "script $log_path -f -c \"echo '$containers_pass' \
#           | sleep_before_gensync=60 experiment_rep=10 ./run_colosseum.sh $hosts $scenario\""
# }

# check_requirements

# mkdir -p "$log_dir"

# for i in ${!experiments[@]}; do
#     expr=( ${experiments[i]} )
#     time="${expr[-1]}"                    # last
#     params="${expr[@]::${#expr[@]}-1}"    # all but last
#     cmd="$(get_cmd $params)"

#     echo -e "--------\nRun at $time: '$cmd'\n--------"

#     echo "$cmd" | at "$time"
# done
