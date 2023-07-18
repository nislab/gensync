/* This code is part of the CPISync project developed at Boston
 * University. Please see the README for use and references.
 *
 * @author Novak Boškov <boskov@bu.edu>
 *
 * Created on July, 2020.
 */

#include <cstdio>
#include <regex>
#include <GenSync/Benchmarks/BenchParams.h>
#include <GenSync/Syncs/ProbCPISync.h>
#include <GenSync/Syncs/InterCPISync.h>
#include <GenSync/Syncs/FullSync.h>
#include <GenSync/Syncs/IBLTSync.h>
#include <GenSync/Syncs/IBLTSync_HalfRound.h>
#include <GenSync/Syncs/IBLTSync_Multiset.h>
#include <GenSync/Syncs/CPISync_HalfRound.h>
#include <GenSync/Syncs/IBLTSetOfSets.h>
#include <GenSync/Syncs/CuckooSync.h>

const char BenchParams::KEYVAL_SEP = ':';
const string BenchParams::FILEPATH_SEP = "/"; // TODO: we currently don't compile for _WIN32!

template<char delimiter>
class DelimitedString : public string {};

/**
 * Extracts data from the next line in input stream. The line is of the form "text: data"
 */
template<typename T>
void getVal(istream& is, T& container) {
    string line;
    getline(is, line);

    stringstream ss(line);
    string segment;
    vector<string> segments;
    while (getline(ss, segment, BenchParams::KEYVAL_SEP))
        segments.push_back(segment);

    istringstream(segments.at(1)) >> std::boolalpha >> container;
}

/**
 * Extract SyncProtocol from the next line in the input stream
 */
inline GenSync::SyncProtocol getProtocol(istream& is) {
    size_t protoInt;
    getVal<decltype(protoInt)>(is, protoInt);
    return static_cast<GenSync::SyncProtocol>(protoInt);
}

ostream& CPISyncParams::serialize(ostream& os) const {
    os << "m_bar: " << m_bar << "\n"
       << "bits: " << bits << "\n"
       << "epsilon: " << epsilon << "\n"
       << "partitions/pFactor(for InterCPISync): " << partitions << "\n"
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

/**
 * Makes the correct Params specialization using the data in the input stream.
 */
inline shared_ptr<Params> decideBenchParams(GenSync::SyncProtocol syncProtocol, ifstream& is) {
    if (syncProtocol == GenSync::SyncProtocol::CPISync
        || syncProtocol == GenSync::SyncProtocol::CPISync_OneLessRound
        || syncProtocol == GenSync::SyncProtocol::CPISync_HalfRound
        || syncProtocol == GenSync::SyncProtocol::ProbCPISync
        || syncProtocol == GenSync::SyncProtocol::InteractiveCPISync
        || syncProtocol == GenSync::SyncProtocol::OneWayCPISync) {
        auto par = make_shared<CPISyncParams>();
        is >> *par;
        return par;
    } else if (syncProtocol == GenSync::SyncProtocol::IBLTSync
               || syncProtocol == GenSync::SyncProtocol::OneWayIBLTSync
               || syncProtocol == GenSync::SyncProtocol::IBLTSetOfSets
               || syncProtocol == GenSync::SyncProtocol::IBLTSync_Multiset) {
        auto par = make_shared<IBLTParams>();
        is >> *par;
        return par;
    } else if (syncProtocol == GenSync::SyncProtocol::CuckooSync) {
        auto par = make_shared<CuckooParams>();
        is >> *par;
        return par;
    } else if (syncProtocol == GenSync::SyncProtocol::FullSync) {
        return nullptr;
    } else {
        stringstream ss;
        ss << "There is no viable sync protocol with ID " << static_cast<size_t>(syncProtocol);
        throw runtime_error(ss.str());
    }
}

/**
 * Figures out whether the file contains real data or a reference to a
 * data file. If there is reference, it returns the path to the
 * referenced file. Otherwise it returns an empty string.
 *
 * Referenced file path is considered relative to the file we are
 * currently reading.
 */
inline string getReference(istream& is, const string& origFile) {
    string line;
    getline(is, line);
    if (line.compare(FromFileGen::DELIM_LINE) != 0) {
        string msg = "File is malformed.\n";
        Logger::error_and_quit(msg);
    }

    getline(is, line);
    auto pos = line.find(FromFileGen::REFERENCE);
    if (pos != string::npos) {
        stringstream ss(line);
        string segment;
        vector<string> segments;
        while (getline(ss, segment, BenchParams::KEYVAL_SEP))
            segments.push_back(segment);

        string refFName = segments.at(1);

        // trim leading and trailing spaces
        refFName = std::regex_replace(refFName, std::regex("^ +| +$"), "");

        // build the relative path to the referenced file
        auto origFNameLen = origFile.substr(origFile.find_last_of(BenchParams::FILEPATH_SEP)).length();
        string pref = origFile.substr(0, origFile.length() - origFNameLen);
        stringstream prefs;
        prefs << pref << BenchParams::FILEPATH_SEP << refFName;
        return prefs.str();
    }

    return "";
}

BenchParams::BenchParams(const string& fName) {
    ifstream is(fName);
    if (!is.is_open()) {
        stringstream ss;
        ss << "File " << fName << " not found";
        throw runtime_error(ss.str());
    }

    syncProtocol = getProtocol(is);
    syncParams = decideBenchParams(syncProtocol, is);

    // Expect Sketches here in file. When there, just skip them.
    string line;
    getline(is, line);
    if (line.find(Sketches::PRINT_KEY) == string::npos) {
        stringstream ss;
        ss << "\"" << Sketches::PRINT_KEY << "\" expected here\n";
        Logger::error_and_quit(ss.str());
    }

    string refFile = getReference(is, fName);
    string fToUse = refFile.empty() ? fName : refFile;

    AElems = make_shared<FromFileGen>(fToUse, FromFileGen::FIRST);
    BElems = make_shared<FromFileGen>(fToUse, FromFileGen::SECOND);
}

/**
 * This constructor should be the only place where we dynamically
 * determine what concrete SyncMethod is in use.
 */
BenchParams::BenchParams(SyncMethod& meth) :
    AElems (nullptr),
    BElems (nullptr),
    sketches (meth.getSketches()) {
    auto cpi = dynamic_cast<CPISync*>(&meth);
    if (cpi) {
        syncProtocol = GenSync::SyncProtocol::CPISync;
        syncParams = make_shared<CPISyncParams>(cpi->getMaxDiff(), cpi->getBits(),
                                                cpi->getProbEps(), cpi->getHashes(),
                                                cpi->getRedundant());
        return;
    }

    auto probCpi = dynamic_cast<ProbCPISync*>(&meth);
    if (probCpi) {
        syncProtocol = GenSync::SyncProtocol::ProbCPISync;
        syncParams = make_shared<CPISyncParams>(probCpi->getMaxDiff(), probCpi->getBits(),
                                                probCpi->getProbEps(), probCpi->getHashes());
        return;
    }

    auto interCpi = dynamic_cast<InterCPISync*>(&meth);
    if (interCpi) {
        syncProtocol = GenSync::SyncProtocol::InteractiveCPISync;
        syncParams = make_shared<CPISyncParams>(interCpi->getMaxDiff(), interCpi->getBitNum(),
                                                interCpi->getProbEps(), interCpi->getHashes(),
                                                interCpi->getPFactor());
        return;
    }

    auto oneWay = dynamic_cast<CPISync_HalfRound*>(&meth);
    if (oneWay) {
        syncProtocol = GenSync::SyncProtocol::OneWayCPISync;
        syncParams = make_shared<CPISyncParams>(oneWay->getMaxDiff(), oneWay->getBits(),
                                                oneWay->getProbEps(), oneWay->getHashes());
        return;
    }

    auto full = dynamic_cast<FullSync*>(&meth);
    if (full) {
        syncProtocol = GenSync::SyncProtocol::FullSync;
        syncParams = make_shared<FullSyncParams>();
        return;
    }

    auto iblt = dynamic_cast<IBLTSync*>(&meth);
    if (iblt) {
        syncProtocol = GenSync::SyncProtocol::IBLTSync;
        syncParams = make_shared<IBLTParams>(iblt->getExpNumElems(), iblt->getElementSize());
        return;
    }

    auto ibltHR = dynamic_cast<IBLTSync_HalfRound*>(&meth);
    if (ibltHR) {
        syncProtocol = GenSync::SyncProtocol::OneWayIBLTSync;
        syncParams = make_shared<IBLTParams>(ibltHR->getExpNumElems(), ibltHR->getElementSize());
        return;
    }

    auto ibltSoS = dynamic_cast<IBLTSetOfSets*>(&meth);
    if (ibltSoS) {
        syncProtocol = GenSync::SyncProtocol::IBLTSetOfSets;
        syncParams = make_shared<IBLTParams>(ibltSoS->getExpNumElems(), ibltSoS->getElemSize(), ibltSoS->getChildSize());
        return;
    }

    auto cuc = dynamic_cast<CuckooSync*>(&meth);
    if (cuc) {
        syncProtocol = GenSync::SyncProtocol::CuckooSync;
        syncParams = make_shared<CuckooParams>(cuc->getFngprtSize(), cuc->getBucketSize(),
                                               cuc->getFilterSize(), cuc->getMaxKicks());
        return;
    }

    auto ibltMulti = dynamic_cast<IBLTSync_Multiset*>(&meth);
    if (ibltMulti) {
        syncProtocol = GenSync::SyncProtocol::IBLTSync_Multiset;
        syncParams = make_shared<IBLTParams>(ibltMulti->getExpNumElems(), ibltMulti->getElementSize());
        return;
    }

    throw runtime_error("The SyncMethod is not known to BenchParams");
}

BenchParams::~BenchParams() {}

ostream& operator<<(ostream& os, const BenchParams& bp) {
    os << "Sync protocol (as in GenSync.h): " << (int) bp.syncProtocol << "\n"
       << *bp.syncParams << "\n"
       << **bp.sketches << "\n"
       << FromFileGen::DELIM_LINE << "\n";

    return os;
}
