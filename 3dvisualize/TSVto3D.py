# -*- coding: utf-8 -*-
"""
# Copyright 2020-2021 PDF Association, Inc. https://www.pdfa.org
#
# This material is based upon work supported by the Defense Advanced
# Research Projects Agency (DARPA) under Contract No. HR001119C0079.
# Any opinions, findings and conclusions or recommendations expressed
# in this material are those of the author(s) and do not necessarily
# reflect the views of the Defense Advanced Research Projects Agency
# (DARPA). Approved for public release.
#
# SPDX-License-Identifier: Apache-2.0
#
# Generates both 3D visualization JSON files and separate JSON database
# for the PDF DOM for a specific PDF version, using TSV data.
#
# Usage: TSVto3D --tsvdir <tsv-dir> --outdir <outdir>
#
# See https://github.com/vasturiano/3d-force-graph-vr/
#
# Requires:
# - Python 3
#
# Python QA:
# - flake8 --ignore E501,E221,E226,E251 TSVto3D.py
# - pyflakes TSVto3D.py
# - mypy TSVto3D.py
#
# Author: Peter Wyatt
"""
import json
import csv
import os
import glob
import re
import argparse
# import pprint
import sys
from operator import itemgetter


def FindPDFobject(arr, nm: str):
    # Helper function: find object matching
    for x in arr:
        if x["id"] == nm:
            return x
    return None


def ArlingtonTo3D(tsvdir: str, outdir: str, pdfver: str):
    # Reads the TSV file set and creates 2 JSON outputs - one for 3D and one for VR
    jsonpdffile: str = os.path.join(outdir, "pdf-"+pdfver+"-dom.json")
    json3dfile: str  = os.path.join(outdir, "3d-pdf-"+pdfver+"-dom.json")
    tsvdir = os.path.join(os.path.abspath(tsvdir), "*.tsv")

    print("Processing Arlington TSV file set from %s for PDF version %s\n" % (tsvdir, pdfver))

    if (pdfver == "latest"):
        pdfver = "2.0"

    # Read all the TSV files into a large Python data structure
    pdfdom: list = []
    for filepath in glob.iglob(tsvdir):
        obj_name: str = os.path.splitext(os.path.basename(filepath))[0]
        print("\rReading %s                        " % obj_name, end='')
        with open(filepath, newline='') as csvfile:
            tsvreader = csv.DictReader(csvfile, delimiter='\t')
            tsvobj: dict = {}
            for row in tsvreader:
                # 'Key' is the first column of the OrderedDict so pull it out then delete it
                keyname: str = row['Key']
                row.popitem()
                if (float(row['SinceVersion']) <= float(pdfver)):
                    row['SinceVersion']: float = float(row['SinceVersion'])
                    if (len(row['DeprecatedIn']) > 0):
                        row['DeprecatedIn']: float = float(row['DeprecatedIn'])
                    if (row['Required'] in ["TRUE", "FALSE"]):
                        row['Required']: bool = (row['Required'] == "TRUE")
                    if (row['IndirectReference'] in ["TRUE", "FALSE"]):
                        row['IndirectReference']: bool = (row['IndirectReference'] == "TRUE")
                    if (row['Inheritable'] in ["TRUE", "FALSE"]):
                        row['Inheritable']: bool = (row['Inheritable'] == "TRUE")
                    # Iterate all columns converting to Pythonesque and compacting
                    # "FALSE" to False, "TRUE" to True, all "" columns get deleted
                    o: dict = dict(row)           # Convert away from an ordered dict
                    for field, val in list(o.items()):
                        if (val == ""):
                            del o[field]
                        elif type(o[field]) is str:
                            o[field] = re.sub("TRUE", "true", o[field])
                            o[field] = re.sub("FALSE", "false", o[field])
                    tsvobj[keyname] = o
        pdfobj: dict = {}
        pdfobj['id']   = obj_name    # use TSV filename as pseudo-object name
        pdfobj['keys'] = tsvobj      # index (numeric values) if array
        pdfdom.append(pdfobj)

    # print("\n\n", pprint.pformat(pdfdom))

    for obj in pdfdom:
        print("\rProcessing object %s                     " % obj['id'], end='')
        for pdfkey in obj['keys']:
            if ('Link' in obj['keys'][pdfkey]):
                newlnks: str = obj['keys'][pdfkey]['Link']
                # print('\t\tProcessing Links %s' % newlnks)

                # Need to support our declarative functions ("fn:SinceVersion(x.y,...)") in Links
                # This Linux command can confirm all functions in Links:
                #   cut -f 11 ../tsv/latest/*.tsv | grep -ho "fn:[a-zA-Z]*" | sort | uniq
                newlnks = re.sub(r"fn\:SinceVersion\(\d\.\d,", "", newlnks)
                newlnks = re.sub(r"\)", "", newlnks)
                pdflinks = re.split(r"\;|\,|\]|\[", newlnks)

                # print('\t\t\pdfLinks %s\n' % pdflinks)
                for ln in pdflinks:
                    if (len(ln) > 0):
                        # print("\t\tProcessing Link for '%s'" % ln)
                        x = FindPDFobject(pdfdom, ln)
                        if x is None:
                            print("\n\tDeleting %s::%s link to %s\n" % (obj['id'], pdfkey, ln))
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

    # Sort to minimize noise on git diff
    pdfdom = sorted(pdfdom, key=itemgetter('id'))

    # Write out a rather large single JSON of the full PDF DOM
    with open(jsonpdffile, 'w') as domfile:
        json.dump(pdfdom, domfile, indent=2)
    print("\r%s created.                      \n" % jsonpdffile)

    # Make the 3D/VR data file
    #   Nodes = PDF objects. Size is proportional to number of keys/indices.
    #           Classification grouping (dict, array, stream, map) is inferred.
    #           Trailer and Catalog are red (as visual anchor points)
    #   Links = Keys which are array, dict or streams. Description is key name. Required keys are red.
    #           Keys which are basic types (names, integers, numbers, strings, etc) are NOT visualized.
    nodes: list = []
    for obj in pdfdom:
        print("\rProcessing node for %s                      " % obj['id'], end='')
        n: dict = {}
        n['id']: str = obj['id']                              # Use pseudo-name as makes linking easier
        n['val']: int = len(obj['keys']) * len(obj['keys'])   # Size == square of # of keys/indices of object
        k: dict = obj['keys']
        if ('DecodeParams' in k.keys()):                      # Arbitrary key to test for streams
            n['group']: str = "stream"
            n['desc']: str  = str(len(obj['keys'])) + ' keys'
        elif ('*' in k.keys()):                               # Maps use '*' as a key name
            n['group']: str = "map"
            n['desc']: str = '(unspecified)'
        elif ('Array' in obj['id']) or ('0' in k.keys()) or ('0*' in k.keys()):  # Arrays in filename or use numbers as keys
            n['group']: str = "array"
            n['desc']: str  = str(len(obj['keys'])) + ' elements'
        else:                                                 # Otherwise a dictionary
            n['group']: str = "dict"
            n['desc']: str  = str(len(obj['keys'])) + ' keys'

        if (n['id'] in ['FileTrailer', 'Catalog', 'XRefStream']):
            n['color']: str = "red"

        n['name']: str = obj['id'] + ' ' + n['group']         # Append group to name for nicer node name
        nodes.append(n)

    links: list = []
    for obj in pdfdom:
        for pdfkey in obj['keys']:
            if ('Link' in obj['keys'][pdfkey]):
                print("\rProcessing links for %s key %s                              " % (obj['id'], pdfkey), end='')
                pdflinks = re.split(r"\;|\,|\]|\[", obj['keys'][pdfkey]['Link'])
                for ln in pdflinks:
                    if (len(ln) > 0):
                        lnk: dict = {}
                        lnk['source'] = obj['id']
                        lnk['target'] = ln
                        lnk['name']   = obj['id'] + '::' + pdfkey   # Name is object and key name
                        if (obj['keys'][pdfkey]['Required']):       # Required keys have red links
                            lnk['color'] = "red"
                        links.append(lnk)

    # Sort to minimize noise on git diff
    nodes = sorted(nodes, key=itemgetter('id'))
    links = sorted(links, key=itemgetter('name'))

    outdata = {}
    outdata["nodes"] = nodes
    outdata["links"] = links
    with open(json3dfile, 'w') as vrfile:
        json.dump(outdata, vrfile, indent=4)
    print("\r%s created.                              " % json3dfile)


if __name__ == '__main__':
    cli_parser = argparse.ArgumentParser()
    cli_parser.add_argument('-t', '--tsvdir', dest="tsvdir",
                            help='folder containing Arlington TSV file set')
    cli_parser.add_argument('-o', '--outdir', dest="outdir", default=".",
                            help='folder for output JSON files (default is current directory)')
    cli = cli_parser.parse_args()

    if (cli.tsvdir is None) or not os.path.isdir(cli.tsvdir):
        print("'%s' is not a valid TSV directory" % cli.tsvdir)
        cli_parser.print_help()
        sys.exit()

    if (cli.outdir is None) or not os.path.isdir(cli.outdir):
        print("'%s' is not a valid output directory" % cli.outdir)
        cli_parser.print_help()
        sys.exit()

    pdf_ver = os.path.basename(os.path.normpath(cli.tsvdir))
    if (pdf_ver not in ["1.0", "1.1", "1.2", "1.3", "1.4", "1.5", "1.6", "1.7", "2.0", "latest"]):
        print("'%s' was not recognized as a valid PDF version (1.0, 1.1, ..., 2.0, latest)" % pdf_ver)
        cli_parser.print_help()
        sys.exit()

    ArlingtonTo3D(cli.tsvdir, cli.outdir, pdf_ver)
    print("\nDone\n")
