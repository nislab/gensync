/* This code is part of the CPISync project developed at Boston
 * University. Please see the README for use and references.
 *
 * @author Novak Boškov <boskov@bu.edu>
 *
 * Created on July, 2020.
 */

// TODO: Support dynamic set evolution while syncing

#include <CPISync/Benchmarks/BenchParams.h>
#include <CPISync/Syncs/GenSync.h>
#include <assert.h>
#include <chrono>
#include <random>
#include <thread>
#include <unistd.h>

static const string HELP = R"(Usage: ./Benchmarks -p PRAMS_FILE [OPTIONS]

Do not run multiple instances of -m server or -m client in the same
directory at the same time. When server and client are run in two
separate runs of this script, a lock file is created in the current
directory.

OPTIONS:
    -h print this message and exit
    -g whether to generate sets or to use those from PARAMS_FILE.
       In SERVER and CLIENT modes data from PARAMS_FILE is always used.
       The first set from PARAMS_FILE is loaded into the peer.
    -m MODE mode of operation (can be "server", "client", or "both")
    -r PEER_HOSTNAME host name of the peer (requred when -m is client))";

// When mode is client only, the client cannot start syncing until it
// sees this file.
static const string LOCK_FILE = ".cpisync_benchmarks_server_lock";

using namespace std;
using GenSyncPair = pair<shared_ptr<GenSync>, shared_ptr<GenSync>>;

// hardware source of randomness
static std::random_device rd;
static std::mt19937 gen(rd());

// maximum cardinality of a set.
// The cardinality in the multiset sense depends on the repetitions of the
// elements, which is also random.
static const size_t MAX_CARD = 1 << 10;
// one item can repeat only up to (CARD_MAX / REP_RATIO) + 1 times
static const size_t REP_RATIO = 1 << 7;
// maximum value of an element
static const size_t MAX_DOBJ = 1 << 31;
// default set cardinality
static const size_t DEFAULT_CARD = 1;
// default Zipfian distribution alpha parameter
static const double ZF_ALPHA = 1.0;

// distribution used in Zipfian distribution
static uniform_real_distribution<double> dist(0.0, 1.0);
// synthetic set cardinalities
static uniform_int_distribution<int> card(128, MAX_CARD);
// synthetic common elements count
static uniform_int_distribution<int> comm(MAX_CARD * 0.03, MAX_CARD * 0.5);
// DataObject value distribution
static uniform_int_distribution<size_t> elem(0, MAX_DOBJ);

/**
 * What part of the reconciliation protocol does this program run.
 */
enum RunningMode { CLIENT, SERVER, BOTH };

/**
 * Builds GenSync objects for syncing.
 * @param par The benchmark parameters objects that holds the details
 * @param zipfElems If true, the elements are not taken from benchmark
 * parameters but are generated randomly from a Zipfian distribution
 * @param mode the running mode
 * @param peerHostname the hostname of the peer if mode is SERVER or CLIENT
 * @return The pair of generated GenSync objects. The first is the server, the
 * second is the client. If mode is SEVER or CLIENT, then the second one is
 * NULL.
 */
GenSyncPair buildGenSyncs(const BenchParams &par, RunningMode mode,
                          bool zipfElems = false, string peerHostname = "");
/**
 * Generates zipf distributed random numbers from 1 to n.
 * @param n The highest number
 * @param alpha The alpha parameter of Zipfian distribution
 * @return A number sampled from the distribution
 */
int zipf(int n, double alpha = ZF_ALPHA);
/**
 * Makes a new DataObject sampled from a uniform distribution.
 * @return The constructed object
 */
inline shared_ptr<DataObject> makeObj();
/**
 * Adds elements to the passed GenSync object. Handles the repetitions.
 * @param gs The GenSync object
 * @param obj The element to add
 */
inline void addAnElemWithReps(shared_ptr<GenSync> gs,
                              shared_ptr<DataObject> obj);

int main(int argc, char *argv[]) {
    /*********************** Parse command line options ***********************/
    int opt;
    string paramFile = "";
    string peerHostname = "";
    RunningMode mode = BOTH;
    bool generateSets = false;
    while ((opt = getopt(argc, argv, "p:m:r:gh")) != -1) {
        switch (opt) {
        case 'p':
            paramFile = optarg;
            break;
        case 'r':
            peerHostname = optarg;
            break;
        case 'm': {
            char *value = optarg;
            if (strcmp(value, "server") == 0)
                mode = SERVER;
            else if (strcmp(value, "client") == 0)
                mode = CLIENT;
            else if (strcmp(value, "both") == 0)
                mode = BOTH;
            else {
                cerr << "Invalid option for running mode.\n";
                return 0;
            }
            break;
        }
        case 'g':
            generateSets = true;
            break;
        case 'h':
            cout << HELP;
            return 0;
        default:
            cerr << HELP;
            return 0;
        }
    }

    // Options that make no sense.
    if (paramFile.empty()) {
        cerr << "You need to pass the parameters file.\n" << HELP;
        return 0;
    }
    if (peerHostname.empty() && mode == CLIENT) {
        cerr << "When mode is client, you need to pass the hostname of the "
                "server.\n"
             << HELP;
        return 0;
    }
    if (mode != BOTH && generateSets) {
        cerr << "Sets can be generated only in both mode.\n" << HELP;
        return 0;
    }

    /**************************** Create the peers ****************************/

    BenchParams bPar = BenchParams{paramFile};
    GenSyncPair genSyncs =
        buildGenSyncs(bPar, mode, generateSets, peerHostname);

    Logger::gLog(Logger::TEST, "Sets are ready, reconciliation starts...");

    if (mode == SERVER) { // run only server
        // create the lock file to signalize that the server is ready
        ofstream lock(LOCK_FILE);

        try {
            genSyncs.first->serverSyncBegin(0);
        } catch (exception &e) {
            cout << "Sync exception: " << e.what() << "\n";
        }
    } else if (mode == CLIENT) { // run only client
        // wait until the server is ready to start first
        bool waitMsgPrinted = false;
        while (true) {
            ifstream lock(LOCK_FILE);
            if (lock.good()) {
                remove(LOCK_FILE.c_str());
                break;
            }

            if (!waitMsgPrinted) {
                Logger::gLog(Logger::TEST,
                             "Waiting for the server to create the lock file.");
                waitMsgPrinted = true;
            }

            this_thread::sleep_for(chrono::milliseconds(100));
        }

        Logger::gLog(Logger::TEST,
                     "Client detects that the server is ready to start.");

        try {
            genSyncs.first->clientSyncBegin(0);
        } catch (exception &e) {
            cout << "Sync exception: " << e.what() << "\n";
        }
    } else {
        // run both
        pid_t pid = fork();
        if (pid == 0) { // child process
            try {
                genSyncs.first->serverSyncBegin(0);
            } catch (exception &e) {
                cout << "Sync Exception [server]: " << e.what() << "\n";
            }
        } else if (pid > 0) { // parent process
            try {
                genSyncs.second->clientSyncBegin(0);
            } catch (exception &e) {
                cout << "Sync Exception [client]: " << e.what() << "\n";
            }
        } else if (pid < 0) {
            throw runtime_error("Fork has failed");
        }
    }
}

GenSyncPair buildGenSyncs(const BenchParams &par, RunningMode mode,
                          bool zipfElems, string peerHostname) {
    GenSync::Builder builderA = GenSync::Builder()
                                    .setComm(GenSync::SyncComm::socket)
                                    .setProtocol(par.syncProtocol);

    GenSync::Builder builderB = GenSync::Builder()
                                    .setComm(GenSync::SyncComm::socket)
                                    .setProtocol(par.syncProtocol);

    if (mode == CLIENT)
        builderA.setHost(peerHostname);

    par.syncParams->apply(builderA);
    if (mode == BOTH)
        par.syncParams->apply(builderB);

    shared_ptr<GenSync> genA = make_shared<GenSync>(builderA.build());
    shared_ptr<GenSync> genB = NULL;
    if (mode == BOTH)
        genB = make_shared<GenSync>(builderB.build());

    if (zipfElems && mode == BOTH) {
        size_t cardA = DEFAULT_CARD;
        size_t cardB = DEFAULT_CARD;
        size_t common = comm(gen);

        do {
            cardA = card(gen);
        } while (cardA < common + 1); // at least one local element

        do {
            cardB = card(gen);
        } while (cardB < common + 1);

        stringstream ss;
        ss << "Benchmarks generated sets:  Peer A: " << cardA
           << ", Peer B: " << cardB << ", Common: " << common;
        Logger::gLog(Logger::TEST, ss.str());

        Logger::gLog(Logger::TEST, "Adding common elements...");
        for (size_t ii = 0; ii < common; ii++) {
            auto obj = makeObj();
            addAnElemWithReps(genA, obj);
            addAnElemWithReps(genB, obj);
        }

        Logger::gLog(Logger::TEST, "Adding peer A local elements...");
        for (size_t ii = 0; ii < (cardA - common); ii++)
            addAnElemWithReps(genA, makeObj());

        Logger::gLog(Logger::TEST, "Adding peer B local elements...");
        for (size_t ii = 0; ii < (cardB - common); ii++)
            addAnElemWithReps(genB, makeObj());

    } else {
        for (auto elem : *par.AElems)
            genA->addElem(make_shared<DataObject>(elem));

        if (mode == BOTH)
            for (auto elem : *par.BElems)
                genB->addElem(make_shared<DataObject>(elem));
    }

    return {genA, genB};
}

/**
 * Original implementation from Ken Christensen
 * https://www.csee.usf.edu/~kchriste/tools/genzipf.c
 *
 * TODO: This should be implemented as a part of RandGen
 */
int zipf(int n, double alpha) {
    // The map of c constants for Zipfian distributions.
    // The idea is that all the calls with the same n sample from the exact same
    // distribution. Thus for each n, c need be calculated only once.
    static map<int, double> c_map;

    double c = 0;          // Normalization constant
    double z = 0;          // Uniform random number (0 < z < 1)
    double sum_prob = 0;   // Sum of probabilities
    double zipf_value = 0; // Computed exponential value to be returned
    int i = 0;             // Loop counter

    if (c_map.find(n) == c_map.end()) {
        for (i = 1; i <= n; i++)
            c = c + (1.0 / pow((double)i, alpha));

        c = 1.0 / c;
        c_map.insert(std::pair<int, double>(n, c));
    } else {
        c = c_map.at(n);
    }

    // Pull a uniform random number (0 < z < 1)
    do {
        z = dist(gen);
    } while (z == 0 || z == 1);

    // Map z to the value
    sum_prob = 0;
    for (i = 1; i <= n; i++) {
        sum_prob = sum_prob + c / pow((double)i, alpha);
        if (sum_prob >= z) {
            zipf_value = i;
            break;
        }
    }

    // Assert that zipf_value is between 1 and N
    assert((zipf_value >= 1) && (zipf_value <= n));

    return zipf_value;
}

shared_ptr<DataObject> makeObj() {
    return make_shared<DataObject>(DataObject(ZZ(elem(rd))));
}

void addAnElemWithReps(shared_ptr<GenSync> gs, shared_ptr<DataObject> obj) {
    for (size_t rep = 0; rep < zipf(MAX_CARD / REP_RATIO); rep++)
        gs->addElem(obj);
}
