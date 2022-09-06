#!/usr/bin/env bash
#
# Author: Novak Boškov <boskov@bu.edu>
# Date: Jan, 2021.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

set -e

############################### PARAMETERS BEGIN ###############################
# How many times to repeat the same experiment
repeat=100

# When we do incremental reconciliation, reconciliation is invoked
# after this much elelmenta insertions (on both server and client
# side).
# CAUTION: Put only one server-client pair in params_dir or
# provide server_params_file and client_params_file.
# chunk_size=100

# Define either server and client files...
# server_params_file=
# client_params_file=

# ... or the directory where to find the data sets, and a .cpisync header
# If params_header contains SET_OPTIMAL, the script tries to do so.
params_dir=../example/example_params_dir

params_header="Sync protocol (as in GenSync.h): 1
m_bar: SET_OPTIMAL
bits: 64
epsilon: 3
partitions/pFactor(for InterCPISync): 0
hashes: false
Sketches:
--------------------------------------------------------------------------------"

# Network parameters
latency=10
bandwidth="12/60"
packet_loss=0.001
cpu_server=100
cpu_client=100

# Where to obtain needed executables
mininet_path=../mininet_exec/mininet_exec.py
cpisync_path=..
# When on remote, python_path is a path on remote
python_path=/home/novak/.virtualenvs/statistics/bin/python
################################ PARAMETERS END ################################

help() {
cat<<EOF
USAGE: ./run_experiments.sh [-q] [-s] [-i] [-f] [-r REMOTE_PATH] [-p EXPERIMENT_DIR] [-pp PULL_REMOTE]

See the beginning of this script to set the parameters for experiments.

OPTIONS:
    -r REMOTE_PATH the path on remote to copy all the needed parts to.
    -p EXPERIMENT_DIR make a csv file out of this dir, and pull it to the local working directory.
    -pp PULL_REMOTE pull from here to my DATA/. Used to gather data from experimetns.
    -s used with -r when we only want to prepare an experiment on remote but not to run it.
    -q when this script is run on a remote set this.
    -i ignore mininet and run the client and the server on bare machine.
    -f force recreating param files' headers (works only with params_dir).

EXAMPLES:
    # runs locally
    ./run_experiments.sh
    # runs on remote
    ./run_experiments.sh -r remote_name:/remote/path
    # collects data from remote and creates CPI_Experiment_1.csv locally
    ./run_experiments.sh -p remote_name:/home/novak/EXPERIMENTS/CPI_Experiment_1
    # pulls the experimental data from the remote. Creates DATA/CPISync/1/.cpisync
    ./run_experiments.sh -pp remote_name:/home/novak/EXPERIMENTS/./CPISync/1/.cpisync

If a remote machine is used, it needs the following dependencies:
    - Mininet,
    - Python 3 (with Mininet API),
    - CPISync dependencies (see cpisync/README.md), and
    - Ninja build system.

CPISync source code will be compiled on the remote machine.
EOF
}

############################### FUNCTIONS BEGIN ################################

call_common_el() {
    echo "$(./count_common.py $1 -- "--------")"
}

next_power_of_two() {
    python <<EOF
p = 1
while p < $1:
    p = p << 1
print(p)
EOF
}

get_prot_from_header_string() {
    echo "$(echo -e "$params_header" | awk 'NF' | head -n 1 | awk -F' ' '{print $NF}')"
}

# Only used with CuckooSync parameters
get_bucket_size() {
    echo -e "$1" | grep "bucketSize" | awk -F' ' '{print $NF}'
}

# Gets original header text and the path to the param file
# Returns the new header text where SET_OPTIMAL is replaced appropriately
calc_set_optimal_subs() {
    sync_prot="$(get_prot_from_header_string)"

    case $sync_prot in
        1 | 8) # CPISync or IBLTSync
            read -a common_ret <<< "$(call_common_el $2)"
            optimal=$((${common_ret[1]} + ${common_ret[2]} + 1)) # plus 1!
            header_text="$(echo -e "$1" | sed "s/SET_OPTIMAL/$optimal/g")"
            echo -e "$header_text"
            ;;
        12) # CuckooSync
            read -a common_ret <<< "$(call_common_el $2)"
            my_set_size=${common_ret[3]}
            bucket_size=$(get_bucket_size "$1")
            needed_buckets=$(python -c "import math; print(math.ceil($my_set_size/$bucket_size))")
            optimal=$(next_power_of_two "$needed_buckets")
            header_text="$(echo -e "$1" | sed "s/SET_OPTIMAL/$optimal/g")"
            echo -e "$header_text"
            ;;
        *)  >&2 echo "Cannot calc_set_optimal_subs for protocol: $sync_prot"
            exit 1
    esac
}

# Gets the id from the name of a .cpisync file (server or client) id
# is normally the second part of the file name. For example
# server_10_10_100 (number of differences "_", again, cardinality of
# the sets)
infer_id_from_f_name() {
    f_name="$(basename $1)"
    file_name_pts=`echo $f_name | awk -F'_' '{ print NF }'`
    case $file_name_pts in
        3) id="$(echo $f_name | awk -F'_' '{ print $(NF-1)"_"$NF }')"
           ;;
        # When there are 4 parts, then the last part is the
        # cardinality of the sets
        4) id="$(echo $f_name | awk -F'_' '{ print $(NF-2)"_"$(NF-1)"_"$NF }')"
           ;;
        *) echo ""
    esac

    id=${id//.cpisync/}
    echo $id
}

# Remove header from the param file passed as the first parameter. If
# there is no header, terminate the program.
remove_header_from_param_file() {
    grep_ret=`grep -E -n "\-{30,}" $1`  # 30 or more repetitions of "-"
    grep_ret_parts=(${grep_ret//:/ })
    line_num=${grep_ret_parts[0]}

    sed -i -e "1,${line_num}d" $1
}

# Third parameter is optional and determines whether the optimal
# maximal number of mutual differences is used. It works only with
# CPISync-based parameter headers.
prepend_params() {
    sync_prot="$(get_prot_from_header_string)"
    # if header text contains "SET_OPTIMAL", anywhere
    infer_optimal="$(echo -e "$params_header" | grep "SET_OPTIMAL" || true)"

    # works only for some sync protocols
    if [ "$infer_optimal" ]; then
        case $sync_prot in
            1) ;;               # CPISync
            8) ;;               # IBLTSync
            12) ;;              # CuckooSync
            *) >&2 echo "Cannot infer optimal parameters for this sync protocol."
               exit 1
               ;;
        esac
    fi

    echo "Adding headers to raw data sets in $1, if needed..."

    for file in $params_dir/server*.cpisync; do
        # set only if not already set, or when user forces it
        if ! [[ $(head -n 1 $file) == "Sync protocol"* ]] \
                || [[ $force_prepend_params ]]; then
            header_text=$2
            if [ "$infer_optimal" ]; then
                header_text="$(calc_set_optimal_subs "$header_text" "$file")"
            fi

            # find the corresponding client file
            id="$(infer_id_from_f_name $file)"
            if [ -z "$id" ]; then
                echo "infer_id_from_f_name failed."
                exit 1
            fi
            cli_f=$(find $params_dir -name "client_$id.cpisync")

            # if forcing is enabled, first remove the existing headers
            if [[ $force_prepend_params ]]; then
                remove_header_from_param_file $file
                remove_header_from_param_file $cli_f
            fi

            # Add headers to the server file
            tmp_file=$(mktemp)
            echo -e "$header_text" | awk 'NF' | cat - $file > $tmp_file
            mv $tmp_file $file

            # Add headers to the client file
            if [[ ("$sync_prot" == "12") && "$infer_optimal" ]]; then
                # CPISync allows possibly different headers for server
                # and client
                header_text="$(calc_set_optimal_subs "$header_text" "$cli_f")"
            fi
            echo -e "$header_text" | awk 'NF' | cat - $cli_f > $tmp_file
            mv $tmp_file $cli_f
        fi
    done
}

push_and_run() {
    address_and_path=(${remote_path//:/ })
    address=${address_and_path[0]}
    path=${address_and_path[1]}

    ssh $address mkdir -p $path

    # Put CPISync source code on remote
    rsync -a --info=progress2   \
          --exclude build       \
          --exclude scripts     \
          --exclude '.*'        \
          --exclude *.deb       \
          --exclude *.rpm       \
          --exclude *.tar.gz    \
          $cpisync_path $remote_path

    rsync -a --info=progress2 \
          $mininet_path $remote_path/$(basename $mininet_path)
    rsync -a --info=progress2 \
          count_common.py $remote_path/count_common.py
    rsync -a --info=progress2 \
          extract_data.py $remote_path/extract_data.py

    if [[ $server_params_file && $client_params_file ]]; then
        rsync -a --info=progress2 \
              $server_params_file $remote_path/$(basename $server_params_file)
        rsync -a --info=progress2 \
              $client_params_file $remote_path/$(basename $client_params_file)
    else                        # We get data sets from a dir
        rsync -a --info=progress2 \
              $params_dir $remote_path
    fi
    rsync -a --info=progress2 \
          $(basename $0) $remote_path/$(basename $0)

    if ! [[ $prepare_only ]]; then
        # Build CPISync on remote
        ssh $address "cd $path
                  mkdir build && cd build
                  cmake -GNinja . ../
                  ninja"

        ssh -t $address "cd $path
                         timestamp=\$(date +%s)
                         sudo nohup ./$(basename $0) -q > nohup_\$timestamp.out &
                         echo \"~~~~~~~~> nohup.out:\"
                         tail -f -n 1 nohup_\$timestamp.out"
    fi
}

when_on_remote() {
    if [[ $server_params_file && $client_params_file ]]; then
        server_params_file=$(basename $server_params_file)
        client_params_file=$(basename $client_params_file)
    else
        params_dir=$(basename $params_dir)
    fi
    mininet_path=$(basename $mininet_path)
    benchmarks_path=./$(basename $cpisync_path)/build/Benchmarks
    python_path=/usr/bin/python
}

print_common_el() {
    common_script_out="$(call_common_el $1)"
    read -a cmn_out <<< $common_script_out
    echo -e "\n--------------------------- GROUND TRUTH ABOUT SETS ---------------------------"
    echo -e "|s intersect c|: ${cmn_out[0]}, |s \ c|: ${cmn_out[1]}, |c \ s|: ${cmn_out[2]}"
    echo -e "|(s)erver|: ${cmn_out[3]}, |(c)lient|: ${cmn_out[4]}"
    echo -e "As per $server_params_file"
    echo -e "-------------------------------------------------------------------------------\n"
}

# Runs an experiment on the bare machine, ignoring mininet
run_plain() {
    if [[ $chunk_size ]]; then
        $benchmarks_path -i $chunk_size -m "server" -p $1 &
        $benchmarks_path -i $chunk_size -m "client" -r localhost -p $2
    else
        $benchmarks_path -m "server" -p $1 &
        $benchmarks_path -m "client" -r localhost -p $2
    fi
}

run_mininet_exec() {
    # Assure that Mininet resources are cleaned
    sudo mn -c
    # echo -e "\n>>>>>>>>>>>>>>>>>>>> Sleeping for 10 seconds...\n"
    # sleep 10

    server_script="$benchmarks_path -p $1 -m server"
    client_script="$benchmarks_path -p $2 -m client -r 192.168.1.1"

    if [[ $chunk_size ]]; then
        server_script="${server_script} -i $chunk_size"
        client_script="${client_script} -i $chunk_size"
    fi

    sudo $python_path $mininet_path    \
         --latency $latency            \
         --bandwidth $bandwidth        \
         --packet-loss $packet_loss    \
         --cpu-server $cpu_server      \
         --cpu-client $cpu_client      \
         "$server_script" "$client_script"
}

pull_from_remote() {
    rsync --recursive --relative --info=progress2 $1 DATA/
}

pull_csv() {
    full_path_to_experiemnts_dir=$1

    address_and_path=(${full_path_to_experiemnts_dir//:/ })
    address=${address_and_path[0]}
    path=${address_and_path[1]}

    ssh -t $address "cd $path; $python_path extract_data.py $path"

    csv_file_name="$(basename $path)".csv

    rsync -a --info=progress2 \
          $full_path_to_experiemnts_dir/$csv_file_name $csv_file_name
}

################################ FUNCTIONS END #################################

while getopts "hqsifr:p:" option; do
    case $option in
        s) prepare_only=yes
           ;;
        r) remote_path=$OPTARG
           ;;
        p) pull_csv $OPTARG
           exit
           ;;
        pp) pull_from_remote $OPTARG
            exit
            ;;
        # When we are on remote, all the needed parts are in the same directory
        q) # Build the source code on remote
           rm -r build || true && mkdir build && cd build && cmake .. && make -j$(nproc) && cd ..
           when_on_remote
           we_are_on_remote=yes
           ;;
        i) ignore_mininet=yes
           ;;
        f) force_prepend_params=yes
           ;;
        h|*) help
             exit
    esac
done

if [ $remote_path ]; then
    if [[ $params_dir && $params_header ]]; then
        prepend_params "$params_dir" "$params_header"
    fi

    push_and_run
    exit
fi

if ! [[ $we_are_on_remote ]]; then
    # When we are on remote, it is set appropriately in when_on_remote
    benchmarks_path=$cpisync_path/build/Benchmarks

    # Parameters that do not make sense
    if ! [[ ($server_params_file && $client_params_file) \
                || ($params_dir && $params_header) ]]
    then
        echo "You either need to specify server_params_file and client_params_file,"
        echo "or params_dir and params_header"
        exit 1
    fi
fi

if ! [[ -f $mininet_path ]]; then
    echo "Mininet path does not exist: $mininet_path"
    exit 1
fi
if ! [[ -f $python_path ]]; then
    echo "Python path does not exist: $python_path"
    exit 1
fi

# Prepare .cpisync param files if only the raw params_dir data is passed
# Always done locally
if [[ $params_dir && $params_header ]] \
       && ! [[ $we_are_on_remote ]]
then
    prepend_params "$params_dir" "$params_header"
fi

# reset log directories
rm -rf '.cpisync'
sudo rm -rf '.mnlog'

if [[ $server_params_file && $client_params_file ]]; then
    print_common_el $server_params_file

    for i in `seq $repeat`
    do
        echo "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
        echo "Experiment run $i: [`date`]"
        echo "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
        run_mininet_exec $server_params_file $client_params_file
    done

    echo "Completed $repeat mininet_exec calls!"
    exit                        # finish here!
fi

# Otherwise, we're loading data from a params_dir
for p_file in $params_dir/*.cpisync; do
    # Obtain server-client .cpisync param file pairs
    if [[ $p_file == *"server"* ]]; then
        id="$(infer_id_from_f_name $p_file)"
        if [ -z "$id" ]; then
            echo "infer_id_from_f_name failed."
            exit 1
        fi
        server_params_file=$p_file
        client_params_file=$(find $params_dir -name "*client*$id.cpisync")
    else
        continue
    fi

    print_common_el $server_params_file

    echo "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
    echo "Experiment run for $server_params_file: [`date`]"
    echo "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"

    # Run experiment loop
    for i in `seq $repeat`
    do
        echo "%%%%%%%% Repetition #$i [`date`]"

        if [[ $ignore_mininet ]]; then
            run_plain $server_params_file $client_params_file
        else
            run_mininet_exec $server_params_file $client_params_file
        fi

        # All the observations in .cpisync are after chunk_size added elements
        if [[ $chunk_size ]]; then
            if [[ -d .cpisync ]]; then
                mv .cpisync .cpisync_rep_${i}_chunks_${chunk_size}_${id}
            else
                echo "run_experiments.sh: ERROR: No .cpisync after iteration $i. See .mnlog"
                exit 1
            fi
        fi
    done

    # post processing
    if [[ -d .cpisync ]]; then
        mv .cpisync .cpisync_$id
    elif ! [[ $chunk_size ]]; then
        # .cpisync is not there because it's already renamed above
        echo "run_experiments.sh: ERROR: No .cpisync directory, something went wrong. See .mnlog"
        exit 1
    fi
done

echo "Completed run_experiments job!"
