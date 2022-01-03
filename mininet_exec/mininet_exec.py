#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Runs arbitrary programs within mininet nodes in a single-switch-two-nodes
topology.
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
"""

import argparse
import time
import os
import json
from typing import Dict, Any
from argparse import RawTextHelpFormatter
from multiprocessing import cpu_count
from mininet.topo import Topo
from mininet.net import Mininet
from mininet.node import CPULimitedHost
from mininet.link import TCLink
from mininet.log import setLogLevel, info

now_is = int(time.time())

DEFAULT_LATENCY = 5                 # Default latency for all links
DEFAULT_BANDWIDTH = 10              # Default bandwidth for all links
DEFAULT_PACKETLOSS = 2              # Default packet loss for all links
DEFAULT_CPU = .5                    # Default bandwidth for all links
SERVER_IP = '192.168.1.1'
CLIENT_IP = '192.168.1.2'
SERVER_HOST_NAME = 'h1'
CLIENT_HOST_NAME = 'h2'
SWITCH_NAME = 's1'

LOG_DIR = '.mnlog'
LOG_FILES_EXT = '.mnlog'
CLIENT_OUT_FILE = os.path.join(LOG_DIR, f"client_{now_is}{LOG_FILES_EXT}")
SERVER_OUT_FILE = os.path.join(LOG_DIR, f"server_{now_is}{LOG_FILES_EXT}")

LINKS = 2                       # Number of links we have in the topology
INFO_SEP = 80*'~'               # Line that separates command execution-specific logs

# Number of processors on the system.
# Mininet interprets host cpu share as a ratio of overall processing power
# on the system. However, we interpret server and client cpu shares as
# the ratios of the single core's processing power.
# See access.redhat.com/documentation/en-us/red_hat_enterprise_linux/6/html/resource_management_guide/sect-cpu-example_usage
NPROC = cpu_count()


class SyncTopo(Topo):
    """Minimal two hosts one switch topology."""

    def build(self,
              latency=DEFAULT_LATENCY, bandwidth=DEFAULT_BANDWIDTH,
              packet_loss=DEFAULT_PACKETLOSS,
              cpus=DEFAULT_CPU, cpuc=DEFAULT_CPU):
        """Build the topology."""
        lat_per_link = latency / LINKS
        p_loss_per_link = packet_loss / LINKS

        switch = self.addSwitch(SWITCH_NAME)
        right_host = self.addHost(
            SERVER_HOST_NAME, ip=SERVER_IP, cpu=cpus/NPROC)
        left_host = self.addHost(
            CLIENT_HOST_NAME, ip=CLIENT_IP, cpu=cpuc/NPROC)

        network_args = {'bw': bandwidth,
                        'delay': f"{lat_per_link}ms",
                        'loss': p_loss_per_link,
                        'use_htb': True}
        self.addLink(left_host, switch, **network_args)
        self.addLink(right_host, switch, **network_args)


def exec_cmd(cmd: str, out_file: str) -> str:
    """Build the actual shell command to execute on the hosts."""
    return f"echo \"`date`:\" >{out_file}; {cmd} >>{out_file} 2>&1"


def pprint_args(pargs) -> str:
    """Turn program arguments into a json string."""
    args_dict = {}                   # type: Dict[str, Any]
    for arg in vars(pargs):
        args_dict[str(arg)] = getattr(pargs, arg)

    return json.dumps(args_dict)


def build_net(latency, bandwidth, packet_loss, cpus, cpuc) \
             -> Mininet:
    """Build the proper topology."""
    # convert CPU power into percentages
    cpus = cpus / 100
    cpuc = cpuc / 100

    # parse bandwidth
    bndw_parts = bandwidth.split('/')
    if len(bndw_parts) == 1:
        bndw = int(bndw_parts[0])
        topo = SyncTopo(latency, bndw, packet_loss,
                        cpus, cpuc)
        net = Mininet(topo=topo, host=CPULimitedHost,
                      link=TCLink, autoStaticArp=True)
    elif len(bndw_parts) == 2:
        bndw_uplink = int(bndw_parts[0])
        bndw_downlink = int(bndw_parts[1])
        net = Mininet(autoStaticArp=True)
        net.addController('c0')
        s1 = net.addSwitch('s1')
        h1 = net.addHost(SERVER_HOST_NAME, ip=SERVER_IP,
                         cpu=cpus/NPROC,
                         cls=CPULimitedHost)
        h2 = net.addHost(CLIENT_HOST_NAME, ip=CLIENT_IP,
                         cpu=cpuc/NPROC,
                         cls=CPULimitedHost)

        # Seems like TCLink assigns delay using both directions of the
        # link. Thus, latency of 10ms will assign 5ms latency in one,
        # and 10 ms in the other direction. By assigning our `latency`
        # directly as `delay` of TCLink's for both links, we achieve
        # that the latency from server to client equals `latency`
        # (and that our server-to-client latency is also symmetric).
        lat = f"{latency}ms"
        link1 = net.addLink(h1, s1, delay=lat, cls=TCLink)
        link2 = net.addLink(h2, s1, delay=lat, cls=TCLink)

        link1.intf1.config(bw=bndw_uplink, use_tbf=True)
        link2.intf1.config(bw=bndw_downlink, use_tbf=True)
    else:
        raise Exception(f"{bandwidth} is the wrong bandwidth parameter.")

    return net


def main(pargs):
    """Run the main functionality."""
    # be verbose
    setLogLevel('info')
    # log the program parameters
    info('{}\n'.format(pprint_args(pargs)))

    net = build_net(pargs.latency, pargs.bandwidth, pargs.packet_loss,
                    pargs.cpu_server, pargs.cpu_client)

    # topology initialization
    net.start()

    srvr, cli = net.getNodeByName(SERVER_HOST_NAME, CLIENT_HOST_NAME)
    # verify latency and bandwidth
    if pargs.ping_iperf:
        net.pingFull()
        net.iperf((srvr, cli))
        net.iperf((cli, srvr))

    srvr_exec = exec_cmd(pargs.SERVER_SCRIPT, SERVER_OUT_FILE)
    cli_exec = exec_cmd(pargs.CLIENT_SCRIPT, CLIENT_OUT_FILE)
    info(f"{INFO_SEP}\nExecutes on server [{srvr.name}]:\n{srvr_exec}\n\n"
         f"Executes on client [{cli.name}]:\n{cli_exec}\n")
    srvr.sendCmd(srvr_exec)
    cli.sendCmd(cli_exec)

    # wait until the commands are done
    srvr.monitor()
    cli.monitor()

    # stop and cleanup when done
    net.stop()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=RawTextHelpFormatter)

    SSCRIPT_H = "script that runs within the server (server node IP is " \
                f"{SERVER_IP}, host name {SERVER_HOST_NAME})."
    CSCRIPT_H = "script that runs within the client (client node IP is " \
                f"{CLIENT_IP}, host name {CLIENT_HOST_NAME})."

    parser.add_argument('SERVER_SCRIPT', help=SSCRIPT_H, type=str)
    parser.add_argument('CLIENT_SCRIPT', help=CSCRIPT_H, type=str)
    parser.add_argument(
        '-l', '--latency',
        help='symmetric latency between server and client (in ms).',
        type=float, default=DEFAULT_LATENCY)
    parser.add_argument(
        '-b', '--bandwidth',
        help="bandwidth on each node to switch link (in Mbps)",
        type=str, default=DEFAULT_BANDWIDTH)
    parser.add_argument(
        '-pl', '--packet-loss',
        help="packet loss on each node to switch link "
             "(in percentages, e.g., 10).",
        type=float,
        default=DEFAULT_PACKETLOSS)
    parser.add_argument(
        '-cpus', '--cpu-server',
        help='CPU share of the server (in percentages, e.g., 33).',
        type=int, default=DEFAULT_CPU)
    parser.add_argument(
        '-cpuc', '--cpu-client',
        help='CPU share of the client (in percentages, e.g., 33).',
        type=int, default=DEFAULT_CPU)
    parser.add_argument(
        '--ping-iperf',
        help="if passed, ping and iperf tests between server and client"
             " are run before running the scripts.",
        default=False, action='store_true')

    p_args = parser.parse_args()

    # create the logs directory
    if not os.path.exists(LOG_DIR):
        os.makedirs(LOG_DIR)

    main(p_args)
