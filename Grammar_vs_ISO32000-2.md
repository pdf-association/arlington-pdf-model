# Grammar vs. ISO 32000-2
This file shows small diffrences between csv files and ISO 32000-2.  
To record changes and keep them readable, the following formula is used:
- name
  - key -> value (ISO 32000-2)  


        name - name of a csv file
        key - key where the diffrence has been made
        value - value used in Grammar
        (ISO 32000-2) - original value used in ISO 32000-2 enclosed in Parentheses
___

- Catalog
	- Lang -> string (text string)
- PageObject
	- ID -> string (byte string)
- Annot
	- Contents -> string (text string)
	- NM -> string (text string)
	- M -> date or string (text string)
	- Lang -> string (text string)
