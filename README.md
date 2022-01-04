# *GenSync*

*GenSync* is a framework for benchmarking and optimizing
reconciliation of data. It can be compiled to a shared library and
integrated into other applications, or can be built as a standalone
application and used to benchmark the implemented algorithms under a
broad range of practical compute and network conditions (see below for
the list of implemented [algorithms](#algorithms)).

***This work is currently under review for IEEE Transactions on
Network and Service Management***.

- - -

## Table Of Contents
- [Concepts of *GenSync*](#concepts)
  - [*GenSync* Interface](#concepts_GenSync)
  - [*SyncMethod* Interface](#concepts_SyncMethod)
  - [*Communicant*](#concepts_Communicant)
  - [*BenchParams*](#concepts_BenchParams)
  - [*BenchObserv*](#concepts_BenchObserv)
  - [*Runner*](#concepts_Runner)
  - [`mininet_exec`](#concepts_mininet_exec)
  - [`run_experiments`](#concepts_run_experiments)
    - [Remote testbed](#concepts_run_experiments_remote_testbed)
- [Included Algorithms](#algorithms)
- [Usage](#usage)
  - [Compilation](#usage_compilation)
- [Examples](#examples)
- [License](#license)
- [Contact](#contacts)
- [Acknowledgments](#acknowledgments)

- - -

<a name="concepts"></a>
## Concepts of *GenSync*

*GenSync* includes several concepts that facilitate the integration
into higher-level applications, allow for benchmarking, and unify
algorithm implementations. The concepts of *GenSync* interface,
*SyncMethod* interface, and *Communicant* are initially introduced by
*CPISync*[^1]. We refine these concepts and introduce the five new
ones (*BenchParams*, *BenchObserv*, *Runner*, `mininet_exec`, and
`run_experiments`). Further we discuss the purpose and details of each
concept.

<a name="concepts_GenSync"></a>
### *GenSync* Interface
*GenSync* interface is the core abstraction of the *GenSync*
framework. It encompasses a generalized data representation (as
`DataSet`), a set of `Communicant`s, and a set of `SyncMethod`s. When
the framework is compiled in the benchmarking mode, a *GenSync* object
will also contain the `BenchParams`, and `BenchObserv` files (see
below for more details).

The main purpose of this abstraction is to offer a unified
reconciliation interface to all data sources, communication channels,
and all protocol implementations. Besides that, it is also the basis
for the fair comparison among the protocol implementations.

<a name="concepts_SyncMethod"></a>
### *SyncMethod* Interface
*GenSync* framework implements set reconciliation in the server-client
fashion. Thus, each set reconciliation algorithm needs to implement
the two core functions:

- `serverSyncBegin` which implements the client side of the algorithm, and
- `clientSyncBegin` which implements the server side of the algorithm.

Note that this separation is needed because not all set reconciliation
protocols are symmetric. For example, IBLT-based set reconciliation
requires different steps on client and server (similar is true for
CPI).

<a name="concepts_Communicant"></a>
### *Communicant*
*Communicant* is an abstraction for the communication channel. We have
so far implemented TCP and String based communicants. The former is
what we use by default, while the latter can be used when the nodes
are disconnected in time or space (*i.e.,* there is no communication
channel at the moment).

<a name="concepts_BenchParams"></a>
### *BenchParams*
*BenchParams* serves as a data source for all *GenSync* algorithms. It
can consume data from anything that conforms to the C++ stream
abstraction. For example, we may have our server and client data in
files (see [examples](#examples) below), or we may consume data from a
network stream. Each node in the reconciliation process is associated
with one *BenchParams* as its primary data source.

Besides the data, *BenchParams* object also contains the information
needed to construct a *SyncMethod*. This includes the specification of
the algorithm that we want to use and its parameters. When the
*GenSync* framework is compiled in the benchmarking mode (*i.e.,* the
data is available upfront), we can infer optimal algorithm parameters
from the data (see [`run_experimetns`](#concepts_run_experiments) for
more details). Note that this is possible *only* in the benchmarking
mode. In the reconciliation without a prior context (*i.e.,* parties
do not know the data of the other party up front), inferring the
optimal algorithm parameters may be a hard task (see the paper for
more details).

Besides consuming data from an existing data source *BenchParams*
object can also generate its own data according to some random
distribution (*e.g.,* Zipfian).

A *BenchParams* object can be fully reconstructed from a file. Such
files typically have the `.cpisync` extension and the following
format:

```
Sync protocol (as in GenSync.h): <INTEGER_TO_IDENTIFY_PROTOCOL>
<PROTOCOL_PARAMETERS>
Sketches:
--------------------------------------------------------------------------------
<BASE64_ENCODED_DATA_POITS|REFERENCE_TO_OTHER_DATA_FILE>
```

Each Protocol has unique set of parameters. For example:
- CPI
```
m_bar: <INT>
bits: <INT>
epsilon: <INT>
partitions/pFactor(for InterCPISync): <INT>
hashes: <true|false>
```
- IBLT
```
expected: <INT>
eltSize: <INT>
numElemChild: <INT>
```
- Cuckoo
```
fngprtSize: <INT>
bucketSize: <INT>
filterSize: <INT>
maxKicks: <INT>
```

When we create multiple parameter files with the same data points,
*GenSync* automatically detects that and uses file path references to
save storage on the testbed machine.

<a name="concepts_BenchObserv"></a>
### *BenchObserv*
A *BenchObserv* object represents an experimental observation. When
*GenSync* is compiled in the benchmark mode for the purpose of
experimentation, each experiment run generates one *BenchObserv* with
the following content:

- `BenchParams` parameters that are used in the experiment.
- `success` indicates whether the synchronization has succeeded (is
  server's and client's data exactly equal after reconciliation
  protocol has completed),
- `exception` the exception information if the program has failed to
  execute, and
- Reconciliation statistics that consist of:
  - bytes sent by the server,
  - bytes sent by the client,
  - server idle time (time spent waiting for the message from the client),
  - client idle time,
  - server communication time (time spent sending the messages),
  - client communication time,
  - server computation time (time spent executing the algorithm operations),
  - client computation time, and
  - total time (total wall clock time from the moment when the client
    initiates the communications to the moment when the protocol has
    completed).

Note that the previous works typically only report the overall
computation time, which makes *GenSync* unique.

<a name="concepts_Runner"></a>
### *Runner*
When *GenSync* is compiled with its benchmarking suite, the *Runner*
serves as the benchmarking entry point. The runner program is called
`Benchmarks` and has the following interface:

```
Usage: ./Benchmarks -p PARAMS_FILE [OPTIONS]

Do not run multiple instances of -m server or -m client in the same
directory at the same time. When server and client are run in two
separate runs of this script, a lock file is created in the current
directory.

OPTIONS:
    -h print this message and exit
    -p PARAMS_FILE to be used.
    -g whether to generate sets or to use those from PARAMS_FILE.
       In SERVER and CLIENT modes data from PARAMS_FILE is always used.
       The first set from PARAMS_FILE is loaded into the peer.
    -i ADD_ELEM_CHUNK_SIZE add elements incrementally in chunks.
       Synchronization is invoked after each chunk is added.
       Can be used only when data is consumed from parameter files
       and this script is run in either SERVER or CLIENT mode.
    -m MODE mode of operation (can be "server", "client", or "both")
    -r PEER_HOSTNAME host name of the peer (requred when -m is client)
```

The *Runner* can invoke any *GenSync* algorithm on arbitrary data
(provided in `PARAM_FILE`), or can generate the random data. It also
supports two reconciliation types (*i.e.,* one-time and
incremental). The *Runner* can run the client and the server on the
same machine, or can be run on two remote machines.

<a name="concepts_mininet_exec"></a>
### `mininet_exec`
The program that integrates *GenSync* set reconciliation protocols and
[Mininet](http://mininet.org/) is called `mininet_exec` and you can
find it in [`/mininet_exec`](/mininet_exec). By default,
`mininet_exec` instantiates the two Mininet nodes and runs separate
*Runner*s in each of them. It achieves so by running the *Runner* in
the server mode on one host and in the client mode on the other. It
uses the single switch and two hosts topology. `mininet-exec` has the
following interface:

```
Runs arbitrary programs within mininet nodes in a single-switch-two-nodes
topology.
Network parameters and the computational power of nodes are adjustable.

positional arguments:
  SERVER_SCRIPT         script that runs within the server (server node IP is 192.168.1.1, host name h1).
  CLIENT_SCRIPT         script that runs within the client (client node IP is 192.168.1.2, host name h2).

optional arguments:
  -h, --help            show this help message and exit
  -l LATENCY, --latency LATENCY
                        symmetric latency between server and client (in ms).
  -b BANDWIDTH, --bandwidth BANDWIDTH
                        bandwidth on each node to switch link (in Mbps)
  -pl PACKET_LOSS, --packet-loss PACKET_LOSS
                        packet loss on each node to switch link (in percentages, e.g., 10).
  -cpus CPU_SERVER, --cpu-server CPU_SERVER
                        CPU share of the server (in percentages, e.g., 33).
  -cpuc CPU_CLIENT, --cpu-client CPU_CLIENT
                        CPU share of the client (in percentages, e.g., 33).
  --ping-iperf          if passed, ping and iperf tests between server and client are run before running the scripts.
```

<a name="concepts_run_experiments"></a>
### `run_experiments`
The focal point of the *GenSync* benchmarking infrastructure is the
`run_experiments` program. It orchestrates all parts of the *GenSync*
framework and allows for easy benchmarking. Typical inputs to
`run_experiments` program are:
1. Two `PARAM_FILE`s (representing server and client data, and the
   protocol specification). Instead of using only two `PARAM_FILE`s,
   you can specify the directory that contains many of them (as
   `params_dir` in [`run/run_experiments.sh`](run/run_experiments.sh))
   and they will be run in alphabetical order. This is particularly
   useful when we want to run many experiments as a batch job.
2. Network parameters in the following form:
```
latency=<INT>  # in milliseconds
bandwidth="<INT_UPLOAD>/<INT_DOWNLOAD>" | <INT>  # when <INT>, bandwidth is symmetrical
packet_loss=<DOUBLE>  # for example, 0.01 for 0.01% packet loss
```
3. Compute parameters of the nodes where each integer represents the compute power **relative to the single-thread score of the testbed machine** (you can find these scores at [cpubenchmark.net](https://www.cpubenchmark.net/) or any other place of your choice):
```
cpu_server=<INT>  # for example, 33 for 33% of testbed CPU
cpu_client=<INT>
```
Please see the paper for more details on how we constrain the compute power of the nodes.

The outputs of the `run_experiment` program are as follows:
1. `.cpisync` directory is created in the current working directory
   and it contains an experimental observation for each experiment
   run. These experimental observations can be obtained as CSV files
   using the `-p` option of the `run_experiments` program (see the
   full interface below).
2. `.mnlog` directory is created in the same location and contains the
   logs from the two Mininet hosts.

By default, each experiment runs 100 times. To adjust this, see
`repeat` variable in
[`run/run_experiments.sh`](run/run_experiments.sh).

To persist the benchmark settings, you can specify the values of all
`run_experiments` parameters in
[`run/run_experiments.sh`](run/run_experiments.sh) file directly.

The `run_experiments` program has the following interface:

```
USAGE: ./run_experiments.sh [-q] [-s] [-i] [-r REMOTE_PATH] [-p EXPERIMENT_DIR] [-pp PULL_REMOTE]

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
    # collects data on remote and makes CPI_Experiment_1.csv locally
    ./run_experiments.sh -p remote_name:/home/novak/EXPERIMENTS/CPI_Experiment_1
    # pulls the experimental data from the remote. Creates DATA/CPISync/1/.cpisync
    ./run_experiments.sh -pp remote_name:/home/novak/EXPERIMENTS/./CPISync/1/.cpisync

If a remote machine is used, it needs the following dependencies:
    - Mininet,
    - Python 3 (with Mininet API),
    - CPISync dependencies (see cpisync/README.md), and
    - Ninja build system.
CPISync source code will be compiled on the remote machine.
```

`run_experiment` contains some other useful features that we omit
here. To learn more about them, please refer to
[`run/run_experiments.sh`](run/run_experiments.sh) directly.

<a name="concepts_run_experiments_remote_testbed"></a>
#### Remote testbed
We typically use `run_experiments` on a dedicated physical testbed
server to minimize the interference with other
processes. `run_experiments.sh` allows you to run the program itself
on your local machine (*e.g.,* your laptop) while the experiments are
run on a remote testbed:

```
$ ./run_experiments.sh -r remote_name:/remote/path
```

Then pull the experimental observations as CSV files back to your
local machine:

```
$ ./run_experiments.sh -p remote_name:/remote/path
```

For this to work, your remote testbed machine must satisfy all the
`run_experiments` dependencies (see above) and you laptop needs
[`rsync`](https://man7.org/linux/man-pages/man1/rsync.1.html)
(alongside standard Linux user tools).

<a name="algorithms"></a>
## Included Algorithms

*GenSync* currently supports the following set reconciliation
protocols:

- Characteristic Polynomial Interpolation-based set reconciliation
  (CPI). The main theoretical underpinnings are introduced in:
  - Y. Minsky, A. Trachtenberg, and R. Zippel, "Set Reconciliation
    with Nearly Optimal Communication Complexity", IEEE Transactions
    on Information Theory, 2003.
  - Y. Minsky and A. Trachtenberg, "Scalable set reconciliation" 40th
    Annual Allerton Conference on Communication, Control, and
    Computing, 2002.
- Interactive CPI is an extension of CPI to support interactive
  reconciliation.
- Prioritized CPI is an extension of CPI to support elements with
  different priorities.
- Invertible Bloom Lookup Tables-based set reconciliation (IBLT). The
  main theoretical underpinnings are introduced in:
  - Eppstein et al. "What's the difference? Efficient set
    reconciliation without prior context." ACM SIGCOMM Computer
    Communication Review, 2011.
  - M. Goodrich and M. Mitzenmacher, "Invertible bloom
    lookup tables." 2011 49th Annual Allerton Conference on
    Communication, Control, and Computing (Allerton). IEEE, 2011.
  - M. Mitzenmacher, and T. Morgan, "Reconciling graphs and sets of
    sets." Proceedings of the 37th ACM SIGMOD-SIGACT-SIGAI Symposium
    on Principles of Database Systems. ACM, 2018.
- Cuckoo filters-based set reconciliation. The theoretical basis
  introduced in:
  - Fan et al., "Cuckoo filter: Practically better than bloom."
    Proceedings of the 10th ACM International on Conference on
    emerging Networking Experiments and Technologies. 2014.
  - Luo et al., "Set reconciliation with cuckoo filters."  Proceedings
    of the 28th ACM International Conference on Information and
    Knowledge Management. 2019.
  - Shangsen, et al. "Multiset Synchronization with Counting Cuckoo
    Filters." International Conference on Wireless Algorithms,
    Systems, and Applications. Springer, Cham, 2020.

For more implementation details as well as the full list of the CPI
variants that we have implement see [doc/README.md](doc/README.md).

<a name="usage"></a>
## Usage
As we mentioned before, *GenSync* has two modes of operation; 1) as a
library for integration into higher-level applications, and 2) the
benchmarking mode. The list of the set reconciliation protocols that
we support is the same in both cases. For the usage as a library refer
to [`use-instructions`](/doc#use-instructions). For the benchmarking
purposes, please see [*Runner*](#concepts_Runner),
[`mininet_exec`](#concepts_mininet_exec), and
[`run_experiments`](#concepts_run_experiments).

<a name="usage_compilation"></a>
### Compilation
The only external dependency you will need here is
[NTL](https://libntl.org/). Many Linux distributions already have NTL
in their package archives (*e.g.,*
[Debian](https://packages.debian.org/sid/libntl-dev),
[Ubuntu](https://packages.ubuntu.com/bionic/libntl-dev),
[Fedora](https://src.fedoraproject.org/rpms/ntl),
[Arch](https://archlinux.org/packages/community/x86_64/ntl/)).

To compile *GenSync* use:

``` shell
$ git clone --recurse-submodules git@github.com:nislab/gensync.git
$ cd gensync
$ cmake -B build
$ cmake --build build
```

The compilation process produces a static library that you can include
in other projects (as `.deb` and `.rpm` packages), and all the
standalone executables that you need for benchmarking.

<a name="examples"></a>
## Examples
For this to work you will need
- Linux machine with superuser privileges,
- Mininet with Python API (see [here](http://mininet.org/download/)), and
- *GenSync* (compile as [above](#usage_compilation)).

To try *GenSync*, you will need `PARAM_FILE`s. We provide the default
ones in [`/example`](/example). The current version of the
`run_experiment` program already refers to them. Without modifying any
files, you can try *GenSync* running the following commands:

```
$ cd run
$ ./run_experiments.sh
```

See [`run_experiments`](#concepts_run_experiments) for more details on
how to gauge the benchmark parameters and analyze the experimental
observations.

<a name="acknowledgments"></a>
## Acknowledgments

This project was supported in part by the National Science Foundation (NSF).

<a name="license"></a>
## License

General Public License v3.0 (GPL-3.0).

<a name="contacts"></a>
## Contacts

- Novak Boskov ([boskov@bu.edu](mailto:boskov@bu.edu))
- Ari Trachtenberg ([trachten@bu.edu](mailto:trachten@bu.edu))
- David Starobinski ([staro@bu.edu](mailto:staro@bu.edu))

[^1]: A version of *CPISync* is also maintained [here](https://github.com/trachten/cpisync). Although we are actively working to integrate the two projects, *CPISync* currently does *not* include the *GenSync* benchmarking infrastructure and abstractions improvements that are contained in this repository. Our final goal is to merge the two projects into one.
