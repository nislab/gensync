/* This code is part of the CPISync project developed at Boston
 * University. Please see the README for use and references.
 *
 * @author Novak Boškov <boskov@bu.edu>
 *
 * Created on July, 2020.
 *
 * Usage:
 * $ ./Benchmarks [-proto PROTOCOL] [-sim SIMILAR] [-smc SMC] [-cms CMS] [-multi MULTISET] [-in "FILENAME,FILENAME,FILENAME"] [-out FILENAME]
 * Example:
 * $ ./Benchmarks -proto CPISync -sim 12 -smc 100 -cms 122 -multi false -out my_out_file.txt
 *
 * If -in is specified, all the command line arguments except -out are
 * ignored and their values are read from the file. File paths are given in the following order:
 * parameters file, server data file, client data file
 *
 * PROTOCOL := "CPISync" | "ProbCPISync" | ... (See parse function)
 * SIMILAR := int
 * SMC := int
 * CMS := int
 * MULTISET := "true" | "false"
 * FILENAME := string
 *
 * See the CMDParams definition bellow.
 */

// TODO: Support dynamic set evolution while syncing
// TODO: Support marshaling and unmarshaling of BenchParams for reuse in different syncs
// TODO: Keep all the runs as separate from each other as possible

#include <iostream>
#include <sstream>
#include <limits>
#include <unistd.h>
#include <CPISync/Syncs/GenSync.h>
#include <CPISync/Benchmarks/BenchParams.h>
#include <CPISync/Benchmarks/BenchObserv.h>
#include <CPISync/Benchmarks/RandGen.h>
#include <CPISync/Benchmarks/FromFileGen.h>

#include <CPISync/Aux/Auxiliary.h>

using namespace std;
using GeneratorPair = pair<shared_ptr<DataObjectGenerator>, shared_ptr<DataObjectGenerator>>;
using GenSyncPair = pair<shared_ptr<GenSync>, shared_ptr<GenSync>>;

const float targetMachineLoad = 0.9;

struct CMDParams {
    static const size_t uspec = numeric_limits<size_t>::max(); // parameter unspecified
    enum DataSource {FILE,    // from log file
                     RANDOM}; // generate data at random

    GenSync::SyncProtocol protocol = GenSync::SyncProtocol::CPISync;
    size_t sim = uspec;             // count of similar elements between server and client
    size_t smc = uspec;             // count of elements only in server
    size_t cms = uspec;             // count of elements in client only
    bool multiset = false;          // whether the sets are multisets
    DataSource dataS = DataSource::RANDOM;        // where to get data
    string outFile = "benchmark_out.txt";         // the name of output file
    string serverDataFile = "";      // file that stores the server data
    string clientDataFile = "";      // file that stores the client data
    string paramsFile = "";          // file that stores the parameters
};

// The ugly command line arguments parsing
CMDParams parse(int argc, char* argv[]);
// Builds the set generators to yield the sets for reconciliation
GeneratorPair buildGenerators(const CMDParams& par);
// Builds GenSync objects for sync
GenSyncPair buildGenSyncs(const BenchParams& par);

int main(int argc, char* argv[]) {
    CMDParams par = parse(argc, argv);
    GeneratorPair sets = buildGenerators(par);
    BenchParams bPar{sets.first, sets.second, par.protocol, par.sim, par.smc, par.cms, par.multiset,
                     par.serverDataFile, par.clientDataFile, par.paramsFile};
    GenSyncPair genSyncs = buildGenSyncs(bPar);

    bool serverSuccess = false, clientSuccess = false;
    string serverStats, clientStats, serverEx = "", clientEx = "";

    pid_t pid = fork();
    if (pid == 0) { // child process
        try {
            serverSuccess = genSyncs.first->serverSyncBegin(0);
            serverStats = genSyncs.first->printStats(0);
        } catch (exception& e) {
            cout << e.what() << endl;
            serverEx = e.what();
        }
    } else if (pid > 0) { // parent process
        try {
            clientSuccess = genSyncs.second->clientSyncBegin(0);
            clientStats = genSyncs.second->printStats(0);
        } catch (exception& e) {
            cout << e.what() << endl;
            clientEx = e.what();
        }
    } else if (pid < 0) {
        throw runtime_error("Fork has failed");
    }

    // write the benchmark statistics to the file
    BenchObserv bObs{bPar, serverStats, clientStats, serverSuccess, clientSuccess, serverEx, clientEx};
    ofstream outputFile(par.outFile);
    outputFile << bObs;
    outputFile.close();

    // RandGen rg{0, 100};

    // for(DataObject dob : rg)
    //     cout << dob.to_ZZ() << "\n";

    // for(auto it = rg.begin(); it != rg.end(); it++)
    //     cout << (*it).to_ZZ() << "\n";

    // auto beg = rg.begin();
    // for (size_t ii = 0; ii < 5; ii++)
    //     cout << (*beg++).to_ZZ() << "\n";

    // cout << "\nOther 12\n";
    // for (size_t ii = 0; ii < 12; ii++)
    //     cout << (*beg++).to_ZZ() << "\n";

    // cout << "First number divisible by 3 is: "
    //      << (*find_if(rg.begin(), rg.end(), [](DataObject d) {
    //                                             return to_int(d.to_ZZ()) && (d.to_ZZ() % 3 == 0);
    //                                         })).to_ZZ()
    //      << "\n";

    // ofstream of("output.txt");
    // auto first = toStr<ZZ>(ZZ(19));
    // auto second = toStr<ZZ>(ZZ(42));
    // of << base64_encode(first.data(), first.length()) << "\n";
    // of << base64_encode(second.data(), second.length()) << "\n";
    // of.close();

    // FromFileGen ffgen("output.txt");
    // cout << "From file is:\n";
    // for (auto dd : ffgen)
    //     cout << dd.to_ZZ() << "\n";
}

// Constructs the parameters from log file
CMDParams getParamsFromFile(const CMDParams& par, string fileNames);

CMDParams parse(int argc, char* argv[]) {
    CMDParams par;
    for (int aa = 1; aa < argc; aa++) {
        auto arg = (string) argv[aa];
        if (arg.find("-", 0) != string::npos) { // this is an argument name
            auto argName = arg.substr(1);
            auto value = (string) argv[aa + 1];
            if (argName == "in") {
                return getParamsFromFile(par, (string) argv[aa + 1]);
            } else if (argName == "proto") {
                auto value = (string) argv[aa + 1];
                if (value == "CPISync")
                    par.protocol = GenSync::SyncProtocol::CPISync;
                else if (value == "ProbCPISync")
                    par.protocol = GenSync::SyncProtocol::ProbCPISync;
                else if (value == "InterCPISync")
                    par.protocol = GenSync::SyncProtocol::InteractiveCPISync;
                else if (value == "CPISync_OneLessRound")
                    par.protocol = GenSync::SyncProtocol::CPISync_OneLessRound;
                else if (value == "CPISync_HalfRound")
                    par.protocol = GenSync::SyncProtocol::CPISync_HalfRound;
                else if (value == "OneWayCPISync")
                    par.protocol = GenSync::SyncProtocol::OneWayCPISync;
                else if (value == "OneWayCPISync")
                    par.protocol = GenSync::SyncProtocol::OneWayCPISync;
                else if (value == "FullSync")
                    par.protocol = GenSync::SyncProtocol::FullSync;
                else if (value == "IBLTSync")
                    par.protocol = GenSync::SyncProtocol::IBLTSync;
                else if (value == "OneWayIBLTSync")
                    par.protocol = GenSync::SyncProtocol::OneWayIBLTSync;
                else if (value == "IBLTSync_Multiset")
                    par.protocol = GenSync::SyncProtocol::IBLTSync_Multiset;
                else if (value == "CuckooSync")
                    par.protocol = GenSync::SyncProtocol::CuckooSync;
                else {
                    stringstream ss;
                    ss << "There is no " << value << " protocol";
                    throw ss.str();
                }
            } else if (argName == "sim") {
                stringstream ss((string) argv[aa + 1]);
                ss >> par.sim;
            } else if (argName == "smc") {
                stringstream ss(argv[aa + 1]);
                ss >> par.smc;
            } else if (argName == "cms") {
                stringstream ss(argv[aa + 1]);
                ss >> par.cms;
            } else if (argName == "multi") {
                if (value == "true")
                    par.multiset = true;
                else if (value == "false")
                    par.multiset = false;
                else
                    throw runtime_error("Wrong value of the -multi argument."
                                        " The only valid options are \"true\" and \"false\".");
            } else if (argName == "out") {
                par.outFile = (string) argv[aa + 1];
            } else {
                stringstream error;
                error << "Wrong argument " << argv[aa] << ". See Runner.cpp\n";
                throw runtime_error(error.str());
            }
        }
    }

    if (par.serverDataFile.empty())
        if (par.smc == CMDParams::uspec || par.cms == CMDParams::uspec || par.sim == CMDParams::uspec)
            throw "-smc -cms and -sim all have to be specified in this case";

    return par;
}

GeneratorPair buildGenerators(const CMDParams& cmd) {
    if (cmd.serverDataFile.empty()) {
        // if no input files, generate elements at random
        return {make_shared<RandGen>(), make_shared<RandGen>()};
    } else {
        // TODO:
        throw "Not implemented yet";
    }
}

CMDParams getParamsFromFile(const CMDParams& par, string fileNames) {
    // TODO: Ensure that if par.serverDataFile is set, the other two are set too
    throw "getParamsFromFile not implemented yet";
}

GenSyncPair buildGenSyncs(const BenchParams& par) {
    GenSync::Builder server = GenSync::Builder()
        .setComm(GenSync::SyncComm::socket)
        .setProtocol(par.syncProtocol);
    GenSync::Builder client = GenSync::Builder()
        .setComm(GenSync::SyncComm::socket)
        .setProtocol(par.syncProtocol);

    if (par.serverDataFile.empty()) {
        // if (par.syncProtocol == GenSync::SyncProtocol::CPISync) {
        //     auto pr = par.syncParams;
        // } else if (value == "ProbCPISync") {
        //     par.protocol = GenSync::SyncProtocol::ProbCPISync;
        // } else if (value == "InterCPISync") {
        //     par.protocol = GenSync::SyncProtocol::InteractiveCPISync;
        // } else if (value == "CPISync_OneLessRound") {
        //     par.protocol = GenSync::SyncProtocol::CPISync_OneLessRound;
        // } else if (value == "CPISync_HalfRound") {
        //     par.protocol = GenSync::SyncProtocol::CPISync_HalfRound;
        // } else if (value == "OneWayCPISync") {
        //     par.protocol = GenSync::SyncProtocol::OneWayCPISync;
        // } else if (value == "OneWayCPISync") {
        //     par.protocol = GenSync::SyncProtocol::OneWayCPISync;
        // } else if (value == "FullSync") {
        //     par.protocol = GenSync::SyncProtocol::FullSync;
        // } else if (value == "IBLTSync") {
        //     par.protocol = GenSync::SyncProtocol::IBLTSync;
        // } else if (value == "OneWayIBLTSync") {
        //     par.protocol = GenSync::SyncProtocol::OneWayIBLTSync;
        // } else if (value == "IBLTSync_Multiset") {
        //     par.protocol = GenSync::SyncProtocol::IBLTSync_Multiset;
        // } else if (value == "CuckooSync") {
        //     par.protocol = GenSync::SyncProtocol::CuckooSync;
        // }
    } else {
        // TODO: not yet implemented
        throw "Not implemented yet";
    }
}
