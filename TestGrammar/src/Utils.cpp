////////////////////////////////////////////////////////////////////////////////////////////////////
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
// Contributors: Roman Toda, Frantisek Forgac, Normex
///////////////////////////////////////////////////////////////////////////////

#include <string>
#include <iostream>
#include <locale.h>
#include <codecvt>
#include <math.h>
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

#include "Pdfix.h"
using namespace PDFixSDK;

Pdfix_statics;

#define kPi 3.1415926535897932384626433832795f

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
    out.push_back(prefix[nbytes - 2] | (code / order));
    for (int i = 0; i < nbytes - 1; i++) {
      code = code % order;
      order >>= 6;
      out.push_back(0x80 | (code / order));
    }
  }
  return out;
}

std::string ToUtf8(const std::wstring& wstr) {
  const wchar_t* buffer = wstr.c_str();
  auto len = wcslen(buffer);
  std::string out;
  while (len-- > 0)
    out.append(ToUtf8(*buffer++));
  return out;
}

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

std::string get_path_dir(const std::string& path) {
  auto pos = path.find_last_of("\\/");
  if (pos == std::string::npos) return path;
  std::string dir(path.begin(), path.begin() + pos);
  return dir;
}

std::wstring get_path_dir(const std::wstring& path) {
  auto pos = path.find_last_of(L"\\/");
  if (pos == std::wstring::npos) return path;
  std::wstring dir(path.begin(), path.begin() + pos);
  return dir;
}

std::string check_folder_path(const std::string& path) {
  std::string result = path;
  if (result.back() != '/' && result.back() != '\\')
    result += "/";
  return result;
}

bool folder_exists(const std::wstring& path) {
#ifdef _WIN32
  struct _stat64i32 s;
  if (_wstat(path.c_str(), &s) == 0) {
    if (s.st_mode & S_IFDIR) return true;
  }
#elif defined __linux__
  struct stat s;
  std::string str_path = ToUtf8(path);
  if (stat(str_path.c_str(), &s) == 0) {
    if (s.st_mode & S_IFDIR) return true;
  }
#endif
  return false;
}

bool file_exists(const std::string& path) {
  struct stat s;
  if (stat(path.c_str(), &s) == 0) {
    if (s.st_mode & S_IFREG) return true;
  }
  return false;
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

std::vector<std::string> split_old(const std::string& s, char separator) {
  std::vector<std::string> output;
  std::string::size_type prev_pos = 0, pos = 0;
  while ((pos = s.find(separator, pos)) != std::string::npos){
    std::string substring(s.substr(prev_pos, pos - prev_pos));
    output.push_back(substring);
    prev_pos = ++pos;
  }
  output.push_back(s.substr(prev_pos, pos - prev_pos)); // Last word
  return output;
}

//////////////////////////////////////////////////////////////////////////
// PdfMatrix utils

void PdfMatrixTransform(PdfMatrix& m, PdfPoint& p) {
  PdfPoint ret = {
    m.a * p.x + m.c * p.y + m.e, m.b * p.x + m.d * p.y + m.f
  };
  p.x = ret.x;
  p.y = ret.y;
}

void PdfMatrixConcat(PdfMatrix& m, PdfMatrix& m1, bool prepend) {
  PdfMatrix left(m), right(m1);
  if (prepend)
    std::swap(left, right);
  m.a = left.a * right.a + left.b * right.c;
  m.b = left.a * right.b + left.b * right.d;
  m.c = left.c * right.a + left.d * right.c;
  m.d = left.c * right.b + left.d * right.d;
  m.e = left.e * right.a + left.f * right.c + right.e;
  m.f = left.e * right.b + left.f * right.d + right.f;
}

void PdfMatrixRotate(PdfMatrix& m, double radian, bool prepend) {
  double cosValue = cos(radian);
  double sinValue = sin(radian);
  PdfMatrix m1;
  m1.a = cosValue;
  m1.b = sinValue;
  m1.c = -sinValue;
  m1.d = cosValue;
  PdfMatrixConcat(m, m1, prepend);
}

void PdfMatrixScale(PdfMatrix& m, double sx, double sy, bool prepend) {
  m.a *= sx;
  m.d *= sy;
  if (prepend) {
    m.b *= sx;
    m.c *= sy;
  }
  m.b *= sy;
  m.c *= sx;
  m.e *= sx;
  m.f *= sy;
}

void PdfMatrixTranslate(PdfMatrix& m, double x, double y, bool prepend) {
  if (prepend) {
    m.e += x * m.a + y + m.c;
    m.f += y * m.d + x * m.b;
  }
  m.e += x;
  m.f += y;
}
