#!/usr/bin/env python3

"""
Runs multiple "bunches" of the specified sync method and records the training data.
Input and output data is serialized into `pandas.DataFrame`.

Sync method, its initial parameters and the initial data are given in `params_data.cpisync` file.
A "bunch" consists of BUNCH iterations that run the sync method with exact same parameters.
Each iteration of the bunch produces one data point in our training set.

Once when the bunch is done, a new set of input parameters is selected from
the set of meaningful parameters of the given sync method. The new parameters
are used in the next bunch.

Usage: ./data_collection.py [A_PARAMS_DATA_CPISYNC_FILE]

Author: Novak Boskov <boskov@bu.edu>
Date: Sep, 2020.

This code is part of the CPISync project developed at Boston
University.  Please see the README for use and references.
"""

# Number of iterations for the exact same sync parameters
BUNCH = 10

import os
import numpy as np
import pandas as pd
from sklearn import linear_model

# 1. fix the sync to basic CPISync
# 2. fix the objective; e.g., communication cost (Bytes Transmitted + Bytes Received)
# 3. make 5 CPISync parameters settings that make sense
# 4. train 100 times per each parameters settings (get 5x100 _observ files)
#    - a script that extracts _observ from the cpisync dir and runs the UnitTest again
# 5. train 5 linear regressions (each with 100 data points)
# 6. give some new cardinalities to each model and see what do they predict
#    - this is to simulate that two GenSyncs exchange set cardinalities before syncing
# 7. the lowest predicted communication cost (of all 5) is selected as the best parameters settings!

# CPISync params:
# - bits: 64, mBar: 512, err: 8
# - bits: 32, mBar: 512, err: 6
# - bits: 64, mBar: 1024, err: 8
# - bits: 32, mBar: 1024, err: 3
# - bits: 128, mBar: 4096, err: 9

def make_df(data_dir: str):
    """Parse a directory of data. Store it in a data frame. Save it as a
    CSV file."""
    dtypes = np.dtype([('cardinality', int), ('bytes transmitted', int), ('bytes received', int)])
    data = np.empty(0, dtype=dtypes)
    df = pd.DataFrame(data)

    for (dirname, _, filenames) in os.walk(data_dir):
        for f_name in filenames:
            with open('{}/{}'.format(dirname, f_name), 'r') as f:
                lines = f.readlines()
                card = [int(l.split(':')[2][:-2]) for l in lines if 'cardinality:' in l][0]
                b_trans = [int(l.split(':')[1]) for l in lines if 'Bytes Transmitted:' in l][0]
                b_recv = [int(l.split(':')[1]) for l in lines if 'Bytes Received:' in l][0]
                df = df.append({'cardinality': card, 'bytes transmitted': b_trans, 'bytes received': b_recv},
                               ignore_index=True)

    df.to_csv('{}.csv'.format(data_dir.split('/')[-1]))
    return df


def main():
    # load the data
    data_64_512_8 = make_df('/home/novak/Desktop/CODE/cpisync/scripts/OBSERVS_collection_64_512_8')
    data_32_512_6 = make_df('/home/novak/Desktop/CODE/cpisync/scripts/OBSERVS_collection_32_512_6')
    data_64_1024_8 = make_df('/home/novak/Desktop/CODE/cpisync/scripts/OBSERVS_collection_64_1024_8')

    # initiate the 3 models
    lr_64_512_8 = linear_model.LinearRegression()
    lr_32_512_6 = linear_model.LinearRegression()
    lr_64_1024_8 = linear_model.LinearRegression()

    # train all 3 models
    lr_64_512_8.fit(data_64_512_8['cardinality'].to_numpy().reshape(-1, 1),
                    (data_64_512_8['bytes transmitted'] + data_64_512_8['bytes received']).to_numpy())
    lr_32_512_6.fit(data_32_512_6['cardinality'].to_numpy().reshape(-1, 1),
                    (data_32_512_6['bytes transmitted'] + data_32_512_6['bytes received']).to_numpy())
    lr_64_1024_8.fit(data_64_1024_8['cardinality'].to_numpy().reshape(-1, 1),
                    (data_64_1024_8['bytes transmitted'] + data_64_1024_8['bytes received']).to_numpy())

    # cardinality we want to find the best parameters for
    pred_for = np.array([234]).reshape(-1, 1)

    # predict for all models
    pred_64_512_8 = lr_64_512_8.predict(pred_for)[0]
    pred_32_512_6 = lr_32_512_6.predict(pred_for)[0]
    pred_64_1024_8 = lr_64_1024_8.predict(pred_for)[0]

    print('bits: 64, mBar: 512, err: 8 ---> {}\n'
          'bits: 32, mBar: 512, err: 6 ---> {}\n'
          'bits: 64, mBar: 1024, err: 8 ---> {}'
          .format(pred_64_512_8, pred_32_512_6, pred_64_1024_8))
