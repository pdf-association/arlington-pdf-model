# Grammar vs. ISO 32000-2
During the formalising the ISO3200-2 we noticed few misconceptions between standard and common expectations. 
This file shows small differences between ISO 32000-2 and our grammar. Some of them are straightforward and we did decisions that won't affect implementations.

#Changes made when converting ISO3200-2 to formalized grammar

accross the whole spec:
- changing type **text string** to **string**  (for example Catalog.Lang)
- changing type **byte string** to **string**  (for example  PageObject.ID)

#Open questions

- 12.3.4 Thumbnail images
refers to 8.9.8. Image dictionaries where Subtype is required but later states:  "(If a Subtype entry is specified, its value shall be Image.)"
Question: for Thumbnail, is it possible not to specify Subtype?
Suggestion: either doesn't require Subtype in Table 87, or explicitely say what is required for thumbnail. Many implementationa don't write Subtype for thumbnails
Resolution:  Subtype is not required

- 12.5.6.24 Projection annotations
Subtype is required for all annotations, we don't say anything about value of Subtype key in Projection annotation. The only place we mention it, is in Table 166 under AP key
Resolution:  Subtype key is "Projection"

- 13.7.2.3.3 RichMediaConfiguration dictionary
Name is of type "text tree"  - should be "name tree"
Resolution:  "text tree" changed to "name tree"

- 13.6.6 3D markup
Subtype in ExData dictionary is required (Markup3D)
in Example bellow obj 7 0 does have ExData with wrong Type and missing Subtype 


7 0 obj %Callout comment on CommentView1
<<
/Type /Annot
/Subtype /FreeText
/IT /FreeTextCallout
/ExData <<
/Type /Markup3D
/3DA 3 0 R
/3DV 4 0 R
>>


- Table 173: Additional entries in an annotation dictionary specific to external data
It is not explicitely said which annotation could have ExData only in 13.6.6 we say that ExData are present in 3D markup annotations that implies ExData is valid for all markup annotations?
Resolution: ExData is allowed for all annotations


- 13.7.2.3.5 View dictionary
"A View dictionary entry in a rich media annotation contains a superset of the information present in a 3D view (3DV) dictionary as specified in "Table 315: Entries in a 3D view
dictionary"
However "View" entry is in Rich media activation dictionary (Table 335)
Resolution: "View" is in Rich media activation dictionary (Table 335)

- Table 317 Entries in a 3D background dictionary
CS entry is name or array, but only valid value is DeviceRGB. what is array here for? what are valid entries in array? is [/DeviceRGB /DeviceRGB] valid?
Resoluion: array with any number of names is allowed