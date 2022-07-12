#!/usr/bin/env bash

# echo "echo 'Spiteful Corgi Bites' | sleep_before_gensync=60 ./run_colosseum.sh -b sync-edge-033 sync-edge-034 sync-edge-037 1009 iperf >lat_iperf2_1009.log 2>&1" | at 4:08pm

################ BEGIN PARAMETERS ################
experiments=(
    # "sync-edge-033 sync-edge-034 sync-edge-037 sync-edge-038 1017 8:49pm"
    # "sync-edge-039 sync-edge-041 sync-edge-043 1018 8:55pm"
    # "sync-edge-045 sync-edge-046 sync-edge-047 1019 8:55pm"
    # # Boston
    # "sync-edge-033 sync-edge-034 sync-edge-037 1031 12:55am"
    # "sync-edge-049 sync-edge-051 sync-edge-052 1033 12:55am"
    # "sync-edge-039 sync-edge-041 sync-edge-043 1024 12:59am"
    # # Utah
    # "sync-edge-066 sync-edge-067 sync-edge-068 1025 1:40am"
    # "sync-edge-070 sync-edge-071 sync-edge-072 1026 1:45am"
    # "sync-edge-074 sync-edge-075 sync-edge-076 1027 1:47am"

    # "sync-edge-065 sync-edge-066 sync-edge-067 1026 10:16pm"

    # "-b sync-edge-033 sync-edge-034 sync-edge-037 1009 iperf 18:46"
    # "-b sync-edge-038 sync-edge-039 sync-edge-044 1009 18:46"
    # "-b sync-edge-046 sync-edge-047 sync-edge-048 1017 iperf 18:52"
    # "-b sync-edge-049 sync-edge-051 sync-edge-052 1017 18:52"
    # "-b sync-edge-053 sync-edge-054 sync-edge-056 1018 iperf 18:52"
    # "-b sync-edge-057 sync-edge-058 sync-edge-059 1018 18:52"
    # "-b sync-edge-060 sync-edge-064 sync-edge-065 1019 iperf 18:52"
    # "-b sync-edge-066 sync-edge-067 sync-edge-068 1019 18:57"

    # "-b sync-edge-033 sync-edge-034 sync-edge-037 1017 iperf 5:36pm"
    # "-b sync-edge-038 sync-edge-039 sync-edge-044 1017 5:36pm"
    # "-b sync-edge-045 sync-edge-046 sync-edge-047 1031 iperf 5:36pm"
    # "-b sync-edge-048 sync-edge-049 sync-edge-051 1031 5:36pm"
    "-b sync-edge-037 sync-edge-038 sync-edge-039 1033 iperf 12:16pm"
    # "-b sync-edge-056 sync-edge-057 sync-edge-058 1033 5:36pm"
    "-b sync-edge-044 sync-edge-045 sync-edge-046 1024 iperf 12:18pm"
    "-b sync-edge-047 sync-edge-048 sync-edge-049 1024 12:20pm"
    "-b sync-edge-051 sync-edge-052 sync-edge-053 1025 iperf 12:21pm"
    # "-b sync-edge-052 sync-edge-053 sync-edge-054 1025 11:52pm"
    "-b sync-edge-054 sync-edge-056 sync-edge-057 1026 iperf 12:24pm"
    "-b sync-edge-058 sync-edge-059 sync-edge-060 1026 12:26pm"
    "-b sync-edge-064 sync-edge-065 sync-edge-066 1027 iperf 12:28pm"
    # "-b sync-edge-068 sync-edge-069 sync-edge-070 1027 11:52pm"
)

log_dir=start_experiments_at
containers_pass='Spiteful Corgi Bites'
################ END PARAMETERS ################

set -e
trap "exit 1" TERM
export TOP_PID=$$

requirements=( at )

err() {
    >&2 echo "error: $1"
    kill -s TERM $TOP_PID
}

check_requirements() {
    for r in ${requirements[@]}; do
        if ! command -v $r 2>&1 >/dev/null; then
            err "$r not installed."
        fi
    done
}

# Return experiment command
# $@ parameters to `run_colosseum.sh`
get_cmd() {
    if [ "${#}" -lt 4 ]; then
        err "get_cmd needs at least 4 arguments, but passed ${#}."
    fi

    local all=( "$@" )

    if [[ "${all[0]}" == "-"* ]]; then
        local flag="${all[0]}"
        case "$flag" in
            -b) local hosts="${all[@]:1:3}"
                local rest="${all[@]:4}"
                ;;
            *) err "$flag is not supported."
               ;;
        esac
    else
        local flag=""
        local hosts="${all[@]::3}"
        local rest="${all[@]:3}"
    fi

    local log_name="$(echo $flag $hosts $rest | sed 's/ /_/g')"
    local log_path="$log_dir"/"$log_name"-"$(date +%N)".log

    echo "script $log_path -f -c \"echo '$containers_pass' \
          | sleep_before_gensync=120 experiment_rep=10 ./run_colosseum.sh $flag $hosts $rest\""
}

check_requirements

mkdir -p "$log_dir"

for i in ${!experiments[@]}; do
    expr=( ${experiments[i]} )
    time="${expr[-1]}"                    # last
    params="${expr[@]::${#expr[@]}-1}"    # all but last
    cmd="$(get_cmd $params)"

    echo -e "--------\nRun at $time: '$cmd'\n--------"

    echo "$cmd" | at "$time"
done
