/* This code is part of the CPISync project developed at Boston
 * University. Please see the README for use and references.
 *
 * @author Novak Boškov <boskov@bu.edu>
 *
 * Created on July, 2020.
 */

#ifndef DATAOBJECTGENERATOR_H
#define DATAOBJECTGENERATOR_H

#include <CPISync/Data/DataObject.h>

/**
 * All the generators that produce DataObjects for synchronization should inherit this class.
 */
class DataObjectGenerator {
public:
    virtual ~DataObjectGenerator() {}

    class iterator {
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = DataObject;
        using pointer = DataObject*;
        using reference = DataObject&;
        using iterator_category = input_iterator_tag;

        iterator(DataObjectGenerator* generator_, DataObject do_) :
            generator (generator_),
            currentDataObject (do_) {}

        iterator& operator=(const iterator& other) = default;
        bool operator==(const iterator& other) const {return currentDataObject == other.currentDataObject;}
        bool operator!=(const iterator& other) const {return !(currentDataObject == other.currentDataObject);}
        value_type operator*() {return currentDataObject;}
        iterator& operator++() {
            currentDataObject = generator->produce();
            return *this;
        }
        iterator operator++(int) {
            auto old = currentDataObject;
            currentDataObject = generator->produce();
            return {generator, old};
        }

    private:
        DataObjectGenerator* generator; // the delegate to a concrete class that implements produce()
        value_type currentDataObject;   // current state of the iterator
    };

    // Produces a DataObject. Specificity of each subclass.
    virtual DataObject produce() = 0;
    virtual iterator begin() = 0;
    // DataObjectGenerators normally signalize they finished when they return an empty DataObject
    virtual iterator end() {return {this, DataObject()};}
};

#endif // DATAOBJECTGENERATOR_H
