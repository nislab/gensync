#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Counts common elements among the sets.

Usage: ./count_common.py FILE_PATH SETS_SEPARATOR_LINE_STARTS_WITH
Returns: INTERSECTION_SIZE A_DIFF_B_SIZE B_DIFF_A_SIZE A_SIZE B_SIZE

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
        first_line = True
        has_header = False
        while True:
            line = fi_le.readline()
            if not line:
                break

            if first_line and 'Sync protocol' in line:
                has_header = True

            first_line = False

            if line.startswith(sep_line_starts):
                sep_count = sep_count + 1
                continue

            if (has_header and sep_count == 1) \
               or (not has_header and sep_count == 0):
                set_A.add(line.strip())
            elif (has_header and sep_count == 2) \
                    or (not has_header and sep_count == 1):
                set_B.add(line.strip())

    if sep_count > 2:           # files are expected to have 2 separators
        sys.stderr.write('Found {} separators in file. Something went wrong.'
                         .format(sep_count()))
        exit(1)

    sys.stdout.write('{} {} {} {} {}'
                     .format(len(set_A.intersection(set_B)),
                             len(set_A.difference(set_B)),
                             len(set_B.difference(set_A)),
                             len(set_A), len(set_B)))
