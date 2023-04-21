# Arlington PDF Model Notes

This file documents various aspects of the Arlington PDF data model, including assumptions, limitations, and suggestions for adopters.  

Currently the Arlington PDF Model defines all PDF objects defined by, or mentioned within, all Tables in ISO 32000-2:2020 and as updated by PDF Association [errata on ISO 32000-2:2020](https://pdf-issues.pdfa.org/32000-2-2020/) which fully includes all errata in ISO [32000-2:2020 Amd1](https://pdfa.org/errata/errata-for-iso-32000-2-2020/). This is documented in  the spreadsheet [Arlington-vs-ISO32K-Tables.xlsx](./Arlington-vs-ISO32K-Tables.xlsx) 

## Assumptions

* PDF is an extensible language. As such, any dictionary or stream can have undocumented or private keys whether these are officially second-class or third-class names or not. The current Arlington PDF Model does **not** define this, as it reserves the use of the wildcard `*` for those objects that are **explicitly** defined in ISO 32000-2 to contain keys with arbitrary names.

* ISO 32000-2:2020 subclause 14.3.2 _Metadata streams_ permits the `Metadata` key to be any dictionary or stream. This is **not** explicitly modelled across every dictionary in the current Arlington PDF model. Arlington only defines a `Metadata` entry when ISO 32000-2:2020 explicitly declares it. 
    - In the future, a `Metadata` entry might be added to every dictionary or stream object. Please add comments to [Issue #65](https://github.com/pdf-association/arlington-pdf-model/issues/65) if you feel strongly one way or the other.

* ISO 32000-2:2020 subclause 14.13 _Associated files_ permits the `AF` key to be any dictionary or stream. This is **not** explicitly modelled across every dictionary in the current Arlington PDF model. Arlington only defines an `AF` entry when ISO 32000-2:2020 explicitly declares it. 
    - In the future, a `AF` entry might be added to every dictionary or stream object. Please add comments to [Issue #65](https://github.com/pdf-association/arlington-pdf-model/issues/65) if you feel strongly one way or the other.


## Limitations

* FDF specific objects and entries specific to FDF in existing PDF objects are currently **not** defined in the Arlington PDF Model. See subclause 12.7.8 _Forms data format_.

* objects, keys or array entries that were removed in legacy Adobe PDF specifications (i.e. prior to adoption of PDF 1.7 as ISO 32000-1:2008) are **not** defined in the Arlington PDF Model.
    - very few from PDF 1.0 or PDF 1.1 have been defined as proof-of-concept of the PDF versioning predicates
    - Arlington does not distinguish between removal and deprecation. Removed features are considered deprecated in the model.

* PDF lexical rules are currently **not** defined in the Arlington PDF Model - they are assumed.
    - In the future an EBNF or ANTLR definition may be added.
    - You may also be interested in the SafeDocs [Compacted Syntax tests](https://github.com/pdf-association/safedocs/tree/main/CompactedSyntax)

* PDF graphical operators and operands are currently **not** defined in the Arlington PDF Model.
    - In the future an ANTLR definition such as [iText pdfcop](https://github.com/itext/pdfcop) may be added.

* the full details of PDF Linearization are currently **not** defined in the Arlington PDF Model.
    - the Linearization dictionary is defined by [LinearizationParameterDict.tsv](./tsv/latest/LinearizationParameterDict.tsv) but this is **not** linked into the rest of the Arlingon PDF Model (since Linearization stands separately from the `trailer`)

* the semantics of PDF file structure including cross-references tables, incremental updates, etc. is currently **not** defined in the Arlington PDF Model.


## Adoption Notes

This section makes suggestions to users who are implementing technology based on the Arlington PDF Model:

* don't focus on PDF versions. It is widely accepted/known that PDF versions are not a reliable indicator of features.
    - This is why the [TestGrammar C++ PoC](TestGrammar) "rounds up" PDF versions... see the `--force` command line option.

* consider reporting unexpected (i.e. not officially documented) keys in dictionaries and streams unless there is a `*` wildcard entry as the last row in the TSV data file. Ideally analyze each custom key name to see if it is an official second-class or third-class PDF name as per Annex E.2 of ISO 32000-2:2020. 
    - This is what the [TestGrammar C++ PoC](TestGrammar) does.

* consider reporting the occurance of `Metadata` and `AF` keys in all non-document objects. This is perfectly valid but useful information.
    - This is what the [TestGrammar C++ PoC](TestGrammar) does.

* consider reporting when arrays contain more entries than needed (including for `matrix` and `rectangle`)
    - This is what the [TestGrammar C++ PoC](TestGrammar) does.

* consider reporting deprecation. This is "fat" that can be trimmed...

* some predicates may make no sense for your use-case... this is because the Arlington PDF Model reflects what the spec states, not how it is to be interpreted!

* be careful about making assumptions of streams vs dictionaries. As far as the Arlington PDF Model is concerned there is no real difference... 
