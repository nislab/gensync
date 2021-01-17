#!/usr/bin/env python
"""
Description

Usage: ./count_common.py FILE_PATH SETS_SEPARATOR_LINE_STARTS_WITH

Author: Novak Boškov <boskov@bu.edu>
Date: Jan, 2021.
"""

import sys

if __name__ == '__main__':
    file_path = sys.argv[1]
    sep_line_starts = sys.argv[2]

    set_A = set()
    set_B = set()
    sep_count = 0
    with open(file_path, 'r') as fi_le:
        while True:
            line = fi_le.readline()
            if not line:
                break
            if line.startswith(sep_line_starts):
                sep_count = sep_count + 1
                continue
            if sep_count == 1:
                set_A.add(line.strip())
            elif sep_count == 2:
                set_B.add(line.strip())

    if sep_count > 2:           # files are expected to have 2 separators
        sys.stderr.write('Found {} separators in file. Something went wrong.'
                         .format(sep_count()))
        exit(1)

    sys.stdout.write('{}'.format(len(set_A.intersection(set_B))))
