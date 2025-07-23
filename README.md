# GenSync - 2.0.4
GenSync is a framework for _efficiently_ synchronizing similar data across multiple devices.

The framework provides a shared library for benchmarking and optimizing a variety of state-of-the-art data synchronization protocols, either offline or directly embedded within application code.  In one typical use-case, an application would use GenSync for its core data synchronization needs, and developers can compare and optimize the performance of different synchronization protocols to suit their needs.  Alternatively, users could utilize the GenSync library to profile synchronization usage for their application, and then experiment with synchronization protocols offline to improve perfromance.

The current implementation of the framework includes four families of data synchronization protocols (and [their variants](#SyncTypes)):
- _FullSync_ - a trivial protocol that transfers all data from one device to another for a local comparison
- _CPISync_ - based on Characteristic Polynomial Interpolation
- _IBLTSync_ - based on Invertible Bloom Lookup Tables
- _CuckooSync_ - based on Cuckoo tables

---
## Contents:
   * [Motivating example](README.md#Motivation)
   * [Code](README.md#Code)
   * [Supported protocols](README.md#SyncTypes)
   * [References](README.md#References)
   * [Contributors](README.md#Contributors)
   * [Acknowledgments](README.md#Acknowledgments)

---
<a name="Motivation"></a>
## Motivating example
Suppose that laptop `A` has a list of contacts:

> Alice, Jim, Jane, Rick, Bob


and cellphone `B` has the contacts:
> Alice, Jim, Suzie, Jane, Rick

Then an efficient synchronization protocol might quickly idenfity the differences (`Bob` on the laptop, and `Suzie` on the cellphone)
and exchange only these contacts, rather than sending the entire contact list from one device to another.

---
<a name="Code"></a>
## Code
The source code for this library is divided among several repositories.

- [gensync-core](https://github.com/nislab/gensync-core) - the core framework code that is used by other repositories.
- [gensync-lib](https://github.com/nislab/gensync-lib) - used to produce the GenSync library and associated headers.
- [gensync-macports](https://github.com/nislab/gensync-macports) - used to produce the [macports](https://ports.macports.org/port/gensync/details/) version of the GenSync library for MacOS.
- [gensync-benchmarking](https://github.com/nislab/gensync-benchmarking) - used for including benchmarking capabilities.

---
<a name="SyncTypes"></a>
## Supported protocols:
Each of these protocols is implemented as a peer-to-peer protocol.  For purposes of explanation, one peer is called a _client_ and the other a _server_ .
* __FullSync family__
  * *FullSync*
     * The client sends all its data to the server, which determines what differs between the two devices and notifies the client of the differences.
* __CPISync family__
    * *CPISync*
        * Implements the Characteristic Polynomial Interpolation protocol from [MTZ03](http://ipsit.bu.edu/documents/ieee-it3-web.pdf) _with a known bound_ on the number of differences between client and server. The client sends evaluations of the characteristic polynomial of _hashes_ of its data to the server.  The server interpolates (and factors) a rational function that identifies the differences between client and server, and then reports the hashes of these differences back to the client.  A final communication round exchanges the actual data corresponding to the identified hashes.
    * *ProbCPISync*
        * Perform CPISync _without a known bound_ on the number of differences between client and server.  Client and server start with a guess on the difference bound.  If the guess is wrong, it is doubled and the process is repeated, until a correct bound is identified, whereupon CPISync is run.
    * *InteractiveCPISync*
        * Performs CPISync interactively according to the protocol of [MT02](http://ipsit.bu.edu/documents/BUTR2002-01.pdf).  No bound on differences is needed, and both computation and communication are efficient.
    * *CPISync_OneLessRound*
        * A variant of CPISync that does not using hashes ( _i.e.,_ it operates directly on the data ).  This avoides the final communication round where hash inverses are exchanged between client and server.
    * *OneWayCPISync*
        * A variant of CPISync that communications only in one direction, providing the server with information about the clients data (but not vice versa). This requires no feedback from server to client, and the transmitted data can be stored as a string for subsequent synchronization.
* __IBLTSync family__
    * *IBLTSync*
        * Client and server encode and exchange their data within an [Invertible Bloom Lookup Table](https://arxiv.org/pdf/1101.2245.pdf).  Subtracting these tables and deecoding the result provides the differences between client and server
    * *OneWayIBLTSync*
        * A variant of IBLTSync that communications only in one direction, providing the server with information about the clients data (but not vice versa). This requires no feedback from server to client, and the transmitted data can be stored as a string for subsequent synchronization.
   * *Set of Sets Sync*
        * This implements an IBLT-based synchronization of sets of sets described in [MM18](https://dl.acm.org/doi/abs/10.1145/3196959.3196988).
   * *MET-IBLT Sync*
        * The [Multi-Edge-Type IBLT](https://arxiv.org/pdf/2211.05472) is an IBLT-based set reconciliation protocol that does not require estimation of the size of the set-difference. This is due to the scalable nature of the MET-IBLT data structure.
   * *Bloom Filter Sync*
        * The [Bloom Filter](https://dl.acm.org/doi/pdf/10.1145/362686.362692) is a space-efficient probabilistic data structure for testing set membership, and its protocol enables set reconciliation by exchanging filters and transferring only elements likely missing from the other party's set.
* __CuckooSync family__
    * CuckooSync
        *  Client and server encode and exchange their data within a [cuckoo filter](https://www.cs.cmu.edu/~dga/papers/cuckoo-conext2014.pdf).  A comparison of the filters yields the differences between client and server

---
<a name="References"></a>
## References:
If you use this software, please cite _at least_ the following paper (pdf,
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
The following works are also significant to this software implementation:

#### Theoretical foundations
* [MTZ03] Y. Minsky, A. Trachtenberg, and R. Zippel,
  "Set Reconciliation with Nearly Optimal Communication Complexity",
  IEEE Transactions on Information Theory, 49:9.
  <http://ipsit.bu.edu/documents/ieee-it3-web.pdf>

* [MT02] Y. Minsky and A. Trachtenberg,
  "Scalable set reconciliation"
  40th Annual Allerton Conference on Communication, Control, and Computing, 2002.
  <http://ipsit.bu.edu/documents/BUTR2002-01.pdf>

#### Applications and extensions
* [DTA03] D. Starobinski, A. Trachtenberg and S. Agarwal,
  "Efficient PDA synchronization"
  IEEE Transactions on Mobile Computing 2:1, pp. 40-51 (2003).
  <http://ipsit.bu.edu/documents/efficient_pda_web.pdf>

* [SCT06] S. Agarwal, V. Chauhan and A. Trachtenberg,
  "Bandwidth efficient string reconciliation using puzzles"
  IEEE Transactions on Parallel and Distributed Systems 17:11,pp. 1217-1225 (2006).
  <http://ipsit.bu.edu/documents/puzzles_journal.pdf>

* [KLT03] M.G. Karpovsky, L.B. Levitin. and A. Trachtenberg,
   "Data verification and reconciliation with generalized error-control codes"
   IEEE Transactions on Information Theory 49:7, pp. 1788-1793 (2003).
  
* [EGUV11] D. Eppstein, M.T. Goodrich, F. Uyeda, and G. Varghese.
  "What's the difference?: efficient set reconciliation without prior context."
  ACM SIGCOMM Computer Communication Review 41.4 (2011): 218-229.

* [GM11] M.T. Goodrich and M. Mitzenmacher. "Invertible bloom lookup tables."
  49th Annual Allerton Conference on Communication, Control, and Computing (Allerton), 2011.

* [MM18] M. Mitzenmacher and T. Morgan. "Reconciling graphs and sets of sets."
  Proceedings of the 37th ACM SIGMOD-SIGACT-SIGAI Symposium on Principles of Database
  Systems. ACM, 2018.

* More at <https://people.bu.edu/trachten>.

---
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
* Anish Sinha
* Thomas Poimenidis

---
<a name="Acknowledgments"></a>
## Acknowledgments:
* NSF
* Red Hat Collaboratory
* Professors:
    * Ari Trachtenberg, trachten@bu.edu, Boston University
    * David Starobinski, staro@bu.edu, Boston University
