///////////////////////////////////////////////////////////////////////////////
/// @file
/// @brief A left-to-right, recursive descent regex-based parser for Arlington predicates.
///
/// @copyright
/// Copyright 2022 PDF Association, Inc. https://www.pdfa.org
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

#include "LRParsePredicate.h"
#include "ArlPredicates.h"
#include "utils.h"

#include <iterator>
#include <regex>
#include <cassert>
#include <math.h>
#include <algorithm>

/// @def \#define ARL_PARSER_DEBUG to enable very verbose debugging of predicate and expression parsing
#undef ARL_PARSER_DEBUG


/// @def \#define PP_DEBUG to get VERY verbose debugging of predicate processing (PP)
#undef PP_DEBUG


/// @brief Regex to process "Links" fields
/// - $1 = predicate name
/// - $2 = single Link (TSV filename) or single Arlington predefined Type
const std::regex  r_Links("fn:(SinceVersion|Deprecated|BeforeVersion|IsPDFVersion)\\(" + ArlPDFVersion + "\\,([a-zA-Z0-9_.]+)\\)");


/// @brief Regex to process "Types" fields
/// - $1 = predicate name
/// - $2 = single Link (TSV filename) or single Arlington predefined Type
const std::regex  r_Types("fn:(SinceVersion|Deprecated|BeforeVersion|IsPDFVersion)\\(" + ArlPDFVersion + "\\,([a-z\\-]+)\\)");


/// @brief Regex to process "Key" fields
/// Alphanumeric, integer, ASTERISK or \<digit\>+ASTERISK
/// - $1 = key name
const std::regex  r_Keys("(\\*|[0-9]+|[0-9]+\\*|[a-zA-Z0-9\\-\\._]+)");



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

                assert(s.size() > 0);
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
            assert((s.size() > 0) && (s[0] == ')'));
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
    while ((loop > 0) && ((nested_expressions > 0) || ((s.size() >0) && (s[0] != ',') && (s[0] != ')'))));

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

    if (s.size() == 0)
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

            assert(s.size() > 0);
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
        assert((s.size() > 0) && (s[0] == ')'));
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
