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

/// @brief All Arlington pre-defined types (alphabetically sorted vector)
const std::vector<std::string>  v_ArlAllTypes = {
        "array",
        "bitmask",
        "boolean",
        "date",
        "dictionary",
        "integer",
        "matrix",
        "name",
        "name-tree",
        "null",
        "number",
        "number-tree",
        "rectangle",
        "stream",
        "string",
        "string-ascii",
        "string-byte",
        "string-text"
};

/// @brief Arlingon pre-defined types which REQUIRE a Link - aka "Complex types" (alphabetically sorted vector)
const std::vector<std::string>  v_ArlComplexTypes = {
    "array",
    "dictionary",
    "name-tree",
    "number-tree",
    "stream"
};

/// @brief Arlington pre-defined types that must NOT have Links - aka "Non-complex types" (alphabetically sorted vector)
const std::vector<std::string>  v_ArlNonComplexTypes = {
    "bitmask",
    "boolean",
    "date",
    "integer",
    "matrix",
    "name",
    "null",
    "number",
    "rectangle",
    "string",
    "string-ascii",
    "string-byte",
    "string-text"
};


// @brief full set of Arlington supported PDF versions (numerically sorted vector)
const std::vector<std::string>  v_ArlPDFVersions = {
    "1.0",
    "1.1",
    "1.2",
    "1.3",
    "1.4",
    "1.5",
    "1.6",
    "1.7",
    "2.0"
};


/// @brief Arlington Integer - only optional leading negative sign
const std::string ArlInt = "(\\-)?[0-9]+";

/// @brief Arlington Number (requires at least 1 decimal place either side of decimal point ".")
const std::string ArlNum = ArlInt + "\\.[0-9]+";

/// @brief Arlington PDF Strings use single quotes (to disambiguate from bracketed names, keys, etc.). 
/// Empty strings are invalid (same as empty field in Arlington).
const std::string ArlString = "'[^']+'";

/// @brief Arlington key or array index regex, including path separator "::" and wildcards
/// Examples: SomeKey, 3, *, 2*, parent::SomeKey, SomeKeyA::SomeKeyB::3, SomeKeyA::SomeKeyB::@SomeKeyC,
const std::string  ArlKeyBase = "[a-zA-Z0-9_\\.]+";
const std::string  ArlKey = "([a-zA-Z]+\\:\\:)*(" + ArlKeyBase + "|[0-9]+(\\*)?|\\*)+";
const std::string  ArlKeyValue = "([a-zA-Z]+\\:\\:)*@(" + ArlKeyBase + "|[0-9]+(\\*)?|\\*)+";

/// @brief Arlington PDF version regex (1.0, 1.1, ... 1.7, 2.0)
const std::string  ArlPDFVersion = "(1\\.[0-7]|2\\.0)";

/// @brief pre-defined Arlington Types (all lowercase with some sub-types include DASH and a qualifier)
const std::string ArlPredfinedType = "(array|bitmask|boolean|date|dictionary|integer|matrix|name|name-tree|null|number-tree|number|rectangle|stream|string-ascii|string-byte|string-text|string)";

/// @brief Arlington Link name (i.e. TSV filename without extension). Only UNDERBAR, never DASH or PERIOD.
const std::string ArlLink = "[a-zA-Z0-9_]+";

/// @brief Arlington math comparisons - currently NOT required to have SPACE either side
const std::string ArlMathComp = "(==|!=|>=|<=|>|<)";

/// @brief Arlington math operators - MULTIPLY and MINUS need a SPACE either side to disambiguate from wildcards and negative numbers
const std::string ArlMathOp = "( \\* |\\+| \\- | mod )";

/// @brief Arlington logical operators. Require SPACE either side. Also expect bracketed expressions either side or a predicate:
/// e.g. "...) || (..." or "...) || fn:..."  
const std::string ArlLogicalOp = "( && | \\|\\| )";

/// @brief Arlington PDF boolean keywords
const std::string ArlBooleans = "(true|false)";

/// @brief Arlington predicates with zero or one parameter
const std::string ArlPredicate0Arg  = "fn:[A-Z][a-zA-Z14]+\\(\\)";
const std::string ArlPredicate1Arg  = "fn:[A-Z][a-zA-Z14]+\\(" + ArlKey + "\\)";
const std::string ArlPredicate1ArgV = "fn:[A-Z][a-zA-Z14]+\\(" + ArlKeyValue + "\\)";


/// @brief #define ARL_PARSER_TESTING to test a small set of hard coded predicates
//#define ARL_PARSER_TESTING


/// @brief #define ARL_PARSER_DEBUG to enable very verbose debugging of predicate and expression parsing
//#define ARL_PARSER_DEBUG


/// @brief Predicate parser creates a binary tree of these simple ASTNodes
struct ASTNode {
    /// @brief predicate operator or operand
    std::string     node;            
    /// @brief Optional arguments for operators
    ASTNode         *arg[2];         

    ASTNode(ASTNode *p = nullptr)
        { /* constructor */ arg[0] = arg[1] = nullptr; }

    ~ASTNode() { 
        /* destructor - recursive */
        if (arg[0] != nullptr) delete arg[0];
        if (arg[1] != nullptr) delete arg[1];
    }

    /// @brief assignment operator =
    ASTNode& operator=(const ASTNode& n) {
        node   = n.node;
        arg[0] = n.arg[0];
        arg[1] = n.arg[1];
        return *this;
    }

    /// @brief output operator <<
    friend std::ostream& operator<<(std::ostream& ofs, const ASTNode& n) {
        if (!n.node.empty())
            ofs << "{'" << n.node << "'";
        else
            ofs << "{''";

        if ((n.arg[0] != nullptr) && (n.arg[1] != nullptr))
            ofs << ",[" << *n.arg[0] << "],[" << *n.arg[1] << "]";
        else {
            if (n.arg[0] != nullptr)
                ofs << ",[" << * n.arg[0] << "]";
            else if (n.arg[1] != nullptr) // why is arg[0] nullptr when arg[1] isn't???
                ofs << ",???,[" << *n.arg[1] << "]";
        }
        ofs << "}";
        return ofs;
    }

    /// @brief  Validate if an AST node is correctly configured. Can only be done AFTER a full parse is completed.
    /// @return true if valid. false if the node is incorrect or partially populated.
    bool valid() {
        bool ret_val = !node.empty();
        if (ret_val && arg[0] != nullptr) {
            ret_val = ret_val && arg[0]->valid();
            if (ret_val && arg[1] != nullptr) 
                ret_val = ret_val && arg[1]->valid();
        }
        else if (arg[1] != nullptr)
            ret_val = false;
        return ret_val;
    }
};


/// @brief A stack of AST-Nodes
typedef std::vector<ASTNode*>  ASTNodeStack;


/// @brief Left-to-right recursive descent parser, based on regex pattern matching
/// 
/// @param[in] s    input string to parse
/// @param root     the root node of AST that is being constructed
/// 
/// @return         the remainder of the string that is being parsed.  
std::string LRParsePredicate(std::string s, ASTNode *root);


class PredicateProcessor {
protected:
    /// @brief Single field from a row in an Arlington TSV grammar file
    std::string     tsv_field;
    ASTNode         *ast;
public:
    PredicateProcessor(std::string s) : 
        tsv_field(s)
        { /* constructor */ ast = new ASTNode(); };
    ~PredicateProcessor()
        { /* destructor */ delete ast; };
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


/// @brief "IndirectReference" column has 3 possible outcomes
enum class ReferenceType { MustBeDirect, MustBeIndirect, DontCare };

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
    ReferenceType ReduceRow(const int type_index);
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
/// - '[' and ']' also used for PDF arrays
class DefaultValuePredicateProcessor : public PredicateProcessor {
public:
    DefaultValuePredicateProcessor(std::string s) :
        PredicateProcessor(s)
        { /* constructor */ };
    virtual bool ValidateRowSyntax();
};


/// @brief Implements predicate support for the Arlington "PossibleValues" field (column 9)
/// - can be [];[];[]
/// - complex expressions
/// - '[' and ']' also used for sets and PDF arrays
class PossibleValuesPredicateProcessor : public PredicateProcessor {
public:
    PossibleValuesPredicateProcessor(std::string s) :
        PredicateProcessor(s)
        { /* constructor */ };
    virtual bool ValidateRowSyntax();
};



/// @brief Implements predicate support for the Arlington "SpecialCase" field (column 10)
/// - inconsistent [] or no [] at all
/// - NO SEMI-COLONS
/// - complex expressions
class SpecialCasePredicateProcessor : public PredicateProcessor {
public:
    SpecialCasePredicateProcessor(std::string s) :
        PredicateProcessor(s)
        { /* constructor */ };
    virtual bool ValidateRowSyntax();
};


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
