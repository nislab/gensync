#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Extracts data from the experiments directory into pandas.DataFrame's.
One DataFrame per experiment run.
Experiment runs are assumed to be executed in separate directories.

Usage: ./extract_data.py PATH_TO_EXPERIMENTS_DIR [t]

If t is passed, then assume Colosseum specific `run` naming.

Author: Novak Boškov <boskov@bu.edu>
Date: Feb, 2021.
"""

import sys
import os
import datetime
from enum import Enum
import pandas as pd

OBSERV_SUFFIX = '_observ.cpisync'
COLOSSEUM_SPECIFIC_DOT_CPI_DIR = '.cpisync__'
COLOSSEUM_DEFAULT_CARDINALITY = 10000
# The order MUST match the order of lines the measurements appear in
# _observ.cpisync files. Except for 'server', 'client', and 'cardinality'.
COLUMNS = ['server', 'client', 'cardinality', 'success',
           'bytes transmitted', 'bytes received',
           'communication time(s)', 'idle time(s)', 'computation time(s)']
AUX_COLUMNS = ['latency', 'uBandwidth', 'dBandwidth', 'measuremntsDuration',
               'MasurementsStartedAt']


class Algo(Enum):
    """Possible GenSync Algorithms."""

    CPI = 1,
    IBLT = 2,
    Cuckoo = 3,
    I_CPI = 4


def figure_out_algo_from_colosseum(run_path: str) -> Algo:
    """Return algorithm name based on the path to the run."""
    if ('IBLT' in run):
        return Algo.IBLT
    elif ('Cuckoo' in run):
        return Algo.Cuckoo
    elif ('_I_CPI' in run):
        return Algo.I_CPI
    else:
        return Algo.CPI


if __name__ == '__main__':
    experiments_root_dir = os.path.normpath(sys.argv[1])
    is_colosseum = True if len(sys.argv) > 2 else False

    # a run is a sync execution on a pair of server-client files
    experiment_runs = []
    for node in os.listdir(experiments_root_dir):
        if os.path.isdir(os.path.join(experiments_root_dir, node)) \
           and node.startswith('.cpisync_'):
            experiment_runs.append(node)

    if is_colosseum:
        COLUMNS.insert(0, 'algorithm')
        COLUMNS += AUX_COLUMNS
        columns_to_match = COLUMNS[4:]
    else:
        columns_to_match = COLUMNS[3:]

    experiment_df = pd.DataFrame(columns=COLUMNS)
    for run in experiment_runs:
        run_dir = os.path.join(experiments_root_dir, run)
        run_dir_content = os.listdir(run_dir)
        run_dir_content.sort()

        if is_colosseum:
            algo = figure_out_algo_from_colosseum(run).name

        for observ in run_dir_content:
            if not observ.endswith(OBSERV_SUFFIX):
                continue

            f_name_parts_count = len(run.split('_'))
            if (f_name_parts_count == 3):
                server, client = run.split('_')[-2:]
                cardinality = ''
            elif (f_name_parts_count == 4):
                server, client, cardinality = run.split('_')[-3:]
            elif (is_colosseum
                  and run.startswith(COLOSSEUM_SPECIFIC_DOT_CPI_DIR)):
                client, server = run.split('_')[-2:]
                cardinality = COLOSSEUM_DEFAULT_CARDINALITY
                gensync_algo = figure_out_algo_from_colosseum(run)
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
                    for measure in columns_to_match:
                        if line.lower().startswith(measure.lower()):
                            # success is treated differently
                            if measure == 'success':
                                if 'true' in line or '1' in line:
                                    measurements.append(True)
                                else:
                                    measurements.append(False)
                                break
                            # measurements start time is treated differently
                            if measure == 'MasurementsStartedAt':
                                str_t = line.split(':')[-1].strip()
                                d_t = datetime.datetime.strptime(str_t, '%c')
                                measurements.append(d_t)
                                break

                            val = float(line.split(':')[-1].strip())
                            measurements.append(val)
                            break

            prefix = [server, client, cardinality]
            if is_colosseum:
                prefix.insert(0, algo)

            # Fix server _observ measurements for Colosseum data. This
            # is needed because only the server _observ file holds the
            # auxiliary measurements.
            needed_cols = len(COLUMNS) - len(prefix)
            if is_colosseum and len(measurements) < needed_cols:
                more = needed_cols - len(measurements)
                measurements += [float('NaN')] * more

            # add a row
            experiment_df.loc[
                len(experiment_df.index)] = prefix + measurements

    # write the data frame of this experiments_root_dir
    experiment_df.to_csv('{}.csv'.format(
        os.path.basename(experiments_root_dir)))
