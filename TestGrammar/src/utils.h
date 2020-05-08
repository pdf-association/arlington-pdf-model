////////////////////////////////////////////////////////////////////////////////////////////////////
// utils.h
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

#pragma once

#include <string>
#include "Pdfix.h"

#ifdef GetObject
#undef GetObject
#endif

//std::wstring FromUtf8(const std::string& str);
std::string ToUtf8(const std::wstring& str);
std::wstring utf8ToUtf16(const std::string& utf8Str);

// gets folder from provided filename
std::string get_path_dir(const std::string& path);
std::wstring get_path_dir(const std::wstring& path);
// returns folder path with trailing backslash
std::string check_folder_path(const std::string& path);

// check folder of file
bool folder_exists(const std::wstring& path);
//bool folder_exists(const std::wstring& path);
bool file_exists(const std::string& path);

//split string into vector
std::vector<std::string> split(const std::string& s, char seperator);

//////////////////////////////////////////////////////////////////////////
// PdfMatrix utils
//void PdfMatrixTransform(PdfMatrix& m, PdfPoint& p);
//void PdfMatrixConcat(PdfMatrix& m, PdfMatrix& m1, bool prepend);
//void PdfMatrixRotate(PdfMatrix& m, double radian, bool prepend);
//void PdfMatrixScale(PdfMatrix& m, double sx, double sy, bool prepend);
//void PdfMatrixTranslate(PdfMatrix& m, double x, double y, bool prepend);
