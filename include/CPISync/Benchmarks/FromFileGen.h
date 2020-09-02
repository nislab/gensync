/* This code is part of the CPISync project developed at Boston
 * University. Please see the README for use and references.
 *
 * @author Novak Boškov <boskov@bu.edu>
 *
 * Created on July, 2020.
 */

#ifndef FROMFILEGEN_H
#define FROMFILEGEN_H

#include <iostream>
#include <CPISync/Benchmarks/DataObjectGenerator.h>
#include <CPISync/Aux/Auxiliary.h>

/**
 * Generates DataObjects from a file of base64 encoded ZZ objects.
 */
class FromFileGen : public DataObjectGenerator {
public:
    static const string DELIM_LINE; // parameter list delimiter in printouts

    explicit FromFileGen(const string& fName);

    /**
     * FromFileGen objects can be constructed from a BenchParam file.
     * BenchParam file contain two blocks separated by BenchParams::DELIM_LINE.
     * WhichOne signalizes from which block is this FromFileGen constructed.
     */
    enum WhichOne {FIRST, SECOND};

    FromFileGen(const string& fName, WhichOne firstOrSecond);

    /* Implementation of DataObjectGenerator virtual functions */

    DataObjectGenerator::iterator begin();
    DataObject produce();

private:
    string fName;
    ifstream file;              // input file stream wrapped into this iterator
};

#endif // FROMFILEGEN_H
