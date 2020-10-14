/* This code is part of the CPISync project developed at Boston
 * University. Please see the README for use and references.
 *
 * @author Novak Boškov <boskov@bu.edu>
 *
 * Created on October, 2020.
 */

#include <CPISync/Aux/Sketches.h>

const string Sketches::PRINT_KEY = "Sketches";

Sketches::Sketches(std::initializer_list<Types> sketches) {
    for (auto st : sketches) {
        if (st ==  Types::CARDINALITY) {
            cardinality.value = std::unique_ptr<int>(new int(0));
            cardinality.initiated = true;
        }
    }
}

Sketches::Sketches() {
    cardinality.value = std::unique_ptr<int>(new int(0));
    cardinality.initiated = true;
}

void Sketches::inc(std::shared_ptr<DataObject>) {
    if (cardinality.initiated)
        (*cardinality.value)++;
}

Sketches::Values Sketches::get() {
    Values ret;

    if (cardinality.initiated)
        ret.cardinality = *(cardinality.value);

    return ret;
}
