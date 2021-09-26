# Arlington Changelog

Lists significant changes that may break existing implementations. Does **not** track data corrections to the PDF model:

## Branch: testgrammar-rewrite-pdfium

### Arlington PDF model grammar
* significant updates to [INTERNAL_GRAMMAR.md](INTERNAL_GRAMMAR.md) to ensure parsing consistency - read this!
* PDF string objects now use single quotes rather than rounded brackets to disambiguate from predicates
* rationalized list of predicates and their use for ensure parsing consistency

### Proof Of Concept apps
* significant restructure and rewrite of TestGrammar C++ application
* introduced a "shim" layer to decouple C++ from PDF SDK of choice.
* switched support to use pdfium as preferred PDF SDK
* introduced basic parsing of predicates - errors in model may cause crashes
* detailed Arlington model validation
* additional error and warning messages when processing PDF files against Arlington
* various updates to Python scripts

### New
* introduced a new Python script (arlington-to-pandas.py) to combine all TSV files in an Arlington file set into a monolithic TSV suitable for use by EBay 'tsv-utlities', pandas and Jupyter notebooks

### Still To-Do
* implement inheritance in TestGrammar C++
* implement predicate support in TestGrammar C++
* update Java `gcxml` to correctly support versioning predicates
* offer QPDF as a PDF SDK for TestGrammar C++
* update pikepdf (QPDF) support in Python arlington.py script

## v0.3

Baselined prior to switching to pdfium as the preferred PDF SDK and corresponding edits.
