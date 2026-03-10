#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright 2026 PDF Association, Inc. https://www.pdfa.org
#
# SPDX-License-Identifier: Apache-2.0
# Contributors: Peter Wyatt, PDF Association
#
# A simple Python script to convert a set of Arlington TSV files to
# JSON or YAML so that Metanorma may utilize the data.
#
# In AsciiDoc Metanorma documents:
#
# Refer to https://www.metanorma.org/author/topics/automation/data_to_text/
# and https://www.metanorma.org/blog/2025-04-22-data2text/.
#

import sys
import csv
import os
import glob
import argparse
import json
import yaml


def ArlingtonObjectType(obj_name: str) -> str:
    # Explicitly determine object type from filename only

    ## TODO: remove once https://github.com/pdf-association/arlington-pdf-model/issues/164 is resolved
    issue164 = ['FunctionType0',
                'FunctionType4',
                'HalftoneType10',
                'HalftoneType16',
                'HalftoneType6',
                'Metadata',
                'PatternType1',
                'ShadingType4',
                'ShadingType5',
                'ShadingType6',
                'ShadingType7',
                'SoundObject',
                'Thumbnail' ]
    obj_type = 'dictionary'
    if obj_name.startswith('ArrayOf') or obj_name.endswith('Array') or obj_name.endswith('ColorSpace'):
        obj_type = 'array'
    elif obj_name.endswith('Map'):
        obj_type = 'map'
    elif obj_name.endswith('Stream') or obj_name.startswith('XObject') or obj_name.startswith('FontFile') or obj_name in issue164:
        obj_type = 'stream'
    return obj_type


def ArlingtonToCombined(dir: str, combined_file: str, as_yaml: bool):
    # Output a single mega JSON/YAML file containing a full Arlington file set
    fcount = 0
    mega = []
    fmt = "YAML" if as_yaml else "JSON"

    print(f"Combining Arlington file set from '{dir}' to single {fmt} file '{combined_file}'")
    
    for filepath in glob.iglob(os.path.join(dir, r"*.tsv")):
        obj_name = os.path.splitext(os.path.basename(filepath))[0]
        fcount += 1
        arl = {}
        keys = []
        arl['object_name'] = str(obj_name)
        arl['object_type'] = ArlingtonObjectType(obj_name)
        arl['object_keys'] = []
        print(f"{obj_name} is {arl['object_type']}")
        with open(filepath, newline='') as csvfile:
            tsvreader = csv.DictReader(csvfile, delimiter='\t')
            for row in tsvreader:
                keyname = row['Key']
                keys.append(keyname)
                row.pop('Key', None)
                arl[keyname] = row
        csvfile.close()
        arl['object_keys'] = keys
        mega.append(arl)

    if (fcount == 0):
        print(f"There were no TSV files in directory '{dir}'")
        return

    with open(combined_file, 'w') as f:
        if not as_yaml:
            json.dump(mega, f, indent=2)
        else:
            yaml.dump(mega, f)

    print(f"TSV files processed: {fcount}")


def ArlingtonToFileSet(dir: str, json_folder: str, as_yaml: bool):
    # Output a single JSON/YAML file for each TSV file in an Arlington file set
    fcount = 0
    fmt = "YAML" if yaml else "JSON"

    print(f"Mirroring Alrington TSV file set from '{cli.tsvdir}' to folder '{json_folder}' as {fmt}")

    for filepath in glob.iglob(os.path.join(dir, r"*.tsv")):
        obj_name = os.path.splitext(os.path.basename(filepath))[0]
        fcount += 1
        arl = {}
        keys = []
        arl['object_name'] = str(obj_name)
        arl['object_type'] = ArlingtonObjectType(obj_name)
        arl['object_keys'] = []
        print(f"{obj_name} is {arl['object_type']}")
        with open(filepath, newline='') as csvfile:
            tsvreader = csv.DictReader(csvfile, delimiter='\t')
            for row in tsvreader:
                keyname = row['Key']
                keys.append(keyname)
                row.pop('Key', None)
                arl[keyname] = row
        csvfile.close()
        arl['object_keys'] = keys
        with open(os.path.join(cli.json_out, obj_name + ".json"), 'w') as f:
            if not as_yaml:
                json.dump(arl, f, indent=2)
            else:
                yaml.dump(arl, f)

    if (fcount == 0):
        print(f"There were no TSV files in directory '{dir}'")
        return

    print(f"TSV files processed: {fcount}")


if __name__ == '__main__':
    cli_parser = argparse.ArgumentParser()
    cli_parser.add_argument('-y', '--yaml', dest="as_yaml", action='store_true',
                            help='Use YAML, not JSON ouput. Default: JSON')
    cli_parser.add_argument('-t', '--tsvdir', dest="tsvdir",
                            help='folder containing Arlington TSV file set')
    cli_parser.add_argument('-c', '--combine', dest="combine", action='store_true',
                            help='Combine full Arlington TSV file set into a single JSON ouput. Default: not specified.')
    cli_parser.add_argument('-s', '--save', dest="json_out",
                            help='Folder for JSON output files, or a filename if --combine is used')
    cli = cli_parser.parse_args()

    if (cli.tsvdir is None) or not os.path.isdir(cli.tsvdir):
        print(f"{cli.tsvdir}' is not a valid directory")
        cli_parser.print_help()
        sys.exit()

    if (cli.json_out is None):
        print(f"A folder name for JSON/YAML output files, or a filename if --combine is used is required with -s/--save.")
        cli_parser.print_help()
        sys.exit()

    print("Loading from", cli.tsvdir)
    if (not cli.combine):
        try:
            # Make new folder for the Arlington file set in JSON/YAML format
            os.makedirs(cli.json_out, exist_ok=True)
            arl = ArlingtonToFileSet(cli.tsvdir, os.path.normpath(cli.json_out), cli.as_yaml)
        except Exception as e:
            print(f"'{cli.json_out}' was not a valid folder name: {e}")
            cli_parser.print_help()
            sys.exit()
    else:
        extn = ".yaml" if cli.as_yaml else ".json"
        combined_file = os.path.splitext(os.path.normpath(os.fspath(cli.json_out)))[0] + extn
        arl = ArlingtonToCombined(cli.tsvdir, combined_file, cli.as_yaml)

    print("Done.")
