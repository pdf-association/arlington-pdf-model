#!/usr/bin/python3
# -*- coding: utf-8 -*-
# Copyright 2021 PDF Association, Inc. https://www.pdfa.org
#
# This material is based upon work supported by the Defense Advanced
# Research Projects Agency (DARPA) under Contract No. HR001119C0079.
# Any opinions, findings and conclusions or recommendations expressed
# in this material are those of the author(s) and do not necessarily
# reflect the views of the Defense Advanced Research Projects Agency
# (DARPA). Approved for public release.
#
# SPDX-License-Identifier: Apache-2.0
# Contributors: Peter Wyatt, PDF Association
#
# Converts the Arlington TSV data into Pythonesque equivalent.
# Can also export as JSON.
# The conversion process may generate errors for various kinds of data entry issues
# (i.e. mistakes in the TSV data) but there is also a ValidateDOM() method that will
# perform other checks.
#

import csv
import os
import glob
import re
import argparse
import pprint
import logging
import json
from sly import Lexer


class ArlingtonFnLexer(Lexer):
    #debugfile = 'parser.out'

    # Set of token names.   This is always required.
    tokens = {
        FUNC_NAME,    KEY_VALUE,    KEY_NAME,     KEY_PATH,
        MOD,          PDF_PATH,     EQ,           NE,
        GE,           LE,           GT,           LT,
        LOGICAL_AND,  LOGICAL_OR,   REAL,         INTEGER,
        PLUS,         MINUS,        TIMES,        DIVIDE,
        LPAREN,       RPAREN,       COMMA
    }

    # Precedence rules
    precedence = (
          ('nonassoc', EQ, NE, LE, GE, LT, GT),  # Non-associative operators
          ('left', PLUS, MINUS),
          ('left', TIMES, DIVIDE, MOD),
          ('left', LOGICAL_AND, LOGICAL_OR)
    )

    # String containing ignored characters between tokens (just SPACE)
    ignore       = ' '

    # Regular expression rules for tokens
    FUNC_NAME    = r'fn\:[A-Z][a-zA-Z0-9]+\('
    MOD          = r'mod'
    KEY_VALUE    = r'@(\*|[0-9]+|[0-9]+\*|[a-zA-Z0-9_\.\-]+)'
    # Key name of just '*' is ambiguous TIMES (multiply) operator.
    # Key name which is numeric array index (0-9*) is ambiguous with integers.
    # Array indices are integers, or integer followed by ASTERISK (wildcard) - need to use SPACEs to disambiguate
    KEY_PATH     = r'(parent::)?(([a-zA-Z]|[a-zA-Z][0-9]*|[0-9]*\*|[0-9]*[a-zA-Z])[a-zA-Z0-9_\.\-]*::)+'
    KEY_NAME     = r'([a-zA-Z]|[a-zA-Z][0-9]*|[0-9]*\*|[0-9]*[a-zA-Z])[a-zA-Z0-9_\.\-]*'
    PDF_PATH     = r'::'
    EQ           = r'=='
    NE           = r'!='
    GE           = r'>='
    LE           = r'<='
    LOGICAL_AND  = r'\&\&'
    LOGICAL_OR   = r'\|\|'
    GT           = r'>'
    LT           = r'<'
    REAL         = r'\-?\d+\.\d+'
    INTEGER      = r'\-?\d+'
    PLUS         = r'\+'
    MINUS        = r'-'
    TIMES        = r'\*'
    DIVIDE       = r'/'
    LPAREN       = r'\('
    RPAREN       = r'\)'
    COMMA        = r'\,'

    @_(r'\-?\d+\.\d+')
    def REAL(self, t):
        t.value = float(t.value)
        return t

    @_(r'\-?\d+')
    def INTEGER(self, t):
        t.value = int(t.value)
        return t

    @_(r'false')
    def PDF_FALSE(self, t):
        t.value = False
        return t

    @_(r'true')
    def PDF_TRUE(self, t):
        t.value = True
        return t


class Arlington:

    # All the Arlington pre-defined types (pre-sorted alphabetically)
    _known_types = [ 'array', 'bitmask',  'boolean',  'date',  'dictionary',  'integer',
                     'matrix',  'name',  'name-tree',  'null',  'number',  'number-tree',
                     'rectangle', 'stream',  'string',  'string-ascii',  'string-byte',  'string-text' ]

    # Types for which a valid Link is required
    _links_required = [ 'array', 'dictionary', 'name-tree', 'number-tree', 'stream' ]

    # Current set of versions for the SinceVersion and Deprecated columns, as well as some functions
    _pdf_versions = [ '1.0', '1.1', '1.2', '1.3', '1.4', '1.5', '1.6', '1.7', '2.0' ]

    # Only strip off outer "[...]" as inner square brackets may exist for PDF arrays
    def _StripSquareBackets(self, li):
        if (isinstance(li, str)):
            # Single string
            if ((li[0] == r'[') and (li[len(li)-1] == r']')):
                return li[1 : len(li)-1]
            else:
                return li
        elif (isinstance(li, list)):
            # Was SEMI-COLON separated, now a Python list
            l = []
            for i in li:
                if (i == r'[]'):
                    l.append(None)
                elif ((i[0] == r'[') and (i[len(i)-1] == r']')):
                    l.append(i[1 : len(i)-1])
                else:
                    l.append(i)
            return l
        else:
            raise TypeError("Unexpected type when removing square brackets")


    # Convert spreadsheet booleans to Python: "FALSE" to False, "TRUE" to True
    #   (lowercase "false"/"true" are retained as PDF keywords)
    # Note also that declarative functions may be used in place of Booleans!!
    def _ConvertBooleans(self, obj):
        if (isinstance(obj, str)):
            if ((obj == r'FALSE') or (obj == r'[FALSE]')):
                return False
            elif ((obj == r'TRUE') or (obj == r'[TRUE]')):
                return True
            else:
                return obj
        elif (isinstance(obj, list)):
            l = []
            for o in obj:
                if ((o == r'FALSE') or (o == r'[FALSE]')):
                    l.append(False)
                elif ((o == r'TRUE') or (o == r'[TRUE]')):
                    l.append(True)
                else:
                    l.append(o)
            return l
        else:
            raise TypeError("Unexpected type for converting booleans")


    # Process declarative functions in Link
    # Parses strings such as "FilterLZWDecode,fn:SinceVersion(1.2,FilterFlateDecode),FilterCCITTFaxDecode,fn:SinceVersion(1.4,FilterJBIG2Decode)"
    # into: [ 'FilterLZWDecode', 'fn:SinceVersion(1.2,FilterFlateDecode)', 'FilterCCITTFaxDecode', 'fn:SinceVersion(1.4,FilterJBIG2Decode)' ]
    def _ConvertLinks(self, links):
        m = re.findall("(fn:[a-zA-Z]*\(\d\.\d\,[^)]*\))|([a-zA-Z0-9_\-]*)", links)
        # Returns pairs: function with link in-situ, non-function links
        # Separating commas are parsed as ('', '') so need to be skipped over
        lnk = []
        for i,j in m:
            if (i != ''):
                lnk.append(i)
            elif (j != ''):
                lnk.append(j)
        return lnk


    # Use Sly to parse a declaractive function
    # Sly will raise exceptions if there are errors
    # Returns the series of tokens as a Python list
    def _CheckFunction(self, func):
        fn = []
        for tok in self.__lexer.tokenize(func):
            fn.append(tok)
        return fn


    # Constructor - takes a TSV directory as a parameter
    # Reads in TSV file-by-file and converts to Pythonesque
    def __init__(self, dir="."):
        self.__directory = dir
        self.__filecount = 0
        self.__pdfdom = {}
        self.__lexer  = ArlingtonFnLexer()

        # Load Arlington into Python
        for filepath in glob.iglob(os.path.join(dir, r"*.tsv")):
            obj_name = os.path.splitext(os.path.basename(filepath))[0]
            self.__filecount = self.__filecount + 1
            logging.debug('Reading %s' % obj_name)
            with open(filepath, newline='') as csvfile:
                tsvreader = csv.DictReader(csvfile, delimiter='\t')
                tsvobj = {}
                for row in tsvreader:
                    keyname = row['Key']
                    if (len(row) != 12):
                        logging.error("%s::%s does not have 12 columns!" % (obj_name, keyname))
                    row.pop('Key')
                    if (keyname == ''):
                        raise TypeError("Key name field cannot be empty")

                    # Make multi-type fields into arrays (aka Python lists) and empty fields into None
                    if (r';' in row['Type']):
                        i = re.split(r';', row['Type'])
                        row['Type'] = i
                    else:
                        row['Type'] = [ row['Type'] ]

                    # Must be FALSE or TRUE
                    row['Required'] = self._ConvertBooleans(row['Required'])

                    # Optional, but must be a known PDF version
                    if (row['DeprecatedIn'] == ''):
                        row['DeprecatedIn'] = None

                    if (r';' in row['IndirectReference']):
                        row['IndirectReference'] = self._ConvertBooleans(self._StripSquareBackets(re.split(r';', row['IndirectReference'])))
                    else:
                        row['IndirectReference'] = self._ConvertBooleans(row['IndirectReference'])

                    # Must be FALSE or TRUE
                    row['Inheritable'] = self._ConvertBooleans(row['Inheritable'])

                    # Can only be one value for Key, but Key can be multi-typed
                    if (row['DefaultValue'] == ''):
                        row['DefaultValue'] = None
                    elif (r';' in row['DefaultValue']):
                        row['DefaultValue'] = self._StripSquareBackets(re.split(r';', row['DefaultValue']))
                    else:
                        if ('array' in row['Type']):
                            # PDF array values stored as Python strings that start with "[" and end with "]"
                            row['DefaultValue'] = [ row['DefaultValue'] ]
                        else:
                            row['DefaultValue'] = [ self._StripSquareBackets(row['DefaultValue']) ]

                    if (row['DefaultValue'] != None):
                        dv = False
                        for i, t in enumerate(row['Type']):
                            if (isinstance(row['DefaultValue'][i], str) and (r'fn:' not in row['DefaultValue'][i])):
                                if ((t.find(r'integer') != -1) or (t.find(r'bitmask') != -1)):
                                    if (dv):
                                        logging.error("DefaultValue already processed for %s::%s!" % (obj_name, keyname))
                                    row['DefaultValue'][i] = int(row['DefaultValue'][i], base=10)
                                    dv = True
                                elif (t.find(r'number') != -1):
                                    if (dv):
                                        logging.error("DefaultValue already processed for %s::%s!" % (obj_name, keyname))
                                    row['DefaultValue'][i] = float(row['DefaultValue'][i])
                                    dv = True
                                elif (t.find(r'boolean') != -1):
                                    if ('true' == row['DefaultValue'][i]):
                                        if (dv):
                                            logging.error("DefaultValue already processed for %s::%s!" % (obj_name, keyname))
                                        row['DefaultValue'][i] = True
                                        dv = True
                                    elif ('false' == row['DefaultValue'][i]):
                                        if (dv):
                                            logging.error("DefaultValue already processed for %s::%s!" % (obj_name, keyname))
                                        row['DefaultValue'][i] = False
                                        dv = True
                                    else:
                                        logging.error("Boolean DefaultValue '%s' not recognized for %s::%s!" % (row['DefaultValue'][i], obj_name, keyname))

                    if (row['PossibleValues'] == ''):
                        row['PossibleValues'] = None
                    else:
                        if (r';' in row['PossibleValues']):
                            row['PossibleValues'] = self._StripSquareBackets(re.split(r';', row['PossibleValues']))
                        else:
                            row['PossibleValues'] = [ self._StripSquareBackets(row['PossibleValues']) ]

                        # Break COMMA-separated possible values into a list (avoiding functions for now!)
                        for i, pv in enumerate(row['PossibleValues']):
                            if (isinstance(pv, str) and (r',' in pv) and (r"fn:" not in pv)):
                                row['PossibleValues'][i] = re.split(r',', pv)

                    if (row['PossibleValues'] != None):
                        for i, t in enumerate(row['Type']):
                            if (isinstance(row['PossibleValues'][i], str) and (r"fn:" not in row['PossibleValues'][i])):
                                if ((t.find('integer') != -1) or (t.find('bitmask') != -1)):
                                    row['PossibleValues'][i] = int(row['PossibleValues'][i], base=10)
                                elif (t.find('number') != -1):
                                    row['PossibleValues'][i] = float(row['PossibleValues'][i])
                                elif (t.find(r'boolean') != -1):
                                    if ('true' == row['PossibleValues'][i]):
                                        row['PossibleValues'][i] = True
                                    elif ('false' == row['PossibleValues'][i]):
                                        row['PossibleValues'][i] = False
                                    else:
                                        logging.error("Boolean PossibleValues '%s' not recognized for %s::%s!" % (row['PossibleValues'][i], obj_name, keyname))
                            elif (isinstance(row['PossibleValues'][i], list)):
                                for j, pv in enumerate(row['PossibleValues'][i]):
                                    if (r"fn:" not in pv):
                                        if ((t.find('integer') != -1) or (t.find('bitmask') != -1)):
                                            row['PossibleValues'][i][j] = int(pv, base=10)
                                        elif (t.find('number') != -1):
                                            row['PossibleValues'][i][j] = float(pv)
                                        elif (t.find(r'boolean') != -1):
                                            if ('true' == pv):
                                                row['PossibleValues'][i][j] = True
                                            elif ('false' == pv):
                                                row['PossibleValues'][i][j] = False
                                            else:
                                                logging.error("Boolean PossibleValues '%s' not recognized for %s::%s!" % (pv, obj_name, keyname))

                    if (row['SpecialCase'] == ''):
                        row['SpecialCase'] = None
                    elif (r';' in row['SpecialCase']):
                        row['SpecialCase'] = self._StripSquareBackets(re.split(r';', row['SpecialCase']))
                    else:
                        row['SpecialCase'] = [ self._StripSquareBackets(row['SpecialCase']) ]

                    if (row['Link'] == ''):
                        row['Link'] = None
                    else:
                        if (r';' in row['Link']):
                            row['Link'] = self._StripSquareBackets(re.split(r';', row['Link']))
                        else:
                            row['Link'] = [ self._StripSquareBackets(row['Link']) ]

                        for i, lnk in enumerate(row['Link']):
                            if (isinstance(lnk, str)):
                                if not (r"fn:" in lnk):
                                    row['Link'][i] = re.split(r'\,', lnk)
                                else:
                                    row['Link'][i] = self._ConvertLinks(lnk)
                            elif (isinstance(lnk, list)):
                                for j, l in enumerate(lnk):
                                    if (isinstance(l, str)):
                                        if not (r"fn:" in l):
                                            row['Link'][i][j] = re.split(r'\,', l)
                                        else:
                                            row['Link'][i][j] = self._ConvertLinks(l)

                    if (row['Note'] == ''):
                        row['Note'] = None

                    tsvobj[keyname] = row
                self.__pdfdom[obj_name] = tsvobj

        if (self.__filecount == 0):
            logging.error("There were no TSV files in directory '%s'!" % self.__directory)
            return

        logging.info('%d TSV files processed from %s' % (self.__filecount, self.__directory))


    # Does a detailed Validation of the in-memory Python data structure
    def ValidateDOM(self):
        if ((self.__filecount == 0) or (len(self.__pdfdom) == 0)):
            logging.error("There is no Arlington DOM to validate!")
            return

        for obj_name in self.__pdfdom:
            obj = self.__pdfdom[obj_name]

            # Check if object contain any duplicate keys or has no keys
            if (len(obj) != len(set(obj))):
                logging.error("Duplicate keys in %s!" % obj_name)
            if (len(obj) == 0):
                logging.error("Object %s has no keys/array entries!" % obj_name)

            for keyname in obj:
                row = obj[keyname]
                logging.debug("Validating %s::%s" % (obj_name, keyname))

                # Check validity of key names and array indices
                m = re.search(r'^[a-zA-Z0-9_\-\.]*\*?$', keyname)
                if (m == None):
                    logging.error("Key '%s' in object %s has unexpected characters" % (keyname, obj_name))

                if (row['SinceVersion'] not in self._pdf_versions):
                    logging.error("SinceVersion '%s' in %s::%s has unexpected value!" % (row['SinceVersion'], obj_name, keyname))

                if (row['DeprecatedIn'] != None) and (row['DeprecatedIn'] not in self._pdf_versions):
                    logging.error("DeprecatedIn '%s' in %s::%s has unexpected value!" % (row['DeprecatedIn'], obj_name, keyname))

                if not ((row['Required'] == True) or (row['Required'] == False) or row['Required'].startswith("fn:IsRequired(")):
                    logging.error("Required %s '%s' in %s::%s is not FALSE, TRUE or fn:IsRequired!" % (type(row['Required']), row['Required'], obj_name, keyname))

                if  (isinstance(row['Required'], str) and (r'fn:' in row['Required'])):
                    fn = self._CheckFunction(row['Required'])

                if (r'*' in keyname) and (row['Required'] != False):
                    logging.error("Required needs to be FALSE for wildcard key '%s' in %s!" % (keyname, obj_name))

                if (isinstance(row['IndirectReference'], list)):
                    if (len(row['Type']) != len(row['IndirectReference'])):
                        logging.error("Incorrect number of elements between Type (%d) and IndirectReference (%d) for %s::%s" % (len(row['Type']), len(row['IndirectReference']), obj_name, keyname))
                    for i, t in enumerate(row['Type']):
                        if ('stream' in t):
                            if (row['IndirectReference'][i] != True):
                                logging.error("Type 'stream' requires IndirectReference to be TRUE for %s::%s" % (obj_name, keyname))
                else:
                    if ('stream' in row['Type']):
                        if (row['IndirectReference'] != True):
                            logging.error("Type 'stream' requires IndirectReference to be TRUE for %s::%s" % (obj_name, keyname))
                    if not ((row['IndirectReference'] == True) or (row['IndirectReference'] == False) or (r"fn:MustBeDirect(" in row['IndirectReference'])):
                        logging.error("IndirectReference %s '%s' in %s::%s is not FALSE, TRUE or fn:MustBeDirect!" % (type(row['IndirectReference']), row['IndirectReference'], obj_name, keyname))

                if  (isinstance(row['IndirectReference'], str) and (r'fn:' in row['IndirectReference'])):
                    fn = self._CheckFunction(row['IndirectReference'])

                if not ((row['Inheritable'] == True) or (row['Inheritable'] == False)):
                    logging.error("Inheritable %s '%s' in %s::%s is not FALSE or TRUE!" % (type(row['Inheritable']), row['Inheritable'], obj_name, keyname))


                # Validate all types are known and match DefaultValue into PossibleValues
                found_dv = False
                dv = None
                for t in row['Type']:
                    if (r"fn:" in t):
                        # Only "fn:SinceVersion(" or "fn:Deprecated(" allowed
                        if not (t.startswith(r"fn:SinceVersion(") or t.startswith(r"fn:Deprecated(")):
                            logging.error("Unknown function '%s' for %s::%s!" % (t, obj_name, keyname))
                        fn = self._CheckFunction(t)
                        found = False
                        for i in self._known_types:
                            if (i in t):
                                found = True
                        if not found:
                            logging.error("No known Arlington type in function '%s' for %s::%s!" % (t, obj_name, keyname))
                    elif (t not in self._known_types):
                        logging.error("Unknown type '%s' for %s::%s!" % (t, obj_name, keyname))

                    # Check specific syntaxes for array and strings of default values
                    if (row['DefaultValue'] != None):
                        if (len(row['Type']) != len(row['DefaultValue'])):
                            logging.error("Incorrect number of elements between Type and DefaultValue for %s::%s" % (obj_name, keyname))

                        for i, t in enumerate(row['Type']):
                            if (row['DefaultValue'][i] != None):
                                if isinstance(dv, str):
                                    if (dv.find(r'fn:') == -1):
                                        if ("FALSE" in dv) or ("TRUE" in dv):
                                            logging.error("DefaultValue '%s' has incorrect FALSE/TRUE for PDF boolean for %s::%s" % (dv, obj_name, keyname))

                                        dv = row['DefaultValue'][i]
                                        if (t.find(r'array') != -1):
                                            if (dv[0] != r'['):
                                                logging.error("Default array '%s' does not start with '[' for %s::%s" % (dv, obj_name, keyname))
                                            if (dv[len(dv)-1] != r']'):
                                                logging.error("Default array '%s' does not end with ']' for %s::%s" % (dv, obj_name, keyname))
                                        elif (t.find(r'string') != -1):
                                            if (dv[0] != r'('):
                                                logging.error("Default string '%s' does not start with '(' for %s::%s" % (dv, obj_name, keyname))
                                            if (dv[len(dv)-1] != r')'):
                                                logging.error("Default string '%s' does not end with ')' for %s::%s" % (dv, obj_name, keyname))
                                    else:
                                        fn = self._CheckFunction(dv)
                                elif (isinstance(dv, (bool, int, float))):
                                   dv = row['DefaultValue'][i]

                    # Check specific syntaxes for array and strings of possible values
                    if (row['PossibleValues'] != None):
                        if (len(row['Type']) != len(row['PossibleValues'])):
                            logging.error("Incorrect number of elements between Type and PossibleValues for %s::%s" % (obj_name, keyname))

                        for i, t in enumerate(row['Type']):
                            if isinstance(row['PossibleValues'][i], str):
                                if (row['PossibleValues'][i].find(r'fn:') == -1):
                                    if (dv != None) and (not found_dv) and (row['PossibleValues'][i].find(str(dv)) != -1):
                                        found_dv = True

                                    if ("FALSE" in row['PossibleValues'][i]) or ("TRUE" in row['PossibleValues'][i]):
                                        logging.error("PossibleValues '%s' has incorrect FALSE/TRUE for PDF boolean for %s::%s" % (row['PossibleValues'][i], obj_name, keyname))

                                    # allow for functions which wrap a pre-defined type: e.g. fn:Deprecated(x.y,array)
                                    if (t.find(r'array') != -1):
                                        if (row['PossibleValues'][i][0] != r'['):
                                            logging.error("PossibleValues array '%s' does not start with '[' for %s::%s" % (row['PossibleValues'][i], obj_name, keyname))
                                        if (row['PossibleValues'][i][len(row['PossibleValues'][i]) - 1] != r']'):
                                            logging.error("PossibleValues array '%s' does not end with ']' for %s::%s" % (row['PossibleValues'][i], obj_name, keyname))
                                    elif (t.find(r'string') != -1):
                                        if (row['PossibleValues'][i][0] != r'('):
                                            logging.error("PossibleValues string '%s' does not start with '(' for %s::%s" % (row['PossibleValues'][i], obj_name, keyname))
                                        if (row['PossibleValues'][i][len(row['PossibleValues'][i]) - 1] != r')'):
                                            logging.error("PossibleValues string '%s' does not end with ')' for %s::%s" % (row['PossibleValues'][i], obj_name, keyname))
                                else:
                                    fn = self._CheckFunction(row['PossibleValues'][i])
                            elif (isinstance(row['PossibleValues'][i], list)):
                                for j, pv in enumerate(row['PossibleValues'][i]):
                                    if (isinstance(pv, str) and (pv.find(r'fn:') == -1)):
                                        if (dv != None) and (not found_dv) and (pv.find(str(dv)) != -1):
                                            found_dv = True

                                        if ("FALSE" in pv) or ("TRUE" in pv):
                                            logging.error("PossibleValues '%s' has incorrect FALSE/TRUE for PDF boolean for %s::%s" % (pv, obj_name, keyname))

                                        if (t.find(r'array') != -1):
                                            if (pv[0] != r'['):
                                                logging.error("PossibleValues array '%s' does not start with '[' for %s::%s" % (pv, obj_name, keyname))
                                            if (pv[len(pv)-1] != r']'):
                                                logging.error("PossibleValues array '%s' does not end with ']' for %s::%s" % (pv, obj_name, keyname))
                                        elif (t.find(r'string') != -1):
                                            if (pv[0] != r'('):
                                                logging.error("PossibleValues string '%s' does not start with '(' for %s::%s" % (pv, obj_name, keyname))
                                            if (pv[len(pv)-1] != r')'):
                                                logging.error("PossibleValues string '%s' does not end with ')' for %s::%s" % (pv, obj_name, keyname))
                                    elif (isinstance(pv, (bool,int,float))):
                                        if (dv != None) and (not found_dv) and (str(pv).find(str(dv)) != -1):
                                            found_dv = True
                                    elif (isinstance(pv, list)):
                                        if (dv != None) and (not found_dv) and (dv in pv):
                                            found_dv = True


                    # Check links
                    if (row['Link'] != None):
                        if (len(row['Type']) != len(row['Link'])):
                            logging.error("Incorrect number of elements between Type and Links for %s::%s" % (obj_name, keyname))

                    for i, t in enumerate(row['Type']):
                        if (t in self._links_required):
                            # Links are required!
                            if (row['Link'][i] == None):
                                logging.error("Link is missing for type %s in %s::%s" % (t, obj_name, keyname))
                            else:
                                if (r"fn:" in row['Link'][i]):
                                    fn = self._CheckFunction(row['Link'][i])
                                    if not ((r"fn:SinceVersion(" in row['Link'][i]) or (r"fn:BeforeVersion(" in row['Link'][i]) or (r"fn:IsPDFVersion(" in row['Link'][i])):
                                        logging.error("Bad function '%s' for type %s in %s::%s" % (row['Link'][i], t, obj_name, keyname))
                                else:
                                    for l in row['Link'][i]:
                                        if (r"fn:" in l):
                                            fn = self._CheckFunction(l)
                                        elif (r"fn:" not in l):
                                            lnk = self.__pdfdom[l]
                                            if (lnk == None):
                                                logging.error("Broken link to '%s' for type %s in %s::%s" % (l, t, obj_name, keyname))
                                        elif not ((r"fn:SinceVersion(" in l) or (r"fn:BeforeVersion(" in l) or (r"fn:IsPDFVersion(" in l)):
                                            logging.error("Bad function '%s' for type %s in %s::%s" % (l, t, obj_name, keyname))
                        else:
                            # Links are NOT wanted - should be None
                            if (row['Link'] != None) and (row['Link'][i] != None):
                                logging.error("Link '%s' is present for type %s in %s::%s" % (row['Link'][i], t, obj_name, keyname))

                # Check if DefaultValue was in any PossibleValues
                if (row['PossibleValues'] != None) and (dv != None) and (not found_dv):
                    logging.error("DefaultValue (%s %s) not in PossibleValues (%s) for %s::%s" % (type(dv), dv, row['PossibleValues'], obj_name, keyname))


    # Pretty-print the DOM as JSON to a  file
    # Pretty-print helps readability for debugging
    def SaveDOMtoJSON(self, json_file):
        with open(json_file, r'w') as f:
            pprint.pprint(self.__pdfdom, f, compact=True, width=200)


if __name__ == '__main__':
    cli_parser = argparse.ArgumentParser()
    cli_parser.add_argument('-t', '--tsvdir', help='directory containing Alrington TSV file set', dest="tsvdir")
    cli_parser.add_argument('-j', '--json',   help="save Arlington DOM as JSON file", default=None, dest="json")
    cli_parser.add_argument('-v', '--validate', help="Detailed validation on the Arlington DOM", action='store_true', default=False, dest="validate")
    cli_parser.add_argument('-d', '--debug',  help="Enable debug logging", action="store_const", dest="loglevel", const=logging.DEBUG, default=logging.WARNING)
    cli_parser.add_argument('-i', '--info',   help="Enable informative logging", action="store_const", dest="loglevel", const=logging.INFO)
    cli = cli_parser.parse_args()

    logging.basicConfig(level=cli.loglevel)

    if ((cli.tsvdir == None) or not os.path.isdir(cli.tsvdir)):
        print("'%s' is not a directory" % cli.tsvdir)
        cli_parser.print_help()
        quit()

    print("Loading...")
    arl = Arlington(cli.tsvdir)

    if (cli.validate):
        print("Validating...")
        arl.ValidateDOM()

    if (cli.json is not None):
        print("Saving JSON to '%s'..." % cli.json)
        arl.SaveDOMtoJSON(cli.json)

    print("Done.")
