///////////////////////////////////////////////////////////////////////////////
/// @file
/// @brief Various definitions for Arlington predicates
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

#ifndef ArlPredicates_h
#define ArlPredicates_h
#pragma once

#include <string>
#include <vector>
#include <regex>

/// @brief Arlington PDF version regex (1.0, 1.1, ... 1.7, 2.0).
const std::string  ArlPDFVersion = "(1\\.[0-7]|2\\.0)";

// @brief full set of Arlington supported PDF versions (numerically pre-sorted vector)
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

/// @brief All Arlington pre-defined types (alphabetically pre-sorted vector)
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

/// @brief Arlingon pre-defined types which REQUIRE a Link - aka "Complex types" (alphabetically pre-sorted vector)
const std::vector<std::string>  v_ArlComplexTypes = {
    "array",
    "dictionary",
    "name-tree",
    "number-tree",
    "stream"
};

/// @brief Arlington pre-defined types that must NOT have Links - aka "Non-complex types" (alphabetically pre-sorted vector)
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


/// @brief Arlington Integer - only optional leading negative sign.
/// Avoid matching the front part of keys that start with digits "3DRenderMode" and array indexed wildcards "1*"
const std::string ArlInt = "(\\-)?[0-9]+(?![a-zA-Z\\*])";

/// @brief Arlington Number (requires at least 1 decimal place either side of decimal point ".")
const std::string ArlNum = ArlInt + "\\.[0-9]+(?![a-zA-Z\\*])";

/// @brief Arlington PDF Strings use single quotes (to disambiguate from bracketed names, keys, etc.).
/// Empty strings are invalid (same as empty field in Arlington). No escapes supported.
const std::string ArlString = "'[^']+'";

/// @brief Arlington key or array index regex, including path separator "::" and wildcards.
/// Intersects with ArlLink and ArlPredfinedType.
/// Examples: SomeKey, 3, *, 2*, parent::SomeKey, SomeKeyA::SomeKeyB::3, SomeKeyA::SomeKeyB::\@SomeKeyC,
const std::string  ArlKeyBase = "[a-zA-Z0-9_\\.]+";
const std::string  ArlKey = "([a-zA-Z]+::)*(" + ArlKeyBase + "|[0-9]+(\\*)?|\\*)+";
const std::string  ArlKeyValue = "(([a-zA-Z0-9]+::)*)@(" + ArlKeyBase + "|([0-9]+(\\*)?)+|\\*)+";


/// @brief pre-defined Arlington Types (all lowercase with some sub-types include DASH and qualifier).
/// Intersects with ArlLink and ArlKeyBase.
const std::string ArlPredfinedType = "(array|bitmask|boolean|date|dictionary|integer|matrix|name|name-tree|null|number-tree|number|rectangle|stream|string-ascii|string-byte|string-text|string)";

/// @brief Arlington Link name (i.e. TSV filename without extension). Only UNDERBAR allowed. Never DASH or PERIOD.
/// Intersects with ArlPredfinedType and ArlKeyBase.
const std::string ArlLink = "[a-zA-Z0-9_]+";

/// @brief Arlington math comparisons - currently NOT required to have SPACE either side
const std::string ArlMathComp = "(==|!=|>=|<=|>|<)";

/// @brief Arlington math operators - MULTIPLY and MINUS need a SPACE either side
///        to disambiguate from keys with wildcards and negative numbers
const std::string ArlMathOp = "( \\* |\\+| \\- | mod )";

/// @brief Arlington logical operators. Require SPACE either side.
/// Also expect bracketed expressions either side or a predicate:
/// e.g. "...) || (..." or "...) || fn:..."
const std::string ArlLogicalOp = "( && | \\|\\| )";

/// @brief Arlington PDF boolean keywords (case sensitive)
const std::string ArlBooleans = "(true|false)";

/// @brief Tolerance for floating-point equality and inequality comparison.
/// Old Adobe PDF specs used to recommend 5 digits so go +/- half of that
const double ArlNumberTolerance = 0.000005;


extern const std::regex  r_Types;
extern const std::regex  r_Keys;

/// @brief Regexes for matching versioning predicates
extern const std::regex  r_sinceVersionExtension;
extern const std::regex  r_sinceVersion;
extern const std::regex  r_beforeVersion;
extern const std::regex  r_Deprecated;
extern const std::regex  r_isPDFVersion;
extern const std::regex  r_LinkExtension;


#endif // ArlPredicates_h
