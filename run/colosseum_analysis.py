"""
Data analysis for GenSync on Colosseum.

Author: Novak Boskov <boskov@bu.edu>
Date: May 2022.
"""

from typing import Optional
import pandas as pd
import json

SUMMARY_COLUMNS = ['algorithm', 'diffs', 'cardinality',
                   'success', 'bytes exchanged', 'ttr']


def sanity_check(data: pd.DataFrame) -> Optional[pd.DataFrame]:
    """Raise exceptions if DataFrame has suspicious data."""
    unsuccessful = data[~data['success']]

    if unsuccessful.shape[0]:
        raise ValueError(f"Unsuccessful syncs at: {unsuccessful.index}")

    return data


def parse(path: str, summarize=False) -> pd.DataFrame:
    """
    Parse GenSync experimental observations CSV file into a DataFrame.

    Parameters
    ----------
    summarize : bool
        Aggregate each pair of rows into one (otherwise, first in the
        pair is server's and the second is client's perspective)
    """
    df = pd.read_csv(path)
    df = df.drop(columns=['Unnamed: 0'])  # this column is index, redundant
    df['success'] = df['success'].astype(bool)

    if not summarize:
        return sanity_check(df)

    if len(df.index) % 2:
        raise ValueError(
            f"DataFrame.shape is {df.shape}. Rows number not even.")

    ret = pd.DataFrame(columns=SUMMARY_COLUMNS)
    for i in df.index[::2]:
        algo = df['algorithm'][i]
        assert df['server'][i] == df['client'][i + 1]
        diffs = df['server'][i]
        card = df['cardinality'][i]
        assert df['success'][i] == df['success'][i + 1]
        succ = df['success'][i]

        this_btr = float(df['bytes transmitted'][i])
        this_bre = float(df['bytes received'][i])
        next_btr = float(df['bytes transmitted'][i + 1])
        next_bre = float(df['bytes received'][i + 1])
        bts = max(this_btr + this_bre, next_btr + next_bre)

        this_cmt = float(df['communication time(s)'][i])
        this_it = float(df['idle time(s)'][i])
        this_ptt = float(df['computation time(s)'][i])
        next_cmt = float(df['communication time(s)'][i + 1])
        next_it = float(df['idle time(s)'][i + 1])
        next_ptt = float(df['computation time(s)'][i + 1])
        ttr = max(this_cmt + this_it + this_ptt, next_cmt + next_it + next_ptt)

        row = [algo, diffs, card, succ, bts, ttr]
        ret.loc[len(ret.index)] = row

    ret['success'] = ret['success'].astype(bool)

    return sanity_check(ret)


class Violations:
    """Validity information."""

    def __init__(self):
        """Construct a validity object."""
        self.idle_time_outliers_even_index = []

    def __str__(self):
        """To string."""
        return self.__dict__.__str__()


def check_validity(data: pd.DataFrame) -> Violations:
    """
    Check validity of data.

    Primarily performs a sanity check for unusually high idle times.

    Parameters
    ----------
    data : DataFrame
        Data frame to check
    """
    it_s = 'idle time(s)'

    prc_68 = data[it_s].mean() + data[it_s].std()
    index_v = [x for x in data[data[it_s] > prc_68].index if x & 1]

    v = Violations()
    v.idle_time_outliers_even_index = index_v

    return v


def fix(original: pd.DataFrame, fix: pd.DataFrame) -> pd.DataFrame:
    """
    Replace data points from `original` with those in `fix`.

    Parameters
    ----------
    original: DataFrame
        Original data that will be modified
    fix: DataFrame
        Data with same columns as `original` whole rows will be
        embedded in `original`
    """
    assert original.columns.tolist() == fix.columns.tolist()

    criteria = 'algorithm'
    criteria_values = fix[criteria].unique()

    assert len(criteria_values) == 1

    original = original.drop(
        original[original[criteria] == criteria_values[0]].index)
    original = pd.concat([original, fix], ignore_index=True)

    return original


def parse_ping(f_name: str) -> pd.DataFrame:
    """
    Parse ping output into a Data Frame.

    ----------
    f_name : str
        Path to the f_name to parse
    """
    time_s = 'time='

    ret = pd.DataFrame(columns=['rtt'])
    with open(f_name, 'r') as f:
        while True:
            line = f.readline()
            if not line:
                break
            if time_s not in line:
                continue
            val = float(line.split(time_s)[1].split(' ')[0])
            ret.loc[len(ret.index)] = [val]

    return ret


def parse_iperf(f_name: str) -> pd.DataFrame:
    """
    Parse iperf3 json output into a DataFrame.

    ----------
    f_name : str
        Path to the json file
    """
    with open(f_name, 'r') as f:
        plain = json.load(f)

    sums = [inter['sum'] for inter in plain['intervals']]

    return pd.DataFrame.from_dict(sums)
