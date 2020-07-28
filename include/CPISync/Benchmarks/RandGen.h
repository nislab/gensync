/* This code is part of the CPISync project developed at Boston
 * University. Please see the README for use and references.
 *
 * @author Novak Boškov <boskov@bu.edu>
 *
 * Created on July, 2020.
 */

#ifndef RANDGEN_H
#define RANDGEN_H

#include <limits>
#include <NTL/ZZ.h>
#include <CPISync/Benchmarks/DataObjectGenerator.h>

/**
 * Generates DataObjects from NTL's random generator.
 */
class RandGen : public DataObjectGenerator {
public:
    RandGen() : max(numeric_limits<int>::max()) {}
    RandGen(size_t seed, size_t max = numeric_limits<int>::max()) : max(max) {
        // For seeding to work properly no one should reseed NTL's
        // PRNG during the life time of the object. NTL's PRNG is a shared resource.
        NTL::SetSeed(conv<ZZ>(seed));
    }

    /* Implementation of DataObjectGenerator virtual functions */

    DataObjectGenerator::iterator begin() {return {this, DataObject(NTL::RandomBnd(max))};}
    DataObject produce() {return DataObject(NTL::RandomBnd(max));}

private:
    size_t max;                 // RandGen chooses from [0, max)
};

#endif // RANDGEN_H
