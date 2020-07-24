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
#include <filesystem>

#include "Pdfix.h"
#include "CheckGrammar.h"
#include "GrammarFile.h"

void CheckGrammar(std::string& grammar_folder, std::ofstream& ofs) {
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
      for (size_t i = 1; i < data.size(); i++) {
        std::vector<std::string> vc = data[i];
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
              if (lnk != "") to_process.push_back(lnk + ".tsv");
          }
        }
      }
    }
  }

  std::filesystem::path p(grammar_folder);
  for (const auto& entry : std::filesystem::directory_iterator(p)) 
    if (entry.is_regular_file() && entry.path().extension().string()==".tsv") {
      const auto file_name = entry.path().filename().string();
      if (std::find(processed.begin(), processed.end(), file_name) == processed.end()) {
        // file not reachable from Catalog
        ofs << "Can't reach from Trailer:" << file_name << std::endl;
      }
      std::string str = grammar_folder + file_name;
      CGrammarReader reader(str);
      if (!reader.load())
        ofs << "Can't load grammar file:" << file_name << std::endl;
      else reader.check(ofs);
    }
}


void CompareWithAdobe(std::wstring& adobe_file, std::string& grammar_folder, std::ofstream& ofs) {

}
