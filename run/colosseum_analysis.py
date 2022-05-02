"""
Data analysis for GenSync on Colosseum.

Author: Novak Boskov <boskov@bu.edu>
Date: May 2022.
"""

import pandas as pd


def parse(path: str) -> pd.DataFrame:
    """Parse CSV file in a DataFrame."""
    return pd.read_csv(path)
