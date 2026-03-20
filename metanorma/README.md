# Using Arlington with Metanorma

First generate the JSON or YAML equivalent outputs. There are both file sets and single (combined) file variants:

```sh
python3 .\arlington-to-json.py -t ..\tsv\latest\ -s ..\json > arl-set-json.log   
python3 .\arlington-to-json.py -t ..\tsv\latest\ -s ..\json\arlington.json -c > arl-combined-json.log
python3 .\arlington-to-json.py -t ..\tsv\latest\ -s ..\yaml --yaml > arl-set-yaml.log
python3 .\arlington-to-json.py -t ..\tsv\latest\ -s ..\yaml\arlington.yaml --yaml -c > arl-combined-yaml.log
```

Note that due to Arlington wildcards (`*`) and repeating indices (e.g. `1*`), YAML needs explicit quoting.

For efficiency in using [Liquid expressions](https://shopify.github.io/liquid/), there is also a slightly different structure than the TSV files:

```json
{
  "object_name": "ArlingtonName",
  "object_type": "dictionary",  // or array, stream, map
  "object_keys": [
    "A"
  ],
  "object_rows": [
    {
      "Key": "A",
      "Type": "stream",
      "SinceVersion": "1.0",
      "DeprecatedIn": "",
      "Required": "FALSE",
      "IndirectReference": "TRUE",
      "Inheritable": "FALSE",
      "DefaultValue": "",
      "PossibleValues": "",
      "SpecialCase": "",
      "Link": "[XObjectFormType1,XObjectImage,fn:SinceVersion(1.1,XObjectFormPS),fn:SinceVersion(1.1,XObjectFormPSpassthrough)]",
      "Note": "Table xx"
    }
  ]
}
```

## Example block of AsciiDoc

Refer to <https://www.metanorma.org/author/topics/automation/data_to_text/> and <https://www.metanorma.org/blog/2025-04-22-data2text>/.

```asciidoc
[data2text,pdfobj=../yaml/ArrayOfQuadPoints.yaml]
----
The {{ pdfobj.object_type }} object name is **{{ pdfobj.object_name }}** which contains 
{% case pdfobj.object_type %}
{% when "dictionary" or "stream" %} the following {{ pdfobj.object_keys.size }} keys in alphabetical order:
{% assign sorted_keys = pdfobj.object_keys | sort %}
* {% for key in sorted_keys %}`{{- key -}}`, {% endfor %}
{% when "map" %}a map.
{% when "array" %}{% if pdfobj.object_keys.size > 1 %}{% assign maxidx = pdfobj.object_keys.size | minus: 1 %}{{ pdfobj.object_keys.size }} array elements (indexed from 0 to {{ maxidx }}){% else %} 1 array element (index 0){% endif %}.
{% endcase %}
----
```

To inspect a variable in Liquid, do the following which will render a JSON string:

```asciidoc
`{{ some_var | json }}`
```

To compile with Metanorma:

```sh
metanorma site generate metanorma.yml > mn.log 2>&1
```
