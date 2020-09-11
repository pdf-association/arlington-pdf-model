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

template<typename charT>

struct my_equal {
  my_equal(const std::locale& loc) : loc_(loc) {}
  bool operator()(charT ch1, charT ch2) {
    return std::toupper(ch1, loc_) == std::toupper(ch2, loc_);
  }
private:
  const std::locale& loc_;
};

// find substring (case insensitive)
template<typename T>
int ci_find_substr(const T& str1, const T& str2, const std::locale& loc = std::locale())
{
  typename T::const_iterator it = std::search(str1.begin(), str1.end(),
    str2.begin(), str2.end(), my_equal<typename T::value_type>(loc));
  if (it != str1.end()) return it - str1.begin();
  else return -1; // not found
}


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

/*
 Checks the validity of the loaded CVS data
 - correct # of columns
 - correct headings (first line)
 - correct basic types 1st column
 */
bool CGrammarReader::check(std::ostream &report_stream) 
{
  if (data_list.empty()) {
    report_stream << "Empty grammar file:" << file_name << std::endl;
    return false;
  }

  if (header_list.size() < 10) {
    report_stream << "Wrong number of columns: " << file_name << std::endl;
    return false;
  }

  if ((header_list[TSV_KEYNAME] != "Key") || (header_list[TSV_TYPE] != "TYPE") || (header_list[TSV_SINCEVERSION] != "SinceVersion") ||
    (header_list[TSV_DEPRECATEDIN] != "DeprecatedIn") || (header_list[TSV_REQUIRED] != "REQUIRED") ||
    (header_list[TSV_INDIRECTREF] != "INDIRECTREFERENCE") || (header_list[TSV_INHERITABLE] != "INHERITABLE") ||
    (header_list[TSV_DEFAULTVALUE] != "DefaultValue") || (header_list[TSV_POSSIBLEVALUES] != "PossibleValues") ||
    (header_list[TSV_SPECIALCASE] != "SpecialCase") || (header_list[TSV_LINK] != "Link")) {
    report_stream << "Wrong headers for columns: " << file_name << std::endl;
    return false;
  }

  // check basic types (ignoring first line)
  // check existing link
  // check duplicate keys
  std::vector<std::string> processed;
  for (auto& vc : data_list) {
    //std::string str2 = "fn:";
    //if (ci_find_substr(vc[TSV_KEYNAME], str2) != -1)
    //  report_stream << "TSV_KEYNAME: " << file_name << std::endl;
    //if (ci_find_substr(vc[TSV_TYPE], str2) != -1)
    //  report_stream << "TSV_TYPE: " << file_name << std::endl;
    //if (ci_find_substr(vc[TSV_SINCEVERSION], str2) != -1)
    //  report_stream << "TSV_SINCEVERSION: " << file_name << std::endl;
    //if (ci_find_substr(vc[TSV_DEPRECATEDIN], str2) != -1)
    //  report_stream << "TSV_DEPRECATEDIN: " << file_name << std::endl;
    //if (ci_find_substr(vc[TSV_REQUIRED], str2) != -1)
    //  report_stream << "TSV_REQUIRED: " << file_name << std::endl;
    //if (ci_find_substr(vc[TSV_INDIRECTREF], str2) != -1)
    //  report_stream << "TSV_INDIRECTREF: " << file_name << std::endl;
    //if (ci_find_substr(vc[TSV_INHERITABLE], str2) != -1)
    //  report_stream << "TSV_INHERITABLE: " << file_name << std::endl;
    //if (ci_find_substr(vc[TSV_DEFAULTVALUE], str2) != -1)
    //  report_stream << "TSV_DEFAULTVALUE: " << file_name << std::endl;
    //if (ci_find_substr(vc[TSV_POSSIBLEVALUES], str2) != -1)
    //  report_stream << "TSV_POSSIBLEVALUES: " << file_name << std::endl;
    //if (ci_find_substr(vc[TSV_SPECIALCASE], str2) != -1)
    //  report_stream << "TSV_SPECIALCASE: " << file_name << std::endl;

  //for (size_t i = 0; i < data_list.size(); i++) {
  //  std::vector<std::string> vc = data_list[i];
    if (std::find(processed.begin(), processed.end(), vc[TSV_KEYNAME]) == processed.end())
      processed.push_back(vc[TSV_KEYNAME]);
    else
      report_stream << "Duplicate keys in: " << file_name << "::" << vc[TSV_KEYNAME] << std::endl;

    // possible multiple types separated with ";"
    // need to compare all of them with basic_types
    std::vector<std::string> types = split(vc[TSV_TYPE], ';');
    std::vector<std::string> links = split(vc[TSV_LINK], ';');
    std::regex regex("^\\[[A-Z,a-z,0-9,_,\\,]*\\]$");
    
    // if link exists we check
    // - number of links and number of types match
    // - each link follows patter [];[]..
    // - each dictionary, array etc.. is linked
    // - each link actuall exists

    //if (vc[TSV_KEYNAME]=="Subtype" && (vc[TSV_POSSIBLEVALUES]=="" || vc[TSV_POSSIBLEVALUES]=="[]"))
    //  report_stream << "Undefined Subtype " << file_name << "::" << vc[TSV_KEYNAME] << std::endl;
    if (vc[TSV_LINK] != "") {
      if (links.size() != types.size())
        report_stream << "Wrong # of types vs. # of links " << file_name << "::" << vc[TSV_KEYNAME] << std::endl;
      for (size_t link_pos = 0; link_pos < links.size(); link_pos++) {
        if (!std::regex_match(links[link_pos], regex)) {
          report_stream << "Wrong pattern in links " << file_name << "::" << vc[TSV_KEYNAME] << std::endl;
        }
        else {
          //report all unliked complex types
          if ((types.size() > link_pos) && (links[link_pos] == "[]") &&
            (types[link_pos] == "DICTIONARY" || types[link_pos] == "NUMBER-TREE"
              || types[link_pos] == "NAME-TREE" || types[link_pos] == "STREAM"
              || types[link_pos] == "ARRAY"))
            report_stream << "Type " << types[link_pos] << " not linked in: " << file_name << "::" << vc[TSV_KEYNAME] << std::endl;

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
                report_stream << "Link doesn't exist: " << lnk << " in: " << file_name << "::" << vc[TSV_KEYNAME] << std::endl;
            }
        }
      }
    }

    //check if each type is ok
    for (auto type:types)
      if (std::find(basic_types.begin(), basic_types.end(), type) == basic_types.end())
        report_stream << "Wrong type:" << type << " in:" << file_name << "::" << vc[TSV_KEYNAME] << std::endl;

    // check if complex type does have possible value
    for (size_t t_pos = 0; t_pos < types.size(); t_pos++)
      if ( (types[t_pos] == "ARRAY" || types[t_pos] == "DICTIONARY" || types[t_pos] == "NUMBER-TREE"
            || types[t_pos] == "NAME-TREE" || types[t_pos] == "STREAM") && vc[TSV_POSSIBLEVALUES] != "") {
        std::vector<std::string> def_val = split(vc[TSV_POSSIBLEVALUES], ';');
        if (def_val[t_pos]!="[]") 
          report_stream << "Complex type does have possible value defined:"<< vc[TSV_POSSIBLEVALUES] << " in:"<< file_name << "::" << vc[TSV_KEYNAME] << std::endl;
      }

    //if we have more types, check pattern in Required, default and possible values
    if (types.size() > 1) {
      if (vc[TSV_POSSIBLEVALUES] != "") {
        std::vector<std::string> poss_val = split(vc[TSV_POSSIBLEVALUES], ';');
        if (types.size()!=poss_val.size())
          report_stream << "Wrong # of types vs. # of possible values " << file_name << "::" << vc[TSV_KEYNAME] << std::endl;
     }
    }

    //report all possible values
    //if (vc[TSV_POSSIBLEVALUES]!="")
    //  report_stream << vc[TSV_POSSIBLEVALUES] << std::endl;

    //std::string s = vc[TSV_POSSIBLEVALUES];
    //std::regex functionStr("fn:\\w*\\([ A-Za-z0-9<>=@&|,]+\\)");
    //std::smatch match;
    //std::string str2 = "fn:";
    //if (ci_find_substr(vc[TSV_POSSIBLEVALUES], str2) != -1)
    //  report_stream << "TSV_POSSIBLEVALUES: " << file_name << std::endl;

    //if (std::regex_search(s, match, functionStr))
    //  std::cout << "match: " << match[1] << '\n';

    //report all special cases
    //if (vc[TSV_SPECIALCASE] != "")
    //  report_stream << vc[TSV_SPECIALCASE] << std::endl;

    //if ((vc[TSV_INHERITABLE] != "TRUE") && (vc[TSV_INHERITABLE] != "FALSE"))
    //  report_stream << file_name << "::" << vc[TSV_KEYNAME] << std::endl;
  }
  return true;
}
