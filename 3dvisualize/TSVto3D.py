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
// Generates both 3D visualization JSON files and separate JSON database 
// for the PDF DOM for a specific PDF version, using TSV data.
//
// Usage: TSVto3D [ <pdf-version> | latest ]
//   Default is "latest"
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
import sys, getopt

# Helper function: find object matching 
def FindPDFobject(arr, nm):
    for x in arr:
        if x["id"] == nm:
            return x
    return None
            

# Work out PDF version or latest based on CLI option
pdfver      = "latest"
tsvdir      = "../tsv/latest/*.tsv"
jsonpdffile = "pdf-latest-dom.json"
json3dfile  = "3d-pdf-latest-dom.json"

if (len(sys.argv) > 1):
    pdfver      = sys.argv[1]
    tsvdir      = "../tsv/"+pdfver+"/*.tsv"
    jsonpdffile = "pdf-"+pdfver+"-dom.json"
    json3dfile  = "3d-pdf-"+pdfver+"-dom.json"

print("Processing PDF %s...\n" % pdfver)
    
# Read all the TSV files in ../tsv/<pdfver>/ sub-directory
pdfdom = []
for filepath in glob.iglob(tsvdir):
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
            # Iterate all columns converting to Pythonesque  
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
    
# BUG WORKAROUND: there is an issue where some Links may no longer be valid for a specific PDF version.
#          Iterate through all PDF DOM objects to check Links and delete any invalid Links.
print("\n")
for obj in pdfdom:
    print("\rProcessing object %s" % obj['id'], end ='')
    for pdfkey in obj['keys']:
        # print("\tProcessing key %s" % pdfkey)
        if ('Link' in obj['keys'][pdfkey]):
            newlnks = obj['keys'][pdfkey]['Link'];
            # print('\t\tProcessing Links %s' % newlnks)
            pdflinks = re.split('\;|\,|\]|\[', newlnks) 
            for ln in pdflinks:
                if (len(ln) > 0): 
                    # print("\t\tProcessing Link for '%s'" % ln)
                    x = FindPDFobject(pdfdom, ln)
                    if x == None:
                        print('\r\tDeleting %s::%s link to %s' % (obj['id'], pdfkey, ln))
                        # Avoid stub-matching!
                        # [ln] or [ln,...] or [...,ln,...] or [...,ln]
                        if not re.search(ln+',', newlnks):
                            if not re.search(','+ln, newlnks):
                                # Must be [ln] --> convert to []
                                newlnks = newlnks.replace(ln, '')
                            else:
                                # Must be [...,ln] --> convert to [...]
                                newlnks = newlnks.replace(','+ln, '')
                        else:
                            # Must be either [ln,...] or [...,ln,...]
                            newlnks = newlnks.replace(ln+',', '')
            obj['keys'][pdfkey]['Link'] = newlnks
        
# Write out a rather large single JSON of the full PDF DOM 
with open(jsonpdffile, 'w') as domfile:
    json.dump(pdfdom, domfile, indent=2)
print("\n%s created." % jsonpdffile)

# Make the 3D/VR data file
#   Nodes = PDF objects. Size is proportional to number of keys/indices. 
#           Classification grouping (dict, array, stream, map) is inferred. 
#           Trailer and Catalog are red (as visual anchor points)
#   Links = Keys which are array, dict or streams. Description is key name. Required keys are red.
#           Keys which are basic types (names, integers, numbers, strings, etc) are NOT visualized.
nodes = []
for obj in pdfdom:
    print('\rProcessing node for %s                    ' % obj['id'], end ='')
    n = {}          
    n['id']  = obj['id']                                  # Use pseudo-name as makes linking easier
    n['val'] = len(obj['keys']) * len(obj['keys'])        # Size == square of # of keys/indices of object
    k = obj['keys']
    if ('DecodeParams' in k.keys()):                      # Arbitrary key to test for streams
        n['group'] = "stream"
        n['desc']  = str(len(obj['keys'])) + ' keys'
    elif ('*' in k.keys()):                               # Maps use '*' as a key name
        n['group'] = "map"
        n['desc'] = '(unspecified)'
    elif ('Array' in obj['id']) or ('0' in k.keys()):     # Arrays in filename or use numbers as keys
        n['group'] = "array"
        n['desc']  = str(len(obj['keys'])) + ' entries'
    else:                                                 # Otherwise a dictionary 
        n['group'] = "dict"                               
        n['desc']  = str(len(obj['keys'])) + ' keys'
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
with open(json3dfile, 'w') as vrfile:
    json.dump(outdata, vrfile, indent=4)
print("\n%s created." % json3dfile)
