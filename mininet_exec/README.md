# Execute Arbitrary Programs In Mininet Hosts With mininet_exec

This is a script that executes arbitrary programs within two
[mininet](http://mininet.org/) hosts. It uses the simplest possible
mininet topology with a single switch connected to two hosts. We refer
to the first host as the `server` and the second one as the `client`.

The main use case of this script is to serve as a testbed for data
synchronization algorithms.

``` shell
$ ./mininet_exec.py --help
usage: mininet_exec.py [-h] [-l LATENCY] [-b BANDWIDTH] [-pl PACKET_LOSS]
                       [-cpus CPU_SERVER] [-cpuc CPU_CLIENT] [--ping-iperf]
                       SERVER_SCRIPT CLIENT_SCRIPT

Runs arbitrary programs within mininet nodes in a single-switch-two-nodes topology.
Network parameters and the computational power of nodes are adjustable.

Author: Novak Boškov <boskov@bu.edu>
Date: Jan, 2021.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

positional arguments:
  SERVER_SCRIPT         The script that runs within the server (server node IP is 192.168.1.1, host name h1).
  CLIENT_SCRIPT         The script that runs within the client (client node IP is 192.168.1.2, host name h2).

optional arguments:
  -h, --help            show this help message and exit
  -l LATENCY, --latency LATENCY
                        Latency on each node to switch link (in ms).
  -b BANDWIDTH, --bandwidth BANDWIDTH
                        Bandwidth on each node to switch link (in Mbps).
  -pl PACKET_LOSS, --packet-loss PACKET_LOSS
                        Packet loss on each node to switch link (in percentages, e.g., 10).
  -cpus CPU_SERVER, --cpu-server CPU_SERVER
                        CPU share of the server (in percentages, e.g., 33).
  -cpuc CPU_CLIENT, --cpu-client CPU_CLIENT
                        CPU share of the client (in percentages, e.g., 33).
  --ping-iperf          If passed, ping and iperf tests between server and client are run before running the scripts.
```

## A minimal example
Don't forget to install mininet in your environment:

``` shell
pip install requirements.txt
```

You need to run `mininet_exe` as root (just like the basic mininet):

``` shell
sudo ~/.virtualenvs/my_env/bin/python mininet_exec.py \
    --latency 5                                       \
    --bandwidth 50                                    \
    --packet-loss 2                                   \
    --cpu-server 20 --cpu-client 30                   \
    --ping-iperf                                      \
    "./doc/test.sh server" "./doc/test.sh"
```

The output contains the mininet logs:

``` shell
*** Creating network
*** Adding controller
*** Adding hosts:
h1 h2
*** Adding switches:
s1
*** Adding links:
(50.00Mbit 2.5ms delay 1.00000% loss) (50.00Mbit 2.5ms delay 1.00000% loss) (h1, s1) (50.00Mbit 2.5ms delay 1.00000% loss) (50.00Mbit 2.5ms delay 1.00000% loss) (h2, s1)
*** Configuring hosts
h1 (cfs 60000/100000us) h2 (cfs 40000/100000us)
*** Starting controller
c0
*** Starting 1 switches
s1 ...(50.00Mbit 2.5ms delay 1.00000% loss) (50.00Mbit 2.5ms delay 1.00000% loss)
*** Ping: testing ping reachability
h1 -> h2
h2 -> h1
*** Results:
 h1->h2: 1/1, rtt min/avg/max/mdev 13.584/13.584/13.584/0.000 ms
 h2->h1: 1/1, rtt min/avg/max/mdev 13.241/13.241/13.241/0.000 ms
*** Iperf: testing TCP bandwidth between h1 and h2
*** Results: ['9.44 Mbits/sec', '10.5 Mbits/sec']
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Executes on server [h1]:
echo "`date` [$((`date +%s%N`/1000000))]:" >client_1610253303.mnlog; ./doc/test.sh server >>client_1610253303.mnlog 2>&1

Executes on client [h2]:
echo "`date` [$((`date +%s%N`/1000000))]:" >server_1610253303.mnlog; ./doc/test.sh >>server_1610253303.mnlog 2>&1
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*** Stopping 1 controllers
c0
*** Stopping 2 links
..
*** Stopping 1 switches
s1
*** Stopping 2 hosts
h1 h2
*** Done
```

It also prints the exact commands that are run on the hosts. Both the
`stdout` and `stderr` from the hosts are redirected to
`server_UID.mnlog` and ``client_UID.mnlog`` in the current working
directory.

## How to set parameters?
Think of the network as a single channel and set the channel
parameters. Use `--ping-iperf` option and observe the outputs of
`ping` and `iperf`.

- When you set latency of, e.g., 10 ms, then client-switch link will
  have latency of 5 ms, and the switch-server link will also have 5 ms
  latency. However, `ping` will measure ~20 ms because it measures
  RTT.
- When you set bandwidth of, e.g., 50 Mbps, that will be roughly the
  server-client throughput.
- When you set packet loss to 2%, then client-switch link has 1%
  packet loss, and switch-server also has 1% packet loss. That is,
  `--packet-loss`/2 = client-switch packet loss = switch-server packet
  loss.

Notes:
- Package loss has a big impact on throughput. For example, with
  `--bandwidth 50` and `--packet-loss 0`, the throughput will be close
  to 50 Mbps. However, with 2% packet loss, throughput will drop to
  ~10 Mbps.

## A CPISync example
[doc/CPISync](doc/CPISync) has a precompiled executable of the CPISync
benchmark runner as well as the example `*_param_data.cpisync` files
that the benchmark runner requires. An example of benchmark runner
usage is given in [doc/cpisync_test.sh](doc/cpisync_test.sh). CPISync
benchmarks can be run within `mininet_exec` as follows:

``` shell
sudo ~/.virtualenvs/my_env/bin/python mininet_exec.py \
    --latency 5                                       \
    --bandwidth 50                                    \
    --packet-loss 2                                   \
    --cpu-server 20 --cpu-client 30                   \
    "./doc/cpisync_test.sh server" "./doc/cpisync_test.sh"
```
