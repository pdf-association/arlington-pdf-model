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
        LPAREN,       RPAREN,       COMMA,        ARRAY_START,
        ARRAY_END,    ELLIPSIS,     PDF_TRUE,     PDF_FALSE, PDF_STRING
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
    # Longer rules need to be at the top
    FUNC_NAME    = r'fn\:[A-Z][a-zA-Z0-9]+\('
    PDF_TRUE     = r'(true)|(TRUE)'
    PDF_FALSE    = r'(false)|(FALSE)'
    PDF_STRING   = r'\([a-zA-Z]+\)'
    MOD          = r'mod'
    ELLIPSIS     = r'\.\.\.'
    KEY_VALUE    = r'@(\*|[0-9]+|[0-9]+\*|[a-zA-Z0-9_\.\-]+)'
    # Key name is both a PDF name or a TSV filename
    # Key name of just '*' is ambiguous TIMES (multiply) operator.
    # Key name which is numeric array index (0-9*) is ambiguous with integers.
    # Array indices are integers, or integer followed by ASTERISK (wildcard) - need to use SPACEs to disambiguate
    KEY_PATH     = r'(parent::)?(([a-zA-Z]|[a-zA-Z][0-9]*|[0-9]*\*|[0-9]*[a-zA-Z])[a-zA-Z0-9_\.\-]*::)+'
    KEY_NAME     = r'([_a-zA-Z]|[_a-zA-Z][0-9]*|[0-9]*\*|[0-9]*[_a-zA-Z])[a-zA-Z0-9_\.\-]*'
    PDF_PATH     = r'::'
    ARRAY_START  = r'\['
    ARRAY_END    = r'\]'
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

    @_(r'(false)|(FALSE)')
    def PDF_FALSE(self, t):
        t.value = False
        return t

    @_(r'(true)|(TRUE)')
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
        if (li == None):
            return None
        elif (isinstance(li, str)):
            # Single string
            if ((li[0] == r'[') and (li[-1] == r']')):
                return li[1 : len(li)-1]
            else:
                return li
        elif (isinstance(li, list)):
            # Was SEMI-COLON separated, now a Python list
            l = []
            for i in li:
                if (i == r'[]'):
                    l.append(None)
                elif ((i[0] == r'[') and (i[-1] == r']')):
                    l.append(i[1 : len(i)-1])
                else:
                    l.append(i)
            return l
        else:
            raise TypeError("Unexpected type (%s) when removing square brackets" % type(li))


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


    # Recurse through the Types list structure seeing if 'typ' is present
    # (incliding anywhere in a declarative functions). This is NOT smart.
    def _FindType(self, typ, typelist):
        if (typ not in self._known_types):
            logging.error("%s is not a well known type!" % typ)

        for i, t in enumerate(typelist):
            if isinstance(t, str) and (t == typ):
                return i
            elif isinstance(t, list):
                if (self._FindType(typ, t) != -1):
                    return i
        return -1


    # Assumes a fully valid parse tree with fully bracketed "( .. )" expressions
    # Also nests PDF array objects "[ ... ]"
    # Recursive
    def _ToNestedAST(self, stk, idx=0):
        ast = []
        i = idx

        while (i < len(stk)):
            if (stk[i].type == 'FUNC_NAME'):
                j, k = self._ToNestedAST(stk, i+1)
                k = [ stk[i] ] + [ k ]  # Insert the func name at the start
                ast.append(k)
                i = j
            elif (stk[i].type == 'LPAREN') or (stk[i].type == 'ARRAY_START'):
                j, k = self._ToNestedAST(stk, i+1)
                ast.append(k)
                i = j
            elif (stk[i].type == 'RPAREN') or (stk[i].type == 'ARRAY_END'):
                # go up recursion 1 level
                return i+1, ast
            elif (stk[i].type == 'COMMA'):
                # skip COMMAs
                i = i + 1
            else:
                ast.append( stk[i] )
                i = i + 1
        logging.debug(pprint.pformat(ast))
        return i, ast


    # De-tokenize for all the simple PDF stuff (integers, numbers, true/false keywords)
    # Recursive
    def _FlattenAST(self, ast):
        i = 0
        while (i < len(ast)):
            if not isinstance(ast[i], list):
                if (ast[i].type in ('REAL', 'INTEGER', 'PDF_TRUE', 'PDF_FALSE', 'KEY_NAME', 'PDF_STRING')):
                    ast[i] = ast[i].value
            else:
                self._FlattenAST(ast[i])
            i = i + 1


    # Use Sly to parse any string with TSV names, PDF names or declaractive functions
    # Sly will raise exceptions if there are errors
    # Returns a Python list with top level TSV names or PDF names as strings and functions as lists
    def _ParseFunctions(self, func, col, obj, key):
        logging.debug("In row['%s'] %s::%s: '%s'" % (col, obj, key, func))
        stk = []
        for tok in self.__lexer.tokenize(func):
            stk.append(tok)
        num_toks = len(stk)
        i, ast = self._ToNestedAST(stk)
        logging.debug("AST: %s" % pprint.pformat(ast))
        self._FlattenAST(ast)
        if (num_toks == 1):
            ast = ast[0]
        logging.debug("Out: %s" % pprint.pformat(ast))
        return ast


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

                    # Make multi-type fields into arrays (aka Python lists)
                    if (r';' in row['Type']):
                        row['Type'] = re.split(r';', row['Type'])
                    else:
                        row['Type'] = [ row['Type'] ]
                    for i, v in enumerate(row['Type']):
                        if (r'fn:' in v):
                            row['Type'][i] = self._ParseFunctions(v, 'Type', obj_name, keyname)

                    row['Required'] = self._ParseFunctions(row['Required'], 'Required', obj_name, keyname)
                    if (row['Required'] != None) and not isinstance(row['Required'], list):
                        row['Required'] = [ row['Required'] ]

                    # Optional, but must be a known PDF version
                    if (row['DeprecatedIn'] == ''):
                        row['DeprecatedIn'] = None

                    if (r';' in row['IndirectReference']):
                        row['IndirectReference'] = self._StripSquareBackets(re.split(r';', row['IndirectReference']))
                        for i, v in enumerate(row['IndirectReference']):
                            if (v != None):
                                row['IndirectReference'][i] = self._ParseFunctions(v, 'IndirectReference', obj_name, keyname)
                    else:
                        row['IndirectReference'] = self._ParseFunctions(row['IndirectReference'], 'IndirectReference', obj_name, keyname)
                    if not isinstance(row['IndirectReference'], list):
                        row['IndirectReference'] = [ row['IndirectReference'] ]
                    # For conciseness in some cases a single FALSE/TRUE is used in place of an expanded array [];[];[]
                    # Expand this out so direct indexing is always possible
                    if (len(row['Type']) > len(row['IndirectReference'])) and (len(row['IndirectReference']) == 1):
                        for i in range(len(row['Type']) - len(row['IndirectReference'])):
                            row['IndirectReference'].append( row['IndirectReference'][0] );

                    # Must be FALSE or TRUE (uppercase only!)
                    row['Inheritable'] = self._ConvertBooleans(row['Inheritable'])

                    # Can only be one value for Key, but Key can be multi-typed
                    if (row['DefaultValue'] == ''):
                        row['DefaultValue'] = None
                    elif (r';' in row['DefaultValue']):
                        row['DefaultValue'] = self._StripSquareBackets(re.split(r';', row['DefaultValue']))
                        for i, v in enumerate(row['DefaultValue']):
                            if (v != None):
                                row['DefaultValue'][i] = self._ParseFunctions(v, 'DefaultValue', obj_name, keyname)
                    else:
                        row['DefaultValue'] = self._ParseFunctions(row['DefaultValue'], 'DefaultValue', obj_name, keyname)
                    if (row['DefaultValue'] != None) and not isinstance(row['DefaultValue'], list):
                        row['DefaultValue'] = [ row['DefaultValue'] ]
                    if (row['PossibleValues'] == ''):
                        row['PossibleValues'] = None
                    elif (r';' in row['PossibleValues']):
                        row['PossibleValues'] = self._StripSquareBackets(re.split(r';', row['PossibleValues']))
                        for i, pv in enumerate(row['PossibleValues']):
                            if (pv != None):
                                row['PossibleValues'][i] = self._ParseFunctions(pv, 'PossibleValues', obj_name, keyname)
                    else:
                        row['PossibleValues'] = self._ParseFunctions(row['PossibleValues'], 'PossibleValues', obj_name, keyname)
                    if (row['PossibleValues'] != None) and not isinstance(row['PossibleValues'], list):
                        row['PossibleValues'] = [ row['PossibleValues'] ]

                    # This is hack because a few PDF key values look like floats but are really names.
                    # Sly parsing here does not use any hints from other rows
                    # See /Version key and DocMDPTransformParameters::V
                    if (row['DefaultValue'] != None) and isinstance(row['DefaultValue'][0], float) and (row['Type'][0] == 'name'):
                        row['DefaultValue'][0] = str(row['DefaultValue'][0])
                        if (row['PossibleValues'] != None):
                            row['PossibleValues'][0] = str(row['PossibleValues'][0])

                    if (row['SpecialCase'] == ''):
                        row['SpecialCase'] = None
                    elif (r';' in row['SpecialCase']):
                        row['SpecialCase'] = self._StripSquareBackets(re.split(r';', row['SpecialCase']))
                        for i, v in enumerate(row['SpecialCase']):
                            if (v != None):
                                row['SpecialCase'][i] = self._ParseFunctions(v, 'SpecialCase', obj_name, keyname)
                    else:
                        row['SpecialCase'] = self._ParseFunctions(row['SpecialCase'], 'SpecialCase', obj_name, keyname)
                    if (row['SpecialCase'] != None) and not isinstance(row['SpecialCase'], list):
                        row['SpecialCase'] = [ row['SpecialCase'] ]

                    if (row['Link'] == ''):
                        row['Link'] = None
                    else:
                        if (r';' in row['Link']):
                            row['Link'] = self._StripSquareBackets(re.split(r';', row['Link']))
                            for i, v in enumerate(row['Link']):
                                if (v != None):
                                    row['Link'][i] = self._ParseFunctions(v, 'Link', obj_name, keyname)
                        else:
                            row['Link'] = self._ParseFunctions(row['Link'], 'Link', obj_name, keyname)
                    if (row['Link'] != None) and not isinstance(row['Link'], list):
                        row['Link'] = [ row['Link'] ]

                    if (row['Note'] == ''):
                        row['Note'] = None

                    tsvobj[keyname] = row
                self.__pdfdom[obj_name] = tsvobj

        if (self.__filecount == 0):
            logging.error("There were no TSV files in directory '%s'!" % self.__directory)
            return

        logging.info("%d TSV files processed from %s" % (self.__filecount, self.__directory))


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

                # Check if Types are sorted alphabetically
                sorted = all(isinstance(row['Type'][i], str) and isinstance(row['Type'][i+1], str) and (row['Type'][i] <= row['Type'][i+1]) for i in range(len(row['Type'])-1))
                if not sorted:
                    logging.error("Types '%s' are not sorted alphabetically for %s::%s" % (row['Type'], obj_name, keyname))

                if (row['SinceVersion'] not in self._pdf_versions):
                    logging.error("SinceVersion '%s' in %s::%s has unexpected value!" % (row['SinceVersion'], obj_name, keyname))

                if (row['DeprecatedIn'] != None) and (row['DeprecatedIn'] not in self._pdf_versions):
                    logging.error("DeprecatedIn '%s' in %s::%s has unexpected value!" % (row['DeprecatedIn'], obj_name, keyname))

                for v in row['Required']:
                    if isinstance(v, list):
                        if (v[0].type != 'FUNC_NAME') and (v[0].value != 'fn:IsRequired('):
                            logging.error("Required function '%s' does not start with 'fn:IsRequired' for %s::%s" % (row['Required'], obj_name, keyname))
                    if (r'*' in keyname) and isinstance(v, bool) and (v != False):
                        logging.error("Required needs to be FALSE for wildcard key '%s' in %s!" % (keyname, obj_name))

                if (isinstance(row['IndirectReference'], list) and (len(row['IndirectReference']) > 1)):
                    if (len(row['Type']) != len(row['IndirectReference'])):
                        logging.error("Incorrect number of elements between Type (%d) and IndirectReference (%d) for %s::%s" % (len(row['Type']), len(row['IndirectReference']), obj_name, keyname))

                i = self._FindType('stream', row['Type'])
                if (i != -1):
                    if (row['IndirectReference'][i] != True):
                        logging.error("Type 'stream' requires IndirectReference (%s) to be TRUE for %s::%s" % (row['IndirectReference'][i], obj_name, keyname))

                if not ((row['Inheritable'] == True) or (row['Inheritable'] == False)):
                    logging.error("Inheritable %s '%s' in %s::%s is not FALSE or TRUE!" % (type(row['Inheritable']), row['Inheritable'], obj_name, keyname))

                if (row['DefaultValue'] != None):
                    if (len(row['Type']) != len(row['DefaultValue'])):
                        logging.error("Incorrect number of elements between Type and DefaultValue for %s::%s" % (obj_name, keyname))

                # Validate all types are known and match DefaultValue into PossibleValues
                for i, t in enumerate(row['Type']):
                    if isinstance(t, str):
                        if (t not in self._known_types):
                            logging.error("Unknown Arlington type '%s' for %s::%s!" % (t, obj_name, keyname))

                        # Check if type and DefaultValue match in data type
                        if (row['DefaultValue'] != None) and (row['DefaultValue'][i] != None):
                            if (t == 'name') and not isinstance(row['DefaultValue'][i], str):
                                logging.error("DefaultValue '%s' is not a name for %s::%s" % (row['DefaultValue'][i], obj_name, keyname))
                            elif ((t == 'array') and not isinstance(row['DefaultValue'][i], list)):
                                logging.error("DefaultValue '%s' is not an array for %s::%s" % (row['DefaultValue'][i], obj_name, keyname))
                            elif (t == 'boolean') and not isinstance(row['DefaultValue'][i], bool):
                                logging.error("DefaultValue '%s' is not a boolean for %s::%s" % (row['DefaultValue'][i], obj_name, keyname))
                            elif (t == 'number') and not isinstance(row['DefaultValue'][i], (int, float)):
                                logging.error("DefaultValue '%s' is not a number for %s::%s" % (row['DefaultValue'][i], obj_name, keyname))
                            elif (t == 'integer') and not isinstance(row['DefaultValue'][i], int):
                                logging.error("DefaultValue '%s' is not an integer for %s::%s" % (row['DefaultValue'][i], obj_name, keyname))
                            elif ('string' in t):
                                if not isinstance(row['DefaultValue'][i], str):
                                    logging.error("DefaultValue '%s' is not a string for %s::%s" % (row['DefaultValue'][i], obj_name, keyname))
                                elif (row['DefaultValue'][i][0] != '('):
                                    logging.error("DefaultValue '%s' does not start with '(' for %s::%s" % (row['DefaultValue'][i], obj_name, keyname))
                                elif (row['DefaultValue'][i][-1] != ')'):
                                    logging.error("DefaultValue '%s' does not end with ')' for %s::%s" % (row['DefaultValue'][i], obj_name, keyname))

                    elif isinstance(t, list):
                        if not isinstance(t[0], list):
                            # Only "fn:SinceVersion(" or "fn:Deprecated(" allowed
                            if (t[0].type != 'FUNC_NAME') and (t[0].value not in ("fn:SinceVersion(", "fn:Deprecated(")):
                                logging.error("Unknown function '%s' for Type in %s::%s!" % (t, obj_name, keyname))
                            if not isinstance(t[1][1], str) or (t[1][1] not in self._known_types):
                                logging.error("Unknown type inside function '%s' for Type in %s::%s!" % (t, obj_name, keyname))
                        else:
                            # Only "fn:SinceVersion(" or "fn:Deprecated(" allowed
                            if (t[0][0].type != 'FUNC_NAME') and (t[0][0].value not in ("fn:SinceVersion(", "fn:Deprecated(")):
                                logging.error("Unknown function '%s' for Type in %s::%s!" % (t, obj_name, keyname))
                            if not isinstance(t[0][1][1], str) or (t[0][1][1] not in self._known_types):
                                logging.error("Unknown type inside function '%s' for Type in %s::%s!" % (t, obj_name, keyname))
                        # Cannot check much else for functions

#                    # Check specific syntaxes for array and strings of possible values
#                    # Check links
#                    # Check if DefaultValue was in any PossibleValues


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

    if (cli.json is not None):
        print("Saving JSON to '%s'..." % cli.json)
        arl.SaveDOMtoJSON(cli.json)

    if (cli.validate):
        print("Validating...")
        arl.ValidateDOM()

    print("Done.")
