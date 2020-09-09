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

/**
 * Represents the performance observation of a sync run.
 */
struct BenchObserv {
    BenchObserv() = default;
    ~BenchObserv() = default;

    BenchObserv(BenchParams& params, string& stats, bool success, const string& exception) :
        params (params),
        stats (stats),
        success (success),
        exception (exception) {}

    friend ostream& operator<<(ostream& os, const BenchObserv& bo) {
        os << bo.params
           << "Success: " << bo.success << " [" << bo.exception << "]" << "\n"
           << bo.stats << "\n";

        return os;
    }

    BenchParams params;   // the parameters used to run this benchmark
    string stats;
    bool success;
    string exception;
};

#endif // BENCHOBSERV_H
