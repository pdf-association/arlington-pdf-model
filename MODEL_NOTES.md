# Arlington PDF Model Notes

This file documents various aspects of the Arlington PDF data model, including assumptions, limitations, and suggestions for adopters.  

Currently the Arlington PDF Model defines all PDF objects defined by, or mentioned within, all Tables in ISO 32000-2:2020 and as updated by PDF Association [errata on ISO 32000-2:2020](https://pdf-issues.pdfa.org/32000-2-2020/) which fully includes all resolved errata in ISO 32000-2:2020/Amd1. Table coverage is documented in the spreadsheet [Arlington-vs-ISO32K-Tables.xlsx](./Arlington-vs-ISO32K-Tables.xlsx) 

## Assumptions

* PDF is an extensible language. As such, any dictionary or stream can have undocumented or private keys whether these are officially second-class or third-class names or not. The current Arlington PDF Model does **not** define this, as it uses the wildcard `*` for those objects that are **explicitly** defined in ISO 32000-2 to contain keys with arbitrary names.

* ISO 32000-2:2020 subclause 14.3.2 _Metadata streams_ permits the `Metadata` key to be any dictionary or stream. This is **not** explicitly modelled across every dictionary in the current Arlington PDF model. Arlington only defines a `Metadata` entry when ISO 32000-2:2020 explicitly declares it. 
    - In the future, a `Metadata` entry might be added to every dictionary or stream object. Please add comments to [Issue #65](https://github.com/pdf-association/arlington-pdf-model/issues/65) if you feel strongly one way or the other.
    - Note also that the Document Part Metadata **DPM** dictionary (_new in PDF 2.0_) also explicitly prohibits streams which means **Metadata** cannot occur below **DPM** objects. The TestGrammar PoC does **not** check for this!
    - See also [PDF Errata #403](https://github.com/pdf-association/pdf-issues/issues/403)

* ISO 32000-2:2020 subclause 14.13 _Associated files_ permits the `AF` key to be any dictionary or stream. This is **not** explicitly modelled across every dictionary in the current Arlington PDF model. Arlington only defines an `AF` entry when ISO 32000-2:2020 explicitly declares it. 
    - In the future, a `AF` entry might be added to every dictionary or stream object. Please add comments to [Issue #65](https://github.com/pdf-association/arlington-pdf-model/issues/65) if you feel strongly one way or the other.
    - Note also that the Document Part Metadata **DPM** dictionary (_new in PDF 2.0_) also explicitly prohibits streams which means Associated Files cannot occur below **DPM** objects.  The TestGrammar PoC does **not** check for this!
    - See also [PDF Errata #403](https://github.com/pdf-association/pdf-issues/issues/403)

* assumptions about duplicate keys, keys with `null` values, and keys that are indirect references are currently **not** encoded in the Arlington PDF Model:
    - e.g. consider `<< /Type /Bar /Type Boo >>`, `<< /Type null /Type /Bar >>` and `<< 10 0 R /Bar >>` when `10 0 obj /Type endobj`
    - in ISO 32000-2:2020 keys are now required to be direct objects. Prior to PDF 2.0 there was no such statement so old implementations vary. 

* `null` is only defined in the Arlington PDF model if it is explicitly mentioned in ISO 32000-2:2020.
    - Dictionary handling is covered by subclause 7.3.7 "_A dictionary entry whose value is null (see 7.3.9, "Null object") shall be treated the same as if the 
    entry does not exist._" so dictionaries will never have a `null` type unless ISO 32000-2 explicitly mentions it or there is a glitch in the matrix 
    (e.g. Table 207 for **Mac** and **Unix** entries).
    - Array objects and name-tree and number-trees are more complex as ISO 32000-2:2020 makes no statements about `null`. See also 
    [Arlington Issue #90](https://github.com/pdf-association/arlington-pdf-model/issues/90) and [PDF 2.0 Errata #157](https://github.com/pdf-association/pdf-issues/issues/157).

* the [veraPDF Arlington repo](https://github.com/veraPDF/veraPDF-arlington-tools) also has additional useful information 

## Limitations

* FDF specific objects and entries specific to FDF in existing PDF objects are currently **not** defined in the Arlington PDF Model. See subclause 12.7.8 _Forms data format_.

* objects, keys or array entries that were removed in legacy Adobe PDF specifications (i.e. prior to adoption of PDF 1.7 as ISO 32000-1:2008) are **not** currently fully defined in the Arlington PDF Model.
    - very few from PDF 1.0 or PDF 1.1 have been defined as proof-of-concept of the PDF versioning predicates
    - Arlington does not distinguish between removal (obsoleting) and deprecation. Removed features are considered deprecated in the model.

* PDF lexical rules are currently **not** defined in the Arlington PDF Model - they are assumed.
    - In the future an EBNF or ANTLR definition may be added.
    - You may also be interested in the SafeDocs [Compacted Syntax tests](https://github.com/pdf-association/safedocs/tree/main/CompactedSyntax)
    - the [veraPDF Arlington repo](https://github.com/veraPDF/veraPDF-arlington-tools) also has additional useful information 

* PDF graphical operators and operands used in content streams are currently **not** defined in the Arlington PDF Model.
    - In the future an ANTLR definition such as [iText pdfcop](https://github.com/itext/pdfcop) may be added.

* the full details of PDF Linearization are currently **not** defined in the Arlington PDF Model.
    - the Linearization dictionary is defined by [LinearizationParameterDict.tsv](./tsv/latest/LinearizationParameterDict.tsv) but this is **not** linked into the rest of the Arlington PDF Model (since Linearization stands separately from the `trailer`)

* the semantics of PDF file structure and revisions including cross-references tables, incremental updates, etc. is currently **not** defined in the Arlington PDF Model.


## Adoption Notes

This section makes suggestions to users who are implementing technology based on the Arlington PDF Model:

* lexing and parsing differences between PDF libraries **will** result in different behavior! There is **not** necessarily one true answer for a PDF file that has errors! 
    - for example, the escape sequences used with `name` and `string` objects may be hidden or exposed by PDF libraries. Results from Arlington implementations may thus vary!

* don't focus on PDF versions. It is widely accepted/known that PDF versions are not a reliable indicator of features!!!
    - This is why the [TestGrammar C++ PoC](TestGrammar) "rounds up" PDF versions... see also the `--force` command line option.
    - Also see [this PDF Association article](https://pdfa.org/pdf-versions/)

* consider reporting unexpected (i.e. not officially documented) keys in dictionaries and streams unless there is a `*` wildcard entry as the last row in the TSV data file. Ideally, analyze each custom key name to see if it is an official second-class or third-class PDF name as per Annex E.2 of ISO 32000-2:2020. 
    - This is what the [TestGrammar C++ PoC](TestGrammar) does.

* consider reporting the occurrence of `Metadata` and `AF` keys in all non-document objects. The Arlington PDF Model explicitly defines these keys when ISO 32000-2:2020 does. This is perfectly valid but useful information.
    - This is what the [TestGrammar C++ PoC](TestGrammar) does, except for Document Part Metadata constraints described above.

* consider reporting when arrays contain more entries than needed (including for `matrix` and `rectangle`). This includes fixed-length arrays, as well as variable-length arrays that have repeating sets.
    - This is what the [TestGrammar C++ PoC](TestGrammar) does.

* consider reporting deprecation. This is "fat" that can be trimmed and that may lead to future issues in the long-term...

* some predicates may make no sense for your use-case... this is because the Arlington PDF Model reflects what the spec states, not how it is to be interpreted by a reader, writer, validator, editor, etc.!

* be careful about making assumptions of `stream` vs `dictionary`. As far as the Arlington PDF Model is concerned there is no real difference as the **stream** and **endstream** keywords are not analyzed. 

* remember that an encrypted PDF file (_even with unknown encryption!_) can still be checked - only the _content_ of `string-*`s and `stream`s can't. This is highly dependent on the PDF library and API in use as to whether this actually works. Results from Arlington implementations may thus vary!

* remember that PDF dictionaries with duplicate keys **do** exist in the wild however many PDF libraries and APIs will hide this fact from users. Thus results from Arlington implementations may vary!

* the Arlington PDF model does not define how implementations should read a PDF file - they are free to always use the cross-reference table information in the PDF, always rebuild the PDF cross-reference, or use some other custom algorithm to detect PDF objects. Such decisions can and will lead to different results!
    - support for hybrid-reference PDFs will also result in differences.

The Arlington PDF Model accurately reflects the latest agreed ISO 32000-2:2020 PDF 2.0 specification ([available for no-cost](https://pdfa.org/sponsored-standards/)) and as amended by industry-agreed errata from https://pdf-issues.pdfa.org. If this state of affairs is unsuitable for adopters of the Arlington PDF Model (e.g. unresolved errata are causing issues for implementations) then the recommended practice is for those specific implementations to create private `diff` patches against the official model in GitHub, as it is entirely text-based (TSV files).
