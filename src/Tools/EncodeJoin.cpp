/* This code is part of the CPISync project developed at Boston
 * University. Please see the README for use and references.
 *
 * @author Novak Boškov <boskov@bu.edu>
 *
 * Created on January, 2021.
 */

#include <GenSync/Aux/Auxiliary.h>
#include <GenSync/Benchmarks/FromFileGen.h>
#include <fstream>
#include <iostream>

static const size_t SHRINK = 64; // bits to shrink transaction hashes to
static_assert(SHRINK >= 8, "SHRINK must be >= 8.");

using namespace std;

/**
 * Converts ASCII character to integer.
 * @param theChar the character to convert
 * @returns the corresponding ASCII integer
 */
int char2int(char theChar);

/**
 * Converts a hexadecimal string to a binary buffer.
 * @param source the source hexadecimal string
 * @param destinationBuffer the buffer where the binary is written
 */
void hex2bin(const char *source, char *destinationBuffer);

/**
 * Shrinks the hexadecimal string of arbitrary size to the least significant
 * SHRINK bits.
 * @param fullString the original string
 * @returns the shrunk string
 */
string shrink(string fullString);

/**
 * Encodes a line from the original file into the format for the target file.
 * @param line the line from the original file
 * @returns the line in the format of the target file
 */
string encodeLine(string line);

/**
 * Receives paths to two files that contain constant length hexadecimal strings
 * in each line. Reads hexadecimal strings from file, shrinks them, and base64
 * encode them. Then joins the results from both input files into the third
 * file. It uses the separator from Benchamrks/FromFileGen.
 */
int main(int argc, char *argv[]) {
    if (argc != 4)
        throw invalid_argument("Requires exactly 3 command line arguments.");
    if (SHRINK < 8)
        throw invalid_argument("SHRINK must be >= 8.");

    ofstream outFile(argv[3]);
    if (!outFile.is_open()) {
        stringstream ss;
        ss << "Cannot open " << argv[3];
        throw invalid_argument(ss.str());
    }

    string line;
    ifstream firstFile(argv[1]);
    if (firstFile.is_open()) {
        while (getline(firstFile, line))
            outFile << encodeLine(line) << "\n";

        firstFile.close();
    } else {
        stringstream ss;
        ss << "Unable to open " << argv[1];
        throw invalid_argument(ss.str());
    }

    outFile << FromFileGen::DELIM_LINE << "\n";

    ifstream secondFile(argv[2]);
    if (secondFile.is_open()) {
        string line;
        while (getline(secondFile, line))
            outFile << encodeLine(line) << "\n";

        secondFile.close();
    } else {
        stringstream ss;
        ss << "Unable to open " << argv[2];
        throw invalid_argument(ss.str());
    }

    outFile.close();
}

int char2int(char theChar) {
    if (theChar >= '0' && theChar <= '9')
        return theChar - '0';
    if (theChar >= 'A' && theChar <= 'F')
        return theChar - 'A' + 10;
    if (theChar >= 'a' && theChar <= 'f')
        return theChar - 'a' + 10;

    stringstream ss;
    ss << theChar << " is not a hexadecimal digit.";
    throw invalid_argument(ss.str());
}

void hex2bin(const char *source, char *destinationBuffer) {
    if (strlen(source) % 2 != 0) {
        stringstream ss;
        ss << "Input string to hex2bin must be of an even length. "
           << "Passed " << source;
        throw invalid_argument(ss.str());
    }

    while (source && source[1]) {
        *(destinationBuffer++) = char2int(source[0]) * 16 + char2int(source[1]);
        source += 2;
    }
}

string shrink(string fullString) {
    // two characters one byte
    size_t startPos = fullString.length() - (SHRINK / 4);
    return fullString.substr(startPos, fullString.length());
}

string encodeLine(string line) {
    size_t buffer_len = SHRINK / 8 + (SHRINK % 8 != 0);
    char buffer[buffer_len];
    hex2bin(shrink(line).c_str(), buffer);
    return base64_encode(buffer, buffer_len);
}
