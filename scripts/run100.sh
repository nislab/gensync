#!/bin/bash
# This script runs a UnitTest 100 times and records the
# _observ.cpisync files in the OBSERVS dir.
#
# The first command line parameter is the path to the dir where
# UnitTest binary resides.

# Where the UnitTest binary is
WHERE=$1
# Name of the dir where the data is stored
RECORD="cpisync"
# Dir where to put the _observ files
OBSERVS="OBSERVS_collection"

rm -r $OBSERVS 2> /dev/null
mkdir $OBSERVS

OLD_DIR=$(pwd)
cd $WHERE

for i in {0..99}
do
    ./UnitTest
    mv $(find $RECORD -type f -iname "*_observ.cpisync" | sort | head -n 1) $OLD_DIR/$OBSERVS
    rm -r $RECORD
    echo "---> Iteration $i done."
done
