# The Arlington PDF Model

<img src="resources/ArlingtonPDFSymbol300x300.png" width="150">

![GitHub](https://img.shields.io/github/license/pdf-association/arlington-pdf-model)
&nbsp;&nbsp;&nbsp;
![PDF support](https://img.shields.io/badge/PDF-2.0-blue)
&nbsp;&nbsp;&nbsp;
![LinkedIn](https://img.shields.io/static/v1?style=social&label=LinkedIn&logo=linkedin&message=PDF-Association)
&nbsp;&nbsp;&nbsp;
![Twitter Follow](https://img.shields.io/twitter/follow/PDFAssociation?style=social)
&nbsp;&nbsp;&nbsp;
![YouTube Channel Subscribers](https://img.shields.io/youtube/channel/subscribers/UCJL_M0VH2lm65gvGVarUTKQ?style=social)

## Background

The Arlington PDF Model is a specification-derived, machine-readable definition of the full PDF document object model (DOM) as defined by the official PDF 2.0 specification [ISO 32000-2:2020](https://www.iso.org/standard/75839.html) and its [related resolved errata](https://www.pdfa.org/pdf-issues/). It provides an easy-to-process structured definition of all formally defined PDF objects (dictionaries, arrays and map objects) and their relationships beginning with the file trailer using a simple text-based syntax and a small set of declarative functions. The Arlington PDF Model is applicable to both parsers (PDF readers) and unparsers (PDF writers).

The Arlington definition does **not** replace the official ISO PDF specification and must always be used in conjunction with the PDF 2.0 document in order to fully understand the PDF DOM.

Each object from the latest PDF 2.0 dated revision (ISO 32000-2:2020) specification is represented by a single tabbed separated values (TSV) text file. Each TSV represents a single PDF object (either a dictionary, array, stream, or map) and contains all necessary data in the form of predicates (declarative functions or assertions) to validate real-world PDF files against the formal PDF specification. Sets of TSV files are also provided for each earlier PDF version - these are "down-derived" from the "latest" TSV file set using the version-based predicates.

The Arlington PDF Model predicates define various data integrity and other requirements expressed in the ISO PDF specification. All predicates start with `fn:` followed by a descriptive name. Predicates can be nested to form logical or mathematical expressions.   

Text-based TSV files are platform independent, and easy to view/edit either in a spreadsheet application, in a text editor, in something like a [Jupyter notebook](https://jupyter.org/), or directly here in Github. They are trivial to process using simple Linux commands (`cut`, `grep`, `sed`, etc.) or with more specialized "big data" utilities such as the [EBay TSV-Utilities](https://github.com/eBay/tsv-utils) or [GNU datamash](https://www.gnu.org/software/datamash/). Scripting languages like Python also natively support TSV via:

```python
import csv
tsvreader = csv.DictReader(file, delimiter='\t')
```

**The Arlington PDF model is still under active development via the DARPA-funded SafeDocs fundamental research program!**

### What's new?

The latest release of Arlington includes:

* significant data model updates and corrections,
* array naming convention of TSV files (`*Array*.tsv`, or `*ColorSpace.tsv` ) so that dictionaries and arrays are easily distinguishable,
* significant documentation updates,
* a fully consistent set of predicates for PDF object data integrity rules and requirements,
* a far more comprehensive validation of the Arlington PDF grammar (to avoid typos or other potential errors),
* significant updates to the TestGrammar (C++) proof-of-concept application to confirm predicates and perform a detailed version-based assessment of PDF files
* a far more detailed comparison with the Adobe DVA grammar.

### Limitations

The Arlington PDF Model currently does not define:

* PDF lexical rules and dialects (such as might be expressed with [EBNF](https://en.wikipedia.org/wiki/Extended_Backus%E2%80%93Naur_form)),
* PDF content streams (operators and operands),
* rules for the PDF file structure and layout including incremental updates, cross reference table data or linearization.

# TSV Data Overview

Each row in a TSV files is the definition for a specific key in a dictionary or element in an array. All the characteristics captured in the TSV for that key/array element are defined in the PDF 2.0 specification, as corrected by PDF 2.0 errata.

- All PDF names are always expressed **without** the leading FORWARD-SLASH (`/`).
- PDF strings are required to have `'` and `'` (single quotes).
- PDF array objects must also have `[` and `]` and, of course, do **not** use commas between array elements.
- PDF Boolean (keywords) are always lowercase `true` and `false`.
- Logical Boolean values related to the description of PDF objects in the Arlington PDF Data Model are always uppercase `TRUE`/`FALSE`.
- TSV field names (a.k.a. column titles) are shown double-quoted (`"`) in documentation to hopefully avoid confusion.


In the context of the Arlington PDF Model the following terminology is often used:

- "complex type" - a complex type is a key or array element which is allowed to be more than a single PDF type (e.g. `array;dictionary`). This often written as `[];[];[]` since this is how the Arlington model encodes this.
- "simple type" - a simple type is a predefined Arlington type that does not link to another Arlington TSV definition. This include `bitmask`, `boolean`, `integer`, `number`, `matrix`, etc.
- "version-based predicates" - a lot of details in the Arlington PDF model are dependent on the PDF version. These rules are encoded using several predicates including `fn:SinceVersion(...)`, `fn:IsPDFVersion(...)`, `fn:BeforeVersion(...)`, and `fn:Deprecated(...)`   


TSV fields are always in the following order and TABs must exist between all fields:

1. "[**Key**](#Key)" - key in dictionary, or a zero-based integer index into an array. ASTERISK (`*`) represents a wildcard and means any key/index. An integer followed by an ASTERISK (`*`) represents the requirements to have repeating sets of array elements.
1. "[**Type**](#Type)" - one or more of the pre-defined Arlington types alphabetically sorted and separated by SEMI-COLONs `;`, possibly with version-based predicates.
1. "[**SinceVersion**](#SinceVersion-and-DeprecatedIn)" - version of PDF this key/array element was introduced in.
1. "[**DeprecatedIn**](#SinceVersion-and-DeprecatedIn)" - version of PDF this key/array element was deprecated in. Empty if not deprecated.
1. "[**Required**](#Required)" - whether the key or array element is required. Might be expressed as a predicate.
1. **IndirectReference**" - whether the key is required to be an indirect reference (`TRUE`/`FALSE`) or if it must be a direct or indirect object (e.g. `fn:MustBeDirect(...)`).
1. **Inheritable**" - whether the key is inheritable (`TRUE`/`FALSE`). Might be expressed as a predicate.
1. **DefaultValue**" - optional default value of key/array element.
1. "[**PossibleValues**](#PossibleValues)" - list of possible values. For dictionary `/Type` keys that must have a specific value, this will be a choice of just a single value.
1. "[**SpecialCase**](#SpecialCase)" - predicates defining additional data integrity relationships.
1. "[**Link**](#Link)" - name(s) of other TSV files for validating the values of this key/array element for dictionaries, arrays, streams, maps, name-trees or number-trees.
1. "**Notes**" - free text for arbitrary notes. Often this will be a reference to a Table or subclause in ISO 32000-2:2020.

The two special objects [\_UniversalArray](tsv/latest/_UniversalArray.tsv) and [\_UniversalDictionary](tsv/latest/_UniversalDictionary.tsv) are not formally defined in the PDF 2.0 specification and represent generic an arbitrarily-sized PDF array and PDF dictionary respectively. They are used to resolve "Links" to generic objects for a few PDF objects.

## TSV Field Summary

A very precise definition of all syntax rules for the Arlington PDF model as well as Python equivalent data structure descriptions and useful Linux commands is in [INTERNAL_GRAMMAR.md](INTERNAL_GRAMMAR.md). Only a _simplified summary_ is provided below to get started. Note that not all TSV fields are shown in the examples below. And normally "Links" are not conveniently hyperlinked to actual TSV files either!


### Key

Field 1 is "Key" and represents a single key in a dictionary, an index into an array, multiple wildcard entries (`*`), or an array with required sets of entries (DIGIT+`*`). Dictionary keys are obviously case sensitive and array indices are always integers. To locate a key easily using Linux begin a regex with the start-of-line (`^`). For a precise match end the regex with a TAB (`\t`). Conveniently, ISO 32000 only uses ASCII characters for 1st class key names so there are no #-escapes used in Arlington. ISO 32000 also does not define any dictionary keys that are purely just an integer - Arlington leverages this fact so that array "keys" are always 0-based integers. Note that ISO 32000 does define some keys that start with integers (e.g. `/3DD`) but these are clearly distinguishable from array indices.  

[Example](tsv/latest/3DBackground.tsv) of a single entry in a dictionary with a `/Type` key:

<div style="font-family: monospace">

Key | Type | Required | ...
--- | --- | --- | ---
Type | name | TRUE | ...

</div>

Example of an [array requiring 3 floating-point numbers](tsv/latest/ArrayOf_3RGBNumbers.tsv), such as RGB values:

<div style="font-family: monospace">

Key | Type | ...
--- | --- | ---
0 | number | ...
1 | number | ...
2 | number | ...

</div>

Example of an array with unlimited number of elements of the same type [ArrayOfThreads](tsv/latest/ArrayOfThreads.tsv):

<div style="font-family: monospace">

Key | Type | ... |Link
--- | --- | --- | ---
\* | dictionary | ... | \[[Thread](tsv/latest/Thread.tsv)]

</div>

Dictionaries or arrays can also serve as maps, where an arbitrary name is associated with another definition. Examples include [ClassMap](tsv/latest/ClassMap.tsv) or the Shading dictionary in the Resources dictionary. In such cases the Shading key is linked to ShadingMap and [ShadingMap](tsv/latest/ShadingMap.tsv) looks like this:

<div style="font-family: monospace">

Key |  Type | ... | Link
--- | --- | --- | ---
\* | dictionary;stream | ... | \[[ShadingType1](tsv/latest/ShadingType1.tsv),[ShadingType2](tsv/latest/ShadingType2),[ShadingType3](tsv/latest/ShadingType3.tsv)];\[[ShadingType4](tsv/latest/ShadingType4.tsv),[ShadingType5](tsv/latest/ShadingType5.tsv),[ShadingType6](tsv/latest/ShadingType6.tsv),[ShadingType7](tsv/latest/ShadingType7.tsv)]

</div>

### Type

PDF 2.0 formally defines 9 basic types of object, but within the specification other types are commonly referred to. Therefore the Arlington PDF Model uses the following extended set of pre-defined types (case sensitive, alphabetically sorted, SEMI-COLON (`;`) separated):

- `array`
- `bitmask`
- `boolean`
- `date`
- `dictionary`
- `integer`
- `matrix`
- `name`
- `name-tree`
- `null`
- `number`
- `number-tree`
- `rectangle`
- `stream`
- `string`
- `string-ascii`
- `string-byte`
- `string-text`

A single key in a dictionary can often be of different types. A common example is when a key is either a dictionary or an array of dictionaries. In this case "**Type**" would be defined as `array;dictionary`. Types are always stored in alphabetical order in the 2nd field using SEMI-COLON (`;`) separators. In addition, version-based predicates can occur indicating when new data types were added or removed.

These Linux commands lists all combinations of the Arlington types used in PDF:

```bash
cut -f 2 * | sort | uniq
cut -f 2 * | sed -e 's/;/\n/g' | sort | uniq
```

### SinceVersion

Field 3 defines the PDF version when the relevant key or array element was introduced, as described in ISO 32000-2:2020. All TSV rows must have a valid (non-empty) "SinceVersion" entry. Valid values are PDF versions: `1.0`, `1.1`, ..., `1.7`, or `2.0`.

### DeprecatedIn

Field 4 defines the PDF version when the relevant key or array element was deprecated, as described in ISO 32000-2:2020. If the key/array element is still valid in PDF 2.0, then "DeprecatedIn" will be empty.  Valid values are PDF versions: `1.0`, `1.1`, ..., `1.7`, or `2.0`.

### Required

Field 5 is effectively a Boolean field (`TRUE`/`FALSE`) but may also contain a `fn:IsRequired(...)` predicate. Examples include:

- when a key changes from optional to required in a particular PDF version then the expression `fn:IsRequired(fn:SinceVersion(x.y))` is used.

- if a key/array entry is conditional based on the value of another key then an expression such as `fn:IsRequired(@Filter!=JPXDecode)` can be used. The `@` syntax means "value of a key/array index".

- if a key/array entry is conditional based on the presence or absence of another key then the nested expressions `fn:IsRequired(fn:IsPresent(OtherKeyName))` or `fn:IsRequired(fn:NotPresent(OtherKeyName))` can be used.


### PossibleValues

Field 6 "PossibleValues" follows the same pattern as "Links":

<div style="font-family: monospace">

Type | ... | PossibleValues | ...
--- | --- | --- | ---
integer;string | ... | \[1,3,99];\[(Hello),(World)] | ...

</div>

Often times it is necessary to use a predicate (`fn:`) for situations when values are valid.


### SpecialCase

A set of predicates is used to define more advanced kinds of relationships. Every predicate is always prefixed with `fn:`. Current predicates include:

```
fn:ArrayLength
fn:BeforeVersion
fn:BitSet
fn:BitsClear
fn:BitsSet
fn:Deprecated
fn:Expr
fn:ImageIsStructContentItem
fn:ImplementationDependent
fn:IsAssociatedFile
fn:IsMeaningful
fn:IsPDFTagged
fn:IsPDFVersion
fn:IsPresent
fn:IsRequired
fn:KeyNameIsColorant
fn:MustBeDirect
fn:NoCycle
fn:NotPresent
fn:PageContainsStructContentItems
fn:RequiredValue
fn:SinceVersion
fn:StringLength
```

### Link

If a specific key or array element requires further definition (i.e. represents another dictionary, stream or array) the key is linked to another TSV via the "Link" field. It is the name of another TSV file without any file extension. Links are always encapsulated in `[` and `]`.

Example in [PageObject](tsv/latest/PageObject.tsv):

<div style="font-family: monospace">

Key | Type | ... | Link
--- | --- | --- | ---
Resources  | dictionary | ... | \[[Resource](tsv/latest/Resource.tsv)]

</div>

If "Key" is represented by different types we use following pattern with SEMI-COLON ";" separators:

<div style="font-family: monospace">

Type | ... | Link
--- | --- | ---
array<b>;</b>dictionary | ... | \[ValidateArray];\[ValidateDictionary]

</div>

Another common example is that one dictionary could be based on few different dictionaries. For example an Annotation might be Popup, Stamp, etc. In such cases the TSV filenames are separated with COMMA (",") separators like this:

<div style="font-family: monospace">

Type | ... | Link
--- | --- | ---
array;dictionary | ... | \[[ArrayOfAnnots](tsv/latest/ArrayOfAnnots.tsv)];\[[AnnotStamp](tsv/latest/AnnotStamp.tsv),[AnnotRedact](tsv/latest/AnnotRedact.tsv),[AnnotPopup](tsv/latest/AnnotPopup.tsv)]

</div>

Links may also be wrapped in the `fn:Deprecated` or `fn:SinceVersion` predicates if a specific type of PDF object has been deprecated or introduced in specific PDF versions.

---

# Proof of Concept Implementations

All PoCs are command line based with built-in help if no command line arguments are provided. Command line options for all Python scripts and TestGrammar C++ PoC are aligned to keep things simple.

## Python scripts

The scripts folder contains several Python3 scripts and an example Jupyter Notebook which are all cross-platform (tested on both Windows and Linux). See [scripts/README.md](scripts/README.md).

## TestGrammar (C++17)

A CLI utility that can validate an Arlington grammar (set of TSV files), compare the Arlington PDF Model to the Adobe DVA definition, or perform a detailed validation check of PDF files against the Arlington PDF Model. All documentation is now located in [TestGrammar/README.md](TestGrammar/README.md).

## GCXML (Java)

A CLI Java-based proof of concept application that can convert the main Arlington TSV file set (in `./tsv/latest`) into PDF version specific file sets in both TSV and XML formats. The XML format is defined by [this schema](xml/schema/objects.xsd). In addition, some research oriented queries can be performed using the XML as input. Detailed documentation is now located in [gcxml/README.md](gcxml/README.md).

The Java gcxml.jar file must be run in this top-level folder (such that `./tsv/` and `./xml/` are both sub-folders).

## 3D and VR visualizations

The latest Arlington PDF Models for each PDF version can be visualized in 3D or using VR goggles at [https://safedocs.pdfa.org](https://safedocs.pdfa.org).


## Linux commands

Basic Linux commands can be used on an Arlington TSV data set (`cut`, `grep`, `sed`, etc.), however field (column) numbering needs to be remembered and screen display can be messed up unless you have a wide monitor and small fonts. Alternative more specialized utilities such as the [EBay TSV-Utilities](https://github.com/eBay/tsv-utils) or [GNU datamash](https://www.gnu.org/software/datamash/) can also be used.

```bash
# Ensure sorting is consistent...
export LC_ALL=C

# If you have a wide terminal, this helps with TSV display from cat, etc.
tabs 1,20,37,50,64,73,91,103,118,140,158,175,190,210,230

# Change directory to a specific PDF version or "latest"
cd ./tsv/latest

# Confirm consistent field headers across all TSV files
head -qn1 * | sort | uniq | sed -e 's/\t/\\t/g'
# Correct response: Key\tType\tSinceVersion\tDeprecatedIn\tRequired\tIndirectReference\tInheritable\tDefaultValue\tPossibleValues\tSpecialCase\tLink\tNote

# Confirm the Type field
cut -f 2 * | grep -v "fn:" | sort | uniq
# Correct response: each line only has Types listed above, separated by semi-colons, sorted alphabetically.
cut -f 2 * | grep -v "fn:" | sed -e 's/;/\n/g' | sort | uniq
# Correct response: Type, array, boolean, date, dictionary, integer, name, name-tree, nll, number,
#                   number-tree, rectangle, stream, string, string-ascii, string-byte, string-text
#                   and version-based predicates (fn:SinceVersion, fn:Deprecated, etc).

# Confirm all "SinceVersion" values
cut -f 3 * | sort | uniq
# Correct response: pdf-version values 1.0, ..., 2.0, fn:SinceVersion, etc. predicates No blank lines.

# Confirm all "DeprecatedIn" values
cut -f 4 * | sort | uniq
# Correct response: pdf-version values 1.0, ..., 2.0, fn:DeprecatedIn. Blank lines OK.

# Confirm all "Required" values (TRUE, FALSE or fn:IsRequired predicate)
cut -f 5 * | sort | uniq
# Correct response: TRUE, FALSE, Required, fn:IsRequired(...). No blank lines.

# Confirm all "IndirectReference" values (TRUE, FALSE or fn:MustBeDirect() predicate)
cut -f 6 * | sort | uniq
# Correct response: TRUE, FALSE, IndirectReference or a fn:MustBeDirect() predicate. No blank lines.

# Field 7 is "Inheritable" (TRUE or FALSE)
cut -f 7 * | sort | uniq
# Correct response: TRUE, FALSE, Inheritable.

# Field 8 is "DefaultValue"
cut -f 8 * | sort | uniq

# Field 9 is "PossibleValues"
cut -f 9 * | sort | uniq
# Responses should all be inside '[' .. ']', separated by semi-colons if more than one. Empty sets '[]' OK.

# Field 10: List all "SpecialCases"
cut -f 10 * | sort | uniq

# Field 11: Sets of "Link" to other TSV objects
cut -f 11 * | sort | uniq
# Responses should all be inside '[' .. ']', separated by semi-colons if more than one. Empty sets '[]' OK.

# All "Notes" from field 12 (free form text)
cut -f 12 * | sort | uniq

# Set of all unique custom predicates (starting "fn:")
grep -ho "fn:[a-zA-Z]*" * | sort | uniq

# Custom predicates with context
grep -Pho "fn:[^\t]*" * | sort | uniq

# Unique set of key names (case-sensitive strings), array indices (0-based integers) or '*' for dictionary or array maps
cut -f 1 * | sort | uniq
```

Example of a GNU `datamash check` command that can confirm that all TSV files in an Arlington data set have the correct number of fields:

```bash
for f in *.tsv; do echo -n "$f, " ; datamash --headers check 12 fields < $f || break; done
```

If the monolithic single TSV file `pandas.tsv` created by [scripts/arlington-to-pandas.py] is used (being a merge of all individual TSV files in an Arlington file set, with a left-most field object name being added), then GNU datamash can also be used to get some basic statistics with reference to field titles:

```bash
# Count the number of unique PDF objects and keys in an Arlington PDF Model
datamash --headers --sort countunique Object < pandas.tsv
datamash --headers --sort countunique Key < pandas.tsv

# Count number of keys / array elements in each object
datamash --headers --sort groupby 1 count 1 < pandas.tsv

# Count the number of new keys / array elements introduced in each PDF version
datamash --headers --sort groupBy SinceVersion count SinceVersion < pandas.tsv

# Count the number of keys / array elements that got deprecated in each PDF version
datamash --headers --sort groupBy DeprecatedIn count DeprecatedIn < pandas.tsv

# For each PDF object, when was it first introduced and when was something last added to it
datamash -H --round=1 --group Object min SinceVersion max SinceVersion < pandas.tsv
```

Examples of the more powerful [EBay TSV-Utilities](https://github.com/eBay/tsv-utils) commands. Note that Linux shell requires the use of backslash to stop shell expansion. These commands can use TSV field names:

```bash
# Find all keys that are only 'string-byte'
tsv-filter -H --str-eq Type:string-byte *.tsv

# Find all keys that are only 'string-byte' but introduced in PDF 1.5 or later
tsv-filter -H --str-eq Type:string-byte --ge SinceVersion:1.5 *.tsv

# Find all keys that can be any type of string
tsv-filter -H --regex Type:string\* --ge SinceVersion:1.5 *.tsv
```

## Publications

* "[_Demystifying PDF through a machine-readable definition_](https://langsec.org/spw21/papers.html#pdfReadable)"; Peter Wyatt, LangSec Workshop at IEEE Security & Privacy, May 27th and 28th, 2021 \[[Paper](https://github.com/gangtan/LangSec-papers-and-slides/raw/main/langsec21/papers/Wyatt_LangSec21.pdf)] \[[Talk Video](https://www.youtube.com/watch?v=c1Lxf-JMcH4)]

* "[_The Arlington PDF Model_](PDF-Days-2021-Arlington-PDF-model.pdf)" \[presentation], Peter Wyatt, PDF Asssociation's "PDF Days 2021" online event, Tuesday 28 Sept 2021.

---

Copyright 2021-22 PDF Association, Inc. https://www.pdfa.org

This material is based upon work supported by the Defense Advanced Research Projects Agency (DARPA) under Contract No. HR001119C0079. Any opinions, findings and conclusions or recommendations expressed in this material are those of the author(s) and do not necessarily reflect the views of the Defense Advanced Research Projects Agency (DARPA). Approved for public release.
