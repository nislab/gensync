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
default_remote=novak@novaksXPS                 # remote where to push entire repository
default_dest=/home/novak/Desktop/CODE/gensync  # destination dir on destination host

######## Handle env variables
remote=${remote:="$default_remote"}
dest=${dest:="$default_dest"}

root_dir="$(cd ..; pwd)"

push_to_remote() {
    rsync -Pav                             \
          --exclude build                  \
          --exclude '*.tar.gz'             \
          --exclude '*.deb'                \
          --exclude '*.rpm'                \
          --exclude '*/plain_data/'        \
          "$root_dir"/* "$remote":"$dest"
}

while getopts "p" option; do
    case $option in
        p) push_to_remote
           exit
           ;;
    esac
done
