# Internal grammar  
This file describes the internal grammar used in the PDF DOM spreadsheet and corresponding TSV files to describe internal relationships in and between PDF objects. The declarative grammar is based on strongly-typed functions which are structured such that basic processing can be done using regex. To see the list of all functions [**click here**](##Functions).

[[_TOC_]]

## Usage
Functions can occur in various columns in the spreadsheet under vaious constraints:
- *column SinceVersion*: functions define a **pdf_version** constant declaring the first PDF specification in which this key/array entry was defined.
- *column DeprecatedIn*: functions define a **pdf_version** constant declaring the PDF specification when this key/array entry was no longer defined, or explicitly described as deprecated or obsoleted.
- *column Required*: functions define the logical condition (TRUE) when a key/array entry is required 
- *column IndirectReference*: functions define the logical condition (TRUE) when a key/array entry is required to be an indirect reference
- column *PossibleValues*: specific values can be conditional based on function results
- column *SpecialCase*: defines additional constraints related to the key/array entry
- *column Link*: individual link entries can be conditional based on function results

## Constants
**pdf_version** - represents a specific PDF version according to a PDF specification document. 
- The values `1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7` and `2.0` represent the PDF version according to ISO 32000-2:2020. 
- The values "Adobe-1.0", "Adobe-1.2", "Adobe-1.3", "Adobe-1.4", "Adobe-1.5", "Adobe-1.6" and "Adobe-1.7" represent the PDF version according to the Adobe specification and any published errata.
- The value "Adobe-DVA" represents PDF as defined by the Adobe DVA (ISO 32000-1:2008) data file shipped with Acrobat.
- The value "reserved" means a future PDF version that is yet to be formally specified.
- The prefix "ISO-" followed by a number represents an ISO publication.
- The prefix "Extn-" followed by a string represents a PDF extension identified by the string.

## Variables
**value** - a predefined variable name and refers to the value of the current key or array entry. This is redudant if the **@** syntax is used with the current key/array index. e.g. `[value>0]`

**@** - prefix used to indicate a key name or array index number (0-based). e.g. `@Type, @Subtype, @1`

## Sets and Ranges 
**[ a, b, c ]** - represents a set of the values a, b and c which can be of any typed. The `[` and `]` are overloaded with the syntax used for Links!!! ** Could switch to purely declarative form: #Set(a,b,c)??? **

**< min, max >** - represents an inclusive range of values from min to max. ** Could switch to declarative form: #Range(min,max)??? **

## Logical operations
**or**, **and**, **not** - keywords for logical operators

## Functions
This section describes each function used in the internal grammar. Each function uses prefix (**TBD**), has a predefined name and strongly typed parameters.

### IsVersion(pdf_version)
**Syntax:** `IsVersion(pdf_version) -> bool`   
- *pdf_version* - should be any valid PDF version or set of versions (see above)

**Description:**  
Used as "if condition" to check if the version is equal to the given parameter. Used in *Required* column.

**Example:**  
- Type 1 Font Name key: "(Required in PDF 1.0; optional in PDF 1.1 through 1.7, deprecated in PDF 2.0)":
```
Key: Name
SinceVersion: 1.0
DeprecatedIn: 2.0
Required: #IsVersion(1.0)
```

### RequiredValue(condition, value)
**Syntax:** `RequiredValue(condition, value)`
- *condition* - an expression
- *value* - appropriately typed value that is required when *condition* is true

**Description:**  
Used as "if condition" to check if the version is equal to the given parameter.  

**Example:** 
Table 21 - Additional encryption entries for the standard security handler (EncryptionStandard.tsv), `R` key, value `6`, *PossibleValues* column:
```
[ #Deprecated(2.0, #RequiredValue(@V<2,        2) ),
  #Deprecated(2.0, #RequiredValue(@V in [2,3], 3) ),
  #Deprecated(2.0, #RequiredValue(@V==4,       4) ),
  #Deprecated(2.0,                             5),
  #SinceVersion(2.0, #RequiredValue(@V==5,     6) ) 
]
```

### SinceVersion(pdf_version, value)  
**Syntax:** `SinceVersion(pdf_version, value)`  
- *pdf_version* - should be any of the valid PDF versions (see above).  
- *value* - appropriately typed value that was introduced in **pdf_version**  

**Description:**  
This function overrides value in *SinceVersion* column. Needs to be later than the value in *SinceVersion* column and less than the value in the *DeprecatedIn* column.  

**Example:**  
- Table 21 - Additional encryption entries for the standard security handler (EncryptionStandard.tsv), `R` key, value `6`, *PossibleValues* column:
```
[ #Deprecated(2.0, #RequiredValue(@V<2,        2) ),
  #Deprecated(2.0, #RequiredValue(@V in [2,3], 3) ),
  #Deprecated(2.0, #RequiredValue(@V==4,       4) ),
  #Deprecated(2.0,                             5),
  #SinceVersion(2.0, #RequiredValue(@V==5,     6) ) 
]
```  

- Annotation Dictionaries - Table 166 for `Subtype` key, *PossibleValues* column: 
```
[ Text, Link,
$SinceVersion(1.3, FreeText),
$SinceVersion(1.3, Line),
$SinceVersion(1.3, Square),
$SinceVersion(1.3, Circle),
$SinceVersion(1.5, Polygon),
$SinceVersion(1.5, PolyLine),
$SinceVersion(1.3, Highlight),
$SinceVersion(1.3, Underline),
$SinceVersion(1.4, Squiggly),
$SinceVersion(1.3, StrikeOut),
$SinceVersion(1.5, Caret),
$SinceVersion(1.3, Stamp),
$SinceVersion(1.3, Ink),
$SinceVersion(1.3, Popup),
$SinceVersion(1.3, FileAttachment),
$Deprecated(2.0, $SinceVersion(1.2, Sound)),
$Deprecated(2.0, $SinceVersion(1.2, Movie)),
$SinceVersion(1.5, Screen),
$SinceVersion(1.2, Widget),
$SinceVersion(1.4, PrinterMark),
$Deprecated(2.0, $SinceVersion(1.3, TrapNet)),
$SinceVersion(1.6, Watermark),
$SinceVersion(1.6, 3D),
$SinceVersion(1.7, Redact),
$SinceVersion(2.0, Projection),
$SinceVersion(2.0, RichMedia) 
]
```

### Deprecated(version-range, value)
**Syntax:** `Deprecated(pdf_version, value)`  
- *pdf_version* - should be any of the valid PDF versions (see above).  
- *value* - appropriately typed value that was deprecated or obsoleted in **pdf_version**   

**Description**

**Example:**
- Table 21 - Additional encryption entries for the standard security handler (EncryptionStandard.tsv), `R` key, value `6`, *PossibleValues* column:
```
[ #Deprecated(2.0, #RequiredValue(@V<2,        2) ),
  #Deprecated(2.0, #RequiredValue(@V in [2,3], 3) ),
  #Deprecated(2.0, #RequiredValue(@V==4,       4) ),
  #Deprecated(2.0,                             5),
  #SinceVersion(2.0, #RequiredValue(@V==5,     6) ) 
]
```  

- Blend Modes (BM) in External Graphics State was introduced in PDF 1.4 and allowed name or an array of names (ArrayOfBlendModes.tsv), but in PDF 2.0 the array option (only) was deprecated. Also the BM name "Compatible" was deprecated in PDF 2.0:
```
Key:  BM
Type: #Deprecated(2.0, array);name
SinceVersion: 1.4
PossibleValues: [];[Normal,#Deprecated(2.0, Compatible),Multiply,Screen,Darken,Lighten,ColorDodge,ColorBurn,HardLight,SoftLight,Overlay,Difference,Exclusion,Hue,Saturation,Color,Luminosity]
Link: [ #Deprecated(2.0, ArrayOfBlendModes) ];[]
```

### IsStandard14Font()
**Syntax:** `IsStandard14Font()`  

**Description**
Returns TRUE if ... xxx ... one of the standard 14 PDF fonts

**Example:**
(**TBD**)


### LengthIf(cond, len)
**Syntax:** `IsStandard14Font()`  

**Description**
Returns TRUE if ... xxx ... one of the standard 14 PDF fonts

**Example:**
Table 21 - Additional encryption entries for the standard security handler (EncryptionStandard.tsv), `O` and `U` keys of type 'string-byte', *SpecialCase* column:
```
#LengthIf(@R==4, 32) or #LengthIf(@R==6, 48)
```  

### MulitpleOf(number)

**Syntax:** `MulitpleOf(number)`  
- *number* - numeric value for which the key value or array entry must be a whole multiple of   

**Description**
only in PossibleValues column??

**Example:**
- Page /Rotate key: #MulitpleOf(90)
- Various bit depths for encryption: #MultipleOf(8) - need to also combine with a min and max value??? 
- Table 21 - Additional encryption entries for the standard security handler (EncryptionStandard.tsv), `Length` key, *PossibleValues* column:
```
#MutipleOf(8,40,128)
<40,128> and #MultipleOf(8)
```

### NotPresent(condition)

**Syntax:** `NotPresent(condition)`  

**Description**
Indicates that a key or array entry must NOT be present under the specified condition.
only in Required column????

**Example:**
(**TBD**)

### nColorants(cs-obj)
**Syntax:** `nColorants(cs-obj)`  

**Description**
Returns the integer number of colorants for a specified PDF object - can be 1 (Gray), 3 (RGB or Lab), 4 (CMYK). 

**Example:**
(**TBD**)

### ValidWhen(condition)
**Syntax:** `ValidWhen(condition)`  
- *condition* - a condition for when the current key/array entry is valid

**Description**
Condition for when the current key/array entry is valid.

**Example:**
- Table 21 - Additional encryption entries for the standard security handler (EncryptionStandard.tsv), `Length` key, *SpecialCase* column:
```
#ValidWhen(@V in [2,3])
```


# TODO
- what prefix to use
$ - does not work (agreed)
\# - does not work in LO Calc  ** I'm not having any issues??? What issues are you having? **
[] - already used in spreadsheets - I think we can use this as a kind of "set" syntax since we already use it as this 
() - used for functions params  (agreed)
? -  maybe?  
Can we use a key word as prefix?? Like for example fn? `fn:isVersion(1.5)`

- return value  
In syntax I have used `-> bool` just to show that value that comes out of the function is boolean. But this is unnecessary.  
- key is not allowed  
This is not covered in our grammar. Since we only use true/false values. It should be a part of Required column  
- $Deprecated(version-range, obj)  
version-range?? Why do we need a range? Is there ever a case where key was deprecated and then re-introduced in later versions?? ** Not sure if required or not - but should consider in case it is **
