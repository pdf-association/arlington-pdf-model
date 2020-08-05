# Internal grammar  
This file describes the internal grammar used in spreadsheets. Grammar is based on functions. To see the list of all functions [**click here**](##Functions).
## Functions
This section describes each function used in grammar files. Each function uses prefix (**TBD**), has a predefined name and set of parameters.

### isVersion
**Syntax:** `isVersion(pdf_version) -> bool`   
- *pdf_version* - should be any valid PDF version  

**Description:**  
Used as "if condition" to check if the version is equal to the given parameter.  
**Example:**  
Here we might provide a real example from spreadsheet, where this function is being used.  
### sinceVersion  
**Syntax:** `sinceVersion(pdf_version, value)`  
- *pdf_version* - should be any valid PDF version  
- *value* - value that was introduced in pdf_version  

**Description:**  
This function overrides value in "since version" column. Should be later than the original value.  
**Example:**  
todo


## TODO
- what prefix to use
$ - does not work
\# - does not work in LO Calc  
[] - already used in spreadsheets  
() - used for functions params  
? -  maybe?  
Can we use a key word as prefix?? Like for example fn? `fn:isVersion(1.5)`

- return value  
In syntax I have used `-> bool` just to show that value that comes out of the function is boolean. But this is unnecessary.  
- key is not allowed  
This is not covered in our grammar. Since we only use true/false values. It should be a part of Required column  
- $Deprecated(version-range, obj)  
version-range?? Why do we need a range? Is there ever a case where key was deprecated and then re-introduced in later versions??
