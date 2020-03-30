///////////////////////////////////////////////////////////////////////////////
// GrammarFile.h
// Copyright (c) 2020 Normex. All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

/*!
  A class to read grammar data from a csv file.
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

class CGrammarReader
{
  std::string file_name;
  std::string delimeter;
  std::vector<std::vector<std::string>> data_list;
  std::vector<std::string> basic_types = { "BOOLEAN", "NUMBER", "NAME",
    "STRING", "STREAM", "ARRAY", "DICTIONARY", "INTEGER", "DATE", "RECTANGLE", "NUMBER TREE", "NAME TREE" };
public:
  CGrammarReader(std::string f_name, std::string delm = "\t") :
    file_name(f_name), delimeter(delm)
  { }

  // Function to fetch data from a CSV File
  bool load();
  const std::vector<std::vector<std::string>>& get_data();
  bool check(std::ostream &report_stream);
};