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

using namespace std;
using GenSyncPair = pair<shared_ptr<GenSync>, shared_ptr<GenSync>>;

struct CMDParams {
    static const size_t uspec = numeric_limits<size_t>::max(); // parameter unspecified

    GenSync::SyncProtocol protocol = GenSync::SyncProtocol::CPISync;
    size_t sim = uspec;             // count of similar elements between server and client
    size_t smc = uspec;             // count of elements only in server
    size_t cms = uspec;             // count of elements in client only
    bool multiset = false;          // whether the sets are multisets
    string outFile = "benchmark_out.txt";         // the name of output file
    string inFile = "";             // file that stores the parameters and data for syncronization.
                                    // If not set, data is generated at random
};

// The ugly command line arguments parsing
CMDParams parse(int argc, char* argv[]);
// Builds GenSync objects for sync
GenSyncPair buildGenSyncs(const BenchParams& par);

int main(int argc, char* argv[]) {
    CMDParams par = parse(argc, argv);

    BenchParams bPar;
    if (!par.inFile.empty())
        bPar = BenchParams{par.inFile};
    else
        throw runtime_error("Not implemented");

    GenSyncPair genSyncs = buildGenSyncs(bPar);

    // initialize indicators
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
}

CMDParams parse(int argc, char* argv[]) {
    CMDParams par;
    for (int aa = 1; aa < argc; aa++) {
        auto arg = (string) argv[aa];
        if (arg.find("-", 0) != string::npos) { // this is an argument name
            auto argName = arg.substr(1);
            auto value = (string) argv[aa + 1];
            if (argName == "in") {
                par.inFile = (string) argv[aa + 1];
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

    if (par.inFile.empty())
        if (par.smc == CMDParams::uspec || par.cms == CMDParams::uspec || par.sim == CMDParams::uspec)
            throw "-smc -cms and -sim all have to be specified in this case";

    return par;
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
