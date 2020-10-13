///////////////////////////////////////////////////////////////////////////////
// GrammarFile.h
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

/*!
  A class to read grammar data from a tsv file.
*/

#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include <algorithm>

#include "Pdfix.h"
#include "utils.h"

// TSV columns
#define TSV_KEYNAME         (0)     // "*" means any
#define TSV_TYPE            (1)     // in alphabetical order of basic_types, "," separated
#define TSV_SINCEVERSION    (2)     // 1.0, 1.1, ..., 2.0
#define TSV_DEPRECATEDIN    (3)     // blank or 1.0, 1.1, ..., 2.0
#define TSV_REQUIRED        (4)     // TRUE or FALSE
#define TSV_INDIRECTREF     (5)     // TRUE or FALSE
#define TSV_INHERITABLE     (6)
#define TSV_DEFAULTVALUE    (7)
#define TSV_POSSIBLEVALUES  (8)
#define TSV_SPECIALCASE     (9)     // ignore for now...
#define TSV_LINK            (10)     // ";" separated list of "[xxx]"
#define TSV_NOTES           (11)    // free text


class CGrammarReader
{
public:
  std::string file_name;
  std::string delimeter;
  std::vector<std::string> header_list;
  std::vector<std::vector<std::string>> data_list;
  std::vector<std::string> basic_types = { "BOOLEAN", "NUMBER", "NULL", "NAME",
    "STRING", "STRING-BYTE", "STRING-TEXT", "STRING-ASCII", "STREAM", "ARRAY", "DICTIONARY", "INTEGER", "DATE", "RECTANGLE", "NUMBER-TREE", "NAME-TREE" };

  CGrammarReader(std::string f_name, std::string delm = "\t") :
    file_name(f_name), delimeter(delm)
  { }

  // Function to fetch data from a TSV File
  bool load();
  const std::vector<std::vector<std::string>>& get_data();
};