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

The TestGrammar (C++17) proof of concept application is a command-line utility.
It performs a number of functions:

1. validates all TSV files in an Arlington TSV file set.
    - Check the uniformity (number of columns), if all types are one of Arlington types, etc.
2. validates a PDF file against an Arlington TSV file set. Starting from trailer, the tool validates:
    - if all required keys are present
    - if values are of correct type (_processing of predicates (declarative functions) are not fully supported_)
    - if objects are indirect if required (_requires pdfium PDF SDK to be used_)
    - if value is correct if `PossibleValues` are defined (_processing of predicates (declarative functions) are not fully supported_)
    - all error messages are prefixed with `Error:` or `Warning:` to enable post-processing
    - that all Arlington predicates can be parsed using a simple regex-based recursive descent parser
3. recursively validates a folder containing many PDF files.
    - for PDFs with duplicate filenames, an underscore is appended to the report filename to avoid overwriting.
4. compares the Arlington PDF model grammar with the Adobe DVA FormalRep
    - the output report needs manual review afterwards, as predicates will trigger differences


## Usage

The command line options are very similar to the Python proof-of-concept:

```
Arlington PDF Model C++ P.o.C. version vX.Y built <date>> <time> (<platform & compiler>)
Choose one of: --pdf, --checkdva or --validate.

Usage:
        TestGrammar --tsvdir <dir> [--out <fname|dir>] [--clobber] [--debug] [--brief] [--validate | --checkdva <formalrep> | --pdf <fname|dir> ]

Options:
-h, --help        This usage message.
-b, --brief       terse output when checking PDFs - no object numbers, details of errors, etc.
-c, --checkdva    Adobe DVA formal-rep PDF file to compare against Arlington PDF model.
-d, --debug       output additional debugging information (verbose!)
    --clobber     always overwrite PDF output report files (--pdf) rather than append underscores
-m, --batchmode   stop popup error dialog windows - redirect errors to console (Windows only)
-o, --out         output file or folder. Default is stdout. See --clobber for overwriting behavior
-p, --pdf         input PDF file or folder.
-t, --tsvdir      [required] folder containing Arlington PDF model TSV file set.
-v, --validate    validate the Arlington PDF model.

Built using <pdf-sdk> vX.Y.Z
```

## Notes

* TestGrammar PoC does NOT currently confirm PDF version validity, or understand most predicates (declarative functions). So there may be some false "Error:" messages!!

* all messages from validating PDF files are prefixed with `^Error:` or `^Warning:` to make regex-based post processing easier

* possible warning and error messages from PDF file validation are as follows. Each message may also provide context (e.g. a PDF object number) so long as `--brief` is **not** specified (default):

```
Error: EXCEPTION ...
Error: Failed to open ...
Error: failed to acquire Trailer ...
Error: can't select any link from ...
Error: not an indirect reference as required: ...
Error: wrong type: ...
Error: wrong value: ...
Error: non-inheritable required key doesn't exist ...
Error: inheritable required key doesn't exist ...
Error: object validated in two different contexts. First: ...
Error: array length incorrect for ...
Error: could not get value for key ...
Error: PDF array object encountered, but using Arlington dictionary
Error: name tree Kids array element number xxx was not a dictionary
Error: number tree Kids array element number xxx was not a dictionary
Error: name tree Kids object was not an array
Error: number tree Kids object was not an array
Error: name tree Names array element xxx was missing 2nd element in a pair
Error: name tree Names array element xxx error for 1st element in a pair (not a string?)
Error: name tree Names object was missing or not an array when Kids was also missing
Error: number tree Nums object was missing when Kids was also missing
Error: number tree Nums array was invalid
Error: number tree Nums array element xxx was not an integer
Error: number tree Nums array element xxx was not an object
Warning: key 'xxx' is not defined in Arlington for "yyy"
Warning: possibly wrong value (predicates NOT supported): ...
```

* the Arlington PDF model is based on PDF 2.0 (ISO 32000-2:2020) where some previously optional keys are now required
(e.g. font dictionary **FirstChar**, **LastChar**, **Widths**) which means that matching legacy PDFs will falsely report errors unless these keys are present.

* all output should have a final line "END" (`grep --files-without-match "^END"`) - if not then something nasty has happened (crash!?!)

* an exit code of 0 indicates successful processing.

* An exit code of -1 indicates an error: typically an encrypted PDF file requiring a password or with  unsupported crypto, or a corrupted PDF where the trailer cannot be located (this will also depend on the PDF SDK in use).

* if processing a folder of PDFs under Microsoft Windows, then use `--batchmode` so that if things do crash or assert then the error dialog box doesn't block unattended execution from continuing.

### Understanding errors and warnings

Most error and warning messages are obvious from the text. If `--brief` is **not** specified, then PDF object numbers may also be in the output which can help rapidly identify the cause of error and warning messages. Error and warning message always occur **after** the PDF path so adding a `-A 2` to a grep can provide such context.

`Error: object validated in two different contexts. First ...`

This error means that a direct PDF object (`x y obj ... endobj`) has been arrived at by two different paths in the PDF, but that the Arlington Link determination in each case resulted in two different object definitions (TSV filenames). This can happen if the Arlington PDF Model is under- or ill-specified (_is there sufficient information in the parent object to correctly determine which definition should get used? Is a predicate required?_) or if there is object reuse occurring in the PDF file. For example, a SoftMask Image XObject can be reused (just by being in the XObject Resources) as a normal Image XObject. Until predicates are fully supported, false error messages may occur more often than they should. Currently this error is **expected** for combined widget and field annotations, as permitted by sub-clause 12.5.6.19:

> As a convenience, when a field has only a single associated widget annotation, the contents of the field dictionary (12.7.4, "Field dictionaries") and the annotation dictionary may be merged into a single dictionary containing entries that pertain to both a field and an annotation.

This might be corrected in a future version of Arlington, by adding the Widget annotation keys to all field annotations (or vice-versa).

Note that TestGrammar uses a point-scoring system to resolve potential Link ambiguities, with Type and Subtype keys have a very strong influence, followed by other required keys. However disambiguation for arrays often occurs through context but, by design, TestGrammar does **not** track context - it merely follows all object references. Thus an array link may be incorrectly chosen...  

Inheritance is only tested for keys that are also "Required", as the required-ness condition can be met via inheritance. The algorithm uses recursive back-tracking following explicit `Parent` key references, which is currently sufficient for the Page Tree. It does **not** build a forward-looking stack, such as renderer might need to construct.     

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

The 5th line will report the PDF version, as reported by the PDF SDK being used. This may be the version from the PDF Header line (`%PDF-x.y`), the Document Catalog Version key, or both. PDF SDKs that do not support this functionality will report 2.0 (the latest PDF version):
```
PDF Header version X.Y
```

The last line should always be just `END` on a line by itself. If this is missing, then it means that the TestGrammar application has not cleanly completed processing (_crash? exception? assertion failure? timeout?_).

## Useful post-processing

After processing a tree of PDF files and saving the output into a folder via a command such as:

```bash
TestGrammar --batchmode --tsvdir ../arlington-pdf-model/tsv/latest --out . --pdf ../test/
```

the following Linux CLI commands can be useful in quickly filtering the output:

```bash
grep "^Error:" * | sort | uniq
grep "^Warning:" * | sort | uniq
grep -h "^Error:" * | cut -c 1-66 | sort | uniq
grep --files-without-match "^END" *
```

## Coding Conventions

* platform independent C++17 with STL and minimal other dependencies (_no Boost please!_)
* no tabs. 4 space indents
* std::wstrings need to be used for many things (such as PDF names and strings from PDF files) - don't assume ASCII or UTF-8!
* can assume Arlington TSV data is all ASCII/UTF-8
* liberal comments with code readability
* classes and methods use Doxygen style `///` comments (as supported by Visual Studio IDE)
* `/// @todo` are to-do comments
* zero warnings on all builds and all platforms (excepting PDF SDKs)
* all error messages matched with `^Error:` (i.e. case sensitive regex at start of a line)
* do not create unnecessary dependencies on specific PDF SDKs - isolate through the shim layer
* liberal use of asserts with the Arlington PDF model, which can be assumed to be correct (but never for data from PDF files!)
* performance and memory is **not** critical (this is just a PoC!) - so long as a full Arlington model can be processed and reasonably-sized PDFs can be checked

## PDF SDK Requirements

Checking PDF files requires a PDF SDK with certain key features (_we shouldn't need to write yet-another PDF parser!_). Key features required of a PDF SDK are:
* able to iterate over all keys in PDF dictionaries and arrays, including any additional keys not defined in the PDF spec
* able to test if a specific key name exists in a PDF dictionary
* able to report the true number of array elements  
* able to report key value type against the 9 PDF basic object types (integer, number, boolean, name, string, dictionary, stream, array, null)
* able to report if a key value is direct or an indirect reference - **this is a big limiting factor for many PDF SDKs!**  
* able to treat the trailer as a PDF dictionary and report if key values are direct or indirect references - **this is a big limiting factor for many PDF SDKs!**  
* able to report PDF object number for objects that are not direct - **this is a limiting factor for some PDF SDKs!**  
* not confuse values, such as integer and real numbers, so that they are expressed exactly as they appear in a PDF file - **this is a limiting factor for some PDF SDKs!**
* return the raw bytes from the PDF file for PDF name and string objects
* not do any PDF version based processing while parsing

Another recent discovery of behavior differences between PDF SDKs is when a dictionary key is an indirect reference to an object that is well beyond the trailer Size key or maximum cross-reference table object number. In some cases, the PDF SDK "sees" the key, allowing it to be detected and the error that it is invalid is deferred until the TestGrammar app attempts to resolve the indirect reference (e.g. PDFix). Then an error message such as `Error: could not get value for key XXX` will be generated. Other PDF SDKs completely reject the key and the key is not at all visible so no error about can be reported - the key is completely invisible when using such PDF SDKs (e.g. pdfium).

All code for a specific PDF SDK should be kept isolated in a single shim layer CPP file so that all Arlington specific logic and validation checks can be performed against the minimally simple API defined in `ArlingtonPDFShim.h`. There are `#defines` to select which PDF SDK to build with.

## Source code dependencies

TestGrammar has the following module dependencies:

* PDFium: OSS PDF SDK (`ARL_PDFSDK_PDFIUM`)
  - this is the **default PDF SDK used** as it provides the most functionality, has source code, and is debuggable if things don't work as expected
  - a local copy of pdfium is used to reduce the build complexity and time - and to fix a number of issues.
  - reduced source code located in `./pdfium`
  -  PDFium is also slightly modified in order to validate whether key values are direct or indirect references in trailer and normal PDF objects
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
  - still work-in-progress / incomplete - **DO NOT USE**
  - download `qpdf-10.x.y-bin-msvc64.zip` from https://github.com/qpdf/qpdf/releases
  - place into `./qpdf`

## Building

### Windows Visual Studio 2019 GUI

Open [/TestGrammar/platform/msvc2019/TestGrammar.sln](/TestGrammar/platform/msvc2019/TestGrammar.sln) with Microsoft Visual Studio 2019 and compile. Valid configurations are: 32 (`x86`) or 64 (`x64`) bit, Debug or Release. Compiled executables will be in [TestGrammar/bin/x64](./bin/x64) (64 bit) and [TestGrammar/bin/x86](./bin/x86) (32 bit).

Under TestGrammar | Properties | C/C++ | Preprocessor, add the define to select the PDF SDK you wish to use: `ARL_PDFSDK_PDFIUM`, `ARL_PDFSDK_PDFIX` or `ARL_PDFSDK_QPDF` (_QPDF support is not currently working_)

### Windows Visual Studio 2019 command line

To build via `msbuild` command line in a Visual Studio 2019 "Native Tools" command prompt:

```dos
cd TestGrammar\platform\msvc2019
msbuild -m TestGrammar.sln -t:Rebuild -p:Configuration=Debug -p:Platform=x64
```

Compiled binaries will be in [TestGrammar/bin/x86](./bin/x86) or [TestGrammar/bin/x64](./bin/x64). Debug binaries end with `..._d.exe`. In order to select the PDF SDK, either open and change in the Visual Studio IDE (_as above_) or hand edit [platform/msvc/2019/TestGrammar.vcxproj](platform/msvc/2019/TestGrammar.vcxproj) in an XML aware text editor:

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

Run `doxygen Doxyfile` to generate full documentation for the TestGrammar C++ PoC application. Then open [./doc/index](./doc/index). `dot` is also required.


---

# TODO

- see the Doxygen output for miscellaneous improvements
- when processing PDF files, also calculate predicates

---

Copyright 2021 PDF Association, Inc. https://www.pdfa.org

This material is based upon work supported by the Defense Advanced Research Projects Agency (DARPA) under Contract No. HR001119C0079. Any opinions, findings and conclusions or recommendations expressed in this material are those of the author(s) and do not necessarily reflect the views of the Defense Advanced Research Projects Agency (DARPA). Approved for public release.
