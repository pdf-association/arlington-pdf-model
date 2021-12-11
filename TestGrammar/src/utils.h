///////////////////////////////////////////////////////////////////////////////
// utils.h
// Copyright 2020-2021 PDF Association, Inc. https://www.pdfa.org
//
// This material is based upon work supported by the Defense Advanced
// Research Projects Agency (DARPA) under Contract No. HR001119C0079.
// Any opinions, findings and conclusions or recommendations expressed
// in this material are those of the author(s) and do not necessarily
// reflect the views of the Defense Advanced Research Projects Agency
// (DARPA). Approved for public release.
//
// SPDX-License-Identifier: Apache-2.0
// Contributors: Roman Toda, Frantisek Forgac, Normex. Peter Wyatt, PDF Association
//
///////////////////////////////////////////////////////////////////////////////

#ifndef Utils_h
#define Utils_h
#pragma once

#include <string>
#include <filesystem>

/// @brief /dev/null equivalents
extern std::ostream  cnull;
extern std::wostream wcnull;

/// @brief string conversion to/from wide strings needed by PDF files
std::string  ToUtf8(const std::wstring& str);
std::wstring utf8ToUtf16(const std::string& utf8Str);
std::wstring ToWString(const std::string& s);

/// @brief check if a folder, or if a file
bool is_folder(const std::filesystem::path& p);
bool is_file(const std::filesystem::path& p);

/// @brief split Arlington-style strings into vector based on separator character
std::vector<std::string> split(const std::string& s, char separator);

/// @brief Arlington predicate support
std::string remove_link_predicates(const std::string& link_in);
std::string remove_type_predicates(const std::string& types_in);

int get_type_index(std::string single_type, std::string types);
std::string get_link_for_type(std::string single_type, const std::string& types, const std::string& links);

/// @brief Strip leading whitespace
std::string& strip_leading_whitespace(std::string& str);

/// @brief Case insensitive comparison of two strings
bool iequals(const std::string& a, const std::string& b);

/// @brief Case insensitive substring match of s1 in s
bool icontains(const std::string& s, const std::string& s1);

/// @brief Finds a string in a vector of strings
bool FindInVector(const std::vector<std::string> list, const std::string v);

/// @brief Check if Arlington data represents an array
bool check_valid_array_definition(const std::string& fname, const std::vector<std::string>& keys, std::ostream& ofs, bool* wildcard_only);

// @brief Tests if a key is a valid PDF second or third class name according to Annex E of ISO 32000-2:2020
bool is_second_or_third_class_pdf_name(const std::string key);

#endif // Utils_h
