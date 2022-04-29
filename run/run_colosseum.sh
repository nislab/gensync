#!/usr/bin/env bash
#
# Author: Novak Boškov <boskov@bu.edu>
# Date: March, 2022.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

set -e
trap "exit 1" TERM
export TOP_PID=$$

requirements=( sudo ssh sshpass rsync lxc lxd )

######## Global constants
default_team_name=sync-edge                     # Colosseum team name
default_srn_user_passwd="Spiteful Corgi Bites"  # password for `root` and `srn_user` in containers
default_base_image=scope.tar.gz                 # base image when `-c`
default_modified_image=gensync-colosseum        # name of produced image when `-c`
default_shared_path=/share/exec                 # where containers mount the shared NAS
default_colosseumcli_path=colosseumcli          # path to `colosseumcli` install artifacts
default_instance=colosseum-instance             # temporary container name
default_net_interface=srs                       # network interface in the container to sync through
default_sleep_before_gensync=30                 # seconds to wait for radio hardware to get set up
default_data_loc=/share/gensync_data            # topmost dir of GenSync data input files
default_experiment_rep=1                        # number of experiment repetitions for each GenSync data file

######## Handle env variables
team_name=${team_name:="$default_team_name"}
srn_user_passwd=${srn_user_passwd:="$default_srn_user_passwd"}
base_image=${base_image:="$default_base_image"}
modified_image=${modified_image:="$default_modified_image"}
shared_path=${shared_path:="$default_shared_path"}
colosseumcli_path=${colosseumcli_path:="$default_colosseumcli_path"}
instance=${instance:="$default_instance"}
net_interface=${net_interface:="$default_net_interface"}
sleep_before_gensync=${sleep_before_gensync:="$default_sleep_before_gensync"}
data_loc=${data_loc:="$default_data_loc"}
experiment_rep=${experiment_rep="$default_experiment_rep"}

mapfile -t custom_vars \
        < <(( set -o posix; set ) | grep 'default_' | sed 's/default_//g')

######## Calculated constants
gensync_path="`cd ..; pwd`"
gensync_basename="$(basename "$gensync_path")"

######## Extended commands
lxc="sudo lxc"

help() {
    cat<<EOF
USAGE: run_colosseum [-h] [-c] [-u IMAGE] [-p SOURCE DESTINATION [CONTAINER]] SERVER CLIENT BASE_STATION SCENARIO_ID

Executes GenSync synchornization on Colosseum wireless network emulator.

Requirements: ${requirements[@]}
You need be behind Colosseum's VPN and have your Colosseum remotes set in ~/.ssh/config.

SERVER and CLIENT are the two hostnames of Colosseum SRN's among which you want to perform pair-wise sync.
BASE_STATION is the hostmane of the base station node.
SCENARIO_ID is the Colosseum scenario identifier.

OPTIONS:
    -h Show this message and exit.
    -c Create GenSync container image '$modified_image.tar.gz' and exit
       (installs 'colosseumcli' down the road).
    -u Similar to -c but uploads GenSync code to an existing IMAGE instead of creating the new one.
    -p Push SOURCE container image to DESTINATION (typically at file-proxy:).
       If CONTAINER is passed, it must be a running container's name to snapshopt into SOURCE
       and push to DESTINATION.

You can custmize the following script variables:
$( for ((i = 0; i < ${#custom_vars[@]}; i++)) do echo ${custom_vars[$i]}; done )
EOF
}

# Make sure that all requirements are available
# No arguments
check_requirements() {
    for r in ${requirements[@]}; do
        if ! command -v $r 2>&1 >/dev/null; then
            err "'$r' is not installed. Please install it."
        fi
    done
}

# Echo green bold text
# $1 text to output to standard output
echo_g() {
    echo -e "\e[32;1m$1\e[0m"
}

# Echo red bold text
# $1 text to print to the standard output
echo_r() {
    echo -e "\e[31;1m$1\e[0m"
}

# Echo grey bold text
# $1t ext to print to standard output
echo_o() {
    echo -e "\e[33;1m$1\e[0m"
}

# Echo the error message and exit.
# $1 error message (optional)
err() {
    if ! [ -z "$1" ]; then echo_r "Error: $1"; fi
    kill -s TERM $TOP_PID
    exit
}

# Build `lxc` execution command.
# No arguments
get_lxc_exec() {
    echo "$lxc exec $instance -- bash -c"
}

# Obtain the IP address of a Colosseum host.
# $1 host name
get_ip() {
    local host_name="$1"

    sshpass -e ssh root@"$host_name"       \
            "ifconfig                      \
             | grep -A1 $net_interface     \
             | awk '/inet/ { print \$2 }'  \
             | sed 's/[^0-9.]//g'"
}

# Obtain GenSync compile commands.
# No arguments
get_compile_cmd() {
    echo "cd /$gensync_basename
          rm -rf build || true
          rm CMakeCache.txt || true
          mkdir build && cd build
          cmake -DIGNORE_TEST=True ../ && make -j\$(nproc)"
}

# Obtain nanoseconds since epoch
# No arguments
get_current_date() {
    date +%N
}

# Stop the container and export it to a `.tar.gz` file as an image.
# $1 instance to export
# $2 the name for the modified image
export_modified_image() {
    $lxc stop "$1"
    printf "\nPublishing '$2' ...\n"
    $lxc publish "$1" --alias "$2"
    printf "\nExporting '$2' ...\n"
    $lxc image export "$2" "$2"
}

# Install `colosseumcli` in the container (container must be running).
# $1 running instance name
install_colosseumcli() {
    local lxc_exec="$(get_lxc_exec)"
    local cli_basename="$(basename "$colosseumcli_path")"

    if ! $lxc_exec "command -v colosseumcli"; then
        # In case `colosseumcli` is not already installed
        printf "\nInstalling 'colosseumcli' in '$1' container...\n"
        $lxc file push -rp "$colosseumcli_path" "$1"/tmp/
        $lxc_exec \
            "cd /tmp/'$cli_basename'
               tar zxvf colosseum_cli_prereqs.tar.gz
               tar zxvf colosseumcli-18.05.0-3.tar.gz
               cp -r colosseum_cli_prereqs /root/
               cp -r colosseumcli-18.05.0 /root/
               cd /root/colosseum_cli_prereqs
               ./install_prereqs.sh
               cd ../colosseumcli-18.05.0
               python3 setup.py install
               rm -rf /root/colosseum_cli_prereqs /root/colosseumcli-18.05.0"
        echo_g "\n'colosseumcli' successfully installed.\n"
    else
        printf "\n'colosseumcli' already installed in '$1'.\n"
    fi
}

# Kill `unattended-upgrades` so we can obtain the lock when we need it.
# $1 the number of repeated tries to kill unattended-upgrades
get_rid_of_unattended_upgrades() {
    local repeat=5
    if ! [ -z $1 ]; then repeat=$1; fi

    echo "for i in {1..$repeat}; do lslocks | awk '/unattended-upgrades.lock/ { print \$2 }' | xargs kill -9; sleep 1; done || true"
}

# Push GenSync codebase to the container, but try to make it a little faster.
# $1 path to dir that contains `.tar.gz` files to shuffle.
lxc_file_push_with_fixup() {
    local t=$(mktemp -d)

    mv "$1"/*.tar.gz $t 2>/dev/null || true
    $lxc file push -rp "$gensync_path"/ "$instance"/
    mv $t/*.tar.gz "$1" 2>/dev/null || true
    rm -r $t 2>/dev/null || true
}

# Change the password of the `srn-user` and `root` users
# No arguments
change_passwd() {
    local lxc_exec="$(get_lxc_exec)"

    printf "\nChanging 'srn-user' and 'root' passwords in the container to '$srn_user_passwd' ... \n"
    $lxc_exec "echo -e '$srn_user_passwd\n$srn_user_passwd' | passwd srn-user"
    $lxc_exec "echo -e '$srn_user_passwd\n$srn_user_passwd' | passwd root"
}

# Push the container image to the shared storage on Colosseum.
# $1 path to image file
# $2 path where to push it on remote
# $3 currently running container name. If passed, snapshot it into $1 and push it to $2.
push_image() {
    local source="$modified_image".tar.gz
    if ! [ -z "$1" ]; then source="$1"; fi
    local source_base_name="$(echo $source | cut -d . -f 1)"
    local image_remote_loc="file-proxy:/share/nas/$team_name/images"
    if ! [ -z "$2" ]; then image_remote_loc="$2"; fi
    local remote_path="$(echo $image_remote_loc | awk -F: '{ print $2 }')"

    if ! [ -z "$3" ]; then
        echo "Exporting '$3' into '$source_base_name'..."
        $lxc stop "$3" || err
        $lxc publish "$3" --alias "$source_base_name" || err
        $lxc image export "$source_base_name" "$source_base_name" || err
    fi

    printf "Uploading image to Colosseum testbed @ $image_remote_loc ...\n"
    sshpass -e rsync -Pav "$source" "$image_remote_loc" || err
    printf "\nFixing container permissions on Colosseum testbed ...\n"
    sshpass -e ssh file-proxy "chmod 755 $remote_path/$source" || err
}

# Prepare a container image and upload it to Colosseum.
# No arguments
setup() {
    local lxc_exec="$(get_lxc_exec)"

    if [ -e "$modified_image.tar.gz" ]; then
        printf "'$modified_image.tar.gz' already exists.\n"
        return
    fi

    # Download base image if needed
    if ! [ -e "$base_image" ]; then
        printf "\n Pulling up '$base_image' from file-proxy ... \n"
        rsync -Pv file-proxy:/share/nas/common/"$base_image" .
    fi

    # Import container image from file
    printf "\nImporting '$base_image' ...\n"
    $lxc image delete colosseum-base 2>/dev/null || true
    $lxc image import "$base_image" --alias colosseum-base

    # Launch container from the image
    $lxc delete --force "$instance" 2>/dev/null || true
    $lxc launch colosseum-base "$instance"

    change_passwd

    # Get rid of unattended-upgrades
    echo_o "\nGetting rid of unattended-upgrades ...\n"
    $lxc_exec "$(get_rid_of_unattended_upgrades)"

    # Install NTL and its dependencies
    $lxc_exec "apt update -y; apt install -y libntl-dev libgmp3-dev libcppunit-dev"

    # If the image is older than Ubuntu 18.04, we can attempt an upgrade
    # TODO 1: communicate the issue with Colosseum folks and use newer images.
    local img_ver="$($lxc_exec \
    "lsb_release -a 2>/dev/null | awk '/Release/ { print \$2 }' | awk -F. '{ print \$1 }'")"

    if [[ "$img_ver" -lt 18 ]]; then
        echo -e "\n"
        cat <<EOF
Ubuntu '$img_ver' major version image detected.
This Ubuntu version is too old for GenSync.
I can try container upgrade in a VERY LONG interactive session.
EOF
        read -p "Do you want to proceed? (y/N): " yn
        case $yn in
            [Yy]* ) ;;
            *) exit
        esac

        # Update and upgrade the current Ubuntu version
        $lxc_exec "apt update -y; apt upgrade -y;"
        $lxc restart "$instance"

        # Upgrade to a newer Ubuntu version
        $lxc exec "$instance" -- script /dev/null -c \
             "sleep 5; apt update -y; apt upgrade -y; do-release-upgrade -m server"
        $lxc restart "$instance"

        # Fix the Python pip issues
        echo -e "\nFixing Python pip issues and transitioning to python3.6...\n"
        $lxc_exec "wget https://bootstrap.pypa.io/pip/3.6/get-pip.py
                   python3 get-pip.py"
        # Install dependencies needed for `SCOPE`
        echo -e "\nInstalling 'SCOPE' framework dependencies...\n"
        $lxc_exec "pip install dill numpy"

        printf "\nThis was my best effort. The container is now at:\n\n"
        $lxc_exec "lsb_release -a 2>/dev/null"
        read -p "Do you want to proceed with this version of Ubuntu? (Y/n): " yn
        case $yn in
            [Nn]* ) exit
        esac
    fi

    # Transfer GenSync codebase to the container
    lxc_file_push_with_fixup "$gensync_path"/run
    $lxc_exec "rm -f /$gensync_basename/run/*.tar.gz || true"

    # Compile GenSync in the container
    $lxc_exec "$(get_compile_cmd)"

    install_colosseumcli "$instance"

    export_modified_image "$instance" "$modified_image"

    # Cleanup
    printf "\nCleanup ...\n"
    $lxc delete --force "$instance"
    $lxc image delete colosseum-base "$modified_image"

    push_image
}

# Load the image and refresh the GenSync version in it.
# $1 image name
refresh_code() {
    local image="$(basename "$1" .tar.gz)"
    local lxc_exec="$(get_lxc_exec)"
    local is_imported=$($lxc --format=csv image ls | awk -F, '{ print $1 }' | grep -c "^$image\$")

    if [[ $is_imported -eq 0 ]]; then
        # Image is not imported. Is it at least on the disk?
        if [[ -e "$image".tar.gz ]]; then
            printf "\nImporting '$image' ...\n"
            $lxc image import "$image".tar.gz --alias "$image"
        else
            echo_r "\nCannot find '$image' image.\n"
            exit
        fi
    fi

    $lxc delete --force "$instance" 2>/dev/null || true
    $lxc launch "$image" "$instance"

    change_passwd

    # Make sure that NTL is installed
    found=$($lxc_exec "find / -type f -name libntl.a 2>/dev/null | wc -l")
    if [ "$found" -eq "0" ]; then
        echo_o "\nNTL is not installed in the '$image'. Installing now ...\n"
        $lxc_exec "sleep 2; apt update -y"
        $lxc_exec "$(get_rid_of_unattended_upgrades)"
        echo -e "That was my best effort to kill 'unattended upgrades'.\n"
        $lxc_exec "apt install -y libgmp3-dev
                   wget https://libntl.org/ntl-10.5.0.tar.gz
                   gunzip ntl-10.5.0.tar.gz
                   tar xf ntl-10.5.0.tar
                   cd ntl-10.5.0/src
                   ./configure
                   make -j\$(nproc) && make check && make install"
    fi

    # Copy code and compile it in the container
    $lxc_exec "rm -rf /$gensync_basename"
    lxc_file_push_with_fixup "$gensync_path"/run
    $lxc_exec "rm -f /$gensync_basename/run/*.tar.gz || true"
    $lxc_exec "$(get_compile_cmd)"

    export_modified_image "$instance" "$image"_"$(get_current_date)"

    # Cleanup
    $lxc delete --force "$instance"
}

# Ask for Colosseum SSH password
# $1 if passed, ask for the ssh password for that container
ask_ssh_pass() {
    if ! [ -z "$1" ]; then
        echo_o "This script assumes that all '$1' containers have the same SSH password."
        echo -n "Enter password: "
    else
        echo -n "This action will require your Colosseum SSH password: "
    fi

    read -s pass
    echo -e "\n"
    export SSHPASS="$pass"
}

# Setup scenario and srsLTE on Colosseum using SCOPE
# $1 host name through which we want to conduct configuration
# $2 scenario identifier (when a call uses this argument, we're setting up a base station)
setup_colosseum() {
    local host="$1"
    local scenario="$2"
    local cmd="sshpass -e ssh root@$host"

    echo_o "Setting up SCOPE on '$host' ..."

    if ! [ -z "$scenario" ]; then
        echo -e "$host is being set up as a base station.\nRunning scenario: $scenario"
        $cmd "colosseumcli rf start $scenario -c; colosseumcli rf info"
    fi

    # Clear stale configuration
    $cmd "cd /root/radio_code/scope_config;
          ./remove_experiment_data.sh || true"

    # Run SCOPE
    $cmd "cd /root/radio_api/;
          python3 scope_start.py --config-file radio_interactive.conf 2>&1 | tee scope_start.log"
}

# Discover the first two working SRNs in the current reservation. An
# SRN is considered functional if `get_ip` returns something.
# $1 array of length two where first is server host and second is client host
# ${@:2} hosts to consider
# Return via $1
dicover_two_working_hosts() {
    declare -n ret="$1"
    ret=( )
    local potential_hosts="${@:2}"

    echo_o "Discovering two working SRNs in your reservation ..."

    for h in ${potential_hosts[@]}; do
        if [ "${#ret[@]}" -eq 2 ]; then break; fi
        local ip="$(get_ip $h)"
        if ! [ -z "$ip" ]; then
            ret+=( "$h" )
        fi
    done

    if [ "${#ret[@]}" -lt 2 ]; then
        err "There are no two functional SRNs among: ${potential_hosts[@]}"
    fi
}

# Return absolute paths to all GenSync server data files in the given
# directory on the container (recursively).
# $1 container host name
# $2 data dir location
get_server_data_files() {
    local host="$1"
    local data_dir="$2"

    sshpass -e ssh srn-user@"$host" \
            "find '$data_dir' -type f -name '*server*.cpisync' 2>/dev/null"
}

# Find the GenSync client data file that corresponds to the given server data file
# $1 container host name
# $2 full server data file path
# $3 full client data file path
# Return via $3
get_client_data_file_for() {
    local host="$1"
    local server_file_path="$2"
    declare -n client="$3"

    local sf_dir="$(dirname $server_file_path)"
    local sf_name="$(basename $server_file_path)"
    local sf_name_ne="${sf_name%.*}"
    local sf_suffix="$(echo $sf_name_ne | cut -d '_' -f2,3)"
    local find_cmd="find $sf_dir -type f -name 'client*${sf_suffix}.cpisync'"

    client="$(sshpass -e ssh srn-user@$host "$find_cmd")"

    if [ -z "$client" ]; then
        err "There is no matching client file for $server_file_path"
    fi
}

# Execute GenSync experiments on Colosseum.
# $1 base station host
# ${@:2:(( $# - 2 ))} other hosts (first working host is the server, the next is the client)
# ${@: -1} scenario id
exec_on_colosseum() {
    local base_station_host="$1"
    local scenario_id="${@: -1}"
    local other_hosts="${@:2:(( $# - 2 ))}"
    local copy_dest="$shared_path/gensync_$(get_current_date)_$scenario_id"
    local scenario_info_path="$copy_dest"/scenario.info
    local benchmark_path="$copy_dest/build/Benchmarks"
    local examples_path="$copy_dest/example"

    # Setup base station and all other hosts
    setup_colosseum "$base_station_host" "$scenario_id"
    for h in ${other_hosts[@]}; do
        setup_colosseum "$h"
    done

    echo_o "Waiting $sleep_before_gensync seconds for radios to get set up ..."
    sleep $sleep_before_gensync

    dicover_two_working_hosts hosts "${other_hosts[@]}"
    local server_host=${hosts[0]}
    local client_host=${hosts[1]}

    # Get server and client IPs
    local server_ip="$(get_ip $server_host)"
    local client_ip="$(get_ip $client_host)"
    if [ -z "$server_ip" ]; then
        err "Server's IP is not detected for server host '$server_host'."
    fi
    if [ -z "$client_ip" ]; then
        err "Client's IP is not detected for client host '$client_host'."
    fi
    echo -e \
         "\nExecuting GenSync experiments with:"              \
         "\n\tServer IP: '$server_ip', HOST: '$server_host'"  \
         "\n\tClient IP: '$client_ip', HOST: '$client_host'"

    # Remember the scenario
    echo_o "\nResults will be placed in '$copy_dest', scenario:"
    sshpass -e ssh srn-user@"$base_station_host" \
            "mkdir -p $copy_dest;
             colosseumcli rf info | tee $scenario_info_path"

    # Move the code to the shared location (it's enough to do this on the server only)
    echo -e "\nCopying artifacts to '$copy_dest' ...\n"
    sshpass -e ssh srn-user@"$server_host" \
            "cp -r /'$gensync_basename'/* '$copy_dest'/"

    # Execute GenSync for all parameter files in `$data_loc`
    local data_files="$(get_server_data_files $server_host $data_loc)"
    for server_f in ${data_files[@]}; do
        echo "----> [$(date)] Start GenSync for server data file: $server_f"

        get_client_data_file_for "$server_host" "$server_f" client_f
        echo "----> Matching client file: $client_f"

        local server_f_bn="$(basename $server_f)"
        local client_f_bn="$(basename $client_f)"
        local sf_basename=${server_f_bn%.*}
        local sf_dirname="$(dirname $server_f)"
        local sf_path="$(echo $sf_dirname | sed 's/\//_/g')"
        local suffix="${sf_path}_${sf_basename}"

        # Temporarily move parameter files to `$copy_dest` and fix
        # permissions (due to messed up permissions in the container)
        sshpass -e ssh srn-user@"$server_host" \
                "cd '$copy_dest'
                 cp '$server_f' '$client_f' .
                 chmod 777 '$server_f_bn' '$client_f_bn'"

        # Repeat experiments for the same parameter files
        for rep in $(seq $experiment_rep); do
            echo "--------> Repetition $rep"
            # TODO 2: there should be an easier way to execute commands in a container
            sshpass -e ssh srn-user@"$server_host" \
                    "cd '$copy_dest'
                     '$benchmark_path' -m server -p '$server_f_bn'" &
            sshpass -e ssh srn-user@"$client_host" \
                    "cd '$copy_dest'
                     '$benchmark_path' -m client -r '$server_ip' -p '$client_f_bn'"
        done

        # Remove temporary parameter files
        sshpass -e ssh srn-user@"$server_host" \
                "cd '$copy_dest
                 rm '$server_f_bn' '$client_f_bn'"

        # Rename experimental observation directory
        local rename_cmd="mv .cpisync .cpisync_${suffix}"
        local copy_scenario_cmd="cp '$scenario_info_path' .cpisync_${suffix}/"
        sshpass -e ssh srn-user@"$server_host" "$rename_cmd; $copy_scenario_cmd"

        echo "----> [$(date)] End GenSync for server data file $server_f"
    done
}

# Stop SCOPE and scenario on Colosseum.
# $1 server host
# $2 client host
# $3 base station host
stop_on_colosseum() {
    # TODO 3: SCOPE's stop.sh does not allow us to start SCOPE again
    # afterwards. Radios may fail to connect.
    echo_o "Warning: 'stop_on_colosseum' does not ensure a clean slate.\n"

    local server_host="$1"
    local client_host="$2"
    local base_station_host="$3"
    local stop_cmd="cd /root/radio_api/ && ./stop.sh"

    echo_o "Stopping the scenario ..."
    sshpass -e ssh root@$server_host "colosseumcli rf stop"
    echo_o "Stopping server ..."
    sshpass -e ssh root@$server_host "$stop_cmd"
    echo_o "Stopping client ..."
    sshpass -e ssh root@$client_host "$stop_cmd"
}

check_requirements

while getopts "hcpu:" option; do
    case $option in
        c) ask_ssh_pass
           setup
           exit
           ;;
        u) if [ $# -lt 1 ]; then
               help
               err "You need to pass at least 1 argument."
           fi
           refresh_code "$OPTARG"
           exit
           ;;
        p) if [ $# -lt 2 ]; then
               help
               err "You need to pass at least 3 arguments."
           fi
           ask_ssh_pass
           push_image "$2" "$3" "$4"
           exit
           ;;
        h|* ) help
              exit
    esac
done

if [ $# -lt 4 ]; then
    help
    err "You need to pass at least 4 arguments."
fi

ask_ssh_pass "$(echo $1 | cut -d'-' -f1,2)"

if [ "${!#}" == "stop" ]; then
    stop_on_colosseum ${@:1:(( $# - 1 ))}  # pass all but last argument
else
    exec_on_colosseum $@  # pass all arguments
fi