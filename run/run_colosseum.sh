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

######## Global constants
default_team_name=sync-edge                     # Colosseum team name
default_srn_user_passwd="Spiteful Corgi Bites"  # password for `root` and `srn_user` in containers
default_base_image=base-1604-nocuda.tar.gz      # base image when `-c`
default_modified_image=gensync-colosseum        # name of produced image when `-c`
default_shared_path=/share/exec                 # where containers mount the shared NAS
default_colosseumcli_path=colosseumcli          # path to `colosseumcli` install artifacts
default_instance=colosseum-instance             # temporary container name
default_net_interface=tr0                       # network interface in the container to sync through

######## Handle env variables
team_name=${team_name:="$default_team_name"}
srn_user_passwd=${srn_user_passwd:="$default_srn_user_passwd"}
base_image=${base_image:="$default_base_image"}
modified_image=${modified_image:="$default_modified_image"}
shared_path=${shared_path:="$default_shared_path"}
colosseumcli_path=${colosseumcli_path:="$default_colosseumcli_path"}
instance=${instance:="$default_instance"}
net_interface=${net_interface:="$default_net_interface"}

mapfile -t custom_vars \
        < <(( set -o posix; set ) | grep 'default_' | sed 's/default_//g')

######## Calculated constants
gensync_path="`cd ..; pwd`"
gensync_basename="$(basename "$gensync_path")"

######## Extended commands
lxc="sudo lxc"

help() {
    cat<<EOF
USAGE: run_colosseum [-h] [-c] [-u IMAGE] [-p SOURCE DESTINATION] SERVER CLIENT

Executes GenSync synchornization on Colosseum wireless network emulator.

Requirements: sudo, ssh, sshpass, rsync, lxc, lxd.
You need be behind Colosseum's VPN and have your Colosseum remotes set in ~/.ssh/config.

SERVER and CLIENT are the two hostnames of Colosseum SRN's among which you want to perform pair-wise sync.

OPTIONS:
    -h Show this message and exit.
    -c Create GenSync container image '$modified_image.tar.gz' and exit (installs 'colosseumcli' down the road).
    -u Similar to -c but uploads GenSync code to an existing IMAGE instead of creating the new one.
    -p Push SOURCE container image to DESTINATION (typically at file-proxy:).

You can custmize the following script variables:
$(for ((i = 0; i < ${#custom_vars[@]}; i++)) do echo ${custom_vars[$i]}; done)
EOF
}

# Echo green bold text
# $1 the text
echo_g() {
    echo -e "\e[32;1m$1\e[0m"
}

# Echo red bold text
# $1 the text
echo_r() {
    echo -e "\e[31;1m$1\e[0m"
}

# Echo grey bold text
# $1 the text
echo_o() {
    echo -e "\e[33;1m$1\e[0m"
}


# Build `lxc` execution command.
# No arguments
get_lxc_exec() {
    echo "$lxc exec $instance -- bash -c"
}

# Obtain the IP address of a host.
# No arguments
get_ip_cmd() {
    echo "ifconfig
          | grep -A1 '$net_interface'
          | awk '/inet/ { print \$2 }'
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
get_current_date() {
    date +%N
}

# Stop the container and export it to a `.tar.gz` file as an image.
# $1 the instance to export
# $2 the name for the modified image
export_modified_image() {
    $lxc stop "$1"
    printf "\nPublishing '$2' ...\n"
    $lxc publish "$1" --alias "$2"
    printf "\nExporting '$2' ...\n"
    $lxc image export "$2" "$2"
}

# Install `colosseumcli` in the container (container must be running).
# $1 the running instance name
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
# $1 the path to dir that contains `.tar.gz` files to shuffle.
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
# $1 the path to image file
# $2 the path where to push it on remote
push_image() {
    local source="$modified_image".tar.gz
    if ! [ -z "$1" ]; then source="$1"; fi
    local image_remote_loc="file-proxy:/share/nas/$team_name/images"
    if ! [ -z "$2" ]; then image_remote_loc="$2"; fi
    local remote_path="$(echo $image_remote_loc | awk -F: '{ print $2 }')"

    printf "Uploading image to Colosseum testbed @ $image_remote_loc ...\n"
    rsync -Pv "$source" "$image_remote_loc"
    printf "\nFixing container permissions on Colosseum testbed ...\n"
    ssh file-proxy "chmod 755 $remote_path/$(basename $source)"
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
# $1 the image name
refresh_code() {
    local image="$(basename "$1" .tar.gz)"
    local lxc_exec="$(get_lxc_exec)"
    local is_imported=$($lxc --format=csv image ls | awk -F, '{ print $1 }' | grep -c $image)

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

# Execute GenSync experiments on Colosseum.
# $1 the server host
# $2 the client host
exec_on_colosseum() {
    local server_host="$1"
    local client_host="$2"
    local copy_dest="$shared_path/gensync_$(get_current_date)"
    local benchmark_path="$copy_dest/build/Benchmarks"
    local examples_path="$copy_dest/example"
    local server_ip="$(sshpass -p "$srn_user_passwd" ssh root@"$server_host" "$(get_ip_cmd)")"

    # Move the code to the shared location (it's enough to do that on the server only)
    printf "\nCopying artifacts to $copy_dest ...\n"
    sshpass -p "$srn_user_passwd" ssh srn-user@"$server_host" \
            "mkdir -p '$copy_dest';
            cp -r /'$gensync_basename'/* '$copy_dest'/"

    printf "\n\nExecuting GenSync experiments. Server's IP is $server_ip ...\n"

    # TODO 2: there should be an easier way to execute commands in a container
    sshpass -p "$srn_user_passwd" ssh srn-user@"$server_host" \
            "cd '$copy_dest';
            '$benchmark_path' -m server -p '$examples_path'/server_params_data.cpisync" &
    sshpass -p "$srn_user_passwd" ssh srn-user@"$client_host" \
            "cd '$copy_dest';
            ping -c 5 '$server_ip'
            '$benchmark_path' -m client -r '$server_ip' -p '$examples_path'/client_params_data.cpisync"
}

while getopts "hcpu:" option; do
    case $option in
        c) setup
           exit
           ;;
        u) refresh_code "$OPTARG"
           exit
           ;;
        p) push_image "$2" "$3"
           exit
           ;;
        h|* ) help
              exit
    esac
done

exec_on_colosseum "$1" "$2"
