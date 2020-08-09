/* This code is part of the CPISync project developed at Boston
 * University. Please see the README for use and references.
 *
 * @author Novak Boškov <boskov@bu.edu>
 *
 * Created on July, 2020.
 */

#include <CPISync/Benchmarks/BenchParams.h>
#include <cstdio>

const string BenchParams::DELIM_LINE = string(80, '-');
const string BenchParams::SERVER_ELEM_FILE = "server.cpisynctmp";
const string BenchParams::CLIENT_ELEM_FILE = "client.cpisynctmp";

template<char delimiter>
class DelimitedString : public string {};

// Extracts data from the next line in input stream. The line is of the form "text: data"
template<typename T>
void getVal(istream& is, T& container) {
    string line;
    getline(is, line);
    istringstream lineS(line);
    vector<string> delimited ((istream_iterator<DelimitedString<':'>>(lineS)),
                              istream_iterator<DelimitedString<':'>>());
    istringstream(delimited.at(1)) >> container;
}

ostream& CPISyncParams::serialize(ostream& os) const {
    os << "m_bar: " << m_bar << "\n"
       << "bits: " << bits << "\n"
       << "epsilon: " << epsilon << "\n"
       << "partitions: " << partitions << "\n"
       << "hashes: " << std::boolalpha << hashes;

    return os;
}

istream& CPISyncParams::unserialize(istream& is) {
    getVal<decltype(m_bar)>(is, m_bar);
    getVal<decltype(bits)>(is, bits);
    getVal<decltype(epsilon)>(is, epsilon);
    getVal<decltype(partitions)>(is, partitions);
    getVal<decltype(hashes)>(is, hashes);

    return is;
}

void CPISyncParams::apply(GenSync::Builder& gsb) const {
    gsb.setBits(bits);
    gsb.setMbar(m_bar);
    gsb.setErr(epsilon);
    gsb.setNumPartitions(partitions);
    gsb.setHashes(hashes);
}

ostream& IBLTParams::serialize(ostream& os) const {
    os << "expected: " << expected << "\n"
       << "eltSize: " << eltSize << "\n"
       << "numElemChild: " << numElemChild;

    return os;
}

istream& IBLTParams::unserialize(istream& is) {
    getVal<decltype(expected)>(is, expected);
    getVal<decltype(eltSize)>(is, eltSize);
    getVal<decltype(numElemChild)>(is, numElemChild);

    return is;
}

void IBLTParams::apply(GenSync::Builder& gsb) const {
    gsb.setExpNumElems(expected);
    gsb.setBits(eltSize);
    gsb.setExpNumElemChild(numElemChild);
}

ostream& CuckooParams::serialize(ostream& os) const {
    os << "fngprtSize: " << fngprtSize << "\n"
       << "bucketSize: " << bucketSize << "\n"
       << "filterSize: " << filterSize << "\n"
       << "maxKicks: " << maxKicks;

    return os;
}

istream& CuckooParams::unserialize(istream& is) {
    getVal<decltype(fngprtSize)>(is, fngprtSize);
    getVal<decltype(bucketSize)>(is, bucketSize);
    getVal<decltype(filterSize)>(is, filterSize);
    getVal<decltype(maxKicks)>(is, maxKicks);

    return is;
}

void CuckooParams::apply(GenSync::Builder& gsb) const {
    gsb.setFngprtSize(fngprtSize);
    gsb.setBucketSize(bucketSize);
    gsb.setFilterSize(filterSize);
    gsb.setMaxKicks(maxKicks);
}

BenchParams::BenchParams(istream& is) :
    loadedFromFile (true),
    similarCount (0),
    serverMinusClientCount (0),
    clientMinusServerCount (0)
{
    string line;
    getline(is, line);
    stringstream lineS(line);
    size_t protInt;
    lineS >> protInt;
    syncProtocol = static_cast<GenSync::SyncProtocol>(protInt);

    if (syncProtocol == GenSync::SyncProtocol::IBLTSync_Multiset)
        multiset = true;
    else
        multiset = false;

    if (syncProtocol == GenSync::SyncProtocol::CPISync
        || syncProtocol == GenSync::SyncProtocol::CPISync_OneLessRound
        || syncProtocol == GenSync::SyncProtocol::CPISync_HalfRound
        || syncProtocol == GenSync::SyncProtocol::ProbCPISync
        || syncProtocol == GenSync::SyncProtocol::InteractiveCPISync
        || syncProtocol == GenSync::SyncProtocol::OneWayCPISync) {
        auto par = make_shared<CPISyncParams>();
        is >> *par;
        syncParams = par;
    } else if (syncProtocol == GenSync::SyncProtocol::IBLTSync
               || syncProtocol == GenSync::SyncProtocol::OneWayIBLTSync
               || syncProtocol == GenSync::SyncProtocol::IBLTSetOfSets
               || syncProtocol == GenSync::SyncProtocol::IBLTSync_Multiset) {
        auto par = make_shared<IBLTParams>();
        is >> *par;
        syncParams = par;
    } else if (syncProtocol == GenSync::SyncProtocol::CuckooSync) {
        auto par = make_shared<CuckooParams>();
        is >> *par;
        syncParams = par;
    } else if (syncProtocol == GenSync::SyncProtocol::FullSync) {
        syncParams = nullptr;
        multiset = false;
    } else {
        stringstream ss;
        ss << "There is no viable sync protocol with ID " << static_cast<size_t>(syncProtocol);
        throw runtime_error(ss.str());
    }

    // Construct DataObject generators with corresponding temporary files
    getline(is, line);
    if (line.find(DELIM_LINE) != string::npos) { // after this line the sets data starts
        ofstream serverF(SERVER_ELEM_FILE);      // temporary files
        ofstream clientF(CLIENT_ELEM_FILE);
        size_t nextDelim = 0;
        while (getline(is, line)) {
            if (line.find(DELIM_LINE) != string::npos)
                if (++nextDelim > 1)
                    throw runtime_error("BenchParams: more than two sets in the file");

            if (nextDelim)
                clientF << line << "\n";
            else
                serverF << line << "\n";
        }
    } else {
        throw runtime_error("BenchParams: malformed content in the input stream");
    }

    serverElems = make_shared<FromFileGen>(SERVER_ELEM_FILE);
    clientElems = make_shared<FromFileGen>(CLIENT_ELEM_FILE);
}

BenchParams::BenchParams(GenSync::SyncProtocol prot,
                         shared_ptr<Params> syncParams,
                         size_t similar,
                         size_t serverMinusClient,
                         size_t clientMinusServer,
                         bool multiset) :
    syncProtocol(prot),
    syncParams (syncParams),
    loadedFromFile (false),
    similarCount (similar),
    serverMinusClientCount (serverMinusClient),
    clientMinusServerCount (clientMinusServer),
    multiset (multiset)
{
    // TODO: Reconstruct this from parameters
    serverElems = NULL;
    clientElems = NULL;

    throw runtime_error("Not implemented");
}

BenchParams::~BenchParams() {
    if (remove(SERVER_ELEM_FILE.c_str()) != 0 || remove(CLIENT_ELEM_FILE.c_str()) != 0) {
        stringstream ss("BenchParams: temporary files deletion has failed.\n");
        ss << "Please manually delete " << SERVER_ELEM_FILE << " and " << CLIENT_ELEM_FILE << "\n";
        cerr << ss.str();
    }
}

ostream& operator<<(ostream& os, const BenchParams& bp) {
    os << "Sync protocol (as in GenSync.h): " << (int) bp.syncProtocol << "\n"
       << "# similar elements: " << bp.similarCount << "\n"
       << "# server only elements: " << bp.serverMinusClientCount << "\n"
       << "# client only elements: " << bp.clientMinusServerCount << "\n"
       << "multiset: " << std::boolalpha << bp.multiset << "\n"
       << BenchParams::DELIM_LINE << "\n"
       << "Sync params:\n" << bp.syncParams << "\n"
       << BenchParams::DELIM_LINE;

    return os;
}
