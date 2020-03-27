# PDF 2.0 Grammar (sort of)

We extracted all tables from PDF 2.0 and represent them in series of CSV files (generated from spreadsheet, using LibreOffice Calc). Each CVS file basically represents specific dictionary in PDF with allowed keys.

## Basic types
ISO32000-2 defines few basic types, but from inside of the spec we refer "some other" types as well. Therefore we work with following basic types:

- "ARRAY"
- "BOOLEAN"
- "DATE"
- "DICTIONARY"
- "INTEGER"
- "NAME"
- "NAME TREE"
- "NUMBER"
- "NUMBER TREE"
- "RECTANGLE"
- "STREAM"
- "STRING"

Whenever we find other type in the spec(e.g. BYTE STRING) we change that to basic (STRING in this example) and record the decision. For more information [click here](Grammar_vs_ISO32000-2.md).

## CSV structure
All csv files are unified. First line contains heading with identifiers of columns.
Each key can be of specific type. If more types are allowed (eg. array of dictionaries or dictionary) they are seprated by ";".

Extracted information are as follows:
- Key		- key
- Type	- one of basic types or types separated by ";"
- SinceVersion	- version of PDF this key was introduced in
- DeprecatedIn	- version of PDF this key was deprecated in
- Required		- TRUE/FALSE  
- IndirectRefrence	- TRUE/FALSE
- RequiredValue	- the only possible value
- DefaultValue	- depends on the type
- PossibleValues	- list of possible values separated by ";"
- SpecialCase	 	- expression
- Link			- name of another csv file for validating "this" container

# Tool / Validator
This repository contains a commandline tool based on the PDFix library that allows two different tasks:
- validates CSV files. Check the uniformity (number of columns), if all types are one of basic types etc..
- validates PDF file. Starting from Catalog, the tool validates:
	- if required keys are present
	- if values are of proper type
	- if objects are indirect if required
	- if value is correct if PossibleValues are defined
