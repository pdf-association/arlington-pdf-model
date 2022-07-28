///////////////////////////////////////////////////////////////////////////////
/// @file
/// @brief ASTNode class declaration
///
/// Simple AST Node class and related types for building parsed predicate AST trees
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

#ifndef ASTNode_h
#define ASTNode_h
#pragma once

#include "utils.h"

#include <string>
#include <iostream>
#include <vector>

/// @enum ASTNodeType 
/// AST Node types (based on regex matches)
enum class ASTNodeType {
    ASTNT_Unknown = 0,
    ASTNT_Predicate,
    ASTNT_MathComp,
    ASTNT_MathOp,
    ASTNT_LogicalOp,
    ASTNT_ConstPDFBoolean,
    ASTNT_ConstString,
    ASTNT_ConstInt,
    ASTNT_ConstNum,     // also matches a PDF version
    ASTNT_Key,          // also matches Arlington Link (TSV filename)
    ASTNT_KeyValue,
    ASTNT_Type
};


/// @brief Human readable strings of enum class ASTNodeType
static const std::string ASTNodeType_strings[] = {
    "???",
    "Predicate",
    "MathComp",
    "MathOp",
    "LogicalOp",
    "Boolean",
    "String",
    "Integer",
    "Number",
    "Key",
    "KeyValue",
    "Type"
};


/// @brief Predicate parser creates a binary tree of these simple ASTNodes
struct ASTNode {
    /// @brief predicate operator or operand
    std::string     node;

    /// @brief type of operator/operand
    ASTNodeType     type;

    /// @brief Optional arguments for operators (left ptr, right ptr)
    ASTNode         *arg[2];

    /// @brief Constructor that takes a parent ptr [NOT USED]
    ASTNode(ASTNode *parent = nullptr)
        { /* constructor */ UNREFERENCED_FORMAL_PARAM(parent);
          arg[0] = arg[1] = nullptr; type = ASTNodeType::ASTNT_Unknown; }

    /// @brief Destructor
    ~ASTNode() {
        /* destructor - recursive */
        if (arg[0] != nullptr) delete arg[0];
        if (arg[1] != nullptr) delete arg[1];
    }

    /// @brief assignment operator =
    ASTNode& operator=(const ASTNode& n) {
        node   = n.node;
        type   = n.type;
        arg[0] = n.arg[0];
        arg[1] = n.arg[1];
        return *this;
    }

    /// @brief output operator <<
    friend std::ostream& operator<<(std::ostream& ofs, const ASTNode& n) {
        if (n.node.size() > 0)
            ofs << "{" << ASTNodeType_strings[(int)n.type] << ":'" << n.node << "'";
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
        bool ret_val = (node.size() > 0);
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


/// @brief A vector (stack) of AST-Nodes
typedef std::vector<ASTNode*>      ASTNodeStack;


/// @brief A vector of vector of AST-Nodes
typedef std::vector<ASTNodeStack>  ASTNodeMatrix;

#endif // ASTNode_h
