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
    for (auto st : sketches)
        switch (st) {
        case Types::CARDINALITY:
            cardinality.value = std::unique_ptr<int>(new int(0));
            cardinality.initiated = true;
            break;
        case Types::UNIQUE_ELEM:
            uniqueElem.value = std::unique_ptr<datasketches::hll_sketch_alloc<>>(new datasketches::hll_sketch_alloc<>(14));
            uniqueElem.initiated = true;
            break;
        }
}

Sketches::Sketches() {
    cardinality.value = std::unique_ptr<int>(new int(0));
    cardinality.initiated = true;

    uniqueElem.value = std::unique_ptr<datasketches::hll_sketch_alloc<>>(new datasketches::hll_sketch_alloc<>(14));
    uniqueElem.initiated = true;
}

void Sketches::inc(std::shared_ptr<DataObject> elem) {
    if (cardinality.initiated)
        (*cardinality.value)++;

    if (uniqueElem.initiated)
        uniqueElem.value->update(to_uint(elem->to_ZZ()));
}

Sketches::Values Sketches::get() const {
    Values ret;

    if (cardinality.initiated)
        ret.cardinality = *(cardinality.value);

    if (uniqueElem.initiated)
        ret.uniqueElem = uniqueElem.value->get_estimate();

    return ret;
}
