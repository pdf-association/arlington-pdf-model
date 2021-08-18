///////////////////////////////////////////////////////////////////////////////
// Utils.cpp
// Copyright 2020 PDF Association, Inc. https://www.pdfa.org
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

#include <string>
#include <iostream>
#include <locale>
#include <codecvt>
#include <math.h>
#include <algorithm>
#include <regex>
#ifdef _WIN32
#include <Windows.h>
extern HINSTANCE ghInstance;
#else
#include <cstring>
#include <limits.h>
#include <locale>
#include <sys/stat.h>
#endif


#include "utils.h"

///
/// @brief Converts a Unicode string to UTF8
/// @param[in] unicode Unicode input
/// @return equivalent UTF8 string
/// 
std::string ToUtf8(const wchar_t unicode) {
  std::string out;
  if ((unsigned int)unicode < 0x80) {
    out.push_back((char)unicode);
  }
  else {
    if ((unsigned int)unicode >= 0x80000000) {
      return out;
    }
    int nbytes = 0;
    if ((unsigned int)unicode < 0x800) {
      nbytes = 2;
    }
    else if ((unsigned int)unicode < 0x10000) {
      nbytes = 3;
    }
    else if ((unsigned int)unicode < 0x200000) {
      nbytes = 4;
    }
    else if ((unsigned int)unicode < 0x4000000) {
      nbytes = 5;
    }
    else {
      nbytes = 6;
    }
    static uint8_t prefix[] = { 0xc0, 0xe0, 0xf0, 0xf8, 0xfc };
    int order = 1 << ((nbytes - 1) * 6);
    int code = unicode;
    out.push_back((char)(prefix[nbytes - 2] | (code / order)));
    for (int i = 0; i < nbytes - 1; i++) {
      code = code % order;
      order >>= 6;
      out.push_back((char)(0x80 | (code / order)));
    }
  }
  return out;
}

///
/// @brief Converts a Unicode string to UTF8
/// @param[in] unicode Unicode input
/// @return equivalent UTF8 string
///
std::string ToUtf8(const std::wstring& wstr) {
    const wchar_t* buffer = wstr.c_str();
    auto len = wcslen(buffer);
    std::string out;
    while (len-- > 0)
        out.append(ToUtf8(*buffer++));
    return out;
}

/// @brief 
/// @param str 
/// @return 
std::wstring utf8ToUtf16(const std::string& str) {
  const char* s = str.c_str();
  typedef unsigned char byte;
  struct Level {
    byte Head, Data, Null;
    Level(byte h, byte d) {
      Head = h; // the head shifted to the right
      Data = d; // number of data bits
      Null = h << d; // encoded byte with zero data bits
    }
    bool encoded(byte b) { return b >> Data == Head; }
  }; // struct Level
  Level lev[] = {
    Level(2, 6),
    Level(6, 5),
    Level(14, 4),
    Level(30, 3),
    Level(62, 2),
    Level(126, 1)
  };

  wchar_t wc = 0;
  const char* p = s;
  std::wstring result;
  while (*p != 0) {
    byte b = *p++;
    if (b >> 7 == 0) { // deal with ASCII
      wc = b;
      result.push_back(wc);
      continue;
    } // ASCII
    bool found = false;
    for (int i = 1; i < (sizeof(lev) / sizeof(lev[0])); ++i) {
      if (lev[i].encoded(b)) {
        wc = b ^ lev[i].Null; // remove the head
        wc <<= lev[0].Data * i;
        for (int j = i; j > 0; --j) { // trailing bytes
          if (*p == 0) return result; // unexpected
          b = *p++;
          if (!lev[0].encoded(b)) // encoding corrupted
            return result;
          wchar_t tmp = b ^ lev[0].Null;
          wc |= tmp << lev[0].Data * (j - 1);
        } // trailing bytes
        result.push_back(wc);
        found = true;
        break;
      } // lev[i]
    }   // for lev
    if (!found) return result; // encoding incorrect
  }   // while
  return result;
}

/// @brief 
/// 
/// @param path 
/// @return 
std::string get_path_dir(const std::string& path) {
  auto pos = path.find_last_of("\\/");
  if (pos == std::string::npos) 
    return path;
  std::string dir(path.begin(), path.begin() + pos);
  return dir;
}

/// @brief 
/// @param path 
/// @return 
std::wstring get_path_dir(const std::wstring& path) {
  auto pos = path.find_last_of(L"\\/");
  if (pos == std::wstring::npos) return path;
  std::wstring dir(path.begin(), path.begin() + pos);
  return dir;
}




/// @brief   Checks if a path is a folder (directory)
/// @param   p[in]  path 
/// @return  true if path p is a folder
bool is_folder(const std::filesystem::path& p)
{
    std::filesystem::file_status    st(std::filesystem::status(p));

    return (std::filesystem::is_directory(st));
}


/// @brief   Checks if a path is a file (not a folder)
/// @param   p[in]  path 
/// @return  true if path p is a regular file
bool is_file(const std::filesystem::path& p)
{
    std::filesystem::file_status    st(std::filesystem::status(p));

    return (std::filesystem::is_regular_file(st));
}


/// @brief   
/// @param value[in] Arlington TSV field that might contain a predicate function 
/// @param function[out] the predicate function, if any. Otherwise function.clear()
/// @return the Arlington value with any predicates stripped off
std::string extract_function(const std::string& value, std::string &function) {
    std::regex      functionStr("fn:\\w*\\([ A-Za-z0-9<>=@&|.]+");  // matches "fn:<predicate-name>(
    std::smatch     match;
    std::string     to_ret = value;
    function.clear();

    if (std::regex_search(value, match, functionStr)) {
        to_ret = match.suffix();
        if (to_ret.size() >= 2)
            to_ret = to_ret.substr(1, to_ret.size() - 2);
        else
            to_ret = "";
        for (auto a: match)
            function += a;
    }
    return to_ret;
}

//////////////////////////////////////////////////////////////////////////
// 
int get_type_index(std::string single_type, std::string types) {
  std::vector<std::string> opt = split(types, ';');
  for (auto i = 0; i < (int)opt.size(); i++) {
    if (opt[i] == single_type)
      return i;
  }
  return -1;
}

//////////////////////////////////////////////////////////////////////////
// 
std::string get_link_for_type(std::string single_type, const std::string& types, const std::string& links) {
  int  index = get_type_index(single_type, types);
  if (index == -1)
    return "[]";
  std::vector<std::string> lnk = split(links, ';');
  if (index >= (int)lnk.size())  // for ArrayOfDifferences: types is "INTEGER;NAME", links is "" and we get buffer overflow in lnk!
    return "";
  return lnk[index];
}


//////////////////////////////////////////////////////////////////////////
// 
std::vector<std::string> split(const std::string& s, char separator) {
  //std::regex functionStr("fn:\\w*\\([ A-Za-z0-9<>=@&|,]+\\)");
  //std::smatch match;
  //if (std::regex_search(s, match, functionStr))
  //  std::cout << "match: " << match[1] << '\n';

  std::vector<std::string> output;
  std::string::size_type pos_prev = 0, pos_separator=0, pos_fn = 0, pos=0;

  auto finish = false;
  while (!finish) {
    pos_separator = s.find(separator, pos_prev);
    auto pos1 = s.find("FN:", pos);
    auto pos2 = s.find("fn:", pos);
    if (pos1 < pos2)
      pos_fn = pos1;
    else
      pos_fn = pos2;

    if (pos_separator <= pos_fn)
      pos = pos_separator;
    else {
      int num_brackets = 0;
      bool found = false;
      while (!found && pos_fn < s.size()) {
        if (s[pos_fn] == '(') num_brackets++;
        if (s[pos_fn] == ')') num_brackets--;
        if ((s[pos_fn] == separator) && (num_brackets == 0))
          found = true;
        else pos_fn++;
      }
      if (pos_fn == s.size())
        pos = std::string::npos;
      else 
        pos = pos_fn;
    }

    if (pos == std::string::npos) {
      output.push_back(s.substr(pos_prev, pos - pos_prev)); // Last word
      finish = true;
    } else {
      std::string substring(s.substr(pos_prev, pos - pos_prev));
      output.push_back(substring);
      pos_prev = ++pos;
    }
  }
  
  return output;
}

//@brief Strip leading whitespace from a string (e.g. the indented PDF path)
std::string& strip_leading_whitespace(std::string& str) {
    auto it2 = std::find_if(str.begin(), str.end(), [](char ch) { return !std::isspace<char>(ch, std::locale::classic()); });
    str.erase(str.begin(), it2);
    return str;
}
