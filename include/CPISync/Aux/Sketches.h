/* This code is part of the CPISync project developed at Boston
 * University. Please see the README for use and references.
 *
 * @author Novak Boškov <boskov@bu.edu>
 *
 * Created on October, 2020.
 */

#ifndef SKETCHES_H
#define SKETCHES_H

#include <CPISync/Data/DataObject.h>
#include "hll.hpp"

/**
   This class encapsulates various data sketches computed over the
   sets being reconciled.
*/
class Sketches {
public:
    static const string PRINT_KEY; // string to use as a key when print sketches

    struct Values {
        int cardinality = 0;
        double uniqueElem = 0;
    };

    enum class Types {
        CARDINALITY,
        UNIQUE_ELEM
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
};

#endif  // Sketches
