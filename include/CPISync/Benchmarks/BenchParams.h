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
#include <CPISync/Benchmarks/FromFileGen.h>

// See the corresponding sync classes for the parameters documentation
struct Params {
    virtual ostream& serialize(ostream& os) const = 0;
    virtual istream& unserialize(istream& is) = 0;

    friend ostream& operator<<(ostream& os, const Params& x) {return x.serialize(os);}
    friend istream& operator>>(istream& is, Params& x) {return x.unserialize(is);}

    /**
     * Sets up the GenSync::Builder object using this parameters
     * @param gsb The builder object to be used
     */
    virtual void apply(GenSync::Builder& gsb) const = 0;
};

struct CPISyncParams : Params {
    size_t m_bar;
    size_t bits;
    float epsilon;
    size_t partitions;
    bool hashes;

    CPISyncParams() : m_bar (0), bits (0), epsilon (0), partitions (0), hashes (false) {}

    ostream& serialize(ostream& os) const;
    istream& unserialize(istream& is);

    void apply(GenSync::Builder& gsb) const;
};

struct IBLTParams : Params {
    size_t expected, eltSize, numElemChild;

    IBLTParams() : expected (0), eltSize (0), numElemChild (0) {}

    ostream& serialize(ostream& os) const;
    istream& unserialize(istream& is);

    void apply(GenSync::Builder& gsb) const;
};

struct CuckooParams : Params {
    size_t fngprtSize, bucketSize, filterSize, maxKicks;

    CuckooParams() : fngprtSize (0), bucketSize (0), filterSize (0), maxKicks (0) {}

    ostream& serialize(ostream& os) const;
    istream& unserialize(istream& is);

    void apply(GenSync::Builder& gsb) const;
};

class BenchParams {
public:
    static const string DELIM_LINE; // parameter list delimiter in printouts
    // the paths to the files where the server and the client elements are temporarily stored
    static const string SERVER_ELEM_FILE, CLIENT_ELEM_FILE;

    BenchParams() = default;
    ~BenchParams();

    /**
     * Reconstructs the sync parameters from the input stream. The
     * format of the file is as follows:
     *
     * SYNC_METHOD_IDENTIFIER (see GenSync::SyncProtocol)
     * Params
     * BenchParams::DELIM_LINE
     * BASE64_ENCODED_ELEMENTS_OF_A_SET(as DataObjects)
     * BenchParams::DELIM_LINE
     * BASE64_ENCODED_ELEMENTS_OF_A_SET(as DataObjects)
     *
     * @param is The input stream from which to construct this object. Normally a file.
     */
    BenchParams(istream& is);

    /**
     * @param prot The sync protocol
     * @param syncParams The sync parameters
     * @param similar The number of elements common among the server and client
     * @param serverMinusClient The number of elements local to server
     * @param clientMinusServer The number of elements local to client
     * @param multiset Set to true when syncing multisets
     */
    BenchParams(GenSync::SyncProtocol prot,
                shared_ptr<Params> syncParams,
                size_t similar = 0,
                size_t serverMinusClient = 128,
                size_t clientMinusServer = 128,
                bool multiset = false);

    friend ostream& operator<<(ostream& os, const BenchParams& bp);

    GenSync::SyncProtocol syncProtocol;
    shared_ptr<Params> syncParams;
    shared_ptr<DataObjectGenerator> serverElems;
    shared_ptr<DataObjectGenerator> clientElems;

    bool loadedFromFile;        // whether this object is deserialized from a file
    size_t similarCount;
    size_t serverMinusClientCount;
    size_t clientMinusServerCount;
    bool multiset;
};

#endif // BENCHPARAMS_H
