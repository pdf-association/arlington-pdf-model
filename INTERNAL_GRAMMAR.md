# Arlington PDF Model Grammar Validation Rules

This document describes some strict rules for the Arlington PDF model, for both the data and the predicates (custom declarative predicates that start `fn:`). Only some of these rules are currently implemented by various PoCs, but everything is precisely documented here.


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
* all TSV files will have matching numbers of `[`, `]` and `(`, `)`
* for a single row in any TSV, spliting each field on ';' will either result in 1 or _N_.
* files that represent PDF arrays match either `ArrayOf*.tsv`, `*Array.tsv` or `*ColorSpace.tsv`
    - many are also identifiable by having a Key name of `0` (or `0*` or `*`)
    ```shell
    grep "^0" *
    ```
* files that represent PDF 'map' objects (meaning that the dictionary key name can be anything) match `*Map.tsv`
    - note that CMaps are in `CMapStream.tsv`
* **NOT** all files that are PDF stream objects match `*Stream.tsv`
    - since each Arlington object is fully self-contained, many objects can be streams. The best method is to search for `DecodeParms` key instead:
    ```shell
    grep "^DecodeParms" * | tsv-pretty
    ```

# PDF Object conventions

* There are NO leading SLASHES for PDF names (_ever_!)
* PDF names don't use `#`-escaping (currently unsupported)
* PDF strings use single quotes `'` and `'` (since `(` and `)` are ambiguous with expressions and single quotes are supported natively by Python `csv` module)
* Expressions with integers need to use integers. Integers can be used in place of numbers.
* `*` represents a wildcard (i.e. anything). Other regex are not supported. Wildcards can be used in the Key field and in the PossibleValues field for names (when the PDF standard specifically states that other arbitrary names can be used)
* Leading `@` indicates "_value of_" a key or array element
* PDF Booleans are `true` and `false` lowercase.
    - Uppercase `TRUE`/`FALSE` are reserved for logical Boolean TSV data fields such as the "Required" field.
* expressions using `&&` or `||` logical operators need to be either fully bracketed or be just a predicate and have a single SPACE either side of the logical operator. precedence rules are NOT implemented.
* the predefined Arlington paths `parent::` and `trailer::` represent the parent of the current object and file trailer (either traditional or a cross-reference stream) respectively. All other paths are relative from the containing PDF object
* PDF arrays always use `[` and `]` (which may require some additional processing so as not to be confused with our [];[];[] syntax for complex fields)
    - elements in a PDF array do **not** use COMMA-separators and are specified just like in PDF e.g. `[0 1 0]`
    - if a PDF array needs to be specified as part of a complex typed key (`[];[];[]`) then 2 sets of `[` and `]` need to be used for the array values
        - e.g. `[[0 1]];[123];[SomeThing]` might be a Default Value for a PDF key that can be an array, an integer or a name (alphabetically sorted in the "Type" field!) each with a default value.
        - this extra pair of `[` and `]` is only needed for complex types.


# TSV Data Fields

*  A key or array element is so-called "complex" if it can be multiple values. This is represented by `[];[];[]`-type expressions.
*  Something is so called a "wildcard" if the "Key" field contains an ASTERISK.
*  An array is so-called a "repeating array" if it requires _N_ x a set of elements. This is represented by DIGIT+ASTERISK in the "Key" field.
    - Repeating array elements with DIGIT+ASTERISK must be the _last_ rows in a TSV
    - e.g. `0*` `1*` `2*` would be an array of 3 * _N_ triplets of elements
    - e.g. `0` `1*` `2*` would be an array of 2 * _N_ + 1 elements, where the first element has a fixed definition, followed by repeating pairs of elements


## Column 1 - "Key"

*   Must not be blank
*   Case sensitive (as per PDF spec)
*   No duplicates keys in any single TSV file
*   Only alphanumeric, `.`, `-`, `_` or ASTERISK characters (no whitespace or other special characters)
    * The proprietary Apple APPL extensions also use `:` (COLON) as in `AAPL:ST`
*   If a dictionary, then "Key" may also be an ASTERISK `*` meaning wildcard, so anything is allowed
*   If ASTERISK `*` by itself then must be last row in TSV file
*   If ASTERISK `*` by itself then "Required" column must be FALSE
*   If representing a PDF array, then "Key" name is really an integer array index.
    - Zero-based increasing (always by 1) integers always starting at ZERO (0), with an optional ASTERISK appended after the digit (indicating repeat)
    - Or just an ASTERISK `*` meaning that any number of array elements may exist
*   If representing a PDF array with a repeating set of array elements (such as alternating pairs of elements) then use `digit+ASTERISK` where the last set of rows must all be `digit+ASTERISK` (indicating a repeating group of _N_ elements starting at array element _M_ (so array starts with a fixed set (non-repeating) array elements 0 to _M_-1, followed by the repeating set of element _M_ to (_M_ + _N_-1)) array elements).
*   If representing a PDF array with `digit+ASTERISK`  then the "Required" column should be TRUE if all _N_ entries must always be repeated as a full set (e.g. in pairs or quads).
*   **Python pretty-print/JSON**
    *   String (as JSON dictionary key)
*   **Linux CLI tests:**
    ```shell
    # List of all key names and array indices
    cut -f 1 * | sort -u
    ```
* files that define objects with an arbitrary number of keys or array elements use the wildcard `*`. If the line number of the wildcard is line 2 then it is a map-like object. If the line number is after 2, then are additional fixed keys/elements.
    ```shell
    grep --line-number "^\*" * | sed -e 's/\:/\t/g' | tsv-pretty
    ```
* files that define arrays with repeating sequences of _N_ elements use the `digit+ASTERISK` syntax. Digit is currently restricted to a SINGLE digit 0-9.
   ```shell
   grep "^[0-9]\*" * | tsv-pretty
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
*   Each type may also be wrapped in a version-based predicate (e.g. `fn:SinceVersion(version,type)` or
    `fn:Deprecated(version,type)`).
*   When a predicate is used, the internal simple type is still kept in its alphabetic sort order
* The following predefined Arlington types ALWAYS REQUIRE a link:
    - `array`, `dictionary`, `stream`
* The following predefined Arlington types MAY have a link (this is because name and number trees can have nodes which are the primitive Arlington types below or a complex type above):
    - `name-tree`, `number-tree`
    - e.g. `Navigator\Strings` is a name-tree of string objects
* The following predefined Arlington types NEVER have a link (they are the primitive Arlington types):
    - `bitmask`, `boolean`, `date`, `integer`, `matrix`, `name`, `null`, `number`, `rectangle`, `string`, `string-ascii`, `string-byte`, `string-text`
*   **Python pretty-print/JSON:**
    *   Always a list
    *   List elements are either:
        *   Strings for the basic types listed above
        *   Python lists for predicates - a simple search through the list for a match to the types above is
            sufficient (if understanding the predicate is not required)
    *   _Not to be confused with "/Type" keys which is why the `[` is included in this grep!_
    *   `grep "'Type': \[" dom.json | sed -e 's/^ *//' | sort -u`
*   **Linux CLI tests:**
    ```shell
    cut -f 2 * | sort -u
    cut -f 2 * | sed -e "s/;/\n/g" | sort -u
    ```


## Column 3 - "SinceVersion"

*   Must not be blank
*   Must resolve to one of `1.0`, `1.1`, ... `1.7` or `2.0`
*   Can be a predicate such as `fn:Extension(...)` or `fn:Eval(...)`
    - e.g. `fn:Extension(XYZ,2.0)` or `fn:Eval(fn:Extension(XYZ,1.3) || 1.6)`
*   In the future the set of versions may be increased - e.g. `2.1`
* Version-based predicates in other fields should all be based on versions explicitly AFTER the version in this column
*   **Python pretty-print/JSON**
    *   Always a string (never blank!)
    *   Value is one of the values listed above
    *   `grep "'SinceVersion'" dom.json | sed -e 's/^ *//' | sort -u`
*   **Linux CLI tests:**
    ```shell
    cut -f 3 * | sort -u
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
    *   `grep "'Deprecated': " dom.json | sed -e 's/^ *//' | sort -u`
*   **Linux CLI tests:**
    ```shell
    cut -f 4 * | sort -u
    ```

## Column 5 - "Required"

*   Must not be blank
*   Either:
    *   Single word: `FALSE` or `TRUE` (uppercase only)
    *   The predicate `fn:IsRequired(...)` - no SQUARE BRACKETS!
        *   This may then have further nested predicates (e.g. `fn:SinceVersion`, `fn:IsPresent`, `fn:Not`)
*   If "Key" column contains ASTERISK (as a wildcard), then "Required" field must be FALSE
    *   Cannot require an infinite number of keys! If need at least one element, then have explicit first rows
        with "Required"==`TRUE` followed by ASTERISK with "Required"==`FALSE`)
*   **Python pretty-print/JSON:**
    *   Always a list
    *   List length is always 1
    *   List element is either:
        *   Boolean
        *   Python list for predicates which must be `fn:IsRequired(`
    *   `grep "'Required': " dom.json | sed -e 's/^ *//' | sort -u`
*   **Linux CLI tests:**
    ```shell
    cut -f 5 * | sort -u
    ```


## Column 6 - IndirectReference

*   Must not be blank
*   Streams must always have "IndirectReference" as `TRUE`
*   For name- and number-trees, the value represents what the direct/indirect requirements of the values of tree (e.g. if it is a stream, it would be `TRUE`) 
*   Either:
    *   Single word: `FALSE` or `TRUE` (uppercase only, as it is not a PDF keyword!); or
    *   Single predicate `fn:MustBeDirect()` or `fn:MustBeIndirect()` indicating that the corresponding key/array element must
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
        *   Python list for predicates where the outer-most predicate must be `fn:IsRequired(`, with
            an optional argument for a condition
    *   `grep "'IndirectReference':" dom.json | sed -e 's/^ *//' | sort -u`
*   **Linux CLI tests:**
    ```shell
    cut -f 6 * | sort -u
    ```


## Column 7 - Inheritable

*   Must not be blank
*   Single word: `FALSE` or `TRUE` (uppercase only, as it is not a PDF keyword!)
*   **Python pretty-print/JSON:**
    *   Always a boolean
    *   `grep "'Inheritable'" dom.json | sed -e 's/^ *//' | sort -u`
*   **Linux CLI tests:**
    ```shell
    cut -f 7 * | sort -u
    ```


## Column 8 - DefaultValue

*   Represents a default value for the PDF key/array element. As such it is always a _single value_ for each Type.
    * see "PossibleValues" field below for when multiple values need to be specified.
*   Can be blank
*   SQUARE-BRACKETS are also used for PDF arrays, in which case they must use double SQUARE-BRACKETS if part of a complex type (not that lowercase `true`/`false` are the PDF keywords). If the array is the only valid type, then single SQUARE-BRACKETS are used.  PDF array elements are NOT separated with COMMAs.
    *  e.g. `[[false false]];[123]` vs `[false false]`
    * thus a complex expression can first be split by SEMI-COLON, then each portion has the SQUARE-BRACKETS stripped off - any remaining SQUARE-BRACKETS indicate an array.
*   If there is a "DefaultValue" AND there are multiple types, then require a complex `[];[];[]` expression
    *  If the "DefaultValue" is a PDF array _as part of a complex type_, then this will result in nested SQUARE-BRACKETS as in `[];[[0 0 1]];[]`
*   The only valid predicates are:
    *  `fn:ImplementationDependent()`, or
    *  `fn:DefaultValue(condition, value)` where _value_ must match the appropriate type (e.g. an integer for an integer key, a string for a string-\* key, etc), or
    *  `fn:Eval(expression)`
    *  Predicates only need [];[];[] expression if a multi-typed key
*   **Python pretty-print/JSON:**
    *   A list or `None`
    *   If list, then length always matches length of "Type"
    *   If list element is also a list then it is either:
        *   Predicate with 1st element being a FUNC_NAME token
        *   "Key" value (`@key`) with 1st element being a KEY_VALUE token
        *   A PDF array (1st token is anything else) - including an empty PDF array
    *   `grep -o "'DefaultValue': .*" dom.json | sed -e 's/^ *//' | sort -u`
*   **Linux CLI tests:**
    ```shell
    cut -f 8 * | sort -u
    cut -f 2,8 * | sort -u | grep -P "\t[[:graph:]]+.*" | tsv-pretty
    cut -f 1,2,8 * | sort -u | grep -P "\t[[:graph:]]*\t[[:graph:]]+.*$" | tsv-pretty
    ```


## Column 9 - "PossibleValues"

*   Can be blank
*   SQUARE-BRACKETS are only required for complex types. A single type does not use them.
    - e.g. `12.34` is a valid default for a key which can only be a number
*   SEMI-COLON separated, SQUARE-BRACKETS expressions that exactly match the number of items in "Type" column
*   SQUARE-BRACKETS are also used for PDF arrays, in which case they must use double SQUARE-BRACKETS if part of a complex type. If the array is the only valid type, then single SQUARE-BRACKETS are used. PDF array elements are NOT separated with COMMAs - they are only used _between_ arrays.
    *  e.g. `[[0 1],[1 0]];[Value1,Value2,Value3]` is a choice of 2 arrays `[0 1]` and `[1 0]` if the type is an array or a choice of `Value1` or `Value2` or `Value3` if the type was something else (e.g. name)
    * thus a complex expression can first be split by SEMI-COLON, then each portion has the SQUARE-BRACKETS stripped off, then multiple options can be split by COMMA as any remaining SQUARE-BRACKETS indicate an array.
*   If there is a "DefaultValue" AND there are multiple types, then require a complex `[];[];[]` expression
    *  If the "DefaultValue" is a PDF array _as part of a complex type_, then this will result in nested SQUARE-BRACKETS as in `[];[[0 0 1]];[]`
*   For keys or arrays that are PDF names, a wildcard `*` indicates that any arbitrary name is _explicitly_ permitted according to the PDF specification along with formally defined values (e.g. OptContentCreatorInfo, Subtype key: `[Artwork,Technical,*]`).
    *  Do not use `*` as the _only_ value - since an empty cell has the same meaning as "anything is OK" although there is some subtle nuances regarding whether custom keys have to be 2nd class names or can be really anything. See [Errata #229](https://github.com/pdf-association/pdf-issues/issues/229)
    * The TestGrammar PoC will no longer report an error about unexpected values in this case, but produce an informational (`Info:`) message instead (so it is visible that a non-standard value is being used).
*   **Python pretty-print/JSON:**
    *   A list or `None`
    *   If list, then length always matches length of "Type"
        *   Elements can be anything, including `None`
    ```shell
    grep -o "'PossibleValues': .*" dom.json | sed -e 's/^ *//' | sort -u
    ```
*   **Linux CLI tests:**    
    ```shell
    # Lists those PDF objects which explicitly support any name
    grep -P ",\*" *
    ```


## Column 10 - "SpecialCase"

*   Can be blank
*   SEMI-COLON separated, SQUARE-BRACKETED complex expressions that exactly match the number of items in "Type" column
*   Each expression inside a SQUARE-BRACKET is a predicate that reduces to TRUE/FALSE or is indeterminable.
    * TRUE means that it is a valid, FALSE means it would be invalid.
*   A SpecialCase predicate is not meant to reflect all rules from the PDF specification (_things are declarative, not programmatic!_)
    * It should not test for required/optional-ness, whether an object is indirect or not, etc. as those rules should live in the other fields
*   **Python pretty-print/JSON:**
    *   A list or `None`
    *   If list, then length always matches length of "Type"
        *   Elements can be anything, including `None`
    ```shell
    grep -o "'SpecialCase': .*" dom.json | sed -e 's/^ *//' | sort -u
    ```


## Column 11 - "Link"

*   Can be blank (but only when "Type" is a single basic type)
*   If non-blank, always uses SQUARE-BRACKETS
*   SEMI-COLON separated, SQUARE-BRACKETED complex expressions that exactly match the number of items in "Type" column
*   Valid "Links" must exist for these selected object types only:
    * `array`
    * `dictionary`
    * `stream`
    * `name-tree` - the value represents the node in the tree, not how trees are specified
    * `number-tree` - the value represents the node in the tree, not how trees are specified
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
*   These sub-expressions MUST BE one of these version-based predicates:
    *   `fn:SinceVersion(pdf-version,link)`
    *   `fn:SinceVersion(pdf-version,fn:Extension(name,link))`
    *   `fn:IsPDFVersion(pdf-version,fn:Extension(name,link))`
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
    # A list of all predicates used in the Link field (column 11)
    cut -f 11 * | sort -u | grep -o "fn:[a-zA-Z0-4]*" | sort -u
    ```

## Column 12- "Notes"

*   Can be blank
*   Free text - no validation possible
*   Often contains a reference to Table(s) (search for case sensitive "Table ") or clause number(s) (search for case sensitive "Clause ") from ISO 32000-2:2020 (PDF 2.0) or a PDF Association Errata issue link (as GitHub URL) where the Arlington machine-readable definition is defined.
    * For dictionaries, this is normally on the first key on the `Type` or `Subtype` row depending on what is the primary differentiating definition
    * Note that this is **not** where an object is referenced from, but where its key and values are **defined**. Sometimes this is within body text prose of ISO 32000-2:2020 (so outside a Table and a Clause reference is used) or as prose within the "Description" cell of some other key in another Table. _Where_ an object is referenced is encoded by the Arlington PDF Model "Link" field - just grep for the case-sensitive TSV file (no extension)!
*   The spreadsheet [Arlington-vs-ISO32K-Tables.xlsx](Arlington-vs-ISO32K-Tables.xlsx) provides a cross reference from all mentions of "Table" within the Arlington PDF Model against the an index of every Table in ISO 32000-2:2020 as published by ISO. Tables that are not mentioned anywhere in Arlington TSV files _may_ indicate poor coverage in the Arlington PDF Model - or that the table is inappropriate for incorporating into the Arlington PDF Model.
    * Current known limitations include no support for FDF; less-than-perfect definition for Linearization objects; and no definition of content streams.
    * Note also that Arlington does additionally reference other ISO and Adobe publications, sometimes also with specific clause and Table references (such as for Adobe Extension Level 3).
*   **Python pretty-print/JSON:**
    *   A string or `None`
*   **Linux CLI voodoo:**
    ```shell
    # Find all TSV files in a data set that do not have either a Table number or Clause reference
    grep -PL "(Table )|(Clause )" *
    # A list of most (but not all!) Table numbers referenced in an Arlington TSV file set. Does not capture Annex tables.
    grep --color=none -Pho "(?<=Table) [0-9]+" * | sort -un
    # Some PDF objects are defined by prose in clauses, rather than Tables
    grep -Pho "Clause [0-9A-H\.]*" * | sort -u
    # Find all ISO publication that are explicitly referenced
    grep -Pho "ISO[^_]*$" * | sort -u
    ```


# Validation of predicates (declarative functions)

First and foremost, the predicate system is not based on **functional programming**!

The best way to understand an expression with a predicate is to read it out aloud, from left to right.
Its verbalization should relatively closely match wording found in the PDF specification.
Predicate simplifcation is **avoided** so that wording (when read aloud) is kept as close as possible to wording in the PDF specification.


* the internal Arlington grammar is loosely typed (so things need to match or be interpreted as matching the "Type" field (column 2)).
    * integers may be used in place of numbers (_but not vice-versa!_)
* `_parent::_` (all lowercase) is a special Arlington grammar keyword that forms the basis of a conceptual "relative" path in the PDF DOM. There can be multiple `parent::`s.
* `_trailer::_` (all lowercase) is a special Arlington grammar keyword that forms the basis of a conceptual "absolute" path in the PDF DOM. Arlington always starts with the trailer, so that trailer keys and values can also be used in predicates.
    * `trailer::Catalog` is a special Arlington alias for `trailer::Root`, as the **Root** key in the trailer is the reference to the Document Catalog, however normal PDF terminology refers to the "Document Catalog" and so that commonly understood term is preferred over the ambiguous word "root" (as that could ambiguously mean either the trailer as the root or the Document Catalog as the root) - and reading aloud "Catalog" sounds more natural. 
       - Either `trailer::Catalog` or `trailer::Root` can be used, but the preference is `trailer::Catalog` because it verbalises better
* `null` (all lowercase) is the PDF **null** object (_Note: it is also valid predefined Arlington type_).
    * `null` gets used in "DefaultValue" or "PossibleValue" fields only when it is explicitly mentioned in the PDF specification.
* `Key` means `key is present` (`Key` is case-sensitive match and may include an Arlington path)
* `@Key` means `the value of key` (`Key` is case-sensitive match and may include an Arlington path).
    * this also applies after a `path` - e.g. `keyA::keyB::@keyC` is valid and is the value of `keyC` when the path `KeyA::keyB` is traversed
*   Arlington paths are separated by `::` (double COLONs)
    * e.g. `parent::@Key`. `KeyA::KeyB`, `trailer::Catalog::Size`, `Object::&lt;0-based integer&gt;`
    * the `@` operator only applies to the right-most portion
    * The `@` sign is always required for math and comparison operations, since those operate on values.
       - if an array or stream length is needed then use the specific predicate
    * The predefined Arlington types used with `@` are the primitive types such as boolean, integer, number, string-*, name, etc.
    * It is also possible to use the key name of an array for certain predicates such as `fn:Contains(...)`
    * For complex types, if the "DefaultValue" for KeyA is `@KeyB` then it means that the "default value for Key A is the value of Key B" and so long as Keys A and B both have the same type then this is logical.
* `true` and `false` (all lowercase) are the PDF keywords (required for explicit comparison with `@key`) - uppercase `TRUE` and `FALSE` **never** get used in predicates as they represent Arlington model values such as for "Required", "IndirectReference" or "Inheritable" fields.
* All predicates start with `fn:` (case-sensitive, single COLON) followed by an uppercase character (`A`-'Z')
* All predicate names are CamelCase case sensitive with BRACKETS `(` and `)` and do NOT use DASH or UNDERSCOREs (i.e. must match a simple alphanumeric regex)
* Predicates can have 0, 1 or 2 arguments that are always COMMA separated
    * Predicates need to end with `()` for zero arguments
    * Arguments always within `(...)`
    * Predicates can nest (as arguments of other Predicates)
* Support two C/C++ style boolean operators: `&&` (logical and), `||` (logical or). There is also a special `fn:Not(...)` predicate.
* Support six C/C++ style comparison operators: &lt;. &lt;=, &gt;, &gt;=, ==, !=
* NO bit-wise operators - _use predicates instead_
* NO unary NOT (`!`) operator (_use predicate `fn:Not(...)`_)
* All expressions MUST be fully bracketed between Boolean operators (_to avoid defining precedence rules_)
* NO conditional if/then, switch or loop style statements - _its purely declarative!_
* NO local variables - _its purely declarative!_
* Using comparison operators requires that the full expression is wrapped in `fn:Eval(...)`

# Linux CLI voodoo

```bash
# List all predicates by names:
grep --color=always -ho "fn:[[:alnum:]]*" * | sort -u

# List all predicates and their Arguments
grep -Pho "fn:[a-zA-Z0-9]+\((?:[^)(]+|(?R))*+\)" * | sort -u

# List all predicates that take no parameters:
grep --color=always -Pho "fn:[a-zA-Z0-9]+\(\)" * | sort -u

# List all parameter lists (but not predicate names) (and a few PDF strings too!):
grep --color=always -Pho "\((?>[^()]|(?R))*\)" * | sort -u

# List all predicates with their arguments:
grep --color=always -Pho "fn:[a-zA-Z0-9]+\([^\t\]\;]*\)" * | sort -u
```


# EBay TSV Utilities

Any Linux command that outputs a row from an Arlington TSV data file can be piped through `tsv-pretty` to improve readability.

```bash
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

# Find all annotations which have the ExData key
grep ^ExData Annot* | tsv-pretty
```

# Parameters to predicates

The term "reduction" is used to describe how predicates and their parameters get recursively processed from left-to-right.
At any point a predicate or argument can be indeterminable, such as when a PDF does not have a key, or if the key is the wrong type, etc.

When thinking about predicates, it is important to remember that not all the parameters (arguments) to predicates will
exist - thus only a portion of a predicate statement may be determinable when checking a PDF file. For example, a
predicate of the form `fn:Eval(fn:SomeThing(@A, fn:Not(@B==b))` is expecting that both the `/A` and `/B` keys will
exist in the current object so that their values can be obtained, but this may not be required (and PDFs don't always
follow requirements anyway!).

Note also that if both `/A` and `/B` are optional and both had a "DefaultValue" in TSV column 9
then this predicate would always be determinable. Further note that if a key is present but `null` then it is
the same as not present!


<table style="border: 0.25px solid; border-collapse: collapse;">
  <tr>
   <td><code><i>bit-posn</i></code>
   </td>
   <td>
    <ul>
     <li>bits are numbered 1-32 inclusive.</li>
     <li>bit 1 is the low-order bit.</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code><i>version</i></code>
   </td>
   <td>
    <ul>
     <li>One of "1.0, "1.1", ..., "1.7", and "2.0" currently.</li>
     <li>Same set as used in "SinceVersion" (column 3) and "Deprecated" (column 4).</li>
    </ul>
   </td>
  </tr>
</table>


# Predicates (declarative functions)

**Do not use additional whitespace!**

Single SPACE characters are only required around logical operators ("&nbsp;`&&`&nbsp;" and "&nbsp;`||`&nbsp;"), MINUS ("&nbsp;`-`&nbsp;", to disambiguate from a negative number) and the "&nbsp;`mod`&nbsp;" mathematical operator.


<table style="border: 0.25px solid; border-collapse: collapse;">
  <tr>
   <td><code>fn:AlwaysUnencrypted()</code></td>
   <td>
    <ul>
     <li>Asserts that the current key or array element is a PDF string object and is always unencrypted when the PDF file itself is encrypted.</li>
     <li>There are no parameters.</li>
     <li>Read aloud as: "&lt;<i>current key</i>&gt; shall always be unencrypted."</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:ArrayLength(<i>key</i>)</code></td>
   <td>
    <ul>
     <li>Asserts that <i>key</i> exists and is an array, and returns the array length as an integer value >= 0.</li>
     <li>There is only one parameter and it must be an <code>array</code>.</li>
     <li>Read aloud as: "... the length of array &lt;<i>key</i>&gt; shall ..."</li>
    </ul>
   </td>
  </tr>  
  <tr>
   <td><code>fn:ArraySortAscending(<i>key</i>,<i>integer</i>)</code></td>
   <td>
    <ul>
     <li>Asserts <i>key</i> references something of type <code>array</code>, and</li>
     <li>Asserts that the <i>integer</i>-th array elements are sorted in ascending order.</li>
     <li>Requires that all <i>integer</i>-th array elements are numeric. Other array elements however can be anything.</li>
     <li>An empty array will be considered sorted.</li>
     <li><i>integer</i> is 1 or greater. 1 means all array elements, 2 means every second element, 3 means every 3rd element, etc. It does not imply anything about the array lenegth, as an empty array is considered logically sorted.</li>
     <li>e.g. <code>fn:ArraySortAscending(Index,2)</code> tests that the array elements at indices 0, 2, 4, ... are all sorted.</li>
     <li>Read aloud as (approx.): "... the &lt;<i>integer</i>&gt;-th elements in array &lt;<i>key</i>&gt; shall all be sorted in ascending order ..."</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:BeforeVersion(<i>version</i>)</code><br/>
   <code>fn:BeforeVersion(<i>version</i>,<i>statement</i>)</code></td>
   <td>
    <ul>
     <li><i>version</i> must be 1.1, ..., 2.0 (1.0 makes no sense!).</li>
     <li>Asserts that the optional <i>statement</i> only applies before (i.e. strictly less than) PDF <i>version</i>.</li>
     <li><i>version</i> must also make sense in light of the "SinceVersion" and "DeprecatedIn" fields for the current row (i.e. is between them).</li>      
     <li>Read aloud as: "... prior to PDF version &lt;<i>version</i>&gt;, &lt;<i>statement</i>&gt; ..."</li>
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
     <li>Read aloud as: "... bit position &lt;<i>bit-posn</i>&gt; of <i>current key</i> shall be clear (zero) ..."</li>
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
     <li>Read aloud as: "... bit position &lt;<i>bit-posn</i>&gt; of <i>current key</i> shall be set (one) ..."</li>
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
     <li>Note that there is NO reference to a key or key-value. It always applies to the current key which must be of type <code>bitmask</code>. This keeps all Arlington predicates to having 2 parameters.</li>
     <li>Read aloud as: "... bit positions from &lt;<i>low-bit</i>&gt; to &lt;<i>high-bit</i>&gt; inclusive of <i>current key</i> shall be clear (zero) ..."</li>
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
     <li>Note that there is NO reference to a key or key-value. It is always assumed to apply to the current key which must be of type <code>bitmask</code>. This keeps all Arlington predicates to having 2 parameters.</li>
     <li>Read aloud as: "... bit positions from &lt;<i>low-bit</i>&gt; to &lt;<i>high-bit</i>&gt; inclusive of <i>current key</i> shall be set (one) ..."</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:Contains(<i>@key</i>,<i>value</i>)</code></td>
   <td>
    <ul>
     <li>Asserts that <i>key</i> can be an <code>array</code> object (and thus can hold multiple values) containing <i>value</i>, or a PDF basic object (such as a <code>name</code>) that can have the value <i>value</i>.</li>
     <li>Specific use-case are stream <code>Filter</code> keys which can be an array or a name, so testing this cannot just use <code>@Filter==XXX</code> as this will only work if Filter is a <code>name</code> as the <code>@</code> logic returns <code>true</code> for an array to indicate existence.</li>
     <li>Always use <code>@array-key</code> for </i>array-key</i></li>
     <li>Read aloud as: "... the value of &lt;<i>key</i>&gt; shall be &lt;<i>value</i>&gt;, or if &lt;<i>key</i>&gt; is an array it shall contain an array element equal to &lt;<i>value</i>&gt;, ..."</li>
    </ul>
   </td>
  </tr>  
  <tr>
   <td><code>fn:DefaultValue(<i>condition</i>,<i>value</i>)</code></td>
   <td>
    <ul>
     <li>States a conditionally-based default value.</li>
     <li>When <i>condition</i> is true, then the Default Value is specified by <i>value</i>.</li>
     <li>Only used in "DefaultValue" field (column 8).</li>
     <li>Read aloud as: "The default value of <i>current-key</i> shall be &lt;<i>value</i>&gt;when &lt;<i>condition</i>&gt;."</li>
    </ul>
   </td>
  </tr>  
  <tr>
   <td><code>fn:Deprecated(<i>version</i>,<i>statement</i>)</code></td>
   <td>
    <ul>
     <li>indicates that <i>statement</i> was deprecated, such as a type (in "Type" field), a value (e.g. in "PossibleValues" or "SpecialCases" field ) or as a link in the "Links" field.</li>
     <li>The <i>version</i> is inclusive of the deprecation (i.e. when the feature was first stated it was deprecated).</li>
     <li>Obsolescence is different to deprecation in ISO 32000: deprecation is allowed/permitted but is strongly recommended against ("should not"). Obsolescence is a "shall not" appear in a PDF and documentation has been removed.</li>
     <li><i>version</i> must also make logical sense in light of the "SinceVersion" and "DeprecatedIn" fields for the current row (i.e. is between them).</li>
     <li>Read aloud as: "&lt;<i>statement</i>&gt; was deprecated in PDF version &lt;<i>version</i>&gt;."</li>
     </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:Eval(<i>expr</i>)</code></td>
   <td>
    <ul>
     <li>In the "SpecialCase" field, always the outer-most predicate</li>
     <li>For other fields such as "Required", "IndirectRef", can be the 2nd most outer predicate (for example, directly inside <code>fn:IsRequired()</code> or <code>fn:MustBeDirect()</code>)</li>
     <li>Evaluates the expression <i>expr</i> that may involve multiple terms with logical operators <code> && </code> or <code> || </code>.</li>
     <li>The result of <i>expr</i> can be anything: a numeric value, true/false, a type, a statement but must be appropriate to its usage.</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:Extension(<i>name</i>)</code><br/>
       <code>fn:Extension(<i>name</i>,<i>value</i>)</code></td>
   <td>
    <ul>
     <li>Used in the "SinceVersion", "PossibleValues" or "SpecicalCase" fields.</li>
     <li><i>name</i> is an arbitrary identifier for the extension or subset and uses the same lexical conventions as for the "Key" field (e.g. no SPACEs).</li>
     <li>In the "SinceVersion" field must reduce down to a valid PDF version for when the key or array element or which extension <i>name</i> introduced the key/array element. This may be combined with <i>value</i> to express a version-based introduction such as ISO subsets:</li>
     <ul>
       <li><code>fn:Extension(XYZ)</code> - under extension XYZ for any PDF version</li>
       <li><code>fn:Extension(XYZ,1.5)</code> - under extension XYZ but only since PDF 1.5 (inclusive)</li>
       <li><code>fn:Eval(fn:Extension(XYZ,1.6) || 2.0)</code> - under extension XYZ since PDF 1.6, but then became a standardized feature since PDF 2.0</li>
     </ul>
     <li>In other fields such as "PossibleValues" or "SpecialCase" identifies that a specific value for the key or array element is only valid for the specified extension <i>name</i>. This may be combined with <code>fn:SinceVersion</code> to express a more nuanced introduction</li>
     <ul><li>e.g. <code>fn:SinceVersion(2.0,fn:Extension(ISO_TS_12345,AESV99))</code></li></ul>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:FileSize()</code></td>
   <td>
    <ul>
     <li>Represents the length of the "PDF file" in bytes (from <code>%PDF-<i>x.y</i></code> to last <code>%%EOF</code> but in reality depends on PDF SDK).</li>
     <li>Will always be an integer > 0.</li>
     <li>There are no parameters.</li>
     <li>This may not be the same as the physical file size!</li>
     <li>This is mostly used as an upper integer bound for values that represent byte offsets.</li>
     <li>Read aloud as: "... length of the PDF file in bytes."</li>
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
     <li>Read aloud as: "... the font shall contain Latin characters."</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:HasProcessColorants(<i>array</i>)</code></td>
   <td>
    <ul>
     <li>Asserts that the given array object of PDF names contains at least one process colorant name (Cyan, Magenta, Yellow or Black).</li>
     <li>Read aloud as: "... array &lt;<i>array</i>&gt; of names shall contain at least one process colorant name."</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:HasSpotColorants(<i>array</i>)</code></td>
   <td>
    <ul>
     <li>Asserts that the given array object of PDF names contains at least one spot colorant name. A spot colorant is any name besides the CMYK colorant names.</li>
     <li>Read aloud as: "... array &lt;<i>array</i>&gt; of names shall contain at least one spot colorant name."</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td>
    <code>fn:Ignore()</code><br/>
    <code>fn:Ignore(<i>condition</i>)</code></td>
   <td>
    <ul>
     <li>Zero or one parameters.</li>
     <li>Asserts that the current row (key or array element) is to be ignored when <code><i>condition</i></code> evaluates to true, or ignored all the time (no parameter).</li>
     <li>Only used in "SpecialCase" field (column 10).</li>
     <li>Read aloud as: "<i>current-key</i> shall be ignored when &lt;<i>condition</i>&gt;."</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:ImageIsStructContentItem()</code></td>
   <td>
    <ul>
     <li>Asserts that a PDF image object is a Tagged PDF structure content item.</li>
     <li>Asserts that the PDF object has the entry <code>/Subtype /Image</code>.</li>
     <li>There are no parameters.</li>
     <li>Read aloud as: "<i>current-key</i> shall be in the structural parent tree." (exact quote from ISO 32000-2:2020 Table 359)</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:ImplementationDependent()</code></td>
   <td>
    <ul>
     <li>Asserts that the current row (key or array element) is formally defined to be implementation dependent in the PDF specifications.</li>
     <li>There are no parameters.</li>
     <li>Read aloud as: "<i>current-key</i> is implementation dependent."</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:InKeyMap(<i>key</i>)</code></td>
   <td>
    <ul>
     <li><i>key</i> is a reference to a PDF dictionary which can have arbitrary key names.</li>
     <li>Asserts that the current row (key or array element) and which must be a PDF name exists as a key in the specified map dictionary.</li>
     <li><i>Note that this predicate is <b>not</b> for use with name-trees or number-trees!</i></li>
     <li>Read aloud as: "The name <i>current-key</i> shall be a key in &lt;<i>key</i>&gt;."</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:InNameTree(<i>key</i>)</code></td>
   <td>
    <ul>
     <li><i>key</i> is a reference to a PDF name-tree which use PDF strings as indices. Names trees are complex PDF data structures that use strings as indices.</li>
     <li>Asserts that the current row (key or array element) and which must be a PDF string exists in the specified name-tree.</li>
     <li><i>Note that this predicate is <b>not</b> for use with dictionaries that support arbitrary key names or number-trees!</i></li>
     <li><B>TO BE REPLACED - SEE BELOW!<B></li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:IsAssociatedFile()</code></td>
   <td>
    <ul>
     <li>Asserts that the containing object of the current row (key or array element) needs to be a PDF 2.0 Associated File object.</li>
     <li>There are no parameters.</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:IsEncryptedWrapper()</code></td>
   <td>
    <ul>
     <li>Asserts that the current PDF file needs to be a PDF 2.0 Encrypted Wrapper.</li>
     <li>There are no parameters.</li>    
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:IsFieldName(<i>value</i>)</code></td>
   <td>
    <ul>
     <li>Asserts that the value is a PDF string object and that is a valid partial Field Name according to clause 12.7.4.2 of ISO 32000-2:2020.</li>
     <li>There is one key-value (<code>@key</code>) parameter.</li>    
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:IsHexString()</code></td>
   <td>
    <ul>
     <li>Asserts that the current object is a PDF string object and that was in the PDF as a hex string (`<...>`).</li>
     <li>There are no parameters.</li>    
     <li>Read aloud as: "<i>current-key</i> shall be a hexadecimal string."</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:IsLastInNumberFormatArray(<i>key</i>)</code></td>
   <td>
    <ul>
     <li>Asserts that the current row is the last array element in a number format array (normally the containing object).</li>
     <li>Read aloud as: "<i>current-key</i> shall be the last array element in the number format array defined by &lt;<i>key</i>&gt;."</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:IsMeaningful(<i>condition</i>)<code></td>
   <td>
    <ul>
     <li>Asserts that the current row is only "meaningful" (<b>precise quote from ISO 32000-2:2020!</b>) when <i>condition</i> is true.</li>
     <li>Possibly the inverse of <code>fn:Ignore(...)</code>!</li>
     <li>See also [Errata #6](https://github.com/pdf-association/pdf-issues/issues/6)</li>
     <li>Read aloud as: "<i>current-key</i> shall only be meaningful when &lt;<i>condition</i>&gt;."</li>
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
    <code>fn:IsPDFVersion(<i>version</i>,<i>statement</i>)</code>
   </td>
   <td>
    <ul>
     <li>Can have one or two parameters.</li>
     <li>If no optional <i>statement</i>, then always TRUE for the stated PDF version <i>version</i>.</li>
     <li>Otherwise asserts that the optional <i>statement</i> only applies to the stated PDF version <i>version</i>. This might be a type, a possible value, a new kind of linked object, etc.</li>
     <li><i>version</i> must also make sense in light of the "SinceVersion" and "DeprecatedIn" fields for the current row (i.e. is between them).</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:IsPresent(<i>key</i> or <i>expr</i>)</code><br>
       <code>fn:IsPresent(<i>key</i>,<i>condition</i>)</code>
   </td>
   <td>
    <ul>
     <li>Can have one or two parameters.</li>
     <li>For a single parameter: asserts that the current row (key or array element) must be present in a PDF if <i>key</i> is present, or when the expression </i>expr</i> is true.</li>
     <li>e.g. <code>fn:IsPresent(StructParent)</code> or <code>fn:IsPresent(@SMaskInData>0)</code></li>
     <li>For two parameters: asserts that when <i>key</i>is present in a PDF, that <i>condition</i> should also be true.</li>
     <li>e.g. <code>fn:Eval(fn:IsPresent(Matte,(@Width==parent::@Width)))</code></li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:IsRequired(<i>condition</i>)</code></td>
   <td>
    <ul>
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
     <li>Asserts that the current (arbitrary) key or array element is also a colorant name.</li>
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
     <li>Commonly used in the "IndirectRef" field.</li>
     <li>If <i>condition</i> is true or is not specified, then asserts that the current key value must be a direct object.</li>
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
     <li>If <i>condition</i> is true or not specified, the current key value must be an indirect object.
     <li>If <i>condition</i> is false, the current key value can be direct or indirect.
    </ul>
 </td>
  </tr>
  <tr>
   <td><code>fn:NoCycle()<code></td>
   <td>
    <ul>
     <li>Asserts that the PDF file shall not contain any cycles (loops) when using the current key or array index to key into the linked list of objects.</li>
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
    <li>e.g. <code>fn:IsRequired(fn:SinceVersion(2.0) || fn:NotStandard14Font())</code>.</li>
   </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:NumberOfPages()<code></td>
   <td>
    <ul>
     <li>Number of pages in the PDF document (integer value).</li>
     <li>For valid PDFs will always be 1 or greater.</li>
     <li>There are no parameters.</li>
     <li>Mostly used as an upper-bound for key values that represent a PDF page index</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:PageContainsStructContentItems()</code></td>
   <td>
    <ul>
     <li>Asserts that the current PDF page contains the structure content item represented by the integer value of the current row.</li>
     <li>Only used in the "Required" field, as in <code>fn:IsRequired(fn:PageContainsStructContentItems())</code>.</li>
     <li>There are no parameters.</li>   
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:PageProperty(<i>page-ref</i>,<i>key</i>)</code></td>
   <td>
    <ul>
     <li>References a property (i.e. dictionary entry) of a page, that cannot be accessed via a fixed or relative Arlington path (using <code>::</code>).</li>
     <li>The page is defined by the first parameter <i>page-ref</i> which is a value-reference (<code>@Key</code>) to a Page Object.</li>
     <li>The page property can be a key or key-value and is defined by the second parameter.</li>
     <li>e.g. <code>fn:ArrayLength(fn:PageProperty(@P,Annots))</code></li>
     <li>e.g. <code>fn:Eval(@A==fn:PageProperty(@P,Annots::@NM)</code></li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:RectHeight(<i>key</i>)</code></td>
   <td>
    <ul>
     <li><i>key</i> needs to be <code>rectangle</code> in Arlington predefined types.</li>
     <li>Returns a number >= 0.0, representing the height of the rectangle.</li>
     <li>Needs to be wrapped inside the <code>fn:Eval(...)</code> predicate.</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:RectWidth(<i>key</i>)</code></td>
   <td>
    <ul>
    <li><i>key</i> needs to be <code>rectangle</code> in Arlington predefined types.</li>
    <li>Returns a number >= 0.0, representing the width of the rectangle.</li>
    <li>Needs to be wrapped inside the <code>fn:Eval(...)</code> predicate.</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:RequiredValue(<i>condition</i>,<i>value</i>)<code></td>
   <td>
    <ul>
     <li>Only used in the "PossibleValue" field to indicate if a specific value is required under a specific condition.</li>
     <li>Asserts that the current row must by <i>value</i> when <i>condition</i> evaluates to true.</li>
    </ul>
   </td>
  </tr>
  <tr>
   <td><code>fn:SinceVersion(<i>version</i>)</code></br>
       <code>fn:SinceVersion(<i>version</i>,<i>statement</i>)</code></td>
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
     <li><i>key</i> needs to be a stream and returns an integer >= 0 representing the compressed stream length.</li>
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
     <li><i>key</i> needs to be a string object and returns an integer >= 0. Empty strings (length 0) are valid in PDF.</li>
     <li>Needs to be used inside <code>fn:Eval(...)</code>.</li>
     <li> e.g. <code>fn:Eval(fn:StringLength(Panose)==12)</code>
    </ul>
   </td>
  </tr>
</table>

# Proposals for future predicates

Please review and add any feedback or comments to the appropriate issue!

* Should <code>fn:Eval(...)</code> be scrapped (i.e. removed) as it is kind of redundant?

<table style="border: 0.25px solid; border-collapse: collapse;">
  <tr>
   <td><code>fn:ValueOnlyWhen(<i>value</i>,<i>condition</i>)</code></td>
   <td>
    <ul>
     <li>See <a href="https://github.com/pdf-association/arlington-pdf-model/issues/74">Issue #74</a></li>
     <li>Only used in PossibleValues field</lI>
     <li>Asserts that a specific possible value (single value) of the current key or array element is conditionally valid (<i>condition</i> evaluates to true).</li>
     <li>Read aloud as "<i>value can be X but only when Y</i>"</li>
     <li>this is a <i>logically weaker</i> predicate than <code>fn:RequiredValue(...)</code> as there may be other allowable PossibleValues (either conditionally or not)</li>
     <li>Example for <b>Di</b> key in Transition dictionary:<br/>
     <code>[0,fn:ValueOnlyWhen(90,(@S==Wipe)),fn:ValueOnlyWhen(180,(@S==Wipe)),270,fn:ValueOnlyWhen(315,(@S==Glitter)))];[fn:RequiredValue(((@S==Fly) && (@SS!=1.0)),None)]</code>
     </li>    
    </ul>
   </td>
  </tr>
   <tr>
   <td><code>fn:IsArray(<i>key</i>)<br/>fn:IsDictionary(<i>key</i>)<br/>fn:IsStream(<i>key</i>)</code></td>
   <td>
    <ul>
     <li>See <a href="https://github.com/pdf-association/arlington-pdf-model/issues/61">Issue #61</a></li>
     <li>Asserts that the specified key or array element is the specified kind of PDF object (dictionary, array, stream).</li>
     <li><i>key</i> may be just a key name, array element (integer), or a longer relative or absolute Arlington path (using <code>::</code>).</li>
     <li>Read aloud as "<i>when X is a Y then ...</i>"</li>
     <li>Example is for <b>AS</b> in  annotations:<br/>
         <code>fn:IsRequired(fn:IsDictionary(AP::N) || fn:IsDictionary(AP::R) || fn:IsDictionary(AP::D))</code>
     </li>
     </li>    
    </ul>
   </td>
  </tr>
  </tr>
   <tr>
   <td><code>fn:IsNameTreeValue(<i>tree-reference</i>,<i>key</i>)<br/>fn:IsNameTreeIndex(<i>tree-reference</i>,<i>@key</i>)</br></br>
             fn:IsNumberTreeValue(<i>tree-reference</i>,<i>key</i>)<br/>fn:IsNumberTreeIndex(<i>tree-reference</i>,<i>@key</i>)
   </code></td>
   <td>
    <ul>
      <li>See <a href="https://github.com/pdf-association/arlington-pdf-model/issues/49">Issue #49</a>. This proposal will replace <code>fn:InNameTree(...)</code> with these predicates.</li>
      <li>PDF name-/number-trees have complex internal data structures that vary per PDF file (see subclauses 7.9.6 and 7.9.7 in ISO 32000-2:2020). Arlington hides this complexity by defining both these as Arlington data types.</li>
      <li>The indexing of nodes in a name-tree are always by a string. Thus the type of <i>key</i> in <code>fn:IsNameTreeIndex(...)</code> must be a string (any type).</li>
      <li>The value of leaf nodes in a name-tree can be any type of PDF object, including strings. Thus the type of <i>key</i> in <code>fn:IsNameTreeValue(...)</code> can be anything.</li>
      <li>The indexing of nodes in a number-tree are always by an integer. Thus the type of <i>key</i> in <code>fn:IsNumberTreeIndex(...)</code> must be an integer.</li>
      <li>The value of leaf nodes in a number-tree can be any type of PDF object, including integers. Thus the type of <i>key</i> in <code>fn:IsNumberTreeValue(...)</code> can be anything.</li>
      <li><i>tree-reference</i> is a name- or number-tree as appropriate for the predicate. It will commonly be a reference to <code>trailer::Catalog::Names::<i>key</i></code>.</li> 
     </li>    
    </ul>
   </td>
  </tr>
</table>


# Checks still needing to be completed in ISO 32000-2:2020

- Check array length requirements of all `array` search hits - *done up to Table 95*

- Check ranges of all `integer` search hits - *done up to Table 170*.
See also: [Errata #15](https://github.com/pdf-association/pdf-issues/issues/15)

- Read every Table for dictionary keys and update declarative rules - *done up to Table 186 Popup Annots*

- Check ranges of all `number` search hits - *not started yet*
