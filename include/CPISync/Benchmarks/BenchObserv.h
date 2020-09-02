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

    BenchObserv(BenchParams& params, string& serverStats, string& clientStats,
                bool serverSuccess, bool clientSuccess,
                string& serverException, string& clientException) :
        params (params),
        clientStats (clientStats),
        serverStats (serverStats),
        serverSuccess (serverSuccess),
        clientSuccess (clientSuccess),
        serverException (serverException),
        clientException (clientException) {}

    friend ostream& operator<<(ostream& os, const BenchObserv& bo) {
        os << "Parameters:\n" << bo.params
           << FromFileGen::DELIM_LINE << "\n"
           << "Server stats:\n"
           << FromFileGen::DELIM_LINE << "\n"
           << "Success: " << bo.serverSuccess << " [" << bo.serverException << "]" << "\n"
           << bo.serverStats << "\n"
           << FromFileGen::DELIM_LINE << "\n"
           << "Client stats:\n"
           << FromFileGen::DELIM_LINE << "\n"
           << "Success: " << bo.clientSuccess << " [" << bo.clientException << "]" << "\n"
           << bo.clientStats << "\n";

      return os;
    }

    BenchParams params;   // the parameters used to run this benchmark
    string clientStats;
    string serverStats;
    bool serverSuccess;
    bool clientSuccess;
    string serverException;
    string clientException;
};

#endif // BENCHOBSERV_H
