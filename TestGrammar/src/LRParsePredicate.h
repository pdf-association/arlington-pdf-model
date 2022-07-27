///////////////////////////////////////////////////////////////////////////////
/// @file
/// @brief Arlington Left-to-Right predicate parser to make ASTs
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

#ifndef LRParsePredicate_h
#define LRParsePredicate_h
#pragma once

#include "ASTNode.h"

#include <string>
#include <regex>


extern const std::regex  r_Links;
extern const std::regex  r_Types;
extern const std::regex  r_Keys;

/// @brief Left-to-right recursive descent parser, based on regex pattern matching
std::string LRParsePredicate(std::string s, ASTNode *root);

#endif // LRParsePredicate_h
