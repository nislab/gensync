/* This code is part of the GenSync project developed at Boston University.  Please see the README for use and references. */
#include <NTL/RR.h>

// project libraries
#include <GenSync/Aux/SyncMethod.h>
#include <GenSync/Syncs/CPISync.h>
#include <GenSync/Syncs/ProbCPISync.h>

ProbCPISync::ProbCPISync(long m_bar, long bits, int epsilon,bool hashes) :
CPISync(m_bar, bits, epsilon + (int) ceil(log(bits) / log(2)), 0, hashes) // adding lg(b) gives an equivalent probability of error for GenSync
{

  // tweak parameters of GenSync for probabilistic implementation
   probEps = epsilon; // this was the designed probability of error
   probCPI = true; // switches some code in the GenSync superclass
   currDiff = 1;

}
