# Testing TestGrammar C++ PoCs

This folder contains a few "bad" Arlington TSV files that can be used to confirm that the Arlington PDF model validation is correctly performing certain checks. Feel free to add more checks.

Copy `XRefStream.tsv` and `FileTrailer.tsv` from the `../../tsv/latest` folder into this directory as these are hard-coded in the C++ as the "roots" of the PDF DOM. The use this folder as the argument to `--tsvdir`:

```bash
cp ../../tsv/latest/XRefStream.tsv .
cp ../../tsv/latest/FileTrailer.tsv .
./TestGrammar --tsvdir . --validate
```
