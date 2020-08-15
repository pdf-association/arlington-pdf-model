# **PDF 2.0 DOM Grammar**

We extracted all Tables from the latest PDF 2.0 dated revision (ISO/FDIS 32000-2) specification and represent them in series of worksheets. Each worksheet represents a single PDF object - either a dictionary, array, stream, map, etc. and contains necessary data to validate real world PDF files.

Our main source is [PDF20Grammar.ods](PDF20Grammar.ods) which is a [LibreOffice Calc](https://www.libreoffice.org/) spreadsheet. There is a specific worksheet **!TableMap** that identifies each worksheet and then each worksheet is the representation of a PDF object from the PDF spec (most often mapping back to a Table in the PDF spec.) Note that due to the very large number of worksheets (\>490!), Microsoft Excel cannot be used. 

Columns must be in the following order:
1. **Key** - key in dictionary, or index into an array. "\*" means any key / index.
1. **Type**	- one or more [type](#Type) or types separated by ";".
1. **SinceVersion**	- version of PDF this key was introduced in. Possible values are 1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7 or 2.0.
1. **DeprecatedIn**	- version of PDF this key was deprecated in. Blank if not deprecated. Possible values are 1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7 or 2.0.
1. **Required**	- whether the key or array element is required (TRUE/FALSE).  
1. **IndirectReference** - whether the key is required to be an indirect reference (TRUE/FALSE).
1. **Inheritable** - whether the key is inheritable (TRUE/FALSE).
1. **DefaultValue** - default value of key.
1. **PossibleValues** - list of possible values. For Type keys of dictionaries which must have a specific value, this will be a choice of just 1.
1. **SpecialCase**	- expression [(TODO: grammar is TBD - needs to include required direct object)](#todo-pushpin).
1. **Link**	- name(s) of other worksheet(s) for validating the value(s) of this key.
1. **Notes** - free text for arbitrary notes.

Rows define specific keys in a dictionary or an element in an array and the characteristics for that key/array element.

All names are expressed **without** the leading FORWARD-SLASH (/).

The two special objects \_UniversalArray and \_UniversalDictionary are not defined in the PDF spec and represent a generic PDF array object and generic PDF dictionary object respectively. They are used to resolve Links for a few PDF objects under special circumstances. 

## **Key**
Key represents a single key in a dictionary, an index in an array, or multiple entries.
Example of a single entry in the PageObject dictionary:  
Key | Type | Required | PossibleValues |
--- | --- | --- | --- |
Type | name | TRUE | \[Page] |

Example of an array with 3 numbers, such as an RGB value:   

Key | Type |
--- | --- |
0 | number |
1 | number |
2 | number |

Example of an array with unlimited number of elements of the same type.  
ArrayOfThreads:

Key | Type | Link |
--- | --- | --- |
\* | dictionary | \[Thread]

We may also have a dictionary that serves as a map, where an arbitrary name is associated with another element. Examples of such constructs are ClassMap or the Shading dictionary in the Resources dictionary. In such case Shading as a type of dictionary is linked to ShadingMap and ShadingMap looks like this:  

Key |  Type | Link |
--- | --- | --- |
\* | dictionary;stream |\[ShadingType1,ShadingType2,ShadingType3];\[ShadingType4,ShadingType5,ShadingType6,ShadingType7]


## **Type**
PDF 2.0 defines a few basic types, but within the spec other types are specified. Therefore we work with following basic types:
- array
- boolean
- date
- dictionary
- integer
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

A single key in a dictionary can often be of different types. A common example is when a key is either a dictionary or an array of dictionaries. In this case **Type** would be defined as "array;dictionary". Types are always stored in alphabetical order in the 2nd column.

This Linux command lists all combinations of these types used throughout PDF:
```
cut -f 2 *.tsv | sort | uniq 
```

## **Link**
If a specific key requires further validation (e.g. represents another dictionary or array) we link this key to another worksheet via the Link column. Example in PageObject:  
Key | Type | Link |
--- | --- | --- |
Resources  | dictionary | \[Resource] |


If Key is represented by different types we use following pattern with SEMI-COLON ";" separators:  

Type | Link |
--- | --- |
array<b>;</b>dictionary |\[ValidateArray];\[ValidateDictionary]

Another common example is that one dictionary could be validated based on few different links (Annotation could either be Popup, Stamp etc.) In such case options would be separated with a COMMA "," separator like this:

Type | Link |
--- | --- |
array;dictionary | \[ArrayOfAnnotations];\[AnnotStamp<b>,</b>AnnotRedact<b>,</b>AnnotPopup]

## **PossibleValues**
PossibleValues also follow the same pattern as Links:

Type | PossibleValues |
--- | --- |
array;dictionary | \[Value1ForType1,Value2ForType1];\[Value1ForType2,Value2ForType2]

Sometimes it's necessary to define formula to cover all possible values: [TODO](#todo-pushpin)

## **SpecialCase**
[TODO](#todo-pushpin)

---

# **Implementations**
This repository contains the following Proof-of-Concept implementations:

- TestGrammar (C++17)	- test existing pdf file against grammar, validates grammar itself, compares grammar with Adobe DVA grammar [TODO](#todo-pushpin)
- gcxml (Java)			- generates xml files that conform to a schema and uses XPath to query grammar, generates specific reports.
- Linux CLI	- basic data validation and some simple analysis can be achieved by using various Linux commands on the TSV data
- Python script	- generates a single JSON file of the PDF DOM as well as a 3D/VR visualization (also JSON based) from the TSV files

## **Exporting to TSV**
In LibreOffice Calc, go Tools | Run Macro.. then pick from PDF20Grammar.ods | Standard | Module the macro called "ExportToTSV". This will write out all TSV files into a folder tree called "./tsv/latest" from where the PDF20Grammar.ods is stored. Existing TSV files will be overwritten! 

Note that the gcxml utility below can additionally generate TSV files for each specific PDF version into **./tsv/<version>/**.

## **TestGrammar**
Command line tool based on the free [PDFix library](https://pdfix.net/download-free/) that works with Grammar TSV files. See above for how to export to TSV.

The tool allows two different tasks
1. validates all TSV files.
	- Check the uniformity (number of columns), if all types are one of basic types etc..
2. validates a PDF file. Starting from Trailer, the tool validates:
	- if all required keys are present
	- if values are of correct type
	- if objects are indirect if required
	- if value is correct if PossibleValues are defined
3. compares grammar with Adobe DVA [TODO](#todo-pushpin)

Notes: 
* the utility does NOT currently confirm version validity

* the grammar is based on PDF 2.0 where some previously optional keys are now required (e.g. font dictionary FirstChar, LastChar, Widths) which means that matching legacy PDFs will fail unless these keys are present. A warning such as the following will be issued: 
```
Error: Can't select any link from \[FontType1,FontTrueType,FontMultipleMaster,FontType3,FontType0,FontCIDType0,FontCIDType2\] to validate provided object: xxx
```

#### Building

##### Windows
Open [/TestGrammar/platform/msvc2019/TestGrammar.sln](/TestGrammar/platform/msvc2019/TestGrammar.sln) with Microsoft Visual Studio 2019 and compile.
Valid configurations are: 32 or 64 bit, Debug or Release.
Compiled executables will be in [/TestGrammar/bin/x64](/TestGrammar/bin/x64) (64 bit) and [/TestGrammar/bin/x86](/TestGrammar/bin/x86) (32 bit).

##### Linux
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

#### Usage (Windows): 
To validate single PDF file call:
-	TestGrammar.exe \<input_file> \<grammar_folder> \<report_file>

	- input_file      - full pathname to input pdf   
	- grammar_folder  - folder with TSV files representing PDF 2.0 Grammar  
	- report_file     - text file for storing results

```
TestGrammar.exe "C:\Test grammar\test file.pdf" "C:\Grammar folder\tsv\" "c:\temp\test file.txt"
```

## **GCXML**
Command line tool writen in Java that does two different things:
1. converts all TSV files into XML files that must be valid based on [schema](/xml/schema/objects.xsd) 
1. gives answers to various queries
 - https://docs.google.com/document/d/11wXQmITNiCFB26fWAdxEq4TGgQ4VPQh1Qwoz1PU4ikY

To compile, run "ant" from [/gcxml](/gcxml) directory or use NetBeans. Output JAR is in [/gcxml/dist/gcxml.jar](/gcxml/dist/gcxml.jar).

#### Usage
To use gcxml tool run the following command from terminal/commandline in the top-level PDF20_Grammar folder (so that ./tsv/ is a sub-folder):  
```
java -jar ./gcxml/dist/gcxml.jar --help  

List of available commands:
        --version : print version information
        --help : show list of available commands
        --all  : convert latest TSV to XML and TSV for each specific PDF version
        --conv version : convert TSV to XML for specified PDF version
        --tsv  : create TSV files for each PDF version
        --sin [ version | -all ] : return all keys introduced in ("since") a specified PDF version (or -all)
        --dep [ version | -all ] : return all keys deprecated in a specified PDF version (or -all)
        --sc   : list special cases for every PDF version
        --so   : return objects that are not defined to have key Type, or where the Type key is specified as optional
        --keys : return every key name and their occurrence counts for each version of PDF
        --po key[,key1,...]  : return list of potential objects based on a set of given keys for each version of PDF
```
>*Note: output might be too long to display in terminal, so it is recommended to redirect the output to file (eg \<command> > report.txt)*

The XML version of the PDF DOM grammar (one XML file per PDF version) is created from the TSV files and written to ./xml/objects. All of the answers to queries are based on processing the XML files in ./xml/objects. 
 
## Useful Linux commands
```
## Ensure sorting is consistent...
export LC_ALL=C

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
cut -f 2 *.tsv | sort | uniq 
# Correct response: each line only has Types listed above, separated by semi-colons, sorted alphabetically. No blank lines.

## Confirm all "SinceVersion" values 
cut -f 3 *.tsv | sort | uniq
# Correct response: values 1.0, ..., 2.0, SinceVersion. No blank lines.

## Confirm all "DeprecatedIn" values
cut -f 4 *.tsv | sort | uniq
# Correct response: values 1.0, ..., 2.0, DeprecatedIn. Blank lines OK.

## Confirm all "Required" values
cut -f 5 *.tsv | sort | uniq
# Correct response: TRUE, FALSE, Required. No blank lines.

## Confirm all "IndirectReference" values (TRUE or FALSE)
cut -f 6 *.tsv | sort | uniq
# Correct response: TRUE, FALSE, IndirectReference. No blank lines.

## Column 7 is "Inheritable"
cut -f 7 *.tsv | sort | uniq
# Correct response: TRUE, FALSE, Inheritable. No blank lines.

## Column 8 is "DefaultValue"
cut -f 8 *.tsv | sort | uniq

## Column 9 is "PossibleValues"
cut -f 9 *.tsv | sort | uniq
# Responses should all be inside '[' .. ']', separated by semi-colons if more than one. Empty sets '[]' OK if multiples. 
# Includes some very basic expressions using 'value' as a reserved keyword. Blank lines OK.

## Column 10: List all "SpecialCases"
cut -f 10 *.tsv | sort | uniq
# WORK-IN-PROGRESS. IGNORE!!

## Column 11: Sets of "Link" to other TSV objects
cut -f 11 *.tsv | sort | uniq
# Responses should all be inside '[' .. ']', separated by semi-colons if more than one. Empty sets '[]' OK if multiples.

## All "Notes" (free form text)
cut -f 12 *.tsv | sort | uniq

## Unique set of key names (case-sensitive strings), array indices (0-based integers) or '*' for dictionary or array maps
cut -f 1 *.tsv | sort | uniq
```

---

# TODO :pushpin:

## Grammar
- define language for formulas in PossibleValues (currently we have following intervals: <0,100>, <-1.0,1.0>, <0,1>, <0,2>,	<0.0,1.0> and also some expressions: value\>=2, value\>=1, value\>=0, value\<0, value\>0 and also combinations: <40,128> value\*8,<40,128> value\*8, <40,128> value\*8, value\*90, value\*1/72
- define language for SpecialCase column, needs to include required direct object, inheritance, special conditions etc.

## Tools
- when validating pdf file, check required values in parent dictionaries when inheritance is allowed. 
- compare grammar with Adobe DVA. 
- update 3D/VR to support each PDF version 
