# The Arlington PDF Model

<img src="../resources/ArlingtonPDFSymbol300x300.png" width="150">

![GitHub](https://img.shields.io/github/license/pdf-association/arlington-pdf-model)
&nbsp;&nbsp;&nbsp;
![PDF support](https://img.shields.io/badge/PDF-2.0-blue)
&nbsp;&nbsp;&nbsp;
![LinkedIn](https://img.shields.io/static/v1?style=social&label=LinkedIn&logo=linkedin&message=PDF-Association)
&nbsp;&nbsp;&nbsp;
![Twitter Follow](https://img.shields.io/twitter/follow/PDFAssociation?style=social)
&nbsp;&nbsp;&nbsp;
![YouTube Channel Subscribers](https://img.shields.io/youtube/channel/subscribers/UCJL_M0VH2lm65gvGVarUTKQ?style=social)

# TestGrammar (C++17) proof of concept application

The TestGrammar (C++17) proof of concept application is a multi-platform command-line utility. It performs a number of functions:

1. validates all TSV files in an Arlington TSV file set.
    - Check the uniformity (number of columns), if all types are one of Arlington types, predicates are parsable, etc.
2. validates a PDF file against an Arlington TSV file set. Starting from trailer, the tool validates:
    - if all required keys are present
    - if additional undocumented keys are present and whether such keys conform to 2nd or 3rd class PDF name conventions
    - if values are of correct type (_processing of predicates (declarative functions) are not fully supported_)
    - if objects are indirect objects as specified (_requires pdfium PDF SDK to be used_)
    - if value is correct if `PossibleValues` are defined (_processing of predicates (declarative functions) are not fully supported_)
    - all messages are prefixed with `Error:`, `Warning:` or `Info:` to enable post-processing
    - messages can be colorized
    - a specific PDF version can be forced via the command line (`--force`) for validating a PDF file (_PDFIUM only!_).
    - the PDF version of the latest feature is reported (the actual listed feature may be one of many - it is the first feature of that version that was encountered). This version may be different to the PDF header version, the optional DocCatalog/Version key or the `--force` PDF version on the command line
3. recursively validates a folder containing many PDF files.
    - for PDFs with duplicate filenames, an underscore is appended to the report filename to avoid overwriting.
    - for ease of post-processing large quantities of log files from corpora, there are also `--batchmode` and `--no-color` options that can be specified
    - using `--brief` will also keep output log size under control
4. compares the Arlington PDF model grammar with the Adobe DVA Formal Representation
    - the understanding of the Adobe DVA model is hard-coded with a mix of asserts and error messages. For this reason it is best to always use a debug build at least once to confirm the Adobe FormalRep is understood by this PoC!
    - a manually curated list of multiple DVA objects that combine to be equivalent to an Arlington TSV file is hard-coded.
    - not all aspects of the Adobe DVA model are processed and mapped across to Arlington so there will be some "noise" in the output
    - a list of Adobe DVA objects and Arlington TSV files that were are not compared is given. This is mostly FYI for diagnostic purposes.
    - the output report needs manual review afterwards, as the Arlington predicates will trigger many differences

All output using different PDF SDKs is made as easily comparable as possible, normally with only 1 or 2 lines of difference (e.g. the build date/time and the PDF SDK version information). Most often other differences are caused by PDF SDK limitations (such as not being able to report the PDF header version, whether a PDF object is direct or indirect, etc). See below for a list of challenges with PDF SDKs.

## Usage

The command line options are very similar to the Python proof-of-concept:

```
Arlington PDF Model C++ P.o.C. version vX.Y built <date>> <time> (<platform & compiler details>)
Choose one of: --pdf, --checkdva or --validate.

Usage:
        TestGrammar --tsvdir <dir> [--force <ver>] [--out <fname|dir>] [--no-color] [--clobber] [--debug] [--brief] [--extensions <extn1[,extn2]>] [--validate | --checkdva <formalrep> | --pdf <fname|dir> ]

Options:
-h, --help        This usage message.
-b, --brief       terse output when checking PDFs. The full PDF DOM tree is NOT output.
-c, --checkdva    Adobe DVA formal-rep PDF file to compare against Arlington PDF model.
-d, --debug       output additional debugging information (verbose!)
    --clobber     always overwrite PDF output report files (--pdf) rather than append underscores
    --no-color    disable colorized text output (useful when redirecting or piping output)
-m, --batchmode   stop popup error dialog windows and redirect everything to console (Windows only, includes memory leak reports)
-o, --out         output file or folder. Default is stdout. See --clobber for overwriting behavior
-p, --pdf         input PDF file or folder.
-f, --force       force the PDF version to the specified value (1,0, 1.1, ..., 2.0). Requires --pdf
-t, --tsvdir      [required] folder containing Arlington PDF model TSV file set.
-v, --validate    validate the Arlington PDF model.
-e, --extensions   a comma-separated list of extensions.

Built using <pdf-sdk vX.Y.Z>
```

`Info:` messages are cyan, `Warning:` messages are yellow, and `Error:` messages are red.

### Notes

* TestGrammar PoC processes _almost_ all predicates (declarative functions). When a predicate is processed that is known to have an incomplete implementation, a yellow warning message is output, whereas red error messages mean that all predicates that were processed are fully implemented.

* all messages from validating PDF files are prefixed with `Error:`, `Warning:` or `Info:` to make regex-based post processing easier.  

## Arlington validation (--validate)

This reads the specified Arlington TSV file set and processes all data to ensure that it is holistic, unified, correct according to the Arlington grammar rules, and that all predicates can be parsed. Reported errors generally indicate typos or data entry issues. Missing or unlinked TSV files are commonly caused by version mismatches in either the SinceVersion field or via a version predicate in the Links field.

`--brief` has no impact on `--validation`. With grammar validation, `--debug` will try an additional brute-force parse using hard-coded regex expressions as well as print out the name of each Arlington TSV file as it goes. This can be useful if the PoC is crashing...

The Python script `./scripts/arlington.py` can also perform syntax validation using a slightly different algorithm. Both validators should always pass!

## Arlington vs Adobe DVA (--checkdva)

Compares the Adobe DVA PDF 1.7 (typically `PDF1_7FormalRep.pdf`) which is supposedly based on ISO 32000-1:2008 against an Arlington model set which is based on ISO 32000-2:2020. This report is verbose and requires a human to interpret - it is **not** designed to be "zero output is good"/"some output is bad". The Adobe DVA processing is hard-coded and changes or updates by Adobe will require further maintenance of the PoC! The PoC does not report against dictionaries (TSV files) that were added in PDF 2.0, but smaller changes such as deprecation, new keys, additions to value sets, etc. are reported (along with the version) and thus can be identified as PDF 2.0 change. Predicates are also NOT calculated and thus will be reported as a difference.

Other differences occur because of the way the different formalisms represent data: DVA uses far fewer but more complex inter-dependent COS data structures and semantics, whereas Arlington has standalone individual definitions for subtly different objects based on `Type` and/or `Subtype` key values (e.g. many Arlington Font*.tsv files but single Adobe DVA Font object). This results in the following kind of output where a generic DVA Font is compared to Arlington's highly specific FontType0:

```
...
PossibleValue DVA: Font vs Arlington: FontType0/Subtype
        DVA has: Type1, TrueType, MMType1, Type3, CIDFontType0, CIDFontType2
...
```

By default, colorized output is produced with a cyan `Info:` line listing each DVA and Arlington pair being compared, before the results of the comparison. Comparison messages are not colorized. Red `Error:` messages indicate likely issues with processing the Adobe DVA COS objects - there should be no red errors! Use `--no-color` for pure uncolorized text output.

The Adobe DVA definitions for content stream operators and operands are **not** checked as Arlington does not yet have an equivalent.

If `--brief` is _not_ specified, then a list of unprocessed DVA key names and Arlington TSV filenames will also be given at the end. Of most importance are the Adobe DVA keys that are **not** checked as this means the Arlington model equivalents (_if any_) are also **not** chceked or compared.

*Typical messages*:
```
Missing key in DVA: object/key (1.x)
Missing key in Arlington: arlington/key
SinceVersion is different for key key: DVA: dva-object (1.x) vs Arlington arlington-tsv (1.y
Required is different for key T: DVA==FALSE vs Arlington==fn:IsRequired(@S==T)
Indirect is different for key Dests: DVA==TRUE vs Arlington==FALSE
```

## PDF file check (--pdf)

When processing PDF files, is recommended to use `--brief` to see a single line of context (i.e. the PDF DOM path of the object) immediately prior to all related `Error:`, `Warning:` or `Info:` messages. Each line of context is preceded by a number indicating a reference number in the PDF DOM - this is mainly useful for debugging. Numbers will match between runs for the same PDF SDK when using `--brief` and not. Somewhat counter-intuitively, both `--brief` and `--debug` can be used together: `--debug` will output PDF file specific information such as object numbers which can make bulk post-processing (e.g. using `grep`) more difficult to locate unique messages.

Due to a **severe** lack of compliance with PDF versions, if a PDF file is between 1.4 and 1.7 inclusive, it will automatically be processed as PDF 1.7. Files with versions 1.3 or earlier or PDF 2.0 are processed as per the PDF standard (where the Catalog/Version key can override the PDF header comment line). Use the `--force` command line option to override this default behavior.

Messages report raw data from the Arlington TSV files (such as `SpecialCase` predicates) to make searching for the specifics and matching to  Arlington TSV files much easier. This can be slightly confusing when deprecated features are used, since the PDF version of the PDF file may also need to be known. The version used in the comparison is logged as `Info` messages in the first few lines as well as the 2nd last line of output.

All output should have a final line "END" (`grep --files-without-match "END"`) - if not then something nasty has happened prematurely (crash or assert)

An exit code of 0 indicates successful processing. An exit code of -1 indicates an error: typically an encrypted PDF file requiring a password or with  unsupported crypto, or a corrupted PDF where the trailer cannot be located (this will also depend on the PDF SDK in use). If processing a folder of PDFs under Microsoft Windows, then use `--batchmode` so that if things do crash or assert then the error dialog box doesn't block unattended execution from continuing.

### Understanding extensions

As the Arlington PDF model is defined using text-based TSV files, it is very easy to extend the model to match a specific implementation or additional sets of requirements by patching an Arlington TSV model set of files. For example, you can suppress messages regarding specific malformations or proprietary extensions by simply adding definitions to the appropriate TSV file. Because these are not part of the official ISO PDF specification, these are referred to generically as "extensions" and the "SinceVersion" field which normally contains a PDF version is replaced by a predicate: `fn:Extension(name)` or `fn:SinceVersion(x.y,fn:Extension(name))` depending on whether the extension is version based or not.

The names of extensions are arbitrary but must following the conventions used in Arlington for keys: alphanumerics with UNDERSCORE. No SPACES, COMMAs or MINUS (dash). By default, no extensions are supported so a "pure specification" report is generated.

The TestGrammar CLI option `-e` or `--extensions` is used to specify a COMMA-separated list of case-sensitive extension names to support. Enabling support means that keys matching these extension names will **not** get reported as unknown keys and that these keys will also be further checked against their Arlington definitions.

Note also that the Extensions Dictionary in the PDF file is **not** consulted!

```bash
TestGrammar --brief --tsvdir ./tsv/latest --extensions AAPL,Malforms --pdf /tmp/folder_of_pdfs/ --out /tmp/out
```

Prototyped extensions:
- "AAPL": adds `AAPL:Keywords` to DocInfo, `AAPL:AA` boolean and the `AAPL:ST` Style dictionary to GraphicsStateParameter and adds a new dictionary object in `AAPL_ST.tsv`
- "ISO_TS_24064": adds STEP AP 242 support as another 3D format for 3DStreams, and a new requirements dictionary in `RequirementsSTEP.tsv`
- "ISO_TS_24654": adds `Path` to AnnotLink for non-rectangular links
- "ISO_TS_32003": adds AES-GCM support to PDF 2.0 by specifying additional values for some keys in Encryption dictionaries
    - note that because encryption results in all streams and strings being encrypted, PDF SDK support will vary
- "ISO_TS_32004": adds `KDFSalt` to Encryption*.tsv, `AuthCode` to FileTrailer and XRefStream, and a new dictionary object in `AuthCode.tsv`
- "Malforms": adds misspelled `SubType` key to OptContentCreatorInfo as an alternate spelling of `Subtype` and misspelled `Blackls1` for `BlackIs1` (lowercase L instead of uppercase i) in FilterCCITTFaxDecode
    - the existing row is simply duplicated with the key spelling then changed and the official "SinceVersion" PDF version replaced with the extension predicate: `fn:Extension(Malforms)`.
    - because Optional Content was only introduced in PDF 1.5, the `SubType` malform predicate also uses the `fn:SinceVersion` predicate to further express this requirement for the misspelled key

    ```bash
    # See all the details for all extensions in an Arlington data set
    grep -P "fn:Extension\([^\t]+\)" *

    # A list of all the unique extension names in an Arlington data set
    grep -Pho "fn:Extension\([^\t)]+\)" * | sort | uniq
    ```

### Understanding errors and warnings

Most error and warning messages are obvious from the message text. If `--debug` **is** specified, then PDF object numbers will also be in the output which can help rapidly identify the cause of error and warning messages. Messages always occur **after** the PDF DOM path so adding a `-B 1` to `grep` can provide such context. Multiple messages can occur for any context line.

`Error: object validated in two different contexts. First ...`

This error means that a direct PDF object (`x y obj ... endobj`) has been arrived at by two different paths in the PDF, but that the Arlington Link determination in each case resulted in two different object definitions (TSV filenames). This can happen if the Arlington PDF Model is under- or ill-specified (_is there sufficient information in the parent object to correctly determine which definition should get used? Is a predicate required?_) or if there is object reuse occurring in the PDF file. For example, a SoftMask Image XObject can be reused (just by being in the XObject Resources) as a normal Image XObject. Until predicates are fully supported, false error messages may occur more often than they should. Currently this error is **expected** for combined widget and field annotations, as permitted by sub-clause 12.5.6.19:

> As a convenience, when a field has only a single associated widget annotation, the contents of the field dictionary (12.7.4, "Field dictionaries") and the annotation dictionary may be merged into a single dictionary containing entries that pertain to both a field and an annotation.

This might be corrected in a future version of Arlington, by adding the Widget annotation keys to all field annotations (or vice-versa).

Note that TestGrammar uses a point-scoring system to resolve potential Link ambiguities, with `Type` and `Subtype` keys have a very strong influence, followed by other required keys. However disambiguation for arrays often occurs through context but, by design, TestGrammar does **not** track context - it merely follows all object references.  

Inheritance is only tested for keys that are also "Required" in the Arlington PDF Model, as the required-ness condition can be met via inheritance. The algorithm uses recursive back-tracking following explicit `Parent` key references, which is currently sufficient for the Page Tree. It does **not** build a forward-looking stack, such as renderer might need to construct.     

## Understanding output

Examining extant PDF files against the specification-derived Arlington PDF model can produce a lot of output. Using simple CLI commands such as `head`, `tail` and `grep` can be used to bulk analyze a directory full of output files.

The first line of all output is as follows:
```
BEGIN - TestGrammar vX.Y built <date> <time> (<compiler> <platform> <debug|release>) <pdf-sdk>
BEGIN - TestGrammar v0.5 built Sep 24 2021 13:38:51 (MSC x64 release) pdfium
BEGIN - TestGrammar v0.5 built Sep 24 2021 13:50:03 (GNU-C linux release) pdfium
BEGIN - TestGrammar v0.5 built Sep 24 2021 14:23:32 (MSC x86 release) PDFix v6.1.0
```
where:
* `BEGIN` is a magic keyword indicating the start of a new PDF
* `vX.Y` is the version of the TestGrammar proof-of-concept application
* `<date>` and `<time>` are the date and time of the build (C/C++ \__DATE__ and \__TIME__ macros)
* `(<compiler> <platform> <debug|release>)` indicate the C++ compiler, platform and whether it is a debug or release build.
    - Knowing this is critical for debug as PDF SDKs can (and do!) differ in their output and behavior because of such factors!
* `<pdf-sdk>` is the name and version (_when available_) of the PDF SDK being used.  

The second line will always be the location of the Arlington PDF Model TSV file set being used (the path format is appropriate to operating system):
```
Arlington TSV data: "C:\\arlington-pdf-model\\tsv\\latest"
```

The 3rd line will always be the full path and filename of the PDF file (the file path format is appropriate to operating system):
```
PDF: "c:\\Users\\peter\\Documents\\test.pdf"
PDF: "/mnt/c/Users/peter/Documents/test.pdf"
```

The 4th line will give an indication of how this PDF is structured - whether it uses a traditional cross-reference table with `startxref`, `xref` and `trailer` keywords, or whether cross-reference streams are used. For those PDF SKDs that do not report this directly (such as pdfium), this is based on the presence of the required `Type` key in the trailer dictionary (as defined in the [Arlington XRefStream object](../tsv/latest/XRefStream.tsv)):

```
XRefStream detected
Traditional trailer dictionary detected
```

The next few lines will report the PDF versions, as reported by the PDF SDK being used and the `--force` command line option. This includes the version from the PDF Header line (`%PDF-x.y`), the Document Catalog Version key, the forced version and the version being used for PDF file validation. PDF SDKs that do not support this functionality will report `2.0` (the latest PDF version):
```
Header is version PDF X.Y
Document Catalog/Version is version PDF X.Y
Command line forced to be version PDF X.Y
Processing file as version PDF X.Y
```

The second last line of every successful output file will report the PDF version of an arbitrary Arlington definition (file/key):
```
Info: Latest Arlington feature was version PDF x.y (file/key)
```   

The last line of every output should always be `END` on a line by itself. If this is missing, then it means that the TestGrammar application has not cleanly completed processing (_crash? unhandled exception? assertion failure? stack overflow? timeout?_).

## Using a helpful editor

The [free Atom editor](https://atom.io/) has a useful plugin called [language-ansi-styles](https://atom.io/packages/language-ansi-styles) that will support the ANSI terminal codes that are used for the colorization of messages.

## Useful post-processing

After processing a tree of PDF files and saving the output into a folder via a command such as:

```bash
# Colorized output
TestGrammar --brief --tsvdir ../arlington-pdf-model/tsv/latest --out . --pdf ../test/

# Colorized output but ignoring the PDF version in all PDF files
TestGrammar --force 2.0 --brief --tsvdir ../arlington-pdf-model/tsv/latest --out . --pdf ../test/

# Non-colorized output
TestGrammar --force 2.0 --brief --batchmode --tsvdir ../arlington-pdf-model/tsv/latest --out . --pdf ../test/
```

The following Linux CLI commands can be useful in filtering the output:

```bash
# Get a more unique set of messages without PDF filenames. May include PDF object numbers if --debug was specified.
grep -Ph "Error:" * | sort | uniq
grep -Ph "Warning:" * | sort | uniq
grep -Ph "Info:" * | sort | uniq

# Get messages with their context line (the line before the message). If more than 1 message per context change the line count...
grep -B 1 "Error:" * | sort | uniq
grep -B 1 "Warning:" * | sort | uniq
grep -B 1 "Info:" * | sort | uniq

# Summarized versioning of all PDFs
grep "version PDF" *

# Files that were not processed (likely password encrypted)
grep "acquire Trailer" *

# See what version of PDF actually got used
grep "Processing file as PDF" *

# Find PDFs that crashed, CTRL-C or did not complete
grep --files-without-match "END" *
```

# All messages

This is a list of all messages that can occur from the PoC so that post-processing can be done based on grep, etc. Specific messages will only occur for `--validate`, `--check`, `--pdf` or for invalid command line options. `...` indicates additional information which may further vary depending on both the PDF SDK in use and if `--debug` is also specified.

## Error messages (red)

From `grep COLOR_ERORR src/*.cpp`:

```
Error: loading Arlington TSV file ...
Error: Adobe DVA key not found: ...
Error: Adobe DVA key was not a dictionary: ...
Error: DVA PDFMajorVersion/PDFMinorVersion was invalid for ...
Error: No ValueType defined for DVA for ...
Error: ValueType is not an array in DVA for ...
Error: DVA ValueType array element is not a name object for key ...
Error: Bounds is not a dictionary in DVA for ...
Error: DVA Bounds/Equals[i] was an unexpected type for ...
Error: DVA Bounds/Equals[i] was a null object for ...
Error: DVA Bounds/Equals was not an array for ...
Error: failed to acquire Trailer/Root/FormalRepTree
Error: failed to acquire Trailer/Root
Error: failed to acquire Trailer
Error: EXCEPTION: ...
Error: empty Arlington TSV grammar file: ...
Error: wrong number of columns in TSV file: ...
Error: wrong column headers for file: ...
Error: mismatched number of open '[' and close ']' set brackets ...
Error: mismatched number of open '(' and close ')' brackets ...
Error: KeyName field validation error ... for key ...
Error: Type field validation error ... for key ...
Error: SinceVersion field validation error ...
Error: DeprecatedIn field validation error ...
Error: Required field validation error ...
Error: IndirectRef field validation error ...
Error: Inheritable field validation error ...
Error: DefaultValue field validation error ...
Error: PossibleValues field validation error ...
Error: SpecialCase field validation error ... <
Error: Link field validation error ...
Error: wrong # of Types vs. # of links ...
Error: basic type ... should not be linked in ...
Error: complex type ... is unlinked in ...
Error: unexpected type ... in ...
Error: wrong # of types vs. # of DefaultValue ...
Error: wrong # of types vs. # of PossibleValues ...
Error: wrong # of types vs. # of SpecialCase ...
Error: duplicate key in ... for key #i ...
Error: at least one required inheritable key in ... but no Parent key
Error: at least one required inheritable key in ... but Parent key is not a dictionary
Error: wildcard key '*' in ... was not last key
Error: array definition file ... does not meet array file naming conventions!
Error: array definition file ... did not validate as an array!
Error: ... definition file ... appears to be an array!
Error: ... has simple type ... when link ... is present
Error: ... has bad link ... - missing enclosing [ ]
Error: linked file ... failed to load!
Error: can't reach ... from Trailer or XRefStream (assumed as dictionary)
Error: can't load Arlington TSV grammar file ...
Error: Failure to terminate parsing of ...
Error: failed to acquire Trailer
Error: error parsing command line arguments
Error: required -t/--tsvdir was not specified!
Error: -t/--tsvdir "..." is not a valid folder!
Error: -f/--force PDF version '...' is not valid!
Error: --checkdva argument '...' was not a valid PDF file!
Error: no PDF file or folder was specified!
Error: --pdf argument '...' was not a valid file!
Error: Document Catalog major version is earlier than PDF header version! Ignoring.
Error: Document Catalog minor version is earlier than PDF header version! Ignoring.
Error: Both Document Catalog and header versions are invalid. Assuming PDF 2.0.
Error: can't select any Link to validate PDF object ...
Error: recursive inheritance depth of x exceeded for ...
Error: wrong type: ...
Error: not an indirect reference as required: ...
Error: invalid date string for key ...
Error: rectangle does not have 4 numeric elements for key ...
Error: matrix does not have 6 numeric elements for key ...
Error: special case not correct: ...
Error: wrong value for possible values: ...
Error: name tree Names array element #i - missing 2nd element in a pair for ...
Error: name tree Names array element #i - 1st element in a pair returned null for ...
Error: name tree Names array element #i - 1st element in a pair was not a string for ...
Error: name tree Names object was missing when Kids was also missing for ...
Error: name tree Names object was not an array when Kids was also missing for ...
Error: name tree Kids array element number #i was not a dictionary for ...
Error: name tree Kids object was not an array for ...
Error: number tree Nums array element #i was null for ...
Error: number tree Nums array element #i was not an integer for ...
Error: number tree Nums array was invalid for ...
Error: number tree Nums object was not an array for ...
Error: number tree Nums object was missing when Kids was also missing for ...
Error: number tree Kids array element number #i was not a dictionary for ...
Error: number tree Kids object was not an array for ...
Error: wrong type for dictionary wildcard for object/key
Error: could not get value for key 'key' (...)
Error: non-inheritable required key does not exist: ...
Error: inheritable required key does not exist: ...
Error: PDF array object encountered, but using Arlington dictionary ...
Error: minimum required array length incorrect for ...
Error: unexpected object type ... for ...
Error: arrays must use integers: was ...
Error: arrays need to use contiguous integers: was ...
Error: array using numbered wildcards (integer+'*') need to be contiguous last rows in ...
Error: object number ... is illegal. trailer Size is ...
```

## Warning messages (yellow)

From `grep COLOR_WARN src/*.cpp`:

```
Warning: Arlington wildcard did not have any match in Adobe DVA for ...
Warning: Arlington array did not have any matching Adobe DVA array in ...
Warning: Adobe DVA comparison did not check DVA key: ...
Warning: Adobe DVA comparison did not check Arlington: ...
Warning: in path, object filename contains 'Array' but is linked as ...
Warning: in path, object filename contains 'Dict' but is linked as ...
Warning: in path, object filename contains 'Stream' but is linked as ...
Warning: single element array with '0*' should use '*' ...
Warning: bitmask was not a 32-bit value for key ...
Warning: bitmask was not an integer value for key ...
Warning: integer value exceeds PDF 1.x integer range for ...
Warning: PDF 1.x names were limited to 127 bytes (was x) for ...
Warning: string for key ... starts with UTF-16LE byte order marker
Warning: ASCII string contained at least one unprintable byte for key ...
Warning: rectangle does not have exactly 4 elements for key ...
Warning: matrix does not have exactly 6 elements for key ...
Warning: special case possibly incorrect (some predicates NOT supported): ...
Warning: possibly wrong value for possible values (some predicates NOT supported): ...
Warning: non-inheritable required key may not exist: ...
Warning: inheritable required key may not exist: ...
Warning: required key may not exist: ...
Warning: array length was not an exact multiple of x (was y) for ...
Warning: object identified in two different contexts. Originally: ...; second: ...
```

## Informational messages (cyan)

From `grep COLOR_INFO src/*.cpp`:

```
Info: Adobe DVA comparison did not check ... Adobe DVA keys
Info: Adobe DVA comparison did not check ... Arlington definitions
Info: Header is version PDF x.y
Info: Document Catalog/Version is PDF x.y
Info: Command line forced to PDF x.y
Info: Rounding up PDF x.y to PDF a.b
Info: Processing as PDF x.y
Info: Traditional trailer dictionary detected.
Info: XRefStream detected.
Info: Latest Arlington feature was PDF x.y (object/key) compared using PDF a.b
Info: found a PDF 1.4 Metadata key
Info: found a PDF 2.0 Associated File AF key
Info: second class key '...' is not defined in Arlington for ... in PDF x.y
Info: third class key '...' found in ...
Info: unknown key '...' is not defined in Arlington for ... in PDF x.y
Info: detected an array version-based feature after obsolescence in PDF x.y (using PDF a.b) for ...
Info: detected an array version-based feature before official introduction in PDF x.y (using PDF a.b) for ...
Info: detected an array version-based feature that was deprecated in PDF x.y (using PDF a.b) for ...
Info: detected an array version-based feature that was only in PDF x.y (using PDF a.b) for ...
Info: array was longer than needed for object/key
Info: detected a dictionary key version-based feature after obsolescence in PDF x.y (using PDF a.b) for ...
Info: detected a dictionary key version-based feature before official introduction in PDF x.y (using PDF a.b)...
Info: detected a dictionary key version-based feature that was deprecated in PDF x.y (using PDF a.b) for ...
Info: detected a dictionary key version-based feature that was only in PDF x.y (using PDF a.b) for ...
Info: detected a dictionary wildcard version-based feature after obsolescence in PDF x.y (using PDF a.b) for ...
Info: detected a dictionary wildcard version-based feature before official introduction in PDF x.y (using PDF a.b) for ...
Info: detected a dictionary wildcard version-based feature that was deprecated in PDF x.y (using PDF a.b) for ...
Info: detected a dictionary wildcard version-based feature that was only in PDF x.y (using PDF a.b) for ...
```

# Development
## Coding Conventions

* platform independent C++17 with STL and minimal other dependencies (_no Boost please!_)
* no tabs. 4 space indents
* std::wstrings need to be used for many things (such as PDF names and strings from PDF files) - don't assume PDF content is always ASCII or UTF-8!
* can safely assume Arlington TSV data is all ASCII/UTF-8
* liberal comments with code readability ahead of efficiency and performance
* classes and methods use Doxygen-style `/// @` comments (as supported by Visual Studio IDE)
* `/// @todo` are to-do comments
* zero warnings on all builds and all platforms (excepting PDF SDKs) for completed code
* do not create unnecessary dependencies on specific PDF SDKs - isolate through the shim layer
* liberal use of asserts with the Arlington PDF model, which can be assumed to be correct (but never for data from PDF files!)
* performance and memory is **not** critical (this is just a PoC!) - so long as a full Arlington model can be processed and reasonably-sized PDFs can be checked
* some PDF SDKs do absorb far too much memory, are excessively slow or cause stack overflows. This is not the PoC's issue!

## Debugging tips (most for `--pdf`)

* Ensure you validate the TSV file set before anything else! If validation fails, then PDF processing will undoubtedly be incorrect as it makes assumptions about correctness of data in the Arlington model!
* Look at the top of various C++ files for #defines. e.g. in ParseObjects.cpp there is CHECKS_DEBUG and SCORING_DEBUG.
* When debugging a PDF that is reporting strange or incorrect messages, set conditional breakpoints in ParseObjects.cpp based on the integer CParsePDF::counter (which is the number for each line of PDF DOM tree output).
* Try using alternate PDF SDKs and comparing output line-by-line to discount issues PDF lexing and parsing issues.
* Double check the TSV against appropriate Table in ISO 32000-2:2020. Don't assume it is perfect!
* Temporarily edit to remove predicates from the Arlington TSV file that is causing major issues - if things now work, then it is likely to be predicate parsing or processing that is the issue.
* You will need a PDF internal browser such as [iText RUPS](https://github.com/itext/i7j-rups) (free), [Apache PDFBox Debugger standalone](https://pdfbox.apache.org/download.html) (free) or the internal viewer in Adobe Acrobat (commercial). Many others exist too.

## PDF SDK Requirements

Checking PDF files requires a PDF SDK with certain key features (_we shouldn't need to write yet-another PDF parser!_). Key features required of a PDF SDK are:
* able to iterate over all keys in PDF dictionaries and arrays, including any additional keys not defined in the PDF spec. Keys are sorted alphabetically by the PoC so output from different PDF SDKs is hopefully in the same order.
* able to test if a specific key name exists in a PDF dictionary
* able to report the true number of array elements  
* able to report key value type against the 9 PDF basic object types (integer, number, boolean, name, string, dictionary, stream, array, null)
* able to report if a key value is direct or an indirect reference - **this is a big limiting factor for many PDF SDKs!**  
* able to treat the trailer as a PDF dictionary and report if key values are direct or indirect references - **this is a big limiting factor for many PDF SDKs!**  
* able to report PDF object number for objects that are not direct - **this is a limiting factor for some PDF SDKs!**  
* not confuse values, such as integer and real numbers, so that they are expressed exactly as they appear in a PDF file - **this is a limiting factor for some PDF SDKs!**
* return the raw bytes from the PDF file for PDF name and string objects, including empty names ("`/`")
* not do any PDF version based processing while parsing

Another recent discovery of behavior differences between PDF SDKs is when a dictionary key is an indirect reference to an object that is well beyond the trailer `Size` key or maximum cross-reference table object number. In some cases, the PDF SDK "sees" the key, allowing it to be detected and the error that it is invalid is deferred until the TestGrammar app attempts to resolve the indirect reference (e.g. PDFix). Then an error message such as `Error: could not get value for key XXX` will be generated. Other PDF SDKs completely reject the key and the key is not at all visible so no error about can be reported - the key is completely invisible when using such PDF SDKs (e.g. pdfium).

All code for a specific PDF SDK should be kept isolated in a single shim layer CPP file so that all Arlington specific logic and validation checks can be performed against the minimally simple API defined in `ArlingtonPDFShim.h`. There are `#defines` to select which PDF SDK to build with.

## Source code dependencies

TestGrammar has the following module dependencies:

* PDFium: OSS PDF SDK (`ARL_PDFSDK_PDFIUM`)
  - this is the **default PDF SDK used** as it provides the most functionality, has source code, and is debuggable if things don't work as expected
  - a local copy of pdfium is used to reduce the build complexity and time - and to fix a number of issues.
  - reduced source code located in `./pdfium`
  - PDFium is also slightly modified in order to validate whether key values are direct or indirect references in trailer and normal PDF objects
  - see `src/ArlingtonPDFShimPDFium.cpp`

* PDFix: a free but closed source PDF SDK (`ARL_PDFSDK_PDFIX`)
  - see `src/ArlingtonPDFShimPDFix.cpp`
  - single .h dependency [src/Pdfix.h](src/Pdfix.h)
  - see https://pdfix.net/ with SDK documentation at https://pdfix.github.io/pdfix_sdk_builds/
  - cannot report whether trailer keys are direct or indirect
  - necessary runtime shared libraries/DLLs are in `TestGrammar/bin/...`
  - there are no debug symbols so when things go wrong it is difficult to know what and/or why

* Sarge: a light-weight C++ command line parser
  - command line options are kept aligned with Python PoCs
  - modified source code located in `./sarge`
  - originally from https://github.com/MayaPosch/Sarge
  - slightly modified to support wide-string command lines and remove compiler warnings across all platforms and builds  

* QPDF: an OSS PDF SDK (`ARL_PDFSDK_QPDF`)
  - still work-in-progress / incomplete - **DO NOT USE!**
  - download `qpdf-10.x.y-bin-msvc64.zip` from https://github.com/qpdf/qpdf/releases
  - place into `./qpdf`

* MuPDF: an OSS PDF SDK (_in the future_)

## Building

### Windows Visual Studio GUI

Open [/TestGrammar/platform/msvc2022/TestGrammar.sln](/TestGrammar/platform/msvc2022/TestGrammar.sln) with Microsoft Visual Studio 2022 and compile. Valid configurations are: 32 (`x86`) or 64 (`x64`) bit, Debug or Release. Compiled executables will be in [TestGrammar/bin/x64](./bin/x64) (64 bit) and [TestGrammar/bin/x86](./bin/x86) (32 bit), with debug builds ending  `..._d.exe`.

Under TestGrammar | Properties | C/C++ | Preprocessor, add the define to select the PDF SDK you wish to use: `ARL_PDFSDK_PDFIUM`, `ARL_PDFSDK_PDFIX` or `ARL_PDFSDK_QPDF` (_QPDF support is not currently working_)

### Windows Visual Studio command line

To build via `msbuild` command line in a Visual Studio 2019 "Native Tools" command prompt:

```dos
cd TestGrammar\platform\msvc20xx
msbuild -m TestGrammar.sln -t:Rebuild -p:Configuration=Debug -p:Platform=x64
```

Compiled binaries will be in [TestGrammar/bin/x86](./bin/x86) or [TestGrammar/bin/x64](./bin/x64). Debug binaries end with `..._d.exe`. In order to select the PDF SDK, either open and change in the Visual Studio IDE (_as above_) or hand edit [platform/msvc/2022/TestGrammar.vcxproj](platform/msvc/2022/TestGrammar.vcxproj) in an XML aware text editor:

```
<PreprocessorDefinitions>ARL_PDFSDK_PDFIUM;...
```

### Windows command line via CMake and nmake (MSVC)

[CMake](https://cmake.org/) for Windows is required. In a Visual Studio "Native Tools" command prompt:

```dos
cd TestGrammar
mkdir Win
cd Win
cmake -G "NMake Makefiles" -DPDFSDK_xxx=ON ..
nmake debug
nmake release
cd ..
```

where `xxx` is `PDFIUM`, `PDFIX` or `QPDF` (_QPDF support is not currently working_) - as in `PDFSDK_PDFIUM`. Compiled binaries will be in [TestGrammar/bin/x64](./bin/x64). Debug binaries end with `..._d.exe`.

### Linux

Note that due to C++17, gcc/g++ v8 or later is required. [CMake](https://cmake.org/) is also required. CMake system supports both Makefiles and Ninja build systems. `CMAKE_BUILD_TYPE` values include the standard set including `Debug` (binary ends with `_d`), `Release` and `RelWithDebInfo`:

```bash
# Makefile build system
cmake -B cmake-linux/debug -DPDFSDK_xxx=ON -DCMAKE_BUILD_TYPE=Debug .
cmake --build cmake-linux/debug --config Debug
./bin/linux/TestGrammar_d

# To delete everything before doing a clean build...
cmake --build cmake-linux/debug --target clean

# Ninja build system (alternative)
cmake -G Ninja -B cmake-ninja/release -DPDFSDK_xxx=ON -DCMAKE_BUILD_TYPE=Release .
ninja -C cmake-ninja/release
```

where `xxx` is `PDFIUM`, `PDFIX` or `QPDF` (_not currently working_) - as in `PDFSDK_PDFIUM`. Compiled Linux binaries will be in [TestGrammar/bin/linux](./bin/linux). Debug binaries end with `..._d`.

If using a PDFix build, then the shared library `libpdfix.so` must also be accessible. The following command may help:

```bash
export LD_LIBRARY_PATH=$PWD/bin/linux:$LD_LIBRARY_PATH
```


### Mac OS/X

Follow the instructions for Linux. Compiled binaries will be in [TestGrammar/bin/darwin](./bin/darwin).

## Code documentation

Run `doxygen Doxyfile` to generate full documentation for the TestGrammar C++ PoC application. Then open [./doc/html/index.html](./doc/html/index.html). `dot` is also required.


---

# TODO

- see the Doxygen output for miscellaneous improvements marked by `@todo` in the C++ source code
- when processing PDF files, calculate all predicates
- finish PDF SDK bindings for QPDF, MuPDF and a later/better version of pdfium
- detect stack overflows, memory exhaustion and timeouts to be able to fail gracefully

---

Copyright 2021 PDF Association, Inc. https://www.pdfa.org

This material is based upon work supported by the Defense Advanced Research Projects Agency (DARPA) under Contract No. HR001119C0079. Any opinions, findings and conclusions or recommendations expressed in this material are those of the author(s) and do not necessarily reflect the views of the Defense Advanced Research Projects Agency (DARPA). Approved for public release.
