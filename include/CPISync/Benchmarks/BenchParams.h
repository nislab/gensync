/* This code is part of the CPISync project developed at Boston
 * University. Please see the README for use and references.
 *
 * @author Novak Boškov <boskov@bu.edu>
 *
 * Created on July, 2020.
 */

#ifndef BENCHPARAMS_H
#define BENCHPARAMS_H

#include <iostream>
#include <CPISync/Syncs/GenSync.h>
#include <CPISync/Benchmarks/DataObjectGenerator.h>

// See the corresponding sync classes for the parameters documentation
struct Params {
    friend ostream& operator<<(ostream&, const Params&);
};

struct CPISyncParams : Params{
    size_t m_bar = 0;
    size_t bits = 0;
    float epsilon = 0;
    size_t redundant = 0;
    bool hashes = false;

    friend ostream& operator<<(ostream& os, const CPISyncParams& x) {
        os << "m_bar: " << x.m_bar << "\n"
           << "bits: " << x.bits << "\n"
           << "epsilon: " << x.epsilon << "\n"
           << "redundant: " << x.redundant << "\n"
           << "hashes: " << std::boolalpha << x.hashes << "\n";

        return os;
    };
};

struct IBLTParams : Params {
    size_t expected = 0;
    size_t eltSize = 0;
    size_t numElemChild = 0;

    friend ostream& operator<<(ostream& os, const IBLTParams& x) {
        os << "expected: " << x.expected << "\n"
           << "eltSize: " << x.eltSize << "\n"
           << "numElemChild: " << x.numElemChild;

        return os;
    };
};

struct CuckooParams : Params {
    size_t fngprtSize = 0;
    size_t bucketSize = 0;
    size_t filterSize = 0;
    size_t maxKicks = 0;

    friend ostream& operator<<(ostream& os, const CuckooParams& x) {
        os << "fngprtSize: " << x.fngprtSize << "\n"
           << "bucketSize: " << x.bucketSize << "\n"
           << "filterSize: " << x.filterSize << "\n"
           << "maxKicks: " << x.maxKicks;

        return os;
    };
};

class BenchParams {
public:
    static const string PARAM_DELIM; // parameter list delimiter in printouts

    BenchParams() = default;
    ~BenchParams() = default;

    BenchParams(shared_ptr<DataObjectGenerator> serverElems, shared_ptr<DataObjectGenerator> clientElems,
                GenSync::SyncProtocol prot,
                size_t similar,
                size_t serverMinusClient,
                size_t clientMinusServer,
                bool multiset,
                string serverDataFile,
                string clientDataFile,
                string paramsFile);

    friend ostream& operator<<(ostream& os, const BenchParams& bp);

    GenSync::SyncProtocol syncProtocol; // protocol used to sync
    size_t similarCount;        // the number of similar elements between the two sets
    size_t serverMinusClientCount;
    size_t clientMinusServerCount;
    bool multiset;                          // whether the sets to be reconciled are multisets
    shared_ptr<Params> syncParams;          // parameters of the concrete sync protocol
    shared_ptr<DataObjectGenerator> serverElems;
    shared_ptr<DataObjectGenerator> clientElems;
    string serverDataFile;
    string clientDataFile;
    string paramsFile;
};

#endif // BENCHPARAMS_H
