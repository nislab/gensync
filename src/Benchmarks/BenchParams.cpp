/* This code is part of the CPISync project developed at Boston
 * University. Please see the README for use and references.
 *
 * @author Novak Boškov <boskov@bu.edu>
 *
 * Created on July, 2020.
 */

#include <CPISync/Benchmarks/BenchParams.h>

const string BenchParams::PARAM_DELIM = string(80, '-');

template<char delimiter>
class DelimitedString : public string {};

// Extracts data from line of form "text: data"
template<typename T>
void getVal(string line, T& container) {
    istringstream lineS(line);
    vector<string> delimited ((istream_iterator<DelimitedString<':'>>(lineS)),
                              istream_iterator<DelimitedString<':'>>());
    istringstream(delimited.at(1)) >> container;
}

ostream& IBLTParams::serialize(ostream &os) const {
    os << "expected: " << expected << "\n"
       << "eltSize: " << eltSize << "\n"
       << "numElemChild: " << numElemChild;

    return os;
}

istream& IBLTParams::unserialize(istream &is) {
    string line;
    getline(is, line);
    getVal<decltype(expected)>(line, expected);
    getline(is, line);
    getVal<decltype(eltSize)>(line, eltSize);
    getline(is, line);
    getVal<decltype(numElemChild)>(line, numElemChild);

    return is;
}

ostream& CPISyncParams::serialize(ostream &os) const {
    os << "m_bar: " << m_bar << "\n"
       << "bits: " << bits << "\n"
       << "epsilon: " << epsilon << "\n"
       << "redundant: " << redundant << "\n"
       << "hashes: " << std::boolalpha << hashes;

    return os;
}

istream& CPISyncParams::unserialize(istream &is) {
    string line;
    getline(is, line);
    getVal<decltype(m_bar)>(line, m_bar);
    getline(is, line);
    getVal<decltype(bits)>(line, bits);
    getline(is, line);
    getVal<decltype(epsilon)>(line, epsilon);
    getline(is, line);
    getVal<decltype(redundant)>(line, redundant);
    getline(is, line);
    getVal<decltype(hashes)>(line, hashes);

    return is;
}

ostream& CuckooParams::serialize(ostream& os) const {
    os << "fngprtSize: " << fngprtSize << "\n"
       << "bucketSize: " << bucketSize << "\n"
       << "filterSize: " << filterSize << "\n"
       << "maxKicks: " << maxKicks;

    return os;
}

istream& CuckooParams::unserialize(istream& is) {
    string line;
    getline(is, line);
    getVal<decltype(fngprtSize)>(line, fngprtSize);
    getline(is,line);
    getVal<decltype(bucketSize)>(line, bucketSize);
    getline(is,line);
    getVal<decltype(filterSize)>(line, filterSize);
    getline(is,line);
    getVal<decltype(maxKicks)>(line, maxKicks);

    return is;
}

BenchParams::BenchParams(shared_ptr<DataObjectGenerator> serverElems, shared_ptr<DataObjectGenerator> clientElems,
                         GenSync::SyncProtocol prot, size_t similar, size_t serverMinusClient,
                         size_t clientMinusServer, bool multiset,
                         string serverDataFile, string clientDataFile, string paramsFile) :
    syncProtocol(prot),
    similarCount(similar),
    serverMinusClientCount(serverMinusClient),
    clientMinusServerCount(clientMinusServer),
    multiset(multiset),
    serverElems (serverElems),
    clientElems (clientElems),
    serverDataFile (serverDataFile),
    clientDataFile (clientDataFile),
    paramsFile (paramsFile) {}

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
