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
# Normal usage with Arlington PDF model (Linux CLI):
#  $ echo "fn:Eval(@key,-27,1.23,'string',path::key1,((expr1==expr2) && (a!=b)" || (4>3))) | python3 arlington-fn-lex.py
#
#  $ grep --color=never -Pho "fn:[a-zA-Z0-9]+\([^\t\]\;]*\)" ../tsv/latest/*.tsv | sort | uniq | python3 arlington-fn-lex.py
#    --> expected outcome: no errors or crashes means that the predicates in Arlington are likely correct and valid
#
#
# Requires Python 3 and sly:
# - pip3 install sly
# - See https://sly.readthedocs.io/en/latest/sly.html
#
# Python QA:
# - flake8 --ignore E501,E221,E226 arlington-fn-lex.py
# - pyflakes arlington-fn-lex.py
# - mypy arlington-fn-lex.py
#
import fileinput
import pprint
from typing import Any, Dict, List, Optional, Tuple, Union
import sly        # type: ignore


##############################################################################################
class ArlingtonFnLexer(sly.Lexer):
    """
    Sly parser
    """

    # debugfile = 'parser.out'

    # Set of token names.   This is always required.
    tokens = {
        FUNC_NAME, KEY_VALUE, KEY_NAME, KEY_PATH, MOD, PDF_PATH, EQ, NE, GE, LE, GT, LT,
        LOGICAL_AND, LOGICAL_OR, REAL, INTEGER, PLUS, MINUS, TIMES, DIVIDE, LPAREN,
        RPAREN, COMMA, ARRAY_START, ARRAY_END, ELLIPSIS, PDF_TRUE, PDF_FALSE, PDF_STRING
    }

    # Precedence rules
    precedence = (
        ('nonassoc', EQ, NE, LE, GE, LT, GT),  # Non-associative operators
        ('left', PLUS, MINUS),
        ('left', TIMES, DIVIDE, MOD),
        ('left', LOGICAL_AND, LOGICAL_OR)
    )

    # String containing ignored characters between tokens (just SPACE)
    # Because we are reading from stdin and/or text files, also ignore EOLs
    ignore = ' '

    # Regular expression rules for tokens
    FUNC_NAME    = r'fn\:[A-Z][a-zA-Z0-9]+\('
    PDF_TRUE     = r'(true)|(TRUE)'
    PDF_FALSE    = r'(false)|(FALSE)'
    PDF_STRING   = r'\'[^\']+\''
    MOD          = r'mod'
    ELLIPSIS     = r'\.\.\.'
    KEY_VALUE    = r'@(\*|[0-9]+|[0-9]+\*|[a-zA-Z0-9_\.\-]+)'
    # Key name is both a PDF name or a TSV filename
    # Key name of just '*' is ambiguous TIMES (multiply) operator.
    # Key name which is numeric array index (0-9*) is ambiguous with integers.
    # Array indices are integers, or integer + ASTERISK (wildcard) - need to use SPACEs to disambiguate
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


def ToNestedAST(stk: List[sly.lex.Token], idx: int = 0) -> Tuple[int, List[sly.lex.Token]]:
    """
    Assumes a fully valid parse tree with fully bracketed "( .. )" expressions.
    Recursive.

    @param[in]  stk: stack of sly.lex.Tokens
    @param[in]  idx: token index into stk

    @returns:  index to next token in token stack, AST
    """
    ast: List[sly.lex.Token] = []
    i: int = idx

    while (i < len(stk)):
        if (stk[i].type == 'FUNC_NAME'):
            ast.append(stk[i])
            j, k = ToNestedAST(stk, i+1)
            ast.append(k)
            i = j
        elif (stk[i].type == 'LPAREN'):
            j, k = ToNestedAST(stk, i+1)
            ast.append(k)
            i = j
        elif (stk[i].type == 'RPAREN'):
            # go up recursion 1 level
            return i+1, ast
        elif (stk[i].type == 'COMMA'):
            # skip COMMA
            i = i + 1
        else:
            ast.append(stk[i])
            i = i + 1
    return i, ast


if __name__ == '__main__':
    # "Monkey patch" sly.lex.Token __str__ and __repr__ dunder methods to make JSON nicer
    # Don't do this if we want to read the JSON back in!
    old_str = sly.lex.Token.__str__
    sly.lex.Token.__str__  = MyTokenStr
    old_repr = sly.lex.Token.__repr__
    sly.lex.Token.__repr__ = MyTokenStr

    lexer  = ArlingtonFnLexer()

    i: int
    j: int
    line: str
    tok: sly.lex.Token
    ast: List[sly.lex.Token]

    for line in fileinput.input():
        # Skip blank lines and those starting with '#' (comments)
        line = line.strip()
        if (line != '') and (line[0] != '#'):
            stk: List[sly.lex.Token] = []
            print(line, end='')
            for tok in lexer.tokenize(line):
                # print('type=%20r,  value=%26r' % (tok.type, tok.value))
                stk.append(tok)
            print()
            i, ast = ToNestedAST(stk)
            #pprint.pprint(ast)
            for j, a in enumerate(ast):
                # De-tokenize only the top level PDF keynames
                if (not isinstance(a, list)) and (a.type == 'KEY_NAME'):
                    ast[j] = a.value
            pprint.pprint(ast)
            print()
