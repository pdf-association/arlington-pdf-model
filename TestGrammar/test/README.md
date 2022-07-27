# Testing TestGrammar C++ PoCs

## Testing Validation

This folder contains a few "bad" Arlington TSV files that can be used to confirm that the Arlington PDF model validation is correctly performing certain checks. Feel free to add more checks.

Copy `XRefStream.tsv` and `FileTrailer.tsv` from the `../../tsv/latest` folder into this directory as these are hard-coded in the C++ as the "roots" of the PDF DOM. The use this folder as the argument to `--tsvdir`:

```bash
cp ../../tsv/latest/XRefStream.tsv .
cp ../../tsv/latest/FileTrailer.tsv .
TestGrammar --tsvdir . --validate
```

## Testing PDF processing and predicates

The PDF file `RuleBreaker-INVALID.pdf` is an **INVALID PDF** which has been purposely hacked to change specific key values to invalid values according to specific predicates encoded in the Arlington TSV model. Lines are also commented out in the PDF file which can be uncommented to test other options.

```bash
TestGrammar --tsvdir ../../tsv/latest --pdf RuleBreaker-INVALID.pdf
```
