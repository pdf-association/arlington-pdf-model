#!/usr/bin/python3
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
# Syntactically validate all declarative functions in the Arlington PDF model.
#
# Usage: arlington-fn-lex.py [- | files ]
#
# Lexical errors (such as typos) will result in a Python exception and premature failure.
#
# Normal usage with Arlington PDF model:
#  grep --color=never -Pho "fn:[[:alnum:]]*\([^\t\]\;]*" *.tsv | sort | uniq | arlington-fn-lex.py > out.txt 2>&1
#
# Requires Python 3 and sly: pip3 install sly
# See https://sly.readthedocs.io/en/latest/sly.html
#
import fileinput
from sly import Lexer

##############################################################################################
class ArlingtonFnLexer(Lexer):
    #debugfile = 'parser.out'

    # Set of token names.   This is always required.
    tokens = {
        FUNC_NAME,    KEY_VALUE,       KEY_NAME,     KEY_PATH,
        MOD,          PDF_PATH,     PDF_FALSE,       PDF_TRUE,
        EQUALITY,     INEQUALITY,   GREATER_THAN_EQ, LESS_THAN_EQ,
        LOGICAL_AND,  LOGICAL_OR,   GREATER_THAN,    LESS_THAN,
        REAL,         INTEGER,      PLUS,            MINUS,
        TIMES,        DIVIDE,       LPAREN,          RPAREN,
        COMMA
    }

    # Precedence rules
    precedence = (
          ('nonassoc', EQUALITY, INEQUALITY, LESS_THAN_EQ, GREATER_THAN_EQ, LESS_THAN, GREATER_THAN),  # Non-associative operators
          ('left', PLUS, MINUS),
          ('left', TIMES, DIVIDE, MOD),
          ('left', LOGICAL_AND, LOGICAL_OR)
    )

    # String containing ignored characters between tokens (just SPACE)
    # Because we are reading from stdin and/or text files, also ignore EOLs
    ignore = ' \r\n'

    # Regular expression rules for tokens
    FUNC_NAME    = r'fn\:[A-Z][a-zA-Z0-9]+\('
    PDF_FALSE    = r'false'
    PDF_TRUE     = r'true'
    MOD          = r'mod'
    KEY_VALUE    = r'@(\*|[0-9]+|[0-9]+\*|[a-zA-Z0-9_\.\-]+)'
    # Key name of just '*' is ambiguous TIMES (multiply) operator.
    # Key name which is numeric array index (0-9*) is ambiguous with integers.
    # Array indices are integers, or integer followed by ASTERISK (wildcard) - need to use SPACEs to disambiguate
    KEY_PATH     = r'(parent::)?(([a-zA-Z]|[a-zA-Z][0-9]*|[0-9]*\*|[0-9]*[a-zA-Z])[a-zA-Z0-9_\.\-]*::)+'
    KEY_NAME     = r'([a-zA-Z]|[a-zA-Z][0-9]*|[0-9]*\*|[0-9]*[a-zA-Z])[a-zA-Z0-9_\.\-]*'
    PDF_PATH     = r'::'
    EQUALITY     = r'=='
    INEQUALITY   = r'!='
    GREATER_THAN_EQ = r'>='
    LESS_THAN_EQ = r'<='
    LOGICAL_AND  = r'\&\&'
    LOGICAL_OR   = r'\|\|'
    GREATER_THAN = r'>'
    LESS_THAN    = r'<'
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

if __name__ == '__main__':
    lexer  = ArlingtonFnLexer()

    for line in fileinput.input():
        print(line, end='')
        for tok in lexer.tokenize(line):
            #print(tok)
            print('type=%20r,  value=%26r' % (tok.type, tok.value))
        print()
