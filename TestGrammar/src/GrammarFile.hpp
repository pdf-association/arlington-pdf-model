///////////////////////////////////////////////////////////////////////////////
// GrammarFile.hpp
// Copyright (c) 2020 Normex. All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

/*!
 * A class to read grammar data from a csv file.
 */
 
#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include <algorithm>

#include "Pdfix.h"

  class CGrammarReader
{
  std::string file_name;
  std::string delimeter;
  std::ostream &report_stream;

public:
  CGrammarReader(std::string f_name, std::ostream &ss, std::string delm = "\t") :
    file_name(f_name), delimeter(delm), report_stream(ss)
  { }

  // Function to fetch data from a CSV File
  std::vector<std::vector<std::string> > get_data();
};

/*
* Parses through csv file line by line and returns the data
* in vector of vector of strings.
*/
std::vector<std::vector<std::string> > CGrammarReader::get_data()
{
  std::ifstream file(file_name);
  std::vector<std::vector<std::string> > data_list;

  std::string line = "";
  // Iterate through each line and split the content using delimeter
  while (getline(file, line))
  {
    std::vector<std::string> vec;
    //boost::algorithm::split(vec, line, boost::is_any_of(delimeter));
    std::string::size_type prev_pos = 0, pos = 0;
    while ((pos = line.find(delimeter, pos)) != std::string::npos) {
      std::string substring(line.substr(prev_pos, pos - prev_pos));
      vec.push_back(substring);
      prev_pos = ++pos;
    }
    vec.push_back(line.substr(prev_pos, pos - prev_pos)); // Last word

    // Type, Required, Indirect - convert to uppercase
    std::transform(vec[1].begin(), vec[1].end(), vec[1].begin(), ::toupper);
    std::transform(vec[4].begin(), vec[4].end(), vec[4].begin(), ::toupper);
    std::transform(vec[5].begin(), vec[5].end(), vec[5].begin(), ::toupper);
    // check the first line if it follows the pattern
    if (data_list.empty()) {
      if ((vec[0] != "Key") || (vec[1] != "TYPE") || (vec[2] != "SinceVersion") ||
        (vec[3] != "DeprecatedIn") || (vec[4] != "REQUIRED") || (vec[5] != "INDIRECTREFRENCE") ||
        (vec[6] != "RequiredValue") || (vec[7] != "DefaultValue") || (vec[8] != "PossibleValues") ||
        (vec[9] != "SpecialCase") || (vec[10] != "Link")) {
        report_stream << "Problem with grammar file" << file_name << std::endl;
      }
    }
    data_list.push_back(vec);
  }
  // Close the File
  file.close();
  return data_list;
}
