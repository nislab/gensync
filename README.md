# GenSync - 2.0.4

GenSync is a framework for _efficiently_ synchronizing similar data across multiple devices.

## Motivating example
Suppose that laptop `A` has a list of contacts:

> Alice, Jim, Jane, Rick, Bob


and cellphone `B` has the contacts:
> Alice, Jim, Suzie, Jane, Rick

Then an efficient synchronization protocol might quickly idenfity the differences (`Rick` on the laptop, and `Suzie` on the cellphone)
and exchange only these contacts, rather than sending the entire contact list from one device to another.



<a name="Introduction"></a>
## Introduction
The GenSync framework provides a shared library for benchmarking and optimizing a variety of state-of-the-art data synchronization protocols, either offline or directly embedded within application code.  In one typical use-case, an application would use GenSync for its core data synchronization needs, and developers can compare and optimize the performance of different synchronization protocols to suit their needs.  Alternatively, users could utilize the GenSync library to profile synchronization usage for their application, and then experiment with synchronization protocols offline to improve perfromance.

The current implementation of the framework includes four families of data synchronization protocols (and [their variants](#SyncTypes))):
- _FullSync_ - a trivial protocol that transfers all data from one device to another for a local comparison
- _CPISync_ - based on Characteristic Polynomial Interpolation
- _IBLTSync_ - based on Invertible Bloom Lookup Tables
- _CuckooSync_ - based on Cuckoo tables

## Code
The source code for this library is divided among several repositories.

- [gensync-core](https://github.com/nislab/gensync-core) - the core framework code that is used by other repositories.
- [gensync-lib-linux](https://github.com/nislab/gensync-lib) - used to produce the GenSync library and associated headers.
- [gensync-macports](https://github.com/nislab/gensync-macports) | - used to produce the [macports](https://ports.macports.org/port/gensync/details/) version of the GenSync library for MacOS.
- [gensync-benchmarking](https://github.com/nislab/gensync-benchmarking) - used for including benchmarking capabilities.


<a name="SyncTypes"></a>
## Supporte protocols:
* _FullSync family_
  * One device sends all its data to the second device, which determines what differs between the two devices and notifies the first device of the differences.
* _CPISync family_
    * CPISync
        * Sync using the protocol described [here](http://ipsit.bu.edu/documents/ieee-it3-web.pdf). The maximum number of differences that can be reconciled must be bounded by setting mBar. The server does the necessary computations while the client waits, and returns the required values to the client
    * CPISync_OneLessRound
        * Perform CPISync with set elements represented in full in order to reduce the amount of rounds of communication by one (No hash inverse round of communication). The server does the necessary computations while the client waits, and returns the required values to the client
    * OneWayCPISync
        * Perform CPISync in one direction, only adding new elements from the client to the server. The client's elements are not updated. The server does the necessary computations and determines what elements they need to add to their set. The client does not receive a return message and does not have their elements updated
    * ProbCPISync
        * Perform CPISync with a given mBar but if the amount of differences is larger than that, double mBar until the sync is successful. The server does the necessary computations while the client waits, and returns the required values to the client
    * InteractiveCPISync
        * Perform CPISync but if there are more than mBar differences, divide the set into `numPartitions` subsets and attempt to CPISync again. This recurses until the sync is successful. The server does the necessary computations while the client waits, and returns the required values to the client

    * IBLTSync
        * Each peer encodes their set into an [Invertible Bloom Lookup Table](https://arxiv.org/pdf/1101.2245.pdf) with a size determined by NumExpElements and the client sends their IBLT to their per. The differences are determined by "subtracting" the IBLT's from each other and attempting to peel the resulting IBLT. The server peer then returns the elements that the client peer needs to update their set
    * OneWayIBLTSync
        * The client sends their IBLT to their server peer and the server determines what elements they need to add to their set. The client does not receive a return message and does not update their set
    * CuckooSync
        * Each peer encodes their set into a [cuckoo filter](https://www.cs.cmu.edu/~dga/papers/cuckoo-conext2014.pdf). Peers exchange their cuckoo filters. Each host infers the elements that are not in its peer by looking them up in the peer's cuckoo filter. Any elements that are not found in the peer's cuckoo filter are sent to it.
* **Included Sync Protocols (Set of Sets):**
    * IBLT Set of Sets
        * Sync using the protocol described [here](https://dl.acm.org/doi/abs/10.1145/3196959.3196988). This sync serializes an IBLT containing a child set into a bitstring where it is then treated as an element of a larger IBLT. Each host recovers the IBLT containing the serialized IBLTs and deserializes each one. A matching procedure is then used to determine which child sets should sync with each other and which elements they need. If this sync is two way this info is then sent back to the peer node. The number of differences in each child IBLT may not be larger than the total number of sets being synced

<a name="References"></a>
## Reference:
If you use this software, please cite the following paper (pdf,
[DOI](http://doi.org/10.1109/TNSM.2022.3164369)):

``` bibtex
@article{bovskov2022gensync,
  title={Gensync: A new framework for benchmarking and optimizing reconciliation of data},
  author={Bo{\v{s}}kov, Novak and Trachtenberg, Ari and Starobinski, David},
  journal={IEEE Transactions on Network and Service Management},
  volume={19},
  number={4},
  pages={4408--4423},
  year={2022},
  publisher={IEEE}
}
```
### Additional literature
If you use this work, please cite any relevant papers below.

#### The main theoretical bases for the approaches in this work are:
* Y. Minsky, A. Trachtenberg, and R. Zippel,
  "Set Reconciliation with Nearly Optimal Communication Complexity",
  IEEE Transactions on Information Theory, 49:9.
  <http://ipsit.bu.edu/documents/ieee-it3-web.pdf>

* Y. Minsky and A. Trachtenberg,
  "Scalable set reconciliation"
  40th Annual Allerton Conference on Communication, Control, and Computing, 2002.
  <http://ipsit.bu.edu/documents/BUTR2002-01.pdf>

#### Relevant applications and extensions can be found at:
* D. Starobinski, A. Trachtenberg and S. Agarwal,
  "Efficient PDA synchronization"
  IEEE Transactions on Mobile Computing 2:1, pp. 40-51 (2003).
  <http://ipsit.bu.edu/documents/efficient_pda_web.pdf>

* S. Agarwal, V. Chauhan and A. Trachtenberg,
  "Bandwidth efficient string reconciliation using puzzles"
  IEEE Transactions on Parallel and Distributed Systems 17:11,pp. 1217-1225 (2006).
  <http://ipsit.bu.edu/documents/puzzles_journal.pdf>

*  M.G. Karpovsky, L.B. Levitin. and A. Trachtenberg,
   "Data verification and reconciliation with generalized error-control codes"
   IEEE Transactions on Information Theory 49:7, pp. 1788-1793 (2003).

* More at <http://people.bu.edu/trachten>.

#### Additional algorithms:
* Eppstein, David, et al. "What's the difference?: efficient set reconciliation without
  prior context." ACM SIGCOMM Computer Communication Review 41.4 (2011): 218-229.

* Goodrich, Michael T., and Michael Mitzenmacher. "Invertible bloom lookup tables."
  49th Annual Allerton Conference on Communication, Control, and Computing (Allerton), 2011.

* Mitzenmacher, Michael, and Tom Morgan. "Reconciling graphs and sets of sets."
  Proceedings of the 37th ACM SIGMOD-SIGACT-SIGAI Symposium on Principles of Database
  Systems. ACM, 2018.

<a name="Contributors"></a>
## Contributors:

Elements of the GenSync project code have been worked on, at various points, by:

* Ari Trachtenberg
* Sachin Agarwal
* Paul Varghese
* Jiaxi Jin
* Jie Meng
* Alexander Smirnov
* Eliezer Pearl
* Sean Brandenburg
* Zifan Wang
* Novak Boškov
* Xingyu Chen
* Nathan Strahs
  
<a name="Acknowledgments"></a>
## Acknowledgments:
* NSF
* Professors:
    * Ari Trachtenberg, trachten@bu.edu, Boston University
    * David Starobinski, staro@bu.edu, Boston University
