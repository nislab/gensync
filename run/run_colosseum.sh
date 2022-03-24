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

# Global constants
team_name=sync-edge                     # Colosseum team name
base_image=base-1604-nocuda.tar.gz      # base image when `-c`
modified_image=gensync-colosseum        # name of produced image when `-c`
instance=colosseum-instance             # temporary container name
shared_path=/share/exec                 # where containers mount the shared NAS
srn_user_passwd="Spiteful Corgi Bites"  # password for `root` and `srn_user` in containers

# Calculated constants
gensync_path="`cd ..; pwd`"
gensync_basename="$(basename "$gensync_path")"

# Extended commands
lxc="sudo lxc"

help() {
    cat<<EOF
USAGE: run_colosseum [-h] [-c] [-u IMAGE] SERVER CLIENT

Executes GenSync synchornization on Colosseum wireless network simulator.

Requirements: sudo, ssh, sshpass, rsync, lxc, lxd.
You need be behind Colosseum's VPN and have your Colosseum remotes set in ~/.ssh/config.

SERVER and CLIENT are hostnames of Colosseum SRN's.

OPTIONS:
    -h Show this message and exit.
    -c Create GenSync container image '$modified_image.tar.gz' and exit.
    -u Similar to -c but uploads GenSync code to an existing IMAGE instead of creating the new one.
EOF
}

# Build `lxc` execution command.
# No arguments
get_lxc_exec() {
    echo "$lxc exec $instance -- bash -c"
}

# Obtain the IP address of a host.
# No arguments
get_ip_cmd() {
    echo "ifconfig | grep -A1 'col0: ' | awk '/inet/ { print \$2 }'"
}

# Obtain GenSync compile commands.
# No arguments
get_compile_cmd() {
    echo "cd /$gensync_basename;
          rm -rf build || true;
          rm CMakeCache.txt || true;
          mkdir build; cd build; cmake ../; make -j\$(nproc)"
}

# Obtain nanoseconds since epoch
get_current_date() {
    date +%N
}

# Stop the container and export it to a `.tar.gz` file as an image
# $1 the instance to export
# $2 the name for the modified image
export_modified_image() {
    $lxc stop "$1"
    printf "\nPublishing '$2' ...\n"
    $lxc publish "$1" --alias "$2"
    printf "\nExporting '$2' ...\n"
    $lxc image export "$2" "$2"
}

# Prepare a container image and upload it to Colosseum.
# No arguments
setup() {
    local image_remote_loc=file-proxy:/share/nas/"$team_name"/images
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

    # Change the password of the `srn-user` and `root` users
    printf "\nChanging 'srn-user' and 'root' passwords in the container to '$srn_user_passwd' ... \n"
    $lxc_exec "echo -e '$srn_user_passwd\n$srn_user_passwd' | passwd srn-user"
    $lxc_exec "echo -e '$srn_user_passwd\n$srn_user_passwd' | passwd root"

    # Get rid of unattended-upgrades
    printf "\nGetting rid of unattended-upgrades ...\n"
    $lxc_exec \
        "for i in {1..3}; do lslocks | awk '/unattended-upgrades.lock/ { print \$2 }' | xargs kill -9; sleep 1; done || true"

    # Install NTL and its dependencies
    $lxc_exec "apt install -y libntl-dev libgmp3-dev libcppunit-dev"

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

        # Update repositories and upgrade all packages
        $lxc_exec "apt update -y; apt upgrade -y"
        $lxc stop "$instance"; $lxc start "$instance"

        # Upgrade to a newer Ubuntu version
        $lxc exec "$instance" -- script /dev/null -c \
             "sleep 2; apt update -y; do-release-upgrade -m server"
        $lxc stop "$instance"; $lxc start "$instance"

        printf "\nContainer upgrade completed!\n\n"
        $lxc_exec "lsb_release -a 2>/dev/null"
        read -p "Do you want to proceed with this version of Ubuntu? (Y/n): " yn
        case $yn in
            [Nn]* ) exit
        esac
    fi

    # Transfer GenSync codebase to the container
    $lxc file push -rp "$gensync_path"/ "$instance"/
    $lxc_exec "rm -f /$gensync_basename/run/*.tar.gz || true"

    # Compile GenSync in the container
    $lxc_exec "$(get_compile_cmd)"

    export_modified_image "$instance" "$modified_image"

    # Cleanup
    printf "\nCleanup ...\n"
    $lxc delete --force "$instance"
    $lxc image delete colosseum-base "$modified_image"

    # Upload the container to the Colosseum testbed
    printf "\nUploading image to Colosseum testbed ...\n"
    rsync -Pv "$modified_image".tar.gz "$image_remote_loc"
    printf "\nFixing container permissions on Colosseum testbed ...\n"
    ssh file-proxy "chmod 755 $image_remote_loc/$modified_image".tar.gz
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
            printf "\nCannot find '$image' image.\n"
            exit
        fi
    fi

    $lxc delete --force "$instance" 2>/dev/null || true
    $lxc launch "$image" "$instance"

    # Copy code and compile it in the container
    $lxc_exec "rm -rf /$gensync_basename"
    $lxc file push -rp "$gensync_path"/ "$instance"/
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

while getopts "hcu:" option; do
    case $option in
        c) setup
           exit
           ;;
        u) refresh_code "$OPTARG"
           exit
           ;;
        h|* ) help
              exit
    esac
done

exec_on_colosseum "$1" "$2"
