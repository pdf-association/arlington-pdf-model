# Arlington PDF Model Notes

This file documents various aspects of the Arlington PDF data model, including assumptions, limitations, and suggestions for adopters.  

Currently the Arlington PDF Model defines all PDF objects defined by, or mentioned within, all Tables in ISO 32000-2:2020 and as updated by PDF Association [errata on ISO 32000-2:2020](https://pdf-issues.pdfa.org/32000-2-2020/) which fully includes all resolved errata in ISO 32000-2:2020/Amd1. Table coverage is documented in the spreadsheet [Arlington-vs-ISO32K-Tables.xlsx](./Arlington-vs-ISO32K-Tables.xlsx) 

## Assumptions

* PDF is an extensible language. As such, any dictionary or stream can have undocumented or private keys whether these are officially second-class or third-class names or not. The current Arlington PDF Model does **not** define this, as it uses the wildcard `*` for those objects that are **explicitly** defined in ISO 32000-2 to contain keys with arbitrary names.


* Arlington defines `name-tree` and `number-tree` as pre-defined types. This means that rules and predicates relating to **Kids**, **Names**, and **Limits** are _not_ defined by the model (e.g. validity with `null` values or empty arrays).
    - the [veraPDF Arlington repo](https://github.com/veraPDF/veraPDF-arlington-tools) also has additional useful information such as how it validates `name-tree` and `number-tree` data structures to account for issues such as [this bug report](https://github.com/pdfcpu/pdfcpu/issues/1111#issuecomment-2724308872).
    
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

* `null` is only defined in the Arlington PDF model if explicitly mentioned in ISO 32000-2:2020.
    - Dictionary handling is covered by subclause 7.3.7 "_A dictionary entry whose value is null (see 7.3.9, "Null object") shall be treated the same as if the 
    entry does not exist._" so dictionaries will never have a `null` type unless ISO 32000-2 explicitly mentions it or there is a glitch in the matrix 
    (e.g. Table 207 for **Mac** and **Unix** entries).
    - Array objects and `name-tree` / `number-tree`s are more complex as ISO 32000-2:2020 makes no statements about `null`. See also 
    [Arlington Issue #90](https://github.com/pdf-association/arlington-pdf-model/issues/90) and [PDF 2.0 Errata #157](https://github.com/pdf-association/pdf-issues/issues/157).


## Limitations

* FDF specific objects and entries specific to FDF in existing PDF objects are currently **not** defined in the Arlington PDF Model. See subclause 12.7.8 _Forms data format_.

* objects, keys or array entries that were removed in legacy Adobe PDF specifications (i.e. prior to adoption of PDF 1.7 as ISO 32000-1:2008) are **not** currently fully defined in the Arlington PDF Model.
    - very few from PDF 1.0 or PDF 1.1 have been defined as proof-of-concept of the PDF versioning predicates
    - Arlington does not distinguish between removal (obsoleting) and deprecation. Removed features are considered deprecated in the model.

* PDF lexical rules are currently **not** defined in the Arlington PDF Model - they are assumed.
    - In the future an EBNF or ANTLR definition may be added.
    - note that recent PDF errata have clarified the lexical constructs for PDF integers, PDF reals, object numbers and generation numbers.
    - You may also be interested in the SafeDocs [Compacted Syntax tests](https://github.com/pdf-association/safedocs/tree/main/CompactedSyntax)
    - the [veraPDF Arlington repo](https://github.com/veraPDF/veraPDF-arlington-tools) also has additional useful information 

* PDF graphical operators and operands used in content streams are currently **not** defined in the Arlington PDF Model.
    - In the future, an ANTLR definition such as [iText pdfcop](https://github.com/itext/pdfcop) may be added.

* the full details of PDF Linearization are currently **not** defined in the Arlington PDF Model.
    - the Linearization dictionary is defined by [LinearizationParameterDict.tsv](./tsv/latest/LinearizationParameterDict.tsv) but this is **not** linked into the rest of the Arlington PDF Model (since Linearization stands separately from the `trailer`)

* the semantics of PDF file structure and revisions including cross-reference tables, incremental updates, etc. is currently **not** defined in the Arlington PDF Model.

* "Annex L (normative) Parent-child relationships between the standard structure elements in the standard structure namespace for PDF 2.0" lists the permitted child and parent relationships for all PDF 2.0 standard structure elements. The Arlington PDF Model currently **does not** implement Annex L (there is an embedded XLSX in ISO 32000-2:2020 so it should be possible to enforce these requirements in the future using this existing machine-readable asset).

## Adoption Notes

This section makes suggestions to users who are implementing technology based on the Arlington PDF Model:

* lexing and parsing differences between PDF libraries **will** result in different behavior! There is **not** necessarily one true answer for a PDF file that has errors! 
    - for example, the escape sequences used with `name` and `string` objects may be hidden or exposed by PDF libraries. Results from Arlington implementations may thus vary!

* don't focus on PDF versions. It is widely accepted/known that PDF versions are not a reliable indicator of features!!!
    - This is why the [TestGrammar C++ PoC](TestGrammar) "rounds up" PDF versions... see also the `--force` command line option.
    - Also see [this PDF Association article](https://pdfa.org/pdf-versions/)

* any pre-amble bytes before the first `%PDF-x.y.` header line as well as any post-amble non-whitespace bytes after the last `%%EOF` should also be reported. Checking for a binary file marker comment as the 2nd line might is also useful.  

* consider reporting unexpected (i.e. not officially documented) keys in dictionaries and streams unless there is a `*` wildcard entry as the last row in the TSV data file. Ideally, analyze each custom key name to see if it is an official second-class or third-class PDF name as per Annex E.2 of ISO 32000-2:2020. 
    - This is what the [TestGrammar C++ PoC](TestGrammar) does.

* consider reporting the occurrence of `Metadata` and `AF` keys in irregular objects. The Arlington PDF Model explicitly defines these keys when ISO 32000-2:2020 does. This is perfectly valid but useful information.
    - This is what the [TestGrammar C++ PoC](TestGrammar) does, except for Document Part Metadata constraints described above.

* consider reporting when arrays contain more entries than needed (including for `matrix` and `rectangle`). This includes fixed-length arrays, as well as variable-length arrays that have repeating sets (e.g. expecting pairs of entries, but have an odd number of entries).
    - This is what the [TestGrammar C++ PoC](TestGrammar) does.

* consider reporting deprecation. This is "fat" that can be trimmed and that may lead to future issues in the long-term...

* some predicates may make no sense for your use-case... this is because the Arlington PDF Model reflects what the spec states, not how it is to be interpreted by a reader, writer, validator, editor, etc.!

* be careful about making assumptions of `stream` vs `dictionary`. As far as the Arlington PDF Model is concerned there is no real difference as the **stream** and **endstream** keywords are not analyzed. 

* remember that an encrypted PDF file (_even with unknown encryption!_) can still be checked - only the _content_ of `string-*`s and `stream`s can't. This is highly dependent on the PDF library and API in use as to whether this actually works. Results from Arlington implementations may thus vary!
    - in order to check the predicate `fn:AlwaysUnencrypted()` used with PDF string objects, the PDF SDK must be able to provide the string before attempting decryption (a "raw" string) and after decryption. If these strings are the same, then it is assumed that the PDF string object was unencrypted in the PDF file.  

* remember that PDF dictionaries with duplicate keys **do** exist in the wild however many PDF libraries and APIs will hide this fact from users - and can be further confused when normalized key names are duplicates (but not when non-normalized with escape sequences!). Thus results from Arlington implementations may vary!

* depending on the PDF SDK being used, objects with object numbers larger than the trailer **Size** entry may or may not be reported. Some SDKs already implement the necessary code to nullify objects with invalid object numbers and thus the Arlington TestGrammar PoC will never be able to report such errors. Other SDKs do not implement this check and thus expose objects with invalid object numbers to TestGrammar. 

* how PDF SDKs handle invalid object references (i.e. indirect references to objects that are not present in the PDF file) will vary. For example, some PDF SDKs may internally check if an indirect reference (`10 0 R`) actually refers to an object and thus report this as **null**, while others may defer this processing and report this as an indirect reference to object 10, others may error, etc.

* the Arlington PDF model does not define how implementations should read a PDF file - they are free to always use the cross-reference table information in the PDF, always rebuild the PDF cross-reference, or use some other custom algorithm to detect PDF objects. Such decisions can and will lead to different results!
    - support for hybrid-reference PDFs will also result in differences between PDF SDKs that are "PDF 1.5 processors" and those that are pre-PDF 1.5. As far as is known, no PDF SDK allows configuration of such behavior.

* most PDF SDKs do not report details on Linearization data (since it is not linked into the PDF DOM from the trailer) or details on file revisions such as cross-reference sections in incremental updates (includes errors in trailers, `startxref` or **Prev** entries in incremental updates). Thus the majority of tools do not report file structural errors when incremental updates are present. Such errors with incremental update data and file structure can result in a _very_ different understanding of which objects are valid or not!  

The Arlington PDF Model accurately reflects the latest agreed ISO 32000-2:2020 PDF 2.0 specification ([available for no-cost](https://pdfa.org/sponsored-standards/)) and as amended by industry-agreed errata from https://pdf-issues.pdfa.org. If this state of affairs is unsuitable for adopters of the Arlington PDF Model (e.g. unresolved errata are causing issues for implementations) then the recommended practice is for those specific implementations to create private `diff` patches against the official model in GitHub, as it is entirely text-based (TSV files).
