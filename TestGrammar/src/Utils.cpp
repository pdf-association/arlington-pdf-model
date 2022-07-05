///////////////////////////////////////////////////////////////////////////////
/// @file 
/// @brief Utility function definitions
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
/// @author Roman Toda, Normex
/// @author Frantisek Forgac, Normex
/// @author Peter Wyatt, PDF Association
/// 
///////////////////////////////////////////////////////////////////////////////

/// @file
/// General utility functions

#define  _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNINGS
#define  _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS

#include "utils.h"
#include "ArlPredicates.h"

#include <iostream>
#include <locale>
#include <codecvt>
#include <math.h>
#include <algorithm>
#include <regex>
#include <cassert>

#ifdef _WIN32
#include <Windows.h>
extern HINSTANCE ghInstance;
#else
#include <cstring>
#include <limits.h>
#include <sys/stat.h>
#endif // _WIN32

/// @brief Regexes for matching versioning predicates: $1 = PDF version "," $2 = Link or predefined Arlington type
static const std::regex  r_sinceVersion("fn:SinceVersion\\(" + ArlPDFVersion + "\\,([A-Za-z0-9_\\-]+)\\)");
static const std::regex  r_isDeprecated("fn:Deprecated\\(" + ArlPDFVersion + "\\,([A-Za-z0-9_\\-]+)\\)");
static const std::regex  r_isPDFVersion("fn:IsPDFVersion\\(" + ArlPDFVersion + "\\,([A-Za-z0-9_\\-]+)\\)");

/// @brief Converts a Unicode string to UTF8
///
/// @param[in] unicode Unicode input
///
/// @returns equivalent UTF8 string
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
        } // for
    }
    return out;
}

/// @brief Converts a Unicode wide string to UTF8
///
/// @param[in] wstr Unicode input
///
/// @returns equivalent UTF8 string
std::string ToUtf8(const std::wstring& wstr) {
    const wchar_t* buffer = wstr.c_str();
    auto len = wcslen(buffer);
    std::string out;
    while (len-- > 0)
        out.append(ToUtf8(*buffer++));
    return out;
}

/// @brief Converts UTF8 input string to UTF16 wide string
///
/// @param[in] str  input string (assumed valid UTF8)
///
/// @returns  UTF16 equivalent wide string
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
                    if (*p == 0)
                        return result; // unexpected
                    b = *p++;
                    if (!lev[0].encoded(b)) // encoding corrupted
                        return result;
                    wchar_t tmp = b ^ lev[0].Null;
                    wc |= tmp << lev[0].Data * (j - 1);
                } // for trailing bytes
                result.push_back(wc);
                found = true;
                break;
            } // if lev[i]
        }   // for lev
        if (!found)
            return result; // encoding incorrect
    }   // while
    return result;
}


/// @brief Converts a std::string to std::wstring
///
/// @param[in] s input string
///
/// @returns   wide string equivalent  of the input string
std::wstring ToWString(const std::string& s)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wide = converter.from_bytes(s);
    return wide;
}


/// @brief   Checks if a path is a folder (directory)
///
/// @param[in] p  path
///
/// @returns true if path p is a folder
bool is_folder(const std::filesystem::path& p)
{
    std::filesystem::file_status    st(std::filesystem::status(p));
    return (std::filesystem::is_directory(st));
}


/// @brief   Checks if a path is a file (not a folder)
///
/// @param[in]   p  path
///
/// @returns true if path p is a regular file
bool is_file(const std::filesystem::path& p)
{
    std::filesystem::file_status    st(std::filesystem::status(p));
    return (std::filesystem::is_regular_file(st));
}


/// @brief  Removes all Alington predicates (declarative functions) from the "Link" or "Type" fields.
///       
/// @param[in] link_in  Arlington TSV Link or Type field that might contain predicates
/// @returns            the Arlington "Links" field with all predicates removed
std::string remove_type_link_predicates(const std::string& in) {
    std::string     to_ret;

    to_ret = std::regex_replace(in,     r_sinceVersion, "$2");
    to_ret = std::regex_replace(to_ret, r_isPDFVersion, "$2");
    to_ret = std::regex_replace(to_ret, r_isDeprecated, "$2");
    return to_ret;
}


/// @brief  Works out if an Arlington type is in the list of Arlington Types from the TSV data. Supports predicates.
///         TO BE DEPRECATED!
///
/// @param[in] single_type a single Arlington predefined type (e.g. array, bitmask, rectangle, matrix, etc)
/// @param[in] types       an Arlington Types field (alphabetically sorted; SEMI-COLON separated)
///
/// @returns -1 if single_type is not types, otherwise the index into the types array (when separated by SEMI-COLON)
int get_type_index(std::string single_type, std::string types) {
    std::vector<std::string> opt = split(remove_type_link_predicates(types), ';');
    for (auto i = 0; i < (int)opt.size(); i++) {
        if (opt[i] == single_type)
            return i;
    }
    return -1;
}



/// @brief  Looks up a single Arlington type in the Types field, and then matches across to the Links field.
///         Predicates are NOT stripped and retained in the result.
///
/// @param[in] single_type  a single Arlington predefined type (e.g. array, bitmask, rectangle, matrix, etc)
/// @param[in] types    Arlington Types field (alphabetically sorted, SEMI-COLON separated)
/// @param[in] links    Arlington Links field (enclosed in [] and semi-colon separated)
///
/// @returns  either a set of Arlington Links (incl. possibly predicates) in "[]", or the empty list "[]" if no type match
std::string get_link_for_type(std::string single_type, const std::string& types, const std::string& links) {
    int  index = get_type_index(single_type, types);
    if (index == -1)
        return "[]";
    std::vector<std::string> lnk = split(links, ';');
    if (index >= (int)lnk.size())  // for ArrayOfDifferences: types is "integer;name", links is "" and we get buffer overflow in lnk!
        return "[]";
    return lnk[index];
}


//////////////////////////////////////////////////////////////////////////
//         TO BE DEPRECATED!
//
std::vector<std::string> split(const std::string& s, char separator) {
  std::vector<std::string> output;
  std::string::size_type pos_prev = 0, pos_separator=0, pos_fn = 0, pos=0;

  auto finish = false;
  while (!finish) {
    pos_separator = s.find(separator, pos_prev);
    pos_fn = s.find("fn:", pos);

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


/// @brief Strip leading whitespace from a string (e.g. the indented PDF path)
///
/// @param[in]  str   the string with leading whitespace
///
/// @returns    the input string with all leading whitespace removed
std::string strip_leading_whitespace(std::string& str) {
    auto it2 = std::find_if(str.begin(), str.end(), [](char ch) { return !std::isspace<char>(ch, std::locale::classic()); });
    str.erase(str.begin(), it2);
    return str;
}


/// @brief Case INsensitive comparison of two strings. e.g. for file extensions
///
/// @param[in] a   string one
/// @param[in] b   string two
///
/// @returns true if the two strings are a case-insensitive match. false otherwise
bool iequals(const std::string& a, const std::string& b)
{
    return std::equal(a.begin(), a.end(),
        b.begin(), b.end(),
        [](char a, char b) {
            return tolower(a) == tolower(b);
        });
}


/// @brief Case INsensitive substring search
///
/// @param[in] s   string 
/// @param[in] s1  sub-string to search for in s
///
/// @returns true if string s1 is in string s. false otherwise
bool icontains(const std::string& s, const std::string& s1)
{
    // Local vars so as not to alter argument strings
    std::string s_in = s;
    std::string s1_in = s1;

    // Convert args lower case
    std::for_each(s_in.begin(), s_in.end(), [](char& c) { // modify in-place
        c = (char)std::tolower(static_cast<unsigned char>(c));
        });
    std::for_each(s1_in.begin(), s1_in.end(), [](char& c) { // modify in-place
        c = (char)std::tolower(static_cast<unsigned char>(c));
        });

    // Find sub string in given string
    return (s_in.find(s1_in, 0) != std::string::npos);
}


/// @brief  Utility function to locate a string in a vector of strings
///
/// @param[in] list   vector to search
/// @param[in] v      string to find
///
/// @returns   true if 'v' is an exact to an element in 'list'. false otherwise.
bool FindInVector(const std::vector<std::string> list, const std::string v) {
    for (auto& li : list)
        if (v == li)
            return true;
    return false;
}


/// @brief Matches integer-only array indices (NO WILDCARDS!) for TSV_KEYNAME field
///        Note that some PDF keys are real number like e.g. "/1.2"
static const std::regex r_KeyArrayKeys("^([0-9]+|[0-9]*\\*?)$");


/// @brief  Checks if an Arlington TSV is an array object. Confirmed by checking
///         that all keys are either pure integers, just "*" (last row) or integer+"*".
///         Also confirms integer array keys start at zero and always increase by +1.
///         A TSV with a single wildcard key "*" is ambiguous but returns true here.
///
/// @param[in] fname TSV filename
/// @param[in] keys  list of keys from an Arlington TSV definition
/// @param[in] ofs   open stream to write any errors to (use cnull/wcnull for /dev/null)
/// @param[out] wildcard_only true if and only if the TSV is a single line with "*" wildcard. This is an ambiguous dictionary or array.
///
/// @returns true if keys can represent a PDF array. false otherwise
bool check_valid_array_definition(const std::string& fname, const std::vector<std::string>& keys, std::ostream& ofs, bool *wildcard_only) {
    assert(keys.size() > 0);
    assert(wildcard_only != nullptr);
    *wildcard_only = false;

    // Trivial cases special-cased
    if (keys.size() == 1) {
        if (keys[0] == "*") {
            *wildcard_only = true;
            return true;
        }
        else if (keys[0] == "0") {
            return true;
        }
        else if (keys[0] == "0*") {
            ofs << COLOR_WARNING << "single element array with '0*' should use '*' " << fname << COLOR_RESET;
            return true;
        }
        else {
            return false;
        }
    }

    int         idx;
    int         first_wildcard = -1;
    bool        row_has_wildcard;
    std::smatch m;
    for (int row = 0; row < (int)keys.size(); row++) {
        if (!std::regex_search(keys[row], m, r_KeyArrayKeys))
            return false;

        // Last row wildcard is common and stoi() dislikes so test first
        if ((keys[row] == "*") && (row == (int)keys.size() - 1))
            return true;

        // Attempt to convert what is possibly an integer (got passed regex above)
        try {
            idx = std::stoi(keys[row]);
        }
        catch (std::exception& ex) {
            ofs << COLOR_ERROR << "arrays must use integers: was '" << keys[row] << "', wanted " << row << " for " << fname << ": " << ex.what() << COLOR_RESET;
            return false;
        }

        if ((idx != row) && (row > 0)) {
            ofs << COLOR_ERROR << "arrays need to use contiguous integers: was '" << keys[row] << "', wanted " << row << " for " << fname << COLOR_RESET;
            return false;
        }

        // Check if we are ending with wildcard sequence
        row_has_wildcard = (keys[keys[row].size() - 1] == "*");
        if (row_has_wildcard) {
            if (first_wildcard < 0)
                first_wildcard = row;
        }
        else if (first_wildcard >= 0) {
            ofs << COLOR_ERROR << "array using numbered wildcards (integer+'*') need to be contiguous last rows in" << fname << COLOR_RESET;
            return false;
        }
    }

    return true;
}


/// @brief Regex for PDF second class names according to Annex E of ISO 32000-2:2020
const std::regex r_SecondClassName("^([a-zA-Z0-9_\\-]{4,5}(_|:)|XX)");


/// @brief  Tests if a key is a valid PDF second class name according to Annex E of ISO 32000-2:2020
///
/// @param[in] key   the key in question
///
/// @returns true if a second class name
bool is_second_class_pdf_name(const std::string key) {
    std::smatch  m;
    return std::regex_search(key, m, r_SecondClassName);
}


/// @brief Regex for PDF third class names according to Annex E of ISO 32000-2:2020
const std::regex r_ThirdClassName("^XX");


/// @brief  Tests if a key is a valid PDF third class name according to Annex E of ISO 32000-2:2020
/// (i.e. starts with "XX")
///
/// @param[in] key   the key in question
///
/// @returns true if a third class name
bool is_third_class_pdf_name(const std::string key) {
    std::smatch  m;
    return std::regex_search(key, m, r_ThirdClassName);
}
