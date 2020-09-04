/* This code is part of the CPISync project developed at Boston
 * University. Please see the README for use and references.
 *
 * @author Novak Boškov <boskov@bu.edu>
 *
 * Created on July, 2020.
 */

#include <cstdio>
#include <CPISync/Benchmarks/BenchParams.h>
#include <CPISync/Syncs/ProbCPISync.h>
#include <CPISync/Syncs/InterCPISync.h>
#include <CPISync/Syncs/FullSync.h>
#include <CPISync/Syncs/IBLTSync.h>
#include <CPISync/Syncs/IBLTSync_HalfRound.h>
#include <CPISync/Syncs/IBLTSync_Multiset.h>
#include <CPISync/Syncs/CPISync_HalfRound.h>
#include <CPISync/Syncs/IBLTSetOfSets.h>
#include <CPISync/Syncs/CuckooSync.h>

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

// Extract SyncProtocol from the next line in the input stream
inline GenSync::SyncProtocol getProtocol(istream& is) {
    size_t protoInt;
    getVal<decltype(protoInt)>(is, protoInt);
    return static_cast<GenSync::SyncProtocol>(protoInt);
}

ostream& CPISyncParams::serialize(ostream& os) const {
    os << "m_bar: " << m_bar << "\n"
       << "bits: " << bits << "\n"
       << "epsilon: " << epsilon << "\n"
       << "partitions: " << partitions << "\n"
       << "hashes: " << std::boolalpha << hashes << "\n"
       << "pFactor: " << pFactor;

    return os;
}

istream& CPISyncParams::unserialize(istream& is) {
    getVal<decltype(m_bar)>(is, m_bar);
    getVal<decltype(bits)>(is, bits);
    getVal<decltype(epsilon)>(is, epsilon);
    getVal<decltype(partitions)>(is, partitions);
    getVal<decltype(hashes)>(is, hashes);
    getVal<decltype(pFactor)>(is, pFactor);

    return is;
}

void CPISyncParams::apply(GenSync::Builder& gsb) const {
    gsb.setBits(bits);
    gsb.setMbar(m_bar);
    gsb.setErr(epsilon);
    gsb.setNumPartitions(partitions);
    gsb.setHashes(hashes);

    if (pFactor)                // pFactor = 0 is treated as not set
        gsb.setNumPartitions(pFactor);
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

BenchParams::BenchParams(const string& fName) {
    ifstream is(fName);
    syncProtocol = getProtocol(is);
    syncParams = decideBenchParams(syncProtocol, is);
    serverElems = make_shared<FromFileGen>(fName, FromFileGen::FIRST);
    clientElems = make_shared<FromFileGen>(fName, FromFileGen::SECOND);
}

/**
 * This function should be the only place where we dynamically
 * determine what concrete SyncMethod is in use.
 */
BenchParams::BenchParams(SyncMethod& meth) : serverElems (nullptr), clientElems (nullptr) {
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
                                                0, interCpi->getPFactor());
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
       << "Sync params:\n" << *bp.syncParams << "\n"
       << FromFileGen::DELIM_LINE << "\n";

    return os;
}
