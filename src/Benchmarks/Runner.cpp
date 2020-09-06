/* This code is part of the CPISync project developed at Boston
 * University. Please see the README for use and references.
 *
 * @author Novak Boškov <boskov@bu.edu>
 *
 * Created on July, 2020.
 *
 * Usage:
 * $ ./Benchmarks PATH_TO_PARAMS_FILE
 */

// TODO: Support dynamic set evolution while syncing
// TODO: Keep all the runs as separate from each other as possible

#include <CPISync/Syncs/GenSync.h>
#include <CPISync/Benchmarks/BenchParams.h>

using namespace std;
using GenSyncPair = pair<shared_ptr<GenSync>, shared_ptr<GenSync>>;

// Builds GenSync objects for sync
GenSyncPair buildGenSyncs(const BenchParams& par);

int main(int argc, char* argv[]) {
    BenchParams bPar = BenchParams{argv[1]};
    GenSyncPair genSyncs = buildGenSyncs(bPar);

    pid_t pid = fork();
    if (pid == 0) { // child process
        try {
            genSyncs.first->serverSyncBegin(0);
        } catch (exception& e) {
            cout << "Sync Exception [server]: " << e.what() << "\n";
        }
    } else if (pid > 0) { // parent process
        try {
            genSyncs.second->clientSyncBegin(0);
        } catch (exception& e) {
            cout << "Sync Exception [client]: " << e.what() << "\n";
        }
} else if (pid < 0) {
        throw runtime_error("Fork has failed");
    }
}

GenSyncPair buildGenSyncs(const BenchParams& par) {
    GenSync::Builder serverBuilder = GenSync::Builder()
        .setComm(GenSync::SyncComm::socket)
        .setProtocol(par.syncProtocol);
    GenSync::Builder clientBuilder = GenSync::Builder()
        .setComm(GenSync::SyncComm::socket)
        .setProtocol(par.syncProtocol);

    par.syncParams->apply(serverBuilder);
    par.syncParams->apply(clientBuilder);
    auto server = make_shared<GenSync>(serverBuilder.build());
    auto client = make_shared<GenSync>(clientBuilder.build());

    for (auto elem : *par.serverElems)
        server->addElem(make_shared<DataObject>(elem));

    for (auto elem : *par.clientElems)
        client->addElem(make_shared<DataObject>(elem));

    return {server, client};
}
