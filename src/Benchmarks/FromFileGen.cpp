#include <CPISync/Benchmarks/FromFileGen.h>

const string FromFileGen::DELIM_LINE = string(80, '-');

FromFileGen::FromFileGen(const string& fName) : fName (fName), file(fName) {}

FromFileGen::FromFileGen(const string& fName, WhichOne firstOrSecond) : fName (fName), file(fName) {
    string line;
    size_t delimsCount = 0;
    while (getline(file, line))  // fast forward the ifstream
        if (line.find(DELIM_LINE) != string::npos)
            if (firstOrSecond == WhichOne::FIRST
                || (firstOrSecond == WhichOne::SECOND && delimsCount++ == 2))
                break;
    // TODO: for very large files fast forwarding can take very long.
};

inline ZZ decodeLine(const string& line) {
    string dLine = base64_decode(line);
    return to_ZZ(dLine.data());
}

DataObjectGenerator::iterator FromFileGen::begin() {
    string line;
    getline(file, line);
    if (file.eof())
        return {this, DataObject()};
    return {this, DataObject(decodeLine(line))};
}

DataObject FromFileGen::produce() {
    string line;
    getline(file, line);
    if (file.eof()
        || (line.find(DELIM_LINE) != string::npos))
        return DataObject();
    return DataObject(decodeLine(line));
}
