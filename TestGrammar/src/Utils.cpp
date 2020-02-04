////////////////////////////////////////////////////////////////////////////////////////////////////
// Utils.cpp
// Copyright (c) 2018 Pdfix. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <string>
#include <iostream>
#include <locale.h>
#include <codecvt>
#include <math.h>
#ifdef _WIN32
#include <Windows.h>
extern HINSTANCE ghInstance;
#elif defined __linux__
#include <cstring>
#include <limits.h>
#include <locale>
#endif
#include "Pdfix.h"
#include "PdfToHtml.h"
#include "OcrTesseract.h"

Pdfix_statics;
PdfToHtml_statics;
OcrTesseract_statics;

#define kPi 3.1415926535897932384626433832795f

// convert UTF-8 string to wstring
std::wstring FromUtf8(const std::string& str) {
  std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
  return myconv.from_bytes(str);
}

// convert wstring to UTF-8 string
std::string ToUtf8(const std::wstring& str) {
  std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
  return myconv.to_bytes(str);
}

std::string GetAbsolutePath(const std::string& path) {
  std::string result;
#ifndef _WIN32
  if (path.length() && path.front() == '/') {
    result = path;
  }
  else {
    result.resize(PATH_MAX);
    realpath(path.c_str(), (char*)result.c_str());
  }
#else
  std::string dir;
  dir.resize(_MAX_PATH);
  GetModuleFileNameA(NULL, (char*)dir.c_str(), _MAX_PATH);
  dir.erase(dir.begin() + dir.find_last_of("\\/") + 1, dir.end());
  SetCurrentDirectoryA(dir.c_str());
  result.resize(_MAX_PATH);
  GetFullPathNameA(path.c_str(), _MAX_PATH, (char*)result.c_str(), NULL);
#endif
  result.resize(strlen(result.c_str()));
  return result;
}

std::wstring GetAbsolutePath(const std::wstring& path) {
  return FromUtf8(GetAbsolutePath(ToUtf8(path)));
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
