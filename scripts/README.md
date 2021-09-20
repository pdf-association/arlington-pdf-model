# Scripts

All scripts require Python3 and are cross-platform (tested on both Windows and Linux). Command line options for all Python scripts and TestGrammar C++ PoC are the same to keep things simple.

### [arlington-to-pandas.py](arlington-to-pandas.py)

This script concatenates an entire Arlington TSV file set specified with the `-t`/`--tsvdir` command line option into a single monolithic TSV file (default file is called `pandas.tsv`, otherwise specify with `-s`/`--save` command line option. An additional first column (i.e. added to the **left** of the standard Arlington fields) represents the Arlington TSV filename (a.k.a. PDF object). This combined TSV is then suitable for processing directly in Jupyter Notebooks using [pandas](https://pandas.pydata.org/).

An example Jupyter Notebook is provided at [Arlington.ipynb](Arlington.ipynb).

###  [arlington-fn-lex.py](arlington-fn-lex.py)

This script syntactically validates (i.e. tries to parse) all predicates in the Arlington PDF model. Output is the AST produced by the [Python Sly parser](https://sly.readthedocs.io/en/latest/). Predicate parsing errors (such as typos) will result in a Python exception and premature failure. This should be one of the first tests done when adding or changing predicates, or working out how to construct a valid predicate.

```
Usage: arlington-fn-lex.py [- | files ]
```

Example usage (Linux CLI):

```bash
echo "fn:Eval(@key,-27,1.23,'string',path::key1,((expr1==expr2) && (a!=b) || (4>3)))" | python3 arlington-fn-lex.py
grep --color=never -Pho "fn:[a-zA-Z0-9]+\([^\t\]\;]*\)" ../tsv/latest/*.tsv | sort | uniq | python3 arlington-fn-lex.py
```

### [arlington.py](arlington.py)

This is the main PoC mega-script and can:

1. convert an Arlington TSV data set into an in-memory Python representation and save out to pure JSON, which can then processed by `jq`. Use `-j/``--json`.
1. perform a detailed validation of the Arlington Python in-memory representation read in from an Arlington TSV file set, including attempted lexing of all predicates. Use `--validate`
1. save the Python representation as a 'pretty-print' which is more friendly for simple Linux commands such as `grep`, etc. but is technically invalid JSON and **not** suitable for `jq`. See `-s`/`--save`.
1. validates a PDF file, or folder of PDFs, against the Arlington model using `pikepdf`. See `-p`/`--pdf` and `-o`/`--out`

It relies on the [Python Sly parser](https://sly.readthedocs.io/en/latest/) and `pikepdf` [doco](pikepdf.readthedocs.io/) which is Python wrapper on top of [QPDF](https://github.com/qpdf/qpdf).

```bash
pip3 install sly pikepdf
qpdf --version
# Colorized JSON output (long!)
jq -C '.' json.json | more
# select a specific key in a specific PDF object
jq '.PageObject.CropBox' json.json
# list all the keys in Arlington as quoted strings
jq 'add | keys[]' json.json
# List the keys in a specific PDF object as an array
jq '.PageObject | keys' json.json
```

A useful JQ cookbook is [here](https://github.com/stedolan/jq/wiki/Cookbook).

---

Copyright 2021 PDF Association, Inc. https://www.pdfa.org

This material is based upon work supported by the Defense Advanced Research Projects Agency (DARPA) under Contract No. HR001119C0079. Any opinions, findings and conclusions or recommendations expressed in this material are those of the author(s) and do not necessarily reflect the views of the Defense Advanced Research Projects Agency (DARPA). Approved for public release.
