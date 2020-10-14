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

/**
   This class encapsulates various data sketches computed over the
   sets being reconciled.
*/
class Sketches {
public:
    static const string PRINT_KEY; // string to use as a key when print sketches

    struct Values {
        int cardinality = 0;
    };

    enum class Types {
        CARDINALITY
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
    Values get();

    class sketches_exception : public std::exception {
        const char* what() const throw() {
            return "Something went wrong with the set sketches.";
        }

    };

    friend ostream& operator<<(ostream& os, const Sketches& sk) {
        os << PRINT_KEY << ": {cardinality: " << *(sk.cardinality.value) << "}";

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
};

#endif  // Sketches
