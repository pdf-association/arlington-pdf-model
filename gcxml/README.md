# GXCML - Java PoC utlity

Java-based proof of concept CLI utility that can:

1. convert an Arlington TSV file set into PDF version specific subsets

1. convert an Arlington TSV file set into XML files that are valid based on [this XSD schema](/xml/schema/arlington-pdf.xsd).
    - This can be verified using `xmllint --noout --schema xml/schema/arlington-pdf.xsd xml/pdf_grammarX.Y.xml`

1. gives answers to various researcher-type queries

To compile, run `ant` from this directory or use [Apache NetBeans](https://netbeans.apache.org/). Output JAR will be in `dist/gcxml.jar`.

## Notes

- The [schema](/xml/schema/arlington-pdf.xsd) has been updated as a result of predicates and a more complex Arlington internal grammar. See [INTERNAL_GRAMMAR.md](../INTERNAL_GRAMMAR.md) for details.

## Usage

To use gcxml tool run the following command from terminal/command line in the top-level folder (such that `./tsv/` and `./xml/` are sub-folders):

```
java -jar ./gcxml/dist/gcxml.jar

GENERAL:
    -version        print version information (current: x.y)
    -help           show list of available commands
CONVERSIONS:
    -all            convert latest TSV to XML and TSV sub-versions for each specific PDF version
    -xml <version>  convert TSV to XML for specified PDF version (or all if no version is specified)
    -tsv            create TSV files for each PDF version
QUERIES:
    -sin <version | -all>   return all keys introduced in ("since") a specified PDF version (or all)
    -dep <version | -all>   return all keys deprecated in a specified PDF version (or all)
    -kc         return every key name and their occurrence counts for each version of PDF
    -po key<,key1,...>  return list of potential objects based on a set of given keys for each version of PDF
    -sc         list special cases for every PDF version
    -so         return objects that are not defined to have key Type, or where the Type key is specified as optional
```

**Note**: output might be too long to display in terminal, so it is recommended to always redirect the output to file.

The XML version of the PDF DOM grammar (one XML file per PDF version called `pdf_grammarX.Y.xml`) is created from the TSV files and written to the `./xml/` subfolder.

All of the answers to queries are based on processing the XML files in the `./xml/` folder in the top-level folder.
