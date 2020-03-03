# PDF 2.0 Grammar (sort of)

We extracted all tables from PDF 2.0 and represent them in series of CSV files (generated from excel) Each CSV file basically represents specific dictionary in PDF with allowed keys. 

## Basic types
ISO32000-2 defines few basic types, but from inside of the spec we refer "some other" types as well. Therefore we work with following basic types
"ARRAY", "BOOLEAN", "DATE", "DICTIONARY", "INTEGER", "NAME", "NUMBER", "RECTANGLE", "STREAM", "STRING"

whenever we find other type in the spec(e.g. BYTE STRING) we change that to basic (STRING in this example) and record the decision. 

## CSV structure
All csv files are unified. First line contains heading with identifiers of columns. 
Each key can be of specific type. If more types are allowed (eg. array of dictionaries or dictionary) they are seprated by ";"

Extracted information are as follows:
Key		- key 
Type	- one of basic types or types separated by ";"
SinceVersion	- version of PDF this key was introduced in
DeprecatedIn	- version of PDF this key was deprecated in
Required		- TRUE/FALSE  
IndirectRefrence	- TRUE/FALSE
RequiredValue	- TRUE/FALSE
DefaultValue	- depends on the type 
PossibleValues	- list of possible values separated by ";"
SpecialCase	 	- expression 
Link			- name of another csv file for validating "this" container

# Tool / Validator
This repository contains a commandline tool based on the PDFix library that allows two different tasks

## validates CSV files
Checks the uniformity of all csv files
	- number of columns
	- if all types are one of basic types
	- if all links to other csv files do exist
	- if all csv files are reachable from Catalog.csv

## validates PDF file
Starting from Catalog, the tool validates 
	- if required keys are present
	- if values are of proper type
	- if objects are indirect if required
	- if value is correct if PossibleValues are defined

# Grammar vs. ISO 32000-2


# Licensing
?????



