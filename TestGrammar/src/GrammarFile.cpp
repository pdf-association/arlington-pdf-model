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
    if (data_list.empty() && (vec.size() < NOTE_COLUMN)) {
      file.close();
      return false;
    }
    // Type, Required, Indirect - convert to uppercase
    std::transform(vec[TYPE_COLUMN].begin(), vec[TYPE_COLUMN].end(), vec[TYPE_COLUMN].begin(), ::toupper);
    std::transform(vec[REQUIRED_COLUMN].begin(), vec[REQUIRED_COLUMN].end(), vec[REQUIRED_COLUMN].begin(), ::toupper);
    std::transform(vec[INDIRECTREFERENCE_COLUMN].begin(), vec[INDIRECTREFERENCE_COLUMN].end(), vec[INDIRECTREFERENCE_COLUMN].begin(), ::toupper);
    std::transform(vec[INHERITABLE_COLUMN].begin(), vec[INHERITABLE_COLUMN].end(), vec[INHERITABLE_COLUMN].begin(), ::toupper);
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
 */
bool CGrammarReader::check(std::ostream &report_stream) {
  if (data_list.empty()) {
    report_stream << "Empty grammar file:" << file_name << std::endl;
    return false;
  }

  // check first line (heading)
  std::vector<std::string> vec = data_list[0];
  if (vec.size() < 10) {
    report_stream << "Wrong number of columns:" << file_name << std::endl;
    return false;
  }

  if ((vec[KEY_COLUMN] != "Key") || (vec[TYPE_COLUMN] != "TYPE") || (vec[SINCEVERSION_COLUMN] != "SinceVersion") ||
    (vec[DEPRECATEDIN_COLUMN] != "DeprecatedIn") || (vec[REQUIRED_COLUMN] != "REQUIRED") || 
    (vec[INDIRECTREFERENCE_COLUMN] != "INDIRECTREFERENCE") || (vec[INHERITABLE_COLUMN] != "INHERITABLE") ||
    (vec[DEFAULTVALE_COLUMN] != "DefaultValue") || (vec[POSSIBLEVALUES_COLUMN] != "PossibleValues") ||
    (vec[SPECIALCASE_COLUMN] != "SpecialCase") || (vec[LINK_COLUMN] != "Link")) {
    report_stream << "Wrong headers for columns:" << file_name << std::endl;
    return false;
  }

  // check basic types (ignoring first line)
  // check existing link
  // check duplicate keys
  std::vector<std::string> processed;
  for (auto i = 1; i < data_list.size(); i++) {
    std::vector<std::string> vc = data_list[i];
    if (std::find(processed.begin(), processed.end(), vc[KEY_COLUMN]) == processed.end())
      processed.push_back(vc[KEY_COLUMN]);
    else
      report_stream << "Duplicate keys in:" << file_name << "::" << vc[KEY_COLUMN] << std::endl;

    // possible multiple types separated with ";"
    // need to compare all of them with basic_types
    std::vector<std::string> types = split(vc[TYPE_COLUMN], ';');
    std::vector<std::string> links = split(vc[LINK_COLUMN], ';');
    std::regex regex("^\\[[A-Z,a-z,0-9,_,\\,]*\\]$");

    // if link exists we check
    // - number of links and number of types match
    // - each link follows patter [];[]..
    // - each dictionary, array etc.. is linked
    // - each link actuall exists

    //if (vc[KEY_COLUMN]=="Subtype" && (vc[POSSIBLEVALUES_COLUMN]=="" || vc[POSSIBLEVALUES_COLUMN]=="[]"))
    //  report_stream << "Undefined Subtype " << file_name << "::" << vc[KEY_COLUMN] << std::endl;

    if (vc[LINK_COLUMN] != "") {
      if (links.size() != types.size())
        report_stream << "Wrong # of types vs. # of links " << file_name << "::" << vc[KEY_COLUMN] << std::endl;
      for (auto link_pos = 0; link_pos < links.size(); link_pos++) {
        if (!std::regex_match(links[link_pos], regex)) {
          report_stream << "Wrong pattern in links " << file_name << "::" << vc[KEY_COLUMN] << std::endl;
        }
        else {
          //report all unliked complex types
          if ((types.size() > link_pos) && (links[link_pos] == "[]") &&
            (types[link_pos] == "DICTIONARY" || types[link_pos] == "NUMBER-TREE"
              || types[link_pos] == "NAME-TREE" || types[link_pos] == "STREAM"
              || types[link_pos] == "ARRAY"))
            report_stream << "Type " << types[link_pos] << " not linked in:" << file_name << "::" << vc[KEY_COLUMN] << std::endl;


          std::vector<std::string> direct_links = split(links[link_pos].substr(1, links[link_pos].size() - 2), ',');

          // report all multiple links - all the places where implementor has to decide which one to validate
          //if (direct_links.size()>1)
          //  for (auto lnk : direct_links)
          //    if (lnk != "") 
          //      report_stream << lnk << std::endl;

          for (auto lnk : direct_links)
            if (lnk != "") {
              std::string new_name = get_path_dir(file_name);
              new_name += "/";
              new_name += lnk;
              new_name += ".tsv";
              if (!file_exists(new_name))
                report_stream << "Link doesn't exist:" << lnk << " in:" << file_name << "::" << vc[KEY_COLUMN] << std::endl;
            }
        }
      }
    }

    //check if each type is ok
    for (auto type:types)
      if (std::find(basic_types.begin(), basic_types.end(), type) == basic_types.end())
        report_stream << "Wrong type:" << type << " in:" << file_name << "::" << vc[KEY_COLUMN] << std::endl;

    // check if complex type does have possible value
    for (auto t_pos = 0; t_pos < types.size(); t_pos++)
      if ( (types[t_pos] == "ARRAY" || types[t_pos] == "DICTIONARY" || types[t_pos] == "NUMBER-TREE"
            || types[t_pos] == "NAME-TREE" || types[t_pos] == "STREAM") && vc[POSSIBLEVALUES_COLUMN] != "") {
        std::vector<std::string> def_val = split(vc[POSSIBLEVALUES_COLUMN], ';');
        if (def_val[t_pos]!="[]") 
          report_stream << "Complex type does have possible value defined:"<< vc[POSSIBLEVALUES_COLUMN] << " in:"<< file_name << "::" << vc[KEY_COLUMN] << std::endl;
      }

    //if we have more types, check pattern in Required, default and possible values
    if (types.size() > 1) {
      if (vc[POSSIBLEVALUES_COLUMN] != "") {
        std::vector<std::string> poss_val = split(vc[POSSIBLEVALUES_COLUMN], ';');
        if (types.size()!=poss_val.size())
          report_stream << "Wrong # of types vs. # of possible values " << file_name << "::" << vc[KEY_COLUMN] << std::endl;
     }
    }

    //report all possible values
    //if (vc[POSSIBLEVALUES_COLUMN]!="")
    //  report_stream << vc[POSSIBLEVALUES_COLUMN] << std::endl;

//    report all special cases
    //if (vc[SPECIALCASE_COLUMN] != "")
    //  report_stream << vc[SPECIALCASE_COLUMN] << std::endl;

    if ((vc[INHERITABLE_COLUMN] != "TRUE") && (vc[INHERITABLE_COLUMN] != "FALSE"))
      report_stream << file_name << "::" << vc[KEY_COLUMN] << std::endl;
  }
  return true;
}
