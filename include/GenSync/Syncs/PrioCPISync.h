/* This code is part of the GenSync project developed at Boston University.  Please see the README for use and references. */

/*
 * Implements a Priority GenSync algorithm
 *
 *
 * Author: Paul
 * Created on June 25, 2013, 04:31 PM
 */

#ifndef PRIO_CPI_H
#define	PRIO_CPI_H

#include <string>
#include <Gensync/Data/DataObject.h>
#include <Gensync/Syncs/InterCPISync.h>
using namespace std;

class PriorityCPISync : public InterCPISync {

public:

  PriorityCPISync(long m_bar, long bits, int epsilon, int partition): InterCPISync(m_bar, bits +1, epsilon,partition)
	{
		SyncID = SYNC_TYPE::Priority_CPISync;
		useExisting = true;
	}

  ~PriorityCPISync() {}
  string getName() { return "Priority GenSync\n";}

protected:
        map< DataObject , shared_ptr<DataObject> > priority_map;
};

#endif
