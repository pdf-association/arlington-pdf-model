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

#pragma once

#include <string>
#include <filesystem>

std::string  ToUtf8(const std::wstring& str);
std::wstring utf8ToUtf16(const std::string& utf8Str);
std::wstring ToWString(const std::string& s);

// gets folder from provided filename
std::string  get_path_dir(const std::string& path);
std::wstring get_path_dir(const std::wstring& path);

// check if a folder, or if a file, or if file/folder already exists
bool is_folder(const std::filesystem::path& p);
bool is_file(const std::filesystem::path& p);

// split string into vector
std::vector<std::string> split(const std::string& s, char separator);

//std::string extract_link(std::string link);
std::string extract_function(const std::string& value, std::string& function);
int get_type_index(std::string single_type, std::string types);
std::string get_link_for_type(std::string single_type, const std::string& types, const std::string& links);

// Strip leading whitespace
std::string& strip_leading_whitespace(std::string& str);

// Case insensitive comparison of two strings
bool iequals(const std::string& a, const std::string& b);
