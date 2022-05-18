/* This code is part of the CPISync project developed at Boston
 * University. Please see the README for use and references.
 *
 * @author Novak Boškov <boskov@bu.edu>
 *
 * Created on July, 2020.
 */

#include <CPISync/Benchmarks/BenchParams.h>
#include <CPISync/Syncs/GenSync.h>
#include <assert.h>
#include <chrono>
#include <random>
#include <stdio.h>
#include <thread>
#include <unistd.h>

using namespace std::chrono;

static const string HELP = R"(Usage: ./Benchmarks -p PARAMS_FILE [OPTIONS]

Do not run multiple instances of -m server or -m client in the same
directory at the same time. When server and client are run in two
separate runs of this script, a lock file is created in the current
directory.

OPTIONS:
    -h print this message and exit
    -p PARAMS_FILE to be used.
    -g whether to generate sets or to use those from PARAMS_FILE.
       In SERVER and CLIENT modes data from PARAMS_FILE is always used.
       The first set from PARAMS_FILE is loaded into the peer.
    -i ADD_ELEM_CHUNK_SIZE add elements incrementally in chunks.
       Synchronization is invoked after each chunk is added.
       Can be used only when data is consumed from parameter files
       and this script is run in either SERVER or CLIENT mode.
    -m MODE mode of operation (can be "server", "client", or "both")
    -r PEER_HOSTNAME host name of the peer (requred when -m is client))"
                           "\n";

// When mode is client only, the client cannot start syncing until it
// sees this file.
static const string LOCK_FILE = ".cpisync_benchmarks_server_lock";
#ifdef NETWORK_PROBING
// Where iperf3 server writes it outputs for debug.
static const string IPERF_SERVER_LOGFILE = "/share/iperf_server.log";
static const int SLEEP_BETWEEN_IPERF_MILLIS = 500;
#endif

#ifdef NETWORK_PROBING
// Duration of one iperf3 session.
static const int iperf_dur_s = 1;
#endif

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
 * @param mode the running mode
 * @param addAllElems whether to add all the elements to the GenSync objects
 * @param zipfElems If true, the elements are not taken from benchmark
 * parameters but are generated randomly from a Zipfian distribution
 * @param peerHostname the hostname of the peer if mode is SERVER or CLIENT
 * @return The pair of generated GenSync objects. The first is the server, the
 * second is the client. If mode is SEVER or CLIENT, then the second one is
 * NULL.
 */
GenSyncPair buildGenSyncs(const BenchParams &par, RunningMode mode,
                          bool addAllElems = true, bool zipfElems = false,
                          string peerHostname = "");
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

/**
 * Invoke appropriate GenSync method.
 * @param genSync the genSync object to work with
 * @param ser if true, then invoke the server method, otherwise invoke the
 * client method
 */
inline void callServerOrClientSyncBegin(shared_ptr<GenSync> genSync, bool ser);

/**
 * Adds the elements to genSync in chunks and invokes synchronization
 * after each chunk is added.
 * @param ser if true, invoke server sync begin, otherwise invoke the
 * client version
 * @param chunkSize the size of elements chunks to be added at once
 * @param genSync the GenSync object to work with
 * @param dataGen the source of elements to be added
 */
void invokeSyncIncrementally(bool serOrCli, size_t chunkSize,
                             shared_ptr<GenSync> genSync,
                             shared_ptr<DataObjectGenerator> dataGen);

/**
 * Checks whether element is already added to the SyncMethod under
 * GenSync.  When we do incremental sync, the previous synchronization
 * invocation may have already brought in some of the elements that we
 * are adding. No need to add the same element twice (as we consider
 * only sets here).
 * @param genSync the GenSync object from which we obtain the SyncMethod
 * @param elem the potentially new element we are adding
 * @returns true if the element is already in the set
 */
bool alreadyThere(shared_ptr<GenSync> genSync, DataObject elem);

#ifdef NETWORK_PROBING
/**
 * Estimate the latency and bandwidth of the network and set the
 * AuxMeasurements of the passed GenSync object.
 * @param genSync The * GenSync object whose AuxMeasurements will be set.
 * @param peerIP The IP address of the peer.
 * @return The indicator of success.
 */
bool estimateNetwork(shared_ptr<GenSync> genSync, string &peerIP);

/**
 * Start iperf3 server on the GenSync server side to make network
 * probing possible.
 */
void startIperfServer();
#endif

int main(int argc, char *argv[]) {
    /*********************** Parse command line options ***********************/
    int opt;
    string paramFile = "";
    string peerHostname = "";
    RunningMode mode = BOTH;
    bool generateSets = false;
    string incChunkStr = "";
    size_t incChunk = 0;

    while ((opt = getopt(argc, argv, "p:m:r:i:gh")) != -1) {
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
        case 'i':
            incChunkStr = optarg;
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
        return 1;
    }
    if (peerHostname.empty() && mode == CLIENT) {
        cerr << "When mode is client, you need to pass the hostname of the "
                "server.\n"
             << HELP;
        return 1;
    }
    if (mode != BOTH && generateSets) {
        cerr << "Sets can be generated only in both mode.\n" << HELP;
        return 1;
    }
    if (!incChunkStr.empty() && (paramFile.empty() || mode == BOTH)) {
        cerr << "Incremental elements addition works only when -p is used"
                " and mode is not BOTH."
             << HELP;
        return 1;
    } else {
        stringstream ss;
        ss << incChunkStr;
        try {
            ss >> incChunk;
        } catch (exception &e) {
            cerr << ss.str() << " cannot be converted to integer.\n";
            return 1;
        }
    }

    /**************************** Create the peers ****************************/

    BenchParams bPar = BenchParams{paramFile};
    GenSyncPair genSyncs =
        buildGenSyncs(bPar, mode, !incChunk, generateSets, peerHostname);

    Logger::gLog(Logger::TEST, "Sets are ready, reconciliation starts...");

    if (mode == SERVER) { // run only server
        try {
            if (incChunk) {
                // handles LOCK_FILE internally
                invokeSyncIncrementally(true, incChunk, genSyncs.first,
                                        bPar.AElems);
            } else {
#ifdef NETWORK_PROBING
                startIperfServer();
#endif
                // create the lock file to signalize that the server is ready
                ofstream lock(LOCK_FILE);
                lock.flush();
                lock.close(); // TODO:

                genSyncs.first->serverSyncBegin(0);
            }
        } catch (exception &e) {
            cout << "Sync exception: " << e.what() << "\n";
        }
    } else if (mode == CLIENT) { // run only client
        try {
            if (incChunk) {
                // handles LOCK_FILE internally
                invokeSyncIncrementally(false, incChunk, genSyncs.first,
                                        bPar.AElems);
            } else {
                // wait until the server is ready to start first
                bool waitMsgPrinted = false;
                while (true) {
                    ifstream lock(LOCK_FILE);
                    if (lock.good()) {
                        remove(LOCK_FILE.c_str());
                        break;
                    }

                    if (!waitMsgPrinted) {
                        Logger::gLog(
                            Logger::TEST,
                            "Waiting for the server to create the lock file.");
                        waitMsgPrinted = true;
                    }

                    this_thread::sleep_for(chrono::milliseconds(100));
                }

                Logger::gLog(
                    Logger::TEST,
                    "Client detects that the server is ready to start.");

#ifdef NETWORK_PROBING
                auto ret = estimateNetwork(genSyncs.first, peerHostname);
                if (!ret)
                    Logger::gLog(
                        Logger::TEST,
                        "Network estimation on the client has failed.");
#endif
                genSyncs.first->clientSyncBegin(0);
            }
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
                          bool addAllElem, bool zipfElems,
                          string peerHostname) {
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

    } else if (addAllElem) {
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

inline void callServerOrClientSyncBegin(shared_ptr<GenSync> genSync, bool ser) {
    // Server always needs to make the lock file for the client to
    // start the sync on its side.
    if (ser) {
        ofstream lock(LOCK_FILE);

        if (!lock.good()) {
            cerr << "ERROR: Benchmarks for incremental sync: server did not "
                    "create the lock.";
            exit(1);
        }

        genSync->serverSyncBegin(0);
    } else {
        while (true) {
            // Wait until the server is ready
            ifstream lock(LOCK_FILE);
            if (lock.good()) {
                remove(LOCK_FILE.c_str());
                break;
            }

            // this will be counted as idle time (in case of InterCPISync)
            this_thread::sleep_for(chrono::milliseconds(10));
        }

        genSync->clientSyncBegin(0);
    }
}

bool alreadyThere(shared_ptr<GenSync> genSync, DataObject elem) {
    // We always consider only one SyncAgent in GenSync.
    auto begin = genSync->getSyncAgt(0)->get()->beginElements();
    auto end = genSync->getSyncAgt(0)->get()->endElements();

    for (auto it = begin; it != end; it++)
        if (elem == **it)
            return true;

    return false;
}

void invokeSyncIncrementally(bool ser, size_t chunkSize,
                             shared_ptr<GenSync> genSync,
                             shared_ptr<DataObjectGenerator> dataGen) {
    // The process is as follows:
    // 1. server adds chunkSize elements and
    // creates the lock file when done. Then calls serverSyncBegin.
    // In parallel, client also adds chunkSize and checks whether lock is there.
    // 2. When client detects the lock, it deletes it and calls clientSyncBegin.
    // 3. When sync is done, the process repeats from step 1.

    size_t currentlyAdded = 0;
    for (auto elem : *dataGen) {
        // add only elements that are not already there
        if (alreadyThere(genSync, elem))
            continue;

        genSync->addElem(make_shared<DataObject>(elem));
        currentlyAdded++;

        if (currentlyAdded % chunkSize == 0) {
            stringstream ss;
            ss << "[" << chrono::system_clock::now().time_since_epoch().count()
               << "]: " << currentlyAdded << " elements added. Wants to sync!\n"
               << std::flush;
            Logger::gLog(Logger::TEST, ss.str());

            callServerOrClientSyncBegin(genSync, ser);
        }
    }
}

#ifdef NETWORK_PROBING
#define BUFF_SIZE 4096

/**
 * Handle iperf3 output.
 * @param s The string output of iperf3
 * @param reverse If enabled, then the function parses the output of iperf with
 * '-R' option.
 */
inline float handleIperfOutput(string &s, bool reverse = false) {
    if (s.find("error") != string::npos) {
        Logger::gLog(Logger::TEST, "handleIperfOutput error:\n" + s +
                                       "\nreverse: " + to_string(reverse));

        return -1;
    }

    float iperf_val;

    // get the right line
    size_t right_line = reverse ? 4 : 3;
    size_t line_cnt = 0;
    istringstream s_lines(s);
    string line;
    while (getline(s_lines, line))
        if (line_cnt++ == right_line)
            break;
    // get the right fields
    auto right_fields = {6, 7};
    vector<string> fields;
    istringstream s_fields(line);
    string field, data;
    while (getline(s_fields, field, ' '))
        if (!all_of(field.begin(), field.end(),
                    [](char c) { return isspace(c); }))
            fields.push_back(field);
    for (auto rf : right_fields)
        data += fields.at(rf) + ' ';

    // convert to bits/sec
    string iperf_out_s(data);
    size_t space_pos = iperf_out_s.find(" ");
    if (space_pos == string::npos) {
        Logger::gLog(Logger::TEST, "handleIperfOutput error: no space in '" +
                                       data + "' obtained from:\n" + s +
                                       "\nreverse: " + to_string(reverse));
        return -1;
    }
    string fst_prt = iperf_out_s.substr(0, space_pos);
    if (iperf_out_s.find("Kbits") != string::npos) {
        iperf_val = stof(fst_prt) * 1024;
    } else if (iperf_out_s.find("Mbits") != string::npos) {
        iperf_val = stof(fst_prt) * 1024 * 1024;
    } else if (iperf_out_s.find("Gbits") != string::npos) {
        iperf_val = stof(fst_prt) * 1024 * 1024 * 1024;
    }

    return iperf_val;
}

/**
 * Handle the output of ping.
 * @param s The output.
 * @returns ping time in milliseconds.
 */
inline float handlePingOutput(string &s) {
    if (s.find("error") != string::npos) {
        Logger::gLog(Logger::TEST, "handlePingOutput error:\n" + s);
        return -1;
    }

    size_t time_pos = s.find("time=");
    if (time_pos == string::npos) {
        Logger::gLog(Logger::TEST,
                     "handlePingOutput error: no 'time=' in:\n" + s);
        return -1;
    }
    string part = s.substr(time_pos);
    size_t space_pos = part.find(" ");

    return stof(part.substr(5, space_pos - 5));
}

inline void log_external_fail(string text, int stat_code) {
    Logger::gLog(Logger::TEST, "Popen fail: " + text +
                                   "\nstatus code: " + to_string(stat_code));
}

inline void log_external_run(string &text) {
    Logger::gLog(Logger::TEST, "Popen run: " + text);
}

bool estimateNetwork(shared_ptr<GenSync> genSync, string &peerIP) {
    array<char, BUFF_SIZE> buff;
    string ping_cmd, iperf_u_cmd, iperf_d_cmd, ping_out, iperf_u_out,
        iperf_d_out;
    float iperf_u_val, iperf_d_val, ping_val;

    // Build the external commands
    ping_cmd = "ping -c 1 " + peerIP;
    iperf_u_cmd = "iperf3 -c " + peerIP + " -t " + to_string(iperf_dur_s);
    iperf_d_cmd =
        "iperf3 -c " + peerIP + " -t " + to_string(iperf_dur_s) + " -R";

    // Start timer to measure the time needed to estimate the network
    // performance.
    auto start = high_resolution_clock::now();

    log_external_run(iperf_u_cmd);
    FILE *iperf_u = popen(iperf_u_cmd.c_str(), "r");
    if (iperf_u == nullptr) {
        log_external_fail("First iperf failed", pclose(iperf_u));
        return false;
    }
    while (fgets(buff.data(), BUFF_SIZE, iperf_u) != nullptr)
        iperf_u_out += buff.data();
    // make sure we wait until the first iperf call ends, then wait a
    // little more before sending the second iperf call to give the
    // server a chance to get ready.
    pclose(iperf_u);
    this_thread::sleep_for(chrono::milliseconds(SLEEP_BETWEEN_IPERF_MILLIS));
    iperf_u_val = handleIperfOutput(iperf_u_out);

    log_external_run(iperf_d_cmd);
    FILE *iperf_d = popen(iperf_d_cmd.c_str(), "r");
    log_external_run(ping_cmd);
    FILE *ping = popen(ping_cmd.c_str(), "r");
    if (iperf_d == nullptr) {
        log_external_fail("Second iperf failed", pclose(iperf_d));
        return false;
    }
    while (fgets(buff.data(), BUFF_SIZE, iperf_d) != nullptr)
        iperf_d_out += buff.data();

    if (ping == nullptr) {
        log_external_fail("Ping failed", pclose(ping));
        return false;
    }
    while (fgets(buff.data(), BUFF_SIZE, ping) != nullptr)
        ping_out += buff.data();
    pclose(iperf_d);
    pclose(ping);
    iperf_d_val = handleIperfOutput(iperf_d_out, true);
    ping_val = handlePingOutput(ping_out);

    // Stop the timer and log the time it took to estimate the
    // network conditions
    auto end = high_resolution_clock::now();
    size_t duration = duration_cast<milliseconds>(end - start).count();
    Logger::gLog(Logger::TEST,
                 "NETWORK PROBING succeeded and took: " + to_string(duration) +
                     "ms @ SLEEP_BETWEEN_IPERF_MILLIS: " +
                     to_string(SLEEP_BETWEEN_IPERF_MILLIS));
    // Update genSync object
    auto ams = make_shared<AuxMeasurements>(ping_val, iperf_u_val, iperf_d_val,
                                            duration);
    genSync->setAuxMeasurements(ams);

    return true;
}

void startIperfServer() {
    string is_already_running_cmd = "pgrep iperf3";
    string run_cmd = "iperf3 -s -D --logfile=" + IPERF_SERVER_LOGFILE;
    stringstream ss;
    int run_ret;

    // We need to remove the log file since iperf3 --logfile appends
    // to file and we want it fresh.
    if (remove(IPERF_SERVER_LOGFILE.c_str()) != 0)
        Logger::gLog(Logger::TEST,
                     IPERF_SERVER_LOGFILE +
                         " failed to remove with errno: " + to_string(errno));

    int ret = system(is_already_running_cmd.c_str());
    switch (ret) {
    case 1:
    case 256:
        run_ret = system(run_cmd.c_str());
        ss << "'" << run_cmd << "' run with status code " << run_ret << endl;
        Logger::gLog(Logger::TEST, ss.str());
        break;
    case 0:
        Logger::gLog(Logger::TEST, "iperf3 server is already running.");
        break;
    case -1:
        throw runtime_error(
            "Shell process cannot be created or its status code "
            "cannot be obtained.");
        break;
    case 127:
        throw runtime_error("Shell cannot be executed.");
        break;
    default:
        ss << "Status code " << ret << " is unexpected as a return value of '"
           << is_already_running_cmd << "'";
        throw logic_error(ss.str());
    }
}
#endif
