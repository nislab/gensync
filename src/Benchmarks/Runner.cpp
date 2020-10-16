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

#include <assert.h>
#include <random>
#include <CPISync/Syncs/GenSync.h>
#include <CPISync/Benchmarks/BenchParams.h>

using namespace std;
using GenSyncPair = pair<shared_ptr<GenSync>, shared_ptr<GenSync>>;

static default_random_engine gen;
static uniform_real_distribution<double> dist(0.0, 1.0);
// synthetic set cardinalities
static uniform_int_distribution<int> card(128, 1024);
// synthetic common elements count
static uniform_int_distribution<int> comm(64, 512);
// maximum value for the elements
static const size_t MAX_DOBJ = 2 << 10;

// Builds GenSync objects for sync
GenSyncPair buildGenSyncs(const BenchParams& par, bool zipfElems = false);

// Generates zipf distributed random numbers from 0 to n
int zipf(double alpha, int n);
// Makes a new DataObject sampled from Zipfian distribution
inline shared_ptr<DataObject> makeObj();

int main(int argc, char* argv[]) {
    BenchParams bPar = BenchParams{argv[1]};
    GenSyncPair genSyncs = buildGenSyncs(bPar, argc >= 2);

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

GenSyncPair buildGenSyncs(const BenchParams& par, bool zipfElems) {
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

    if (zipfElems) {
        size_t cardA;
        size_t cardB;
        size_t common = comm(gen);

        do {
            cardA = card(gen);
        } while (cardA < common + 1); // at least one local element

        do {
            cardB = card(gen);
        } while (cardB < common + 1);

        for (size_t ii = 0; ii < common; ii++) {
            auto dobj = makeObj();
            server->addElem(dobj);
            client->addElem(dobj);
        }

        for (size_t ii = 0; ii < (cardA - common); ii++)
            server->addElem(makeObj());

        for (size_t ii = 0; ii < (cardB - common); ii++)
            client->addElem(makeObj());

    } else {
        for (auto elem : *par.serverElems)
            server->addElem(make_shared<DataObject>(elem));

        for (auto elem : *par.clientElems)
            client->addElem(make_shared<DataObject>(elem));
    }

    return {server, client};
}

/**
 * Original implementation from Ken Christensen
 * https://www.csee.usf.edu/~kchriste/tools/genzipf.c
 */
int zipf(double alpha, int n) {
    static int first = true;      // Static first time flag
    static double c = 0;          // Normalization constant
    double z;                     // Uniform random number (0 < z < 1)
    double sum_prob;              // Sum of probabilities
    double zipf_value = 0;        // Computed exponential value to be returned
    int    i = 0;                 // Loop counter

    // Compute normalization constant on first call only
    if (first == true)
    {
        for (i=1; i<=n; i++)
            c = c + (1.0 / pow((double) i, alpha));
        c = 1.0 / c;
        first = false;
    }

    // Pull a uniform random number (0 < z < 1)
    do {
        z = dist(gen);
    } while (z == 0 || z ==1);

    // Map z to the value
    sum_prob = 0;
    for (i=1; i<=n; i++)
    {
        sum_prob = sum_prob + c / pow((double) i, alpha);
        if (sum_prob >= z)
        {
            zipf_value = i;
            break;
        }
    }

    // Assert that zipf_value is between 1 and N
    assert((zipf_value >=1) && (zipf_value <= n));

    return(zipf_value);
}

shared_ptr<DataObject> makeObj() {
    return make_shared<DataObject>(DataObject(ZZ(zipf(0.5, MAX_DOBJ))));
}
