/* This code is part of the CPISync project developed at Boston
 * University. Please see the README for use and references.
 *
 * @author Novak Boškov <boskov@bu.edu>
 *
 * Created on July, 2020.
 *
 * Usage:
 * $ ./Benchmarks PATH_TO_PARAMS_FILE IGNORE_DATA_SETS_FROM_FILE
 *
 * Anything can be passed as IGNORE_DATA_SETS_FROM_FILE.
 * If you want to use data sets from file, just don't pass anyting as a second command line argument.
 */

// TODO: Support dynamic set evolution while syncing

#include <CPISync/Benchmarks/BenchParams.h>
#include <CPISync/Syncs/GenSync.h>
#include <assert.h>
#include <random>

using namespace std;
using GenSyncPair = pair<shared_ptr<GenSync>, shared_ptr<GenSync>>;

// hardware source of randomness
static std::random_device rd;
static std::mt19937 gen(rd());

// maximum cardinality of a set.
// The cardinality in the multiset sense depends on the repetitions of the
// elements, which is also random.
static const size_t MAX_CARD = 1 << 8;
// one item can repeat only up to (CARD_MAX / REP_RATIO) + 1 times
static const size_t REP_RATIO = 1 << 6;
// maximum value of an element
static const size_t MAX_DOBJ = 2 << 20;
// default set cardinality
static const size_t DEFAULT_CARD = 1;
// default Zipfian distribution alpha parameter
static const double ZF_ALPHA = 0.5;

// distribution used in Zipfian distribution
static uniform_real_distribution<double> dist(0.0, 1.0);
// synthetic set cardinalities
static uniform_int_distribution<int> card(128, MAX_CARD);
// synthetic common elements count
static uniform_int_distribution<int> comm(MAX_CARD * 0.03, MAX_CARD * 0.5);
// DataObject value distribution
static uniform_int_distribution<int> elem(0, MAX_DOBJ);

/**
 * Builds GenSync objects for syncing.
 * @param par The benchmark parameters objects that holds the details
 * @param zipfElems If true, the elements are not taken from benchmark parameters but are generated randomly from a Zipfian distribution.
 * @return The pair of generated GenSync objects. The first is the server, the second is the client.
 */
GenSyncPair buildGenSyncs(const BenchParams &par, bool zipfElems = false);
/**
 * Generates zipf distributed random numbers from 0 to n.
 * @param n The highest number
 * @param alpha The alpha parameter of Zipfian distribution
 * @return A number sampled from the distribution
 */
int zipf(int n, double alpha = ZF_ALPHA);
/**
 * Makes a new DataObject sampled from a uniform distribution.
 * @return The constructed object
 */
inline shared_ptr<DataObject> makeObj();
/**
 * Adds elements to the passed GenSync object. Handles the repetitions.
 * @param gs The GenSync object
 * @param obj The element to add
 */
inline void addAnElemWithReps(shared_ptr<GenSync> gs,
                              shared_ptr<DataObject> obj);

int main(int argc, char *argv[]) {
  BenchParams bPar = BenchParams{argv[1]};
  GenSyncPair genSyncs = buildGenSyncs(bPar, argc >= 3);

  Logger::gLog(Logger::TEST, "Sets are ready, reconciliation starts...");

  pid_t pid = fork();
  if (pid == 0) { // child process
    try {
      genSyncs.first->serverSyncBegin(0);
    } catch (exception &e) {
      cout << "Sync Exception [server]: " << e.what() << "\n";
    }
  } else if (pid > 0) { // parent process
    try {
      genSyncs.second->clientSyncBegin(0);
    } catch (exception &e) {
      cout << "Sync Exception [client]: " << e.what() << "\n";
    }
  } else if (pid < 0) {
    throw runtime_error("Fork has failed");
  }
}

GenSyncPair buildGenSyncs(const BenchParams &par, bool zipfElems) {
  GenSync::Builder serverBuilder = GenSync::Builder()
                                       .setComm(GenSync::SyncComm::socket)
                                       .setProtocol(par.syncProtocol);
  GenSync::Builder clientBuilder = GenSync::Builder()
                                       .setComm(GenSync::SyncComm::socket)
                                       .setProtocol(par.syncProtocol);

  par.syncParams->apply(serverBuilder);
  par.syncParams->apply(clientBuilder);
  auto server = make_shared<GenSync>(serverBuilder.build());
  auto client = make_shared<GenSync>(clientBuilder.build());

  if (zipfElems) {
    size_t cardA = DEFAULT_CARD;
    size_t cardB = DEFAULT_CARD;
    size_t common = comm(gen);

    do {
      cardA = card(gen);
    } while (cardA < common + 1); // at least one local element

    do {
      cardB = card(gen);
    } while (cardB < common + 1);

    stringstream ss;
    ss << "Benchmarks generated sets:  Client: "
       << cardA << ", Server: " << cardB << ", Common: " << common;
    Logger::gLog(Logger::TEST, ss.str());

    Logger::gLog(Logger::TEST, "Adding common elements...");
    for (size_t ii = 0; ii < common; ii++) {
      auto obj = makeObj();
      addAnElemWithReps(server, obj);
      addAnElemWithReps(client, obj);
    }

    Logger::gLog(Logger::TEST, "Adding server local elements...");
    for (size_t ii = 0; ii < (cardA - common); ii++)
      addAnElemWithReps(server, makeObj());

    Logger::gLog(Logger::TEST, "Adding client local elements...");
    for (size_t ii = 0; ii < (cardB - common); ii++)
      addAnElemWithReps(client, makeObj());

  } else {
    for (auto elem : *par.serverElems)
      server->addElem(make_shared<DataObject>(elem));

    for (auto elem : *par.clientElems)
      client->addElem(make_shared<DataObject>(elem));
  }

  return {server, client};
}

/**
 * Original implementation from Ken Christensen
 * https://www.csee.usf.edu/~kchriste/tools/genzipf.c
 *
 * TODO: This should be implemented as a part of RandGen
 */
int zipf(int n, double alpha) {
  // The map of c constants for Zipfian distributions.
  // The idea is that all the calls with the same n sample from the exact same
  // distribution. Thus for each n, c need be calculated only once.
  static map<int, double> c_map;

  double c = 0;          // Normalization constant
  double z = 0;          // Uniform random number (0 < z < 1)
  double sum_prob = 0;   // Sum of probabilities
  double zipf_value = 0; // Computed exponential value to be returned
  int i = 0;             // Loop counter

  if (c_map.find(n) == c_map.end()) {
    for (i = 1; i <= n; i++)
      c = c + (1.0 / pow((double)i, alpha));

    c = 1.0 / c;
    c_map.insert(std::pair<int, double>(n, c));
  } else {
    c = c_map.at(n);
  }

  // Pull a uniform random number (0 < z < 1)
  do {
    z = dist(gen);
  } while (z == 0 || z == 1);

  // Map z to the value
  sum_prob = 0;
  for (i = 1; i <= n; i++) {
    sum_prob = sum_prob + c / pow((double)i, alpha);
    if (sum_prob >= z) {
      zipf_value = i;
      break;
    }
  }

  // Assert that zipf_value is between 1 and N
  assert((zipf_value >= 1) && (zipf_value <= n));

  return zipf_value;
}

shared_ptr<DataObject> makeObj() {
  return make_shared<DataObject>(DataObject(ZZ(elem(rd))));
}

void addAnElemWithReps(shared_ptr<GenSync> gs, shared_ptr<DataObject> obj) {
  for (size_t rep = 0; rep < zipf(MAX_CARD / REP_RATIO) + 1; rep++)
    gs->addElem(obj);
}
