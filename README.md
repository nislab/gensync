# GenSync

GenSync is a tool for benchmarking and optimizing reconciliation of data. It implements multiple synchronization protocols including: CPISync, FullSync, IBLTSync and CuckooSync. It can be compiled to a shared library and integrated into other applications, or can be built as a standalone application and used to benchmark the implemented algorithms under a broad range of practical compute and network conditions.

We provide different versions for Linux and MacOS with different usage.
| Version | Discription | Platform |
| --------------- | --------------- | --------------- |
| [gensync-core](https://github.com/nislab/gensync-core) | Includes only source code and tests for development | NA |
| [gensync-lib-linux](https://github.com/nislab/gensync-lib-linux) | Linux library providing synchronization protocols | Linux |
| [gensync-macports](https://github.com/nislab/gensync-macports) | MacOS library providing synchronization protocols(also available on [macports](https://ports.macports.org/port/gensync/details/)) | MacOS |
| [gensync-benchmarking](https://github.com/nislab/gensync-benchmarking) | Provides both library and benchmarking modes for syncing interactively | Linux |

The current version is 2.0.4

If you use this software, please cite the following paper (pdf,
[DOI](http://doi.org/10.1109/TNSM.2022.3164369)):

``` bibtex
@ARTICLE{gensync,
  author={Boškov, Novak and Trachtenberg, Ari and Starobinski, David},
  journal={IEEE Transactions on Network and Service Management},
  title={GenSync: A New Framework for Benchmarking and Optimizing Reconciliation of Data},
  year={2022},
  volume={},
  number={},
  pages={1-1},
  doi={10.1109/TNSM.2022.3164369}
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
### Contributors:

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

# Acknowledgments:
* NSF
* Professors:
    * Ari Trachtenberg, trachten@bu.edu, Boston University
    * David Starobinski, staro@bu.edu, Boston University
