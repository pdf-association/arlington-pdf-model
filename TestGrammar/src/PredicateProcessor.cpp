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

#include "PredicateProcessor.h"
#include "ArlPredicates.h"
#include "PDFFile.h"
#include "utils.h"

//#include <exception>
#include <iterator>
#include <algorithm>
#include <regex>
#include <cassert>
#include <math.h>


/// @def define ARL_PARSER_DEBUG to enable very verbose debugging of predicate and expression parsing
#undef ARL_PARSER_DEBUG

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
            int pdf_v = pdfc->pdf_version[0] * 10 + pdfc->pdf_version[2];
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

    int pdf_v = pdfc->pdf_version[0] * 10 + pdfc->pdf_version[2];
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
    assert(pdfc->pdf_version.size() == 3);

    int pdf_v = pdfc->pdf_version[0] * 10 + pdfc->pdf_version[2];
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
    assert(obj != nullptr);
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
        /// @todo PROCESS THE AST and apply to PDF "obj"! And delete CParsePDF::is_required_key()
        return false;
    }
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
        assert((ast->node == "fn:MustBeDirect(") || (ast->node == "fn:MustBeIndirect("));
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
/// - fn:MustBeIndirect(...)
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
        assert(type_index < (int)indirect_list.size());
        if (indirect_list[type_index] == "[TRUE]")
            return ReferenceType::MustBeIndirect;
        else if (indirect_list[type_index] == "[FALSE]")
            return ReferenceType::DontCare;
        else { 
            assert(false && "unexpected predicate in a complex IndirectRef field [];[];[] : ");
        }
    }
    else {
        ;
        if (predicate_ast.empty()) {
            ASTNodeStack stack;

            ASTNode* ast = new ASTNode();
            std::string whats_left = LRParsePredicate(tsv_field, ast);
            assert((ast->node == "fn:MustBeDirect(") || (ast->node == "fn:MustBeIndirect("));
            predicate_ast.clear();
            stack.push_back(ast);
            predicate_ast.push_back(stack);
            assert(whats_left.size() == 0);
            delete ast;
        }
        /// @todo PROCESS AST in preduicate_ast for fn:MustBeDirect(...) or fn:MustBeIndirect(...)
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
            int pdf_v = pdfc->pdf_version[0] * 10 + pdfc->pdf_version[2];
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
    assert((type_idx >= 0) && (type_idx < (int)pv_list.size()));
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
    assert(type_idx < (int)predicate_ast.size());
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
    for (auto i = 0; i < (int)stack.size(); i++) {
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
                ASTNode *pp = pdfc->ProcessPredicate(object, n, key_idx, tsv_data, type_idx, fully_processed);
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
