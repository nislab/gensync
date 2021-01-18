#!/usr/bin/env bash
set -e

############################### PARAMETERS BEGIN ###############################
# how many times to repeat the same experiment
repeat=10

# CPISync related parameters files
server_params_file=server_params_data.cpisync
client_params_file=client_params_data.cpisync

# network parameters
latency=20
bandwidth=50
packet_loss=2
cpu_server=30
cpu_client=30

# where to obtain needed executables
mininet_path=~/Desktop/playground/mininet_exec/mininet_exec.py
python_path=~/.virtualenvs/statistics/bin/python
benchmarks_path=~/Desktop/CODE/cpisync/build/Benchmarks
################################ PARAMETERS END ################################

help() {
    echo -e "USAGE ./run_experiments.sh [-r REMOTE_PATH]\n"
    echo "OPTIONS:"
    echo "    -r REMOTE_PATH the path on remote to copy all the needed parts."
    echo "    -q when this script is run on remote set this."
}

while getopts "r:qh" option; do
    case $option in
        r) remote_path=$OPTARG

           address_and_path=(${remote_path//:/ })
           address=${address_and_path[0]}
           path=${address_and_path[1]}

           ssh $address mkdir -p $path

           rsync -a --info=progress2 \
                 $mininet_path $remote_path/$(basename $mininet_path)
           rsync -a --info=progress2 \
                 $benchmarks_path $remote_path/$(basename $benchmarks_path)
           rsync -a --info=progress2 \
                 count_common.py $remote_path/count_common.py
           rsync -a --info=progress2 \
                 $server_params_file $remote_path/$(basename $server_params_file)
           rsync -a --info=progress2 \
                 $client_params_file $remote_path/$(basename $client_params_file)
           rsync -a --info=progress2 \
                 $0 $remote_path/$(basename $0)

           ssh -t $address "sudo mn -c
                            cd $path
                            nohup ./$(basename $0) -q &
                            echo \"~~~~~~~~> nohup.out:\"
                            tail -f -n 1 $path/nohup.out"
           exit
           ;;
        q) on_remote=yes
           ;;
        h|*) help
             exit
    esac
done

# When run on remote, all the needed parts are in the same directory
if [ $on_remote ]; then
    server_params_file=$(basename $server_params_file)
    client_params_file=$(basename $client_params_file)
    mininet_path=$(basename $mininet_path)
    benchmarks_path=$(basename $benchmarks_path)
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
if ! [[ -f $server_params_file ]]; then
    echo "No file: $server_params_file"
    exit 1
fi
if ! [[ -f $client_params_file ]]; then
    echo "No file: $client_params_file"
    exit 1
fi

# reset log directories
rm -rf '.cpisync'
sudo rm -rf '.mnlog'

# What is the actual common elements count
echo "$(./count_common.py $server_params_file -- "--------") common elements."
echo -e "As per $server_params_file\n"

mininet_exec="sudo $python_path $mininet_path                                            \
                   --latency $latency                                                    \
                   --bandwidth $bandwidth                                                \
                   --packet-loss $packet_loss                                            \
                   --cpu-server $cpu_server                                              \
                   --cpu-client $cpu_client                                              \
                   \"$benchmarks_path -p $server_params_file -m server\"                 \
                   \"$benchmarks_path -p $client_params_file -m client -r 192.168.1.1\""

echo "Mininet command:"
echo $mininet_exec

for i in `seq $repeat`
do
    echo "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
    echo "Experiment run: $i [`date`]"
    echo "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%"
    eval $mininet_exec
done

echo "Completed $repeat mininet_exec calls!"
