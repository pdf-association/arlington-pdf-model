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
#include <cstdint>

#ifdef _WIN32
#include <Windows.h>
extern HINSTANCE ghInstance;
#else
#include <cstring>
#include <limits.h>
#include <sys/stat.h>
#endif // _WIN32


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



/// @brief Converts a potentially Unicode wide string to UTF8/ASCII
///
/// @param[in] wstr Unicode input, potentially with a BOM
///
/// @returns equivalent UTF8 string
std::string ToUtf8(const std::wstring& wstr) {
    std::wstring ws = wstr;

    // Check for UTF-16BE or UTF-8 BOM strings
    if ((ws.size() >= 2) && (ws[0] == (wchar_t)254) && (ws[1] == (wchar_t)255)) {
        // Handle UTF-16BE
        ws = ws.substr(2);

        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> conversion;
        std::string utf8;

        for (size_t i = 0; i < ws.size(); i += 2) {
            char16_t c16 = (ws[i] << 8) + ws[i + 1];
            utf8 = utf8 + conversion.to_bytes(c16);
        }
        return utf8;
    }
    else if ((ws.size() >= 3) && (ws[0] == (wchar_t)239) && (ws[1] == (wchar_t)187) && (ws[1] == (wchar_t)191)) {
        // Strip UTF-8 BOM for PDF 2.0
        ws = ws.substr(3);
    }

    const wchar_t* buffer = ws.c_str();
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
/// @param[in]  in      Arlington TSV Link or Type field that might contain predicates
/// @returns            the Arlington "Links" field with all predicates removed
std::string remove_type_link_predicates(const std::string& in) {
    if (in == "")
        return "";   // Common case for basic Arlington types

    std::string     to_ret = in;

    // Specific order!
    to_ret = std::regex_replace(to_ret, r_sinceVersionExtension, "$3");
    to_ret = std::regex_replace(to_ret, r_isPDFVersionExtension, "$3");
    to_ret = std::regex_replace(to_ret, r_sinceVersion,  "$2");
    to_ret = std::regex_replace(to_ret, r_beforeVersion, "$2");
    to_ret = std::regex_replace(to_ret, r_isPDFVersion,  "$2");
    to_ret = std::regex_replace(to_ret, r_Deprecated,    "$2");
    to_ret = std::regex_replace(to_ret, r_LinkExtension, "$2");
    assert(to_ret.find("fn:") == std::string::npos);
    assert(to_ret.size() >= 3);
    return to_ret;
}


/// @brief Split a string based on a single char separator
/// 
/// @param[in] s          string to split
/// @param[in] separator  character to split
/// 
/// @returns a vector of strings.
std::vector<std::string> split(const std::string& s, const char separator) {
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
/// @returns    a copy of the input string with all leading whitespace removed
std::string strip_leading_whitespace(const std::string& str) {
    std::string s = str;
    auto it2 = std::find_if(s.begin(), s.end(), [](char ch) { return !std::isspace<char>(ch, std::locale::classic()); });
    s.erase(s.begin(), it2);
    return s;
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
bool FindInVector(const std::vector<std::string> list, const std::string& v) {
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
        else
            return false;
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
static const std::regex r_SecondClassName("^([a-zA-Z0-9_\\-]{4,5}(_|:))");


/// @brief  Tests if a key is a valid PDF second class name according to Annex E of ISO 32000-2:2020
///
/// @param[in] key   the key in question
///
/// @returns true if a second class name
bool is_second_class_pdf_name(const std::string& key) {
    std::smatch  m;
    return std::regex_search(key, m, r_SecondClassName);
}


/// @brief Regex for PDF third class names according to Annex E of ISO 32000-2:2020
static const std::regex r_ThirdClassName("^XX");


/// @brief  Tests if a key is a valid PDF third class name according to Annex E of ISO 32000-2:2020
/// (i.e. starts with "XX")
///
/// @param[in] key   the key in question
///
/// @returns true if a third class name
bool is_third_class_pdf_name(const std::string& key) {
    std::smatch  m;
    return std::regex_search(key, m, r_ThirdClassName);
}


/// @brief Regex for a full PDF date string
///  (D:YYYYMMDDHHmmSSOHH'mm')
static const std::regex r_DateStart("^D:(\\d{4})(\\d{2})?(\\d{2})?(\\d{2})?(\\d{2})?(\\d{2})?([Z\\+\\-]{1})?(\\d{2})?(\'?)(\\d{2})?(\'?)"); 


/// @brief Tests if a string is a valid PDF date string according to clause 7.9.4 in ISO 32000-2:2020
///
/// @param[in] wdate   the date string in question
///
/// @returns true iff the date string is valid
bool is_valid_pdf_date_string(const std::wstring& wdate) {
    std::smatch  m;

    // Convert from possible UTF-16 and strip off BOM
    std::string date = ToUtf8(wdate);
    if ((date.size() >= 2) && ((uint8_t)date[0] == (uint8_t)254) && ((uint8_t)date[1] == (uint8_t)255)) {
        date = date.substr(2);
    }

    if (std::regex_search(date, m, r_DateStart)) {
        // Range check each of the fields as per ISO 32000-2:2020
        // m[0] is the full date
        // Ensure things are matched AND of the right length as zero-length matches are possible with regex
        if (m[1].matched && (m[1].length() == 4)) {
            // Matched YYYY - not checked
        }
        if (m[2].matched && (m[2].length() == 2)) {
            // Matched MM
            int mon = ((m[2].first[0] - '0') * 10) + (m[2].first[1] - '0');
            if ((mon < 1) || (mon > 12)) return false;
        }
        if (m[3].matched && (m[3].length() == 2)) {
            // Matched DD
            int day = ((m[3].first[0] - '0') * 10) + (m[3].first[1] - '0');
            if ((day < 1) || (day > 31)) return false;
        }
        if (m[4].matched && (m[4].length() == 2)) {
            // Matched HH
            int hr = ((m[4].first[0] - '0') * 10) + (m[4].first[1] - '0');
            if ((hr < 0) || (hr > 23)) return false;
        }
        if (m[5].matched && (m[5].length() == 2)) {
            // Matched mm
            int min = ((m[5].first[0] - '0') * 10) + (m[5].first[1] - '0');
            if ((min < 0) || (min > 59)) return false;
        }
        if (m[6].matched && (m[6].length() == 2)) {
            // Matched SS
            int sec = ((m[6].first[0] - '0') * 10) + (m[6].first[1] - '0');
            if ((sec < 0) || (sec > 59)) return false;
        }
        if (m[7].matched && (m[7].length() == 1)) {
            // Matched -, +, or Z
        }
        if (m[8].matched && (m[8].length() == 2)) {
            // Matched HH for timezone
            int tzhr = ((m[8].first[0] - '0') * 10) + (m[8].first[1] - '0');
            if ((tzhr < 0) || (tzhr > 23)) return false;
        }
        if (m[9].matched && (m[9].length() == 1)) {
            // Matched APOSTROPHE
            assert((m[9].first[0] == '\''));
        }
        if (m[10].matched && (m[10].length() == 2)) {
            // Matched mm for timezone
            int tzmin = ((m[10].first[0] - '0') * 10) + (m[10].first[1] - '0');
            if ((tzmin < 0) || (tzmin > 59)) return false;
        }
        return true;
    }
    return false;
}


/// @brief Convert an Arlington key to an array index (assumed to be an integer). 
/// This might include a wildcard at the end (e.g. 2*).
/// 
/// @param[in]  key   assumed integer array index (from Arlington)
/// 
/// @returns -1 on error or the integer array index (>= 0)
int key_to_array_index(const std::string& key) {
    assert(key.size() > 0);
    // Quick check to avoid exception handling
    if (!isdigit(key[0]))
        return -1;

    int i = -1;
    try {
        i = std::stoi(key);
        if (i < 0)
            i = -1;
    }
    catch (...) {
        i = -1;
    }
    return i;
}


/// @brief Converts a PDF string to the integer equivalent x 10.
/// 
/// @param[in] vers   PDF version as a string. Should be precisely 3 chars.
/// 
/// @returns the PDF version x 10
int string_to_pdf_version(const std::string& vers) {
    assert(vers.size() == 3);
    assert(isdigit(vers[0]));
    assert(vers[1] == '.');
    assert(isdigit(vers[2]));
    assert(FindInVector(v_ArlPDFVersions, vers));

    int pdf_ver = ((vers[0] - '0') * 10) + (vers[2] - '0');
    assert((pdf_ver >= 10) && ((pdf_ver <= 17) || (pdf_ver == 20)));

    return pdf_ver;
}

