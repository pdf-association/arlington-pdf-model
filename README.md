# **PDF 2.0 Grammar (sort of)**

We extracted all Tables from PDF 2.0 dated revision (ISO/DIS 32000-2) and represent them in series of worksheets. Each worksheet represents either a dictionary, array, stream, etc. and contains all necessary data to validate real world PDF files.

Our main source is [PDF20Grammar.ods](PDF20Grammar.ods) which is a [LibreOffice Calc](https://www.libreoffice.org/) spreadsheet. There is a specific sheet **TableMap** that identifies each worksheet and then each sheet is a representation of a table in the PDF spec. Note that due to the very large number of worksheets, Microsoft Excel cannot be used. 

Columns must be in the following order:
1. **Key**		- key in dictionary, or index into an array. "\*" means any key / index.
1. **Type**		- one or more [type](#Type) or types separated by ";".
1. **SinceVersion**	- version of PDF this key was introduced in. Possible values are 1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7 or 2.0.
1. **DeprecatedIn**	- version of PDF this key was deprecated in. Blank if not deprecated. Possible values are 1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7 or 2.0.
1. **Required**	- whether the key or array element is required (TRUE/FALSE).  
1. **IndirectReference**	- whether the key is required to be an indirect reference (TRUE/FALSE).
1. **RequiredValue**	- the only possible value. e.g. value of Type key for some dictionaries.
1. **DefaultValue**	- default value as defined in PDF 2.0, depends on the type.
1. **PossibleValues**	- list of possible values.
1. **SpecialCase**	- expression (TODO: language is TBD - needs to include required direct object).
1. **Link**	- name(s) of other worksheet(s) for validating the value(s) of this key.
1. **Notes**	- free text for arbitrary notes.

Rows define specific keys in a dictionary or an element in an array and the characteristics for that key/array element.

All names are expressed **without** the leading FORWARD-SLASH (/).

## **Key**
Key represents a single key in a dictionary, an index in an array, or multiple entries.
Example of a single entry in the PageObject dictionary:  
- Key=Type	Type=name Required=TRUE RequiredValue=Page  

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
\* |	dictionary;stream |\[ShadingType1,ShadingType2,ShadingType3];\[ShadingType4,ShadingType5,ShadingType6,ShadingType7]


## **Type**
PDF 2.0 defines a few basic types, but within the spec we refer to some other types as well. Therefore we work with following basic types:
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

A single key in a dictionary can be of different types. A common examples is when a key is either a dictionary or an array of dictionaries. In this case Type would be defined as "array;dictionary". Types are stored in alphabetical order in the 2nd column

This Linux command lists all combinations of these types used throughout PDF:
```
cut -f 2 *.tsv | sort | uniq 
```

## **Link**
If a specific key requires further validation (e.g. represents another dictionary) we link this key to another sheet in Link column. Example in PageObject:  
- Key=Resources Type=dictionary Link=\[Dictionary]

If Key is represented by different types we use following pattern:  

Type | Link |
--- | --- |
array<b>;</b>dictionary |\[ValidateArray];\[ValidateDictionary]

Another common example is that one dictionary could be validated based on few different links (Annotation could either be Popup, Stamp etc.) In such case options would be separated with "," like this:

Type | Link |
--- | --- |
array;dictionary | \[ArrayOfAnnotations];\[AnnotStamp<b>,</b>AnnotRedact<b>,</b>AnnotPopup]

## **PossibleValues**
PossibleValues also follow the same pattern as Links:

Type | PossibleValues |
--- | --- |
array;dictionary | \[Value1ForType1,Value2ForType1];\[Value1ForType2,Value2ForType2]

Sometimes it's necessary to define formula to cover all possible values: TODO

## **SpecialCase**
TODO

---

# **Implementations**
This repository contains implementations

- TestGrammar (C++)	- test existing pdf file against grammar, validates grammar itself, compares grammar with Adobe grammar (TODO)
- gcxml (Java)			- generates xml files that conform schema and uses XPath to query grammar, generates specific reports


## **TestGrammar**
commandline tool based on the [PDFix library](https://pdfix.net/download-free/) that works with Grammar in form of TSV (tab separated values) files. The easiest way to generate tsv files from [PDF20Grammar.ods](PDF20Grammar.ods) is to use macro from the ods file (or use folder tsv that is synced with the ods file)

The tools allows two different tasks
1. validates TSV files. Check the uniformity (number of columns), if all types are one of basic types etc..
2. validates PDF file. Starting from Catalog, the tool validates:
	- if required keys are present
	- if values are of proper type
	- if objects are indirect if required
	- if value is correct if PossibleValues are defined
3. compares grammar with Adobe (TODO)

#### Usage (Windows):
Download binaries from [bin folder](/TestGrammar/bin) and run from command line:  
To validate single PDF file call:
-	TestGrammar_x64.exe \<input_file> \<grammar_folder> \<report_file>

	- input_file      - full pathname to input pdf   
	- grammar_folder  - folder with TSV files representing PDF 2.0 Grammar  
	- report_file     - file for storing results

TestGrammar_x64.exe "C:\Test grammar\test file.pdf" "C:\Grammar folder\tsv\" "c:\temp\test file.txt"


## **GCXML**
Command line tool writen in Java that does two different things:
1. converts all TSV files into XML files that must be valid based on [schema](/xml/schema/objects.xsd) (not the final version, yet)
2. gives answers to queries
 - https://docs.google.com/document/d/11wXQmITNiCFB26fWAdxEq4TGgQ4VPQh1Qwoz1PU4ikY

To compile, run "ant" from gcxml directory or use NetBeans. 

#### Usage
To use gcxml tool run the following command from terminal/commandline in the top-level PDF20_Grammar folder (so that ./tsv/ is a sub-folder):  
```
java -jar ./gcxml/dist/gcxml.jar <options>  
```
>*Note: output might be too long to display in terminal, it is recommended to redirect the output to file (eg \<command> > report.txt)*

To represent grammar in XML files (one file = one object), we convert TSV files into XML into folder ./xml/objects. To do that, use option:  
```
--conv -all
```
>*Note: 2nd argument '-all' can be replaced with object name (eg. Catalog) to convert only specific object*  

 All of the answers to queries are based on XML files. To get answers from grammar, use option:  
 ```
 --help
 ```
 this will list all possible options and description of what they do.
 
## Useful Linux commands
```
## Ensure sorting is consistent...
export LC_ALL=C

## Confirm column headers across all TSV files
head -qn1 *.tsv | sort | uniq | sed -e 's/\t/\\t/g'

## Find files with excessive columns to the right - worth investigating in ODS in case of data in other rows...
$ grep -P "Link\t\t" *.tsv | sed -e 's/\t/\\t/g'

## All "Notes"
cut -f 12- *.tsv | sort | uniq

## Set of all "Links"
cut -f 11 *.tsv | sort | uniq

## List all "SpecialCases"
cut -f 10 *.tsv | sort | uniq

## List all "PossibleValues"
cut -f 9 *.tsv | sort | uniq

## Unique set of key names (and array indices)
$ cut -f 1 *.tsv | sort | uniq
```

---

# TODO :pushpin:
- finish all unlinked dictionaries, streams and arrays. These are often represented by \[] in the current data.  
- identify Tables and objects in report (somehow identify table/paragraph etc. in the ISO/DIS 32000-2 and carry this information in grammar so we can report it back)
- define language for formulas in PossibleValues (currently we have following intervals: <0,100>, <-1.0,1.0>, <0,1>, <0,2>,	<0.0,1.0> and also some expressions: value\>=2, value\>=1, value\>=0, value\<0, value\>0 and also combinations: <40,128> value\*8,<40,128> value\*8, <40,128> value\*8, value\*90, value\*1/72
- define language for SpecialCase column
- TableMap is out of sync
- compare grammar with Adobe DVA
