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
    - Does NOT check declarative functions
2. validates a PDF file against an Arlington TSV file set. Starting from trailer, the tool validates:
    - if all required keys are present
    - if values are of correct type
    - if objects are indirect if required
    - if value is correct if PossibleValues are defined
    - all error messages are prefixed with "Error:" to enable post-processing
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

Built using PDFix v x.y.z
```

## Notes

* TestGrammar PoC does NOT currently confirm PDF version validity, understand any predicates (declarative functions), or support inheritance

* all error messages from validating PDF files are prefixed with "Error:" to make regex easier

* possible error messages from PDF file validation are as follows. Each error message also provides some context (e.g. a PDF object number):

```
Error: EXCEPTION ...
Error: Failed to open ...
Error: failed to acquire Trailer ...
Error: Can't select any link from ...
Error: not indirect ...
Error: wrong type ...
Error: wrong value ...
Error: required key doesn't exist ...
Error: object validated in two different contexts ...
```

* the Arlington PDF model is based on PDF 2.0 (ISO 32000-2:2020) where some previously optional keys are now required
(e.g. font dictionary **FirstChar**, **LastChar**, **Widths**) which means that matching legacy PDFs will falsely report errors unless these keys are present.
A warning such as the following will be issued (because PDF 2.0 required keys are not present in legacy PDFs so there is no precise match):

```
Error: Can't select any link from \[FontType1,FontTrueType,FontMultipleMaster,FontType3,FontType0,FontCIDType0,FontCIDType2\] to validate provided object: xxx
```


## Coding Conventions

* platform independent C++17 with STL and minimal other dependencies (no Boost!)
* no tabs. 4 space indents
* liberal comments with code readability
* everything has Doxygen style `///` comments (as supported by Visual Studio IDE)
* zero warnings on all builds and all platforms
* all error messages start with "Error:" (i.e. case sensitive regex)
* do not create unncessary dependencies on specific PDF SDKs - isolate through a layer
* performance and memory is not critical (this is just a PoC!) - so long as a full Arlington model can be processed and reasonably-sized PDFs can be checked


## Source code dependencies

TestGrammar has the following module dependencies:

* PDFix: a free PDF SDK
  - single .h dependency [src/Pdfix.h](src/Pdfix.h)
  - see https://pdfix.net/ with SDK documentation at https://pdfix.github.io/pdfix_sdk_builds/
  - necessary runtime dynamic libraries/DLLs are in `TestGrammar/bin/...`
  - there are no debug symbols so when things go wrong it can be difficult to know what and why

* Sarge: a light-weight C++ command line parser
  - place in `src\Sarge`
  - see https://github.com/MayaPosch/Sarge
  - slightly modified to support wide-string command lines and remove compiler warnings

* QPDF: an OSS PDF SDK
  - download `qpdf-10.x.y-bin-msvc64.zip` from https://github.com/qpdf/qpdf/releases
  - unzip into `src\qpdf`

## Building

### Windows Visual Studio

After installing the dependencies, open [/TestGrammar/platform/msvc2019/TestGrammar.sln](/TestGrammar/platform/msvc2019/TestGrammar.sln) with Microsoft Visual Studio 2019 and compile. Valid configurations are: 32 or 64 bit, Debug or Release. Compiled executables will be in [/TestGrammar/bin/x64](/TestGrammar/bin/x64) (64 bit) and [/TestGrammar/bin/x86](/TestGrammar/bin/x86) (32 bit).

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

Note that due to C++17, gcc v8 or later is required. CMake is also required.

```bash
cd TestGrammar
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
