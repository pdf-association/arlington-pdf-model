///////////////////////////////////////////////////////////////////////////////
// GrammarFile.cpp
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
  A class to read grammar data from a csv file.
*/
 
#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include <algorithm>
#include <regex>

#include "GrammarFile.h"
#include "Pdfix.h"
#include "utils.h"


/*
  Parses through tsv file line by line and loads tsv data 
  into data_list (vector of vector of strings)
  if there is wrong structure, returns false, else returns true
*/
bool CGrammarReader::load()
{
  std::ifstream file(file_name);
  std::string line = "";
  // Iterate through each line and split the content using delimeter
  while (getline(file, line))
  {
    std::vector<std::string> vec;
    std::string::size_type prev_pos = 0, pos = 0;
    while ((pos = line.find(delimeter, pos)) != std::string::npos) {
      std::string substring(line.substr(prev_pos, pos - prev_pos));
      vec.push_back(substring);
      prev_pos = ++pos;
    }
    vec.push_back(line.substr(prev_pos, pos - prev_pos)); // Last word

    // check first line - have to have at least 10 columns (NOTE is optional)
    if (data_list.empty() && (vec.size() < TSV_NOTES)) {
      file.close();
      return false;
    }

    // Type, Required, Indirect, Inheritable - convert to uppercase
    std::transform(vec[TSV_TYPE].begin(), vec[TSV_TYPE].end(), vec[TSV_TYPE].begin(), ::toupper);
    std::transform(vec[TSV_REQUIRED].begin(), vec[TSV_REQUIRED].end(), vec[TSV_REQUIRED].begin(), ::toupper);
    std::transform(vec[TSV_INDIRECTREF].begin(), vec[TSV_INDIRECTREF].end(), vec[TSV_INDIRECTREF].begin(), ::toupper);
    std::transform(vec[TSV_INHERITABLE].begin(), vec[TSV_INHERITABLE].end(), vec[TSV_INHERITABLE].begin(), ::toupper);

    if (header_list.empty()) header_list=vec;
    else data_list.push_back(vec);
  }
  // Close the File
  file.close();
  return true;
}

/*
  Returns internal data_list (vector of vector of strings)
*/
const std::vector<std::vector<std::string>>& CGrammarReader::get_data()
{
  return data_list;
}

