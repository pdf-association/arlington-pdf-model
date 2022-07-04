# Arlington PDF Model Grammar Validation Rules

This document describes some strict rules for the Arlington PDF model, for both the data and the predicates (custom declarative functions that start `fn:`). Only some of these rules are currently implemented by various PoCs, but everything is precisely documented here.


# TSV file rules

* They are TSV, not CSV.
* No double quotes are required
* Every TSV file needs to have the same identical header row as first line in file
* EOL rules for TSV are now set by .gitattributes to be LF -
    - standard Linux CLI works including under Windows WSL2: `cut`, `grep`, `sed`, etc.
    - this means you can also use all the [Ebay TSV utilities](https://github.com/eBay/tsv-utils) even under Windows
    - [GNU datamash](https://www.gnu.org/software/datamash/) can also be used
* Every TSV file needs to have the full set of TABS (for all columns)
* Last row in TSV needs EOL after last TAB
* TSV file names are case sensitive
* TSV file extensions are always `.tsv` (lowercase) but are not present in the TSV data itself
* all TSV files will have matching numbers of `[`, `]` `(` and `(`


# PDF Object conventions

* There are NO leading SLASHES for PDF names (_ever_!)
* PDF names might use `#`-escaping in the future
    *   the PDF specification never specifies any such keys so this is allowing for future-proofing
* PDF strings use single quotes `'` and `'` (as `(` and `)` are ambiguous with expressions and single quotes are supported natively by Python `csv` module)
* PDF arrays always use `[` and `]` (which requires some additional processing so as not to be confused with our [];[];[] syntax for complex types)
* Expressions with integers need to use integers
* Leading `@` indicates "value of" a key or array element
* PDF Booleans are `true` and `false` lowercase.
    - Uppercase `TRUE`/`FALSE` are reserved for logical Boolean TSV data fields such as the "Required" field.
* expressions using `&&` or `||` logical operators need to be either fully bracketed or be just a predicate and have a single SPACE either side of the logical operator. precedence rules are NOT implemented.

# TSV Data Fields

## Column 1 - Key Name


*   Must not be blank
*   Case sensitive (as per PDF spec)
*   No duplicates keys in any single TSV file
*   Only alphanumeric, `.`, `-`, `_` or ASTERISK characters (no whitespace or other special chars)
    *   PDF names might use `#`-escaping in the future (but the PDF specification never specifies any such keys so this is allowing for future-proofing)
*   If a dictionary, then "Key" may also be an ASTERISK `*` meaning wildcard, so anything is allowed
*   If ASTERISK `*` by itself then must be last row in TSV file
*   If ASTERISK `*` by itself then "Required" column must be FALSE
*   If expressing a PDF array, then "Key" name is really the integer array index.
    - Zero-based increasing (always by 1) integers always starting at ZERO (0), with an optional ASTERISK appended after the digit (indicating repeat)
    - Or just an ASTERISK `*` meaning that any number of array elements may exist
*   If expressing a PDF array with `integer+ASTERISK` and then the last set of rows must all be `integer+ASTERISK` (indicating a repeating group of _N_ starting at array element _X_ (numbered _X_ to _X+N_-1) array elements).
*   If expressing a PDF array with `integer+ASTERISK` (and all rows are the same) then the "Required" column should be TRUE if all _N_ entries must always be repeated as a full set (e.g. in pairs).
*   In the future:
    *   "Key" names with `#`-escapes
    *   _How should we define malforms??? e.g. `/type` vs `/Type`; `/SubType` vs `/Subtype`; `/BlackIs1` (uppercase i) vs `/Blackls1` (lowercase L). Are these separate rows in a TSV, a "SpecialCase" column or wrapped in a declarative
       function in the "Key" column? (e.g. `Type;fn:Malform(type,...)`). Need to consider impact on Linux CLI processing, such as grep._
*   **Python pretty-print/JSON**
    *   String (as JSON dictionary key)
*   **Linux CLI tests:**
    ```shell
    cut -f 1 *.tsv | sort | uniq
    cut -f 1 *.tsv | sort | uniq | grep "\*"
    ```


## Column 2 - Type


*   Must not be blank
*   Alphabetically sorted, SEMI-COLON separated list from the following predefined set of Arlington types (always lowercase):
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
*   Each type may also be wrapped in a version-based declarative function (e.g. `fn:SinceVersion(version,type)` or
    `fn:Deprecated(version,type)` ).
*   When a declarative function is used, the internal simple type is still kept in its alphabetic sort order
* The following predefined Arlington types ALWAYS REQUIRE a link:
    - `array`, `dictionary`, `name-tree`, `number-tree`, `stream`
* The following predefined Arlington types NEVER have a link (they are the basic Arlington types):
    - `bitmask`, `boolean`, `date`, `integer`, `matrix`, `name`, `null`, `number`, `rectangle`, `string`, `string-ascii`, `string-byte`, `string-text`
*   **Python pretty-print/JSON:**
    *   Always a list
    *   List elements are either:
        *   Strings for the basic types listed above
        *   Python lists for predicates - a simple search through the list for a match to the types above is
            sufficient (if understanding the declarative function is not required)
    *   _Not to be confused with "/Type" keys which is why the `[` is included in this grep!_
    *   `grep "'Type': \[" dom.json | sed -e 's/^ *//' | sort | uniq`
*   **Linux CLI tests:**
    ```shell
    cut -f 2 *.tsv | sort | uniq
    cut -f 2 *.tsv | sed -e "s/;/\n/g" | sort | uniq
    ```


## Column 3 - SinceVersion


*   Must not be blank
*   Must be one of `1.0`, `1.1`, ... `1.7` or `2.0`
*   In the future:
    *   Set of versions may be increased - e.g. `2.1`
    *   A small set of predicates might also be used
        *   Either as "or" conjunction or by themselves to represent proprietary extensions? Examples include inline image
            abbreviations used in Image XObjects; `DP` as an alias for `DecodeParams` by Adobe; the Apple /AAPL or /PTEX LaTeX
            extensions; the various PDF 2.0 extension ISO specs being developed now
            e.g., `fn:Extension(string)` or `2.0;fn:Extension(string)` where `string` might be `ISO-32002` or `AdobeExtensionLevel5`.
*   **Python pretty-print/JSON**
    *   Always a string (never blank)
    *   Value is one of the values listed above
    *   `grep "'SinceVersion'" dom.json | sed -e 's/^ *//' | sort | uniq`
*   **Linux CLI tests:**
    ```shell
    cut -f 3 *.tsv | sort | uniq
    ```


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
    ```shell
    cut -f 4 *.tsv | sort | uniq
    ```

## Column 5 - Required


*   Must not be blank
*   Either:
    *   Single word: `FALSE` or `TRUE` (uppercase only)
    *   The declarative function `fn:IsRequired(...)` - no SQUARE BRACKETS!
        *   This may then have further nested functions (e.g. `fn:SinceVersion`, `fn:IsPresent`, `fn:NotPresent`)
*   If "Key" column contains ASTERISK, then "Required" field must be FALSE
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
    ```shell
    cut -f 5 *.tsv | sort | uniq
    ```


## Column 6 - IndirectReference


*   Must not be blank
*   Streams must always have "IndirectReference" as `TRUE`
*   Either:
    *   Single word: `FALSE` or `TRUE` (uppercase only); or
    *   Single declarative function `fn:MustBeDirect()` or `fn:MustBeIndirect()` indicating that the corresponding key/array element must
        be a direct object or not
    *   `[];[];[]` style expression - SEMI-COLON separated, SQUARE-BRACKETS expressions that exactly match the
        number of items in the "Type" column. Only the values `TRUE` or `FALSE` can be used.
    *   A more complex set of requirements using the predicate `fn:MustBeDirect(optional-key-path>)` or `fn:MustBeIndirect(...)`
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
    ```shell
    cut -f 6 *.tsv | sort | uniq
    ```


## Column 7 - Inheritable


*   Must not be blank
*   Single word: `FALSE` or `TRUE` (uppercase only, as its not a PDF keyword!)
*   **Python pretty-print/JSON:**
    *   Always a boolean
    *   `grep "'Inheritable'" dom.json | sed -e 's/^ *//' | sort | uniq`
*   **Linux CLI tests:**
    ```shell
    cut -f 7 *.tsv | sort | uniq
    ```


## Column 8 - DefaultValue


*   Can be blank
*   SQUARE-BRACKETS are only used for PDF arrays, in which case they must use double SQUARE-BRACKETS
    *  e.g. [false false] ? [[false false]]
*   If there is a "DefaultValue" AND there are multiple types, then require [];[];[] expressions
    *  If the "DefaultValue" is a PDF array, then this will result in nested SQUARE-BRACKETS as in `[];[[0 0 1]];[]`
*   The only valid predicates are:
    *  `fn:ImplementationDependent()`, or
    *  `fn:DefaultValue(condition, value)` where _value_ must match the appropriate type (e.g. an integer for an integer key, a string for a string-\* key, etc), or
    *  `fn:Eval(expression)`
    *  Predicates only need [];[];[] expression if a multi-typed key
*   **Python pretty-print/JSON:**
    *   A list or `None`
    *   If list, then length always matches length of "Type"
    *   If list element is also a list then it is either:
        *   Declarative function with 1st element being a FUNC_NAME token
        *   "Key" value (`@key`) with 1st element being a KEY_VALUE token
        *   A PDF array (1st token is anything else) - including an empty PDF array
    *   `grep -o "'DefaultValue': .*" dom.json | sed -e 's/^ *//' | sort | uniq`
*   **Linux CLI tests:**
    ```shell
    cut -f 8 *.tsv | sort | uniq
    cut -f 2,8 *.tsv | sort | uniq | grep -P "\t[[:graph:]]+.*"
    cut -f 1,2,8 *.tsv | sort | uniq | grep -P "\t[[:graph:]]*\t[[:graph:]]+.*$"
    ```


## Column 9 - PossibleValues


*   Can be blank
*   SEMI-COLON separated, SQUARE-BRACKETS expressions that exactly match the number of items in "Type" column
*   **Python pretty-print/JSON:**
    *   A list or `None`
    *   If list, then length always matches length of "Type"
        *   Elements can be anything, including `None`
    ```shell
    grep -o "'PossibleValues': .*" dom.json | sed -e 's/^ *//' | sort | uniq
    ```
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
    ```shell
    grep -o "'SpecialCase': .*" dom.json | sed -e 's/^ *//' | sort | uniq
    ```


## Column 11 - Link


*   Can be blank (for when "Type" is a single basic type)
*   If non-blank, always uses SQUARE-BRACKETS
*   SEMI-COLON separated, SQUARE-BRACKETS expressions that exactly match the number of items in "Type" column
*   Valid "Links" must exist for these selected object types only:
    * `array`
    * `dictionary`
    * `stream`
    * `name-tree`
    * `number-tree`
*   "Links" must NOT exist for selected fundamental "Types" (i.e. must be empty `[]` in the SEMI-COLON separated list):
    * `array`
    * `bitmask`
    * `boolean`
    * `date`
    * `integer`
    * `matrix`
    * `name`
    * `null`
    * `number`
    * `rectangle`
    * `string`
    * `string-ascii`
    * `string-byte`
    * `string-text`
*   Each sub-expression inside a SQUARE-BRACKET is a COMMA separate list of case-sensitive filenames of other TSV files (without `.tsv` extension)
*   These sub-expressions can also include the version predicates:
    *   `fn:SinceVersion(pdf-version, link)`
    *   `fn:BeforeVersion(pdf-version, link)`
    *   `fn:IsPDFVersion(1.0, link)`
*   **Python pretty-print/JSON:**
    *   A list or `None`
    *   If list, then length always matches length of "Type"
        *   List elements can be `None`
        *   Validity of list elements aligns with indexed "Type" data
*   **Linux CLI test:**
    ```shell
    cut -f 11 *.tsv | sort | uniq | grep -o "fn:[a-zA-Z]*" | sort | uniq
    ```


## Column 12- Notes



*   Can be blank
*   Free text - no validation possible
*   **Python pretty-print/JSON:**
    *   A string or `None`


# Validation of predicates (declarative functions)



*   `_parent::_` (all lowercase) is a special keyword
*   `null` (all lowercase) is the PDF null object (_Note: it is a valid type_)
*   Change to use PDFPath ([https://github.com/pdf-association/PDFPath](https://github.com/pdf-association/PDFPath))
    *   Paths to objects are separated by `::` (double COLONs)
        *   e.g. `parent::@Key`. `Object::Key`, `Object::&lt;0-based integer&gt;`
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
*   Support six C/C++ style comparison operators: &lt;. &lt;=, &gt;, &gt;=, ==, !=
*   NO bit-wise operators - _use predicates instead_
*   NO unary NOT (`!`) operator (_implement as a new declarative function `fn:Notxxxx`_)
*   All expressions MUST be fully bracketed between Boolean operators (_to avoid defining precedence rules_)
*   NO conditional if/then, switch or loop style statements _ - its declarative!_
*   NO local variables_ - its declarative!_
*   Does using comparison operators always require the expression to be wrapped in fn:Eval(...)? We are currently inconsistent - and I don't like this **AT ALL** as it is functional programming verboseness, not declarative style!!! Need to be more precise about when `fn:Eval` is needed vs not. e.g. in "Required" column (#5) do not require as this is more than sufficient: `fn:IsRequired(parent::@S==Luminosity)`. Will it be needed in "SinceVersion" column if we want to express both an official PDF version and an extension?


# Linux CLI voodoo

List all declarative function names:

```shell
grep --color=always -ho "fn:[[:alnum:]]*." *.tsv | sort | uniq
```

List all predicates and their Arguments

```shell
grep -Pho "fn:[a-zA-Z0-9]+\((?:[^)(]+|(?R))*+\)" *.tsv | sort | uniq
```

List all predicates that take no parameters:

```shell
grep --color=always -Pho "fn:[a-zA-Z0-9]+\(\)" *.tsv | sort | uniq
```

List all parameter lists (but not function names) (and a few PDF strings too!):

```shell
grep --color=always -Pho "\((?>[^()]|(?R))*\)" *.tsv | sort | uniq
```

List all predicates with their arguments:

```shell
grep --color=always -Pho "fn:[a-zA-Z0-9]+\([^\t\]\;]*\)" *.tsv | sort | uniq
```


# EBay TSV Utilities

Pretty columnized output:
```shell
tsv-pretty Catalog.tsv
```

Find all keys that are of "Type" 'string-byte':
```shell
tsv-filter -H --str-eq Type:string-byte *.tsv
```

Only precisely 'string-byte':
```shell
tsv-filter -H --str-eq Type:string-byte --ge SinceVersion:1.5 *.tsv
```

Any string type (using string-based regex):
```shell
tsv-filter -H --regex Type:string\* --ge SinceVersion:1.5 *.tsv
```

"Type" includes 'string-byte':
```shell
tsv-filter -H --regex Type:.\*string-byte\* --ge SinceVersion:1.5 *.tsv
```

# Program Output

A reliable (i.e. easy to grep: `grep "^xx" output.txt`) and repeatable method to highlight class of
difference between an extant PDF file and the Arlington definition for a specific PDF version indicated
by a 2 character prefix at the start of each output line. Each output line is assumed to be a single key
or array element in a PDF file, as is currently output by arlington.py and TestGrammar (C++17).

The first character in the prefix represents the key / array index

The second character in the prefix represents the value of the key or array element.

| Prefix | Description |
| ------ | ----------- |
| `==` | Key/array element and value are fully within the Arlington definition for the required specific PDF version and all data is validated (including "Key", "Type", "PossibleValues", "IndirectReference"/`fn:MustBeDirect`, etc). All predicates are also all validated. _This assumes that the reporting applications implements all predicates_! |
| `=?` | The Key is in the Arlington definition for the required specific version of PDF but there is a data error (such as with "Type", "PossibleValues", "IndirectReference"/`fn:MustBeDirect`, etc. or with one or more of the predicates being invalidated). _This assumes that the reporting applications implements all predicates_! |
| `--` | An Arlington required ("Required"==FALSE) Key for the required specific version of PDF is missing in the PDF (i.e. NOT in the PDF) but is specified in Arlington. |
| `-?` | An Arlington optional Key for the required specific version of PDF is missing in the PDF but is specified in Arlington. _This won't be reported unless an additional option is specified as it would otherwise be too verbose._ |
| `++` | Key is in the PDF, but is not known to Arlington for any version of PDF (i.e. it is not unrecognized by Arlington at all). |
| `+?` | Key is in the PDF, but is not known to Arlington for this specific version of PDF (i.e. it is in the Arlington definition for some future definition of PDF). _This assumes the reporting application is PDF version aware_! |


# Parameters to predicates

<table>
  <tr>
   <td><code><i>bit-posn</i></code>
   </td>
   <td>
    <ul>
     <li>bits are numbered 1-32 inclusive</li>
     <li>bit 1 is the low-order bit
    </ul>
   </td>
  </tr>
  <tr>
   <td><code><i>version</i></code>
   </td>
   <td>
    <ul>
     <li>One of "1.0, "1.1", ... "2.0"</li>
     <li>Matches set used in "SinceVersion" (column 3) and "Deprecated" (column 4)</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code><i>expr</i></code>
   </td>
   <td>
    <ul>
     <li>Mathematical expression (or constant)</li>
     <li>Explicit bracketing <code>(...)</code> required (no order of operation!)</li>
     <li>Mathematical operators are <code>+</code>, <code>-</code>, <code>*</code>, and <code>mod</code></li>
     <li>References to PDF objects need <code>@</code> to get key/array value</li>
     <li>Can use nested predicates that represent an integer or number</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code><i>cond</i></code>
   </td>
   <td>
    <ul>
     <li>Boolean expression, including nested predicates, mathematical expressions that use comparison operators, or boolean sub-expressions</li>
     <li>Use <code>==</code>, <code>!=</code>, <code>&gt;=</code>, <code>&lt;=</code>, <code>&gt;</code>, <code>&lt;</code> mathematical comparison operators</li>
     <li>Use <code>==</code>, <code>!=</code>, <code>&&</code> (AND) or <code>||</code> (OR) logical operators (there is no NOT operator)</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code><i>statement</i></code>
   </td>
   <td>
    <ul>
     <li>PDF object (e.g. name, string, real, integer, array, null)</li>
     <li>A mathematical expression (like <code><i>expr</i></code>)</li>
     <li>Arlington predefined type - if in "Type" (column 2)</li>
     <li>Arlington link (TSV filename)- if in "Link" (column 11)</li>
    </ul>
   </td>
  </tr>
</table>



# Predicates (declarative functions)

**Do not use additional whitespace!**
Single SPACE characters are only required around logical operators (`&&` and `||`), MINUS (`-`) and the `mod` mathematical operators.

<table>
  <tr>
   <td><code>fn:ArrayLength(<i>key</i>)</code></td>
   <td>
    <ul>
     <li>asserts <i>key</i> references something of type <code>array</code></li>
     <li>returns an integer value >= 0</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:ArraySortAscending(<i>key</i>)</code></td>
   <td>
    <ul>
     <li>asserts <i>key</i> references something of type <code>array</code></li>
     <li>asserts that array is sorted in ascending order</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:BeforeVersion(<i>version</i>)</code><br/>
   <code>fn:BeforeVersion(<i>version</i>,<i>assertion</i>)</code></td>
   <td>
    <ul>
     <li><i>version</i> must be 1.0, ..., 2.0</li>
     <li>asserts that optional <i>assertion</i> only applies before PDF <i>version</i></li>
     <li><i>assertion</i> is a <code>fn:Eval(...)</code> expression that only applies before <i>version</i></li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:BitClear(<i>bit-posn</i>)</code></td>
   <td>
    <ul>
     <li><i>bit-posn</i> is 1-32 inclusive</li>
     <li>asserts that <i>bit-posn</i> (1-32 inclusive) is zero (clear)</li>
     <li>asserts <i>key</i> is something of type <code>bitmask</code></li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:BitSet(<i>bit-posn</i>)</code></td>
   <td>
    <ul>
     <li><i>bit-posn</i> is 1-32 inclusive</li>
     <li>asserts that <i>bit-posn</i> is one (set)</li>
     <li>asserts <i>key</i> is something of type <code>bitmask</code></li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:BitsClear(<i>low-bit</i>,<i>high-bit</i>)</code></td>
   <td>
    <ul>
     <li><i>low-bit</i> and <i>high-bit</i> must be 1-32 inclusive</li>
     <li>asserts that all bits between <i>low-bit</i> and <i>high-bit</i> inclusive are all zero (clear)</li>
     <li><i>low-bit</i> and <i>high-bit</i> must be different</li>
     <li>asserts <i>key</i> references something of type <code>bitmask</code></li>
    </ul>
   </td>
  </tr>
  <tr>
  <td><code>fn:BitsSet(<i>low-bit</i>,<i>high-bit</i>)</code></td>
  <td>
   <ul>
    <li><i>low-bit</i> and <i>high-bit</i> must be 1-32 inclusive</li>
    <li>asserts that all bits between <i>low-bit</i> and <i>high-bit</i> inclusive are all one (set)</li>
    <li><i>low-bit</i> and <i>high-bit</i> must be different</li>
    <li>asserts <i>key</i> references something of type <code>bitmask</code></li>
   </ul>
  </td>
  </tr>
  <tr>
   <td><code>fn:DefaultValue(<i>condition</i>,<i>statement</i>)</code></td>
   <td>
    <ul>
     <li>A conditionally-based default value</li>
     <li>Only used in "DefaultValue" field (column 8)</li>
    </ul>
   </td>
  </tr>  <tr>
   <td><code>fn:Deprecated(<i>version</i>,<i>statement</i>)</code></td>
   <td>
    <ul>
     <li>indicates that something was deprecated, such as a type ("Type" field), a value (e.g. "PossibleValues" field )</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:Eval(<i>expr</i>)</code></td>
   <td>
    <ul>
     <li>something complex that needs calculation</li>
     <li>may involve multiple terms with logical operators "&&" or "||"</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:FileSize()</code></td>
   <td>
    <ul>
     <li>represents the length of the PDF file in bytes (from <code>%PDF-<i>x.y</i></code> to last <code>%%EOF</code>)</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:FontHasLatinChars()</code></td>
   <td>
    <ul>
     <li></li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:Ignore(<i>expr</i>)</code></td>
   <td>
    <ul>
     <li>asserts that the current row is to be ignored when <code><i>expr</i></code> evaluates to true</li>
     <li>only used in "SpecialCase" field (column 10)</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:ImageIsStructContentItem()</code></td>
   <td>
    <ul>
     <li></li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:ImplementationDependent()</code></td>
   <td>
    <ul>
     <li>asserts that the current row is formally defined to be implementation dependent in the PDF specification</li>
     <li>key can be any value. There is no right or wrong.</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:InMap(<i>key</i>)</code></td>
   <td>
    <ul>
     <li><i>key</i> must be a map object (<code>dictionary</code> or <code>array</code>)</li>
     <li>current key value is required to be in this map</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:IsAssociatedFile()</code></td>
   <td>
    <ul>
     <li>returns requirement about current key</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:IsEncryptedWrapper()</code></td>
   <td>
    <ul>
     <li>assertion about current PDF file</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:IsLastInNumberFormatArray()</code></td>
   <td>
    <ul>
     <li>asserts that the current key is the last array element in a number format array (i.e. a parent)</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:IsMeaningful(<i>expr</i>)<code></td>
   <td>
    <ul>
     <li></li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:IsPDFTagged()</code></td>
   <td>
    <ul>
     <li>asserts that the PDF file is Tagged</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td>
    <code>fn:IsPDFVersion(1.0)</code>
    <code>fn:IsPDFVersion(1.0,<i>statement</i>)<code>
   </td>
   <td>
    <ul>
     <li>if no optional <i>statement</i>, then always true to PDF 1.0 files</li>
     <li>otherwise optional <i>statement</i> only applies to PDF 1.0 files</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:IsPresent(<i>key</i>)</code></td>
   <td>
    <ul>
     <li></li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:IsRequired(<i>condition</i>)</code></td>
   <td>
    <ul>
     <li>only occurs in "Required" field</li>
     <li>must always be the outer-most predicate</li>
     <li><i>condition</i> is a conditional expression that resolves to a boolean</li>
     <li>if <i>condition</i> evaluates to true, then current key is required</li>
     <li>if <i>condition</i> evaluates to false, then current key is optional</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:KeyNameIsColorant()</code></td>
   <td>
    <ul>
     <li>asserts that the current custom key is also a colorant name</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td>
    <code>fn:MustBeDirect()</code><br/>
    <code>fn:MustBeDirect(<i>condition</i>)</code>
   </td>
   <td>
    <ul>
     <li>only ever used in the "IndirectRef" field.</li>
     <li>if <i>condition</i> is true, the current key value must be a direct object.
     <li>if <i>condition</i> is false, the current key value can be direct or indirect.
    </ul>
 </td>
  </tr>
  <tr>
   <td>
    <code>fn:MustBeIndirect()</code><br/>
    <code>fn:MustBeIndirect(<i>condition</i>)</code>
   </td>
   <td>
    <ul>
     <li>only ever used in the "IndirectRef" field.</li>
     <li>if <i>condition</i> is true, the current key value must be an indirect object.
     <li>if <i>condition</i> is false, the current key value can be direct or indirect.
    </ul>
 </td>
  </tr>
  <tr>
   <td><code>fn:NoCycle()<code></td>
   <td>
    <ul>
     <li></li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:NotPresent(<i>key</i>)</code></td>
   <td>
    <ul>
     <li>evaluates if <i>key</i> is present</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:NotStandard14Font()</code></td>
   <td>
   <ul>
    <li>Asserts that the current font object is not one of the Standard 14 Type 1 fonts.</li>
   </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:PageContainsStructContentItems()</code></td>
   <td>
    <ul>
     <li></li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:RectHeight(<i>rect</i>)</code></td>
   <td>
    <ul>
     <li><i>rect</i> needs to be <code>rectangle</code> in Arlington predefined types</li>
     <li><i>rect</i> is a key name</li>
     <li>evaluates to a number >= 0, representing the height of the rectangle.</li>
     <li>Needs to be used inside <code>fn:Eval(...)</code>.</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:RectWidth(<i>rect</i>)</code></td>
   <td>
    <ul>
     <li><i>rect</i> needs to be <code>rectangle</code> in Arlington predefined types</li>
     <li><i>rect</i> is a key name</li>
     <li>evaluates to a number >= 0 representing the width of the rectangle</li>
     <li>Needs to be used inside <code>fn:Eval(...)</code>.</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:RequiredValue(<i>expr</i>,<i>value</i>)<code></td>
   <td>
    <ul>
     <li></li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:SinceVersion(<i>version</i>,<i>statement</i>)</code></td>
   <td>
    <ul>
     <li><i>version</i> is only 1.0, ..., 2.0</li>
     <li><i>statement</i> can be a pre-defined Arlington type, a Link, or constant (such as PDF name, number, integer, string)</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:StreamLength(<i>key</i>)</code></td>
   <td>
    <ul>
     <li><i>key</i> needs to be a stream and always evaluates to a non-negative integer.</li>
     <li>relies on <code>Length</code> key, rather than reading streams
     <li>Needs to be used inside <code>fn:Eval(...)</code>.</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:StringLength(<i>key</i>)</code></td>
   <td>
    <ul>
     <li><i>key</i> needs to be a string object and always evaluates to a non-negative integer.</li>
     <li>Needs to be used inside <code>fn:Eval(...)</code>.</li>
    </ul>
   </td>
  </tr>
</table>



# Checks still needing to be completed in ISO 32000-2:2020

- Check array length requirements of all `array` search hits - *done up to Table 95*

- Check ranges of all `integer` search hits - *done up to Table 170*.
See also: [https://github.com/pdf-association/pdf-issues/issues/15](https://github.com/pdf-association/pdf-issues/issues/15)

- Read every Table for dictionary keys and update declarative rules - *done up to Table 186 Popup Annots*

- Check ranges of all `number` search hits - *not started yet*
