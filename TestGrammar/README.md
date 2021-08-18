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

The TestGrammar (C++17) proof of concept application is a command-line utility based on the free [PDFix SDK Lite](https://pdfix.net/download-free/).
It performs a number of functions:

1. validates all TSV files in an Arlington TSV file set.
    - Check the uniformity (number of columns), if all types are one of Arlington types, etc.
    - Does **NOT** yet check or validate predicates (declarative functions)
2. validates a PDF file against an Arlington TSV file set. Starting from trailer, the tool validates:
    - if all required keys are present (_inheritance is currently not implemented_)
    - if values are of correct type (_processing of predicates (declarative functions) are not supported_) 
    - if objects are indirect if required (_requires pdfium PDF SDK to be used_) 
    - if value is correct if `PossibleValues` are defined (_processing of predicates (declarative functions) are not supported_)
    - all error messages are prefixed with `Error:` to enable post-processing
3. recursively validates a folder containing many PDF files.
    - for PDFs with duplicate filenames, an underscore is appended to the report filename to avoid overwriting.
4. compares the Arlington PDF model grammar with the Adobe DVA FormalRep
    - the output report needs manual review afterwards


## Usage

The command line options are very similar to the Python proof-of-concept:

```
Arlington PDF Model C++ P.o.C. version v x.y built xxx
Choose one of: --pdf, --checkdva or --validate.

Usage:
        TestGrammar --tsvdir <dir> [--out <fname|dir>] [--debug] [--validate | --checkdva <formalrep> | --pdf <fname|dir> ]

Options:
-h, --help       This usage message.
-t, --tsvdir     [required] folder containing Arlington PDF model TSV file set.
-o, --out        output file or folder. Default is stdout. Existing files will be overwritten.
-p, --pdf        input PDF file or folder.
-c, --checkdva   Adobe DVA formal-rep PDF file to compare against Arlington PDF model.
-v, --validate   validate the Arlington PDF model.
-d, --debug      output additional debugging information (verbose!)

Built using some-PDF-SDK v x.y.z
```

## Notes

* TestGrammar PoC does NOT currently confirm PDF version validity, understand any predicates (declarative functions), or support inheritance

* all error messages from validating PDF files are prefixed with `^Error:` to make regex-based post processing easier

* possible error messages from PDF file validation are as follows. Each error message also provides some context (e.g. a PDF object number):

```
Error: EXCEPTION ...
Error: Failed to open ...
Error: failed to acquire Trailer ...
Error: Can't select any link from ...
Error: not an indirect reference as required: ...
Error: wrong type: ...
Error: wrong value: ...
Error: non-inheritable required key doesn't exist
Error: Error: required key doesn't exist (inheritance not checked) ...
Error: object validated in two different contexts. First: ...
```

* the Arlington PDF model is based on PDF 2.0 (ISO 32000-2:2020) where some previously optional keys are now required
(e.g. font dictionary **FirstChar**, **LastChar**, **Widths**) which means that matching legacy PDFs will falsely report errors unless these keys are present.
A warning such as the following will be issued (because PDF 2.0 required keys are not present in legacy PDFs so there is no precise match):

```
Error: Can't select any link from \[FontType1,FontTrueType,FontMultipleMaster,FontType3,FontType0,FontCIDType0,FontCIDType2\] to validate provided object: xxx
```

## Coding Conventions

* platform independent C++17 with STL and minimal other dependencies (_no Boost please!_)
* no tabs. 4 space indents
* wstrings need to be used for many things (such as PDF names and strings) - don't assume ASCII or UTF-8!
* liberal comments with code readability
* everything has Doxygen style `///` comments (as supported by Visual Studio IDE)
* zero warnings on all builds and all platforms
* all error messages matched with `^Error:` (i.e. case sensitive regex at start of a line)
* do not create unncessary dependencies on specific PDF SDKs - isolate through a shim layer
* performance and memory is **not** critical (this is just a PoC!) - so long as a full Arlington model can be processed and reasonably-sized PDFs can be checked

## PDF SDK Requirements

Checking PDF files requires a PDF SDK with certain key features (_we shouldn't need to write yet-another PDF parser!_). Key features required of a PDF SDK are:
* able to iterate over all keys in PDF dictionaries and arrays, including any additional keys not defined in the PDF spec
* able to test if a specific key name exists in a PDF dictionary
* able to report the true number of array elements  
* able to report key value type against the 9 PDF basic object types
* able to report if a key value is direct or an indirect reference - **this is a limiting factor for many PDF SDKs!**  
* able to treat the trailer as a PDF dictionary and report if key values are direct or indirect references - **this is a limiting factor for many PDF SDKs!**  
* able to report PDF object and generation numbers for objects that are not direct - **this is a limiting factor for some PDF SDKs!**  
* not confuse values, such as integer and real numbers, so that they are expressed exactly as they appear in a PDF file - **this is a limiting factor for some PDF SDKs!**
* return the raw bytes from the PDF file for PDF name and string objects 
* not do any PDF version based processing while parsing

All code for a specific PDF SDK should be kept isolated in a single shim layer CPP file so that all Arlington specific logic and validation checks can be performed against the minimally simple API defined in `ArlingtonPDFShim.h`.

## Source code dependencies

TestGrammar has the following module dependencies:

* PDFium: OSS PDF SDK
  - this is the **default PDF SDK used** as it provides the most functionality, has source code, and is debuggable if things don't work as expected
  - reduced source code located in `./pdfium`
  -  PDFium is slightly modified in order to validate whether key values are direct or indirect references in trailer and normal PDF objects
  - see `src/ArlingtonPDFShimPDFium.cpp`

* PDFix: a free PDF SDK
  - see `src/ArlingtonPDFShimPDFix.cpp`
  - single .h dependency [src/Pdfix.h](src/Pdfix.h)
  - see https://pdfix.net/ with SDK documentation at https://pdfix.github.io/pdfix_sdk_builds/
  - cannot report whether trailer keys are direct or indirect
  - necessary runtime dynamic libraries/DLLs are in `TestGrammar/bin/...`
  - there are no debug symbols so when things go wrong it can be difficult to know what and why

* Sarge: a light-weight C++ command line parser
  - command line options are kept aligned with Python PoCs
  - modified source code located in `./sarge`
  - originally from https://github.com/MayaPosch/Sarge
  - slightly modified to support wide-string command lines and remove compiler warnings across all platforms and buids  

* QPDF: an OSS PDF SDK
  - still work-in-progress
  - download `qpdf-10.x.y-bin-msvc64.zip` from https://github.com/qpdf/qpdf/releases
  - place into `./qpdf`

## Building

### Windows Visual Studio

Open [/TestGrammar/platform/msvc2019/TestGrammar.sln](/TestGrammar/platform/msvc2019/TestGrammar.sln) with Microsoft Visual Studio 2019 and compile. Valid configurations are: 32 (`x86`) or 64 (`x64`) bit, Debug or Release. Compiled executables will be in [/TestGrammar/bin/x64](/TestGrammar/bin/x64) (64 bit) and [/TestGrammar/bin/x86](/TestGrammar/bin/x86) (32 bit).

To build via `msbuild` command line in a Visual Studio Native Tools command prompt:

```dos
cd TestGrammar\platform\msvc2019
msbuild -m TestGrammar.sln -t:Rebuild -p:Configuration=Debug -p:Platform=x64
```

To build in a Visual Studio Native Tools command prompt using nmake via CMake for Windows using ["out of source" builds](https://gitlab.kitware.com/cmake/community/-/wikis/FAQ#out-of-source-build-trees):

```dos
cd TestGrammar
mkdir WinDebug
cd WinDebug
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug ..
nmake
cd ..
mkdir WinRelease
cd WinRelease
cmake -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release ..
nmake
cd ..
```

where `Configuration` can be either `Debug` or `Release`, and `Platform` can be either `x64` or `x86` (32 bit). Compiled binaries will be in [/TestGrammar/bin/x86](/TestGrammar/bin/x86) or [/TestGrammar/bin/x64](/TestGrammar/bin/x64).

### Linux

Note that due to C++17, gcc/g++ v8 or later is required. CMake is also required.

```bash
mkdir LinDebug
cd LinDebug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
cd ..
mkdir LinRelease
cd LinRelease
cmake -DCMAKE_BUILD_TYPE=Release ..
make
cd ..
```

Compiled binaries will be in [/TestGrammar/bin/linux](/TestGrammar/bin/linux).


### Mac OS/X

Follow the instructions for Linux. Compiled binaries will be in [/TestGrammar/bin/darwin](/TestGrammar/bin/darwin).

---

# TODO :pushpin:

- when validating the TSV data files, also do a validation on the predicate (declarative function) expressions

- when validating a PDF file, check required values in parent dictionaries when inheritance is allowed.

- extend TestGrammar with new feature to report all keys that are NOT defined in any PDF specification (as this may indicate either proprietary extensions, undocumented legacy extensions or common errors/malformations from PDF writers).

---

Copyright 2021 PDF Association, Inc. https://www.pdfa.org

This material is based upon work supported by the Defense Advanced Research Projects Agency (DARPA) under Contract No. HR001119C0079. Any opinions, findings and conclusions or recommendations expressed in this material are those of the author(s) and do not necessarily reflect the views of the Defense Advanced Research Projects Agency (DARPA). Approved for public release.
