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

The Arlington PDF Model is a specification-derived, machine-readable definition of the full PDF document object model (DOM) as defined by the PDF 2.0 specification [ISO 32000-2:2020](https://www.iso.org/standard/75839.html) and its [related resolved errata](https://www.pdfa.org/pdf-issues/). It provides an easy-to-process structured definition of all formally defined PDF objects (dictionaries, arrays and map objects) and their relationships beginning with the file trailer using a simple text-based syntax and a small set of declarative functions. The Arlington PDF Model is applicable to both parsers (PDF readers) and unparsers (PDF writers).

The Arlington PDF Model currently does not define:

* PDF lexical rules,
* PDF content streams (operators and operands),
* Linearization rules,
* rules for file structure including incremental updates
* rules for cross reference table data.

The Arlington definition does **not** replace the PDF specification and must always be used in conjunction with the PDF 2.0 document in order to fully understand the PDF DOM.

Each Table from the latest PDF 2.0 dated revision (ISO 32000-2:2020) specification is represented by a tabbed separated values (TSV) file. Each TSV represents a single PDF object (either a dictionary, array, stream, map) and contains all necessary data to validate real-world PDF files. Sets of TSV files are also provided for each earlier PDF version - these are derived from the "latest" TSV file set.

Text-based TSV files are easy to view, either in a spreadsheet application or directly here in Github. They are trivial to process using simple Linux commands (`cut`, `grep`, `sed`, etc.) or with more specialized utilities such as the [EBay TSV-Utilities](https://github.com/eBay/tsv-utils) or [GNU datamash](https://www.gnu.org/software/datamash/). Scripting languages like Python also natively support TSV via:

```python
import csv
tsvreader = csv.DictReader(file, delimiter='\t')
```

**The Arlington PDF model is still under active development via the DARPA-funded SafeDocs fundamental research program!**

## TSV Data Overview

Each row defines a specific key in a dictionary or an array element in an array. All the characteristics captured in the TSV for that key/array element are defined in the PDF 2.0 specification.

- All PDF names are always expressed **without** the leading FORWARD-SLASH (`/`).
- PDF strings are required to have `'` and `'` (single quotes).
- PDF array objects must also have `[` and `]` and, of course, do **not** use commas between array elements.
- PDF Boolean (keywords) are always lowercase `true` and `false`.
- Logical Boolean values related to the description of PDF objects in the Arlington data model are always uppercase `TRUE`/`FALSE`.
- TSV column names (a.k.a. fields) are shown double-quoted (`"`) in documentation to hopefully avoid confusion.

TSV columns are always in the following order and tabs must exist for all columns:

1. "[**Key**](#Key)" - key in dictionary, or integer index into an array. ASTERISK (`*`) represents a wildcard and means any key/index.
1. "[**Type**](#Type)" - one or more of the pre-defined Arlington types separated by SEMI-COLONs `;`.
1. "[**SinceVersion**](#SinceVersion-and-DeprecatedIn)" - version of PDF this key was introduced in.
1. "[**DeprecatedIn**](#SinceVersion-and-DeprecatedIn)" - version of PDF this key was deprecated in. Empty if not deprecated.
1. "[**Required**](#Required)" - whether the key or array element is required.
1. **IndirectReference**" - whether the key is required to be an indirect reference (`TRUE`/`FALSE`) or if it must be a direct object (predicate `fn:MustBeDirect(...)`).
1. **Inheritable**" - whether the key is inheritable (`TRUE`/`FALSE`).
1. **DefaultValue**" - optional default value of key.
1. "[**PossibleValues**](#PossibleValues)" - list of possible values. For dictionary `/Type` keys that must have a specific value, this will be a choice of just 1.
1. "[**SpecialCase**](#SpecialCase)" - declarative functions defining additional relationships.
1. "[**Link**](#Link)" - name(s) of other TSV files for validating the values of this key for dictionaries, arrays and streams.
1. "**Notes**" - free text for arbitrary notes.

The two special objects [\_UniversalArray](tsv/latest/_UniversalArray.tsv) and [\_UniversalDictionary](tsv/latest/_UniversalDictionary.tsv) are not formally defined in the PDF 2.0 specification and represent generic an arbitrarily-sized PDF array and PDF dictionary respectively. They are used to resolve "Links" to generic objects for a few PDF objects.

## TSV Field Summary

A very precise definition of all syntax rules for the Arlington PDF model as well as Python equivalent data structure descriptions and useful Linux commands is in [INTERNAL_GRAMMAR.md](INTERNAL_GRAMMAR.md). Only a _simplified summary_ is provided below to get started. Note that not all TSV columns (fields) are shown in the examples below. And normally "Links" are not conveniently hyperlinked to actual TSV files either!


### Key

"Key" represents a single key in a dictionary, an index into an array, or multiple entries (`*`). Dictionary keys are obviously case sensitive and array indices are integers. To locate a key easily using Linux begin a regex with the start-of-line (`^`). For a precise match end the regex with a TAB (`\t`). Conveniently, ISO 32000 only uses ASCII characters for 1st class key names so there are no #-escapes used in Arlington. ISO 32000 also does not define any dictionary key that is purely just an integer - Arlington leverages this fact so that array "keys" are always 0-based integers. Note that ISO 32000 does define some keys that start with integers (e.g. `/3DD`) but these are clearly distinguishable from array indices.  

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

Dictionaries can also serve as maps, where an arbitrary name is associated with another element. Examples include ClassMap or the Shading dictionary in the Resources dictionary. In such cases the Shading key is linked to ShadingMap and [ShadingMap](tsv/latest/ShadingMap.tsv) looks like this:

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

A single key in a dictionary can often be of different types. A common example is when a key is either a dictionary or an array of dictionaries. In this case "**Type**" would be defined as `array;dictionary`. Types are always stored in alphabetical order in the 2nd column using SEMI-COLON (`;`) separators.

These Linux commands lists all combinations of the Arlington types used in PDF:

```bash
cut -f 2 *.tsv | sort | uniq
cut -f 2 *.tsv | sed -e 's/;/\n/g' | sort | uniq
```

### SinceVersion and DeprecatedIn

These fields define the PDF versions when the relevant key or array element was introduced or optionally deprecated, as described in ISO 32000-2:2020.
All TSV rows must have a valid (non-empty) "SinceVersion" entry. If keys are still valid in PDF 2.0, then "DeprecatedIn" will be empty.

### Required

This is effectively a Boolean field (`TRUE`/`FALSE`) but may also contain a `fn:IsRequired(...)` predicate. Examples include:

- when a key changes from optional to required in a particular PDF version then the expression `fn:IsRequired(fn:SinceVersion(x.y))` is used.

- if a key/array entry is conditional based on the value of another key then an expression such as `fn:IsRequired(@Filter!=JPXDecode)` can be used. The `@` syntax means "value of a key/array index".

- if a key/array entry is conditional based on the presence or absence of another key then the nested expressions `fn:IsRequired(fn:IsPresent(OtherKeyName))` or `fn:IsRequired(fn:NotPresent(OtherKeyName))` can be used.


### PossibleValues

"PossibleValues" follows the same pattern as "Links":

<div style="font-family: monospace">

Type | ... | PossibleValues | ...
--- | --- | --- | ---
integer;string | ... | \[1,3,99];\[(Hello),(World)] | ...

</div>

Often times it is necessary to use a predicate (`fn:`) for situations when values are valid.


### SpecialCase

A set of predicates is used to define more advanced kinds of relationships. Every predicate is always prefixed with `fn:`. Current predicates include (_subject to change!_:

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

If a specific key or array element requires further definition (i.e. represents another dictionary, stream or array) the key is linked to another TSV via the "Link" column. It is the name of another TSV file without any file extension. Links are always encapsulated in `[` and `]`.

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

All PoCs are command line based with builtin help if no command line arguments are provided.

## TestGrammar (C++17)

A CLI utility that can validate an Arlington grammar (set of TSV files) and perform validation against PDF files.  
All documentation is now located in the [TestGrammar](TestGrammar/) folder

## GCXML (Java)

Java CLI utility that:

1. converts all TSV files into XML files that must be valid based on [schema](/xml/schema/objects.xsd)

1. gives answers to various researcher-type queries

To compile, run `ant` from [/gcxml](/gcxml) directory or use NetBeans. Output JAR is in [/gcxml/dist/gcxml.jar](/gcxml/dist/gcxml.jar).

#### Usage

To use gcxml tool run the following command from terminal/commandline in the top-level PDF20_Grammar folder (so that `./tsv/` is a sub-folder):

```
java -jar ./gcxml/dist/gcxml.jar

GENERAL:
    -version        print version information (current: 0.4.9)
    -help           show list of available commands
CONVERSIONS:
    -all            convert latest TSV to XML and TSV sub-versions for each specific PDF version
    -xml <version>      convert TSV to XML for specified PDF version (or all if no version is specified)
    -tsv            create TSV files for each PDF version
QUERIES:
    -sin <version | -all>   return all keys introduced in ("since") a specified PDF version (or all)
    -dep <version | -all>   return all keys deprecated in a specified PDF version (or all)
    -kc         return every key name and their occurrence counts for each version of PDF
    -po key<,key1,...>  return list of potential objects based on a set of given keys for each version of PDF
    -sc         list special cases for every PDF version
    -so         return objects that are not defined to have key Type, or where the Type key is specified as optional

```

**Note**: output might be too long to display in terminal, so it is recommended to always redirect the output to file.

The XML version of the PDF DOM grammar (one XML file per PDF version) is created from the TSV files and written to `./xml`. All of the answers to queries are based on processing the XML files in `./xml`.

## Python3 scripts

The script `arlington-to-pandas.py` will concatenate an entire Arlington TSV file set into a single large TSV file with an additional first column representing the Arlington TSV filename. This combined TSV is then suitable for processing in Jupyter Notebooks with pandas, etc.


The main Python3 PoC mega-script is [arlington.py](scripts/arlington.py). This script can:

1. convert an Arlington TSV data set into an in-memory Python representation and save out to JSON
1. perform a detailed validation of the Arlington Python representation, including attempted lexing of all predicates
1. save the Python representation as a 'pretty-print' which is more friendly for Linux commands, etc. but technically invalid JSON
1. validates a PDF file against the Arlington model using `pikepdf`

It relies on the [Python Sly parser](https://sly.readthedocs.io/en/latest/) and `pikepdf` [doco](pikepdf.readthedocs.io/) which is Python wrapper on top of [QPDF](https://github.com/qpdf/qpdf),

```bash
pip3 install sly pikepdf
```

## Linux commands

Basic Linux commands can be used on an Arlington TSV data set (`cut`, `grep`, `sed`, etc.), however column numbering needs to be remembered and screen display can be messed up unless you have a wide monitor and small fonts. Alternative more specialized utilities such as the [EBay TSV-Utilities](https://github.com/eBay/tsv-utils) or [GNU datamash](https://www.gnu.org/software/datamash/) can also be used.

```bash
## Ensure sorting is consistent...
export LC_ALL=C

## If you have a wide terminal, this helps with TSV display from cat, etc.
tabs 1,20,37,50,64,73,91,103,118,140,158,175,190,210,230

## Change directory to a specific PDF version or "latest"
cd ./tsv/latest

## Confirm consistent column headers across all TSV files
head -qn1 *.tsv | sort | uniq | sed -e 's/\t/\\t/g'
## Correct response: Key\tType\tSinceVersion\tDeprecatedIn\tRequired\tIndirectReference\tInheritable\tDefaultValue\tPossibleValues\tSpecialCase\tLink\tNote

## Find files with column issues - worth investigating in ODS in case of data in other rows...
grep -P "Link\t\t" *.tsv | sed -e 's/\t/\\t/g'
grep -P "Note\t\t" *.tsv | sed -e 's/\t/\\t/g'
# No response is correct!

## Confirm the Type column
cut -f 2 *.tsv | grep -v "fn:" | sort | uniq
# Correct response: each line only has Types listed above, separated by semi-colons, sorted alphabetically.
cut -f 2 *.tsv | grep -v "fn:" | sed -e 's/;/\n/g' | sort | uniq
# Correct response: Type, array, boolean, date, dictionary, fn:Deprecated(type,pdf-version), integer, name, name-tree,
#                   null, number, number-tree, rectangle, stream, string, string-ascii, string-byte, string-text

## Confirm all "SinceVersion" values
cut -f 3 *.tsv | sort | uniq
# Correct response: pdf-version values 1.0, ..., 2.0, SinceVersion. No blank lines.

## Confirm all "DeprecatedIn" values
cut -f 4 *.tsv | sort | uniq
# Correct response: pdf-version values 1.0, ..., 2.0, DeprecatedIn. Blank lines OK.

## Confirm all "Required" values (TRUE, FALSE or fn:IsRequired() function)
cut -f 5 *.tsv | sort | uniq
# Correct response: TRUE, FALSE, Required, fn:IsRequired(...). No blank lines.

## Confirm all "IndirectReference" values (TRUE, FALSE or fn:MustBeDirect() function)
cut -f 6 *.tsv | sort | uniq
# Correct response: TRUE, FALSE, IndirectReference or a fn:MustBeDirect() function. No blank lines.

## Column 7 is "Inheritable" (TRUE or FALSE)
cut -f 7 *.tsv | sort | uniq
# Correct response: TRUE, FALSE, Inheritable.

## Column 8 is "DefaultValue"
cut -f 8 *.tsv | sort | uniq

## Column 9 is "PossibleValues"
cut -f 9 *.tsv | sort | uniq
# Responses should all be inside '[' .. ']', separated by semi-colons if more than one. Empty sets '[]' OK if multiples.

## Column 10: List all "SpecialCases"
cut -f 10 *.tsv | sort | uniq

## Column 11: Sets of "Link" to other TSV objects
cut -f 11 *.tsv | sort | uniq
# Responses should all be inside '[' .. ']', separated by semi-colons if more than one. Empty sets '[]' OK if multiples.

## All "Notes" (free form text)
cut -f 12 *.tsv | sort | uniq

## Set of all unique custom function names (starting "fn:")
grep -ho "fn:[a-zA-Z]*" *.tsv | sort | uniq

## Custom functions in context
grep -Pho "fn:[^\t]*" *.tsv | sort | uniq

## Unique set of key names (case-sensitive strings), array indices (0-based integers) or '*' for dictionary or array maps
cut -f 1 *.tsv | sort | uniq
```

Examples of the more powerful [EBay TSV-Utilities](https://github.com/eBay/tsv-utils) commands. Note that Linux shell requires the use of backslash to stop shell expansion. The commands can use TSV column names:

```bash
## Find all keys that are only 'string-byte'
tsv-filter -H --str-eq Type:string-byte *.tsv

## Find all keys that are only 'string-byte' but introduced in PDF 1.5 or later
tsv-filter -H --str-eq Type:string-byte --ge SinceVersion:1.5 *.tsv

## Find all keys that can be any type of string
tsv-filter -H --regex Type:string\* --ge SinceVersion:1.5 *.tsv
```

---

# TODO

## gcxml utility
- confirm that the XML produced from the TSV data with formulas is still valid

## Python scripts
- confirm why some PDF objects from later PDF versions end up in earlier versions

---

Copyright 2021 PDF Association, Inc. https://www.pdfa.org

This material is based upon work supported by the Defense Advanced Research Projects Agency (DARPA) under Contract No. HR001119C0079. Any opinions, findings and conclusions or recommendations expressed in this material are those of the author(s) and do not necessarily reflect the views of the Defense Advanced Research Projects Agency (DARPA). Approved for public release.
