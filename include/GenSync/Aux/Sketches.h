/* This code is part of the GenSync project developed at Boston
 * University. Please see the README for use and references.
 *
 * @author Novak Boškov <boskov@bu.edu>
 *
 * Created on October, 2020.
 */

#ifndef SKETCHES_H
#define SKETCHES_H

#include <GenSync/Data/DataObject.h>
#include "hll.hpp"
#include "frequent_items_sketch.hpp"

/**
   This class encapsulates various data sketches computed over the
   sets being reconciled.
*/
class Sketches {
public:
    static const string PRINT_KEY; // string to use as a key when print sketches
    static const uint HLL_LOG_K;   // log base 2 of the number of buckets in HyperLogLog sketch
    static const uint FI_LOG_MAX_SIZE;        // number of first heavy hitters considered in the heavy hitters sketch

    struct Values {
        uint cardinality = 0;
        double uniqueElem = 0;
        uint heavyHitters = 0;
    };

    enum class Types {
        CARDINALITY,
        UNIQUE_ELEM,
        HEAVY_HITTERS,
    };

    /**
     * Constructs the object with desired sketches initialized.
     * @sketches The sketches (as SketchesTypes) that need to be
     * initialized.
     */
    explicit Sketches(std::initializer_list<Types> sketches);

    /**
     * Constructs the object with all the sketches initialized.
     */
    Sketches();

    /**
     * Increments all the initiated sketches with the single new element.
     * @throws sketches_exception if something goes wrong in the process
     */
    void inc(std::shared_ptr<DataObject>);

    /**
     * @returns The numerical values of all the sketches.
     * @throws sketches_exception if something goes wrong in the process
     */
    Values get () const;

    class sketches_exception : public std::exception {
        const char* what() const throw() {
            return "Something went wrong with the set sketches.";
        }

    };

    friend ostream& operator<<(ostream& os, const Sketches& sk) {
        Values vals = sk.get();

        os << PRINT_KEY << ": {";

        if (sk.cardinality.initiated)
            os << "cardinality: " << vals.cardinality;
        if (sk.uniqueElem.initiated)
            os << ", unique(HyperLogLog): " << vals.uniqueElem;
        if (sk.heavyHitters.initiated)
            os << ", heavyHitters: " << vals.heavyHitters;

        os << "}";

        return os;
    };
private:
    /**
     * Wrapper for sketches.
     * All the sketches that are to be used need to be initialized in the constructor.
     */
    template<typename T>
    struct Sketch {
        unique_ptr<T> value;
        bool initiated = false;
    };

    /**
     * The cardinality of the set.
     */
    Sketch<int> cardinality;

    /**
     * The HyperLogLog unique elements estimator.
     */
    Sketch<datasketches::hll_sketch_alloc<>> uniqueElem;

    /**
     * Statistics calculated over heavy hitters.
     * Heavy hitters make sense only when we synchronize multisets.
     * We count the number of heavy hitters with their occurrence count lower
     * bound above the maximum error.
     * (see https://datasketches.apache.org/docs/Frequency/FrequentItemsOverview.html)
     */
    Sketch<datasketches::frequent_items_sketch<unsigned long>> heavyHitters;
};

#endif  // Sketches
