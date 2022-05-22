/* This code is part of the CPISync project developed at Boston
 * University. Please see the README for use and references.
 *
 * @author Novak Boškov <boskov@bu.edu>
 *
 * Created on May, 2022.
 */

#ifndef AUXMEASUREMENTS_H
#define AUXMEASUREMENTS_H

#include <iostream>

using namespace std;

class AuxMeasurements {
  public:
    static const string announcerString;

    AuxMeasurements() = default;
    ~AuxMeasurements();

  AuxMeasurements(double lat, double uBand, double dBand, double mDur, time_t mesStart)
        : latBeforeSync(lat), uBBeforeSync(uBand), dBBeforeSync(dBand),
          mesDur(mDur), mesStartAt(mesStart) {}

    friend ostream &operator<<(ostream &, const AuxMeasurements &);

  private:
    double latBeforeSync; /** Estimated network latency before the
                              clientSyncBegin or serverSyncBegin call */
    double uBBeforeSync;  /** Estimated upload bandwidth before the
                              clientSyncBegin or serverSyncBegin call */
    double dBBeforeSync;  /** Estimated download bandwidth before the
                              clientSyncBegin or  serverSyncBegin call */
    double mesDur;        /** Duration of the measurements themselves */
    time_t mesStartAt;    /** Time since epoch when network measurements
                              started */
};

#endif // AUXMEASUREMENTS_H
