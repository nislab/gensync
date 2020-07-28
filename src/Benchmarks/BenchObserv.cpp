/* This code is part of the CPISync project developed at Boston
 * University. Please see the README for use and references.
 *
 * @author Novak Boškov <boskov@bu.edu>
 *
 * Created on July, 2020.
 */

#include <CPISync/Benchmarks/BenchObserv.h>

BenchObserv::BenchObserv(BenchParams params) : params (params) {}

ostream& operator<<(ostream& os, const BenchObserv& bo) {return os;}
