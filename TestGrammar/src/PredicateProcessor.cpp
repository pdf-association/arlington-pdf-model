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

#include <exception>
#include <iterator>
#include <algorithm>
#include <regex>
#include <string>
#include <cassert>
#include <math.h>

#include "PredicateProcessor.h"
#include "utils.h"


/// @brief Regex to process "Links" and "Types" fields
/// - $1 = predicate name
/// - $2 = single Link (TSV filename) or single Arlington predefined Type 
static const std::regex  r_Links("fn:(SinceVersion|Deprecated|BeforeVersion|IsPDFVersion)\\(" + ArlPDFVersion + "\\,([a-zA-Z0-9_.]+)\\)");
static const std::regex  r_Types("fn:(SinceVersion|Deprecated|BeforeVersion|IsPDFVersion)\\(" + ArlPDFVersion + "\\,([a-z\\-]+)\\)");

/// @brief Recursive descent parser regex patterns - all with 'starts with' pattern ("^")
static const std::regex   r_StartsWithPredicate("^fn:[a-zA-Z14]+\\(");
static const std::regex   r_StartsWithKeyValue("^" + ArlKeyValue);
static const std::regex   r_StartsWithKey("^" + ArlKey);
static const std::regex   r_StartsWithMathComp("^" + ArlMathComp);
static const std::regex   r_StartsWithMathOp("^" + ArlMathOp);
static const std::regex   r_StartsWithLogicOp("^" + ArlLogicalOp);
static const std::regex   r_StartsWithBool("^" + ArlBooleans);
static const std::regex   r_StartsWithNum("^" + ArlNum);
static const std::regex   r_StartsWithInt("^" + ArlInt);
static const std::regex   r_StartsWithString("^" + ArlString);
static const std::regex   r_StartswithType("^" + ArlPredfinedType);
static const std::regex   r_StartswithLink("^" + ArlLink);


#ifdef ARL_PARSER_DEBUG
/// @brief enables pretty-printing of recursion depth
static      int call_depth = 0;
#endif // ARL_PARSER_DEBUG


/// @brief         Left-to-right recursive descent parser function that processes only operands/expressions (NOT predicates)
/// 
/// @param[in]     s     string to parse
/// @param[in,out] root  root node of AST 
/// 
/// @returns        remaining string that needs to be parsed
std::string LRParseExpression(std::string s, ASTNode* root) {
    assert(root != nullptr);
    ASTNodeStack    stack;
    int             nested_expressions = 0;
    std::smatch     m, m1;
    int             loop = 100;  // avoid deadlocks due to bad predicates

    if (s.empty())
        return s;

#ifdef ARL_PARSER_DEBUG
    call_depth++;
    std::cout << std::string(call_depth, ' ') << "LRParseExpression(s-in='" << s << "')" << std::endl;
#endif // ARL_PARSER_DEBUG

    stack.push_back(root);

    do {
        // Might start with multiple explicitly bracketed expression / sub-expression
        // e.g.  ((a+b)-c)
        assert(!s.empty());
        while (s[0] == '(') {                   
            s = s.substr(1, s.size() - 1);      
            assert(!s.empty());
            nested_expressions++;
            ASTNode* nested_node = new ASTNode(stack.back());
            stack.back()->arg[0] = nested_node;
            stack.push_back(nested_node);
        }

        if (std::regex_search(s, m, r_StartsWithPredicate)) {
            assert(m.ready());
            ASTNode* p = stack.back();
            assert(p->node.empty());
            p->node = m[0];
            s = m.suffix().str();
            assert(!s.empty());
            // Process up to 2 optional arguments until predicate closing bracket ')'
            if (s[0] != ')') {
                p->arg[0] = new ASTNode(p);
                s = LRParsePredicate(s, p->arg[0]);

                assert(!s.empty());
                if (s[0] == ',') {                          // COMMA = optional 2nd argument in predicate
                    s = s.substr(1, s.size() - 1);          // Remove COMMA
                    p->arg[1] = new ASTNode(p);
                    s = LRParsePredicate(s, p->arg[1]);
                }
                else if (s[0] != ')') {
                    // must be an operator that is part of an expression for arg[0]...
                    s = LRParseExpression(s, p->arg[0]);
                }
            }
            assert(!s.empty() && (s[0] == ')'));
            s = s.substr(1, s.size() - 1);                  // Consume ')' that ends predicate
        } 
        else if (std::regex_search(s, m, r_StartsWithBool) || std::regex_search(s, m, r_StartsWithString) ||
            std::regex_search(s, m, r_StartswithType) || std::regex_search(s, m, r_StartsWithKeyValue) || 
            std::regex_search(s, m, r_StartsWithKey) || std::regex_search(s, m, r_StartswithLink) ||
            std::regex_search(s, m, r_StartsWithNum) || std::regex_search(s, m, r_StartsWithInt)) {
            // Variable / constant. ORDERING of above regex expressions is CRITICAL!! 
            ASTNode* p = stack.back();
            assert(m.ready());
            assert(p->node.empty());
            p->node = m[0];
            s = m.suffix().str();
        }
    
        // Close any explicitly closed  sub-expressions
        while ((nested_expressions > 0) && (s[0] == ')')) {
            assert(!s.empty());
            s = s.substr(1, s.size() - 1);
            nested_expressions--;
            stack.pop_back();
        }

        // Check for in-fix operator - recurse down to parse RHS
        if (std::regex_search(s, m1, r_StartsWithMathComp) || std::regex_search(s, m1, r_StartsWithMathOp) || std::regex_search(s, m1, r_StartsWithLogicOp)) {
            assert(m1.ready());
            std::string op  = m1[0];
            s = m1.suffix().str();
            // top-of-stack is LHS to the operator we just encountered
            // Update top-of-stack for this operator and then add new RHS to stack
            ASTNode*    p   = stack.back();
            assert(p != nullptr);
            ASTNode*    lhs;
            ASTNode*    rhs;
            if (p->node.empty()) {
                // We pushed for an open bracket so an empty node already exists and LHS already set
                // e.g. fn:A(x+(y*z)) where 'op' is '*'
                p->node = op;
                assert(p->arg[1] == nullptr);
                rhs = new ASTNode(p);
                p->arg[1] = rhs;
            }
            else {
                // Infix operator without any extra open bracket
                // e.g. fn:A(x+y) where 'op' is '+'
                lhs = new ASTNode(p);
                rhs = new ASTNode(p);
                *lhs = *p;
                p->node   = op;
                p->arg[0] = lhs;
                p->arg[1] = rhs;
            }
            // Parse RHS
            s = LRParsePredicate(s, rhs);
        }

        while ((nested_expressions > 0) && (s[0] == ')')) {         // Close any explicitly bracketed expressions
            assert(!s.empty());
            s = s.substr(1, s.size() - 1);
            nested_expressions--;
            stack.pop_back();
        }

        // Typos in predicates, etc can cause this loop not to terminate...
        if (--loop <= 0) {
            std::cerr << "Error: Failure to terminate parsing of '" << s << "', AST=" << root << std::endl;
            assert(loop > 0);
        }
    }
    while ((loop > 0) && ((nested_expressions > 0) || (!s.empty() && (s[0] != ',') && (s[0] != ')'))));

    assert(stack.size() == 1); // root      
    assert(nested_expressions == 0);

#ifdef ARL_PARSER_DEBUG
    std::cout << std::string(call_depth, ' ') << "LRParseExpression(" << *root <<" ), s-out='" << s << "'" << std::endl;
    call_depth--;
#endif // ARL_PARSER_DEBUG

    return s;
}


/// @brief   Performs a left-to-right recursive decent parse of a raw Arlington predicate string. 
/// 
/// @param[in] s          a string to be parsed
/// @param[in,out] root   an AST node that needs to be populated. Never nullptr.
/// 
/// @returns remaining string to be parsed
std::string LRParsePredicate(std::string s, ASTNode *root) {
    assert(root != nullptr);
    std::smatch     m, m1;

    if (s.empty())
        return s;

#ifdef ARL_PARSER_DEBUG
    call_depth++;
    std::cout << std::string(call_depth, ' ') << "LRParsePredicate(s-in='" << s << "', root=" << *root << ")" << std::endl;
#endif // ARL_PARSER_DEBUG

    if (std::regex_search(s, m, r_StartsWithPredicate)) {
        assert(m.ready());
        assert(root->node.empty());
        root->node = m[0];
        s = m.suffix().str();
        assert(!s.empty());
        // Process up to 2 optional arguments until predicate closing bracket ')'
        if (s[0] != ')') {
            root->arg[0] = new ASTNode(root);
            s = LRParsePredicate(s, root->arg[0]);      // arg[0] is possibly only argument

            assert(!s.empty());
            if (s[0] == ',') {                          // COMMA = optional 2nd argument in predicate
                s = s.substr(1, s.size() - 1);          // Remove COMMA
                root->arg[1] = new ASTNode(root);
                s = LRParsePredicate(s, root->arg[1]);
            }
            else if (s[0] != ')') {
                // must be an operator that is part of an expression for arg[0]
                // e.g. fn:Eval(@x==1) - encountered first '=' of "=="
                s = LRParseExpression(s, root->arg[0]);
            }
        }
        assert(!s.empty() && (s[0] == ')'));
        s = s.substr(1, s.size() - 1);                  // Consume ')' that ends predicate
    } 
    else {
        assert(root->node.empty());
        assert(root->arg[0] == nullptr);
        assert(root->arg[1] == nullptr);
        s = LRParseExpression(s, root);
        if (root->node.empty()) {
            assert(root->arg[0] != nullptr);
            assert(root->arg[1] == nullptr);
            ASTNode  *tmp = root->arg[0];
            *root = *tmp;           // struct copy!
            tmp->arg[0] = nullptr;
            tmp->arg[1] = nullptr;
            delete tmp;
        }
    }

#ifdef ARL_PARSER_DEBUG
    std::cout << std::string(call_depth, ' ') << "LRParsePredicate(" << *root << ", s-out='" << s << "'" << std::endl;
    call_depth--;
#endif // ARL_PARSER_DEBUG
    return s;
}


/// @brief Validates an Arlington "Key" field (column 1) 
/// - no predicates
/// - No COMMAs or SEMI-COLONs
/// - any alphanumeric or "." or "-" or "_"
/// - any integer (array index)
/// - wildcard "*" - must be last row (cannot be checked here)
/// - integer + "*" for a repeating set of N array elements - all rows (cannot be checked here)
bool KeyPredicateProcessor::ValidateRowSyntax() {
    const std::regex      r_Keys("(\\*|[0-9]+|[0-9]+\\*|[a-zA-Z0-9\\-\\._]+)");
    if (tsv_field.find("fn:") != std::string::npos)
        return false;
    if (std::regex_search(tsv_field,r_Keys)) 
        return true;
    return false;
}


/// @brief Validates an Arlington "Type" field (column 2) 
/// Arlington types are all lowercase.
///  - fn:SinceVersion(x.y,type)
///  - fn:Deprecated(x.y,type)
///  - fn:BeforeVersion(x.y,type)
///  - fn:IsPDFVersion(x.y,type)
bool TypePredicateProcessor::ValidateRowSyntax() {
    std::vector<std::string> type_list = split(tsv_field, ';');
    bool valid;
    for (auto& t : type_list) {
        std::smatch     m;
        if (t.find("fn:") == std::string::npos) {
            valid = FindInVector(v_ArlAllTypes, t);
            if (!valid)
                return false;
        }
        else if (std::regex_search(t, m, r_Types) && m.ready() && (m.size() == 4)) {
            // m[1] = predicate function name (no "fn:")
            // m[2] = PDF version "x.y"
            valid = FindInVector(v_ArlPDFVersions, m[2]);
            if (!valid)
                return false;
            // m[3] = Arlington pre-defined type
            valid = FindInVector(v_ArlAllTypes, m[3]);
            if (!valid)
                return false;
        }
        else
            return false;
    } // for    
    return true;
}

/// @brief Reduces an Arlington "Type" field (column 2) based on a PDF version
/// Arlington types are always lowercase
/// - a;b;c
/// - fn:SinceVersion(x.y,type)
/// - fn:Deprecated(x.y,type)
/// - fn:BeforeVersion(x.y,type)
/// - fn:IsPDFVersion(1.0,type)
/// 
/// @param[in]  pdf_version   A PDF version as a string (1.0, 1.1, ... 1.7, 2.0)
/// 
/// @returns a list of Arlington types WITHOUT any predicates. NEVER an empty string "".
std::string TypePredicateProcessor::ReduceRow(const std::string pdf_version) {
    if (tsv_field.find("fn:") == std::string::npos)
        return tsv_field;

    std::string     to_ret = "";
    assert(pdf_version.size() == 3);

    std::vector<std::string> type_list = split(tsv_field, ';');
    for (auto& t : type_list) {
        std::smatch     m;
        if (std::regex_search(t, m, r_Types) && m.ready() && (m.size() == 4)) {
            // m[1] = predicate function name (no "fn:")
            // m[2] = PDF version "x.y" --> convert to integer as x*10 + y
            int pdf_v = pdf_version[0] * 10 + pdf_version[2];
            assert(m[2].str().size() == 3);
            int arl_v = m[2].str()[0] * 10 + m[2].str()[2];
            if (((m[1] == "SinceVersion")  && (pdf_v >= arl_v)) ||
                ((m[1] == "BeforeVersion") && (pdf_v <  arl_v)) ||
                ((m[1] == "IsPDFVersion")  && (pdf_v == arl_v)) ||
                ((m[1] == "Deprecated")    && (pdf_v <  arl_v))) {
                // m[3] = Arlington type
                for (auto& a : v_ArlAllTypes) {
                    if (m[3] == a) {
                        if (to_ret == "")
                            to_ret = a;
                        else
                            to_ret += ";" + a;
                        break;
                    }
                }
            }
        }
        else {
            // No predicate so just add...
            if (to_ret == "")
                to_ret = t;
            else
                to_ret += ";" + t;
        } 
    } // for

    assert(to_ret != "");
    assert(to_ret.find("fn:") == std::string::npos);
    return to_ret;
}


/// @brief Validates an Arlington "SinceVersion" field (column 3) 
/// - only "1.0" or "1.1" or ... or "1.7 or "2.0"
bool SinceVersionPredicateProcessor::ValidateRowSyntax() {
    if (tsv_field.size() == 3)
        return FindInVector(v_ArlPDFVersions, tsv_field);
    return false;
}


/// @brief Determines if the current Arlington row is valid based on the "SinceVersion" field (column 3) 
/// 
/// @returns true if this row is valid for the specified by PDF version. false otherwise
bool SinceVersionPredicateProcessor::ReduceRow(const std::string pdf_version) {
    // PDF version "x.y" --> convert to integer as x*10 + y
    assert(tsv_field.size() == 3);
    assert(pdf_version.size() == 3);

    int pdf_v = pdf_version[0] * 10 + pdf_version[2];
    int tsv_v = tsv_field[0]   * 10 + tsv_field[2];
    return (tsv_v <= pdf_v);
};


/// @brief Validates an Arlington "DeprecatedIn" field (column 4) 
/// - only "", "1.0" or "1.1" or ... or "1.7 or "2.0"
bool DeprecatedInPredicateProcessor::ValidateRowSyntax() {
    if (tsv_field == "")
        return true;
    else if (tsv_field.size() == 3)
        return FindInVector(v_ArlPDFVersions, tsv_field);
    return false;
}


/// @brief Determines if the current Arlington row is valid based on the "DeprecatedIn" field (column 4) 
/// 
/// @returns true if this row is valid for the specified by PDF version. false otherwise
bool DeprecatedInPredicateProcessor::ReduceRow(const std::string pdf_version) {
    if (tsv_field == "")
        return true;

    // PDF version "x.y" --> convert to integer as x*10 + y
    assert(tsv_field.size() == 3);
    assert(pdf_version.size() == 3);

    int pdf_v = pdf_version[0] * 10 + pdf_version[2];
    int tsv_v = tsv_field[0]   * 10 + tsv_field[2];
    return (tsv_v >= pdf_v);
};


/// @brief Validates an Arlington "Required" field (column 5) 
/// - either TRUE, FALSE or fn:IsRequired(...)
/// - inner can be very flexible expressions, including logical operators " && " and " || ":
///   . fn:BeforeVersion(x.y), fn:IsPDFVersion(x.y)
///   . fn:IsPresent(key) or fn:NotPresent(key)
///   . @key==value or @key!=value
///   . use of Arlington-PDF-Path key syntax "::", "parent::"
///   . various highly specialized predicates: fn:IsEncryptedWrapper(), fn:NotStandard14Font(), ...
bool RequiredPredicateProcessor::ValidateRowSyntax() {
    if ((tsv_field == "TRUE") || (tsv_field == "FALSE"))
        return true;
    else if ((tsv_field.find("fn:IsRequired(") == 0) && (tsv_field[tsv_field.size()-1] == ')')) {
        std::string whats_left = LRParsePredicate(tsv_field, ast);
        return (whats_left.size() == 0);
    } 
    return false;
}


/// @brief Reduces an Arlington "Required" field (column 5) for a given PDF version and PDF object 
/// - either TRUE, FALSE or fn:IsRequired(...)
/// - NO SEMI-COLONs or [ ]
/// - inner can be very flexible, including logical && and || expressions:
///   . fn:BeforeVersion(x.y), fn:IsPDFVersion(x.y)
///   . fn:IsPresent(key) or fn:NotPresent(key)
///   . @key==... or @key!=...
///   . use of Arlington-PDF-Path "::", "parent::"
///   . various highly specialized predicates: fn:IsEncryptedWrapper(), fn:NotStandard14Font(), ...
/// 
/// @returns true if field is required for the given PDF version and PDF object
bool RequiredPredicateProcessor::ReduceRow(const std::string pdf_version, ArlPDFObject* obj) {
    if (tsv_field == "TRUE")
        return true;
    else if (tsv_field == "FALSE")
        return false;
    else {
        std::string whats_left = LRParsePredicate(tsv_field, ast);
        assert(whats_left.size() == 0);
        assert(ast->node == "fn:IsRequired(");
        assert(ast->arg[0] != nullptr);
        /// @todo PROCESS THE AST!
        return false;
    }
    return false;
}


/// @brief Validates an Arlington "IndirectReference" field (column 6) 
/// - TRUE or FALSE or complex array of TRUE/FALSE [];[];[]
/// - fn:MustBeDirect()
/// - fn:MustBeDirect(...)
bool IndirectRefPredicateProcessor::ValidateRowSyntax() {
    if ((tsv_field == "TRUE") || (tsv_field == "FALSE") || (tsv_field == "fn:MustBeDirect()"))
        return true;
    else if (tsv_field.find(";") != std::string::npos) {
        // complex form: [];[];[]
        std::vector<std::string> indirect_list = split(tsv_field, ';');
        for (auto& ir : indirect_list)
            if ((ir != "[TRUE]") && (ir != "[FALSE]"))
                return false;
        return true;
    }
    else {
        std::string whats_left = LRParsePredicate(tsv_field, ast);
        assert(ast->node == "fn:MustBeDirect(");
        return (whats_left.size() == 0);
    }
}



/// @brief Reduces an Arlington "IndirectReference" field (column 6) based on a Type index
/// - TRUE or FALSE or complex array of TRUE/FALSE [];[];[]
/// - fn:MustBeDirect()
/// - fn:MustBeDirect(...)
ReferenceType IndirectRefPredicateProcessor::ReduceRow(const int type_index) {
    if (tsv_field == "TRUE") 
        return ReferenceType::MustBeIndirect;
    else if (tsv_field == "FALSE")
        return ReferenceType::DontCare;
    else if (tsv_field == "fn:MustBeDirect()")
        return ReferenceType::MustBeDirect;
    else if (tsv_field.find(";") != std::string::npos) {
        // complex form: [];[];[]
        std::vector<std::string> indirect_list = split(tsv_field, ';');
        assert(type_index >= 0);
        assert(type_index < indirect_list.size());
        if (indirect_list[type_index] == "[TRUE]")
            return ReferenceType::MustBeIndirect;
        else 
            return ReferenceType::DontCare;
    }
    else {
        std::string whats_left = LRParsePredicate(tsv_field, ast);
        assert(whats_left.size() == 0);
        assert(ast->node == "fn:MustBeDirect(");
        /// @todo PROCESS AST
        return ReferenceType::DontCare;
    }
}

/// @brief Validates an Arlington "Inheritable" field (column 7) 
/// - only TRUE or FALSE 
bool InheritablePredicateProcessor::ValidateRowSyntax() {
    if ((tsv_field == "TRUE") || (tsv_field == "FALSE"))
        return true;
    return false;
}


/// @brief Validates an Arlington "Inheritable" field (column 7) 
/// - only TRUE or FALSE 
/// 
/// @returns true if the row is inheritable, false otherwise
bool InheritablePredicateProcessor::ReduceRow() {
    if (tsv_field == "TRUE") 
        return true;
    else 
        return false;
}


/// @brief Validates an Arlington "DefaultValue" field (column 8) 
/// Can be pretty much anything so as long as it parses, assume it is OK.
/// 
/// @returns true if syntax is valid. false otherwise 
bool DefaultValuePredicateProcessor::ValidateRowSyntax() {
    if (tsv_field == "")
        return true;

    std::string s;

    if (tsv_field.find(";") != std::string::npos) {
        // complex type [];[];[], so therefore everything has [ and ]
        std::vector<std::string> dv_list = split(tsv_field, ';');

        for (auto& dv : dv_list) 
            if (dv.find("fn:") != std::string::npos) {
                int loop = 0;
                s = dv.substr(1, dv.size() - 2); // strip off '[' and ']'
                do {
                    ASTNode* n = new ASTNode();
                    s = LRParsePredicate(s, n);
                    delete n;
                    loop++;
                    while (!s.empty() && ((s[0] == ',') || (s[0] == ' '))) {
                        s = s.substr(1, s.size() - 1); // skip over COMMAs and SPACEs
                    }
                } while (!s.empty() && (loop < 100));
                if (loop >= 100)
                    return false;
            }
        return true;
    }
    else {
        // non-complex type - [ and ] may be PDF array with SPACE separators so don't strip
        if (tsv_field.find("fn:") != std::string::npos) {
            int loop = 0;
            s = tsv_field;
            do {
                ASTNode* n = new ASTNode();
                s = LRParsePredicate(s, n);
                delete n;
                loop++;
                while (!s.empty() && ((s[0] == ',') || (s[0] == ' '))) {
                    s = s.substr(1, s.size() - 1); // skip over COMMAs and SPACEs
                }
            } while (!s.empty() && (loop < 100));
            if (loop >= 100)
                return false;
        }
        return true;
    }
}


/// @brief  Validates an Arlington "PossibleValues" row (column 9)
/// Can be pretty much anything so as long as it parses, assume it is OK.
/// 
/// @returns true if syntax is valid. false otherwise 
bool PossibleValuesPredicateProcessor::ValidateRowSyntax() {
    if (tsv_field == "")
        return true;

    std::vector<std::string> pv_list = split(tsv_field, ';');
    std::string s;
    for (auto& pv : pv_list) 
        if (pv.find("fn:") != std::string::npos) {
            int loop = 0;
            s = pv.substr(1, pv.size()-2); // strip off '[' and ']'
            do {
                ASTNode* n = new ASTNode();
                s = LRParsePredicate(s, n);
                delete n;
                loop++;
                while (!s.empty() && ((s[0] == ',') || (s[0] == ' '))) {
                    s = s.substr(1, s.size() - 1); // skip over COMMAs and SPACEs
                }
            } while (!s.empty() && (loop < 100));
            if (loop >= 100)
                return false;
        } 
    return true;
}


/// @brief Validates an Arlington "SpecialCase" field (column 10) 
bool SpecialCasePredicateProcessor::ValidateRowSyntax() {
    if (tsv_field == "")
        return true;

    std::string s;
    std::vector<std::string> sc_list = split(tsv_field, ';');
    for (auto& sc : sc_list) {
        int loop = 0;
        s = sc.substr(1, sc.size() - 2); // strip off '[' and ']'
        do {
            ASTNode *n = new ASTNode();
            s = LRParsePredicate(s, n);
            delete n;
            loop++;
            while (!s.empty() && ((s[0] == ',') || (s[0] == ' '))) {
                s = s.substr(1, s.size() - 1); // skip over COMMAs and SPACEs
            }
        } while (!s.empty() && (loop < 100));
        if (loop >= 100)
            return false;
    } // for
    return true;
}



/// @brief Validates an Arlington "Links" field (column 11) 
///  - fn:SinceVersion(x.y,link)
///  - fn:Deprecated(x.y,link)
///  - fn:BeforeVersion(x.y,link)
///  - fn:IsPDFVersion(x.y,link)
bool LinkPredicateProcessor::ValidateRowSyntax() {
    // Nothing to do?
    if (tsv_field.find("fn:") == std::string::npos)
        return true;

    std::vector<std::string> link_list = split(tsv_field, ';');
    bool valid;
    for (auto& lnk : link_list) {
        std::smatch     m;
        if (std::regex_search(lnk, m, r_Links) && m.ready() && (m.size() == 4)) {
            // m[1] = predicate function name (no "fn:")
            // m[2] = PDF version "x.y"
            valid = FindInVector(v_ArlPDFVersions, m[2]);
            if (!valid)
                return false;
            // m[3] = Arlington link (TSV filename) 
        }
        else if (lnk.find("fn:") != std::string::npos)
            return false;
    } // for    
    return true;
}

/// @brief Reduces an Arlington "Links" field (column 11) based on a PDF version 
///  - fn:SinceVersion(x.y,link)
///  - fn:Deprecated(x.y,link)
///  - fn:BeforeVersion(x.y,link)
///  - fn:IsPDFVersion(x.y,link)
/// 
/// @returns an Arlington Links field with all predicates removed. May be empty string "".
std::string LinkPredicateProcessor::ReduceRow(const std::string pdf_version) {
    // Nothing to do?
    if (tsv_field.find("fn:") == std::string::npos)
        return tsv_field;

    std::string to_ret = "";
    std::vector<std::string> link_list = split(tsv_field, ';');
    for (auto lnk : link_list) {
        std::smatch     m;
        if (std::regex_search(lnk, m, r_Links) && m.ready() && (m.size() == 4)) {
            // m[1] = predicate function name (no "fn:")
            // m[2] = PDF version "x.y" --> convert to integer as x*10 + y
            int pdf_v = pdf_version[0] * 10 + pdf_version[2];
            assert(m[2].str().size() == 3);
            int arl_v = m[2].str()[0] * 10 + m[2].str()[2];
            if (((m[1] == "SinceVersion")  && (pdf_v >= arl_v)) ||
                ((m[1] == "BeforeVersion") && (pdf_v <  arl_v)) ||
                ((m[1] == "IsPDFVersion")  && (pdf_v == arl_v)) ||
                ((m[1] == "Deprecated")    && (pdf_v <  arl_v))) {
                // m[3] = Arlington link
                if (to_ret == "")
                    to_ret = m[3];
                else
                    to_ret += ";" + m[3].str();
            }
        }
        else {
            if (to_ret == "")
                to_ret = lnk;
            else
                to_ret += ";" + lnk;
        }
    } // for    

    assert(to_ret.find("fn:") == std::string::npos);
    return to_ret;
}


//////////////////////////////////////////////////////////////////////////////////////////////
//
//  Predicate implementations
//   - return true if predicate makes sense, false otherwise
//   - 1st argument is current key (from Arlington), if required
//   - 2nd argument is the PDF object in question
//   - next set of arguments are function specific
//   - last argument(s) are the return (out) parameters
//
//////////////////////////////////////////////////////////////////////////////////////////////


/// @brief   Check if the value of a key is in a dictionary and matches a given set
/// 
/// @param[in] dict     dictionary object
/// @param[in] key      the key name or array index
/// @param[in] values   a set of values to match
/// 
/// @returns true if the key value matches something in the values set
bool check_key_value(ArlPDFDictionary* dict, const std::wstring& key, const std::vector<std::wstring> values)
{
    assert(dict != nullptr);
    ArlPDFObject *val_obj = dict->get_value(key);

    if (val_obj != nullptr) {
        std::wstring  val;
        switch (val_obj->get_object_type()) {
            case PDFObjectType::ArlPDFObjTypeString:
                val = ((ArlPDFString*)val_obj)->get_value();
                for (auto& i : values)
                    if (val == i)
                        return true;
                break;
            case PDFObjectType::ArlPDFObjTypeName:
                val = ((ArlPDFName *)val_obj)->get_value();
                for (auto& i : values)
                    if (val == i)
                        return true;
                break;
        } // switch
    }
    return false;
}


bool fn_ArrayLength(ArlPDFObject* obj, int *arr_len) {
    assert(obj != nullptr);
    assert(arr_len != nullptr);
    if (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeArray) {
        ArlPDFArray *arr = (ArlPDFArray *)obj;
        *arr_len = arr->get_num_elements();
        return true;
    }
    return false;
}


bool fn_ArraySortAscending(ArlPDFObject* obj, bool *sorted) {
    assert(obj != nullptr);
    assert(sorted != nullptr);
    *sorted = false;

    if (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeArray) {
        ArlPDFArray* arr = (ArlPDFArray*)obj;
        if (arr->get_num_elements() > 0) {
            // Make sure all array elements are a consistent numeric type
            PDFObjectType obj_type = arr->get_value(0)->get_object_type();
            if (obj_type == PDFObjectType::ArlPDFObjTypeNumber) {
                double       last_elem_val = ((ArlPDFNumber *)arr->get_value(0))->get_value();
                double       this_elem_val;
                for (int i = 1; i < arr->get_num_elements(); i++) {
                    obj_type = arr->get_value(i)->get_object_type();
                    if (obj_type == PDFObjectType::ArlPDFObjTypeNumber) {
                        this_elem_val = ((ArlPDFNumber*)arr->get_value(i))->get_value();
                        if (last_elem_val > this_elem_val) 
                            return false; // was not sorted!
                        last_elem_val = this_elem_val;
                    }
                    else
                        return false; // inconsistent array element types
                }
                *sorted = true;
                return true;
            }
            else
               return false; // not a numeric array 
        }
        else {
            *sorted = true;
            return true; // empty array is always sorted by definition
        }
    }
    return false; // wasn't an array
}


bool fn_BitClear(ArlPDFObject* obj, int bit, bool *bit_was_clear) {
    assert(obj != nullptr);
    assert((bit >= 1) && (bit <= 32));
    assert(bit_was_clear != nullptr);

    *bit_was_clear = false;
    PDFObjectType obj_type = obj->get_object_type();
    if (obj_type == PDFObjectType::ArlPDFObjTypeNumber) {
        ArlPDFNumber *num_obj = (ArlPDFNumber *)obj;
        if (num_obj->is_integer_value()) {
            int bitmask = 1 << (bit - 1);
            int val = num_obj->get_integer_value();
            *bit_was_clear = ((val & bitmask) == 0);
            return true;
        }
        else
            return false;  // wasn't an integer
    }
    else
        return false; // wasn't a number
}


bool fn_BitSet(ArlPDFObject* obj, int bit, bool* bit_was_set) {
    assert(obj != nullptr);
    assert((bit >= 1) && (bit <= 32));
    assert(bit_was_set != nullptr);

    *bit_was_set = false;
    PDFObjectType obj_type = obj->get_object_type();
    if (obj_type == PDFObjectType::ArlPDFObjTypeNumber) {
        ArlPDFNumber* num_obj = (ArlPDFNumber *)obj;
        if (num_obj->is_integer_value()) {
            int bitmask = 1 << (bit - 1);
            int val = num_obj->get_integer_value();
            *bit_was_set = ((val & bitmask) == 1);
            return true;
        }
        else
            return false;  // wasn't an integer
    }
    else
        return false; // wasn't a number
}


bool fn_BitsClear(ArlPDFObject* obj, int low_bit, int high_bit, bool *all_bits_clear) {
    assert(obj != nullptr);
    assert((low_bit >= 1) && (low_bit <= 32));
    assert((high_bit >= 1) && (high_bit <= 32));
    assert(low_bit < high_bit);

    *all_bits_clear = false;
    PDFObjectType obj_type = obj->get_object_type();
    if (obj_type == PDFObjectType::ArlPDFObjTypeNumber) {
        ArlPDFNumber* num_obj = (ArlPDFNumber*)obj;
        if (num_obj->is_integer_value()) {
            int val = num_obj->get_integer_value();
            for (int bit = low_bit; bit <= high_bit; bit ++) {
                int bitmask = 1 << (bit - 1);
                *all_bits_clear = *all_bits_clear && ((val & bitmask) == 0);
            }
            return true;
        }
        else
            return false;  // wasn't an integer
    }
    else
        return false; // wasn't a number
}


bool fn_BitsSet(ArlPDFObject* obj, int low_bit, int high_bit, bool* all_bits_set) {
    assert(obj != nullptr);
    assert((low_bit >= 1) && (low_bit <= 32));
    assert((high_bit >= 1) && (high_bit <= 32));
    assert(low_bit < high_bit);

    *all_bits_set = false;
    PDFObjectType obj_type = obj->get_object_type();
    if (obj_type == PDFObjectType::ArlPDFObjTypeNumber) {
        ArlPDFNumber* num_obj = (ArlPDFNumber*)obj;
        if (num_obj->is_integer_value()) {
            int val = num_obj->get_integer_value();
            for (int bit = low_bit; bit <= high_bit; bit++) {
                int bitmask = 1 << (bit - 1);
                *all_bits_set = *all_bits_set && ((val & bitmask) == 1);
            }
            return true;
        }
        else
            return false;  // wasn't an integer
    }
    else
        return false; // wasn't a number
}


bool fn_Eval(ArlPDFObject* obj) {
    assert(obj != nullptr);
    return false; /// @todo
}

bool fn_FileSize(size_t limit) {
    assert(limit > 0);
    return false; /// @todo
}


bool fn_FontHasLatinChars(ArlPDFObject* obj) {
    assert(obj != nullptr);
    return false; /// @todo
}


bool fn_GetPageNumber(ArlPDFObject* obj) {
    assert(obj != nullptr);
    return false; /// @todo
}


bool fn_Ignore() {
    return true;
}


bool fn_ImageIsStructContentItem(ArlPDFObject* obj) {
    return false; /// @todo
}


bool fn_ImplementationDependent() {
    return true;
}


bool fn_InMap(ArlPDFObject* obj) {
    assert(obj != nullptr);
    /// @todo
    return false; /// @todo
}


bool fn_IsAssociatedFile(ArlPDFObject* obj) {
    assert(obj != nullptr);
    /// @todo Need to see if obj is in trailer::Catalog::AF (array of File Specification dicionaries)
    return false; /// @todo
}


bool fn_IsEncryptedWrapper(ArlPDFObject* obj) {
    assert(obj != nullptr);
    return false; /// @todo
}


bool fn_IsLastInNumberFormatArray(ArlPDFObject* obj) {
    assert(obj != nullptr);
    return false; /// @todo
}


bool fn_IsMeaningful(ArlPDFObject* obj) {
    assert(obj != nullptr);
    return false; /// @todo
}


bool fn_IsPDFTagged(ArlPDFObject* obj) {
    assert(obj != nullptr);
    /// @todo Need to see trailer::Catalog::StructTreeRoot exists
    return false; /// @todo
}


bool fn_IsPageNumber(ArlPDFObject* obj) {
    assert(obj != nullptr);
    return false; /// @todo
}


bool fn_IsPresent(ArlPDFObject* obj, std::string& key, bool *is_present) {
    assert(obj != nullptr);
    assert(is_present != nullptr);

    *is_present = false;
    switch (obj->get_object_type()) {
        case PDFObjectType::ArlPDFObjTypeArray: {
            ArlPDFArray* arr = (ArlPDFArray*)obj;
            try {
                int idx = std::stoi(key);
                *is_present = (arr->get_value(idx) != nullptr);
                return true;
            }
            catch (...) {
                return false; // key wasn't a number
            }
            }
        case PDFObjectType::ArlPDFObjTypeDictionary: {
            ArlPDFDictionary *dict = (ArlPDFDictionary*)obj;
            *is_present = (dict->get_value(ToWString(key)) != nullptr);
            return true;
            }
    } // switch
    return false; 
}


bool fn_KeyNameIsColorant(std::wstring& key, std::vector<std::wstring>& colorants) {
    for (auto& k : colorants)
        if (k == key)
            return true;
    return false; 
}


bool fn_MustBeDirect(ArlPDFObject* obj) {
    assert(obj != nullptr);
    return !obj->is_indirect_ref();
}


bool fn_NoCycle(ArlPDFObject* obj, std::string key) {
    assert(obj != nullptr);
    /// @todo Starting at obj, recursively look at key to ensure there is no loop
    return false; /// @todo
}


bool fn_NotInMap(ArlPDFObject* obj, std::string& pdf_path) {
    assert(obj != nullptr);
    /// @todo Look up map and then look up a reference to this obj
    return false; /// @todo
}


bool fn_NotPresent(ArlPDFObject* obj, std::string& key, bool* is_not_present) {
    bool ret_val = fn_IsPresent(obj, key, is_not_present);
    *is_not_present = !(*is_not_present);
    return ret_val;
}


/// @brief PDF Standard 14 font names
static const std::vector<std::wstring> Std14Fonts = { 
    L"Times-Roman", 
    L"Helvetica", 
    L"Courier", 
    L"Symbol",  
    L"Times-Bold", 
    L"Helvetica-Bold", 
    L"Courier-Bold", 
    L"ZapfDingbats", 
    L"Times-Italic",
    L"Helvetica-Oblique", 
    L"Courier-Oblique", 
    L"Times-BoldItalic", 
    L"Helvetica-BoldOblique", 
    L"Courier-BoldOblique" 
};

bool fn_NotStandard14Font(ArlPDFObject* parent, bool *not_std14font) {
    assert(parent != nullptr);
    assert(not_std14font != nullptr);

    *not_std14font = false;
    if (parent->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary) {
        ArlPDFDictionary *dict = (ArlPDFDictionary*)parent;
        if (check_key_value(dict, L"Type", {L"Font"}) && check_key_value(dict, L"Subtype", {L"Type1"}) && !check_key_value(dict, L"BaseFont", Std14Fonts)) {
            *not_std14font = true;
            return true;
        }
        else
            return false; // wasn't a Type 1 font dictionary
    }
    return false; 
}


// Object is a StructParent integer
bool fn_PageContainsStructContentItems(ArlPDFObject* obj, bool *struct_content_items) {
    assert(obj != nullptr);
    assert(struct_content_items != nullptr);

    *struct_content_items = false;
    if (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeNumber) {
        if (((ArlPDFNumber*)obj)->is_integer_value()) {
            int val = ((ArlPDFNumber*)obj)->get_integer_value();
            if (val >= 0) {
                /// @todo Need to see if this is a valid index into trailer::Catalog::StructTreeRoot::ParentTree (number tree)  
                *struct_content_items = true;
                return true; 
            }
            else
                return false; // cannot have negative value
        }
        else
            return false; // not an integer
    }
    return false;  // not a number object
}


bool fn_RectHeight(ArlPDFObject* obj, double *height) {
    assert(obj != nullptr);
    assert(height != nullptr);

    *height = -1;
    if (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeArray) {
        ArlPDFArray* rect = (ArlPDFArray*)obj;
        if (rect->get_num_elements() == 4) {
            for (int i = 0; i < 4; i++)
                if (rect->get_value(i)->get_object_type() != PDFObjectType::ArlPDFObjTypeNumber)
                    return false; // not all rect array elements were numbers;
            double lly = ((ArlPDFNumber*)rect->get_value(1))->get_value();
            double ury = ((ArlPDFNumber*)rect->get_value(3))->get_value();
            *height = round(fabs(ury - lly));
            return true;
        }
        else
            return false; // not a 4 element array
    }
    return false; // not an array
}


bool fn_RectWidth(ArlPDFObject* obj, double *width) {
    assert(obj != nullptr);
    assert(width != nullptr);

    *width = -1;
    if (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeArray) {
        ArlPDFArray *rect = (ArlPDFArray*)obj;
        if (rect->get_num_elements() == 4) {
            for (int i = 0; i < 4; i++)
                if (rect->get_value(i)->get_object_type() != PDFObjectType::ArlPDFObjTypeNumber)
                    return false; // not all rect array elements were numbers;
            double llx = ((ArlPDFNumber *)rect->get_value(0))->get_value();
            double urx = ((ArlPDFNumber *)rect->get_value(2))->get_value();
            *width = round(fabs(urx - llx));
            return true;
        }
        else
            return false; // not a 4 element array
    }
    return false; // not an array
}


bool fn_RequiredValue(ArlPDFObject* obj, std::string& expr, std::string& value, bool *was_req_val) {
    assert(obj != nullptr);
    assert(was_req_val != nullptr);

    *was_req_val = false;
    return false; /// @todo
}


bool fn_StreamLength(ArlPDFObject* obj, size_t *stm_len) {
    assert(obj != nullptr);
    assert(stm_len != nullptr);

    *stm_len = -1;
    if (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeStream) {
        ArlPDFStream* stm_obj = (ArlPDFStream*)obj;
        ArlPDFObject* len_obj = stm_obj->get_dictionary()->get_value(L"Length");
        if (len_obj->get_object_type() == PDFObjectType::ArlPDFObjTypeNumber) {
            ArlPDFNumber* len_num_obj = (ArlPDFNumber*)len_obj;
            if (len_num_obj->is_integer_value()) {
                *stm_len = len_num_obj->get_integer_value();
                return true;
            }
            else
                return false; // stream Length key was a float!
        }
        else
            return false; // stream Length key was not an number
    }
    return false; // not a stream
}


bool fn_StringLength(ArlPDFObject* obj, size_t *str_len) {
    assert(obj != nullptr);
    assert(str_len != nullptr);

    *str_len = -1;
    if (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeString) {
        ArlPDFString *str_obj = (ArlPDFString *)obj;
        std::wstring s = str_obj->get_value();
        *str_len = s.size();
        return true;
    }
    return false; // not a string
}


//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Simplistic expression grammar support: 
// - Fully bracketed logical sub-expressions using ( and )
// - Key value variables (@name)
// - Integer and number constants
// - PDF Name constants
// - PDF string constants "(xxx)"
// - PDF Boolean keywords (constants): true, false
// - Logical comparison: &&, ||, == 
// - Mathematical comparison: ==, !=, >, <, >=, <=
// - Mathematical operators: +, -, *, mod
// - Predicates starting with "fn:"
//
// Grep voodoo: grep -Pho "fn:[a-zA-Z]+\((?:[^)(]+|(?R))*+\)" *.tsv | sort | uniq
// 
/////////////////////////////////////////////////////////////////////////////////////////////////////
