# Arlington PDF Model Grammar Validation Rules

This document describes some strict rules for the Arlington PDF model, for both the data and the predicates (custom declarative functions that start `fn:`). Only some of these rules are currently implemented by various PoCs, but everything is precisely documented here.


# TSV file rules

* They are TSV, not CSV. Use tabs (`\t`).
* No double quotes are used.
* Every TSV file needs to have the same identical header row as first line in file
* EOL rules for TSV are now set by `.gitattributes` to be LF -
    - standard Linux CLI works including under Windows WSL2: `cut`, `grep`, `sed`, etc.
    - this means you can also use all the [Ebay TSV utilities](https://github.com/eBay/tsv-utils) even under Windows
    - [GNU datamash](https://www.gnu.org/software/datamash/) can also be used.
* Every TSV file needs to have the full set of TABS (for all columns).
* Last row in TSV needs EOL after last TAB.
* TSV file names are case sensitive.
* TSV file extensions are always `.tsv` (lowercase) but are not present in the TSV data itself.
* all TSV files will have matching numbers of `[`, `]` and `(`, `(`
* for a single row in any TSV, spliting each field on ';' will either result in 1 or _N_.
* files that represent PDF arrays match either `ArrayOf*.tsv`, `*Array.tsv` or `*ColorSpace.tsv`
    - these are identifiable by having a Key name of `0`
    ```shell
    grep "^0" *.tsv
    ```
* files that represent PDF 'map' objects (meaning that the dictionary key name can be anything) match `*Map.tsv`
    - note that CMaps are in `CMapStream.tsv`
* **NOT** all files that are PDF stream objects match `*Stream.tsv`
    - since each Arlington object is fully self-contained, many objects can be streams. The best method is to search for `DecodeParms` key instead:
    ```shell
    grep "^DecodeParms" *.tsv | tsv-pretty
    ```

# PDF Object conventions

* There are NO leading SLASHES for PDF names (_ever_!)
* PDF names don't use `#`-escaping
* PDF strings use single quotes `'` and `'` (as `(` and `)` are ambiguous with expressions and single quotes are supported natively by Python `csv` module)
* PDF arrays always use `[` and `]` (which requires some additional processing so as not to be confused with our [];[];[] syntax for complex fields)
* Expressions with integers need to use integers
* Leading `@` indicates "_value of_" a key or array element
* PDF Booleans are `true` and `false` lowercase.
    - Uppercase `TRUE`/`FALSE` are reserved for logical Boolean TSV data fields such as the "Required" field.
* expressions using `&&` or `||` logical operators need to be either fully bracketed or be just a predicate and have a single SPACE either side of the logical operator. precedence rules are NOT implemented.


# TSV Data Fields

*  A key or array element is so-called "complex" if it can be multiple values. This is represented by `[];[];[]`-type Expressions.
*  Something is so called a "wildcard" if the "Key" field contains an ASTERISK.
*  An array is so-called a "repeating array" if it requires N x a set of elements. This is represented by DIGIT+ASTERISK in the "Key" field  


## Column 1 - "Key"

*   Must not be blank
*   Case sensitive (as per PDF spec)
*   No duplicates keys in any single TSV file
*   Only alphanumeric, `.`, `-`, `_` or ASTERISK characters (no whitespace or other special characters)
*   If a dictionary, then "Key" may also be an ASTERISK `*` meaning wildcard, so anything is allowed
*   If ASTERISK `*` by itself then must be last row in TSV file
*   If ASTERISK `*` by itself then "Required" column must be FALSE
*   If expressing a PDF array, then "Key" name is really an integer array index.
    - Zero-based increasing (always by 1) integers always starting at ZERO (0), with an optional ASTERISK appended after the digit (indicating repeat)
    - Or just an ASTERISK `*` meaning that any number of array elements may exist
*   If expressing a PDF array with `digit+ASTERISK` and then the last set of rows must all be `digit+ASTERISK` (indicating a repeating group of _N_ starting at array element 0 (numbered 0 to _N_-1) array elements).
*   If expressing a PDF array with `integer+ASTERISK` (and all rows are the same) then the "Required" column should be TRUE if all _N_ entries must always be repeated as a full set (e.g. in pairs or quads).
*   In the future (and will require code changes!):
    *   "Key" names with `#`-escapes
    *   _How should we define malforms??? e.g. `/type` vs `/Type`; `/SubType` vs `/Subtype`; `/BlackIs1` (uppercase i) vs `/Blackls1` (lowercase L). Are these separate rows in a TSV, a "SpecialCase" column or wrapped in a declarative
       function in the "Key" column? (e.g. `Type;fn:Malform(type,...)`). Need to consider impact on Linux CLI processing, such as grep._
*   **Python pretty-print/JSON**
    *   String (as JSON dictionary key)
*   **Linux CLI tests:**
    ```shell
    cut -f 1 *.tsv | sort | uniq
    ```
* files that define objects with an arbitrary number of keys or array elements use the wildcard `*`. If the line number of the wildcard is line 2 then it is a map-like object. If the line number is after 2, then are additional fixed keys/elements.
    ```shell
    grep --line-numbers "^\*" *.tsv | tsv-pretty
    ```
* files that define arrays with repeating sequences of _N_ elements use the `digit+ASTERISK` syntax. Digit is currently restricted to a SINGLE digit 0-9.
   ```shell
   grep "^[0-9]\*" *.tsv | tsv-pretty
   ```


## Column 2 - "Type"

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


## Column 3 - "SinceVersion"

*   Must not be blank
*   Must be one of `1.0`, `1.1`, ... `1.7` or `2.0`
*   In the future:
    *   Set of versions may be increased - e.g. `2.1`
    *   A small set of predicates might also be used
        *   Either as "or" conjunction or by themselves to represent proprietary extensions? Examples include inline image
            abbreviations used in Image XObjects; `DP` as an alias for `DecodeParams` by Adobe; the Apple `/AAPL` or `/PTEX` LaTeX
            extensions; the various PDF 2.0 extension ISO specs being developed now
            e.g., `fn:Extension(string)` or `2.0;fn:Extension(string)` where `string` might be `ISO-32002` or `AdobeExtensionLevel5`.
* Version-based predicates in other fields should all be based on versions explicitly AFTER the version in this column
*   **Python pretty-print/JSON**
    *   Always a string (never blank)
    *   Value is one of the values listed above
    *   `grep "'SinceVersion'" dom.json | sed -e 's/^ *//' | sort | uniq`
*   **Linux CLI tests:**
    ```shell
    cut -f 3 *.tsv | sort | uniq
    ```


## Column 4 - "DeprecatedIn"

*   Can be blank
*   Must be one of `1.0`, `1.1`, ... `1.7` or `2.0`
*   Version-based predicates in other fields should all be based on versions explicitly BEFORE the version in this column
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

## Column 5 - "Required"

*   Must not be blank
*   Either:
    *   Single word: `FALSE` or `TRUE` (uppercase only)
    *   The declarative function `fn:IsRequired(...)` - no SQUARE BRACKETS!
        *   This may then have further nested functions (e.g. `fn:SinceVersion`, `fn:IsPresent`, `fn:Not`)
*   If "Key" column contains ASTERISK (as a wildcard), then "Required" field must be FALSE
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
    *   Single word: `FALSE` or `TRUE` (uppercase only, as it is not a PDF keyword!); or
    *   Single declarative function `fn:MustBeDirect()` or `fn:MustBeIndirect()` indicating that the corresponding key/array element must
        be a direct object or not
    *   `[];[];[]` style expression - SEMI-COLON separated, SQUARE-BRACKETS expressions that exactly match the
        number of items in the "Type" column. Only the values `TRUE` or `FALSE` can be used inside each `[...]`.
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
*   Single word: `FALSE` or `TRUE` (uppercase only, as it is not a PDF keyword!)
*   **Python pretty-print/JSON:**
    *   Always a boolean
    *   `grep "'Inheritable'" dom.json | sed -e 's/^ *//' | sort | uniq`
*   **Linux CLI tests:**
    ```shell
    cut -f 7 *.tsv | sort | uniq
    ```


## Column 8 - DefaultValue

*   Can be blank
*   SQUARE-BRACKETS are only used for PDF arrays, in which case they must use double SQUARE-BRACKETS (not that lowercase `true`/`false` are the PDF keywords)
    *  e.g. `[false false] vs [[false false]]``
*   If there is a "DefaultValue" AND there are multiple types, then require a complex [];[];[] expression
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
    cut -f 2,8 *.tsv | sort | uniq | grep -P "\t[[:graph:]]+.*" | tsv-pretty
    cut -f 1,2,8 *.tsv | sort | uniq | grep -P "\t[[:graph:]]*\t[[:graph:]]+.*$" | tsv-pretty
    ```


## Column 9 - "PossibleValues"

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


## Column 10 - "SpecialCase"

*   Can be blank
*   SEMI-COLON separated, SQUARE-BRACKETED complex expressions that exactly match the number of items in "Type" column
*   Each expression inside a SQUARE-BRACKET is a declarative function that calculates to TRUE/FALSE.
    * TRUE means that it is a valid, FALSE means it would be invalid
*   **Python pretty-print/JSON:**
    *   A list or `None`
    *   If list, then length always matches length of "Type"
        *   Elements can be anything, including `None`
    ```shell
    grep -o "'SpecialCase': .*" dom.json | sed -e 's/^ *//' | sort | uniq
    ```


## Column 11 - "Link"

*   Can be blank (but only when "Type" is a single basic type)
*   If non-blank, always uses SQUARE-BRACKETS
*   SEMI-COLON separated, SQUARE-BRACKETED complex expressions that exactly match the number of items in "Type" column
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
*   These sub-expressions MUST BE one of the four version-based predicates:
    *   `fn:SinceVersion(pdf-version,link)`
    *   `fn:Deprecated(pdf-version,link)`
    *   `fn:BeforeVersion(pdf-version,link)`
    *   `fn:IsPDFVersion(version,link)`
*   **Python pretty-print/JSON:**
    *   A list or `None`
    *   If list, then length always matches length of "Type"
        *   List elements can be `None`
        *   Validity of list elements aligns with indexed "Type" data
*   **Linux CLI test:**
    ```shell
    cut -f 11 *.tsv | sort | uniq | grep -o "fn:[a-zA-Z]*" | sort | uniq
    ```

## Column 12- "Notes"

*   Can be blank
*   Free text - no validation possible
*   Often contains a reference to a Table or clause from ISO 32000-2:2020 (PDF 2.0) or a PDF Association Errata issue link (GitHub URL)
    * For dictionaries, this is normally the first key or the `Type` or `Subtype` row depending on what is differentiating the definition
*   **Python pretty-print/JSON:**
    *   A string or `None`


# Validation of predicates (declarative functions)

*   `_parent::_` (all lowercase) is a special keyword that forms the basis of a conceptual "relative" path in the PDF DOM
*   `_trailer::_` (all lowercase) is a special keyword that forms the basis of a conceptual "absolute" path in the PDF DOM. Arlington always starts with the trailer, so that trailer keys and values can be used in predicates.
*   `null` (all lowercase) is the PDF null object (_Note: it is a valid type_)
*   Change to use PDFPath ([https://github.com/pdf-association/PDFPath](https://github.com/pdf-association/PDFPath))
    *   Paths to objects are separated by `::` (double COLONs)
        *   e.g. `parent::@Key`. `Object::Key`, `Object::&lt;0-based integer&gt;`
    *   `Key`_means `key is present` (Key is case-sensitive match)
    *   `@Key` means `value of key` (Key is case-sensitive match)
*   `true` and `false` (all lowercase) are the PDF keywords (required for explicit comparison with `@key`) - uppercase `TRUE` and `FALSE` **never** get used in functions
*   All functions start with `fn:` (case-sensitive, COLON)
*   All functions are CamelCase case sensitive with BRACKETS `(` and `)` and do NOT use Digits, DASH or UNDERSCOREs (i.e. must match a simple alpha word regex)
*   Functions can have 0, 1 or 2 arguments that are COMMA separated
    *   Functions need to end with `()` for zero arguments
    *   Arguments always within `(...)`
    *   Functions can nest (as arguments of other functions)
*   Support two C/C++ style boolean operators: && (logical and), || (logical or)
*   Support six C/C++ style comparison operators: &lt;. &lt;=, &gt;, &gt;=, ==, !=
*   NO bit-wise operators - _use predicates instead_
*   NO unary NOT (`!`) operator (_implement as a presdicate `fn:Not(...)`_)
*   All expressions MUST be fully bracketed between Boolean operators (_to avoid defining precedence rules_)
*   NO conditional if/then, switch or loop style statements _ - its declarative!_
*   NO local variables_ - its declarative!_
*   Using comparison operators requires that the expression is wrapped in `fn:Eval(...)``


# Linux CLI voodoo

```shell
# List all predicates by names:
grep --color=always -ho "fn:[[:alnum:]]*." *.tsv | sort | uniq

# List all predicates and their Arguments
grep -Pho "fn:[a-zA-Z0-9]+\((?:[^)(]+|(?R))*+\)" *.tsv | sort | uniq

# List all predicates that take no parameters:
grep --color=always -Pho "fn:[a-zA-Z0-9]+\(\)" *.tsv | sort | uniq

# List all parameter lists (but not function names) (and a few PDF strings too!):
grep --color=always -Pho "\((?>[^()]|(?R))*\)" *.tsv | sort | uniq

# List all predicates with their arguments:
grep --color=always -Pho "fn:[a-zA-Z0-9]+\([^\t\]\;]*\)" *.tsv | sort | uniq
```


# EBay TSV Utilities

Any Linux command that outputs a row can be piped through `tsv-pretty` to improve readability.

```shell
# Pretty columnized output:
tsv-pretty Catalog.tsv

# Find all keys that are of "Type" 'string-byte':
tsv-filter -H --str-eq Type:string-byte *.tsv

# Only precisely 'string-byte':
tsv-filter -H --str-eq Type:string-byte --ge SinceVersion:1.5 *.tsv

# Any string type (using string-based regex):
tsv-filter -H --regex Type:string\* --ge SinceVersion:1.5 *.tsv

# "Type" includes 'string-byte':
tsv-filter -H --regex Type:.\*string-byte\* --ge SinceVersion:1.5 *.tsv
```

# Parameters to predicates

The term "reduction" is used to describe how predicates and their parameters get recursively processed from the inside
to the outer-most predicate. At any point a predicate or argument can be indeterminable. This can occur if a PDF does
not have a key, of the key is the wrong type,

When thinking about predicates, it is important to remember that not all the parameters (arguments) to predicates will
exist - thus only a portion of a predicate statement may be determinable when checking a PDF file. For example, a
predicate of the form `fn:Eval(fn:SomeThing(@A, fn:Not(@B==b))` is expecting that both the `/A` and `/B` keys will
exist in the current object so that their values can be obtained, but this may not be required (and PDFs don't always
follow requirements anyway!).

Note also that if both `/A` and `/B` are optional and both had a "DefaultValue" in TSV column 9
then this predicate would always be determinable. Further note that if a key is present but `null` then it is
the same as not present!


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
     <li>Same set as used in "SinceVersion" (column 3) and "Deprecated" (column 4)</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code><i>expr</i></code>
   </td>
   <td>
    <ul>
     <li>Mathematical expression (or a constant)</li>
     <li>Explicit bracketing <code>(...)</code> required (no order of operation!)</li>
     <li>Mathematical operators are <code>+</code> (addition), <code> - </code> (subtraction, NOT unary negation!), <code>*</code> (multiple), and <code> mod </code> (modulo). Unary negation can be achieved by multiplication with -1.</li>
     <li>References to PDF objects need <code>@</code> to get the value of key/array element</li>
     <li>Can use nested predicates that represent an integer or number (e.g. `fn:ArrayLength()`)</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code><i>cond</i></code>
   </td>
   <td>
    <ul>
     <li>A Boolean expression, including nested predicates, mathematical expressions that use comparison operators, and Boolean sub-expressions</li>
     <li>Use <code>==</code>, <code>!=</code>, <code>&gt;=</code>, <code>&lt;=</code>, <code>&gt;</code>, <code>&lt;</code> mathematical comparison operators</li>
     <li>Use <code>==</code>, <code>!=</code>, <code> && </code> (AND) or <code> || </code> (OR) logical operators (there is no NOT operator, use the `fn:Not()` predicate instead)</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code><i>statement</i></code>
   </td>
   <td>
    <ul>
     <li>A PDF object (e.g. name, string, real, integer, array, null) -e.g. <code>KeyName</code></li>
     <li>A mathematical expression (like <code><i>expr</i></code>) - e.g. <code>(fn:ArrayLength(KeyName) mod 2)</code></li>
     <li>An Arlington predefined type - if in "Type" (column 2)  - e.g. the last parameter in the predicate of <code>fn:SinceVersion(1.5,array);name</code></li>
     <li>An Arlington link (TSV filename)- if in "Link" (column 11) - e.g. the last parameter in <code>fn:SinceVersion(1.2,SomeDict)</code> which might be in the "DefaultValue" or "SpecialCase" field</li>
    </ul>
   </td>
  </tr>
</table>


# Predicates (declarative functions)

**Do not use additional whitespace!**
Single SPACE characters are only required around logical operators (` &&` and ` || `), MINUS (` - `) and the ` mod ` mathematical operators.

<table>
  <tr>
   <td><code>fn:ArrayLength(<i>key</i>)</code></td>
   <td>
    <ul>
     <li>asserts <i>key</i> references something of type <code>array</code>.</li>
     <li>If <i>key</i> exists, returns an integer value >= 0.</li>
     <li>If <i>key</i> does not exist or is not an array, returns -1.</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:ArraySortAscending(<i>key</i>,<i>integer</i>)</code></td>
   <td>
    <ul>
     <li>Asserts <i>key</i> references something of type <code>array</code>, and</li>
     <li>Asserts that the <i>integer</i>-th array elements are sorted in ascending order.</li>
     <li>Requires that all <i>integer</i>-th array elements are numeric. Other elements can be anything.</li>
     <li>An empty array is always sorted.</li>
     <li>If <i>key</i> does not exist, is not an array, or has elements that are non-numeric, returns false.</li>
     <li>e.g. <code>fn:ArraySortAscending(Index,2)</code> tests that the array elements at indices 0, 2, 4, ... are all sorted.</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:BeforeVersion(<i>version</i>)</code><br/>
   <code>fn:BeforeVersion(<i>version</i>,<i>statement</i>)</code></td>
   <td>
    <ul>
     <li><i>version</i> must be 1.1, ..., 2.0 (1.0 makes no sense!).</li>
     <li>Asserts that optional <i>statement</i> only applies before (i.e. less than) PDF <i>version</i>.</li>
     <li><i>assertion</i> is a <code>fn:Eval(...)</code> expression that only applies before <i>version</i>.</li>
     <li><i>version</i> must also make sense in light of the "SinceVersion" and "DeprecatedIn" fields for the current row (i.e. is between them).</li>      
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:BitClear(<i>bit-posn</i>)</code></td>
   <td>
    <ul>
     <li><i>bit-posn</i> is 1-32 inclusive.</li>
     <li>asserts that <i>bit-posn</i> (1-32 inclusive) is zero (clear).</li>
     <li>asserts <i>key</i> is something of type <code>bitmask</code> and the value fits in 32-bits.</li>
     <li>note that there is NO reference to a key or key-value. It is always assumed to apply to the current key.</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:BitSet(<i>bit-posn</i>)</code></td>
   <td>
    <ul>
     <li><i>bit-posn</i> is 1-32 inclusive.</li>
     <li>Asserts that <i>bit-posn</i> is one (set).</li>
     <li>Asserts <i>key</i> is something of type <code>bitmask</code> and the value fits in 32-bits.</li>
     <li>Note that there is NO reference to a key or key-value. It is always assumed to apply to the current key.</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:BitsClear(<i>low-bit</i>,<i>high-bit</i>)</code></td>
   <td>
    <ul>
     <li><i>low-bit</i> and <i>high-bit</i> must be 1-32 inclusive.</li>
     <li>Asserts that all bits between <i>low-bit</i> and <i>high-bit</i> inclusive are all zero (clear) and the value fits in 32-bits.</li>
     <li><i>low-bit</i> and <i>high-bit</i> must be different. Use <code>fn:BitClear()</code> for single bit assertions.</li>
     <li>Note that there is NO reference to a key or key-value. It is always assumed to apply to the current key. This keeps all Arlington predicates to having 2 parameters.</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:BitsSet(<i>low-bit</i>,<i>high-bit</i>)</code></td>
   <td>
    <ul>
    <li><i>low-bit</i> and <i>high-bit</i> must be 1-32 inclusive</li>
    <li>Asserts that all bits between <i>low-bit</i> and <i>high-bit</i> inclusive are all one (set) and the value fits in 32-bits.</li>
    <li><i>low-bit</i> and <i>high-bit</i> must be different. Use <code>fn:BitSet()</code> for single bit assertions.</li>
     <li>Note that there is NO reference to a key or key-value. It is always assumed to apply to the current key. This keeps all Arlington predicates to having 2 parameters.</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:Contains(<i>key-value</i>,<i>statement</i>)</code></td>
   <td>
    <ul>
     <li>Determines if a key that can be of multiple types, contains ("is") <i>statement</i></li>
     <li>Example are stream <code>Filter</code> keys which can be an array or a name, so testing this cannot just use <code>@Filter==XXX</code> as this will only work if Filter is a name  as the <code>@</code> logic returns <code>true</code> for an array to indicate existence.</li>
     <li>Always use <code>@key</code> for </i>key-value</i></li>
    </ul>
   </td>
  </tr>  
  <tr>
   <td><code>fn:DefaultValue(<i>condition</i>,<i>statement</i>)</code></td>
   <td>
    <ul>
     <li>A conditionally-based default value.
     <li>If <i>condition</i> is true, then the Default Value is specified by <i>statement</i>.</li>
     <li>If <i>condition</i> is false, then there is no Default Value specified.</li>
     <li>Only used in "DefaultValue" field (column 8).</li>
    </ul>
   </td>
  </tr>  
  <tr>
   <td><code>fn:Deprecated(<i>version</i>,<i>statement</i>)</code></td>
   <td>
    <ul>
     <li>indicates that <i>statement</i> was deprecated, such as a type ("Type" field), a value (e.g. "PossibleValues" field ) or a link</li>
     <li>The <i>version</i> is inclusive of the deprecation (i.e. when the feature was first stated it was deprecated).</li>
     <li>Obsolescence is different to deprecation: deprecation is allowed/permitted but is strongly recommended against ("should not"). Obsolescence is a "shall not" appear in a PDF.</li>
     <li><i>version</i> must also make sense in light of the "SinceVersion" and "DeprecatedIn" fields for the current row (i.e. is between them).</li>
     </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:Eval(<i>expr</i>)</code></td>
   <td>
    <ul>
     <li>In the "SpecialCase" field, always the outer-most predicate</li>
     <li>For other fields such as "Required", "IndirectRef", can be the 2nd most outer predicate (for example, directly inside <code>fn:IsRequired()</code> or <code>fn:MustBeDirect()</code>)</li>
     <li>Calculates the expression <i>expr</i>.</li>
     <li>May involve multiple terms with logical operators <code> && </code> or <code> || </code>.</li>
     <li>The result of <i>expr</i> can be anything: a numeric value, true/false, a type, a statement.</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:FileSize()</code></td>
   <td>
    <ul>
     <li>Represents the length of the "PDF file" in bytes (from <code>%PDF-<i>x.y</i></code> to last <code>%%EOF</code>).</li>
     <li>Will always be an integer > 0.</li>
     <li>There are no parameters.</li>
     <li>This may not be the same as the physical file size!</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:FontHasLatinChars()</code></td>
   <td>
    <ul>
     <li>Asserts that the current font descriptor object (the PDF object that contains the row with this predicate) has Latin characters.</li>
     <li>Checks that the PDF object has the entry <code>/Type /FontDescriptor</code>.</li>
     <li>There are no parameters.</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:Ignore()</code><br/><code>fn:Ignore(<i>expr</i>)</code></td>
   <td>
    <ul>
     <li>Asserts that the current row is to be ignored when <code><i>expr</i></code> evaluates to true, or ignored all the time (no parameter).</li>
     <li>Only used in "SpecialCase" field (column 10).</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:ImageIsStructContentItem()</code></td>
   <td>
    <ul>
     <li>Asserts that a PDF image object is a structure content item.</li>
     <li>Checks that the PDF object has the entry <code>/Subtype /Image</code>.</li>
     <li>There are no parameters.</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:ImplementationDependent()</code></td>
   <td>
    <ul>
     <li>Asserts that the current row is formally defined to be implementation dependent in the PDF specifications.</li>
     <li>There are no parameters.</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:InMap(<i>key</i>)</code></td>
   <td>
    <ul>
     <li><i>key</i> must be a map object (<code>name-tree</code> or <code>number-tree</code>).</li>
     <li>Asserts that the current row object is in the specified map.</li>
     <li>Objects are matched by their hash (object/generation number pair).</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:IsAssociatedFile()</code></td>
   <td>
    <ul>
     <li>Only used in the "Required" field (column 5)</li>
     <li>Asserts that the current row needs to be a PDF 2.0 Associated File.</li>
     <li>Realistically this means <code>fn:IsRequired(fn:SinceVersion(2.0,fn:IsAssociatedFile()) ...)</code>.</li>
     <li>There are no parameters.</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:IsEncryptedWrapper()</code></td>
   <td>
    <ul>
     <li>Only used in the "Required" field (column 5)</li>
     <li>Asserts that the current row needs to be a PDF 2.0 Encrypted Wrapper.</li>
     <li>Realistically this means <code>fn:IsRequired(fn:SinceVersion(2.0,fn:IsEncryptedWrapper()) ... )</code>.</li>
     <li>There are no parameters.</li>    
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:IsLastInNumberFormatArray(<i>key</i>)</code></td>
   <td>
    <ul>
     <li>Asserts that the current row is the last array element in a number format array (normally the immediate parent).</li>
     <li>Realistically this means <code>fn:IsMeaningful(fn:IsLastInNumberFormatArray(parent))</code></li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:IsMeaningful(<i>condition</i>)<code></td>
   <td>
    <ul>
     <li>Asserts that the current row is only "meaningful" (<b>precise quote from ISO 32000-2:2020!</b>) when <i>condition</i> is true.</li>
     <li>Only used in the "SpecialCase" field (column 10).</li>
     <li>Possibly the inverse of <code>fn:Ignore(...)</code>!</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:IsPDFTagged()</code></td>
   <td>
    <ul>
     <li>Asserts that the PDF file is a Tagged PDF.</li>
     <li>This means <code>trailer::Root::MarkInfo::Marked</code> exists and is true.</li>
     <li>There are no parameters.</li>    
    </ul>
   </td>
  </tr>
  <tr>
   <td>
    <code>fn:IsPDFVersion(<i>version</i>)</code>
    <code>fn:IsPDFVersion(<i>version</i>,<i>statement</i>)<code>
   </td>
   <td>
    <ul>
     <li>If no optional <i>statement</i>, then always true for the stated PDF version <i>version</i>.</li>
     <li>Otherwise asserts that the optional <i>statement</i> only applies to the stated PDF version <i>version</i>. This might be a type, a possible value, a new kind of linked object, etc.</li>
     <li><i>version</i> must also make sense in light of the "SinceVersion" and "DeprecatedIn" fields for the current row (i.e. is between them).</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:IsPresent(<i>key</i> or <i>expr</i>)</code></td>
   <td>
    <ul>
     <li>Asserts that <i>key</i> must be present in a PDF, or that the expression </i>expr</i> is true.</li>
     <li>e.g. <code>fn:IsPresent(StructParent)</code> or <code>fn:IsPresent(@SMaskInData>0)</code></li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:IsRequired(<i>condition</i>)</code></td>
   <td>
    <ul>
     <li>Only occurs in "Required" field and must always be the outer-most predicate.</li>
     <li><i>condition</i> is a conditional expression that resolves to a Boolean (true/false).</li>
     <li>If <i>condition</i> evaluates to true, then asserts that the current key is required.</li>
     <li>If <i>condition</i> evaluates to false, then asserts that the current key is optional.</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:KeyNameIsColorant()</code></td>
   <td>
    <ul>
     <li>Asserts that the current (arbitrary) key is also a colorant name.</li>
     <li>There are no parameters.</li>   
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
     <li>Only ever used in the "IndirectRef" field.</li>
     <li>If <i>condition</i> is true, then asserts that the current key value must be a direct object.</li>
     <li>if <i>condition</i> is false, then asserts that the current key value can be either direct or indirect.</li>
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
     <li>Only ever used in the "IndirectRef" field.</li>
     <li>If <i>condition</i> is true, the current key value must be an indirect object.
     <li>If <i>condition</i> is false, the current key value can be direct or indirect.
    </ul>
 </td>
  </tr>
  <tr>
   <td><code>fn:NoCycle()<code></td>
   <td>
    <ul>
     <li>Asserts that the PDF file shall not contain any cycles (loops) using this key to key into the linked list of objects.</li>
     <li>There are no parameters.</li>       
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:Not(<i>expr</i>)<code></td>
   <td>
    <ul>
     <li>Logical inverse of the Boolean expression argument.</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:NotStandard14Font()</code></td>
   <td>
   <ul>
    <li>Asserts that the current font object is not one of the Standard 14 Type 1 fonts.</li>
    <li>Requires /Type /Font /Subtype /Type1 and that /BaseFont is not a Standard 14 Type 1 font name.</li>
    <li>Only used in the "Required" field, as in <code>fn:IsRequired(fn:SinceVersion(2.0) || fn:NotStandard14Font())</code>.</li>
   </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:PageContainsStructContentItems()</code></td>
   <td>
    <ul>
     <li>Asserts that the page contains the structure content item represented by the integer value of the current row.</li>
     <li>Only used in the "Required" field, as in <code>fn:IsRequired(fn:PageContainsStructContentItems())</code>.</li>
     <li>There are no parameters.</li>   
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:RectHeight(<i>key</i>)</code></td>
   <td>
    <ul>
     <li><i>key</i> needs to be <code>rectangle</code> in Arlington predefined types.</li>
     <li>Returns a number >= 0.0, representing the height of the rectangle, or -1 if <i>key</i> doesn't exist.</li>
     <li>Needs to be wrapped inside the <code>fn:Eval(...)</code> predicate.</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:RectWidth(<i>rect</i>)</code></td>
   <td>
    <ul>
    <li><i>key</i> needs to be <code>rectangle</code> in Arlington predefined types.</li>
    <li>Returns a number >= 0.0, representing the width of the rectangle, or -1 if <i>key</i> doesn't exist.</li>
    <li>Needs to be wrapped inside the <code>fn:Eval(...)</code> predicate.</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:RequiredValue(<i>expr</i>,<i>value</i>)<code></td>
   <td>
    <ul>
     <li>Only used in the "PossibleValue" field to indicate if a specific value is required under a specific condition.</li>
     <li>Asserts that the current row must by <i>value</i> when <i>expr</i> evaluates to true.</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:SinceVersion(<i>version</i>)</code></br><code>fn:SinceVersion(<i>version</i>,<i>statement</i>)</code></td>
   <td>
    <ul>
     <li>If no optional <i>statement</i>, then always true for the stated PDF version <i>version</i> and later.</li>
     <li>Otherwise asserts that the optional <i>statement</i> applies from the stated PDF version <i>version</i> (inclusive). This might be a type, a possible value, a new kind of linked object, etc.</li>
     <li><i>version</i> must also make sense in light of the "SinceVersion" and "DeprecatedIn" fields for the current row (i.e. is between them).</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:StreamLength(<i>key</i>)</code></td>
   <td>
    <ul>
     <li><i>key</i> needs to be a stream and returns an integer >= 0, or -1 if not present.</li>
     <li>Uses the value of the streams' <code>/Length</code> key, rather than reading and decoding actual streams.</li>
     <li>Needs to be wrapped inside <code>fn:Eval(...)</code>.</li>
     <li>e.g. <code>fn:Eval(fn:StreamLength(DL)==(@Width * @Height))</code></li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:StringLength(<i>key</i>)</code></td>
   <td>
    <ul>
     <li><i>key</i> needs to be a string object and returns an integer >= 0, or -1 on error.</li>
     <li>Needs to be used inside <code>fn:Eval(...)</code>.</li>
     <li> e.g. <code>fn:Eval(fn:StringLength(Panose)==12)</code>
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

- Apply all approved Errata for ISO 32000-2:2020 from https://pdf-issues.pdfa.org/
