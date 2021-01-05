///////////////////////////////////////////////////////////////////////////////
// CheckGrammar.cpp
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

#include <exception>
#include <iostream>
#include <string>
#include <iterator>
#include <filesystem>
#include <queue>
#include <map>
#include <algorithm>
#include <cctype>

#include "Pdfix.h"
#include "CheckGrammar.h"
#include "GrammarFile.h"
#include "TestGrammarVers.h"

using namespace PDFixSDK;

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
 Checks the validity of the loaded tsv data
 - correct # of columns
 - correct headings (first line)
 - correct basic types 1st column
 */
bool check_grammar(CGrammarReader& reader, std::ostream& report_stream)
{
  std::string function;
  auto data_list = reader.get_data();
  if (data_list.empty()) {
    report_stream << "Empty grammar file: " << reader.file_name << std::endl;
    return false;
  }

  if (reader.header_list.size() < 10) {
    report_stream << "Wrong number of columns: " << reader.file_name << std::endl;
    return false;
  }

  if ((reader.header_list[TSV_KEYNAME] != "Key") || (reader.header_list[TSV_TYPE] != "TYPE") || 
    (reader.header_list[TSV_SINCEVERSION] != "SinceVersion") ||
    (reader.header_list[TSV_DEPRECATEDIN] != "DeprecatedIn") || (reader.header_list[TSV_REQUIRED] != "REQUIRED") ||
    (reader.header_list[TSV_INDIRECTREF] != "INDIRECTREFERENCE") || (reader.header_list[TSV_INHERITABLE] != "INHERITABLE") ||
    (reader.header_list[TSV_DEFAULTVALUE] != "DefaultValue") || (reader.header_list[TSV_POSSIBLEVALUES] != "PossibleValues") ||
    (reader.header_list[TSV_SPECIALCASE] != "SpecialCase") || (reader.header_list[TSV_LINK] != "Link")) {
    report_stream << "Wrong headers for columns: " << reader.file_name << std::endl;
    return false;
  }

  // check basic types (ignoring first line)
  // check existing link
  // check duplicate keys
  std::vector<std::string> processed;
  for (auto& vc : data_list) {
    
    if (std::find(processed.begin(), processed.end(), vc[TSV_KEYNAME]) == processed.end())
      processed.push_back(vc[TSV_KEYNAME]);
    else
      report_stream << "Duplicate keys in: " << reader.file_name << "::" << vc[TSV_KEYNAME] << std::endl;

    // possible multiple types separated with ";"
    // need to compare all of them with basic_types
    std::vector<std::string> types = split(vc[TSV_TYPE], ';');
    std::vector<std::string> links = split(vc[TSV_LINK], ';');
    std::vector<std::string> possible_val = split(vc[TSV_POSSIBLEVALUES], ';');

    // if link exists we check
    // - number of links and number of types match
    // - each link follows patter [];[]..
    // - each dictionary, array etc.. is linked
    // - each link actuall exists
    if (vc[TSV_LINK] != "") {
      if (links.size() != types.size())
        report_stream << "Wrong # of types vs. # of links " << reader.file_name << "::" << vc[TSV_KEYNAME] << std::endl;
      for (size_t link_pos = 0; link_pos < links.size(); link_pos++) {
        //if (!std::regex_match(links[link_pos], regex)) {
        //  report_stream << "Wrong pattern in links " << file_name << "::" << vc[TSV_KEYNAME] << std::endl;
        //}
        //else {
          //report all unliked complex types
        if ((types.size() > link_pos) && (links[link_pos] == "[]") &&
          (types[link_pos] == "DICTIONARY" || types[link_pos] == "NUMBER-TREE"
            || types[link_pos] == "NAME-TREE" || types[link_pos] == "STREAM"
            || types[link_pos] == "ARRAY"))
          report_stream << "Type " << types[link_pos] << " not linked in: " << reader.file_name << "::" << vc[TSV_KEYNAME] << std::endl;

        std::vector<std::string> direct_links = split(links[link_pos].substr(1, links[link_pos].size() - 2), ',');

        // report all multiple links - all the places where implementor has to decide which one to validate
        //if (direct_links.size()>1)
        //  for (auto lnk : direct_links)
        //    if (lnk != "") 
        //      report_stream << lnk << std::endl;

        for (auto lnk : direct_links)
          if (lnk != "") {
            std::string new_name = get_path_dir(reader.file_name);
            new_name += "/";
            auto direct_lnk = extract_function(lnk, function);
            //check if all links are encapsulated in a function
            //if (function=="")
            //  report_stream << "Link doesn't have function: " << lnk << " in: " << file_name << "::" << vc[TSV_KEYNAME] << std::endl;
            new_name += direct_lnk;
            new_name += ".tsv";
            if (!file_exists(new_name))
              report_stream << "Link doesn't exist: " << lnk << " in: " << reader.file_name << "::" << vc[TSV_KEYNAME] << std::endl;
          }
      }
    }

    // Check if each Type is ok
    for (auto type : types) {
      if ((std::find(reader.basic_types.begin(), reader.basic_types.end(), type)) == reader.basic_types.end()) {
        // A few types are also wrapped in declarative functions, but these have already been converted to uppercase
        // Need to transpose back to lowercase to use extract_function() then back to uppercase to match.
        std::transform(type.begin(), type.end(), type.begin(), [](unsigned char c) { return std::tolower(c); });
        auto t = extract_function(type, function);
        std::transform(t.begin(), t.end(), t.begin(), [](unsigned char c) { return std::toupper(c); });
        if ((std::find(reader.basic_types.begin(), reader.basic_types.end(), t)) == reader.basic_types.end()) {
          report_stream << "Wrong type: " << type << " in:" << reader.file_name << "::" << vc[TSV_KEYNAME] << std::endl;
        }
      }
    }

    // check if complex type does have possible value
    for (size_t t_pos = 0; t_pos < types.size(); t_pos++)
      if ((types[t_pos] == "ARRAY" || types[t_pos] == "DICTIONARY" || types[t_pos] == "NUMBER-TREE"
        || types[t_pos] == "NAME-TREE" || types[t_pos] == "STREAM") && vc[TSV_POSSIBLEVALUES] != "") {
        if (possible_val[t_pos] != "[]")
          report_stream << "Complex type does have possible value defined: " << vc[TSV_POSSIBLEVALUES] << " in:" << reader.file_name << "::" << vc[TSV_KEYNAME] << std::endl;
      }

    //check pattern in possible values
    if (vc[TSV_POSSIBLEVALUES] != "")
      if (types.size() != possible_val.size())
        report_stream << "Wrong # of types vs. # of possible values " << reader.file_name << "::" << vc[TSV_KEYNAME] << std::endl;

    //if (vc[TSV_KEYNAME]=="Subtype" && (vc[TSV_POSSIBLEVALUES]=="" || vc[TSV_POSSIBLEVALUES]=="[]"))
    //  report_stream << "Undefined Subtype " << file_name << "::" << vc[TSV_KEYNAME] << std::endl;

    //report all indirect fields
    //if (vc[TSV_INDIRECTREF]!="FALSE")
    //  report_stream << "Indirect " << file_name << "::" << vc[TSV_KEYNAME] << std::endl;

    //report all possible values
    //if (vc[TSV_POSSIBLEVALUES]!="")
    //  report_stream << vc[TSV_POSSIBLEVALUES] << std::endl;

    //report all special cases
    //if (vc[TSV_SPECIALCASE] != "")
    //  report_stream << vc[TSV_SPECIALCASE] << std::endl;

    //report all irregular inheritables
    //if ((vc[TSV_INHERITABLE] != "TRUE") && (vc[TSV_INHERITABLE] != "FALSE"))
    //  report_stream << file_name << "::" << vc[TSV_KEYNAME] << std::endl;

    //report all required fields that are inheritable
    //if ((vc[TSV_INHERITABLE] == "TRUE") && (vc[TSV_REQUIRED] == "TRUE"))
    //  report_stream << reader.file_name << "::" << vc[TSV_KEYNAME] << std::endl;

    //report all required fields that have possible value
    //if ((vc[TSV_POSSIBLEVALUES] != "") && (vc[TSV_REQUIRED] == "TRUE"))
    //  report_stream << reader.file_name << "::" << vc[TSV_KEYNAME] << "  "<< vc[TSV_POSSIBLEVALUES]  <<std::endl;
    report_stream.flush();
  }
  return true;
}


void CheckGrammarFolder(std::string& grammar_folder, std::ofstream& ofs) {
  // collecting all tsv starting from Catalog
  std::vector<std::string> processed;
  std::vector<std::string> to_process;
  //to_process.push_back("Catalog.tsv");
  to_process.push_back("FileTrailer.tsv");
  while (!to_process.empty()) { 
    std::string gfile = to_process.back();
    to_process.pop_back();
    if (std::find(processed.begin(), processed.end(), gfile) == processed.end()) {
      processed.push_back(gfile);
      std::string gf = grammar_folder + gfile;
      CGrammarReader reader(gf);
      reader.load();
      const std::vector<std::vector<std::string>>& data = reader.get_data();
      for (auto& vc : data) {
        //for (size_t i = 1; i < data.size(); i++) {
        //  std::vector<std::string> vc = data[i];
          // does link exists ?
          // we have to parse pattern [lnk1,lnk2];[lnk3,lnk4];[]
          //if (vc[TSV_KEYNAME] == "*") {
          //  ofs << "* in" << gfile << std::endl;
          //}
        if (vc[TSV_LINK] != "") {
          std::vector<std::string> links = split(vc[TSV_LINK], ';');
          for (auto type_link : links) {
            std::vector<std::string> direct_links = split(type_link.substr(1, type_link.size() - 2), ',');
            for (auto lnk : direct_links)
              if (lnk != "") {
                std::string function;
                auto direct_lnk = extract_function(lnk, function);
                to_process.push_back(direct_lnk + ".tsv");
              }
          }
        }
      }
    }
  }

  std::filesystem::path p(grammar_folder);
  for (const auto& entry : std::filesystem::directory_iterator(p))
    if (entry.is_regular_file() && entry.path().extension().string() == ".tsv") {
      const auto file_name = entry.path().filename().string();
      if (std::find(processed.begin(), processed.end(), file_name) == processed.end()) {
        // file not reachable from Catalog
        ofs << "Can't reach from Trailer: " << file_name << std::endl;
      }
      std::string str = grammar_folder + file_name;
      CGrammarReader reader(str);
      if (!reader.load())
        ofs << "Can't load grammar file: " << file_name << std::endl;
      else check_grammar(reader, ofs);
    }
}
