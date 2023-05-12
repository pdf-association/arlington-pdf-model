% TestGrammar(1) TestGrammar
% Peter Wyatt, Roman Toda, Frantisek Forgac
% January 2023

# NAME
TestGrammar - a multi-platform portable proof-of-concept Arlington PDF Model application written in C++17

# SYNOPSIS

**TestGrammar** [ OPTIONS ] `--validate`

**TestGrammar** [ OPTIONS ] `--checkdva` _`<file>`_

**TestGrammar** [ OPTIONS ] `--pdf` _`<fname|dir|@file.txt>`_

**TestGrammar_d** is the debug version of **TestGrammar**.

# DESCRIPTION

**TestGrammar** is a proof-of-concept Arlington PDF Model command line application written in C++17 that can be used to validate an Arlington PDF model TSV data set, compare an Arlington PDF model TSV data set against an Adobe DVA definition, or check a PDF file against an Arlington PDF model TSV data set and report all differences. Running **TestGrammar** without any command line parameters will display a brief usage message.

# OPTIONS

Options may be specified in any order and single letter and long-form options may be mixed. Many options have single letter abbreviations which always use a single _`-`_ (DASH). Long-form options aways require two DASHES _`--`_. Some options also have mandatory additional arguments:

**-h, --help**
: Display a brief usage message.

**-b, --brief**
: Output tersely when checking PDFs. Only report a context line in the PDF DOM for any informative, warning or error messages. The full PDF DOM tree is not output when this option is used. It is strongly recommended to use this option when processing either a large PDF file or large numbers of PDF files to reduce output file size and to improve performance.

**-c, --checkdva** _`<dva-file>`_
: Compare an Adobe DVA "formal-rep" PDF definition against an Arlington PDF model TSV file set. On Microsoft Windows, the Adobe DVA "formal-rep" file for PDF 1.7 is located at _`C:\Program Files (x86)\Adobe\Acrobat DC\Acrobat\plug_ins\Preflight\PDF1_7FormalRep.pdf`_. Note that because Adobe DVA defines PDF 1.7 and Arlington defines PDF 2.0 there will always be differences!

**-d, --debug**
: Output additional debugging information into the output file. This additional information can make post-processing of output more difficult as file-specific information is included. For **--pdf** this will include PDF file specific information such as the object number or value of an object. For **--validate** this will include data that needs to be manually checked. For **--checkdva** this will include a list of Arlington TSV files that don't have an Adobe DVA equivalent and a list of Adobe DVA objects that did not have an Arlington equivalent. 

**--clobber**
: always overwrite PDF output report files (**--out**) rather than append sufficient underscores to always create a new output file. The actual filename is recorded inside each output file.

**--no-color**
: disable colorized output (useful when redirecting or piping output). Colorized output is done using ANSI escape sequences with red for errors, yellow for warnings, and cyan for informative messages. When this option is specified, all output files will have the extension _.txt_. When this options is not specified, output files will have the extension _.ansi_ indicating that ANSI color escape sequences are present. 

**-m, --batchmode**
: Stop popup error dialog windows from blocking batch processing and redirect everything to console. Microsoft Windows only, includes memory leak reports.

**-o, --out** _`< file | folder >`_
: file or folder . Default is stdout for a single PDF or current folder if processing multiple PDFs. See also **--clobber** for overwriting behavior.

**-p, --pdf** _`< file | folder | @filelist.txt >`_
: input PDF file, root folder for recursive processing, or a text file containing a list of PDF files/folders (one per line) if starting with _`@`_. Comment lines indicated by _`#`_ (HASH) and blank lines will be ignored.

**-f, --force** _`< 1.0 | 1.1 | 1.2 | 1.3 | 1.4 | 1.5 | 1.6 | 1.7 | 2.0 | exact >`_
: Force the PDF version to the specified value (_1,0_, _1.1_, ..., _2.0_) or _exact_ to use the version that each PDF file specifies. PDF versioning uses the correct logic involving both the PDF Header lines (_%PDF-x.y_) and the optional Document Catalog Version key. Only applicable to **--pdf**. By default (i.e. when this option is not specified), and because so many real-world PDF files get their PDF version wrong, files with a PDF version of 1.4 to 1.7 will be automatically rounded up and processed as PDF 1.7! Using this option wisely can reduce the occurence of informative messages regarding "use before introduction" or "use of deprecated feature" messages.

**-t, --tsvdir** _`<folder>`_
: Required option specifying the folder containing an Arlington PDF model TSV file set. It is very strongly recommend to **--validate** the file set in this folder! The vast majority of the time the _./tsv/latest_ folder Arlington PDF Model data set will be used.

**-v, --validate**
: validate an Arlington PDF model TSV file set for internal consistency, typos, etc. This not necessarily going to locate every possible error with the TSV data files, but it will avoid most runtime errors or false output when using **--pdf**.  This should be run prior to **--pdf** and **--checkdva**. May also be combined with the **--debug** option to report each TSV file as it is processed as well as warning messages for limitations in the internal grammar check that need to be confirmed manually.

**-e, --extensions** _`< * | extension1[,extension2...] >`_
: a comma-separated list of PDF extensions in the Arlington PDF model to support, or _`*`_ (ASTERISK) for all extensions. SPACES must not be used. Note that a BACKSLASH is needed to prevent Linux shells from glob expanding if _`*`_ (ASTERISK) is being used. Only applicable to **--pdf**.

**--password** _`<pwd>`_
: specify a password string for all PDF files. Only applicable to **--pdf**. Note that due to the locale setting of your shell, some Unicode passwords may not be possible to enter correctly! Quoting may also be necessary.

**--exclude** _`< string | @filelist.txt >`_
: PDF exclusion string (no SPACES) or a text file containing a list of filenames or folders to exclude from processing with one entry per line if starting with _`@`_. Comment lines indicated by _`#`_ (HASH) and blank lines will be ignored. Only applicable to **--pdf**. Files explicitly excluded via this option will still be logged to console.

**--dryrun**
: Dry run - don't do any actual processing. Useful when also combined with **--debug** to see behaviour of a complex command line options.
This option is most useful when **--debug** is also specified and when **-pdf**, _\@_ file lists, and/or **--exclude** is used so that the full set of PDF files that will be analyzed can be efficiently confirmed ahead of actual (slow) processing. Note that zero-length output files will get created according to the **--out** option. By comparing the number of input PDF files against the output files, filename collisions can be identified ahead of time. It will also identify any file system issues ahead of time.

**-a, --allfiles**
: Applies only to the **--pdf** option. Process all files as PDFs regardless of file extension. When this option is not specified, only files with an explicit _.pdf_ extension are processed. This is useful for robustness testing when non-PDF are attempted to be processed, as well as for corpora such as SafeDocs CommonCrawl refetch which uses SHA-256 file hashes as filenames and no file extensions.

# EXAMPLES

Check (validate) the internal grammar consistency of an Arlington PDF Model TSV file set. Output (as colored text) goes to console:

```
TestGrammar --tsvdir ./tsv/latest --validate
TestGrammar -t ./tsv/latest -v
```

Process a single password-protected PDF against the previously validated Arlington PDF Model TSV file set including all defined extensions using the explicit PDF version defined in _test.pdf_. Object numbers, etc will be output for any differences detected. Output (as colored text) goes to console:

```
TestGrammar --tsvdir ./tsv/latest --brief --debug --force exact --extensions \* --password "top secret" --pdf ~/files/test.pdf
```

Recursively process a folder subtree potentially containing many PDF files with extension _.pdf_ against the Arlington PDF Model TSV file set and output as pure text (uncolored) _.txt_ files. First do as a dry run to check file permissions, etc. Logging (as colored text) will go _out.log_ when doing actual processing which could be quite slow. The _nohup_ Linux command might also be prepended when processing large numbers of PDFs:

```
TestGrammar --tsvdir ./tsv/latest --brief --out /tmp --no-color --pdf ~/files/ --dryrun
TestGrammar --tsvdir ./tsv/latest --brief --out /tmp --no-color --pdf ~/files/ > out.log 2>&1
```


# EXIT VALUES
**0**
: Success

**non-zero**
: Failure - either bad options or a fatal runtime error/crash.

# BUGS

Please see [https://github.com/pdf-association/arlington-pdf-model/issues](https://github.com/pdf-association/arlington-pdf-model/issues).

This man page can be tested using the following command:

```
pandoc TestGrammar.1.md -s -t man | /usr/bin/man -l -
```

This man page can be generated and installed using the following commands:

```
pandoc TestGrammar.1.md -s -t man -o TestGrammar.1
gzip TestGrammar.1
mv TestGrammar.1.gz ~/.local/share/man/man1
mandb
```

# COPYRIGHT

Copyright 2020-2023 PDF Association, Inc. [https://www.pdfa.org](https://www.pdfa.org).

Licensed under [Apache-2.0](https://github.com/pdf-association/arlington-pdf-model/blob/master/LICENSE)

This material is based upon work supported by the Defense Advanced Research Projects Agency (DARPA) under Contract No. HR001119C0079.
Any opinions, findings and conclusions or recommendations expressed in this material are those of the author(s) and do not necessarily reflect the views of the Defense Advanced Research Projects Agency (DARPA). Approved for public release.
