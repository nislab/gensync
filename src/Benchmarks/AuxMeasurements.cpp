/* This code is part of the CPISync project developed at Boston
 * University. Please see the README for use and references.
 *
 * @author Novak Boškov <boskov@bu.edu>
 *
 * Created on May, 2022.
 */

#include <CPISync/Benchmarks/AuxMeasurements.h>

const string AuxMeasurements::announcerString = "Auxiliary:\n";

ostream &operator<<(ostream &os, const AuxMeasurements &aux) {
    os << AuxMeasurements::announcerString
       << "Latency: " << aux.latBeforeSync << endl
       << "uBandwidth: " << aux.uBBeforeSync << endl
       << "dBandwidth: " << aux.dBBeforeSync << endl
       << "MeasuremntsDuration: " << aux.mesDur << endl;

    return os;
}

AuxMeasurements::~AuxMeasurements() {}
