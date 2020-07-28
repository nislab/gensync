/* This code is part of the CPISync project developed at Boston
 * University. Please see the README for use and references.
 *
 * @author Novak Boškov <boskov@bu.edu>
 *
 * Created on July, 2020.
 */

#ifndef BENCHOBSERV_H
#define BENCHOBSERV_H

#include <vector>
#include <algorithm>
#include <CPISync/Benchmarks/BenchParams.h>
#include <CPISync/Aux/SyncMethod.h>

using namespace std;

class BenchObserv {
public:
    BenchObserv() = default;
    ~BenchObserv() = default;

    BenchObserv(BenchParams params);

    // Only if all the syncs observed succeeded the object is considered to be true
    operator bool() const {return all_of(success.begin(), success.end(), [](bool v) {return v;});};

    friend ostream& operator<<(ostream& os, const BenchObserv& bo);

    BenchParams params;   // the parameters used to run this benchmark
    vector<bool> success; // whether client and the server hold exactly the same set at the end, for all syncs
    vector<SyncMethod::SyncStats> stats; // statistics of all syncs performed
    vector<bool> errors;  // whether an error occurred during the syncs
};

#endif // BENCHOBSERV_H
