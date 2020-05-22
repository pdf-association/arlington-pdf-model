# -*- coding: utf-8 -*-
"""
// Copyright 2020 PDF Association, Inc. https://www.pdfa.org
//
// This material is based upon work supported by the Defense Advanced
// Research Projects Agency (DARPA) under Contract No. HR001119C0079.
// Any opinions, findings and conclusions or recommendations expressed
// in this material are those of the author(s) and do not necessarily
// reflect the views of the Defense Advanced Research Projects Agency
// (DARPA). Approved for public release.
//
// SPDX-License-Identifier: Apache-2.0
//
// Generates both a 3D visualization JSON file and a separate JSON database 
// for the PDF DOM using TSV data.
//
// See https://github.com/vasturiano/3d-force-graph-vr/
//
// Author: Peter Wyatt
"""
import json
import csv
import os
import glob
import re

# Read all the TSV files in ../tsv/ sub-directory
pdfdom = []
for filepath in glob.iglob(r'../tsv/*.tsv'):
    obj_name = os.path.splitext(os.path.basename(filepath))[0]
    print('\rReading %s                 ' % obj_name, end ='')
    with open(filepath, newline='') as csvfile:
        tsvreader = csv.DictReader(csvfile, delimiter='\t')
        tsvobj = {}
        for row in tsvreader:
            # 'Key' is the first column of the OrderedDict so pull it out then delete it
            keyname = row['Key']
            row.popitem(False)
            o = dict(row)           # Convert away from an ordered dict
            # Iterate all columns converting to Pythonese  
            # "FALSE" to False, "TRUE" to True, all "" columns get deleted
            for field, val in list(o.items()):
                if (val == "FALSE"):
                    o[field] = False
                elif (val == "TRUE"):
                    o[field] = True
                if (val == ""):
                    del o[field]
            tsvobj[keyname] = o
    pdfobj = {}
    pdfobj['id']   = obj_name    # use TSV filename as pseudo-object name
    pdfobj['keys'] = tsvobj      # index (numeric values) if array
    pdfdom.append(pdfobj)
        
# Write out a rather large single JSON of the full PDF DOM 
with open('pdf-dom.json', 'w') as domfile:
    json.dump(pdfdom, domfile, indent=2)
print("\npdf-dom.json created.")

# Make the 3D/VR data file
#   Nodes = PDF objects. Size is proportional to number of keys/indices. 
#           Classification grouping (dict, array, stream, map) is inferred. 
#           Trailer and Catalog are red (as visual anchor points)
#   Links = Keys which are array, dict or streams. Description is key name. Required keys are red.
#           Keys which are basic types (names, integers, numbers, strings, etc) are NOT visualized.
nodes = []
for obj in pdfdom:
    print('\rProcessing node for %s                    ' % obj_name, end ='')
    n = {}          
    n['id']  = obj['id']                                  # Use pseudo-name as makes linking easier
    n['val'] = len(obj['keys']) * len(obj['keys'])        # Size is square of # of keys/indices
    k = obj['keys']
    if ('DecodeParams' in k.keys()):                      # Arbitrary key to test for streams
        n['group'] = "stream"
    elif ('Array' in obj['id']) or ('0' in k.keys()):     # Arrays in filename or use numbers as keys
        n['group'] = "array"
    elif ('*' in k.keys()):                               # Maps use '*' as a key name
        n['group'] = "map"                                
    else:                                                 # Otherwise a dictionary 
        n['group'] = "dict"                               
    if ('FileTrailer' == obj['id']) or ('Catalog' == obj['id']):
        n['color'] = "red"
    n['name']  = obj['id'] + ' ' + n['group']             # Append group to name for nice node name
    nodes.append(n)    
print()    
    
links = []
for obj in pdfdom:
    for pdfkey in obj['keys']:
        if ('Link' in obj['keys'][pdfkey]): 
            print('\rProcessing links for %s key %s                              ' % (obj['id'], pdfkey), end='')
            pdflinks = re.split('\;|\,|\]|\[', obj['keys'][pdfkey]['Link']) 
            for l in pdflinks:
                if (len(l) > 0): 
                    lnk = {}          
                    lnk['source'] = obj['id']   
                    lnk['target'] = l   
                    lnk['name']   = obj['id'] + '::' + pdfkey   # Name is object and key name
                    if (obj['keys'][pdfkey]['Required']):       # Required keys have red links
                        lnk['color'] = "red"
                    links.append(lnk)

outdata = {}
outdata["nodes"] = nodes
outdata["links"] = links
with open("3d-pdfdom.json", 'w') as vrfile:
    json.dump(outdata, vrfile, indent=4)
    