/* This code is part of the GenSync project developed at Boston University.  Please see the README for use and references. */

#include <iostream>
#include <Gensync/Aux/ConstantsAndTypes.h>
#include <Gensync/Aux/Auxiliary.h>

void Logger::error(const string& msg) {
    perror(msg.c_str());
}

void Logger::error_and_quit(const string& msg) {
    perror(msg.c_str());
    exit(GENERAL_ERROR);
}
