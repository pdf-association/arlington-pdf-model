# **PDF 2.0 Grammar (sort of)**

We extracted all tables from PDF 2.0 and represent them in series of tables. Every table represents either dictionary,array stream etc.. and contains all necessary to validate real world pdf files.

Our main source is [PDF20Grammar.ods](PDF20Grammar.ods) file (we are using [LibreOffice Calc](libreoffice.org)). There is specific sheet **TableMap** that identifies each sheet in PDF 2.0 specification and then each sheet is representation of the table in PDF spec. Columns represent following information:
- Key				- key in dictionary, or index into an array
- Type				- one of [basic types]() or types separated by ";"
- SinceVersion		- version of PDF this key was introduced in
- DeprecatedIn		- version of PDF this key was deprecated in
- Required			- TRUE/FALSE  
- IndirectReference	- TRUE/FALSE
- RequiredValue		- the only possible value
- DefaultValue		- depends on the type
- PossibleValues	- list of possible values.
- SpecialCase	 	- expression
- Link				- name of another sheet for validating "this" container

Rows are specific keys in dictionary and characteristics for that key.
All the "questions/problems/inconsistences" found in the ISO32000-2 during the process are collected [here](Grammar_vs_ISO32000-2.md).

## **Key**
Key represents single key in dictionary, index in array or multiple entries.
Example of single entry in PageObject dictionary:  
- Key=Type	Type=name Required=TRUE RequiredValue=Page  

Example of an array with 3 numbers:   

Key | Type |   
--- | --- |      
0	| number |  
1	| number |   
2	| number |

Example of an array with unlimited number of elements of the same type.  
ArrayOfThreads:

Key | Type | Link |
--- | --- | --- |
\* | dictionary | [Thread]

We may also have a dictionary that serves as a map. Unknown name is associated with other element. Example of such construct is ClassMap or Shading dictionary in Resource dictionary. In such case Shading as a type of dictionary is linked to ShadingMap and ShadingMap looks like this:  

Key |  Type | Link |
--- | --- | --- |
\* |	dictionary;stream |[ShadingType1,ShadingType2,ShadingType3];\[ShadingType4,ShadingType5,ShadingType6,ShadingType7]


## **Type**
ISO32000-2 defines few basic types, but from inside of the spec we refer "some other" types as well. Therefore we work with following basic types:
- ARRAY
- BOOLEAN
- DATE
- DICTIONARY
- INTEGER
- NAME
- NAME TREE
- NULL
- NUMBER
- NUMBER TREE
- RECTANGLE
- STREAM
- STRING
- STRING-ASCII
- STRING-BYTE
- STRING-TEXT

One single key in dictionary might be of different types. Common example is that one key is either a dictionary or an array of dictionaries. In this case Type would be "array;dictionary"
Except of basic types, currently we recognize following combinations of allowed types: [click here](All_types.md)

## **Link**
If specific key requires further validation (represents another dictionary for example) we link this key to another sheet in Link column. Example in PageObject:  
- Key=Resources Type=dictionary Link=\[Dictionary]

If Key is represented by different types we use following pattern:  

Type | Link |
--- | --- |
array<b>;</b>dictionary |[ValidateArray]<b>;</b>[ValidateDictionary]

Another common example is that one dictionary could be validated based on few different links (Annotation could either be Popup, Stamp etc.) In such case options would be separated with "," like this:

Type | Link |
--- | --- |
array;dictionary | [ArrayOfAnnotations];[AnnotStamp<b>,</b>AnnotRedact<b>,</b>AnnotPopup]

## **PossibleValues**
PossibleValues also follow the same pattern as Links:

Type | PossibleValues |
--- | --- |
array;dictionary | [Value1ForType1,Value2ForType1];[Value1ForType2,Value2ForType2]

Sometimes it's necessary to define formula to cover all possible values: TODO

## **SpecialCase**
TODO

---

# **Implementations**
This repository contains implementations

- TestGrammar (C++)	- test existing pdf file against grammar, validates grammar itself, compares grammar with Adobe grammar
- gcxml (Java)			- generates xml files that conform schema and uses XPath to query grammar, generates specific reports


## **TestGrammar**
commandline tool based on the [PDFix library](https://pdfix.net/download-free/) that works with Grammar in form of tsv files. The easiest way to generate tsv files from [PDF20Grammar.ods](PDF20Grammar.ods) is to use macro from the ods file (or use folder tsv that is synced with the ods file)

The tools allows two different tasks
1. validates TSV files. Check the uniformity (number of columns), if all types are one of basic types etc..
2. validates PDF file. Starting from Catalog, the tool validates:
	- if required keys are present
	- if values are of proper type
	- if objects are indirect if required
	- if value is correct if PossibleValues are defined
3. compares grammar with Adobe (TODO)

#### Usage (Windows):
Download binaries from [bin folder](/TestGrammar/bin) and run from commandline:  
to validate single pdf file call:
-	TestGrammar_x64.exe \<input_file> \<grammar_folder> \<report_file>

	- input_file      - full pathname to input pdf"   
	- grammar_folder  - folder with tsv files representing PDF 2.0 Grammar"  
	- report_file     - file for storing results"

TestGrammar_x64.exe "C:\Test grammar\test file.pdf" "C:\Grammar folder\tsv\" "c:\temp\test file.txt"


## **GCXML**
command line tool writen in Java that does two different things:
1. converts all TSV files into XML files that must be valid based on [schema](/xml/schema/objects.xsd) (not the final version, yet)
2. gives answers to queries
 - https://docs.google.com/document/d/11wXQmITNiCFB26fWAdxEq4TGgQ4VPQh1Qwoz1PU4ikY

#### Usage
To use gcxml tool run the following command from terminal/commandline:  
```
java -jar gxml/dist/gcxml.jar <options>  
```
>*Note: outputs might be too long to display in terminal, it is recommended to redirect the output to file (eg \<command> > report.txt)*

To represent grammar in XML files (one file = one object), we convert TSV files into XML. To do that, use option:  
```
--conv -all
```
>*Note: 2nd argument '-all' can be replaced with object name (eg. Catalog) to convert only specific object*  

 All of the answers to queries are based on XML files. To get answers from grammar, use option:  
 ```
 --help
 ```
 this will list all possible options and description of what they do.

# TODO :pushpin:
- finish all unlinked dictionaries, streams and arrays  
- identify tables and objects in report (somehow identify table/paragraph etc. in the ISO32000-2 and carry this information in grammar so we can report it back)
- define language for formulas in PossibleValues (currently we have following intervals: <0,100>, <-1.0,1.0>, <0,1>, <0,2>,	<0.0,1.0> and also some expressions:value>=2, value>=1,value>=0,value>=0,value<0,value>0 and also combinations:<40,128> value*8,<40,128> value*8, <40,128> value*8 plus these: value*90, value*1/72
- define language for SpecialCase column
- TableMap is out of sync
- compare grammar with Adobe
