# GXCML - Java PoC utlity

Java-based proof of concept CLI utility that can:

1. convert an Arlington TSV file set into PDF version specific subsets (also as TSV)
    - these new Arlington TSV file sets will be in `./tsv/X.Y` subfolders
    - PDF version predicates such as `fn:SinceVersion`, `fn:Deprecated`, `fn:BeforeVersion` and `fn:IsPDFVersion` are processed for the Key and Type fields, with data structures from the future stripped out.

1. convert an Arlington TSV file set into XML files based on the [Arlington XSD schema](/xml/schema/arlington-pdf.xsd).
    - output XML files will be in `./xml` as `pdf-grammarX.Y.xml`
    - This can be verified using `xmllint --noout --schema xml/schema/arlington-pdf.xsd xml/pdf_grammarX.Y.xml`

1. gives answers to various researcher-type queries that illustrate how XPath can be used against the XML files.

To compile, run `ant` from this directory or use [Apache NetBeans](https://netbeans.apache.org/). Output JAR will be in `dist/gcxml.jar`.

## Notes

- The [Arlington XSD schema](/xml/schema/arlington-pdf.xsd) has been updated (and renamed) as a result of predicates and a more complex Arlington internal grammar. See [INTERNAL_GRAMMAR.md](../INTERNAL_GRAMMAR.md) for details.

- The following `xmllint` command can validate an Arlington XML file against [the Arlington XSD schema](/xml/schema/arlington-pdf.xsd):
    ```bash
    xmllint --noout --schema xml/schema/arlington-pdf.xsd xml/pdf_grammarX.Y.xml
    ```

- `xmllint` can also be used directly to query the XML:
    ```bash
    # Return all the elements that are values for PDF /Type keys
    xmllint --xpath "/PDF/OBJECT/ENTRY[NAME='Type']/VALUES/VALUE" xml/pdf_grammarX.Y.xml
    # What objects have a Type key?
    xmllint --xpath "/PDF/OBJECT/ENTRY[NAME='Type']/VALUES/VALUE/../../../@id" xml/pdf_grammarX.Y.xml
    # Return XML descriptions of keys in any XObject that are a matrix or rectangle
    xmllint --xpath "/PDF/OBJECT[contains(@id,'XObject')]/ENTRY/VALUES/VALUE[@type='matrix' or @type='rectangle']/../parent::node()" xml/pdf_grammarX.Y.xml
    ```

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
