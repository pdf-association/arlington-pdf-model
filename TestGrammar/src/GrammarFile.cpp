///////////////////////////////////////////////////////////////////////////////
// GrammarFile.cpp
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
#include "GrammarFile.h"
#include "Pdfix.h"
#include "utils.h"

/*
  Parses through csv file line by line and loads csv data 
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
    // check first line - have to have at least 11 columns
    if (data_list.empty() && (vec.size() < 11)) {
      file.close();
      return false;
    }
    // Type, Required, Indirect - convert to uppercase
    std::transform(vec[1].begin(), vec[1].end(), vec[1].begin(), ::toupper);
    std::transform(vec[4].begin(), vec[4].end(), vec[4].begin(), ::toupper);
    std::transform(vec[5].begin(), vec[5].end(), vec[5].begin(), ::toupper);
    data_list.push_back(vec);
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

/*
 Checks the validity of the loaded CVS data
 - correct # of columns
 - correct headings (first line)
 - correct basic types 1st column
 //todo: ked mam possible values a aj link tak musi byt rovnaky pocet
 */
bool CGrammarReader::check(std::ostream &report_stream) {
  if (data_list.empty()) {
    report_stream << "Empty grammar file:" << file_name << std::endl;
    return false;
  }

  // check first line (heading)
  std::vector<std::string> vec = data_list[0];
  if (vec.size() < 11) {
    report_stream << "Wrong number of columns:" << file_name << std::endl;
    return false;
  }

  if ((vec[0] != "Key") || (vec[1] != "TYPE") || (vec[2] != "SinceVersion") ||
    (vec[3] != "DeprecatedIn") || (vec[4] != "REQUIRED") || (vec[5] != "INDIRECTREFRENCE") ||
    (vec[6] != "RequiredValue") || (vec[7] != "DefaultValue") || (vec[8] != "PossibleValues") ||
    (vec[9] != "SpecialCase") || (vec[10] != "Link")) {
    report_stream << "Wrong headers for columns:" << file_name << std::endl;
    return false;
  }

  // check basic types (ignoring first line)
  // check existing link
  // check duplicate keys
  //todo: skontrolovat vyber link podla value (rovnaky pocet musi byt)
  std::vector<std::string> processed;
  for (int i = 1; i < data_list.size(); i++) {
    std::vector<std::string> vc = data_list[i];
    if (std::find(processed.begin(), processed.end(), vc[0]) == processed.end())
      processed.push_back(vc[0]);
    else
      report_stream << "Duplicate keys in:" << file_name << "::" << vc[0] << std::endl;
     
    // possible multiple types separated with ";"
    // need to compare all of them with basic_types
    std::vector<std::string> types = split(vc[1], ';');
    std::vector<std::string> links = split(vc[10], ';');
    for (auto type_pos = 0; type_pos < types.size(); type_pos++) {
      if (std::find(basic_types.begin(), basic_types.end(), types[type_pos]) == basic_types.end())
        report_stream << "Wrong type:" << types[type_pos] << " in:" << file_name << "::" << vc[0] << std::endl;
      //rt este treba aj ine type>name tree, number tree, array? stream?
      if (types[type_pos] == "DICTIONARY" || types[type_pos] == "NUMBER TREE"
        || types[type_pos] == "NAME TREE" || types[type_pos] == "STREAM"
        || types[type_pos] == "ARRAY") {
        if (links.size() <= type_pos)
          report_stream << "Wrong # of links compared to # of types:" << file_name << "::" << vc[0] << std::endl;
        else
          if (links[type_pos] == "[]" || links[type_pos] == "[null]")
            report_stream << "Type " << types[type_pos] << "not linked in:" << file_name << "::" << vc[0] << std::endl;
      }
    }
    
    // does link exist?
    if (vc[10] != "") {
      std::string new_name = get_path_dir(file_name);
      new_name += "/";
      new_name += vc[10];
      new_name += ".csv";
      if (!file_exists(new_name))
        report_stream << "Link doesn't exists:" << vc[10] << " in:" << file_name << "::" << vc[0] << std::endl;
    }
  }
  return true;
}
