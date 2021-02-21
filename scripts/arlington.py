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
# (i.e. mistakes in the TSV data) but there is also a validate_pdf_dom() method that will
# perform other checks.
#
# Requires Python 3 and sly: pip3 install sly
# See https://sly.readthedocs.io/en/latest/sly.html
#

import sys
import csv
import os
import glob
import re
import argparse
import pprint
import logging
import sly


class ArlingtonFnLexer(sly.Lexer):
    #debugfile = 'parser.out'

    # Set of token names.   This is always required.
    tokens = {
        FUNC_NAME, KEY_VALUE, KEY_NAME, KEY_PATH, MOD, PDF_PATH, EQ, NE, GE, LE, GT, LT,
        LOGICAL_AND, LOGICAL_OR, REAL, INTEGER, PLUS, MINUS, TIMES, DIVIDE, LPAREN,
        RPAREN, COMMA, ARRAY_START, ARRAY_END, ELLIPSIS, PDF_TRUE, PDF_FALSE, PDF_STRING
    }

    # Precedence rules
    precedence = (
        ('nonassoc', EQ, NE, LE, GE, LT, GT), # Non-associative operators
        ('left', PLUS, MINUS),
        ('left', TIMES, DIVIDE, MOD),
        ('left', LOGICAL_AND, LOGICAL_OR)
    )

    # String containing ignored characters between tokens (just SPACE)
    ignore = ' '

    # Regular expression rules for tokens
    # Longer rules need to be at the top
    FUNC_NAME    = r'fn\:[A-Z][a-zA-Z0-9]+\('
    PDF_TRUE     = r'(true)|(TRUE)'
    PDF_FALSE    = r'(false)|(FALSE)'
    PDF_STRING   = r'\([a-zA-Z0-9_\-]+\)'
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

## Terse version of sly.lex.Token.__str__/__repr__ dunder methods
def MyTokenStr(self):
    return "TOKEN(type='%s', value='%s')" % (self.type, self.value)


class Arlington:
    """
    Wrapper class around a set of Arlington TSV definition files
    """

    # All the Arlington pre-defined types (pre-sorted alphabetically)
    _known_types = [ 'array', 'bitmask', 'boolean', 'date', 'dictionary', 'integer',
                     'matrix', 'name', 'name-tree', 'null', 'number', 'number-tree',
                     'rectangle', 'stream', 'string', 'string-ascii', 'string-byte', 'string-text' ]

    # Types for which a valid Link is required
    _links_required = [ 'array', 'dictionary', 'name-tree', 'number-tree', 'stream' ]

    # Current set of versions for the SinceVersion and Deprecated columns, as well as some functions
    _pdf_versions = [ '1.0', '1.1', '1.2', '1.3', '1.4', '1.5', '1.6', '1.7', '2.0' ]

    # Base PDF tokens that will get flattened away in AST
    _basePDFtokens = ('REAL', 'INTEGER', 'PDF_TRUE', 'PDF_FALSE', 'KEY_NAME', 'PDF_STRING')


    @staticmethod
    def strip_square_brackets(li):
        """
        Only strip off outer "[...]" as inner square brackets may exist for PDF arrays
        """
        if (li is None):
            return None
        elif isinstance(li, str):
            # Single string
            if (li[0] == r'[') and (li[-1] == r']'):
                return li[1 : len(li)-1]
            else:
                return li
        elif isinstance(li, list):
            # Was SEMI-COLON separated, now a Python list
            l = []
            for i in li:
                if (i == r'[]'):
                    l.append(None)
                elif (i[0] == r'[') and (i[-1] == r']'):
                    l.append(i[1 : len(i)-1])
                else:
                    l.append(i)
            return l
        else:
            raise TypeError("Unexpected type (%s) when removing square brackets" % type(li))


    @staticmethod
    def convert_booleans(obj):
        """
        Convert spreadsheet booleans to Python: "FALSE" to False, "TRUE" to True
        (lowercase "false"/"true" are retained as PDF keywords)
        Note also that declarative functions may be used in place of Booleans!

        @param obj: the Python object (str or list) to convert
        @returns:   an updated object of the same type that was passed in
        """
        if isinstance(obj, str):
            if (obj == r'FALSE') or (obj == r'[FALSE]'):
                return False
            elif (obj == r'TRUE') or (obj == r'[TRUE]'):
                return True
            else:
                return obj
        elif isinstance(obj, list):
            l = []
            for o in obj:
                if (o == r'FALSE') or (o == r'[FALSE]'):
                    l.append(False)
                elif (o == r'TRUE') or (o == r'[TRUE]'):
                    l.append(True)
                else:
                    l.append(o)
            return l
        else:
            raise TypeError("Unexpected type '%s' for converting booleans!" % obj)


    def _find_pdf_type(self, typ, typelist):
        """
        Recurse through the Types list structure seeing if 'typ' is present
        (incliding anywhere in a declarative functions). This is NOT smart.
        """
        if (typ not in self._known_types):
            logging.error("'%s' is not a well known Arlington type!", typ)

        for i, t in enumerate(typelist):
            if isinstance(t, str) and (t == typ):
                return i
            elif isinstance(t, list):
                if (self._find_pdf_type(typ, t) != -1):
                    return i
        return -1


    @staticmethod
    def to_nested_AST(stk, idx=0):
        """
        Assumes a fully valid parse tree with fully bracketed "( .. )" expressions
        Also nests PDF array objects "[ ... ]". Recursive.
        @param stk:  AST stack
        @param idx:  index into AST stack
        @returns:  index to next item in AST stack, AST stack
        """
        ast = []
        i = idx

        while (i < len(stk)):
            if (stk[i].type == 'FUNC_NAME'):
                j, k = Arlington.to_nested_AST(stk, i+1)
                k = [ stk[i] ] + [ k ]  # Insert the func name at the start
                ast.append(k)
                i = j
            elif (stk[i].type == 'LPAREN') or (stk[i].type == 'ARRAY_START'):
                j, k = Arlington.to_nested_AST(stk, i+1)
                ast.append(k)
                i = j
            elif (stk[i].type == 'RPAREN') or (stk[i].type == 'ARRAY_END'):
                # go up recursion 1 level
                return i+1, ast
            elif (stk[i].type == 'COMMA'):
                # skip COMMAs
                i += 1
            else:
                ast.append( stk[i] )
                i += 1
        return i, ast


    def _flatten_ast(self, ast):
        """
        De-tokenize for all the base PDF stuff (integers, numbers, true/false keywords, strings)
        Recursive.
        @param ast:  AST list
        """
        i = 0
        while (i < len(ast)):
            if not isinstance(ast[i], list):
                if (ast[i].type in self._basePDFtokens):
                    ast[i] = ast[i].value
            else:
                self._flatten_ast(ast[i])
            i += 1


    def _parse_functions(self, func, col, obj, key):
        """
        Use Sly to parse any string with TSV names, PDF names or declaractive functions.
        Sly will raise exceptions if there are errors.
        @returns: Python list with top level TSV names or PDF names as strings and functions as lists
        """
        # logging.debug("In row['%s'] %s::%s: '%s'", col, obj, key, func)
        stk = []
        for tok in self.__lexer.tokenize(func):
            stk.append(tok)
        num_toks = len(stk)
        i, ast = Arlington.to_nested_AST(stk)
        # logging.debug("AST: %s", pprint.pformat(ast))
        self._flatten_ast(ast)
        if (num_toks == 1) and (stk[0].type not in ('FUNC_NAME','KEY_VALUE')):
            ast = ast[0]
        # logging.debug("Out: %s", pprint.pformat(ast))
        return ast


    def __init__(self, dir=".", pdfver="2.0"):
        """
        Constructor. Reads TSV set file-by-file and converts to Pythonese
        @param  dir:  directory folder contain TSV files
        @param  pdfver: the PDF version used for determination (default is '2.0')
        """
        self.__directory = dir
        self.__filecount = 0
        self.__pdfver = pdfver
        self.__pdfdom = {}

        # "Monkey patch" sly.lex.Token __str__ and __repr__ dunder methods to make JSON nicer
        # Don't do this if we want to read the JSON back in!
        sly.lex.Token.__str__  = MyTokenStr
        sly.lex.Token.__repr__ = MyTokenStr
        self.__lexer  = ArlingtonFnLexer()

        # Load Arlington into Python
        for filepath in glob.iglob(os.path.join(dir, r"*.tsv")):
            obj_name = os.path.splitext(os.path.basename(filepath))[0]
            self.__filecount += 1
            logging.debug('Reading %s', obj_name)
            with open(filepath, newline='') as csvfile:
                tsvreader = csv.DictReader(csvfile, delimiter='\t')
                tsvobj = {}
                for row in tsvreader:
                    keyname = row['Key']
                    if (len(row) != 12):
                        logging.error("%s::%s does not have 12 columns!", obj_name, keyname)
                    row.pop('Key')
                    if (keyname == ''):
                        raise TypeError("Key name field cannot be empty!")

                    # Make multi-type fields into arrays (aka Python lists)
                    if (r';' in row['Type']):
                        row['Type'] = re.split(r';', row['Type'])
                    else:
                        row['Type'] = [ row['Type'] ]
                    for i, v in enumerate(row['Type']):
                        if (r'fn:' in v):
                            row['Type'][i] = self._parse_functions(v, 'Type', obj_name, keyname)

                    row['Required'] = self._parse_functions(row['Required'], 'Required', obj_name, keyname)
                    if (row['Required'] is not None) and not isinstance(row['Required'], list):
                        row['Required'] = [ row['Required'] ]

                    # Optional, but must be a known PDF version
                    if (row['DeprecatedIn'] == ''):
                        row['DeprecatedIn'] = None

                    if (r';' in row['IndirectReference']):
                        row['IndirectReference'] = Arlington.strip_square_brackets(re.split(r';', row['IndirectReference']))
                        for i, v in enumerate(row['IndirectReference']):
                            if (v is not None):
                                row['IndirectReference'][i] = self._parse_functions(v, 'IndirectReference', obj_name, keyname)
                    else:
                        row['IndirectReference'] = self._parse_functions(row['IndirectReference'], 'IndirectReference', obj_name, keyname)
                    if not isinstance(row['IndirectReference'], list):
                        row['IndirectReference'] = [ row['IndirectReference'] ]
                    # For conciseness in some cases a single FALSE/TRUE is used in place of an expanded array [];[];[]
                    # Expand this out so direct indexing is always possible
                    if (len(row['Type']) > len(row['IndirectReference'])) and (len(row['IndirectReference']) == 1):
                        for i in range(len(row['Type']) - len(row['IndirectReference'])):
                            row['IndirectReference'].append( row['IndirectReference'][0] );

                    # Must be FALSE or TRUE (uppercase only!)
                    row['Inheritable'] = Arlington.convert_booleans(row['Inheritable'])

                    # Can only be one value for Key, but Key can be multi-typed
                    if (row['DefaultValue'] == ''):
                        row['DefaultValue'] = None
                    elif (r';' in row['DefaultValue']):
                        row['DefaultValue'] = Arlington.strip_square_brackets(re.split(r';', row['DefaultValue']))
                        for i, v in enumerate(row['DefaultValue']):
                            if (v is not None):
                                row['DefaultValue'][i] = self._parse_functions(v, 'DefaultValue', obj_name, keyname)
                    else:
                        row['DefaultValue'] = self._parse_functions(row['DefaultValue'], 'DefaultValue', obj_name, keyname)
                    if (row['DefaultValue'] is not None) and not isinstance(row['DefaultValue'], list):
                        row['DefaultValue'] = [ row['DefaultValue'] ]
                    if (row['PossibleValues'] == ''):
                        row['PossibleValues'] = None
                    elif (r';' in row['PossibleValues']):
                        row['PossibleValues'] = self.strip_square_brackets(re.split(r';', row['PossibleValues']))
                        for i, pv in enumerate(row['PossibleValues']):
                            if (pv is not None):
                                row['PossibleValues'][i] = self._parse_functions(pv, 'PossibleValues', obj_name, keyname)
                    else:
                        row['PossibleValues'] = self._parse_functions(row['PossibleValues'], 'PossibleValues', obj_name, keyname)
                    if (row['PossibleValues'] is not None) and not isinstance(row['PossibleValues'], list):
                        row['PossibleValues'] = [ row['PossibleValues'] ]

                    # Below is a hack(!!!) because a few PDF key values look like floats or keywords but are really names.
                    # Sly-based parsing in Python does not use any hints from other rows so it will convert to int/float/bool as it sees fit
                    # See Catalog::Version, DocMDPTransformParameters::V, DevExtensions::BaseVersion, SigFieldSeedValue::LockDocument
                    if (row['Type'][0] == 'name'):
                        if (row['DefaultValue'] is not None) and isinstance(row['DefaultValue'][0], (int,float)):
                            logging.info("Converting DefaultValue int/float/bool '%s' back to name for %s::%s", str(row['DefaultValue'][0]), obj_name, keyname)
                            row['DefaultValue'][0] = str(row['DefaultValue'][0])
                        if (row['PossibleValues'] is not None):
                            for i, v in enumerate(row['PossibleValues'][0]):
                                if isinstance(v, (int,float)):
                                    logging.info("Converting PossibleValues int/float/bool '%s' back to name for %s::%s", str(v), obj_name, keyname)
                                    row['PossibleValues'][0][i] = str(v)

                    if (row['SpecialCase'] == ''):
                        row['SpecialCase'] = None
                    elif (r';' in row['SpecialCase']):
                        row['SpecialCase'] = self.strip_square_brackets(re.split(r';', row['SpecialCase']))
                        for i, v in enumerate(row['SpecialCase']):
                            if (v is not None):
                                row['SpecialCase'][i] = self._parse_functions(v, 'SpecialCase', obj_name, keyname)
                    else:
                        row['SpecialCase'] = self._parse_functions(row['SpecialCase'], 'SpecialCase', obj_name, keyname)
                    if (row['SpecialCase'] is not None) and not isinstance(row['SpecialCase'], list):
                        row['SpecialCase'] = [ row['SpecialCase'] ]

                    if (row['Link'] == ''):
                        row['Link'] = None
                    else:
                        if (r';' in row['Link']):
                            row['Link'] = self.strip_square_brackets(re.split(r';', row['Link']))
                            for i, v in enumerate(row['Link']):
                                if (v is not None):
                                    row['Link'][i] = self._parse_functions(v, 'Link', obj_name, keyname)
                        else:
                            row['Link'] = self._parse_functions(row['Link'], 'Link', obj_name, keyname)
                    if (row['Link'] is not None) and not isinstance(row['Link'], list):
                        row['Link'] = [ row['Link'] ]

                    if (row['Note'] == ''):
                        row['Note'] = None

                    tsvobj[keyname] = row
                self.__pdfdom[obj_name] = tsvobj

        if (self.__filecount == 0):
            logging.error("There were no TSV files in directory '%s'!", self.__directory)
            return

        logging.info("%d TSV files processed from %s", self.__filecount, self.__directory)


    def validate_pdf_dom(self, pdfver="2.0"):
        """
        Does a detailed Validation of the in-memory Python data structure
        """
        if (self.__filecount == 0) or (len(self.__pdfdom) == 0):
            logging.error("There is no Arlington DOM to validate!")
            return

        for obj_name in self.__pdfdom:
            logging.debug("Validating '%s'", obj_name)
            obj = self.__pdfdom[obj_name]

            # Check if object contain any duplicate keys or has no keys
            if (len(obj) != len(set(obj))):
                logging.error("Duplicate keys in '%s'!", obj_name)
            if (len(obj) == 0):
                logging.error("Object '%s' has no keys/array entries!", obj_name)

            for keyname in obj:
                row = obj[keyname]
                logging.debug("Validating %s::%s" , obj_name, keyname)

                # Check validity of key names and array indices
                m = re.search(r'^[a-zA-Z0-9_\-\.]*\*?$', keyname)
                if (m is None):
                    logging.error("Key '%s' in object %s has unexpected characters", keyname, obj_name)

                # Check if Types are sorted alphabetically
                is_sorted = all(isinstance(row['Type'][i], str) and isinstance(row['Type'][i+1], str) and (row['Type'][i] <= row['Type'][i+1]) for i in range(len(row['Type'])-1))
                if not is_sorted:
                    logging.error("Types '%s' are not sorted alphabetically for %s::%s", row['Type'], obj_name, keyname)

                if (row['SinceVersion'] not in self._pdf_versions):
                    logging.error("SinceVersion '%s' in %s::%s has unexpected value!", row['SinceVersion'], obj_name, keyname)

                if (row['DeprecatedIn'] is not None) and (row['DeprecatedIn'] not in self._pdf_versions):
                    logging.error("DeprecatedIn '%s' in %s::%s has unexpected value!", row['DeprecatedIn'], obj_name, keyname)

                for v in row['Required']:
                    if isinstance(v, list):
                        if (v[0].type != 'FUNC_NAME') and (v[0].value != 'fn:IsRequired('):
                            logging.error("Required function '%s' does not start with 'fn:IsRequired' for %s::%s", row['Required'], obj_name, keyname)
                    if (r'*' in keyname) and isinstance(v, bool) and (v != False):
                        logging.error("Required needs to be FALSE for wildcard key '%s' in %s!", keyname, obj_name)

                if (isinstance(row['IndirectReference'], list) and (len(row['IndirectReference']) > 1)):
                    if (len(row['Type']) != len(row['IndirectReference'])):
                        logging.error("Incorrect number of elements between Type (%d) and IndirectReference (%d) for %s::%s",
                            len(row['Type']), len(row['IndirectReference']), obj_name, keyname)

                i = self._find_pdf_type('stream', row['Type'])
                if (i != -1):
                    if (row['IndirectReference'][i] != True):
                        logging.error("Type 'stream' requires IndirectReference (%s) to be TRUE for %s::%s", row['IndirectReference'][i], obj_name, keyname)

                if not ((row['Inheritable'] == True) or (row['Inheritable'] == False)):
                    logging.error("Inheritable %s '%s' in %s::%s is not FALSE or TRUE!", type(row['Inheritable']), row['Inheritable'], obj_name, keyname)

                if (row['DefaultValue'] is not None):
                    if (len(row['Type']) != len(row['DefaultValue'])):
                        logging.error("Incorrect number of elements between Type and DefaultValue for %s::%s", obj_name, keyname)

                # Validate all types are known and match DefaultValue into PossibleValues
                for i, t in enumerate(row['Type']):
                    if isinstance(t, str):
                        if (t not in self._known_types):
                            logging.error("Unknown Arlington type '%s' for %s::%s!", t, obj_name, keyname)

                        # Check if type and DefaultValue match in data type
                        if (row['DefaultValue'] is not None) and (row['DefaultValue'][i] is not None):
                            # nested lists below represent declarative functions - but they are NOT checked to see
                            # if the first element is a FUNC_NAME!!
                            if (t == 'name') and not isinstance(row['DefaultValue'][i], (str, list)):
                                logging.error("DefaultValue '%s' is not a name for %s::%s", row['DefaultValue'][i], obj_name, keyname)
                            elif (t == 'array') and not isinstance(row['DefaultValue'][i], (list)):
                                logging.error("DefaultValue '%s' is not an array for %s::%s", row['DefaultValue'][i], obj_name, keyname)
                            elif (t == 'boolean') and not isinstance(row['DefaultValue'][i], (bool, list)):
                                logging.error("DefaultValue '%s' is not a boolean for %s::%s", row['DefaultValue'][i], obj_name, keyname)
                            elif (t == 'number') and not isinstance(row['DefaultValue'][i], (int, float, list)):
                                logging.error("DefaultValue '%s' is not a number for %s::%s", row['DefaultValue'][i], obj_name, keyname)
                            elif (t == 'integer') and not isinstance(row['DefaultValue'][i], (int, list)):
                                logging.error("DefaultValue '%s' is not an integer for %s::%s", row['DefaultValue'][i], obj_name, keyname)
                            elif ('string' in t):
                                if not isinstance(row['DefaultValue'][i], (str, list)):
                                    logging.error("DefaultValue '%s' is not a string for %s::%s", row['DefaultValue'][i], obj_name, keyname)
                                elif isinstance(row['DefaultValue'][i], str):
                                    if (row['DefaultValue'][i][0] != '('):
                                        logging.error("DefaultValue '%s' does not start with '(' for %s::%s", row['DefaultValue'][i], obj_name, keyname)
                                    elif (row['DefaultValue'][i][-1] != ')'):
                                        logging.error("DefaultValue '%s' does not end with ')' for %s::%s", row['DefaultValue'][i], obj_name, keyname)

                        # Check if type and PossibleValues match in data type
                        # PossibleValues are SETS of values!
                        if (row['PossibleValues'] is not None) and (row['PossibleValues'][i] is not None):
                            if (t == 'name'):
                                if isinstance(row['PossibleValues'][i], list):
                                    for j, v in enumerate(row['PossibleValues'][i]):
                                        if not isinstance(row['PossibleValues'][i][j], (str, list)):
                                            logging.error("PossibleValues '%s' is not a name for %s::%s", row['PossibleValues'][i][j], obj_name, keyname)
                                elif not isinstance(row['PossibleValues'][i], str):
                                    logging.error("PossibleValues '%s' is not a name for %s::%s", row['PossibleValues'][i], obj_name, keyname)
                            elif (t == 'array'):
                                if isinstance(row['PossibleValues'][i], list):
                                    for j, v in enumerate(row['PossibleValues'][i]):
                                        if not isinstance(row['PossibleValues'][i][j], (list)):
                                            logging.error("PossibleValues '%s' is not an array for %s::%s", row['PossibleValues'][i][j], obj_name, keyname)
                                else:
                                    logging.error("PossibleValues '%s' is not an array for %s::%s", row['PossibleValues'][i], obj_name, keyname)
                            elif (t == 'boolean'):
                                if isinstance(row['PossibleValues'][i], list):
                                    for j, v in enumerate(row['PossibleValues'][i]):
                                        if not isinstance(row['PossibleValues'][i][j], (bool, list)):
                                            logging.error("PossibleValues '%s' is not a boolean for %s::%s", row['PossibleValues'][i][j], obj_name, keyname)
                                elif not isinstance(row['PossibleValues'][i], bool):
                                    logging.error("PossibleValues '%s' is not a boolean for %s::%s", row['PossibleValues'][i], obj_name, keyname)
                            elif (t == 'number'):
                                if isinstance(row['PossibleValues'][i], list):
                                    for j, v in enumerate(row['PossibleValues'][i]):
                                        if not isinstance(row['PossibleValues'][i][j], (int, float, list)):
                                            logging.error("PossibleValues '%s' is not a number for %s::%s", row['PossibleValues'][i][j], obj_name, keyname)
                                elif not isinstance(row['PossibleValues'][i], (int, float)):
                                    logging.error("PossibleValues '%s' is not a number for %s::%s", row['PossibleValues'][i], obj_name, keyname)
                            elif (t == 'integer'):
                                if isinstance(row['PossibleValues'][i], list):
                                    for j, v in enumerate(row['PossibleValues'][i]):
                                        if not isinstance(row['PossibleValues'][i][j], (int, list)):
                                            logging.error("PossibleValues '%s' is not an integer for %s::%s", row['PossibleValues'][i][j], obj_name, keyname)
                                elif not isinstance(row['PossibleValues'][i], int):
                                    logging.error("PossibleValues '%s' is not an integer for %s::%s", row['PossibleValues'][i], obj_name, keyname)
                            elif ('string' in t):
                                if isinstance(row['PossibleValues'][i], list):
                                    for j, v in enumerate(row['PossibleValues'][i]):
                                        if not isinstance(row['PossibleValues'][i][j], (str, list)):
                                            logging.error("PossibleValues '%s' is not a string for %s::%s", row['PossibleValues'][i][j], obj_name, keyname)
                                        elif isinstance(row['PossibleValues'][i][j], str):
                                            if (row['PossibleValues'][i][j][0] != '('):
                                                logging.error("PossibleValues '%s' does not start with '(' for %s::%s", row['PossibleValues'][i][j], obj_name, keyname)
                                            elif (row['PossibleValues'][i][j][-1] != ')'):
                                                logging.error("PossibleValues '%s' does not end with ')' for %s::%s", row['PossibleValues'][i][j], obj_name, keyname)
                                elif isinstance(row['PossibleValues'][i], str):
                                    if (row['PossibleValues'][i][0] != '('):
                                        logging.error("PossibleValues '%s' does not start with '(' for %s::%s", row['PossibleValues'][i], obj_name, keyname)
                                    elif (row['DefaultValue'][i][-1] != ')'):
                                        logging.error("PossibleValues '%s' does not end with ')' for %s::%s", row['PossibleValues'][i], obj_name, keyname)
                                else:
                                    logging.error("PossibleValues '%s' is not a str for %s::%s", row['PossibleValues'][i], obj_name, keyname)

                        if (row['Link'] is not None):
                            if (t in self._links_required):
                                if (row['Link'][i] is None):
                                    logging.error("Link '%s' is missing for type %s in %s::%s", row['Link'][i], t, obj_name, keyname)
                                elif not isinstance(row['Link'][i], (str, list)):
                                    logging.error("Link '%s' is not a list for type %s in %s::%s", row['Link'][i], t, obj_name, keyname)
                                else:
                                    if isinstance(row['Link'][i], str):
                                        lnk = row['Link'][i]
                                        lnkobj = self.__pdfdom[lnk]
                                        if (lnkobj is None):
                                            logging.error("Bad link '%s' in %s::%s", row['Link'][i], obj_name, keyname)
                                    else: # list
                                        for j, v in enumerate(row['Link'][i]):
                                            if isinstance(row['Link'][i][j], str):
                                                lnk = row['Link'][i][j]
                                                lnkobj = self.__pdfdom[lnk]
                                                if (lnkobj is None):
                                                    logging.error("Bad link '%s' in %s::%s", row['Link'][i][j], obj_name, keyname)
                                            elif not isinstance(row['Link'][i][j], list):
                                                logging.error("Link '%s' is not a function for type %s in %s::%s", row['Link'][i][j], t, obj_name, keyname)
                            else:
                                # Confirm explicitly NO links
                                if (row['Link'][i] is not None):
                                    logging.error("Link '%s' exists for type %s in %s::%s", row['Link'][i], t, obj_name, keyname)

                    elif isinstance(t, list):
                        if not isinstance(t[0], list):
                            # Only "fn:SinceVersion(" or "fn:Deprecated(" allowed
                            if (t[0].type != 'FUNC_NAME') and (t[0].value not in ("fn:SinceVersion(", "fn:Deprecated(")):
                                logging.error("Unknown function '%s' for Type in %s::%s!", t, obj_name, keyname)
                            if not isinstance(t[1][1], str) or (t[1][1] not in self._known_types):
                                logging.error("Unknown type inside function '%s' for Type in %s::%s!", t, obj_name, keyname)
                        else:
                            # Only "fn:SinceVersion(" or "fn:Deprecated(" allowed
                            if (t[0][0].type != 'FUNC_NAME') and (t[0][0].value not in ("fn:SinceVersion(", "fn:Deprecated(")):
                                logging.error("Unknown function '%s' for Type in %s::%s!", t, obj_name, keyname)
                            if not isinstance(t[0][1][1], str) or (t[0][1][1] not in self._known_types):
                                logging.error("Unknown type inside function '%s' for Type in %s::%s!", t, obj_name, keyname)

                    # Check if DefaultValue is valid in any PossibleValues
                    # T.B.D.

            # Check for incoming links to this object (obj_name)
            found = 0
            for i in self.__pdfdom:
                lnkobj = self.__pdfdom[i]
                for k in lnkobj:
                    r = lnkobj[k]
                    if (r['Link'] is not None):
                        if isinstance(r['Link'], str) and (r['Link'] == obj_name):
                            found += 1 # Should never happen!
                        elif isinstance(r['Link'], list):
                            for v in r['Link']:
                                if isinstance(v, str) and (v == obj_name):
                                    found += 1
                                elif isinstance(v, list):
                                    for j in v:
                                        if isinstance(j, str) and (j == obj_name):
                                            found += 1
                                        elif isinstance(j, list):
                                            for x in j:
                                                if isinstance(x, str) and (x == obj_name):
                                                    found += 1
                                                elif isinstance(x, list):
                                                    for y in x:
                                                        if isinstance(y, str) and (y == obj_name):
                                                            found += 1
            logging.debug("Found %d links to '%s'", found, obj_name)
            if (found == 0):
                logging.error("Unlinked object %s!", obj_name)


    def save_dom_to_json(self, json_file):
        """
        Pretty-print the DOM as JSON to a  file (helps readability for debugging)
        """
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

    if (cli.tsvdir is None) or not os.path.isdir(cli.tsvdir):
        print("'%s' is not a valid directory" % cli.tsvdir)
        cli_parser.print_help()
        sys.exit()

    print("Loading...")
    arl = Arlington(cli.tsvdir)

    if (cli.json is not None):
        print("Saving JSON to '%s'..." % cli.json)
        arl.save_dom_to_json(cli.json)

    if (cli.validate):
        print("Validating...")
        arl.validate_pdf_dom()

    print("Done.")
