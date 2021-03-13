# **The Arlington PDF Model**

<img src="resources/ArlingtonPDFSymbol300x300.png" width="150">

## **Background**

The Arlington PDF Model is a specification-derived, machine-readable definition of the full PDF document object model (DOM) as defined by the PDF 2.0 specification [ISO 32000-2:2020](https://www.iso.org/standard/75839.html) and its [related resolved errata](https://www.pdfa.org/pdf-issues/). It provides an easy-to-process structured definition of all formally defined PDF objects (dictionaries, arrays and map objects) and their relationships beginning with the file trailer using a simple text-based syntax and a small set of declarative functions. The Arlington PDF Model is applicable to both parsers (PDF readers) and unparsers (PDF writers).

The Arlington PDF Model currently does not define PDF lexical rules, PDF content streams (operators and operands), Linearization data, incremental updates or cross reference table data. The Arlington definition does **not** replace the PDF specification and must always be used in conjunction with the PDF 2.0 document in order to fully understand the PDF DOM.

Each Table from the latest PDF 2.0 dated revision (ISO 32000-2:2020) specification is represented by a tabbed separated values (TSV) file. Each TSV represents a single PDF object (either a dictionary, array, stream, map) and contains all necessary data to validate real-world PDF files. Sets of TSV files are also provided for each earlier PDF version - these are derived from the "latest" TSV file set.

Text-based TSV files are easy to view, either in a spreadsheet application or directly here in Github. They are trivial to process using simple Linux commands (`cut`, `grep`, `sed`, etc.) or with more specialized utilities such as the [EBay TSV-Utilities](https://github.com/eBay/tsv-utils) or [GNU datamash](https://www.gnu.org/software/datamash/). Scripting languages like Python also natively support TSV via:

```
import csv
...
tsvreader = csv.DictReader(file, delimiter='\t')
```

**This model is still under active development via the DARPA-funded SafeDocs fundamental research program!**

## **TSV Data Overview**

Each row defines a specific key in a dictionary or an array element in an array. All the characteristics captured in the TSV for that key/array element are defined in the PDF 2.0 specification.

All PDF names are always expressed **without** the leading FORWARD-SLASH (`/`).
PDF strings are required to have `(` and `)`.
PDF array objects must also have `[` and `]` and, of course do **not** use commas between array elements.

TSV columns must always be in the following order and tabs for all columns must always be present:
1. [**Key**](#Key) - key in dictionary, or integer index into an array. ASTERISK (`*`) represents a wildcard and means any key/index.
1. [**Type**](#Type) - one or more Arlington types separated by SEMI-COLON `;`.
1. [**SinceVersion**](#SinceVersion and DeprecatedIn) - version of PDF this key was introduced in.
1. [**DeprecatedIn**](#SinceVersion and DeprecatedIn) - version of PDF this key was deprecated in. Empty if not deprecated.
1. [**Required**](#Required) - whether the key or array element is required (TRUE/FALSE).
1. **IndirectReference** - whether the key is required to be an indirect reference (TRUE/FALSE) or if it must be a direct object (`fn:MustBeDirect`).
1. **Inheritable** - whether the key is inheritable (TRUE/FALSE).
1. **DefaultValue** - optional default value of key.
1. [**PossibleValues**](#PossibleValues) - list of possible values. For dictionary `/Type` keys that must have a specific value, this will be a choice of just 1.
1. [**SpecialCase**](#SpecialCase)  - declarative functions defining additional relationships.
1. [**Link**](#Link) - name(s) of other TSV files for validating the values of this key for dictionaries, arrays and streams.
1. **Notes** - free text for arbitrary notes.

The two special objects \_UniversalArray and \_UniversalDictionary are not formally defined in the PDF 2.0 specification and represent generic an arbitrarily-sized PDF array and PDF dictionary respectively. They are used to resolve "Links" to generic objects for a few PDF objects.

## **Data Summary**


### **Key**

Key represents a single key in a dictionary, an index into an array, or multiple entries (`*`). Keys are obviously case sensitive. To locate a key easily using Linux begin a regex with the start-of-line (`^`).

Example of a single entry in a dictionary with a `/Type` key:

Key | Type | Required | PossibleValues | ... |
--- | --- | --- | --- | --- |
Type | name | TRUE | \[Page] | ... |

Example of an array requiring 3 floating-point numbers, such as an RGB value:

Key | Type | ... |
--- | --- | --- |
0 | number | ... |
1 | number | ... |
2 | number | ... |

Example of an array with unlimited number of elements of the same type: ArrayOfThreads:

Key | Type | ... |Link |
--- | --- | --- | --- |
\* | dictionary | ... | \[Thread] |

Dictionaries can also serve as maps, where an arbitrary name is associated with another element. Examples include ClassMap or the Shading dictionary in the Resources dictionary. In such case Shading as a type of dictionary is linked to ShadingMap and ShadingMap looks like this:

Key |  Type | ... | Link |
--- | --- | --- | --- |
\* | dictionary;stream | ... | \[ShadingType1,ShadingType2,ShadingType3];\[ShadingType4,ShadingType5,ShadingType6,ShadingType7]


### **Type**

PDF 2.0 formally defines 9 basic types of object, but within the specification other types are commonly referred to. Therefore the Arlington PDF Model uses the following extended set of types (case sensitive, alphabetically sorted, SEMI-COLON (`;`) separated):

- array
- bitmask
- boolean
- date
- dictionary
- integer
- matrix
- name
- name-tree
- null
- number
- number-tree
- rectangle
- stream
- string
- string-ascii
- string-byte
- string-text

A single key in a dictionary can often be of different types. A common example is when a key is either a dictionary or an array of dictionaries. In this case **Type** would be defined as "array;dictionary". Types are always stored in alphabetical order in the 2nd column using SEMI-COLON (`;`) separators.

These Linux commands lists all combinations of "Types" used in PDF:
```
cut -f 2 *.tsv | sort | uniq
cut -f 2 *.tsv | sed -e 's/;/\n/g' | sort | uniq
```

### **SinceVersion** and **DeprecatedIn**

These fields define the PDF versions when the relevant key or array element was introduced or optionally deprecated, as described in ISO 32000-2:2020.
All TSV rows must have a valid (non-empty) "SinceVersion" entry. If keys are still valid in PDF 2.0, then "DeprecatedIn" will be empty.

### **Required**

This is effectively a boolean field (TRUE/FALSE) but may also contain a `fn:IsRequired(...)` declarative function. Examples include:

- when a key changes from optional to required in a particualr PDF version then the expression `fn:IsRequired(fn:SinceVersion(x.y))` is used.

- if a key/array entry is conditional based on the value of another key then an expression such as `fn:IsRequired(@Filter!=JPXDecode)` can be used. The `@` syntax means "value of key/array index ..."

- if a key/array entry is conditional based on the presence or absence of another key then the nested expressions `fn:IsRequired(fn:IsPresent(OtherKeyName))` or `fn:IsRequired(fn:NotPresent(OtherKeyName))` can be used.

### **PossibleValues**

PossibleValues also follow the same pattern as Links:

Type | PossibleValues |
--- | --- |
array;dictionary | \[Value1ForType1,Value2ForType1];\[Value1ForType2,Value2ForType2]

Often times it is necessary to define a formula (fn:...) to define when values are valid.

### **SpecialCase**

A set of declarative function is used to define more advanced kinds of relationships. Every function is always prefixed with `fn:`. Current functions in use include:

```
fn:BeforeVersion
fn:BitSet
fn:BitsClear
fn:BitsSet
fn:CreatedFromNamePageObj
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

### **Link**

If a specific key or array element requires further definition (i.e. represents another dictionary, stream or array) the key is linked to another TSV via the "Link" column. It is the name of another TSV file without any file extension. Links are always encapsulated in `[` and `]`.

Example in PageObject:
Key | Type | ... | Link |
--- | --- | --- | --- |
Resources  | dictionary | ... | \[Resource] |


If "Key" is represented by different types we use following pattern with SEMI-COLON ";" separators:

Type | ... | Link |
--- | --- |
array<b>;</b>dictionary | ... | \[ValidateArray];\[ValidateDictionary]

Another common example is that one dictionary could be based on few different dictionaries. For example an Annotation might be Popup, Stamp, etc. In such cases the TSV filenames are separated with COMMA (",") separators like this:

Type | ... | Link |
--- | --- | --- |
array;dictionary | ... | \[ArrayOfAnnotations];\[AnnotStamp<b>,</b>AnnotRedact<b>,</b>AnnotPopup]

Links may also use the `fn:Deprecated` or `fn:SinceVersion` declarative functions if a specific type of PDF object has been deprecated or introduced in specific PDF versions.

---

# **Proof of Concept Implementations**

This repository contains the following Proof-of-Concept implementations:

- TestGrammar (C++17)   - test existing pdf file against grammar, validates grammar itself, compares grammar with Adobe DVA grammar [TODO](#todo-pushpin)
- gcxml (Java)          - generates xml files that conform to a schema and uses XPath to query grammar, generates specific reports.
- Python scripts - generates a single JSON file of the PDF DOM as well as a 3D/VR visualization (also JSON based) from the TSV files

## **TestGrammar** (C++)

Command line tool based on the free [PDFix SDK Lite](https://pdfix.net/download-free/) that uses a set of Arlington TSV files.

The tool implements various tasks:

1. validates all TSV files.
    - Check the uniformity (number of columns), if all types are one of Arlington types, etc.
2. validates a PDF file. Starting from Trailer, the tool validates:
    - if all required keys are present
    - if values are of correct type
    - if objects are indirect if required
    - if value is correct if PossibleValues are defined
    - all error messages are prefixed with "Error:" to enable post-processing
3. recursively validates a folder containing PDF files.
    - for PDFs with duplicate filenames, an underscore is appended to the report filename to avoid overwriting.
4. compares grammar with Adobe DVA

### Notes

* TestGrammar PoC does NOT currently confirm PDF version validity

* all error messages from validating PDF files are prefixed with "Error:"

* possible error messages from PDF file validation are as follows. Each error message also provides some context (e.g. a PDF object number):
```
Error: EXCEPTION ...
Error: Failed to open ...
Error: failed to acquire Trailer ...
Error: Can't select any link from ...
Error: not indirect ...
Error: wrong type ...
Error: wrong value ...
Error: required key doesn't exist ...
Error: object validated in two different contexts ...
```

* the Arlington definition is based on PDF 2.0 where some previously optional keys are now required (e.g. font dictionary **FirstChar**, **LastChar**, **Widths**) which means that matching legacy PDFs will fail unless these keys are present. A warning such as the following will be issued (because PDF 2.0 required keys are not present in legacy PDFs so there is no precise match):

```
Error: Can't select any link from \[FontType1,FontTrueType,FontMultipleMaster,FontType3,FontType0,FontCIDType0,FontCIDType2\] to validate provided object: xxx
```

### Building

#### Windows

Open [/TestGrammar/platform/msvc2019/TestGrammar.sln](/TestGrammar/platform/msvc2019/TestGrammar.sln) with Microsoft Visual Studio 2019 and compile.
Valid configurations are: 32 or 64 bit, Debug or Release.
Compiled executables will be in [/TestGrammar/bin/x64](/TestGrammar/bin/x64) (64 bit) and [/TestGrammar/bin/x86](/TestGrammar/bin/x86) (32 bit).

#### Linux

Note that due to C++17, gcc v8 or later is required. CMake is also required.

```
sudo apt install g++-8
sudo apt install gcc-8
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-8 800 --slave /usr/bin/g++ g++ /usr/bin/g++-8
cd TestGrammar
cmake .
make
```

Compiled binaries will be in [/TestGrammar/bin/linux](/TestGrammar/bin/linux).

#### Mac OS/X

T.B.D. - try Linux instructions???


## **GCXML** (Java)

Command line tool writen in Java that does two different things:

1. converts all TSV files into XML files that must be valid based on [schema](/xml/schema/objects.xsd)

1. gives answers to various queries
 - https://docs.google.com/document/d/11wXQmITNiCFB26fWAdxEq4TGgQ4VPQh1Qwoz1PU4ikY

To compile, run "ant" from [/gcxml](/gcxml) directory or use NetBeans. Output JAR is in [/gcxml/dist/gcxml.jar](/gcxml/dist/gcxml.jar).

#### Usage

To use gcxml tool run the following command from terminal/commandline in the top-level PDF20_Grammar folder (so that ./tsv/ is a sub-folder):

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

**Note**: output might be too long to display in terminal, so it is recommended to redirect the output to file (eg \<command> > report.txt)*

The XML version of the PDF DOM grammar (one XML file per PDF version) is created from the TSV files and written to ./xml. All of the answers to queries are based on processing the XML files in ./xml.

## **Python scripts**



## **Linux commands**

Basic Linux commands can be used on an Arlington TSV data set (`cut`, `grep`, `sed`, etc.). Alternative more specialized utilities such as the [EBay TSV-Utilities](https://github.com/eBay/tsv-utils) or [GNU datamash](https://www.gnu.org/software/datamash/) can be used.

```
## Ensure sorting is consistent...
export LC_ALL=C

## If you have a wide terminal, this helps with TSV display from grep, etc.
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

---

# TODO :pushpin:


## TestGrammar C++ utility
- when validating the TSV data files, also do a validation on the declarative function expressions
- when validating a PDF file, check required values in parent dictionaries when inheritance is allowed.
- extend TestGrammar with new feature to report all keys that are NOT defined in any PDF specification (as this may indicate either proprietary extensions, undocumented legacy extensions or common errors/malformations from PDF writers).

## gcxml utility
- confirm that the XML produced from the TSV data with formulas is still valid

## Python scripts
- make sure the pure JSON produced with formulas is still valid
- confirm why some PDF objects from later PDF versions end up in earlier versions

---

Copyright 2021 PDF Association, Inc. https://www.pdfa.org

This material is based upon work supported by the Defense Advanced Research Projects Agency (DARPA) under Contract No. HR001119C0079. Any opinions, findings and conclusions or recommendations expressed in this material are those of the author(s) and do not necessarily reflect the views of the Defense Advanced Research Projects Agency (DARPA). Approved for public release.
