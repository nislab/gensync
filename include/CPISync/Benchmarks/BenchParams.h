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
    virtual ostream& serialize(ostream& os) const = 0;
    virtual istream& unserialize(istream& is) = 0;

    friend ostream& operator<<(ostream& os, const Params& x) {return x.serialize(os);}
    friend istream& operator>>(istream& is, Params& x) {return x.unserialize(is);}
};

struct CPISyncParams : Params{
    size_t m_bar = 0;
    size_t bits = 0;
    float epsilon = 0;
    size_t redundant = 0;
    bool hashes = false;

    ostream& serialize(ostream& os) const;
    istream& unserialize(istream& is);
};

struct IBLTParams : Params {
    size_t expected = 0, eltSize = 0, numElemChild = 0;

    ostream& serialize(ostream& os) const;
    istream& unserialize(istream& is);
};

struct CuckooParams : Params {
    size_t fngprtSize = 0, bucketSize = 0, filterSize = 0, maxKicks = 0;

    ostream& serialize(ostream& os) const;
    istream& unserialize(istream& is);
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
