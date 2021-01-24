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
# how many times to repeat the same experiment
repeat=1

# Define either server and client files...
# server_params_file=server_params_data_IBLTSync_optimal.cpisync
# client_params_file=client_params_data_IBLTSync_optimal.cpisync

# ... or the directory where to find the data sets, and a .cpisync header
# If params_header contains SET_OPTIMAL, the script tries to do so.
params_dir=/home/novak/Desktop/CODE/btc-analysis/cpisync_ready
params_header="
Sync protocol (as in GenSync.h): 1
m_bar: SET_OPTIMAL
bits: 64
epsilon: 3
partitions/pFactor(for InterCPISync): 0
hashes: false
Sketches:
--------------------------------------------------------------------------------"

# network parameters
latency=20
bandwidth=50
packet_loss=1
cpu_server=30
cpu_client=30

# where to obtain needed executables
mininet_path=~/Desktop/playground/mininet_exec/mininet_exec.py
python_path=/home/novak/.virtualenvs/statistics/bin/python
benchmarks_path=~/Desktop/CODE/cpisync/build/Benchmarks
################################ PARAMETERS END ################################

help() {
    echo -e "USAGE: ./run_experiments.sh [-q] [-s] [-r REMOTE_PATH] [-p PULL_REMOTE]\n"
    echo -e "If remote machine is used, it needs Mininet and all the CPISync dependencies."
    echo -e "CPISync source code will be compiled on the remote machine.\n"
    echo "OPTIONS:"
    echo "    -r REMOTE_PATH the path on remote to copy all the needed parts."
    echo "    -p PULL_REMOTE pull from here to my DATA/. Used to gather data from experimetns."
    echo "    -s used with -r when we only want to prepare an experiment on remote but not to run it."
    echo "    -q when this script is run on a remote set this."
    echo -e "\nEXAMPLES:"
    echo "    # runs locally"
    echo "    ./run_experiments.sh"
    echo "    # runs on remote"
    echo "    ./run_experiments.sh -r remote_name:/remote/path"
    echo "    # pulls the experimental data from the remote. Creates DATA/CPISync/1/.cpisync."
    echo "    ./run_experiments.sh -p remote_name:/home/novak/EXPERIMENTS/./CPISync/1/.cpisync"
}

push_and_run() {
    if ! [[ $benchmarks_path == *"build"* ]]; then
        echo "benchmark_path not in cpisync/build, cannot infer cpisync path from it."
        exit 1
    fi

    cpisync_dir="$(cd $(dirname $benchmarks_path); cd ..; pwd)"

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
          $cpisync_dir $remote_path

    rsync -a --info=progress2 \
          $mininet_path $remote_path/$(basename $mininet_path)
    rsync -a --info=progress2 \
          count_common.py $remote_path/count_common.py
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

    # Build CPISync on remote
    ssh $address "cd $path/cpisync
                         mkdir build && cd build
                         cmake -GNinja . ../
                         ninja"

    if ! [[ $prepare_only ]]; then
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
    benchmarks_path=./cpisync/build/$(basename $benchmarks_path)
}

pull_from_remote() {
    rsync --recursive --relative --info=progress2 $1 DATA/
}

while getopts "hqsr:p:" option; do
    case $option in
        s) prepare_only=yes
           ;;
        r) remote_path=$OPTARG
           ;;
        p) pull_from_remote $OPTARG
           exit
           ;;
        # When run on remote, all the needed parts are in the same directory
        q) when_on_remote
           we_are_on_remote=yes
           ;;
        h|*) help
             exit
    esac
done

if [ $remote_path ]; then
    push_and_run
    exit
fi

if ! [[ -f $mininet_path ]]; then
    echo "Mininet path does not exist: $mininet_path"
    exit 1
fi
if ! [[ -f $python_path ]]; then
    echo "Python path does not exist: $python_path"
    exit 1
fi
if ! [[ -f $benchmarks_path ]]; then
    echo -e "Benchmark executable does not exist: $benchmarks_path"
    exit 1
fi
if ! [[ ($server_params_file && $client_params_file) \
            || ($params_dir && $params_header) ]]
then
    echo "You either need to specify server_params_file and client_params_file,"
    echo "or params_dir and params_header"
    exit 1
fi

call_common_el() {
    echo "$(./count_common.py $1 -- "--------")"
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

# Third parameter is optional and determines whether the optimal
# maximal number of mutual differences is used. It works only with
# CPISync-based parameter headers.
prepend_params() {
    sync_prot="$(echo -e "$params_header" | awk 'NF' | head -n 1 | awk -F' ' '{print $NF}')"
    # if header text contains "SET_OPTIMAL", anywhere
    infer_optimal=`echo -e "$params_header" | grep "SET_OPTIMAL" || true`

    # works only for CPISync-based parameters
    if [ "$infer_optimal" ]; then
        if ! [[ $sync_prot == "1" ]]; then
            echo "Cannot infer optimal parameters for this sync protocol."
            exit 1
        fi
    fi

    echo "Adding headers to raw data sets in $1, if needed..."

    for file in $params_dir/server*.cpisync; do
        # set only if not already set
        if ! [[ $(head -n 1 $file) == "Sync protocol"* ]]; then
            header_text=$2
            if [ "$infer_optimal" ]; then
                read -a common_ret <<< "$(call_common_el $file)"
                optimal=$((${common_ret[1]} + ${common_ret[2]} + 1)) # plus 1!
                header_text="$(echo -e "$header_text" | sed "/m_bar/c\m_bar: $optimal")"
            fi

            # find the corresponding client file
            id="$(echo $file | awk -F'_' '{ print  $(NF-1)"_"$NF }')"
            id=${id//.cpisync/}
            cli_f=$(find $params_dir -name "client*$id*.cpisync")

            tmp_file=$(mktemp)
            both_files=($file $cli_f)
            for file in ${both_files[@]}; do
                echo -e "$header_text" | awk 'NF' | cat - $file > $tmp_file
                mv $tmp_file $file
            done
        fi
    done
}

# Prepare .cpisync param files if only the raw params_dir data is passed
# Always done locally
if [[ $params_dir && $params_header ]] \
       && ! [[ $we_are_on_remote ]]
then
    prepend_params "$params_dir" "$params_header"
fi

# Cleanup Mininet environment
sudo mn -c

# reset log directories
rm -rf '.cpisync'
sudo rm -rf '.mnlog'

run_mininet_exec() {
    sudo $python_path $mininet_path                         \
         --latency $latency                                 \
         --bandwidth $bandwidth                             \
         --packet-loss $packet_loss                         \
         --cpu-server $cpu_server                           \
         --cpu-client $cpu_client                           \
         "$benchmarks_path -p $1 -m server"                 \
         "$benchmarks_path -p $2 -m client -r 192.168.1.1"
}

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
        id="$(echo $p_file | awk -F'_' '{ print $(NF-1)"_"$NF }')"
        id=${id//.cpisync/}
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
        echo "%%%%%%%% Repetition #$i"
        run_mininet_exec $server_params_file $client_params_file
    done

    # post processing
    if [[ -d .cpisync ]]; then
        mv .cpisync .cpisync_$id
    else
        echo "run_experiments.sh: ERROR: No .cpisync directory, something went wrong. See .mnlog"
    fi
done

echo "Completed mininet_exec job!"
