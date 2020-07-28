/* This code is part of the CPISync project developed at Boston
 * University. Please see the README for use and references.
 *
 * @author Novak Boškov <boskov@bu.edu>
 *
 * Created on July, 2020.
 */

#include <CPISync/Benchmarks/BenchParams.h>

const string BenchParams::PARAM_DELIM = string(80, '-');

BenchParams::BenchParams(shared_ptr<DataObjectGenerator> serverElems, shared_ptr<DataObjectGenerator> clientElems,
                         GenSync::SyncProtocol prot, size_t similar, size_t serverMinusClient,
                         size_t clientMinusServer, bool multiset) :
    syncProtocol(prot),
    similarCount(similar),
    serverMinusClientCount(serverMinusClient),
    clientMinusServerCount(clientMinusServer),
    multiset(multiset),
    serverElems (serverElems),
    clientElems (clientElems) {}

ostream& operator<<(ostream& os, const BenchParams& bp) {
    os << "Sync protocol (as in GenSync.h): " << (int) bp.syncProtocol << "\n"
       << "# similar elements: " << bp.similarCount << "\n"
       << "# server only elements: " << bp.serverMinusClientCount << "\n"
       << "# client only elements: " << bp.clientMinusServerCount << "\n"
       << "multiset: " << std::boolalpha << bp.multiset << "\n"
       << BenchParams::PARAM_DELIM << "\n"
       << "Sync params:\n" << bp.syncParams << "\n"
       << BenchParams::PARAM_DELIM;

    return os;
}
