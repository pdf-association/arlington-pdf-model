///////////////////////////////////////////////////////////////////////////////
/// @file 
/// @brief A left-to-right, recursive descent regex-based parser for Arlington predicates.
/// 
/// @copyright
/// Copyright 2020-2022 PDF Association, Inc. https://www.pdfa.org
/// SPDX-License-Identifier: Apache-2.0
/// 
/// @remark
/// This material is based upon work supported by the Defense Advanced
/// Research Projects Agency (DARPA) under Contract No. HR001119C0079.
/// Any opinions, findings and conclusions or recommendations expressed
/// in this material are those of the author(s) and do not necessarily
/// reflect the views of the Defense Advanced Research Projects Agency
/// (DARPA). Approved for public release.
///
/// @author Peter Wyatt, PDF Association
///
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


/// @brief define PP_DEBUG to get VERY verbose debugging of predicate processing (PP)
#undef PP_DEBUG


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
    ASTNodeType     m_type;
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
        m_type = ASTNodeType::ASTNT_Unknown;

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
            p->type = ASTNodeType::ASTNT_Predicate;
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
        else if ((m_type = ASTNodeType::ASTNT_ConstPDFBoolean, std::regex_search(s, m, r_StartsWithBool)) ||
            (m_type = ASTNodeType::ASTNT_ConstString,          std::regex_search(s, m, r_StartsWithString)) ||
            (m_type = ASTNodeType::ASTNT_Type,                 std::regex_search(s, m, r_StartswithType)) ||
            (m_type = ASTNodeType::ASTNT_KeyValue,             std::regex_search(s, m, r_StartsWithKeyValue)) ||
            (m_type = ASTNodeType::ASTNT_ConstNum,             std::regex_search(s, m, r_StartsWithNum)) ||
            (m_type = ASTNodeType::ASTNT_ConstInt,             std::regex_search(s, m, r_StartsWithInt)) ||
            (m_type = ASTNodeType::ASTNT_Key,                  std::regex_search(s, m, r_StartsWithKey))) {
                // Variable / constant. ORDERING of above regex expressions is CRITICAL!!
            ASTNode* p = stack.back();
            assert(m.ready());
            assert(p->node.empty());
            assert(m_type != ASTNodeType::ASTNT_Unknown);
            p->node = m[0];
            p->type = m_type;
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
        if ((m_type = ASTNodeType::ASTNT_MathComp,  std::regex_search(s, m1, r_StartsWithMathComp)) ||
            (m_type = ASTNodeType::ASTNT_MathOp,    std::regex_search(s, m1, r_StartsWithMathOp)) ||
            (m_type = ASTNodeType::ASTNT_LogicalOp, std::regex_search(s, m1, r_StartsWithLogicOp))) {
            assert(m1.ready());
            assert(m_type != ASTNodeType::ASTNT_Unknown);
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
                p->type = m_type;
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
                p->type = m_type;
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
            std::cerr << COLOR_ERROR << "Failure to terminate parsing of '" << s << "', AST=" << *root << COLOR_RESET;
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
        root->type = ASTNodeType::ASTNT_Predicate;
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


/// @brief Destructor. manually walk matrix of ASTs and delete each node
PredicateProcessor::~PredicateProcessor()
{
    // A vector of ASTNodeStack = a vector of vectors of ASTNodes
    if (!predicate_ast.empty()) {
        for (int i = (int)predicate_ast.size()-1; i >= 0; i--) {
            if (!predicate_ast[i].empty()) {
                for (int j = (int)predicate_ast[i].size()-1; j >= 0; j--) {
                    delete predicate_ast[i][j];
                }
                predicate_ast[i].clear(); // inner vector
            }
        }
        predicate_ast.clear(); // outer vector of matrix
    }
};


/// @brief Sets the PDF version of file.  Used when reducing Arlington down for a specific object.
/// @param[in]  pdfver  "1.0", "1.1", ..., "2.0"
void PredicateProcessor::set_pdf_version(std::string& pdfver) {
    assert(pdfver.size() == 3);
    if (FindInVector(v_ArlPDFVersions, pdfver)) {
        pdf_version = pdfver;
    }
}


/// @brief Validates an Arlington "Key" field (column 1)
/// - no predicates allowed
/// - No COMMAs or SEMI-COLONs
/// - any alphanumeric including "." or "-" or "_" only
/// - any integer (i.e. an array index)
/// - wildcard "*" by itself - must be last row (cannot be checked here)
/// - integer + "*" for a repeating set of N array elements - all rows (cannot be checked here)
bool KeyPredicateProcessor::ValidateRowSyntax() {
    const std::regex      r_Keys("(\\*|[0-9]+|[0-9]+\\*|[a-zA-Z0-9\\-\\._]+)");

    // no predicates allowed
    if (tsv_field.find("fn:") != std::string::npos)
        return false;

    return std::regex_search(tsv_field, r_Keys);
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

/// @brief Reduces an Arlington "Type" field (column 2) based on a PDF version.
/// Arlington types are always lowercase, in alphabetical order.
/// - a;b;c
/// - fn:SinceVersion(x.y,type)
/// - fn:Deprecated(x.y,type)
/// - fn:BeforeVersion(x.y,type)
/// - fn:IsPDFVersion(1.0,type)
///
/// @param[in]  pdf_version   A PDF version as a string ("1.0", "1.1", ... "1.7", "2.0")
///
/// @returns a list of Arlington types WITHOUT any predicates. NEVER an empty string "".
std::string TypePredicateProcessor::ReduceRow() {
    if (tsv_field.find("fn:") == std::string::npos)
        return tsv_field;

    std::string     to_ret = "";

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
bool SinceVersionPredicateProcessor::ReduceRow() {
    // PDF version "x.y" --> convert to integer as x*10 + y
    assert(tsv_field.size() == 3);

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
bool DeprecatedInPredicateProcessor::ReduceRow() {
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
///   . \@key==value or \@key!=value
///   . use of Arlington-PDF-Path key syntax "::", "parent::"
///   . various highly specialized predicates: fn:IsEncryptedWrapper(), fn:NotStandard14Font(), ...
bool RequiredPredicateProcessor::ValidateRowSyntax() {
    if ((tsv_field == "TRUE") || (tsv_field == "FALSE"))
        return true;
    else if ((tsv_field.find("fn:IsRequired(") == 0) && (tsv_field[tsv_field.size()-1] == ')')) {
        ASTNode *ast = new ASTNode();
        ASTNodeStack stack;

        std::string whats_left = LRParsePredicate(tsv_field, ast);
        predicate_ast.clear();
        stack.push_back(ast);
        predicate_ast.push_back(stack);
        return (whats_left.size() == 0);
    }
    return false;
}


/// @brief Reduces an Arlington "Required" field (column 5) for a given PDF version and PDF object.
/// - either TRUE, FALSE or fn:IsRequired(...)
/// - NO SEMI-COLONs or [ ]
/// - inner can be very flexible, including logical && and || expressions:
///   . fn:BeforeVersion(x.y), fn:IsPDFVersion(x.y)
///   . fn:IsPresent(key) or fn:NotPresent(key)
///   . \@key==... or \@key!=...
///   . use of Arlington-PDF-Path "::", "parent::"
///   . various highly specialized predicates: fn:IsEncryptedWrapper(), fn:NotStandard14Font(), ...
///
/// @returns true if field is required for the given PDF version and PDF object
bool RequiredPredicateProcessor::ReduceRow(ArlPDFObject* obj) {
    if (tsv_field == "TRUE")
        return true;
    else if (tsv_field == "FALSE")
        return false;
    else {
        if (predicate_ast.empty()) {
            ASTNode* ast = new ASTNode();
            ASTNodeStack stack;

            std::string whats_left = LRParsePredicate(tsv_field, ast);
            predicate_ast.clear();
            stack.push_back(ast);
            predicate_ast.push_back(stack);
            assert(whats_left.size() == 0);
        }
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
        ASTNode *ast = new ASTNode();
        ASTNodeStack stack;

        std::string whats_left = LRParsePredicate(tsv_field, ast);
        assert(ast->node == "fn:MustBeDirect(");
        predicate_ast.clear();
        stack.push_back(ast);
        predicate_ast.push_back(stack);
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
        else if (indirect_list[type_index] == "[FALSE]")
            return ReferenceType::DontCare;
        else { 
            assert(false && "unexpected predicate in a complex IndirectRef field [];[];[] : ");
        }
    }
    else {
        ASTNode* ast;
        if (predicate_ast.empty()) {
            ASTNodeStack stack;

            ast = new ASTNode();
            std::string whats_left = LRParsePredicate(tsv_field, ast);
            assert(ast->node == "fn:MustBeDirect(");
            predicate_ast.clear();
            stack.push_back(ast);
            predicate_ast.push_back(stack);
            assert(whats_left.size() == 0);
        }
        ast = predicate_ast[0][0];
        assert(ast->node == "fn:MustBeDirect(");
        /// @todo PROCESS AST 
    }
    return ReferenceType::DontCare; // Default behaviour
}

/// @brief Validates an Arlington "Inheritable" field (column 7)
/// - only TRUE or FALSE
bool InheritablePredicateProcessor::ValidateRowSyntax() {
    return ((tsv_field == "TRUE") || (tsv_field == "FALSE"));
}


/// @brief Validates an Arlington "Inheritable" field (column 7)
/// - only TRUE or FALSE
///
/// @returns true if the row is inheritable, false otherwise
bool InheritablePredicateProcessor::ReduceRow() {
    return (tsv_field == "TRUE");
}


/// @brief Validates an Arlington "DefaultValue" field (column 8)
/// Can be pretty much anything but so as long as it parses, assume it is OK.
///
/// @returns true if syntax is valid. false otherwise
bool DefaultValuePredicateProcessor::ValidateRowSyntax() {
    if (tsv_field == "")
        return true;

    std::string s;
    ASTNodeStack stack;

    if (tsv_field.find(";") != std::string::npos) {
        // complex type [];[];[], so therefore everything has [ and ]
        std::vector<std::string> dv_list = split(tsv_field, ';');

        for (auto& dv : dv_list) {
            stack.clear();
            if (dv.find("fn:") != std::string::npos) {
                int loop = 0;
                s = dv.substr(1, dv.size() - 2); // strip off '[' and ']'
                do {
                    ASTNode* n = new ASTNode();

                    s = LRParsePredicate(s, n);
                    stack.push_back(n);
                    loop++;
                    while (!s.empty() && ((s[0] == ',') || (s[0] == ' '))) {
                        s = s.substr(1, s.size() - 1); // skip over COMMAs and SPACEs
                    }
                } while (!s.empty() && (loop < 100));
                if (loop >= 100) {
                    assert(false && "Arlington complex type DefaultValue field too long and complex!");
                    return false;
                }
            }
            predicate_ast.push_back(stack);
        } // for
    }
    else {
        // non-complex type - [ and ] may be PDF array with SPACE separators so don't strip
        if (tsv_field.find("fn:") != std::string::npos) {
            int loop = 0;
            s = tsv_field;
            do {
                ASTNode* n = new ASTNode();
                s = LRParsePredicate(s, n);
                stack.push_back(n);
                loop++;
                while (!s.empty() && ((s[0] == ',') || (s[0] == ' '))) {
                    s = s.substr(1, s.size() - 1); // skip over COMMAs and SPACEs
                }
            } while (!s.empty() && (loop < 100));
            if (loop >= 100) {
                assert(false && "Arlington simple type DefaultValue field too long and complex!");
                return false;
            }
        } 
        predicate_ast.push_back(stack);
    }
    return true;
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
    ASTNodeStack stack;

    for (auto& pv : pv_list) {
        stack.clear();
        if (pv.find("fn:") != std::string::npos) {
            int loop = 0;
            s = pv.substr(1, pv.size() - 2); // strip off '[' and ']'
            do {
                ASTNode* n = new ASTNode();
                s = LRParsePredicate(s, n);
                stack.push_back(n);
                loop++;
                while (!s.empty() && ((s[0] == ',') || (s[0] == ' '))) {
                    s = s.substr(1, s.size() - 1); // skip over COMMAs and SPACEs
                }
            } while (!s.empty() && (loop < 100));
            if (loop >= 100) {
                assert(false && "Arlington complex type PossibleValues field too long and complex when validating!");
                return false;
            }
        }
        predicate_ast.push_back(stack);
    }
    return true;
}


/// @brief Checks if "val" is a valid value from a COMMA-separated set
/// 
/// @returns true if "val" is valid (i.e. in the set)
bool PossibleValuesPredicateProcessor::IsValidValue(ArlPDFObject* object, const std::string& pvalues) {
    PDFObjectType obj_type = object->get_object_type();
    std::vector<std::string> val_list = split(pvalues, ',');
    bool retval = false;

    switch (obj_type) {
    case PDFObjectType::ArlPDFObjTypeNull:
        // null always matches so always OK
        retval = true;
        break;

    case PDFObjectType::ArlPDFObjTypeName:
        {
            // PDF Names are raw with no leading SLASH
            std::string nm = ToUtf8(((ArlPDFName*)object)->get_value());
            auto it = std::find(val_list.begin(), val_list.end(), nm);
            retval = (it != val_list.end());
        }
        break;

    case PDFObjectType::ArlPDFObjTypeString:
        {
            // PDF Strings are single quoted in Arlington
            std::string s = "'" + ToUtf8(((ArlPDFString*)object)->get_value()) + "'";
            auto it = std::find(val_list.begin(), val_list.end(), s);
            retval = (it != val_list.end());
        }
        break;

    case PDFObjectType::ArlPDFObjTypeNumber:
        if (((ArlPDFNumber*)object)->is_integer_value()) {
            // Integers can be directly matched
            int num_value = ((ArlPDFNumber*)object)->get_integer_value();
            auto it = std::find(val_list.begin(), val_list.end(), std::to_string(num_value));
            retval = (it != val_list.end());
        }
        else {
            // Real number need a tolerance for matching
            double num_value = ((ArlPDFNumber*)object)->get_value();
            for (auto &it : val_list) {
                try {
                    auto double_val = std::stod(it);
                    // Double-precision comparison often fails because parsed PDF value is not precisely stored
                    // Old Adobe PDF specs used to recommend 5 digits so go +/- half of that
                    if (fabs(num_value - double_val) <= 0.000005) {
                        retval = true;
                        break;
                    }
                }
                catch (...) {
                    // fallthrough and do next opt in PossibleValues options list
                }
            }
        }
        break;

    case PDFObjectType::ArlPDFObjTypeArray:      
        /// @todo Some arrays do have Possible Values (e.g. XObject Decode arrays for masks: [0,1] or [1,0])
        retval = true;
        break;

    case PDFObjectType::ArlPDFObjTypeBoolean:    // Booleans don't have Possible Values in Arlington!
        assert("Booleans don't have Possible Values" && false);
        break;
    case PDFObjectType::ArlPDFObjTypeDictionary: // Dictionaries are linked types and don't have Possible Values!
        assert("Dictionaries are linked types and don't have Possible Values" && false);
        break;
    case PDFObjectType::ArlPDFObjTypeStream:     // Streams are linked types and don't have Possible Values!
        assert("Streams are linked types and don't have Possible Values" && false);
        break;
    case PDFObjectType::ArlPDFObjTypeReference:  // Should not happen
        assert("ArlPDFObjTypeReference" && false);
        break;
    case PDFObjectType::ArlPDFObjTypeUnknown:    // Should not happen
        assert("ArlPDFObjTypeUnknown" && false);
        break;
    default:
        assert("default" && false);
        break;
    }
    return retval;
}




/// @brief Validates an Arlington "SpecialCase" field (column 10)
bool SpecialCasePredicateProcessor::ValidateRowSyntax() {
    if (tsv_field == "")
        return true;

    std::string s;
    std::vector<std::string> sc_list = split(tsv_field, ';');
    ASTNodeStack stack;

    for (auto& sc : sc_list) {
        int loop = 0;
        stack.clear();
        s = sc.substr(1, sc.size() - 2); // strip off '[' and ']'
        do {
            ASTNode *n = new ASTNode();
            s = LRParsePredicate(s, n);
            stack.push_back(n);
            loop++;
            while (!s.empty() && ((s[0] == ',') || (s[0] == ' '))) {
                s = s.substr(1, s.size() - 1); // skip over COMMAs and SPACEs
            }
        } while (!s.empty() && (loop < 100));
        if (loop >= 100) {
            assert(false && "Arlington complex type SpecialCase field too long and complex when validating!");
            return false;
        }
        predicate_ast.push_back(stack);
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
std::string LinkPredicateProcessor::ReduceRow() {
    // Nothing to do?
    if (tsv_field.find("fn:") == std::string::npos)
        return tsv_field;

    std::string to_ret = "";
    std::vector<std::string> link_list = split(tsv_field, ';');
    for (auto& lnk : link_list) {
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

            default: /* fallthrough */
                break;
        } // switch
    }
    return false;
}


int fn_ArrayLength(ArlPDFObject* obj) {
    assert(obj != nullptr);
    if (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeArray) {
        ArlPDFArray *arr = (ArlPDFArray *)obj;
        ASTNode* out = new ASTNode;
        return arr->get_num_elements();
    }
#ifdef PP_DEBUG
    std::cout << "fn_ArrayLength() was not an array!" << std::endl;
#endif
    return -1;
}


bool fn_ArraySortAscending(ArlPDFObject* obj) {
    assert(obj != nullptr);

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
                    else {
#ifdef PP_DEBUG
                        std::cout << "fn_ArraySortAscending() had inconsistent types!" << std::endl;
#endif
                        return false; // inconsistent array element types
                    }
                } // for
                return true;
            }
            else {
#ifdef PP_DEBUG
                std::cout << "fn_ArraySortAscending() was not a numeric array!" << std::endl;
#endif
                return false; // not a numeric array
            }
        }
        else
            return true; // empty array is always sorted by definition
    }
#ifdef PP_DEBUG
    std::cout << "fn_ArraySortAscending() was not an array!" << std::endl;
#endif
    return false; // wasn't an array
}


/// @brief Single bit is clear. 
bool fn_BitClear(ArlPDFObject* obj, const ASTNode *bit_node) {
    assert(obj != nullptr);

    assert((bit_node != nullptr) && (bit_node->type == ASTNodeType::ASTNT_ConstInt));
    int bit = std::stoi(bit_node->node);
    assert((bit >= 1) && (bit <= 32));

    PDFObjectType obj_type = obj->get_object_type();
    if (obj_type == PDFObjectType::ArlPDFObjTypeNumber) {
        ArlPDFNumber *num_obj = (ArlPDFNumber *)obj;
        if (num_obj->is_integer_value()) {
            int bitmask = 1 << (bit - 1);
            int val = num_obj->get_integer_value();
            return ((val & bitmask) == 0);
        }
        else {
#ifdef PP_DEBUG
            std::cout << "fn_BitClear() was not an integer!" << std::endl;
#endif
            return false;  // wasn't an integer
        }
    }
    else {
#ifdef PP_DEBUG
        std::cout << "fn_BitClear() was not a number!" << std::endl;
#endif
        return false; // wasn't a number
    }
}


// Single bit is set. Bit that should be set is in the left arg
bool fn_BitSet(ArlPDFObject* obj, const ASTNode* bit_node) {
    assert(obj != nullptr);

    assert((bit_node != nullptr) && (bit_node->type == ASTNodeType::ASTNT_ConstInt));
    int bit = std::stoi(bit_node->node);
    assert((bit >= 1) && (bit <= 32));

    PDFObjectType obj_type = obj->get_object_type();
    if (obj_type == PDFObjectType::ArlPDFObjTypeNumber) {
        ArlPDFNumber* num_obj = (ArlPDFNumber *)obj;
        if (num_obj->is_integer_value()) {
            int bitmask = 1 << (bit - 1);
            int val = num_obj->get_integer_value();
            return ((val & bitmask) == 1);
        }
        else {
#ifdef PP_DEBUG
            std::cout << "fn_BitSet() was not an integer!" << std::endl;
#endif
            return false;  // wasn't an integer
        }
    }
    else {
#ifdef PP_DEBUG
        std::cout << "fn_BitSet() was not a number!" << std::endl;
#endif
        return false; // wasn't a number
    }
}


/// @brief Multiple bits should be clear. 
bool fn_BitsClear(ArlPDFObject* obj, const ASTNode* low_bit_node, const ASTNode* high_bit_node) {
    assert(obj != nullptr);

    assert((low_bit_node != nullptr) && (low_bit_node->type == ASTNodeType::ASTNT_ConstInt));
    int low_bit = std::stoi(low_bit_node->node);
    assert((low_bit >= 1) && (low_bit <= 32));

    assert((high_bit_node != nullptr) && (high_bit_node->type == ASTNodeType::ASTNT_ConstInt));
    int high_bit = std::stoi(high_bit_node->node);
    assert((high_bit >= 1) && (high_bit <= 32));

    assert(low_bit <= high_bit);

    PDFObjectType obj_type = obj->get_object_type();
    if (obj_type == PDFObjectType::ArlPDFObjTypeNumber) {
        ArlPDFNumber* num_obj = (ArlPDFNumber*)obj;
        if (num_obj->is_integer_value()) {
            int val = num_obj->get_integer_value();
            bool all_bits_clear = true;
            for (int bit = low_bit; bit <= high_bit; bit ++) {
                int bitmask = 1 << (bit - 1);
                all_bits_clear = all_bits_clear && ((val & bitmask) == 0);
            }
            return all_bits_clear;
        }
        else {
#ifdef PP_DEBUG
            std::cout << "fn_BitsClear() was not an integer!" << std::endl;
#endif
            return false;  // wasn't an integer
        }
    }
    else {
#ifdef PP_DEBUG
        std::cout << "fn_BitsClear() was not a number!" << std::endl;
#endif
        return false; // wasn't a number
    }
}


bool fn_BitsSet(ArlPDFObject* obj, const ASTNode* low_bit_node, const ASTNode* high_bit_node) {
    assert(obj != nullptr);

    assert((low_bit_node != nullptr) && (low_bit_node->type == ASTNodeType::ASTNT_ConstInt));
    int low_bit = std::stoi(low_bit_node->node);
    assert((low_bit >= 1) && (low_bit <= 32));

    assert((high_bit_node != nullptr) && (high_bit_node->type == ASTNodeType::ASTNT_ConstInt));
    int high_bit = std::stoi(high_bit_node->node);
    assert((high_bit >= 1) && (high_bit <= 32));

    assert(low_bit < high_bit);

    PDFObjectType obj_type = obj->get_object_type();
    if (obj_type == PDFObjectType::ArlPDFObjTypeNumber) {
        ArlPDFNumber* num_obj = (ArlPDFNumber*)obj;
        if (num_obj->is_integer_value()) {
            int val = num_obj->get_integer_value();
            bool all_bits_set = true;
            for (int bit = low_bit; bit <= high_bit; bit++) {
                int bitmask = 1 << (bit - 1);
                all_bits_set = all_bits_set && ((val & bitmask) == 1);
            }
            return all_bits_set;
        }
        else {
#ifdef PP_DEBUG
            std::cout << "fn_BitsSet() was not an integer!" << std::endl;
#endif
            return false;  // wasn't an integer
        }
    }
    else {
#ifdef PP_DEBUG
        std::cout << "fn_BitsSet() was not a number!" << std::endl;
#endif
        return false; // wasn't a number
    }
}


bool fn_Eval(ArlPDFObject* obj) {
    assert(obj != nullptr);
    return false; /// @todo
}

int fn_FileSize() {
    return 99999999; /// @todo
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
    assert(obj != nullptr);
    return false; /// @todo
}


bool fn_ImplementationDependent() {
    return true;
}

bool fn_InMap(ArlPDFObject* obj, ASTNode *key) {
    assert(obj != nullptr);
    assert(key != nullptr);
    return true; /// @todo
}


bool fn_IsAssociatedFile(ArlPDFObject* obj) {
    assert(obj != nullptr);
    return false; /// @todo Need to see if obj is in trailer::Catalog::AF (array of File Specification dicionaries)
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
    return true; /// @todo - determine if PDF is a Tagged PDF file: DocCat::MarkInfo::Marked == true
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
                    return false; // key wasn't an integer
                }
            }
        case PDFObjectType::ArlPDFObjTypeDictionary: {
                ArlPDFDictionary *dict = (ArlPDFDictionary*)obj;
                *is_present = (dict->get_value(ToWString(key)) != nullptr);
                return true;
            }
        default:
            /// @todo - is this correct? Streams??
            break;
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


bool fn_NoCycle(ArlPDFObject* obj) {
    assert(obj != nullptr);
    return false; /// @todo Starting at obj, recursively look at key to ensure there is no loop
}


bool fn_NotInMap(ArlPDFObject* obj) {
    assert(obj != nullptr);
    return false; /// @todo - Look up map and then look up a reference to this obj
}


ASTNode* fn_NotPresent(ArlPDFObject* obj, std::string& key, bool* is_not_present) {
    return nullptr; /// @todo - fn_NotPresent
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

ASTNode* fn_NotStandard14Font(ArlPDFObject* parent) {
    assert(parent != nullptr);

    if (parent->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary) {
        ArlPDFDictionary *dict = (ArlPDFDictionary*)parent;
        if (check_key_value(dict, L"Type", {L"Font"}) && 
            check_key_value(dict, L"Subtype", {L"Type1"}) && 
            !check_key_value(dict, L"BaseFont", Std14Fonts)) {
            ASTNode* node = new ASTNode;
            node->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            node->node = std::to_string(true);
            return node;
        }
        else
            return nullptr; // wasn't a Type 1 font dictionary
    }
    return nullptr;
}


/// Object is a StructParent integer
ASTNode* fn_PageContainsStructContentItems(ArlPDFObject* obj) {
    assert(obj != nullptr);

    if (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeNumber) {
        if (((ArlPDFNumber*)obj)->is_integer_value()) {
            int val = ((ArlPDFNumber*)obj)->get_integer_value();
            if (val >= 0) {
                /// @todo Need to see if this is a valid index into trailer::Catalog::StructTreeRoot::ParentTree (number tree)
                ASTNode* node = new ASTNode;
                node->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                node->node = std::to_string(true);
                return node;
            }
            else {
#ifdef PP_DEBUG
                std::cout << "fn_PageContainsStructContentItems() was less than zero!" << std::endl;
#endif
                return nullptr; // cannot have negative value
            }
        }
        else {
#ifdef PP_DEBUG
            std::cout << "fn_PageContainsStructContentItems() was not an integer!" << std::endl;
#endif
            return nullptr; // not an integer
        }
    }
#ifdef PP_DEBUG
    std::cout << "fn_PageContainsStructContentItems() was not number!" << std::endl;
#endif
    return nullptr;  // not a number object
}


/// Used by Target.tsv for A key: 
/// - fn:PageProperty(\@P,Annots)
/// - fn:Eval(\@A==fn:PageProperty(\@P,Annots::NM))
/// 1st arg: PDF page object
/// 2nd arg: a key of a page 
bool fn_PageProperty(ArlPDFObject* obj, ASTNode *pg_key) {
    assert(obj != nullptr);
    assert(pg_key != nullptr);
    return true; /// @todo - implement fn:PageProperty() properly
}


double fn_RectHeight(ArlPDFObject* obj) {
    assert(obj != nullptr);

    if (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeArray) {
        ArlPDFArray* rect = (ArlPDFArray*)obj;
        if (rect->get_num_elements() == 4) {
            for (int i = 0; i < 4; i++)
                if (rect->get_value(i)->get_object_type() != PDFObjectType::ArlPDFObjTypeNumber)
                    return -1.0; // not all rect array elements were numbers;
            double lly = ((ArlPDFNumber*)rect->get_value(1))->get_value();
            double ury = ((ArlPDFNumber*)rect->get_value(3))->get_value();
            double height = round(fabs(ury - lly));
            return height;
        }
        else {
#ifdef PP_DEBUG
            std::cout << "fn_RectHeight() was not a 4 element array!" << std::endl;
#endif
            return -1.0; // not a 4 element array
        }
    }
#ifdef PP_DEBUG
    std::cout << "fn_RectHeight() was not an array!" << std::endl;
#endif
    return -1.0; // not an array
}


double fn_RectWidth(ArlPDFObject* obj) {
    assert(obj != nullptr);

    if (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeArray) {
        ArlPDFArray *rect = (ArlPDFArray*)obj;
        if (rect->get_num_elements() == 4) {
            for (int i = 0; i < 4; i++)
                if (rect->get_value(i)->get_object_type() != PDFObjectType::ArlPDFObjTypeNumber)
                    return -1.0; // not all rect array elements were numbers;
            double llx = ((ArlPDFNumber *)rect->get_value(0))->get_value();
            double urx = ((ArlPDFNumber *)rect->get_value(2))->get_value();
            double width = fabs(urx - llx);
            return width;
        }
        else {
#ifdef PP_DEBUG
            std::cout << "fn_RectWidth() was not a 4 element array!" << std::endl;
#endif
            return -1.0; // not a 4 element array
        }
    }
#ifdef PP_DEBUG
    std::cout << "fn_RectWidth() was not an array!" << std::endl;
#endif
    return -1.0; // not an array
}


// e.g. fn:RequiredValue(@CFM==AESV2,128)
//      - condition argument should already be reduced to true/false
//      - value argument can be any primitive PDF type (int, real, name, string-*, boolean)
bool fn_RequiredValue(ArlPDFObject* obj, ASTNode* condition, ASTNode* value) {
    assert(obj != nullptr);

    /// @todo: temporary code until @-value for non-self referenced keys works
    /// e.g. fn:Deprecated(2.0,fn:RequiredValue(\@V<2,2))
    if (condition == nullptr) {
        return false;
    }

    assert(condition != nullptr);
    assert(condition->type == ASTNodeType::ASTNT_ConstPDFBoolean);
    assert(value != nullptr);

    if (condition->node == "false") {
        // Condition not met so value of obj can be anything (no need to check)
        return false;
    }
    else {
        // Condition is met so value of obj MUST BE 'value'
        PDFObjectType obj_type = obj->get_object_type();
        switch (obj_type) {
            case PDFObjectType::ArlPDFObjTypeName:
                if (value->type == ASTNodeType::ASTNT_Key) {
                    return (value->node == ToUtf8(((ArlPDFName*)obj)->get_value()));
                }
                break;

            case PDFObjectType::ArlPDFObjTypeNumber:
                if ((value->type == ASTNodeType::ASTNT_ConstInt) && ((ArlPDFNumber*)obj)->is_integer_value()) {
                    return (value->node == std::to_string(((ArlPDFNumber*)obj)->get_integer_value()));
                }
                else if (value->type == ASTNodeType::ASTNT_ConstNum) {
                    return (value->node == std::to_string(((ArlPDFNumber*)obj)->get_value()));
                }
                break;

            case PDFObjectType::ArlPDFObjTypeBoolean:
                if (value->type == ASTNodeType::ASTNT_ConstPDFBoolean) {
                    if ((value->node == "true") && ((ArlPDFBoolean*)obj)->get_value()) {
                        return true;
                    } 
                    else if ((value->node == "false") && !((ArlPDFBoolean*)obj)->get_value()) {
                        return true;
                    }
                }
                break;

            case PDFObjectType::ArlPDFObjTypeString:
                if (value->type == ASTNodeType::ASTNT_ConstString) {
                    return (value->node == ToUtf8(((ArlPDFString*)obj)->get_value()));
                }
                break;

            case PDFObjectType::ArlPDFObjTypeDictionary:
            case PDFObjectType::ArlPDFObjTypeStream:
            case PDFObjectType::ArlPDFObjTypeArray:
            case PDFObjectType::ArlPDFObjTypeNull:
            case PDFObjectType::ArlPDFObjTypeUnknown:
            case PDFObjectType::ArlPDFObjTypeReference:
            default:
                assert(false && "unexpected fn:RequiredValue value!");
                return false;
        } // switch obj_type
    }

    return false;
}


/// @brief Stream Length is according to key value, not actual data. -1 on error.
int fn_StreamLength(ArlPDFObject* obj) {
    assert(obj != nullptr);

    if (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeStream) {
        ArlPDFStream* stm_obj = (ArlPDFStream*)obj;
        ArlPDFObject* len_obj = stm_obj->get_dictionary()->get_value(L"Length");
        if (len_obj->get_object_type() == PDFObjectType::ArlPDFObjTypeNumber) {
            ArlPDFNumber* len_num_obj = (ArlPDFNumber*)len_obj;
            if (len_num_obj->is_integer_value()) {
                return len_num_obj->get_integer_value();
            }
            else {
#ifdef PP_DEBUG
                std::cout << "fn_StreamLength() Length was not an integer (was a float)!" << std::endl;
#endif
                return -1; // stream Length key was a float!
            }
        }
        else {
#ifdef PP_DEBUG
            std::cout << "fn_StreamLength() Length was not a number!" << std::endl;
#endif
            return -1; // stream Length key was not an number
        }
    }
#ifdef PP_DEBUG
    std::cout << "fn_StreamLength() was not a stream!" << std::endl;
#endif
    return -1; // not a stream
}


// @brief length of a PDF string object. -1 if an error.
int fn_StringLength(ArlPDFObject* obj) {
    assert(obj != nullptr);

    if (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeString) {
        ArlPDFString *str_obj = (ArlPDFString *)obj;
        int len = (int)str_obj->get_value().size();
        return len;
    }
#ifdef PP_DEBUG
    std::cout << "fn_StringLength() was not a string!" << std::endl;
#endif
    return -1; // not a string
}


/// @brief BeforeVersion means a feature was introduced prior to specific PDF version:
///   - fn:IsRequired(fn:BeforeVersion(1.3))
///   - fn:Eval((\@Colors>=1) && fn:BeforeVersion(1.3,fn:Eval(\@Colors<=4)))
/// 
/// @param[in] pdf_ver   version of the PDF file
/// @param[in] ver_node  version from Arlington PDF model
/// @param[in] thing     (optional) the feature that was introduced
ASTNode *fn_BeforeVersion(const std::string& pdf_ver, const ASTNode* ver_node, const ASTNode* thing) {
    assert(pdf_ver.size() == 3);
    assert(FindInVector(v_ArlPDFVersions, pdf_ver));

    assert(ver_node != nullptr);
    assert(ver_node->type == ASTNodeType::ASTNT_ConstNum);
    assert(ver_node->node.size() == 3);
    assert(FindInVector(v_ArlPDFVersions, ver_node->node));

    // Convert to 10 * PDF version
    int pdf_v = (pdf_ver[0] - '0') * 10 + (pdf_ver[2] - '0');
    int arl_v = (ver_node->node[0] - '0') * 10 + (ver_node->node[2] - '0');

    if (thing != nullptr) {
        if (pdf_v < arl_v) {
            ASTNode* out = new ASTNode;
            out->type = thing->type;
            out->node = thing->node;
            return out;
        }
    }
    else { // thing == nullptr
        ASTNode* out = new ASTNode;
        out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
        out->node = (pdf_v < arl_v) ? "true" : "false";
        return out;
    }
    return nullptr;
}


/// @brief SinceVersion means a feature was introduced in a specific PDF version
/// 
/// @param[in] pdf_ver   version of the PDF file
/// @param[in] ver_node  version when feature 'thing' was introduced from Arlington PDF model
/// @param[in] thing     (optional) the feature that was introduced
ASTNode* fn_SinceVersion(const std::string& pdf_ver, const ASTNode* ver_node, const ASTNode* thing) {
    assert(pdf_ver.size() == 3);
    assert(FindInVector(v_ArlPDFVersions, pdf_ver));

    assert(ver_node != nullptr);
    assert(ver_node->type == ASTNodeType::ASTNT_ConstNum);
    assert(ver_node->node.size() == 3);
    assert(FindInVector(v_ArlPDFVersions, ver_node->node));

    // Convert to 10 * PDF version
    int pdf_v = (pdf_ver[0] - '0') * 10 + (pdf_ver[2] - '0');
    int arl_v = (ver_node->node[0] - '0') * 10 + (ver_node->node[2] - '0');

    if (thing != nullptr) {
        if (pdf_v >= arl_v) {
            ASTNode* out = new ASTNode;
            out->type = thing->type;
            out->node = thing->node;
            return out;
        }
    }
    else { // thing == nullptr
        ASTNode* out = new ASTNode;
        out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
        out->node = (pdf_v >= arl_v) ? "true" : "false";
        return out;
    }
    return nullptr;
}


/// @brief IsPDFVersion means a feature was introduced for only a specific PDF version:
///   - fn:IsRequired(fn:IsPDFVersion(1.0))
///   - fn:IsPDFVersion(1.0,fn:BitsClear(2,32))
/// 
/// @param[in] pdf_ver   version of the PDF file
/// @param[in] ver_node  version when feature 'thing' was introduced from Arlington PDF model
/// @param[in] thing     (optional) the feature that was introduced
ASTNode* fn_IsPDFVersion(const std::string& pdf_ver, const ASTNode* ver_node, const ASTNode* thing) {
    assert(pdf_ver.size() == 3);
    assert(FindInVector(v_ArlPDFVersions, pdf_ver));

    assert(ver_node != nullptr);
    assert(ver_node->type == ASTNodeType::ASTNT_ConstNum);
    assert(ver_node->node.size() == 3);
    assert(FindInVector(v_ArlPDFVersions, ver_node->node));
    assert(ver_node->node == "1.0"); // only use is for PDF 1.0

    // Convert to 10 * PDF version
    int pdf_v = (pdf_ver[0] - '0') * 10 + (pdf_ver[2] - '0');
    int arl_v = (ver_node->node[0] - '0') * 10 + (ver_node->node[2] - '0');
    if (thing != nullptr) {
        if (pdf_v == arl_v) {
            ASTNode* out = new ASTNode;
            out->type = thing->type;
            out->node = thing->node;
            return out;
        }
    }
    else { // thing == nullptr
        ASTNode* out = new ASTNode;
        out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
        out->node = (pdf_v == arl_v) ? "true" : "false";
        return out;
    }
    return nullptr;
}

/// @brief Deprecated predicate. If PDF version is BEFORE Arlington's deprecated version, then return 
/// whatever it was, otherwise return nullptr (meaning `thing` shouldn't exist as it has been deprecated).
///
/// @param[in] pdf_ver  version of the PDF file
/// @param[in] dep_ver  version when deprecated from the Arlington PDF model (1st arg to predicate)
/// @param[in] thing    thing that was deprecated (2nd arg to predicate)      
ASTNode* fn_Deprecated(const std::string& pdf_ver, const ASTNode* dep_ver, const ASTNode* thing) {
    assert(pdf_ver.size() == 3);
    assert(FindInVector(v_ArlPDFVersions, pdf_ver));

    assert(dep_ver != nullptr);
    assert(dep_ver->type == ASTNodeType::ASTNT_ConstNum);
    assert(dep_ver->node.size() == 3);
    assert(FindInVector(v_ArlPDFVersions, dep_ver->node));

    assert(thing != nullptr);

    // Convert to 10 * PDF version
    int pdf_v = (pdf_ver[0] - '0') * 10 + (pdf_ver[2] - '0');
    int arl_v = (dep_ver->node[0] - '0') * 10 + (dep_ver->node[2] - '0');

    if (pdf_v < arl_v) {
        ASTNode* out = new ASTNode;
        out->type = thing->type;
        out->node = thing->node;
        return out;
    }
    return nullptr;
}



int fn_NumberOfPages(ArlPDFObject* obj) {
    assert(obj != nullptr);
    return 99999; /// @todo - determine number of pages in PDF
}


/// @brief Processes an ASTNode by recursively descending and calculating the predicate.
ASTNode* ProcessPredicate(ArlPDFObject* obj, const ASTNode* in_ast, const int key_idx, const ArlTSVmatrix& tsv_data, const int type_idx, bool* fully_processed, int depth = 0) {
    assert(obj != nullptr);
    assert(in_ast != nullptr);
    assert(key_idx >= 0);
    assert(type_idx >= 0);
    assert(fully_processed != nullptr);

    ASTNode* out = new ASTNode;
    ASTNode* out_left = nullptr;
    ASTNode* out_right = nullptr;

#ifdef PP_DEBUG
    std::cout << std::string(depth * 2, ' ') << "In:  " << *in_ast << std::endl;
#endif

    if (in_ast->arg[0] != nullptr) {
        out_left = ProcessPredicate(obj, in_ast->arg[0], key_idx, tsv_data, type_idx, fully_processed, ++depth);
#ifdef PP_DEBUG
        if (out_left != nullptr) { std::cout << std::string(depth * 2, ' ') << " Out-Left:  " << *out_left << std::endl; }
#endif 
    }

    if (in_ast->arg[1] != nullptr) {
        out_right = ProcessPredicate(obj, in_ast->arg[1], key_idx, tsv_data, type_idx, fully_processed, ++depth);
#ifdef PP_DEBUG
        if (out_right != nullptr) { std::cout << std::string(depth * 2, ' ') << " Out-Right:  " << *out_right << std::endl; }
#endif 
    }


    switch (in_ast->type) {
    case ASTNodeType::ASTNT_ConstPDFBoolean:
    case ASTNodeType::ASTNT_ConstString:
    case ASTNodeType::ASTNT_ConstInt:
    case ASTNodeType::ASTNT_ConstNum:
    case ASTNodeType::ASTNT_Key:
        // Primitive type so out = in
        out->type = in_ast->type;
        out->node = in_ast->node;
        break;

    case ASTNodeType::ASTNT_Predicate:
        {
            if (in_ast->node == "fn:ArrayLength(") {
                out->type = ASTNodeType::ASTNT_ConstInt;
                out->node = std::to_string(fn_ArrayLength(obj));
            }
            else if (in_ast->node == "fn:ArraySortAscending(") {
                out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                out->node = fn_ArraySortAscending(obj) ? "true" : "false";
            }
            else if (in_ast->node == "fn:BeforeVersion(") {
                delete out;
                *fully_processed = false; /// @todo 
                out = fn_BeforeVersion("2.0", out_left, out_right);  /// @todo correct PDF version to file version
            }
            else if (in_ast->node == "fn:BitClear(") {
                out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                out->node = fn_BitClear(obj, out_left) ? "true" : "false";
            }
            else if (in_ast->node == "fn:BitSet(") {
                out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                out->node = fn_BitSet(obj, out_left) ? "true" : "false";
            }
            else if (in_ast->node == "fn:BitsClear(") {
                out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                out->node = fn_BitsClear(obj, out_left, out_right) ? "true" : "false";
            }
            else if (in_ast->node == "fn:BitsSet(") {
                out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                out->node = fn_BitsSet(obj, out_left, out_right) ? "true" : "false";
            }
            else if (in_ast->node == "fn:DefaultValue(") {
                out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                out->node = "true"; /// @todo ????
                *fully_processed = false; /// @todo 
            }
            else if (in_ast->node == "fn:Deprecated(") {
                delete out;
                *fully_processed = false; /// @todo 
                out = fn_Deprecated("2.0", out_left, out_right);  /// @todo correct PDF version to file version
            }
            else if (in_ast->node == "fn:Eval(") {
                assert(out_left != nullptr);
                out->type = out_left->type;
                out->node = out_left->node;
            }
            else if (in_ast->node == "fn:FileSize(") {
                out->type = ASTNodeType::ASTNT_ConstInt;
                out->node = std::to_string(fn_FileSize());
                *fully_processed = false; /// @todo 
            }
            else if (in_ast->node == "fn:FontHasLatinChars(") {
                out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                out->node = fn_FontHasLatinChars(obj) ? "true" : "false";
            }
            else if (in_ast->node == "fn:Ignore(") {
                /// @todo - is this correct?
                out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                out->node = "true";
                *fully_processed = false; /// @todo 
            }
            else if (in_ast->node == "fn:ImageIsStructContentItem(") {
                out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                out->node = fn_ImageIsStructContentItem(obj) ? "true" : "false";
                *fully_processed = false; /// @todo 
            }
            else if (in_ast->node == "fn:ImplementationDependent(") {
                /// @todo - is this correct?
                out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                out->node = "true";
            }
            else if (in_ast->node == "fn:InMap(") {
                out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                out->node = fn_InMap(obj, out_left) ? "true" : "false";
                *fully_processed = false; /// @todo 
            }
            else if (in_ast->node == "fn:IsAssociatedFile(") {
                out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                out->node = fn_IsAssociatedFile(obj) ? "true" : "false";
                *fully_processed = false; /// @todo 
            }
            else if (in_ast->node == "fn:IsEncryptedWrapper(") {
                out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                out->node = fn_IsEncryptedWrapper(obj) ? "true" : "false";
                *fully_processed = false; /// @todo 
            }
            else if (in_ast->node == "fn:IsLastInNumberFormatArray(") {
                out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                out->node = fn_IsLastInNumberFormatArray(obj) ? "true" : "false";
                *fully_processed = false; /// @todo 
            }
            else if (in_ast->node == "fn:IsMeaningful(") {
                /// @todo - is this correct?
                out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                out->node = "true";
            }
            else if (in_ast->node == "fn:IsPDFTagged(") {
                out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                out->node = fn_IsPDFTagged(obj) ? "true" : "false";
                *fully_processed = false; /// @todo 
            }
            else if (in_ast->node == "fn:IsPDFVersion(") {
                out = fn_IsPDFVersion("2.0", out_left, out_right);     /// @todo correct PDF version to file version
                *fully_processed = false; /// @todo 
            }
            else if (in_ast->node == "fn:IsPresent(") {
                out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                out->node = "true"; /// @todo /////////////////////////////////////////////////////
                *fully_processed = false; /// @todo 
            }
            else if (in_ast->node == "fn:IsRequired(") {
                out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                out->node = "false"; /// @todo /////////////////////////////////////////////////////
                *fully_processed = false; /// @todo 
            }
            else if (in_ast->node == "fn:KeyNameIsColorant(") {
                out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                out->node = "true"; /// @todo /////////////////////////////////////////////////////
                *fully_processed = false; /// @todo 
            }
            else if (in_ast->node == "fn:MustBeDirect(") {
                out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                out->node = fn_MustBeDirect(obj) ? "true" : "false";
            }
            else if (in_ast->node == "fn:NoCycle(") {
                out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                out->node = fn_NoCycle(obj) ? "true" : "false";
                *fully_processed = false; /// @todo 
            }
            else if (in_ast->node == "fn:NotInMap(") {
                out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                out->node = fn_NotInMap(obj) ? "true" : "false";
                *fully_processed = false; /// @todo 
            }
            else if (in_ast->node == "fn:NotPresent(") {
                out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                out->node = "true"; /// @todo /////////////////////////////////////////////////////
                *fully_processed = false; /// @todo 
            }
            else if (in_ast->node == "fn:NotStandard14Font(") {
                out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                out->node = fn_NotStandard14Font(obj) ? "true" : "false";
            }
            else if (in_ast->node == "fn:NumberOfPages(") {
                out->type = ASTNodeType::ASTNT_ConstInt;
                out->node = std::to_string(fn_NumberOfPages(obj));
                *fully_processed = false; /// @todo 
            }
            else if (in_ast->node == "fn:PageContainsStructContentItems(") {
                out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                out->node = fn_PageContainsStructContentItems(obj) ? "true" : "false";
                *fully_processed = false; /// @todo 
            }
            else if (in_ast->node == "fn:PageProperty(") {
                out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                out->node = fn_PageProperty(obj, out_right) ? "true" : "false";
                *fully_processed = false; /// @todo 
            }
            else if (in_ast->node == "fn:RectHeight(") {
                out->type = ASTNodeType::ASTNT_ConstNum;
                out->node = std::to_string(fn_RectHeight(obj));
            }
            else if (in_ast->node == "fn:RectWidth(") {
                out->type = ASTNodeType::ASTNT_ConstNum;
                out->node = std::to_string(fn_RectWidth(obj));
            }
            else if (in_ast->node == "fn:RequiredValue(") {
                out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                out->node = fn_RequiredValue(obj, out_left, out_right) ? "true" : "false";
                *fully_processed = false; /// @todo 
                delete out_left;
                delete out_right;
                out_left = out_right = nullptr;
            }
            else if (in_ast->node == "fn:SinceVersion(") {
                delete out;
                out = fn_SinceVersion("2.0", out_left, out_right);     /// @todo correct PDF version to file version
                *fully_processed = false; /// @todo 
            }
            else if (in_ast->node == "fn:StreamLength(") {
                out->type = ASTNodeType::ASTNT_ConstInt;
                out->node = std::to_string(fn_StreamLength(obj));
            }
            else if (in_ast->node == "fn:StringLength(") {
                out->type = ASTNodeType::ASTNT_ConstInt;
                out->node = std::to_string(fn_StringLength(obj));
            }
            else {
                assert(false && "unrecognized predicate function!");
                *fully_processed = false; /// @todo 
                delete out;
                delete out_left;
                delete out_right;
                return nullptr;
            }
        }
        break;

    case ASTNodeType::ASTNT_MathComp:
        // Math comparison operators - cannot be start of an AST!
        assert((out_left != nullptr) && (out_right != nullptr));
        out->type = ASTNodeType::ASTNT_ConstPDFBoolean;

        if (in_ast->node == "==") {
            // equality - could be numeric, logical, etc.
            out->node = (out_left->node == out_right->node) ? "true" : "false";
        }
        else if (in_ast->node == "!=") {
            // inequality - could be numeric, logical, etc.
            out->node = (out_left->node != out_right->node) ? "true" : "false";
        }
        else {
            double left = 0.0;
            double right = 0.0;

            try {
                left = std::stod(out_left->node);
            }
            catch (...) {
#ifdef PP_DEBUG
                std::cout << "left side floating point exception for " << out_left->node << "!" << std::endl;
#endif
                delete out;
                delete out_left;
                delete out_right;
                return nullptr;
            }
            try {
                right = std::stod(out_right->node);
            }
            catch (...) {
#ifdef PP_DEBUG
                std::cout << "right side floating point exception for " << out_right->node << "!" << std::endl;
#endif
                delete out;
                delete out_left;
                delete out_right;
                return nullptr;
            }

            if (in_ast->node == "<=") {
                // less than or equal to (numeric only)
                out->node = (left <= right) ? "true" : "false";
            }
            else if (in_ast->node == "<") {
                // less than (numeric only)
                out->node = (left < right) ? "true" : "false";
            }
            else if (in_ast->node == ">=") {
                // greater than or equal to (numeric only)
                out->node = (left >= right) ? "true" : "false";
            }
            else if (in_ast->node == ">") {
                // greater than (numeric)
                out->node = (left > right) ? "true" : "false";
            }
            else {
                assert(false && "unexpected math comparison!");
                delete out;
                delete out_left;
                delete out_right;
                return nullptr;
            }
        }
        break;

    case ASTNodeType::ASTNT_MathOp:
        {
            // Math operators: +, -, *, mod
            assert((out_left != nullptr) && (out_right != nullptr));
            double left = std::stod(out_left->node);
            double right = std::stod(out_right->node);

            // Work out typing - integer vs number
            if ((in_ast->arg[0]->type == ASTNodeType::ASTNT_ConstInt) && (in_ast->arg[1]->type == ASTNodeType::ASTNT_ConstInt))
                out->type = ASTNodeType::ASTNT_ConstInt;
            else
                out->type = ASTNodeType::ASTNT_ConstNum;

            if (in_ast->node == " + ") {
                // addition
                if (out->type == ASTNodeType::ASTNT_ConstInt)
                    out->node = std::to_string(int(left + right));
                else
                    out->node = std::to_string(left + right);
            }
            else if (in_ast->node == " - ") {
                // subtraction
                if (out->type == ASTNodeType::ASTNT_ConstInt)
                    out->node = std::to_string(int(left - right));
                else
                    out->node = std::to_string(left - right);
            }
            else if (in_ast->node == " * ") {
                // multiply
                if (out->type == ASTNodeType::ASTNT_ConstInt)
                    out->node = std::to_string(int(left * right));
                else
                    out->node = std::to_string(left * right);
            }
            else if (in_ast->node == " mod ") {
                // modulo
                out->type = ASTNodeType::ASTNT_ConstInt;
                out->node = std::to_string(int(left) % int(right));
            }
            else {
                assert(false && "unexpected math operator!");
                delete out;
                delete out_left;
                delete out_right;
                return nullptr;
            }
        }
        break;

    case ASTNodeType::ASTNT_LogicalOp:
        {
            // Logical operators - should have 2 operands (left, right) but due to version-based predicates
            // this can reduce to just one in which case the output is just the non-nullptr boolean.
            if ((out_left != nullptr) && (out_right == nullptr)) {
                assert(out_left->type == ASTNodeType::ASTNT_ConstPDFBoolean);
                out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                out->node = out_left->node;
                break;
            }
            else if ((out_left == nullptr) && (out_right != nullptr)) {
                assert(out_right->type == ASTNodeType::ASTNT_ConstPDFBoolean);
                out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                out->node = out_right->node;
                break;
            }

            assert((out_left != nullptr) && (out_right != nullptr));
            assert((out_left->type == ASTNodeType::ASTNT_ConstPDFBoolean) && (out_right->type == ASTNodeType::ASTNT_ConstPDFBoolean));
            if (in_ast->node == " && ") {
                // logical AND
                out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                out->node = ((out_left->node == "true") && (out_right->node == "true")) ? "true" : "false";
            }
            else if (in_ast->node == " || ") {
                // logical OR
                out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                out->node = ((out_left->node == "true") || (out_right->node == "true")) ? "true" : "false";
            }
            else {
                assert(false && "unexpected logical operator!");
                delete out;
                delete out_left;
                delete out_right;
                return nullptr;
            }
        }
        break;

    case ASTNodeType::ASTNT_KeyValue: // @keyname
        {
            assert(in_ast->node[0] == '@');
            std::string key = in_ast->node.substr(1);

            bool self_refer = (tsv_data[key_idx][TSV_KEYNAME] == key);

            if (!self_refer) {
                /// @todo - need to get the parent object
                out->type = ASTNodeType::ASTNT_Key;
                out->node = key;
                break;
            }

            // self-reference: obj is the correct object
            PDFObjectType obj_type = obj->get_object_type();
            switch (obj_type) {
                case PDFObjectType::ArlPDFObjTypeName:
                {
                    out->type = ASTNodeType::ASTNT_Key;
                    out->node = ToUtf8(((ArlPDFName*)obj)->get_value());
                }
                break;

                case PDFObjectType::ArlPDFObjTypeNumber:
                    if (((ArlPDFNumber*)obj)->is_integer_value()) {
                        out->type = ASTNodeType::ASTNT_ConstInt;
                        out->node = std::to_string(((ArlPDFNumber*)obj)->get_integer_value());
                    }
                    else {
                        out->type = ASTNodeType::ASTNT_ConstNum;
                        out->node = std::to_string(((ArlPDFNumber*)obj)->get_value());
                    }
                break;

                case PDFObjectType::ArlPDFObjTypeBoolean:
                {
                    out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                    out->node = std::to_string(((ArlPDFBoolean*)obj)->get_value());
                }
                break;

                case PDFObjectType::ArlPDFObjTypeString:
                {
                    out->type = ASTNodeType::ASTNT_ConstString;
                    out->node = ToUtf8(((ArlPDFString*)obj)->get_value());
                }
                break;

                case PDFObjectType::ArlPDFObjTypeDictionary:
                {
                    ArlPDFObject* val = ((ArlPDFDictionary*)obj)->get_value(ToWString(key));
                    if (val != nullptr) {
                        PDFObjectType val_type = val->get_object_type();
                        /// @todo 
                    }
                    else {
                        delete out;
                        delete out_left;
                        delete out_right;
                        return nullptr;
                    }
                }
                break;

                case PDFObjectType::ArlPDFObjTypeStream:
                {
                    ArlPDFObject* val = ((ArlPDFStream*)obj)->get_dictionary()->get_value(ToWString(key));
                    if (val != nullptr) {
                        PDFObjectType val_type = val->get_object_type();
                        /// @todo 
                    }
                    else {
                        delete out;
                        delete out_left;
                        delete out_right;
                        return nullptr;
                    }
                }
                break;

                case PDFObjectType::ArlPDFObjTypeArray:
                    try {
                        int idx = std::stoi(key);
                        ArlPDFObject* val = ((ArlPDFArray*)obj)->get_value(idx);
                        if (val != nullptr) {
                            PDFObjectType val_type = val->get_object_type();
                            /// @todo 
                        }
                        else {
                            delete out;
                            delete out_left;
                            delete out_right;
                            return nullptr;
                        }
                    }
                    catch (...) {  // Conversion of key as an array index integer failed!
                        delete out;
                        delete out_left;
                        delete out_right;
                        return nullptr;
                    }
                    break;

                case PDFObjectType::ArlPDFObjTypeNull:
                case PDFObjectType::ArlPDFObjTypeUnknown:
                case PDFObjectType::ArlPDFObjTypeReference:
                default:
                    assert(false && "unexpected key-value type!");
                    delete out;
                    delete out_left;
                    delete out_right;
                    return nullptr;
            } // switch obj_type
        }
        break;

    case ASTNodeType::ASTNT_Unknown:
    case ASTNodeType::ASTNT_Type:
    default:
        // Likely a parsing error!
        delete out;
        delete out_left;
        delete out_right;
        assert(false && "unexpected AST node while recursing!");
        return nullptr;
    } // switch

    if (out != nullptr) {
        out->arg[0] = out_left;
        out->arg[1] = out_right;
#ifdef PP_DEBUG
        std::cout << std::string(depth * 2, ' ') << "Out: " << *out << std::endl;
#endif
        assert(out->valid());
    }
    else {
        delete out_left;
        delete out_right;
#ifdef PP_DEBUG
        std::cout << std::string(depth * 2, ' ') << "Out: nullptr" << std::endl;
#endif 
    }

    return out;
}


/// @brief Reduces an Arlington "PossibleValues" row (column 9)
/// Can be pretty much anything.
///
/// @returns true if syntax is valid. false otherwise
bool PossibleValuesPredicateProcessor::ReduceRow(ArlPDFObject* object, const int key_idx, const ArlTSVmatrix& tsv_data, const int type_idx, bool* fully_processed) {
    assert(object != nullptr);
    assert(key_idx >= 0);
    assert(type_idx >= 0);
    assert(fully_processed != nullptr);

    *fully_processed = true; // assume all predicates will get fully processed

    if (tsv_field == "")
        return true;

    /// Split on SEMI-COLON
    std::vector<std::string> pv_list = split(tsv_field, ';');

    // Complex types (arrays, dicts, streams) are just "[]" so this reduces away
    assert((type_idx >= 0) && (type_idx < pv_list.size()));
    if (pv_list[type_idx] == "[]")
        return true;

    ASTNodeStack stack;

    if (predicate_ast.empty())
        for (auto& pv : pv_list) {
            stack.clear();
            if (pv.find("fn:") != std::string::npos) {
                int loop = 0;
                std::string s = pv.substr(1, pv.size() - 2); // strip off '[' and ']'
                do {
                    ASTNode* n = new ASTNode();
                    s = LRParsePredicate(s, n);
                    stack.push_back(n);
                    loop++;
                    while (!s.empty() && ((s[0] == ',') || (s[0] == ' '))) {
                        s = s.substr(1, s.size() - 1); // skip over COMMAs and SPACEs
                    }
                } while (!s.empty() && (loop < 100));
                if (loop >= 100) {
                    assert(false && "Arlington complex type PossibleValues field too long and complex when reducing!");
                    *fully_processed = false;
                    return false;
                }
            }
            predicate_ast.push_back(stack);
        }

    // There should now be a vector of ASTs or nullptr for each type of the TSV field
    assert(predicate_ast.size() == pv_list.size());
    assert(type_idx < predicate_ast.size());
    assert(pv_list[type_idx][0] == '[');

    std::string s = pv_list[type_idx].substr(1, pv_list[type_idx].size() - 2); // strip off '[' and ']'

    if (predicate_ast[type_idx].empty() || (predicate_ast[type_idx][0] == nullptr)) {
        // No predicates - but could be a set of COMMA-separated constants (e.g. names, integers, etc.)
        return IsValidValue(object, s);
    }

    // At least one predicate was in the COMMA list of Possible Values
    // Walk vector of ASTs
#ifdef PP_DEBUG
    std::cout << std::endl << s << std::endl;
#endif 
    stack = predicate_ast[type_idx];
    for (auto i = 0; i < stack.size(); i++) {
        ASTNode* n = stack[i];

        switch (n->type) {
        case ASTNodeType::ASTNT_ConstPDFBoolean:
        case ASTNodeType::ASTNT_ConstString:
        case ASTNodeType::ASTNT_ConstInt:
        case ASTNodeType::ASTNT_ConstNum:
        case ASTNodeType::ASTNT_Key:
            // Primitive type means this is a NON-predicate value so see if it is a match
            // otherwise loop and keep trying...
            assert((n->arg[0] == nullptr) && (n->arg[1] == nullptr));
            if (IsValidValue(object, n->node))
                return true;
            break;

        case ASTNodeType::ASTNT_Predicate:
            {
                ASTNode *pp = ProcessPredicate(object, n, key_idx, tsv_data, type_idx, fully_processed);
                if (pp != nullptr) {
                    bool vv = IsValidValue(object, pp->node);
                    switch (pp->type) {
                        case ASTNodeType::ASTNT_ConstPDFBoolean:
                            // Booleans can either be a valid value OR the result of a predicate calculation
                            if (!vv && (pp->node == "true")) 
                                vv = true;
                            delete pp;
                            return vv;
                        case ASTNodeType::ASTNT_ConstString:
                        case ASTNodeType::ASTNT_ConstInt:
                        case ASTNodeType::ASTNT_ConstNum:
                        case ASTNodeType::ASTNT_Key:
                            delete pp;
                            if (vv) 
                                return true;
                            break;
                        default:
                            assert(false && "unexpected node type from ProcessPredicate!");
                            delete pp;
                            *fully_processed = false;
                            return false;
                    } // switch
                } // if 
            }
            break;

        case ASTNodeType::ASTNT_MathComp:  // Math comparison operators - cannot be start of an AST!
        case ASTNodeType::ASTNT_MathOp:    // Math operators - cannot be start of an AST!
        case ASTNodeType::ASTNT_LogicalOp: // Logical operators - cannot be start of an AST!
        case ASTNodeType::ASTNT_KeyValue:  // @keyname - cannot be start of an AST! Needs to be wrapped in fn:Eval()
        case ASTNodeType::ASTNT_Unknown:
        case ASTNodeType::ASTNT_Type:
        default:
            // Likely a parsing error or bad Arlington data! Check via "--validate" CLI option
            assert(false && "unexpected AST node when reducing Possible Values!");
            *fully_processed = false;
            return false;
        } // switch
    } // for
    return false;
}
