# Internal grammar  
This file describes the internal grammar used in the PDF DOM TSV files to describe internal relationships in and between PDF objects. The declarative grammar is based on strongly-typed functions which are structured such that basic processing can be done using Linux commands and regex.

[[_TOC_]]

## Usage
Functions can occur in various columns in the spreadsheet under vaious constraints:
- *column SinceVersion*: functions define a **pdf_version** constant declaring the first PDF specification in which this key/array entry was defined.
- *column DeprecatedIn*: functions define a **pdf_version** constant declaring the PDF specification when this key/array entry was no longer defined, or explicitly described as deprecated or obsoleted.
- *column Required*: functions define the logical condition (TRUE) when a key/array entry is required.
- *column IndirectReference*: functions define the logical condition (TRUE) when a key/array entry is required to be an indirect reference or if a key must be a direct object.
- column *PossibleValues*: specific values can be conditional based on function results.
- column *SpecialCase*: defines additional constraints related to the key/array entry.
- *column Link*: individual link entries - can be conditional based on function results.

## Constants
**pdf_version** - represents a specific PDF version according to the PDF 2.0 specification (ISO/FDIS 32000-2:2020) document. 
- The values `1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7` and `2.0` represent the PDF version according to ISO 32000-2:2020.
In the future the following kinds of values may also need to be used:
- The values "Adobe-1.0", "Adobe-1.2", "Adobe-1.3", "Adobe-1.4", "Adobe-1.5", "Adobe-1.6" and "Adobe-1.7" represent the PDF version according to the Adobe specification and any published errata.
- The value "reserved" means a future PDF version that is yet to be formally specified.
- The prefix "ISO-" followed by a number represents an ISO publication.
- The prefix "Extn-" followed by a string represents a PDF extension identified by the string.

## Variables
**self** - a predefined variable name and refers to the current key or array entry.`

**@** - prefix used to indicate a key name or array index number (0-based). e.g. `@Subtype (key name), @0 (array index), @self`

## Sets and Ranges 
**[ a, b, c ]** - comma-separated list that represents a set of the values a, b and c which can be of any typed. 

## Mathematical and Logical operations
Mathematical and logical operations use C/C++ style symbology:
- **==** equality, **!=** inequality, **>** greater than, **<** less than, etc.
- **||** logical OR, **&&** logical

## Functions
Each declarative function used in the internal grammar uses a prefix (`fn:`), has a predefined case-sensitive name, and parameters.
Functions are commonly nested, with the inner function typically expressing the oldest and most basic requirement.
