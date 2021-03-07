# Arlington TSV Grammar Validation Rules

This document describes some strict rules for the Arlington PDF model, for both the data and the
custom declarative functions. Only some of these rules are currently implemented, but everything
is precisely documented here


## TSV file rules


*   They are TSV, not CSV.
*   No quotes (single or double quotes) are required
*   Every TSV file needs to have the same identical header row as first line in file
*   EOL rules depend on platform/Git options -
    *   If you use LF then you can also use the [Ebay TSV utilities](https://github.com/eBay/tsv-utils)
    *   [GNU datamash](https://www.gnu.org/software/datamash/) can also be used
*   Every TSV file needs to have the full set of TABS (for all columns)
*   Last row in TSV needs EOL after last TAB
*   TSV file names are case sensitive
*   TSV file extensions are always `.tsv` (lowercase)


## PDF Object conventions


*   There are NO leading SLASHES for PDF names (_ever_!)
*   PDF names might use `#`-escaping
    *   the PDF specification never specifies any such keys so this is allowing for future-proofing
*   PDF strings always use `(` and `)`
*   PDF arrays always use `[` and `]` (which requires some additional processing so as not to be confused
    with our [];[];[] syntax for multiple types)
*   Expressions with integers need to use integers
*   Leading `@` indicates "value of" a key or array element
*   PDF booleans are `true` and `false` lowercase.
    *   Uppercase `TRUE`/`FALSE` are reserved for boolean TSV data fields such as the Required field.


## Column 1 - Key Name


*   Must not be blank
*   Case sensitive (as per PDF spec)
*   No duplicates keys in any single TSV file
*   Only alphanumeric, `.`, `-`, `_` or ASTERISK characters (no whitespace or other special chars)
    *   PDF names might use `#`-escaping in the future (but the PDF specification never specifies any such
        keys so this is allowing for future-proofing)
*   If dictionary, then Key name must start with alpha (and with no spaces) or just an ASTERISK `*`
*   If ASTERISK `*` by itself then must be last row in TSV file
*   If ASTERISK `*` by itself then Required column must be FALSE
*   If expressing a PDF array, then "Key" name is an integer array index. Zero-based increasing (always by 1)
    integers always starting at ZERO (0), with an optional ASTERISK appended after the digit (indicating repeat)
     - or just an ASTERISK `*`
*   If expressing a PDF array with `integer+ASTERISK` and then all rows must be `integer+ASTERISK` (indicating a
    repeating group of _N_ (numbered 0 to _N_-1) array elements).
*   If expressing a PDF array with `integer+ASTERISK` (and all rows are the same) then the Required column should
    be TRUE if all _N_ entries must always be repeated as a full set.
*   In the future:
    *   Key names with `#`-escapes
    *   _How should we define malforms???_ e.g. /type vs /Type; /SubType vs /Subtype; /BlackIs1 (uppercase i) vs
        /Blackls1 (lowercase L). Are these separate rows in a TSV, a "SpecialCase" column or wrapped in a declarative
       function in the Key column? (e.g. `Type;fn:Malform(type, ...)`). Need to consider impact on Linux CLI processing, such as grep.
*   **Python pretty-print/JSON**
    *   String (as JSON dictionary key)
*   **Linux CLI tests:**
    *   `cut -f 1 *.tsv | sort | uniq`
    *   `cut -f 1 *.tsv | sort | uniq | grep "\*"`


## Column 2 - Type


*   Must not be blank
*   Alphabetically sorted, SEMI-COLON separated list from the following predefined set of types (always lowercase):
    * `array`
    * `bitmask`
    * `boolean`
    * `date`
    * `dictionary`
    * `integer`
    * `matrix`
    * `name`
    * `name-tree`
    * `null`
    * `number`
    * `number-tree`
    * `rectangle`
    * `stream`
    * `string`
    * `string-ascii`
    * `string-byte`
    * `string-text`
*   Each type may also be wrapped in a version-based declarative function (e.g. `fn:SinceVersion(version, type)` or
    `fn:Deprecated(version, type)` ).
*   When a declarative function is used, the internal simple type is still kept in its alphabetic sort order
*   **Python pretty-print/JSON:**
    *   Always a list
    *   List elements are either:
        *   Strings for the basic types listed above
        *   Python lists for declarative functions - a simple search through the list for a match to the types above is
            sufficient (if understanding the declarative function is not required)
    *   _Not to be confused with "/Type" keys which is why the `[` is included in this grep!_
    *   `grep "'Type': \[" dom.json | sed -e 's/^ *//' | sort | uniq`
*   **Linux CLI tests:**
    *   `cut -f 2 *.tsv | sort | uniq`
    *   `cut -f 2 *.tsv | sed -e "s/;/\n/g" | sort | uniq`


## Column 3 - SinceVersion


*   Must not be blank
*   Must be one of `1.0`, `1.1`, ... `1.7` or `2.0`
*   In the future:
    *   Set of versions may be increased - e.g. `2.1`
    *   A small set of declarative functions might also be used
        *   Either as "or" conjunction or by themselves to represent proprietary extensions? Examples include inline image
            abbreviations used in Image XObjects; DP as an alias for DecodeParams by Adobe; the Apple /AAPL or /PTEX LaTeX
            extensions; the various PDF 2.0 extension ISO specs being developed now
            e.g., `fn:Extension(string)` or `2.0;fn:Extension(string)` where `string` might be `ISO-32002` or `AdobeExtensionLevel5`.
*   **Python pretty-print/JSON**
    *   Always a string (never blank)
    *   Value is one of the values listed above
    *   `grep "'SinceVersion'" dom.json | sed -e 's/^ *//' | sort | uniq`
*   **Linux CLI tests:**
    *   `cut -f 3 *.tsv | sort | uniq`


## Column 4 - Deprecated


*   Can be blank
*   Must be one of `1.0`, `1.1`, ... `1.7` or `2.0`
*   In the future:
    *   Set of versions may be increased - e.g. `2.1`
*   **Python pretty-print/JSON**
    *   A string or `None`
    *   Value is one of the values listed above
    *   `grep "'Deprecated': " dom.json | sed -e 's/^ *//' | sort | uniq`
*   **Linux CLI tests:**
    *   `cut -f 4 *.tsv | sort | uniq`


## Column 5 - Required


*   Must not be blank
*   Either:
    *   Single word: `FALSE` or `TRUE` (uppercase only)
    *   The declarative function `fn:IsRequired(...)` - no SQUARE BRACKETS!
        *   This may then have further nested functions (e.g. `fn:SinceVersion`, `fn:IsPresent`, `fn:NotPresent`)
*   If Key column contains ASTERISK, then "Required" field must be FALSE
    *   Cannot require an infinite number of keys! If need at least one element, then have explicit first rows
        with "Required"==`TRUE` followed by ASTERISK with "Required"==`FALSE`)
*   **Python pretty-print/JSON:**
    *   Always a list
    *   List length is always 1
    *   List element is either:
        *   Boolean
        *   Python list for functions which must be `fn:IsRequired(`
    *   `grep "'Required': " dom.json | sed -e 's/^ *//' | sort | uniq`
*   **Linux CLI tests:**
    *   `cut -f 5 *.tsv | sort | uniq`


## Column 6 - IndirectReference


*   Must not be blank
*   Streams must always have "IndirectReference" as `TRUE`
*   Either:
    *   Single word: `FALSE` or `TRUE` (uppercase only); or
    *   Single declarative function `fn:MustBeDirect()` indicating that the corresponding key/array element must
        be a direct object
    *   `[];[];[]` style expression - SEMI-COLON separated, SQUARE-BRACKETS expressions that exactly match the
        number of items in the "Type" column. Only the values `TRUE` or `FALSE` can be used.
    *   A more complex set of requirements using the declarative function `fn:MustBeDirect(optional-key-path>)`
        **NOT** enclosed in SQUARE-BRACKETS
*   **Python pretty-print/JSON:**
    *   Always a list
    *   List length always matches length of "Type" column
    *   List elements are either:
        *   Python Boolean (`True`/`False`)
        *   Python list for functions where the outer-most declarative function must be `fn:IsRequired(`, with
            an optional argument for a condition
    *   `grep "'IndirectReference':" dom.json | sed -e 's/^ *//' | sort | uniq`
*   **Linux CLI tests:**
    *   `cut -f 6 *.tsv | sort | uniq`


## Column 7 - Inheritable


*   Must not be blank
*   Single word: `FALSE` or `TRUE` (uppercase only, as its not a PDF keyword!)
*   **Python pretty-print/JSON:**
    *   Always a boolean
    *   `grep "'Inheritable'" dom.json | sed -e 's/^ *//' | sort | uniq`
*   **Linux CLI tests:**
    *   `cut -f 7 *.tsv | sort | uniq`


## Column 8 - DefaultValue


*   Can be blank
*   SQUARE-BRACKETS are only used for PDF arrays, in which case they must use double SQUARE-BRACKETS
    *   e.g. [false false] ? [[false false]]
*   If there is a "DefaultValue" AND there are multiple types, then require [];[];[] expressions
    *   If the "DefaultValue" is a PDF array, then this will result in nested SQUARE-BRACKETS as in `[];[[0 0 1]];[]`
*   The only valid declarative functions are:
    *   `fn:ImplementationDependent()`, or
    *   `fn:Eval(condition, statement)` where statement must match the appropriate type (e.g. an integer for an
        integer key, a string for a string-\* key, etc)
    *   Declarative functions only need [];[];[] expression if a multi-typed key
*   **Python pretty-print/JSON:**
    *   A list or `None`
    *   If list, then length always matches length of "Type"
    *   If list element is also a list then it is either:
        *   Declarative function with 1st element being a FUNC_NAME token
        *   Key value (@key) with 1st element being a KEY_VALUE token
        *   A PDF array (1st token is anything else) - including an empty PDF array
    *   `grep -o "'DefaultValue': .*" dom.json | sed -e 's/^ *//' | sort | uniq`
*   **Linux CLI tests:**
    *   `cut -f 8 *.tsv | sort | uniq `
    *   `cut -f 2,8 *.tsv | sort | uniq | grep -P "\t[[:graph:]]+.*"`
    *   `cut -f 1,2,8 *.tsv | sort | uniq | grep -P "\t[[:graph:]]*\t[[:graph:]]+.*$"`


## Column 9 - PossibleValues


*   Can be blank
*   SEMI-COLON separated, SQUARE-BRACKETS expressions that exactly match the number of items in "Type" column
*   **Python pretty-print/JSON:**
    *   A list or `None`
    *   If list, then length always matches length of "Type"
        *   Elements can be anything, including `None`
    *   `grep -o "'PossibleValues': .*" dom.json | sed -e 's/^ *//' | sort | uniq`
*   _Issues:_
    *   _inconsistent use of [] for Possible Values - cf. CalGrayDict vs Whitepoint_


## Column 10 - SpecialCase


*   Can be blank
*   SEMI-COLON separated, SQUARE-BRACKETS expressions that exactly match the number of items in "Type" column
*   Each expression inside a SQUARE-BRACKET is a declarative function
*   **Python pretty-print/JSON:**
    *   A list or `None`
    *   If list, then length always matches length of "Type"
        *   Elements can be anything, including `None`
    *   `grep -o "'SpecialCase': .*" dom.json | sed -e 's/^ *//' | sort | uniq`


## Column 11 - Link


*   Can be blank (for when Type is a single fundamental type)
*   If non-blank, always uses SQUARE-BRACKETS
*   SEMI-COLON separated, SQUARE-BRACKETS expressions that exactly match the number of items in "Type" column
*   Valid "Links" must exist for these selected object types only:
    * `array`
    * `dictionary`
    * `stream`
*   "Links" must NOT exist for selected fundamental "Types" (i.e. must be empty `[]` in the SEMI-COLON separated list):
    * `array`
    * `bitmask`
    * `boolean`
    * `date`
    * `integer`
    * `matrix`
    * `name`
    * `name-tree`
    * `null`
    * `number`
    * `number-tree`
    * `rectangle`
    * `string`
    * `string-ascii`
    * `string-byte`
    * `string-text`
*   Each sub-expression inside a SQUARE-BRACKET is a COMMA separate list of case-sensitive filenames of other TSV files (without `.tsv` extension)
*   These sub-expressions can also include the version declarative functions:
    *   `fn:SinceVersion(pdf-version, link)`
    *   `fn:BeforeVersion(pdf-version, link)`
    *   `fn:IsPDFVersion(1.0, link)`
*   **Python pretty-print/JSON:**
    *   A list or `None`
    *   If list, then length always matches length of "Type"
        *   List elements can be `None`
        *   Validity of list elements aligns with indexed "Type" data
*   **Linux CLI test:**
    *   `cut -f 11 *.tsv | sort | uniq | grep -o "fn:[a-zA-Z]*" | sort | uniq`


## Column 12- Notes



*   Can be blank
*   Free text - no validation possible
*   **Python pretty-print/JSON:**
    *   A string or `None`


## Validation of declarative functions



*   `_parent::_` (all lowercase) is a special keyword
*   `null` (all lowercase) is the PDF null object (_Note: it is a valid type_)
*   Change to use PDFPath ([https://github.com/pdf-association/PDFPath](https://github.com/pdf-association/PDFPath))
    *   Paths to objects are separated by `::` (double COLONs)
        *   e.g. parent::@Key. Object::Key, Object::&lt;0-based integer>
    *   `Key`_means `key is present` (Key is case-sensitive match)
    *   `@Key` means `value of key` (Key is case-sensitive match)
*   `true` and `false` (all lowercase) are the PDF keywords (required for explicit comparison with `@key`) - uppercase `TRUE` and `FALSE` **never** get used in functions
*   All functions start with `fn:` (case-sensitive, COLON)
*   All functions are CamelCase case sensitive with BRACKETS `(` and `)` and do NOT use Digits, DASH or UNDERSCOREs (i.e. must match a simple alpha word regex)
*   Functions can have 0, 1 or more arguments that are COMMA separated
    *   Functions need to end with `()` for zero arguments
    *   Arguments always within `(...)`
    *   Functions can nest (as arguments of other functions)
*   Support two C/C++ style boolean operators: && (logical and), || (logical or)
*   Support six C/C++ style comparison operators: &lt;. &lt;=, >, >=, ==, !=
*   NO bit-wise operators - _use declarative functions instead_
*   NO unary NOT (`!`) operator (_implement as a new declarative function `fn:Notxxxx`_)
*   All expressions MUST be fully bracketed between Boolean operators (_to avoid defining precedence rules_)
*   NO conditional if/then, switch or loop style statements _ - its declarative!_
*   NO local variables_ - its declarative!_
*   Does using comparison operators always require the expression to be wrapped in fn:Eval(...)? We are currently inconsistent - and I don't like this **AT ALL** as it is functional programming verboseness, not declarative style!!! Need to be more precise about when fn:Eval is needed vs not. e.g. in "Required" column (#5) do not require as this is more than sufficient: `fn:IsRequired(parent::@S==Luminosity)`. Will it be needed in "SinceVersion" column if we want to express both an official PDF version and an extension?


## Linux CLI voodoo


List all declarative function names


```
$ grep --color=always -ho "fn:[[:alnum:]]*." *.tsv | sort | uniq
```



List all declarative functions that take no parameters


```
$ grep --color=always -Pho "fn:[[:alnum:]]*\(\)" *.tsv | sort | uniq
```



List all parameter lists (but not function names) and a few PDF strings too!


```
$ grep --color=always -Pho "\((?>[^()]|(?R))*\)" *.tsv | sort | uniq
```



List all declarative functions with their arguments


```
$ grep --color=always -Pho "fn:[[:alnum:]]*\([^\t\]\;]*" *.tsv | sort | uniq
```



# Parameters


<table>
  <tr>
   <td>`bit-posn`
   </td>
   <td>
<ul>

<li>bits are numbered 1-32 inclusive
</li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`version`
   </td>
   <td>
<ul>

<li>One of `1.0`, `1.1`, ... `2.0`

<li>Matches set used in Column 3 (SinceVersion)
</li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`expr`
   </td>
   <td>
<ul>

<li>Mathematical expression (or constant)

<li>References to PDF objects need ``@`` to get key/array value

<li>Can be nested functions that return integer or real values
</li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`cond`
   </td>
   <td>
<ul>

<li>Boolean expression, including nested functions or mathematical expressions that use comparison operators

<li>Use `==`, `!=`, `>=`, `&lt;=`, `>`, `&lt;` comparison operators

<li>Use `&&` (AND) or `||` (OR) logical operators
</li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`statement`
   </td>
   <td>
<ul>

<li>PDF object (e.g. name, string, real, integer, array, null)

<li>A mathematical expression (like `expr`)

<li>Arlington Type - if in Column 2

<li>Arlington link - if in Column 11
</li>
</ul>
   </td>
  </tr>
</table>



# Declarative Functions


<table>
  <tr>
   <td>`int fn:ArrayLength(key)`
   </td>
   <td>
<ul>

<li>returns integer >= 0

<li>represents requirement of current key which must be of type `array`
</li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`Bool fn:ArraySortAscending()`
   </td>
   <td>
<ul>

<li>statement of fact (requirement) about current key
</li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`statement fn:BeforeVersion(version, [ statement ] )`
   </td>
   <td>
<ul>

<li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`Bool fn:BitClear(bit-posn) `
   </td>
   <td>
<ul>

<li>States requirement that `bit-posn` (1-32) is zero (clear)
</li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`Bool fn:BitSet(bit-posn)`
   </td>
   <td>
<ul>

<li>States requirement that `bit-posn` (1-32) is one (set)
</li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`Bool fn:BitsClear(low-bit, high-bit)`
   </td>
   <td>
<ul>

<li>States requirement that all bits between `low-bit` (1-32) and `high-bit` (1-32) inclusive are zero (clear)

<li>`low-bit` &lt; `high-bit`
</li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`Bool fn:BitsSet(low-bit, high-bit)`
   </td>
   <td>
<ul>

<li>States requirement that all bits between `low-bit` (1-32) and `high-bit` (1-32) inclusive are zero (clear)

<li>`low-bit` &lt; `high-bit`
</li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`fn:CreatedFromNamePageObj()`
   </td>
   <td>
<ul>

<li>
</li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`fn:Deprecated(version, statement-that-is-deprecated)`
   </td>
   <td>
<ul>

<li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`fn:Eval(expr)`
   </td>
   <td>
<ul>

<li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`int fn:FileSize()`
   </td>
   <td>
<ul>

<li>returns integer >= 0

<li>represents fact about PDF file
</li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`Bool fn:FontHasLatinChars()`
   </td>
   <td>
<ul>

<li>
</li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`fn:Ignore(expr)`
   </td>
   <td>
<ul>

<li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`fn:ImageIsStructContentItem()`
   </td>
   <td>
<ul>

<li>
</li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`fn:ImplementationDependent()`
   </td>
   <td>
<ul>

<li>
</li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`Bool fn:InMap(key-name)`
   </td>
   <td>
<ul>

<li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`Bool fn:IsAssociatedFile()`
   </td>
   <td>
<ul>

<li>returns requirement about current key
</li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`Bool fn:IsEncryptedWrapper()`
   </td>
   <td>
<ul>

<li>returns requirement about current PDF file
</li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`Bool fn:IsLastInNumberFormatArray()`
   </td>
   <td>
<ul>

<li>returns requirement about current key
</li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`Bool fn:IsMeaningful(expr)`
   </td>
   <td>
<ul>

<li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`Bool fn:IsPDFTagged()`
   </td>
   <td>
<ul>

<li>represents fact about PDF file
</li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`fn:IsPDFVersion(1.0, statement-only-in-pdf-1.0)`
   </td>
   <td>
<ul>

<li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`Bool fn:IsPresent(key-name)`
   </td>
   <td>
<ul>

<li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`Bool fn:IsRequired(condition)`
   </td>
   <td>
<ul>

<li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`fn:KeyNameIsColorant()`
   </td>
   <td>
<ul>

<li>
</li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`Bool fn:MustBeDirect( [ optional-condition ] )`
   </td>
   <td>
<ul>

<li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`fn:NoCycle()`
   </td>
   <td>
<ul>

<li>
</li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`fn:NotPresent(key-name)`
   </td>
   <td>
<ul>

<li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`Bool fn:NotStandard14Font()`
   </td>
   <td>
<ul>

<li>
</li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`fn:PageContainsStructContentItems()`
   </td>
   <td>
<ul>

<li>
</li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`int fn:RectHeight(key-which-is-a-rect)`
   </td>
   <td>
<ul>

<li>Returns the height of a rectangle (>= 0)
</li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`int fn:RectWidth(key-which-is-a-rect)`
   </td>
   <td>
<ul>

<li>Returns the width of a rectangle (>= 0)
</li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`fn:RequiredValue(condition-expression, value)`
   </td>
   <td>
<ul>

<li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`fn:SinceVersion( version, statement )`
   </td>
   <td>
<ul>

<li>
</ul>
   </td>
  </tr>
  <tr>
   <td>`Bool fn:StreamLength( expr )`
   </td>
   <td>
<ul>

<li>`expr `is stream length of current key

<li>`expr `evaluates to a non-negative integer
</ul>
   </td>
  </tr>
  <tr>
   <td>`Bool fn:StringLength( [ cond ], expr )`
   </td>
   <td>
<ul>

<li>`expr `is string length of current key

<li>`expr `evaluates to a non-negative integer

<li>`cond` is optional

<li>`cond` is condition when string length must be the same `expr`
</li>
</ul>
   </td>
  </tr>
</table>



# Checks to be done in 32K

Check sizes of all `array` search hits - up to Table 95

Check ranges of all `integer` search hits - up to Table 170.
See also: [https://github.com/pdf-association/pdf-issues/issues/15](https://github.com/pdf-association/pdf-issues/issues/15)

Read every Table for dictionary keys and update declarative rules - up to Table 186 Popup Annots

Check ranges of all `number` search hits - not started
