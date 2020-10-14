#!/usr/bin/env python3
"""
1. fix the sync to basic CPISync
2. fix the objective; e.g., communication cost (Bytes Transmitted + Bytes Received)
3. make 4 CPISync parameters settings that make sense
4. train 100 times per each parameters settings (get 4x100 _observ files)
   - a script that extracts _observ from the cpisync dir and runs the UnitTest again (run100.sh)
5. train 4 linear regressions (each with 100 data points)
6. give some new cardinalities to each model and see what do they predict
   - this is to simulate that two GenSyncs exchange set cardinalities before syncing
7. the lowest predicted communication cost (of all 4) is selected as the best parameters settings.

CPISync params:
- bits: 64, mBar: 512, err: 8
- bits: 32, mBar: 512, err: 6
- bits: 64, mBar: 1024, err: 8
- bits: 32, mBar: 1024, err: 3

Usage: ./data_collection.py

Author: Novak Boskov <boskov@bu.edu>
Date: Sep, 2020.

This code is part of the CPISync project developed at Boston
University.  Please see the README for use and references.
"""

import os
import numpy as np
import pandas as pd
from sklearn import linear_model

def make_df(data_dir: str):
    """Parse a directory of data. Store it in a data frame. Save it as a
    CSV file."""
    csv_name = os.path.basename(data_dir)
    csv_f = '{}.csv'.format(csv_name)
    if (os.path.exists(csv_f)):   # if we already have the csv, don't make it again
        return pd.read_csv(csv_f)

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

    df.to_csv(csv_f)
    return df


def main():
    """Runs the basic data collection, fits the model, and predicts."""
    # load the data
    data_64_512_8 = make_df('OBSERVS_collection_64_512_8')
    data_32_512_6 = make_df('OBSERVS_collection_32_512_6')
    data_64_1024_8 = make_df('OBSERVS_collection_64_1024_8')
    data_32_1024_3 = make_df('OBSERVS_collection_32_1024_3')

    # initiate the 3 models
    lr_64_512_8 = linear_model.LinearRegression()
    lr_32_512_6 = linear_model.LinearRegression()
    lr_64_1024_8 = linear_model.LinearRegression()
    lr_32_1024_3 = linear_model.LinearRegression()

    # train all 3 models
    lr_64_512_8.fit(data_64_512_8['cardinality'].to_numpy().reshape(-1, 1),
                    (data_64_512_8['bytes transmitted'] + data_64_512_8['bytes received']).to_numpy())
    lr_32_512_6.fit(data_32_512_6['cardinality'].to_numpy().reshape(-1, 1),
                    (data_32_512_6['bytes transmitted'] + data_32_512_6['bytes received']).to_numpy())
    lr_64_1024_8.fit(data_64_1024_8['cardinality'].to_numpy().reshape(-1, 1),
                     (data_64_1024_8['bytes transmitted'] + data_64_1024_8['bytes received']).to_numpy())
    lr_32_1024_3.fit(data_32_1024_3['cardinality'].to_numpy().reshape(-1, 1),
                     (data_32_1024_3['bytes transmitted'] + data_32_1024_3['bytes received']).to_numpy())

    # cardinality we want to find the best parameters for
    pred_for = np.array([234]).reshape(-1, 1)

    # predict for all models
    pred_64_512_8 = lr_64_512_8.predict(pred_for)[0]
    pred_32_512_6 = lr_32_512_6.predict(pred_for)[0]
    pred_64_1024_8 = lr_64_1024_8.predict(pred_for)[0]
    pred_32_1024_3 = lr_32_1024_3.predict(pred_for)[0]

    print('Pick the smallest:\n'
          'bits: 64, mBar: 512, err: 8 ---> {}\n'
          'bits: 32, mBar: 512, err: 6 ---> {}\n'
          'bits: 64, mBar: 1024, err: 8 ---> {}\n'
          'bits: 32, mBar: 1024, err: 3 ---> {}\n'
          .format(pred_64_512_8, pred_32_512_6, pred_64_1024_8, pred_32_1024_3))


if __name__ == "__main__":
    main()
