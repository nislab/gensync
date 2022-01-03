#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Extracts data from the experiments directory into pandas.DataFrame's.
One DataFrame per experiment run.
Experiment runs are assumed to be executed in separate directories.

Usage: ./extract_data.py PATH_TO_EXPERIMENTS_DIR

Author: Novak Boškov <boskov@bu.edu>
Date: Feb, 2021.
"""

import sys
import os
import pandas as pd

OBSERV_SUFFIX = '_observ.cpisync'
# The order MUST to match the order of lines the measurements appear in
# _observ.cpisync files. Except for 'server', 'client', and 'cardinality'.
COLUMNS = ['server', 'client', 'cardinality', 'success',
           'bytes transmitted', 'bytes received',
           'communication time(s)', 'idle time(s)', 'computation time(s)']

if __name__ == '__main__':
    experiments_root_dir = os.path.normpath(sys.argv[1])

    # a run is a sync execution on a pair of server-client files
    experiment_runs = []
    for node in os.listdir(experiments_root_dir):
        if os.path.isdir(os.path.join(experiments_root_dir, node)) \
           and node.startswith('.cpisync_'):
            experiment_runs.append(node)

    experiment_df = pd.DataFrame(columns=COLUMNS)
    for run in experiment_runs:
        run_dir = os.path.join(experiments_root_dir, run)
        run_dir_content = os.listdir(run_dir)
        run_dir_content.sort()
        for observ in run_dir_content:
            if not observ.endswith(OBSERV_SUFFIX):
                continue

            f_name_parts_count = len(run.split('_'))
            if (f_name_parts_count == 3):
                server, client = run.split('_')[-2:]
                cardinality = ''
            elif (f_name_parts_count == 4):
                server, client, cardinality = run.split('_')[-3:]
            else:
                sys.exit('File name path has {} parts (only 3 and 4 allowed)'
                         .format(f_name_parts_count))

            measurements = []

            with open(os.path.join(run_dir, observ), 'r') as obs_f:
                while True:
                    line = obs_f.readline()
                    if not line:
                        break

                    # one line can match only one measure
                    for measure in COLUMNS[3:]:
                        if line.lower().startswith(measure):
                            # success is treated differently
                            if measure == 'success':
                                if 'true' in line or '1' in line:
                                    measurements.append(True)
                                else:
                                    measurements.append(False)
                                break

                            val = float(line.split(':')[-1].strip())
                            measurements.append(val)
                            break

            # add a row
            prefix = [server, client, cardinality]
            experiment_df.loc[
                len(experiment_df.index)] = prefix + measurements

    # write the data frame of this experiments_root_dir
    experiment_df.to_csv('{}.csv'.format(
        os.path.basename(experiments_root_dir)))
