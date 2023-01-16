# Sample Arlington PDF Model Jupyter Notebook

The `../scripts/arlington-to-pandas.py` script concatenates an entire Arlington TSV file set specified with the `-t`/`--tsvdir` command line option into a single monolithic TSV file (default file is called `pandas.tsv`, otherwise specify with `-s`/`--save` command line option. An additional first column (i.e. added to the **left** of the standard Arlington fields) represents the Arlington TSV filename (a.k.a. PDF object). This combined TSV is then suitable for processing directly in Jupyter Notebooks using [pandas](https://pandas.pydata.org/).

A well commented example Jupyter Notebook is provided at [Arlington.ipynb](Arlington.ipynb). This demonstrates simple queries and analysis that are possible using the Arlington PDF Model inside Jupyter Notebooks for those familiar with Python. A lot of similar functionality is also possible at Linux CLI through the use of EBAY TSV-Utils, `grep`, `sed`, `sort -u`, etc. 

## Using

To install and run:
```
pip3 install jupyter
jupyter-notebook
```

---
Copyright 2021-23 PDF Association, Inc. https://www.pdfa.org

This material is based upon work supported by the Defense Advanced Research Projects Agency (DARPA) under Contract No. HR001119C0079. Any opinions, findings and conclusions or recommendations expressed in this material are those of the author(s) and do not necessarily reflect the views of the Defense Advanced Research Projects Agency (DARPA). Approved for public release.