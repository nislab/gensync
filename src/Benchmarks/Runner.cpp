/* This code is part of the CPISync project developed at Boston
 * University. Please see the README for use and references.
 *
 * @author Novak Boškov <boskov@bu.edu>
 *
 * Created on July, 2020.
 *
 * This runner tries its best to take advantage of modern processors
 * multithreading. When applicable, it will spawn approximately
 * thread::hardware_concurrency() threads and half that much
 * synchronizations (BATCH_SIZE). Each thread will hold a single
 * instance of server or client.
 *
 * Usage:
 * $ ./Benchmarks [-proto PROTOCOL] [-sim SIMILAR] [-smc SMC] [-cms CMS] [-multi MULTISET] [-out FILENAME]
 * Example:
 * $ ./Benchmarks -proto CPISync -sim 12 -smc 100 -cms 122 -multi false -out my_out_file.txt
 *
 * PROTOCOL := "CPISync" | "ProbCPISync" | ... (See parse function)
 * SIMILAR := int
 * SMC := int
 * CMS := int
 * MULTISET := "true" | "false"
 * FILENAME := string
 *
 * See the CommandLineParameters definition bellow.
 */

// TODO: Support dynamic set evolution while syncing
// TODO: Support marshaling and unmarshaling of BenchParams for reuse in different syncs
// TODO: Keep all the runs as separate from each other as possible

#include <iostream>
#include <thread>
#include <sstream>
#include <limits>
#include <CPISync/Syncs/GenSync.h>
#include <CPISync/Benchmarks/BenchParams.h>
#include <CPISync/Benchmarks/BenchObserv.h>
#include <CPISync/Benchmarks/RandGen.h>
#include <CPISync/Benchmarks/FromFileGen.h>

#include <CPISync/Aux/Auxiliary.h>

using namespace std;

const float targetMachineLoad = 0.9;
const size_t BATCH_SIZE = (thread::hardware_concurrency() * targetMachineLoad) / 2;

struct CMDParams {
    static const size_t uspec = numeric_limits<size_t>::max(); // parameter unspecified

    GenSync::SyncProtocol protocol = GenSync::SyncProtocol::END;
    size_t sim = uspec;             // count of similar elements between server and client
    size_t smc = uspec;             // count of elements only in server
    size_t cms = uspec;             // count of elements in client only
    bool multiset = false;          // whether the sets are multisets
    string outFile = "benchmark_out.txt";         // the name of output file
};

// the ugly command line arguments parsing
CMDParams parse(int argc, char* argv[]) {
    CMDParams cmd;
    for (int aa = 1; aa < argc; aa++) {
        auto arg = (string) argv[aa];
        if (arg.find("-", 0) != string::npos) { // this is an argument name
            auto argName = arg.substr(1);
            if (argName == "proto") {
                auto value = (string) argv[aa + 1];
                if (value == "CPISync")
                    cmd.protocol = GenSync::SyncProtocol::CPISync;
                else if (value == "ProbCPISync")
                    cmd.protocol = GenSync::SyncProtocol::ProbCPISync;
                else if (value == "InterCPISync")
                    cmd.protocol = GenSync::SyncProtocol::InteractiveCPISync;
                else if (value == "CPISync_OneLessRound")
                    cmd.protocol = GenSync::SyncProtocol::CPISync_OneLessRound;
                else if (value == "CPISync_HalfRound")
                    cmd.protocol = GenSync::SyncProtocol::CPISync_HalfRound;
                else if (value == "OneWayCPISync")
                    cmd.protocol = GenSync::SyncProtocol::OneWayCPISync;
                else if (value == "OneWayCPISync")
                    cmd.protocol = GenSync::SyncProtocol::OneWayCPISync;
                else if (value == "FullSync")
                    cmd.protocol = GenSync::SyncProtocol::FullSync;
                else if (value == "IBLTSync")
                    cmd.protocol = GenSync::SyncProtocol::IBLTSync;
                else if (value == "OneWayIBLTSync")
                    cmd.protocol = GenSync::SyncProtocol::OneWayIBLTSync;
                else if (value == "IBLTSync_Multiset")
                    cmd.protocol = GenSync::SyncProtocol::IBLTSync_Multiset;
                else if (value == "CuckooSync")
                    cmd.protocol = GenSync::SyncProtocol::CuckooSync;
            } else if (argName == "sim") {
                stringstream ss((string) argv[aa + 1]);
                ss >> cmd.sim;
            } else if (argName == "smc") {
                stringstream ss(argv[aa + 1]);
                ss >> cmd.smc;
            } else if (argName == "cms") {
                stringstream ss(argv[aa + 1]);
                ss >> cmd.cms;
            } else if (argName == "multi") {
                auto value = (string) argv[aa + 1];
                if (value == "true")
                    cmd.multiset = true;
                else if (value == "false")
                    cmd.multiset = false;
                else
                    throw runtime_error("Wrong value of the -multi argument."
                                        " The only valid options are \"true\" and \"false\".");
            } else if (argName == "out") {
                cmd.outFile = (string) argv[aa + 1];
            } else {
                stringstream error;
                error << "Wrong argument " << argv[aa] << ". See Runner.cpp\n";
                throw runtime_error(error.str());
            }
        }
    }

    return cmd;
}

int main(int argc, char* argv[]) {
    CMDParams cmd = parse(argc, argv);

    RandGen rg(0, 100);

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

    ofstream of("output.txt");
    auto first = toStr<ZZ>(ZZ(19));
    auto second = toStr<ZZ>(ZZ(42));
    of << base64_encode(first.data(), first.length()) << "\n";
    of << base64_encode(second.data(), second.length()) << "\n";
    of.close();

    FromFileGen ffgen("output.txt");
    cout << "From file is:\n";
    for (auto dd : ffgen)
        cout << dd.to_ZZ() << "\n";
}
