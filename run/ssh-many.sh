#!/usr/bin/env bash
#
# Author: Novak Boškov <boskov@bu.edu>
# Date: April, 2022.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

set -e

# Prefix to all ssh commands
ssh_prefix=

help() {
    cat <<EOF
ssh-many [-h] [-p] H1 H2 ...

ssh into all hosts and open a balanced tmux session with all of them.
Requiremets: ssh, tmux, sshpass

OPTIONS:
    -p SSH password for all hosts.
    -h Show this message and exit.
EOF
}

ssh-many() {
    local hosts=( $* )
    local session_name=ssh-many-$(date +%N)
    local pane_count=

    tmux new -s $session_name -d

    for ((i = 0; i < ${#hosts[@]}; i++)); do
        if [[ i -gt 0 ]]; then
            pane_count=.$i
            tmux split-window -h
        fi
        tmux send -t $session_name$pain_count "$ssh_prefix ssh ${hosts[$i]}" ENTER
    done

    tmux select-layout tiled
    tmux select-pane -t 0
    tmux a -t $session_name
}

while getopts "hp" option; do
    case $option in
        p) shift
           echo -n "SSH password for all: "
           read -s pass_all
           export SSHPASS="$pass_all"
           ssh_prefix="sshpass -e"
           ;;
        *) help
           exit
           ;;
    esac
done

ssh-many $*
