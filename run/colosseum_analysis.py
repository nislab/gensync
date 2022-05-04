"""
Data analysis for GenSync on Colosseum.

Author: Novak Boskov <boskov@bu.edu>
Date: May 2022.
"""

import pandas as pd

SUMMARY_COLUMNS = ['algorithm', 'diffs', 'cardinality',
                   'success', 'bytes exchanged', 'ttr']


def parse(path: str, summarize=False) -> pd.DataFrame:
    """
    Parse CSV file into a DataFrame.

    Parameters
    ----------
    summarize : bool
        Aggregate each pair of rows into one (otherwise, first in the
        pair is server's and the second is client's perspective)
    """
    df = pd.read_csv(path)
    df = df.drop(columns=['Unnamed: 0'])  # this column is index, redundant

    if not summarize:
        return df

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
        if this_btr + this_bre >= next_btr + next_bre:
            bts = this_btr + this_bre
        else:
            bts = next_btr + next_bre

        this_cmt = float(df['communication time(s)'][i])
        this_it = float(df['idle time(s)'][i])
        this_ptt = float(df['computation time(s)'][i])
        next_cmt = float(df['communication time(s)'][i + 1])
        next_it = float(df['idle time(s)'][i + 1])
        next_ptt = float(df['computation time(s)'][i + 1])
        if this_cmt + this_it + this_ptt >= next_cmt + next_it + next_ptt:
            ttr = this_cmt + this_it + this_ptt
        else:
            ttr = next_cmt + next_it + next_ptt

        row = [algo, diffs, card, succ, bts, ttr]
        ret.loc[len(ret.index)] = row

    return ret


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
