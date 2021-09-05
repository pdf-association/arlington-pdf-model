///////////////////////////////////////////////////////////////////////////////
// PredicateProcessor.cpp
// Copyright 2021 PDF Association, Inc. https://www.pdfa.org
//
// This material is based upon work supported by the Defense Advanced
// Research Projects Agency (DARPA) under Contract No. HR001119C0079.
// Any opinions, findings and conclusions or recommendations expressed
// in this material are those of the author(s) and do not necessarily
// reflect the views of the Defense Advanced Research Projects Agency
// (DARPA). Approved for public release.
//
// SPDX-License-Identifier: Apache-2.0
// Contributors: Peter Wyatt, PDF Association
// 
///////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef PredicateProcessor_h
#define PredicateProcessor_h

#include <iostream>
#include "ArlingtonTSVGrammarFile.h"
#include "ArlingtonPDFShim.h"

using namespace ArlingtonPDFShim;

/// @brief Integer - only optional leading negative sign
const std::string ArlInt = "(\\-)?[0-9]+";

/// @brief Number (requires at least 1 decimal place either side of decimal point ".")
const std::string ArlNum = ArlInt + "\\.[0-9]+";

/// @brief Strings use doubled quotes (to disambiguate from bracketed names, keys, etc.). Empty strings are invalid
const std::string ArlString = "'[^']+'";

/// @brief Arlington key / array index regex, including path separator "::" and wildcards
/// Examples: SomeKey, 3, *, 0*, parent::SomeKey, SomeKeyA::SomeKeyB::3, SomeKeyA::SomeKeyB::@SomeKeyC,
const std::string  ArlKeyBase = "[a-zA-Z0-9_\\.]+";
const std::string  ArlKey = "([a-zA-Z]+\\:\\:)*(" + ArlKeyBase + "|[0-9]+(\\*)?|\\*)+";
const std::string  ArlKeyValue = "([a-zA-Z]+\\:\\:)*@(" + ArlKeyBase + "|[0-9]+(\\*)?|\\*)+";

/// @brief Arlington PDF version regex (1.0, 1.1, ... 1.7, 2.0)
const std::string  ArlPDFVersion = "(1\\.[0-7]|2\\.0)";

/// @brief Arlington Type or Link (TSV filename)
const std::string ArlTypeOrLink = "[a-zA-Z0-9_\\-]+";

/// @brief Arlington math comparisons - currently NOT required to have SPACE either side
const std::string ArlMathComp = "(==|!=|>=|<=|>|<)";

/// @brief Arlington math operators - MULTIPLY and MINUS need a SPACE either side to disambiguate from wildcards and numbers
/// "mod" handled explicitly.
const std::string ArlMathOp = "( \\* |\\+| \\- | mod )";

/// @brief Arlington logical operators. Require SPACE either side. Also expect bracketed expressions either side or a predicate:
/// e.g. ...) || (... or ...) || fn:...  
const std::string ArlLogicalOp = "( && | \\|\\| )";

/// @brief Arlington PDF boolean keywords
const std::string ArlBooleans = "(true|false)";

/// @brief Arlington predicate with zero or one parameter
const std::string ArlPredicate0Arg = "fn:[a-zA-Z14]+\\(\\)";
const std::string ArlPredicate1Arg = "fn:[a-zA-Z14]+\\(" + ArlKey + "\\)";
const std::string ArlPredicate1ArgV = "fn:[a-zA-Z14]+\\(" + ArlKeyValue + "\\)";

bool ValidationByConsumption(const std::string& tsv_file, const std::string& fn, std::ostream& ofs);

/// @brief #define ARL_PARSER_DEBUG to enable verbose debugging of predicate and expression parsing
#define ARL_PARSER_DEBUG

struct ASTNode {
    std::string     node;            // predicate including opening bracket '('
    ASTNode         *arg[2];         // optional arguments
#ifdef ARL_PARSER_DEBUG
    ASTNode*        parent;          // parent pointer makes for easier debugging
#endif  // ARL_PARSER_DEBUG

    ASTNode(ASTNode *p = nullptr)
#ifdef ARL_PARSER_DEBUG
        : parent(p)
#endif  // ARL_PARSER_DEBUG
        { /* constructor */ arg[0] = arg[1] = nullptr; }

    ~ASTNode() { 
        /* destructor */
        // don't delete parent!!
        if (arg[0] != nullptr) delete arg[0];
        if (arg[1] != nullptr) delete arg[1];
    }

    /// @brief assignment operator =
    ASTNode& operator=(const ASTNode& n) {
        // don't copy parent!!
        node   = n.node;
        arg[0] = n.arg[0];
        arg[1] = n.arg[1];
        return *this;
    }

    /// @brief output operator <<
    friend std::ostream& operator<<(std::ostream& ofs, const ASTNode& n) {
        if (!n.node.empty())
            ofs << "{ '" << n.node << "'";
        else
            ofs << "{ ''";
      
        if ((n.arg[0] != nullptr) && (n.arg[1] != nullptr))
            ofs << ",[" << *n.arg[0] << "],[" << *n.arg[1] << "]";
        else {
            if (n.arg[0] != nullptr)
                ofs << ",[" << * n.arg[0] << "]";
            else if (n.arg[1] != nullptr)
                ofs << ",???,[" << *n.arg[1] << "]";
        }
        ofs << " }";
        return ofs;
    }
};

/// @brief A stack of AST-Nodes
typedef std::vector<ASTNode*>  ASTNodeStack;



std::string LRParsePredicate(std::string s, ASTNode *root);


class PredicateProcessor {
protected:
    /// @brief Single field from a row in an Arlington TSV grammar file
    std::string     tsv_field;
public:
    PredicateProcessor(std::string s) : 
        tsv_field(s)
        { /* constructor */ };
    virtual bool ValidateRowSyntax() = 0;
};


/// @brief Implements predicate support for the Arlington "Key" field (column 1) 
/// - No COMMAs or SEMI-COLONs
/// - any alphanumeric or "." or "-" or "_"
/// - any integer (array index)
/// - wildcard "*" - must be last row
/// - all rows are integer + "*" for a repeating set of N array elements
class KeyPredicateProcessor : public PredicateProcessor {
public:
    KeyPredicateProcessor(std::string s) :
        PredicateProcessor(s)
        { /* constructor */ };
    virtual bool ValidateRowSyntax();
};


/// @brief Implements predicate support for the Arlington "Type" field (column 2) 
/// - SEMI-COLON separated, alphabetically sorted, but no [ ] brackets
/// - fn:SinceVersion(x.y,type)
/// - fn:Deprecated(x.y,type)
/// - fn:BeforeVersion(x.y,type)
/// - fn:IsPDFVersion(x.y,type)
class TypePredicateProcessor : public PredicateProcessor {
public:
    TypePredicateProcessor(std::string s) : 
        PredicateProcessor(s)
        { /* constructor */ };
    virtual bool ValidateRowSyntax();
    std::string ReduceRow(const std::string pdf_version);
};


/// @brief Implements predicate support for the Arlington "SinceVersion" field (column 3) 
/// - only "1.0" or "1.1" or ... or "1.7 or "2.0"
class SinceVersionPredicateProcessor : public PredicateProcessor {
public:
    SinceVersionPredicateProcessor(std::string s) :
        PredicateProcessor(s)
        { /* constructor */ };
    virtual bool ValidateRowSyntax();
    bool ReduceRow(const std::string pdf_version);
};


/// @brief Implements predicate support for the Arlington "DeprecatedIn" field (column 4) 
/// - only blank ("") or "1.0" or "1.1" or ... or "1.7 or "2.0"
class DeprecatedInPredicateProcessor : public PredicateProcessor {
public:
    DeprecatedInPredicateProcessor(std::string s) :
        PredicateProcessor(s)
        { /* constructor */ };
    virtual bool ValidateRowSyntax();
    bool ReduceRow(const std::string pdf_version);
};


/// @brief Implements predicate support for the Arlington "Required" field (column 5) 
/// - either TRUE, FALSE or fn:IsRequired(...)
/// - inner can be very flexible, including logical " && " and " || " expressions:
///   . fn:BeforeVersion(x.y), fn:IsPDFVersion(x.y)
///   . fn:IsPresent(key) or fn:NotPresent(key)
///   . @key==... or @key!=...
///   . use of Arlington-PDF-Path "::", "parent::"
///   . various highly specialized predicates: fn:IsEncryptedWrapper(), fn:NotStandard14Font(), ...
class RequiredPredicateProcessor : public PredicateProcessor {
public:
    RequiredPredicateProcessor(std::string s) :
        PredicateProcessor(s)
        { /* constructor */ };
    virtual bool ValidateRowSyntax();
    bool ReduceRow(const std::string pdf_version, ArlPDFObject* obj);
};


/// @brief Implements predicate support for the Arlington "IndirectReference" field (column 6) 
/// - [];[];[]
/// - fn:MustBeDirect()
/// - fn:MustBeDirect(fn:IsPresent(key))
class IndirectRefPredicateProcessor : public PredicateProcessor {
public:
    IndirectRefPredicateProcessor(std::string s) :
        PredicateProcessor(s)
        { /* constructor */ };
    virtual bool ValidateRowSyntax();
};


/// @brief Implements predicate support for the Arlington "Inheritable" field (column 7)
/// -- TRUE or FALSE only
class InheritablePredicateProcessor : public PredicateProcessor {
public:
    InheritablePredicateProcessor(std::string s) :
        PredicateProcessor(s)
        { /* constructor */ };
    virtual bool ValidateRowSyntax();
    bool ReduceRow();
};


/// @brief Implements predicate support for the Arlington "DefaultValue" field (column 8)
/// - can be [];[];[]
/// - constants 
/// 


/// @brief Implements predicate support for the Arlington "PossibleValues" field (column 9)
/// - can be [];[];[]
/// - complex expressions


/// @brief Implements predicate support for the Arlington "SpecialCase" field (column 10)
/// - inconsistent [] or no [] at all
/// - NO SEMI-COLONS
/// - complex expressions


/// @brief Implements predicate support for the Arlington "Links" field (column 11) 
/// - [];[];[]
/// - fn:SinceVersion(x.y,type)
/// - fn:Deprecated(x.y,type)
/// - fn:BeforeVersion(x.y,type)
/// - fn:IsPDFVersion(x.y,type)
class LinkPredicateProcessor : public PredicateProcessor {
public:
    LinkPredicateProcessor(std::string s) :
        PredicateProcessor(s)
        { /* constructor */ };
    virtual bool ValidateRowSyntax();
    std::string LinkPredicateProcessor::ReduceRow(const std::string pdf_version);
};

/// Arlington "Notes" field (column 12) 
/// - free form text so no support required

#endif // PredicateProcessor_h
