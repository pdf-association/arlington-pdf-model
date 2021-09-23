#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright 2021 PDF Association, Inc. https://www.pdfa.org
#
# This material is based upon work supported by the Defense Advanced
# Research Projects Agency (DARPA) under Contract No. HR001119C0079.
# Any opinions, findings and conclusions or recommendations expressed
# in this material are those of the author(s) and do not necessarily
# reflect the views of the Defense Advanced Research Projects Agency
# (DARPA). Approved for public release.
#
# SPDX-License-Identifier: Apache-2.0
# Contributors: Peter Wyatt, PDF Association
#
# A simple Python routine to combine a full set of Arlington PDF Model
# TSV files into a single Pandas-compatible TSV file. The TSV filename
# is added as column 0 (i.e. as the left-most column). The same output
# file also then works very well with the EBay 'tsv-utilities'. See
# https://github.com/eBay/tsv-utils.
#
# Python QA:
# - flake8 arlington-to-pandas.py
# - pyflakes arlington-to-pandas.py
# - mypy arlington-to-pandas.py
#
# In Jupyter/pandas:
#
#   %matplotlib inline
#   import pandas as pd
#   import matplotlib.pyplot as plt
#   import seaborn as sns
#   sns.set(style="darkgrid")
#
#   df = pd.read_csv('pandas.tsv', delimiter='\t', na_filter=False,
#                dtype={'Object':'string', 'Key':'string', 'Type':'string',
#                        'SinceVersion':'string', 'DeprecatedIn':'string',
#                        'Required':'string', 'IndirectReference':'string',
#                        'Inheritable':'string', 'DefaultValue':'string',
#                        'PossibleValues':'string', 'SpecialCase':'string',
#                        'Link':'string', 'Note':'string'})
#
# df is a pandas DataFrame of a full Arlington file set
#

import sys
import csv
import os
import glob
import argparse


def ArlingtonToPandas(dir: str, pandas_fname: str):
    fcount = 0
    pandas_fields = ['Object', 'Key', 'Type', 'SinceVersion', 'DeprecatedIn',
                     'Required', 'IndirectReference', 'Inheritable',
                     'DefaultValue', 'PossibleValues', 'SpecialCase', 'Link',
                     'Note']

    with open(pandas_fname, 'w', newline='') as pandasfile:
        pandaswriter = csv.DictWriter(pandasfile, fieldnames=pandas_fields,
                                      delimiter='\t', quoting=csv.QUOTE_MINIMAL)
        pandaswriter.writeheader()
        for filepath in glob.iglob(os.path.join(dir, r"*.tsv")):
            obj_name = os.path.splitext(os.path.basename(filepath))[0]
            fcount += 1
            with open(filepath, newline='') as csvfile:
                tsvreader = csv.DictReader(csvfile, delimiter='\t')
                for row in tsvreader:
                    row['Object'] = str(obj_name)
                    pandaswriter.writerow(row)
            csvfile.close()

    pandasfile.close()
    if (fcount == 0):
        print("There were no TSV files in directory", dir)
        return

    print("TSV files processed:", fcount)


if __name__ == '__main__':
    cli_parser = argparse.ArgumentParser()
    cli_parser.add_argument('-t', '--tsvdir', dest="tsvdir",
                            help='folder containing Arlington TSV file set')
    cli_parser.add_argument('-s', '--save', dest="pandas", default="pandas.tsv",
                            help='filename for single Pandas-compatible TSV')
    cli = cli_parser.parse_args()

    if (cli.tsvdir is None) or not os.path.isdir(cli.tsvdir):
        print("'%s' is not a valid directory" % cli.tsvdir)
        cli_parser.print_help()
        sys.exit()

    print("Loading from", cli.tsvdir)
    arl = ArlingtonToPandas(cli.tsvdir, cli.pandas)
    print("Done writing to", cli.pandas)
