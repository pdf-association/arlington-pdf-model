# Extending and customizing the Arlington PDF Model

As the Arlington PDF model is defined using simple text-based TSV files, it is very easy to extend or caustomize the data model to match a specific implementation or additional sets of requirements by patching an Arlington TSV model set of files.

For example, extensions can suppress messages regarding specific malformations or proprietary extensions by simply adding definitions to the appropriate TSV file. Because these are not part of the official ISO PDF specification, these are referred to generically as "_extensions_" and the "SinceVersion" field (column 3) which normally contains a PDF version is replaced by a predicate: `fn:Extension(name,x.y)` or `fn:Extension(name)` depending on whether the extension is version-based or not.

Each implementation that uses the Arlington PDF data model must decide which, if any, extensions it wants to include whether by default or possibly through a configuration option.

## Extensions

The names of extensions are somewhat arbitrary but must following the conventions used in Arlington for keys: alphanumerics with UNDERSCORE. No SPACES, COMMAs or MINUS (dash). This file documents those extensions which have been implemented. A simple `grep` for the extension name will find all occurences.

### Adobe Extensions to ISO 32000-1:2008 ("ADBE_Extn3" and "ADBE_Extn5")

Extends Arlington to include both the [Adobe Extension Levels 3 and 5](https://www.pdfa.org/resource/pdf-specification-index/) on top of ISO 32000-1:2008 (PDF 1.7). In many cases, these are features that were later adopted into PDF 2.0 by ISO TC 171 SC 2 WG 8, but this extension defines them as valid for PDF 1.7.

### Apple ("AAPL")

Additional entries commonly seen from Apple software:

- Adds `AAPL:Keywords` to [`DocInfo.tsv`](./tsv/latest/DocInfo.tsv) ([see doco](https://developer.apple.com/documentation/coregraphics/kcgpdfcontextkeywords))
- Adds `AAPL:AA` boolean to [`GraphicsStateParameter.tsv`](./tsv/latest/GraphicsStateParameter.tsv) 
- Adds a `AAPL:ST` Style dictionary to [`GraphicsStateParameter.tsv`](./tsv/latest/GraphicsStateParameter.tsv) for the Gausian blur drop shadow and adds a new dictionary object in [`AAPL_ST.tsv`](./tsv/latest/AAPL_ST.tsv)

### EA-PDF PDF/mail-1 ("EAPDF_1")

Adds support for [EA-PDF PDF/mail-1](https://pdfa.org/resource/ea-pdf/) files, where IANA media type parameters are stored in a `Mail_MediaTypeParameters` key in [`EmbeddedFileStream.tsv`](./tsv/latest/EmbeddedFileStream.tsv).

### PDF/VT-2 ("PDF_VT2")

Adds [ISO 16612-2:2010](https://pdfa.org/resource/iso-16612-pdfvt/) PDF/VT-2 support. DParts/DPM were later adopted and standardized as part of PDF 2.0 (ISO 32000-2), so this extension defines them as valid from PDF 1.6.

- In this case, the "SinceVersion" field will have `fn:Extension(PDF_VT2,1.6) || 2.0)` for those keys that were adopted, or just `fn:Extension(PDF_VT2,1.6)` for those keys specific to PDF/VT-2.
- PDF/VT-2 is based on PDF/X-4 (ISO 15930-7:2010) which is based on Adobe PDF 1.6.

### ISO TS 24064 ("ISO_TS_24064")

Adds [STEP AP 242](https://pdfa.org/resource/3d-formats/) support as another 3D format for [`3DStream.tsv`](./tsv/latest/3DStream.tsv), and a new requirements dictionary in [`RequirementsSTEP.tsv`](./tsv/latest/RequirementsSTEP.tsv).

### ISO TS 24654 ("ISO_TS_24654")

This proposed ISO extension to PDF 2.0 was never finalized and published, but this extension adds `Path` to [`AnnotLink.tsv`](./tsv/latest/AnnotLink.tsv) for supporting non-rectangular links.

### ISO TS 32001 ("ISO_TS_32001")

Adds the new hash and digest algorithms for this PDF 2.0 ISO technical specification extension. 

### ISO TS 32003 ("ISO_TS_32003")

Adds 256-bit AES-GCM support to PDF 2.0 by specifying additional values for some keys in the encryption dictionaries.

- Note that because encryption results in all streams and strings being encrypted, implementations and PDF SDK support will vary.

### ISO TS 32004 ("ISO_TS_32004")

Adds `KDFSalt` to the encryption dictionaries, `AuthCode` to the [conventional file trailer dictionary](./tsv/latest/FileTrailer.tsv) and [cross-reference stream dictionary](./tsv/latest/XRefStream.tsv), and a new dictionary object in [`AuthCode.tsv`](./tsv/latest/AuthCode.tsv).

### ISO TS 320037 ("ISO_TS_32007")

### PDF/A-3 ("ISO_19005_3")

Adds support for PDF/A-3 when Associated Files were first added to PDF 1.7. This adds the `AF` to many objects and `AFRelationship` to the [`FileSpecification.tsv`](./tsv/latest/FileSpecification.tsv).

- See [Issue #113](https://github.com/pdf-association/arlington-pdf-model/issues/113).

### ISO 19593 ("ISO_19593")

Adds support for ISO 19593-1 _Graphic technology - Use of PDF to associate processing steps and content data - Part 1: Processing steps for packaging and labels_ (commonly known as "processing steps").

### ISO 21812 ("ISO_21812")

Adds support for ISO 21812-1 _Graphic technology - Print product metadata for PDF files - Part 1: Architecture and core requirements for metadata_ (commonly known as "PPM") with many `CIP4` related objects.

- See [Issue #123](https://github.com/pdf-association/arlington-pdf-model/issues/123).

### ETSI PAdES ("ETSI_PAdES")

Adds support for ETSI EN 319 142-1 (PAdES) which pre-dates the incorporation of DSS, VSS and TSS features into PDF 2.0 (see [`DocTimeStamp.tsv`](./tsv/latest/DocTimeStamp.tsv)). ETSI uses different wording to ISO 32000-2:2020.

- See [Issue #121](https://github.com/pdf-association/arlington-pdf-model/issues/121)

### OpenOffice ("OpenOffice")

Adds support for additional entries in the  [conventional file trailer dictionary](./tsv/latest/FileTrailer.tsv) and [cross-reference stream dictionary](./tsv/latest/XRefStream.tsv).

- See [Issue #111](https://github.com/pdf-association/arlington-pdf-model/issues/111).

### Well Tagged PDF ("WTPDF")

Adds support for "[Well-Tagged PDF (WTPDF) Using Tagged PDF for Accessibility and Reuse in PDF 2.0](https://pdfa.org/wtpdf/)" when a new attribute owner `FENote` was added along with attribute `NoteType` to [`StructureAttributesDict.tsv`](./tsv/latest/StructureAttributesDict.tsv).

- See [Issue #106](https://github.com/pdf-association/arlington-pdf-model/issues/106).

### C2PA ("C2PA")

Adds support for a new key value for `AFRelationship` being `C2PA_Manifest`.

- See [Issue #112](https://github.com/pdf-association/arlington-pdf-model/issues/112).

### Commonly seen malformations ("Malforms")

Adds misspelled `SubType` key to [`OptContentCreatorInfo.tsv`](./tsv/latest/OptContentCreatorInfo.tsv) as an alternate spelling of `Subtype` and misspelled `Blackls1` for `BlackIs1` (_lowercase L instead of uppercase i_) in [`FilterCCITTFaxDecode.tsv`](./tsv/latest/FilterCCITTFaxDecode.tsv):

- the existing row is simply duplicated with the key spelling then changed and the official "SinceVersion" PDF version replaced with the extension predicate: `fn:Extension(Malforms)`.
- because Optional Content was only introduced in PDF 1.5, the `SubType` malform predicate also uses the `fn:Extension(Malforms,1.5)` predicate to further express this requirement for this misspelled key

## Useful Linux commands

```bash
# See all the details for all extensions in an Arlington data set
grep -P "\t[^\t]*fn:Extension\([^\t]+" *

# A list of all the unique extension names in an Arlington data set
grep -Pho "fn:Extension\([^\t)]+\)\)?" * | sort | uniq
```

## The TestGrammar (C++) Proof-of-concept

The [TestGrammar (C++ PoC)](./TestGrammar/README.md) CLI option `-e` or `--extensions` is used to specify a COMMA-separated list of case-sensitive extension names to support. Enabling support means that keys matching these extension names will **not** get reported as unknown keys and that these keys will also be further checked against their Arlington definitions. The default is not to enable any extensions - i.e. it is a "pure" check against ISO 32000-2:202. Use `--extensions *` (or `--extensions \*` on Linux to stop globbing by bash) to include all extensions when checking the PDF file.

Note that the PDF Extensions Dictionary (ISO 32000-2:2020, 7.12) in the PDF file is **not** consulted!

```bash
TestGrammar --brief --tsvdir ./tsv/latest --extensions AAPL,Malforms --pdf /tmp/folder_of_pdfs/ --out /tmp/out
TestGrammar --brief --tsvdir ./tsv/latest --extensions \* --force 2.0 --pdf /tmp/folder_of_pdfs/ --out /tmp/out
TestGrammar --brief --tsvdir ./tsv/latest --extensions \* --force exact --pdf /tmp/folder_of_pdfs/ --out /tmp/out
```
