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
# Example usage (Linux CLI):
# $ python3 arlington.py --tsvdir ../tsv/latest --validate
#
# Requires:
# - Python 3
# - pip3 install sly pikepdf
# - See https://sly.readthedocs.io/en/latest/sly.html
# - PikePDF is a wrapper around qpdf. See https://pikepdf.readthedocs.io/en/latest/api/main.html
#
# Python QA:
# Note that Sly causes various errors because of the lexer magic!!!
# - flake8 --ignore E501,E221,E226 arlington.py
# - pyflakes arlington.py
# - mypy arlington.py
#

import sys
import csv
import os
import glob
import re
import argparse
import pprint
import logging
import json
import decimal
import pikepdf
import traceback
from typing import Any, Dict, List, Optional, Tuple, Union
import sly     # type: ignore


class ArlingtonFnLexer(sly.Lexer):
    # debugfile = 'arlington-parser.out'

    # Set of token names. This is always required.
    tokens = {
        FUNC_NAME, KEY_VALUE, KEY_NAME, KEY_PATH, MOD, PDF_PATH, EQ, NE, GE, LE, GT, LT,
        LOGICAL_AND, LOGICAL_OR, REAL, INTEGER, PLUS, MINUS, TIMES, DIVIDE, LPAREN,
        RPAREN, COMMA, ARRAY_START, ARRAY_END, ELLIPSIS, PDF_TRUE, PDF_FALSE, PDF_STRING
    }

    # Precedence rules
    precedence = (
        ('nonassoc', EQ, NE, LE, GE, LT, GT),    # Non-associative operators
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
    PDF_STRING   = r'\'[^\']+\''
    MOD          = r'mod'
    ELLIPSIS     = r'\.\.\.'
    KEY_VALUE    = r'@(\*|[0-9]+|[0-9]+\*|[a-zA-Z0-9_\.\-]+)'
    # Key name is both a PDF name or a TSV filename
    # Key name of just '*' is potentially ambiguous with TIMES (multiply) operator.
    # Key name which is numeric array index ([0-9+) and is potentially ambiguous with integers.
    # Array indices are integers, or integer followed by ASTERISK (wildcard) - need to use SPACEs to disambiguate with TIMES
    KEY_PATH     = r'(parent::)?(([a-zA-Z]|[a-zA-Z][0-9]*|[0-9]*\*|[0-9]*[a-zA-Z])[a-zA-Z0-9_\.\-]*::)+'
    KEY_NAME     = r'([_a-zA-Z]|[_a-zA-Z][0-9]*|[0-9]*\*|[0-9]*[_a-zA-Z])[a-zA-Z0-9_:\.\-]*'
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


# Terse version of sly.lex.Token.__str__/__repr__ dunder methods
def MyTokenStr(self) -> str:
    return "TOKEN(type='%s', value='%s')" % (self.type, self.value)


# Function to JSON-ify sly.lex.Token objects
def sly_lex_Token_to_json(self) -> Dict[str, Union[str, sly.lex.Token]]:
    if isinstance(self, sly.lex.Token):
        return {'object': 'sly.lex.Token', 'type': self.type, 'value': self.value}
    return {'error': '!not a sly.lex.Token!'}


class Arlington:
    """
    Wrapper class around a set of Arlington TSV PDF definition files
    """

    # All the Arlington pre-defined types (pre-sorted alphabetically)
    __known_types = frozenset(['array', 'bitmask', 'boolean', 'date', 'dictionary', 'integer',
                     'matrix', 'name', 'name-tree', 'null', 'number', 'number-tree',
                     'rectangle', 'stream', 'string', 'string-ascii', 'string-byte', 'string-text'])

    # Arlington 'Types' for which a valid Link is required
    __links_required = frozenset(['array', 'dictionary', 'name-tree', 'number-tree', 'stream'])

    # Current set of versions for the SinceVersion and Deprecated columns, as well as some functions
    __pdf_versions = frozenset(['1.0', '1.1', '1.2', '1.3', '1.4', '1.5', '1.6', '1.7', '2.0'])

    # Base PDF tokens that will get "flattened away" during declarative function AST processing
    __basePDFtokens = frozenset(['REAL', 'INTEGER', 'PDF_TRUE', 'PDF_FALSE', 'KEY_NAME', 'PDF_STRING'])

    # Mathematical comparison operators for declarative functions
    __comparison_ops = frozenset(['EQ', 'NE', 'GE', 'LE', 'GT', 'LT'])

    # Type definition
    AST = List[sly.lex.Token]

    def validate_fn_void(self, ast: AST) -> bool:
        """
        Validates all functions which take no parameters
        @param ast: AST to be validated.
        @returns: True if a valid void function. False otherwise
        """
        if (len(ast) == 0):
            return True
        return False

    def validate_fn_array_length(self, ast: AST) -> bool:
        """
        fn:ArrayLength( <key-name/key-path/array-index> ) <condition-op> <integer>

        @param ast: AST to be validated.
        @returns: True if a valid 'fn:ArrayLength' function. False otherwise
        """
        if ((len(ast) == 1) and
            not isinstance(ast[0], list) and (ast[0].type in ('KEY_NAME', 'INTEGER'))):
            return True
        elif (len(ast) >= 1) and isinstance(ast[0], list):
            return True
        elif (len(ast) > 1) and (ast[0].type == 'KEY_PATH') and (ast[1].type == 'KEY_NAME'):
            return True
        return False

    def validate_fn_array_sort_ascending(self, ast: AST) -> bool:
        """
        Validates fn:ArraySortAscending(key, step)
        @param ast: AST to be validated.
        @returns: True if a valid function. False otherwise
        """
        if ((len(ast) == 2) and (ast[0].type in ('KEY_NAME')) and (ast[1].type in ('INTEGER'))):
            return True
        return False

    def validate_fn_extension(self, ast: AST) -> bool:
        """
        Validates the fn:Extension predicate with a first argument that a name and an optional second argument.
        @param ast: AST to be validated.
        @returns: True if a valid function. False otherwise
        """
        if ((len(ast) >= 1) and (ast[0].type == 'KEY_NAME')):
            return True
        return False

    def validate_fn_version(self, ast: AST) -> bool:
        """
        Validates a declarative function with a first argument that is a PDF version string and optional
        second argument.
        @param ast: AST to be validated.
        @returns: True if a valid function. False otherwise
        """
        if ((len(ast) >= 1) and (ast[0].type == 'REAL') and (str(ast[0].value) in self.__pdf_versions)):
            return True
        return False

    def validate_fn_bit(self, ast: AST) -> bool:
        """
        Validates a single bit set/clear declarative function with a single bit position argument (1-32 inclusive)
        @param ast: AST to be validated.
        @returns: True if a valid 'fn:BitSet/Clear(' function. False otherwise
        """
        if (len(ast) == 1) and (ast[0].type == 'INTEGER') and (ast[0].value >= 1) and (ast[0].value <= 32):
            return True
        return False

    def validate_fn_bits(self, ast: AST) -> bool:
        """
        Validates a bit range set/clear declarative function with a two bit position arguments (each 1-32
        inclusive). The first argument must be less than the second argument to form a valid bit range.
        @param ast: AST to be validated.
        @returns: True if a valid 'fn:BitsSet/Clear(' function. False otherwise
        """
        if ((len(ast) == 2) and
            (ast[0].type == 'INTEGER') and (ast[0].value >= 1) and (ast[0].value <= 32) and
            (ast[1].type == 'INTEGER') and (ast[1].value >= 1) and (ast[1].value <= 32) and
            (ast[0].value < ast[1].value)):
            return True
        return False

    def validate_fn_contains(self, ast: AST) -> bool:
        """
        Validates the fn:Contains predicate with key-value 1st argument and a statement (anything) 2nd arg.
        @param ast: AST to be validated.
        @returns: True if a valid 'fn:BitsSet/Clear(' function. False otherwise
        """
        if ((len(ast) == 2) and (ast[0].type == 'KEY_VALUE')):
            return True
        return False

    def validate_fn_anything(self, ast: AST) -> bool:
        """
        Validates all generic predicates. At least one argument is required.
        @param ast: AST to be validated.
        @returns: True if a valid function. False otherwise
        """
        return (len(ast) > 0)

    def validate_fn_get_page_property(self, ast: AST) -> bool:
        """
        @param ast: AST to be validated.
        @returns: True if a valid function. False otherwise
        """
        if (len(ast) >= 2) and (ast[0].type == 'KEY_VALUE') and (ast[1].type in ['KEY_NAME', 'KEY_PATH']):
            return True
        return False

    def validate_fn_colorants(self, ast: AST) -> bool:
        """
        @param ast: AST to be validated.
        @returns: True if a valid function. False otherwise
        """
        if (len(ast) == 2) and (ast[0].type == 'KEY_PATH') and (ast[1].type == 'INTEGER'):
            return True
        return False

    def validate_fn_ignore(self, ast: AST) -> bool:
        """
        @param ast: AST to be validated.
        @returns: True if a valid function. False otherwise
        """
        if (len(ast) == 0) or isinstance(ast[0], list):
            return True
        if (len(ast) == 1) and (ast[0].type in ('KEY_NAME', 'INTEGER')):
            return True
        if (len(ast) >= 3) and (ast[0].type == 'KEY_VALUE') and (ast[1].type in self.__comparison_ops):
            return True
        return False

    def validate_fn_in_map(self, ast: AST) -> bool:
        """
        @param ast: AST to be validated.
        @returns: True if a valid function. False otherwise
        """
        if (len(ast) == 2) and (ast[0].type == 'KEY_PATH') and (ast[1].type in ('KEY_NAME', 'INTEGER')):
            return True
        elif (len(ast) == 1) and (ast[0].type == 'KEY_NAME'):
            return True
        return False

    def validate_fn_is_meaningful(self, ast: AST) -> bool:
        """
        @param ast: AST to be validated.
        @returns: True if a valid function. False otherwise
        """
        if (len(ast) == 0) or isinstance(ast[0], list):
            return True
        elif (len(ast) >= 3) and (ast[0].type == 'KEY_VALUE') and (ast[1].type in self.__comparison_ops):
            return True
        elif (len(ast) >= 4) and (ast[0].type == 'KEY_PATH') and (ast[1].type == 'KEY_VALUE') and (ast[2].type in self.__comparison_ops):
            return True
        return False

    def validate_fn_is_required(self, ast: AST) -> bool:
        """
        @param ast: AST to be validated.
        @returns: True if a valid function. False otherwise
        """
        if (len(ast) == 0) or isinstance(ast[0], list):
            return True
        if (len(ast) >= 3):
            if (ast[0].type == 'FUNC_NAME'):
                return True
            if (ast[0].type == 'KEY_VALUE') and (ast[1].type in self.__comparison_ops):
                return True
        if (len(ast) >= 4) and (ast[0].type == 'KEY_PATH') and (ast[1].type in ('KEY_NAME', 'KEY_VALUE')) and (ast[2].type in self.__comparison_ops):
            return True
        return False

    def validate_fn_must_be_direct(self, ast: AST) -> bool:
        """
        @param ast: AST to be validated.
        @returns: True if a valid function. False otherwise
        """
        if (len(ast) == 0) or isinstance(ast[0], list):
            return True
        if (len(ast) == 1) and (ast[0].type == 'KEY_VALUE'):
            return True
        if (len(ast) == 2) and (ast[0].type == 'KEY_PATH') and (ast[1].type in ('KEY_NAME', 'INTEGER')):
            return True
        return False

    def validate_fn_must_be_indirect(self, ast: AST) -> bool:
        """
        @param ast: AST to be validated.
        @returns: True if a valid function. False otherwise
        """
        if (len(ast) == 0) or isinstance(ast[0], list):
            return True
        if (len(ast) == 1) and (ast[0].type == 'KEY_VALUE'):
            return True
        if (len(ast) == 2) and (ast[0].type == 'FUNC_NAME') and (ast[1].type in ('REAL', 'INTEGER')):
            return True
        return False

    def validate_fn_rect(self, ast: AST) -> bool:
        """
        @param ast: AST to be validated.
        @returns: True if a valid 'fn:RectWidth(' function. False otherwise
        """
        if (len(ast) == 1) and (ast[0].type in ('KEY_NAME', 'INTEGER')):
            return True
        return False

    def validate_fn_required_value(self, ast: AST) -> bool:
        """
        @param ast: AST to be validated.
        @returns: True if a valid function. False otherwise
        """
        if not isinstance(ast[0], list):
            if ((len(ast) == 4) and (ast[0].type in ('KEY_VALUE','FUNC_NAME'))):
                return True
        else:
            if (len(ast) == 2):
                if (isinstance(ast[0][0], list) and (ast[0][0][0].type == 'FUNC_NAME')):
                    return True
            elif (len(ast) >= 3):
                if ((((ast[0][0].type == 'KEY_VALUE') and (ast[0][1].type in self.__comparison_ops)) and
                    (ast[1].type in ('LOGICAL_OR', 'LOGICAL_AND')) and
                    ((ast[2][0].type == 'KEY_VALUE') and (ast[2][1].type in self.__comparison_ops))) or
                    ((ast[0][0].type == 'FUNC_NAME'))):
                    return True
        return False

    def validate_fn_is_present(self, ast: AST) -> bool:
        """
        @param ast: AST to be validated.
        @returns: True if a valid 'fn:IsPresent(' function. False otherwise
        """
        if (len(ast) >= 1):
            if (isinstance(ast[0], list) or (ast[0].type in ('KEY_NAME', 'INTEGER', 'KEY_PATH', 'KEY_VALUE'))):
                return True
        return False

    def validate_fn_is_field_name(self, ast: AST) -> bool:
        """
        @param ast: AST to be validated.
        @returns: True if a valid 'fn:IsFieldName(' function. False otherwise
        """
        if (len(ast) >= 1):
            if (isinstance(ast[0], list) or (ast[0].type in ('KEY_VALUE'))):
                return True
        return False

    def validate_fn_stream_length(self, ast: AST) -> bool:
        """
        @param ast: AST to be validated.
        @returns: True if a valid function. False otherwise
        """
        return True    # TODO ###################################

    def validate_fn_string_length(self, ast: AST) -> bool:
        """
        fn:StringLength(<key-name/array-index> , [ <condition> ] ) <comparison-op> <integer>

        @param ast: AST to be validated.
        @returns: True if a valid function. False otherwise
        """
        if (len(ast) == 1) and (ast[0].type in ('KEY_NAME', 'INTEGER')):
            return True
        elif (len(ast) > 1) and (ast[0].type in ('KEY_NAME', 'INTEGER')):
            # ToDo: validate optional <condition> statement
            return True
        return False

    def validate_fn_default_value(self, ast: AST) -> bool:
        """
        fn:DefaultValue( <condition>, value )

        @param ast: AST to be validated.
        @returns: True if a valid function. False otherwise
        """
        if (len(ast) >= 4) and (ast[len(ast)-1].type in ('KEY_NAME', 'PDF_STRING', 'INTEGER')):
            # ToDo: check condition
            return True
        return False

    # Validation functions for every Arlington declarative function
    __validate_fns = {
        'fn:AlwaysUnencrypted(': validate_fn_void,
        'fn:ArrayLength(': validate_fn_array_length,
        'fn:ArraySortAscending(': validate_fn_array_sort_ascending,
        'fn:BeforeVersion(': validate_fn_version,
        'fn:BitClear(': validate_fn_bit,
        'fn:BitSet(': validate_fn_bit,
        'fn:BitsClear(': validate_fn_bits,
        'fn:BitsSet(': validate_fn_bits,
        'fn:Contains(': validate_fn_contains,
        'fn:Deprecated(': validate_fn_version,
        'fn:Eval(': validate_fn_anything,
        'fn:FileSize(': validate_fn_void,
        'fn:FontHasLatinChars(': validate_fn_void,
        'fn:PageProperty(': validate_fn_get_page_property,
        'fn:Ignore(': validate_fn_ignore,
        'fn:HasProcessColorants(': validate_fn_colorants,
        'fn:HasSpotColorants(': validate_fn_colorants,
        'fn:ImageIsStructContentItem(': validate_fn_void,
        'fn:ImplementationDependent(': validate_fn_void,
        'fn:InKeyMap(': validate_fn_in_map,
        'fn:InNameTree(': validate_fn_in_map,
        'fn:IsAssociatedFile(': validate_fn_void,
        'fn:IsEncryptedWrapper(': validate_fn_void,
        'fn:IsFieldName(': validate_fn_is_field_name,
        'fn:IsHexString(': validate_fn_void,
        'fn:IsLastInNumberFormatArray(': validate_fn_anything,
        'fn:IsMeaningful(': validate_fn_is_meaningful,
        'fn:IsPDFTagged(': validate_fn_void,
        'fn:IsPDFVersion(': validate_fn_version,
        'fn:IsPresent(': validate_fn_is_present,
        'fn:IsRequired(': validate_fn_is_required,
        'fn:KeyNameIsColorant(': validate_fn_void,
        'fn:MustBeDirect(': validate_fn_must_be_direct,
        'fn:MustBeIndirect(': validate_fn_must_be_indirect,
        'fn:NoCycle(': validate_fn_void,
        'fn:NotStandard14Font(': validate_fn_void,
        'fn:NumberOfPages(': validate_fn_void,
        'fn:PageContainsStructContentItems(': validate_fn_void,
        'fn:RectHeight(': validate_fn_rect,
        'fn:RectWidth(': validate_fn_rect,
        'fn:RequiredValue(': validate_fn_required_value,
        'fn:Extension(': validate_fn_extension,
        'fn:SinceVersion(': validate_fn_version,
        'fn:StreamLength(': validate_fn_stream_length,
        'fn:StringLength(': validate_fn_string_length,
        'fn:DefaultValue(': validate_fn_default_value,
        'fn:Not(': validate_fn_anything
    }

    @staticmethod
    def __strip_square_brackets(li: Optional[Union[str, List[str]]]) -> Union[str, Optional[List[Optional[str]]]]:
        """
        Only strip off outer "[...]" as inner square brackets may exist for PDF arrays
        @param li: a string or nested list of strings/lists
        """
        if (li is None):
            return None
        elif isinstance(li, str):
            # Single string
            if (li[0] == r'[') and (li[-1] == r']'):
                return li[1:len(li)-1]
            else:
                return li
        elif isinstance(li, list):
            # Was SEMI-COLON separated, now a Python list
            lst: List[Optional[str]] = []
            for i in li:
                if (i == r'[]'):
                    lst.append(None)
                elif (i[0] == r'[') and (i[-1] == r']'):
                    lst.append(i[1:len(i)-1])
                else:
                    lst.append(i)
            return lst
        else:
            raise TypeError("Unexpected type (%s) when removing square brackets" % type(li))

    @staticmethod
    def __convert_booleans(obj: Any) -> Any:
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
            li = []
            for o in obj:
                if (o == r'FALSE') or (o == r'[FALSE]'):
                    li.append(False)
                elif (o == r'TRUE') or (o == r'[TRUE]'):
                    li.append(True)
                else:
                    li.append(o)
            return li
        else:
            raise TypeError("Unexpected type '%s' for converting booleans!" % obj)

    def __reduce_linkslist(self, linkslist: List[str], reduced_list: List[str] = []) -> List[str]:
        """
        Reduces a 'Link' list of strings (potentially including declarative functions) to a
        simple list of Arlington TSV links in the same order as the original list.
        Recursive function.
        @param typelist: list of Arlington 'Links' (TSV filenames) including declarative functions
        @param typelist: reduced list of just Arlington 'Link' TSV names (appended to)
        @returns: reduced list (of at least length 1)
        """
        if (linkslist is None):
            return None

        for lk in linkslist:
            if isinstance(lk, str):
                reduced_list.append(lk)
            elif isinstance(lk, list):
                # Declarative functions are lists so recurse
                reduced_list = self.__reduce_linkslist(lk, reduced_list)
        return reduced_list

    def __reduce_typelist(self, typelist: List[str], reduced_list: List[str] = []) -> List:
        """
        Reduces a 'Types' list of strings (potentially including declarative functions) to a simple
        alphabetically sorted list of Arlington type strings in the same order as TSV.
        Recursive function.
        @param typelist: list of Arlington Types
        @param reduced_list: reduced list of Arlington Types (appended to)
        @returns: reduced list (of at least length 1)
        """
        for t in typelist:
            if isinstance(t, str):
                if (t in self.__known_types):
                    reduced_list.append(t)
            elif isinstance(t, list):
                # Declarative functions are lists so recurse
                reduced_list = self.__reduce_typelist(t, reduced_list)
        return reduced_list

    def __find_pdf_type(self, types: Union[str, List[str]], typelist: List[str]) -> int:
        """
        Recurse through a 'Types' list of strings seeing if one of a string in 'types' list
        is present (including anywhere in a declarative functions). This is NOT smart and
        does not process/understand declarative functions!
        @param types: a list of known Arlington 'Type' strings
        @param typelist: list of Arlington Types
        @returns: index into typelist if a type in 'types' is found, -1 otherwise
        """
        i: int
        t: str
        for i, t in enumerate(typelist):
            if isinstance(t, str):
                if (t not in self.__known_types):
                    logging.critical("'%s' is not a well known Arlington type!", t)
                if isinstance(types, list):
                    for ea in types:
                        if isinstance(ea, str) and (ea in t):
                            return i
                    return -1
                elif isinstance(types, str) and (types in t):
                    return i
            elif isinstance(t, list):
                # Declarative functions are lists so recurse on the function
                if (self.__find_pdf_type(types, t) != -1):
                    return i
        return -1

    def to_nested_AST(self, stk: AST, idx: int = 0) -> Tuple[int, AST]:
        """
        Assumes a fully valid parse tree with fully bracketed "( .. )" expressions
        Also nests PDF array objects "[ ... ]". Recursive.
        @param stk:  AST stack
        @param idx:  index into AST stack
        @returns:  index to next item in AST stack, AST stack
        """
        ast: List[sly.lex.Token] = []
        i: int = idx

        while (i < len(stk)):
            if (stk[i].type == 'FUNC_NAME'):
                j, k = self.to_nested_AST(stk, i+1)
                if (self.__validating):
                    if (stk[i].value in self.__validate_fns):
                        fn_ok = self.__validate_fns[stk[i].value](self, k)
                        if not fn_ok:
                            logging.error("Invalid declarative function %s: %s", stk[i].value, k)
                    else:
                        logging.error("Unknown declarative function %s: %s", stk[i].value, k)
                k = [stk[i]] + [k]  # Insert the func name at the start
                ast.append(k)
                i = j
            elif (stk[i].type == 'LPAREN') or (stk[i].type == 'ARRAY_START'):
                j, k = self.to_nested_AST(stk, i+1)
                ast.append(k)
                i = j
            elif (stk[i].type == 'RPAREN') or (stk[i].type == 'ARRAY_END'):
                # go up recursion 1 level
                return i+1, ast
            elif (stk[i].type == 'COMMA'):
                # skip COMMAs
                i += 1
            else:
                ast.append(stk[i])
                i += 1
        return i, ast

    def __flatten_ast(self, ast: AST) -> None:
        """
        De-tokenize for all the base PDF stuff (integers, numbers, true/false keywords, strings)
        Recursive.
        @param ast:  AST list
        """
        i = 0
        while (i < len(ast)):
            if not isinstance(ast[i], list):
                if (ast[i].type in self.__basePDFtokens):
                    ast[i] = ast[i].value
            else:
                self.__flatten_ast(ast[i])
            i += 1

    def _parse_functions(self, func: str, col: str, obj: str, key: str) -> AST:
        """
        Use Sly to parse any string with TSV names, PDF names or declaractive functions.
        Sly will raise exceptions if there are errors.
        @param func: string from a TSV column that contains a 'fn:' declarative function
        @param col: column name from TSV file (just for error messages)
        @param obj: object name (TSV filename) for this function (just for error messages)
        @param key: name of the key on 'obj' for this function (just for error messages)
        @returns: Python list with top level TSV names or PDF names as strings and functions as lists
        """
        logging.info("In row['%s'] %s::%s: '%s'", col, obj, key, func)
        stk = []
        for tok in self.__lexer.tokenize(func):
            stk.append(tok)
        num_toks = len(stk)
        i, ast = self.to_nested_AST(stk)
        # logging.debug("AST: %s", pprint.pformat(ast))
        self.__flatten_ast(ast)
        if (num_toks == 1) and (stk[0].type not in ('FUNC_NAME', 'KEY_VALUE')):
            if (len(ast) > 0):
                ast = ast[0]
        # logging.info("Out: %s", pprint.pformat(ast))
        if (num_toks == 0) or (len(stk) == 0) or (ast is None):   # or (not isinstance(ast, list)):
            logging.error("_parse_functions('%s', '%s::%s', '%s')", col, obj, key, func)
        return ast

    def __init__(self, dir: str = ".", pdfver: str = "2.0", validating: bool = False):
        """
        Constructor. Reads TSV set file-by-file and converts to Pythonese
        @param  dir:  directory folder contain TSV files
        @param  pdfver: the PDF version used for determination (default is '2.0')
        """
        self.__directory: str = dir
        self.__filecount: int = 0
        self.__pdfver: str = pdfver
        self.__pdfdom: Dict[str, Any] = {}
        self.__validating: bool = validating

        # "Monkey patch" sly.lex.Token __str__ and __repr__ dunder methods to make JSON nicer
        # Don't do this if we want to read the JSON back in!
        self.__old_str = sly.lex.Token.__str__
        sly.lex.Token.__str__  = MyTokenStr
        self.__old_repr = sly.lex.Token.__repr__
        sly.lex.Token.__repr__ = MyTokenStr
        self.__lexer  = ArlingtonFnLexer()

        try:
            # Load Arlington into Python
            filepath: str
            for filepath in glob.iglob(os.path.join(dir, r"*.tsv")):
                obj_name = os.path.splitext(os.path.basename(filepath))[0]
                self.__filecount += 1
                logging.debug("Reading '%s'", obj_name)
                with open(filepath, newline='') as csvfile:
                    tsvreader = csv.DictReader(csvfile, delimiter='\t')
                    tsvobj = {}
                    row: Any
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
                            row['Type'] = [row['Type']]
                        for i, v in enumerate(row['Type']):
                            if (r'fn:' in v):
                                row['Type'][i] = self._parse_functions(v, 'Type', obj_name, keyname)

                        row['Required'] = self._parse_functions(row['Required'], 'Required', obj_name, keyname)
                        if (row['Required'] is not None) and not isinstance(row['Required'], list):
                            row['Required'] = [row['Required']]

                        # SinceVersion now has basic predicates for Extensions
                        row['SinceVersion'] = self._parse_functions(row['SinceVersion'], 'SinceVersion', obj_name, keyname)

                        # Optional, but must be a known PDF version
                        if (row['DeprecatedIn'] == ''):
                            row['DeprecatedIn'] = None

                        if (r';' in row['IndirectReference']):
                            row['IndirectReference'] = Arlington.__strip_square_brackets(re.split(r';', row['IndirectReference']))
                            for i, v in enumerate(row['IndirectReference']):
                                if (v is not None):
                                    row['IndirectReference'][i] = self._parse_functions(v, 'IndirectReference', obj_name, keyname)
                        else:
                            row['IndirectReference'] = self._parse_functions(row['IndirectReference'], 'IndirectReference', obj_name, keyname)
                        if not isinstance(row['IndirectReference'], list):
                            row['IndirectReference'] = [row['IndirectReference']]
                        # For conciseness in some cases a single FALSE/TRUE is used in place of an expanded array [];[];[]
                        # Expand this out so direct indexing is always possible
                        if (len(row['Type']) > len(row['IndirectReference'])) and (len(row['IndirectReference']) == 1):
                            for i in range(len(row['Type']) - len(row['IndirectReference'])):
                                row['IndirectReference'].append(row['IndirectReference'][0])

                        # Must be FALSE or TRUE (uppercase only!)
                        row['Inheritable'] = Arlington.__convert_booleans(row['Inheritable'])

                        # Can only be one value for Key, but Key can be multi-typed or predicate
                        if (row['DefaultValue'] == ''):
                            row['DefaultValue'] = None
                        elif (r';' in row['DefaultValue']):
                            row['DefaultValue'] = self.__strip_square_brackets(re.split(r';', row['DefaultValue']))
                            for i, v in enumerate(row['DefaultValue']):
                                if (v is not None):
                                    row['DefaultValue'][i] = self._parse_functions(v, 'DefaultValue', obj_name, keyname)
                        else:
                            row['DefaultValue'] = self._parse_functions(row['DefaultValue'], 'DefaultValue', obj_name, keyname)
                        if (row['DefaultValue'] is not None) and not isinstance(row['DefaultValue'], list):
                            row['DefaultValue'] = [row['DefaultValue']]
                        if (row['PossibleValues'] == ''):
                            row['PossibleValues'] = None
                        elif (r';' in row['PossibleValues']):
                            row['PossibleValues'] = self.__strip_square_brackets(re.split(r';', row['PossibleValues']))
                            for i, pv in enumerate(row['PossibleValues']):
                                if (pv is not None):
                                    row['PossibleValues'][i] = self._parse_functions(pv, 'PossibleValues', obj_name, keyname)
                        else:
                            row['PossibleValues'] = self._parse_functions(row['PossibleValues'], 'PossibleValues', obj_name, keyname)
                        if (row['PossibleValues'] is not None) and not isinstance(row['PossibleValues'], list):
                            row['PossibleValues'] = [row['PossibleValues']]

                        # Below is a hack(!!!) because a few PDF key values look like floats or keywords but are really names.
                        # Sly-based parsing in Python does not use any hints from other rows so it will convert to int/float/bool as it sees fit
                        # See Catalog::Version, DocMDPTransformParameters::V, DevExtensions::BaseVersion, SigFieldSeedValue::LockDocument
                        if (row['Type'][0] == 'name'):
                            if (row['DefaultValue'] is not None) and isinstance(row['DefaultValue'][0], (int, float)):
                                logging.info("Converting DefaultValue int/float/bool '%s' back to name for %s::%s", str(row['DefaultValue'][0]), obj_name, keyname)
                                row['DefaultValue'][0] = str(row['DefaultValue'][0])
                            if (row['PossibleValues'] is not None) and (row['PossibleValues'][0] is not None):
                                for i, v in enumerate(row['PossibleValues'][0]):
                                    if isinstance(v, (int, float)):
                                        logging.info("Converting PossibleValues int/float/bool '%s' back to name for %s::%s", str(v), obj_name, keyname)
                                        row['PossibleValues'][0][i] = str(v)

                        if (row['SpecialCase'] == ''):
                            row['SpecialCase'] = None
                        elif (r';' in row['SpecialCase']):
                            row['SpecialCase'] = self.__strip_square_brackets(re.split(r';', row['SpecialCase']))
                            for i, v in enumerate(row['SpecialCase']):
                                if (v is not None):
                                    row['SpecialCase'][i] = self._parse_functions(v, 'SpecialCase', obj_name, keyname)
                        else:
                            row['SpecialCase'] = self._parse_functions(row['SpecialCase'], 'SpecialCase', obj_name, keyname)
                        if (row['SpecialCase'] is not None) and not isinstance(row['SpecialCase'], list):
                            row['SpecialCase'] = [row['SpecialCase']]

                        if (row['Link'] == ''):
                            row['Link'] = None
                        else:
                            if (r';' in row['Link']):
                                row['Link'] = re.split(r';', row['Link'])
                                for i, v in enumerate(row['Link']):
                                    if (v == '[]'):
                                        row['Link'][i] = None
                                    else:
                                        row['Link'][i] = self._parse_functions(v, 'Link', obj_name, keyname)
                            else:
                                row['Link'] = self._parse_functions(row['Link'], 'Link', obj_name, keyname)
                        if (row['Link'] is not None) and not isinstance(row['Link'], list):
                            row['Link'] = [row['Link']]

                        if (row['Note'] == ''):
                            row['Note'] = None

                        tsvobj[keyname] = row
                    self.__pdfdom[obj_name] = tsvobj
                    csvfile.close()

            if (self.__validating):
                self.__validate_pdf_dom()

            if (self.__filecount == 0):
                logging.critical("There were no TSV files in directory '%s'!", self.__directory)
                return

            logging.info("%d TSV files processed from %s", self.__filecount, self.__directory)
            self.__validating = False
        except Exception as e:
            logging.critical("Exception: " + str(e))
            traceback.print_exception(type(e), e, e.__traceback__)

    def __validate_pdf_dom(self) -> None:
        """
        Does a detailed Validation of the in-memory Python data structure of the
        Arlington TSV data files.
        """
        if (self.__filecount == 0) or (len(self.__pdfdom) == 0):
            logging.critical("There is no Arlington DOM to validate!")
            return

        # logging.info("Validating against PDF version %s", self.__pdfver)
        for obj_name in self.__pdfdom:
            logging.debug("Validating '%s'", obj_name)
            obj = self.__pdfdom[obj_name]

            # Check if object contain any duplicate keys or has no keys
            if (len(obj) != len(set(obj))):
                logging.critical("Duplicate keys in '%s'!", obj_name)
            if (len(obj) == 0):
                logging.critical("Object '%s' has no keys/array entries!", obj_name)

            for keyname in obj:
                row = obj[keyname]
                logging.debug("Validating %s::%s", obj_name, keyname)

                # Check validity of key names and array indices
                m = re.search(r'^[a-zA-Z0-9_:\-\.]*\*?$', keyname)
                if (m is None):
                    logging.error("Key '%s' in object %s has unexpected characters", keyname, obj_name)

                # Check if Types are sorted alphabetically
                reduced_types = self.__reduce_typelist(row['Type'], [])
                is_sorted = all((reduced_types[i] <= reduced_types[i+1]) for i in range(len(reduced_types)-1))
                if not is_sorted:
                    logging.error("Types '%s' are not sorted alphabetically for %s::%s", row['Type'], obj_name, keyname)

                if isinstance(row['SinceVersion'], list):
                    for v in row['SinceVersion']:
                        if (v[0].type != 'FUNC_NAME') and ((v[0].value != 'fn:IsExtension(') or (v[0].value != 'fn:SinceVersion(')):
                            logging.error("SinceVersion predicate '%s' is not correct for %s::%s", v, obj_name, keyname)
                elif isinstance(row['SinceVersion'], float):
                    v = row['SinceVersion']
                    if (v != 1.0) and (v != 1.1) and (v != 1.2) and (v != 1.3) and (v != 1.4) and (v != 1.5) and (v != 1.6) and (v != 1.7) and (v != 2.0):
                        logging.error("SinceVersion '%s' in %s::%s has unexpected version!", row['SinceVersion'], obj_name, keyname)
                else:
                    logging.error("SinceVersion '%s' in %s::%s has unexpected value!", row['SinceVersion'], obj_name, keyname)

                if (row['DeprecatedIn'] is not None) and (row['DeprecatedIn'] not in self.__pdf_versions):
                    logging.error("DeprecatedIn '%s' in %s::%s has unexpected value!", row['DeprecatedIn'], obj_name, keyname)

                for v in row['Required']:
                    if isinstance(v, list):
                        if (v[0].type != 'FUNC_NAME') and (v[0].value != 'fn:IsRequired('):
                            logging.error("Required function '%s' does not start with 'fn:IsRequired' for %s::%s", row['Required'], obj_name, keyname)

                if (isinstance(row['IndirectReference'], list) and (len(row['IndirectReference']) > 1)):
                    if (len(row['Type']) != len(row['IndirectReference'])):
                        logging.error("Incorrect number of elements between Type (%d) and IndirectReference (%d) for %s::%s",
                            len(row['Type']), len(row['IndirectReference']), obj_name, keyname)

                i = self.__find_pdf_type('stream', row['Type'])
                if (i != -1):
                    if (row['IndirectReference'][i] is not True):
                        logging.error("Type 'stream' requires IndirectReference (%s) to be TRUE for %s::%s", row['IndirectReference'][i], obj_name, keyname)

                if not ((row['Inheritable'] is True) or (row['Inheritable'] is False)):
                    logging.error("Inheritable %s '%s' in %s::%s is not FALSE or TRUE!", type(row['Inheritable']), row['Inheritable'], obj_name, keyname)

                if (row['DefaultValue'] is not None):
                    if (len(row['Type']) != len(row['DefaultValue'])):
                        logging.error("Incorrect number of elements between Type and DefaultValue for %s::%s", obj_name, keyname)

                # Validate all types are known and match DefaultValue into PossibleValues
                for i, t in enumerate(row['Type']):
                    if isinstance(t, str):
                        if (t not in self.__known_types):
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
                                    if (row['DefaultValue'][i][0] != '\''):
                                        logging.error("DefaultValue '%s' does not start with '\'' for %s::%s", row['DefaultValue'][i], obj_name, keyname)
                                    elif (row['DefaultValue'][i][-1] != '\''):
                                        logging.error("DefaultValue '%s' does not end with '\'' for %s::%s", row['DefaultValue'][i], obj_name, keyname)

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
                                            if (row['PossibleValues'][i][j][0] != '\''):
                                                logging.error("PossibleValues '%s' does not start with '\'' for %s::%s", row['PossibleValues'][i][j], obj_name, keyname)
                                            elif (row['PossibleValues'][i][j][-1] != '\''):
                                                logging.error("PossibleValues '%s' does not end with '\'' for %s::%s", row['PossibleValues'][i][j], obj_name, keyname)
                                elif isinstance(row['PossibleValues'][i], str):
                                    if (row['PossibleValues'][i][0] != '\''):
                                        logging.error("PossibleValues '%s' does not start with '\'' for %s::%s", row['PossibleValues'][i], obj_name, keyname)
                                    elif (row['DefaultValue'][i][-1] != '\''):
                                        logging.error("PossibleValues '%s' does not end with '\'' for %s::%s", row['PossibleValues'][i], obj_name, keyname)
                                else:
                                    logging.error("PossibleValues '%s' is not a str for %s::%s", row['PossibleValues'][i], obj_name, keyname)

                        if (row['Link'] is not None):
                            if (t in self.__links_required):
                                if (row['Link'][i] is None):
                                    logging.error("Link '%s' is missing for type %s in %s::%s", row['Link'][i], t, obj_name, keyname)
                                elif not isinstance(row['Link'][i], (str, list)):
                                    logging.error("Link '%s' is not a list for type %s in %s::%s", row['Link'][i], t, obj_name, keyname)
                                else:
                                    if isinstance(row['Link'][i], str):
                                        lnk: str = row['Link'][i]
                                        lnkobj: Dict = self.__pdfdom[lnk]
                                        if (lnkobj is None):
                                            logging.error("Bad link '%s' in %s::%s", row['Link'][i], obj_name, keyname)
                                    else:   # list
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
                            if not isinstance(t[1][1], str) or (t[1][1] not in self.__known_types):
                                logging.error("Unknown type inside function '%s' for Type in %s::%s!", t, obj_name, keyname)
                        else:
                            # Only "fn:SinceVersion(" or "fn:Deprecated(" allowed
                            if (t[0][0].type != 'FUNC_NAME') and (t[0][0].value not in ("fn:SinceVersion(", "fn:Deprecated(")):
                                logging.error("Unknown function '%s' for Type in %s::%s!", t, obj_name, keyname)
                            if not isinstance(t[0][1][1], str) or (t[0][1][1] not in self.__known_types):
                                logging.error("Unknown type inside function '%s' for Type in %s::%s!", t, obj_name, keyname)

                    # Check if DefaultValue is valid in any PossibleValues
                    # T.B.D.

            # Check for incoming links to this object (obj_name) from every other object
            found: int = 0
            f: str
            for f in self.__pdfdom:
                lnkobj = self.__pdfdom[f]
                for k in lnkobj:
                    r = lnkobj[k]
                    if (r['Link'] is not None):
                        rd = self.__reduce_linkslist(r['Link'], [])
                        for v in rd:
                            if isinstance(v, str) and (v == obj_name):
                                found += 1
                                logging.debug("Found %s for %s::%s", obj_name, f, k)

            logging.debug("Found %d links to '%s'", found, obj_name)
            if (found == 0) and (obj_name != "FileTrailer") and (obj_name != "XRefStream"):
                logging.critical("Unlinked object %s!", obj_name)

    def process_matrix(self, mtx: pikepdf.Array, pth: str) -> None:
        """
        Process a matrix (6 element pikepdf.Array) object
        @param mtx: the pikepdf.Array matrix object
        @param pth: the text string of the path to the matrix
        """
        print("=" + pth + ("=[ %.5f %.5f %.5f %.5f %.5f %.5f ] <as matrix>" % (mtx[0], mtx[1], mtx[2], mtx[3], mtx[4], mtx[5])))

    def process_rect(self, rct: pikepdf.Array, pth: str) -> None:
        """
        Process a rectangle (4 element pikepdf.Array) object
        @param rct: the pikepdf.Array rectangle object
        @param pth: the text string of the path to the rectangle
        """
        print("=" + pth + ("=[ %.5f %.5f %.5f %.5f ] <as rectangle>" % (rct[0], rct[1], rct[2], rct[3])))

    def process_dict(self, dct: pikepdf.Object, arlnames: Optional[List[str]], pth: str) -> None:
        """
        Recursively process keys in a pikepdf.Dictionary object
        @param dct: a pikepdf.Dictionary object
        @param arlnames: list of possible Arlington TSV objects that might match the PDF dictionary
        @param pth: the text string of the path to the dict
        """
        rlinks: Optional[List[str]]
        arlobj: Optional[Any]
        if (arlnames is not None):
            rlinks = self.__reduce_linkslist(arlnames, [])
            arlobj = self.__pdfdom[rlinks[0]]
            wildcard = (r'*' in arlobj)
        else:
            rlinks = None
            arlobj = None
            wildcard = False

        for k in sorted(dct.as_dict()):
            row = None
            childlinks = None
            if (wildcard):
                row = arlobj[r'*']
                status = '='
            elif (arlobj is not None) and (k[1:] in arlobj):
                # Key 'k' is in both Arlington and PDF!
                # Remove leading slash from pikepdf Name to match Arlington Name
                row = arlobj[k[1:]]
                status = '?'
            else:
                # Key 'k' is ONLY in the PDF and not Arlington
                status = '+'

            idx: int
            p: str = pth + "%s" % k
            p1: str = ''
            o = dct.get(k)
            if isinstance(o, pikepdf.Dictionary):
                is_tree = False
                if (row is not None):
                    idx = self.__find_pdf_type(['dictionary', 'name-tree', 'number-tree'], row['Type'])
                    if (idx != -1):
                        status = "="
                        childlinks = self.__reduce_linkslist(row['Link'][idx], [])
                        is_tree = (row['Type'][idx] in ['name-tree', 'number-tree'])
                if (o.objgen != (0, 0)):
                    if (o.objgen in self.__visited):
                        print(status + p + (" ** already visited dict %s!" % str(o.objgen)))
                        continue
                    else:
                        self.__visited.append(o.objgen)
                        p1 = " %s" % str(o.objgen)
                if (not is_tree):
                    print(status + p + p1 + (" <as %s>" % childlinks))
                    self.process_dict(o, childlinks, p)
                else:
                    print(status + p + p1 + " <as name/number-tree>")
            elif isinstance(o, pikepdf.Stream):
                if (row is not None):
                    idx = self.__find_pdf_type('stream', row['Type'])
                    if (idx != -1):
                        status = "="
                        childlinks = self.__reduce_linkslist(row['Link'][idx], [])
                if (o.objgen != (0, 0)):
                    if (o.objgen in self.__visited):
                        print(status + p + (" ** already visited stm %s!" % str(o.objgen)))
                        continue
                    else:
                        self.__visited.append(o.objgen)
                        p1 = " %s" % str(o.objgen)
                print(status + p + p1 + (" <as %s>" % childlinks))
                self.process_stream(o, childlinks, p)
            elif isinstance(o, pikepdf.Array):
                is_matrix = False
                is_rect = False
                if (row is not None):
                    idx = self.__find_pdf_type(['array', 'matrix', 'rectangle'], row['Type'])
                    if (idx != -1):
                        # matrix and rectangle don't have links even though they are arrays
                        status = "="
                        if ('array' == row['Type'][idx]):
                            childlinks = self.__reduce_linkslist(row['Link'][idx], [])
                        elif ('matrix' == row['Type'][idx]):
                            is_matrix = True
                        if ('rectangle' == row['Type'][idx]):
                            is_rect = True
                if (o.objgen != (0, 0)):
                    if (o.objgen in self.__visited):
                        print(status + p + (" ** already visited array %s!" % str(o.objgen)))
                        continue
                    else:
                        self.__visited.append(o.objgen)
                        p1 = " %s" % str(o.objgen)
                if (is_matrix):
                    self.process_matrix(o, status + p + p1)
                elif (is_rect):
                    self.process_rect(o, status + p + p1)
                else:
                    print(status + p + p1 + (" <as %s>" % childlinks))
                    self.process_array(o, childlinks, p)
            elif isinstance(o, pikepdf.Name):
                if (row is not None):
                    idx = self.__find_pdf_type('name', row['Type'])
                    if (idx != -1):
                        status = "="
                print(status + p + ("=%s" % o))
            elif isinstance(o, (pikepdf.String, str)):
                if (row is not None):
                    idx = self.__find_pdf_type(['string', 'date'], row['Type'])
                    if (idx != -1):
                        status = "="
                print(status + p + ("=(%s)" % o))
            elif isinstance(o, bool):
                if (row is not None):
                    idx = self.__find_pdf_type(['boolean'], row['Type'])
                    if (idx != -1):
                        status = "="
                if (o):
                    print(status + p + "=true")
                else:
                    print(status + p + "=false")
            elif isinstance(o, int):
                s = str(o)
                if (row is not None):
                    idx = self.__find_pdf_type(['integer', 'number', 'bitmask'], row['Type'])
                    if (idx != -1):
                        status = "="
                        if ('number' == row['Type'][idx]):
                            s = "%.5f" % float(o)
                        elif ('bitmask' == row['Type'][idx]):
                            s = "%d <bitmask>" % o
                print(status + p + "=%s" % s)
            elif isinstance(o, (float, decimal.Decimal)):
                if (row is not None):
                    idx = self.__find_pdf_type('number', row['Type'])
                    if (idx != -1):
                        status = "="
                print(status + p + ("=%.5f" % o))
            elif (o is None):
                if (row is not None):
                    idx = self.__find_pdf_type('null', row['Type'])
                    if (idx != -1):
                        status = "="
                print(status + p + "=null")
            else:
                logging.critical("Unexpected type '%s' processing dictionary! ", o.__class__)
                sys.exit()

    def process_stream(self, dct: pikepdf.Stream, arlnames: Optional[List[str]], pth: str) -> None:
        """
        Recursively process keys in a pikepdf.Stream object
        @param dct: a pikepdf.Stream object
        @param arlnames: list of possible Arlington TSV objects that might match the PDF stream
        @param pth: the text string of the path to the stream
        """
        rlinks: Optional[List[str]]
        arlobj: Optional[Any]
        if (arlnames is not None):
            rlinks = self.__reduce_linkslist(arlnames, [])
            arlobj = self.__pdfdom[rlinks[0]]
            wildcard = (r'*' in arlobj)
        else:
            rlinks = None
            arlobj = None
            wildcard = False

        for k in sorted(dct.stream_dict.as_dict()):
            row = None
            childlinks = None
            if (wildcard):
                row = arlobj[r'*']
                status = '='
            elif (arlobj is not None) and (k[1:] in arlobj):
                # Key 'k' is in both Arlington and PDF!
                # Remove leading slash from pikepdf Name to match Arlington Name
                row = arlobj[k[1:]]
                status = '?'
            else:
                # Key 'k' is ONLY in the PDF and not Arlington
                status = '+'

            p = pth + "%s" % k
            p1 = ''
            o = dct.get(k)
            if isinstance(o, pikepdf.Dictionary):
                is_tree = False
                if (row is not None):
                    idx = self.__find_pdf_type(['dictionary', 'name-tree', 'number-tree'], row['Type'])
                    if (idx != -1):
                        status = "="
                        childlinks = self.__reduce_linkslist(row['Link'][idx], [])
                        is_tree = (row['Type'][idx] in ['name-tree', 'number-tree'])
                if (o.objgen != (0, 0)):
                    if (o.objgen in self.__visited):
                        print(status + p + (" ** already visited dict %s!" % str(o.objgen)))
                        continue
                    else:
                        self.__visited.append(o.objgen)
                        p1 = " %s" % str(o.objgen)
                if (not is_tree):
                    print(status + p + p1 + (" <as %s>" % childlinks))
                    self.process_dict(o, childlinks, p)
                else:
                    print(status + p + p1 + " <as name/number-tree>")
            elif isinstance(o, pikepdf.Stream):
                if (row is not None):
                    idx = self.__find_pdf_type('stream', row['Type'])
                    if (idx != -1):
                        status = "="
                        childlinks = self.__reduce_linkslist(row['Link'][idx], [])
                if (o.objgen != (0, 0)):
                    if (o.objgen in self.__visited):
                        print(status + p + (" ** already visited stm %s!" % str(o.objgen)))
                        continue
                    else:
                        self.__visited.append(o.objgen)
                        p1 = " %s" % str(o.objgen)
                print(status + p + p1 + (" <as %s>" % childlinks))
                self.process_stream(o, childlinks, p)
            elif isinstance(o, pikepdf.Array):
                is_matrix = False
                is_rect = False
                if (row is not None):
                    idx = self.__find_pdf_type(['array', 'matrix', 'rectangle'], row['Type'])
                    if (idx != -1):
                        # matrix and rectangle don't have links even though they are arrays
                        status = "="
                        if ('array' == row['Type'][idx]):
                            childlinks = self.__reduce_linkslist(row['Link'][idx], [])
                        elif ('matrix' == row['Type'][idx]):
                            is_matrix = True
                        if ('rectangle' == row['Type'][idx]):
                            is_rect = True
                if (o.objgen != (0, 0)):
                    if (o.objgen in self.__visited):
                        print(status + p + (" ** already visited array %s!" % str(o.objgen)))
                        continue
                    else:
                        self.__visited.append(o.objgen)
                        p1 = " %s" % str(o.objgen)
                if (is_matrix):
                    self.process_matrix(o, status + p + p1)
                elif (is_rect):
                    self.process_rect(o, status + p + p1)
                else:
                    print(status + p + p1 + (" <as %s>" % childlinks))
                    self.process_array(o, childlinks, p)
            elif isinstance(o, pikepdf.Name):
                if (row is not None):
                    idx = self.__find_pdf_type('name', row['Type'])
                    if (idx != -1):
                        status = "="
                print(status + p + ("=%s" % o))
            elif isinstance(o, (pikepdf.String, str)):
                if (row is not None):
                    idx = self.__find_pdf_type(['string', 'date'], row['Type'])
                    if (idx != -1):
                        status = "="
                print(status + p + ("=(%s)" % o))
            elif isinstance(o, bool):
                if (row is not None):
                    idx = self.__find_pdf_type(['boolean'], row['Type'])
                    if (idx != -1):
                        status = "="
                if (o):
                    print(status + p + "=true")
                else:
                    print(status + p + "=false")
            elif isinstance(o, int):
                s = str(o)
                if (row is not None):
                    idx = self.__find_pdf_type(['integer', 'number', 'bitmask'], row['Type'])
                    if (idx != -1):
                        status = "="
                        if ('number' == row['Type'][idx]):
                            s = "%.5f" % float(o)
                        elif ('bitmask' == row['Type'][idx]):
                            s = "%d <bitmask>" % o
                print(status + p + "=%s" % s)
            elif isinstance(o, (float, decimal.Decimal)):
                if (row is not None):
                    idx = self.__find_pdf_type('number', row['Type'])
                    if (idx != -1):
                        status = "="
                print(status + p + ("=%.5f" % o))
            elif (o is None):
                if (row is not None):
                    idx = self.__find_pdf_type('null', row['Type'])
                    if (idx != -1):
                        status = "="
                print(status + p + "=null")
            else:
                logging.critical("Unexpected type '%s' processing stream! ", o.__class__)
                sys.exit()

    def process_array(self, ary: pikepdf.Array, arlnames: Optional[List[str]], pth: str) -> None:
        """
        Recursively process array elements (by numeric index) in a pikepdf.Array object
        @param ary: a pikepdf.Array object
        @param arlnames: list of possible Arlington TSV objects that might match the PDF dictionary
        @param pth: the text string of the path to ary
        """
        wildcard: bool
        rlinks: Optional[List[str]]
        arlobj: Optional[Any]
        if (arlnames is not None):
            rlinks = self.__reduce_linkslist(arlnames, [])
            arlobj = self.__pdfdom[rlinks[0]]
            wildcard = (r'*' in arlobj)
        else:
            rlinks = None
            arlobj = None
            wildcard = False

        i: int
        o: pikepdf.Object
        status: str
        for i, o in enumerate(ary):
            row = None
            childlinks = None
            if (wildcard):
                row = arlobj[r'*']
                status = '='
            elif (arlobj is not None) and (i < len(arlobj)):
                row = arlobj[str(i)]
                status = '?'
            else:
                status = '+'

            p: str = pth + "[%d]" % i
            p1: str = ''
            if isinstance(o, pikepdf.Dictionary):
                is_tree = False
                if (row is not None):
                    idx = self.__find_pdf_type(['dictionary', 'name-tree', 'number-tree'], row['Type'])
                    if (idx != -1):
                        status = "="
                        childlinks = self.__reduce_linkslist(row['Link'][idx], [])
                        is_tree = (row['Type'][idx] in ['name-tree', 'number-tree'])
                if (o.objgen != (0, 0)):
                    if (o.objgen in self.__visited):
                        print(status + p + (" ** already visited dict %s!" % str(o.objgen)))
                        continue
                    else:
                        self.__visited.append(o.objgen)
                        p1 = " %s" % str(o.objgen)
                if (not is_tree):
                    print(status + p + p1 + (" <as %s>" % childlinks))
                    self.process_dict(o, childlinks, p)
                else:
                    print(status + p + p1 + " <as name/number-tree>")
            elif isinstance(o, pikepdf.Stream):
                if (row is not None):
                    idx = self.__find_pdf_type('stream', row['Type'])
                    if (idx != -1):
                        status = "="
                        childlinks = self.__reduce_linkslist(row['Link'][idx], [])
                if (o.objgen != (0, 0)):
                    if (o.objgen in self.__visited):
                        print(status + p + (" ** already visited stm %s!" % str(o.objgen)))
                        continue
                    else:
                        self.__visited.append(o.objgen)
                        p1 = " %s" % str(o.objgen)
                print(status + p + p1 + (" <as %s>" % childlinks))
                self.process_stream(o, childlinks, p)
            elif isinstance(o, pikepdf.Array):
                is_matrix = False
                is_rect = False
                if (row is not None):
                    idx = self.__find_pdf_type(['array', 'matrix', 'rectangle'], row['Type'])
                    if (idx != -1):
                        # matrix and rectangle don't have links even though they are technically arrays
                        status = "="
                        if ('array' == row['Type'][idx]):
                            childlinks = self.__reduce_linkslist(row['Link'][idx], [])
                        elif ('matrix' == row['Type'][idx]):
                            is_matrix = True
                        if ('rectangle' == row['Type'][idx]):
                            is_rect = True
                if (o.objgen != (0, 0)):
                    if (o.objgen in self.__visited):
                        print(status + p + (" ** already visited array %s!" % str(o.objgen)))
                        continue
                    else:
                        self.__visited.append(o.objgen)
                        p1 = " %s" % str(o.objgen)
                if (is_matrix):
                    self.process_matrix(o, status + p + p1)
                elif (is_rect):
                    self.process_rect(o, status + p + p1)
                else:
                    print(status + p + p1 + (" <as %s>" % childlinks))
                    self.process_array(o, childlinks, p)
            elif isinstance(o, pikepdf.Name):
                if (row is not None):
                    idx = self.__find_pdf_type('name', row['Type'])
                    if (idx != -1):
                        status = "="
                print(status + p + ("=%s" % o))
            elif isinstance(o, (pikepdf.String, str)):
                if (row is not None):
                    idx = self.__find_pdf_type(['string', 'date'], row['Type'])
                    if (idx != -1):
                        status = "="
                print(status + p + ("=(%s)" % o))
            elif isinstance(o, bool):
                if (row is not None):
                    idx = self.__find_pdf_type(['boolean'], row['Type'])
                    if (idx != -1):
                        status = "="
                if (o):
                    print(status + p + "=true")
                else:
                    print(status + p + "=false")
            elif isinstance(o, int):
                s = str(o)
                if (row is not None):
                    idx = self.__find_pdf_type(['integer', 'number', 'bitmask'], row['Type'])
                    if (idx != -1):
                        status = "="
                        if ('number' == row['Type'][idx]):
                            s = "%.5f" % float(o)
                        elif ('bitmask' == row['Type'][idx]):
                            s = "%d <bitmask>" % o
                print(status + p + "=%s" % s)
            elif isinstance(o, (float, decimal.Decimal)):
                if (row is not None):
                    idx = self.__find_pdf_type('number', row['Type'])
                    if (idx != -1):
                        status = "="
                print(status + p + "=%.5f" % o)
            elif (o is None):
                if (row is not None):
                    idx = self.__find_pdf_type('null', row['Type'])
                    if (idx != -1):
                        status = "="
                print(status + p + "=null")
            else:
                logging.critical("Unexpected type '%s' processing array! ", o.__class__)
                sys.exit()

    def validate_pdf_file(self, pdf_file: str) -> None:
        """
        Reads in a PDF file and compares to the Arlington DOM
        @param  pdf_file: filename of PDF file
        """
        pdf = pikepdf.Pdf.open(pdf_file, suppress_warnings=False)
        wrns = pdf.get_warnings()
        if (len(wrns) > 0):
            logging.debug(wrns)
        self.__visited: List[Any] = []

        # Simplistic method to determine of modern or legacy xref
        pdfobj = pdf.trailer.as_dict().get('/Type')
        if (pdfobj is not None):
            if (str(pdfobj) == '/XRef'):
                print("Processing as XRefStream")
                self.process_dict(pdf.trailer, ['XRefStream'], "/trailer")
        else:
            print("Processing as file trailer")
            self.process_dict(pdf.trailer, ['FileTrailer'], "/trailer")
        pdf.close()

    def save_dom_to_pretty_file(self, filenm: str) -> None:
        """
        Pretty-print the DOM to a file (helps readability for debugging)
        This file is NOT JSON and cannot be processed by jq or other JSON aware tools!
        @param: filenm: file name. Will be overwritten.
        """
        with open(filenm, r'w') as f:
            pprint.pprint(self.__pdfdom, f, compact=False, width=200)
            f.close()

    def save_dom_to_json(self, filenm: str) -> None:
        """
        Save DOM to a JSON file
        @param: filenm: file name. Will be overwritten.
        """
        with open(filenm, r'w') as f:
            json.dump(self.__pdfdom, f, indent=4, sort_keys=True, default=sly_lex_Token_to_json)
            f.close()


if __name__ == '__main__':
    cli_parser = argparse.ArgumentParser()
    cli_parser.add_argument('-t', '--tsvdir', help='folder containing Arlington TSV file set', dest="tsvdir")
    cli_parser.add_argument('-s', '--save', help="save pretty Arlington model to a file (Python pretty print)", default=None, dest="save")
    cli_parser.add_argument('-j', '--json', help="save Arlington model to JSON", default=None, dest="json")
    cli_parser.add_argument('-v', '--validate', help="validate the Arlington model", action='store_true', default=False, dest="validate")
    cli_parser.add_argument('-d', '--debug', help="enable debug logging (verbose!)", action="store_const", dest="loglevel", const=logging.DEBUG, default=logging.WARNING)
    cli_parser.add_argument('-i', '--info', help="enable informative logging", action="store_const", dest="loglevel", const=logging.INFO)
    cli_parser.add_argument('-p', '--pdf', help="input PDF file", default=None, dest="pdffile")
    cli_parser.add_argument('-o', '--out', help="output directory", default=".", dest="outdir")
    cli = cli_parser.parse_args()

    logging.basicConfig(level=cli.loglevel)

    if (cli.tsvdir is None) or not os.path.isdir(cli.tsvdir):
        print("'%s' is not a valid directory" % cli.tsvdir)
        cli_parser.print_help()
        sys.exit()

    if (cli.validate):
        print("Loading and validating...")
    else:
        print("Loading...")
    arl = Arlington(cli.tsvdir, validating=cli.validate)

    if (cli.save is not None):
        print("Saving pretty Python data to '%s'..." % cli.save)
        arl.save_dom_to_pretty_file(cli.save)

    if (cli.json is not None):
        print("Saving JSON to '%s'..." % cli.json)
        arl.save_dom_to_json(cli.json)

    if (cli.pdffile is not None):
        if os.path.isfile(cli.pdffile):
            print("Processing '%s'..." % cli.pdffile)
            arl.validate_pdf_file(cli.pdffile)
        elif os.path.isdir(cli.pdffile):
            print("Processing directory '%s'..." % cli.pdffile)
            for pdf in glob.iglob(os.path.join(cli.pdffile, r"*.pdf")):
                outf = os.path.join(os.path.normpath(cli.outdir), os.path.splitext(os.path.basename(pdf))[0])
                while (os.path.isfile(outf + ".txt")):
                    outf = outf + "_"
                outf = outf + ".txt"
                print("Processing '%s' to '%s'..." % (pdf, outf))
                try:
                    oldstdout = sys.stdout
                    oldstderr = sys.stderr
                    sys.stdout = sys.stderr = open(outf, "w")
                    arl.validate_pdf_file(pdf)
                except Exception as e:
                    print("Exception: " + str(e))
                    traceback.print_exception(type(e), e, e.__traceback__)
                finally:
                    sys.stdout.close()
                    sys.stdout = oldstdout
                    sys.stderr = oldstderr
        else:
            print("'%s' is not a valid file or directory!" % cli.pdffile)
            sys.exit()

    print("Done.")
