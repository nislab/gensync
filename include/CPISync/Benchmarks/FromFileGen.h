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
    FromFileGen(string fName) : fName (fName), file(fName) {};

    inline ZZ decodeLine(const string& line) {
        string dLine = base64_decode(line);
        return to_ZZ(dLine.data());
    }

    /* Implementation of DataObjectGenerator virtual functions */

    DataObjectGenerator::iterator begin() {
        string line;
        getline(file, line);
        if (file.eof())
            return {this, DataObject()};
        return {this, DataObject(decodeLine(line))};
    }

    DataObject produce() {
        string line;
        getline(file, line);
        if (file.eof())
            return DataObject();
        return DataObject(decodeLine(line));
    }

private:
    string fName;
    ifstream file;              // input file stream wrapped into this iterator
};

#endif // FROMFILEGEN_H
