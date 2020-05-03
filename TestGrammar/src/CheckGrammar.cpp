///////////////////////////////////////////////////////////////////////////////
// CheckGrammar.cpp
// 2020 Roman Toda, Normex
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
  to_process.push_back("Catalog.tsv");
//  to_process.push_back("FileTrailer.tsv");
  while (!to_process.empty()) {
    std::string gfile = to_process.back();
    to_process.pop_back();
    if (std::find(processed.begin(), processed.end(), gfile) == processed.end()) {
      processed.push_back(gfile);
      std::string gf = grammar_folder + gfile;
      CGrammarReader reader(gf);
      reader.load();
      const std::vector<std::vector<std::string>>& data = reader.get_data();
      for (auto i = 1; i < data.size(); i++) {
        std::vector<std::string> vc = data[i];
        // does link exists ?
        // we have to parse pattern [lnk1,lnk2];[lnk3,lnk4];[]
        //if (vc[0] == "*") {
        //  ofs << "* in" << gfile << std::endl;
        //}
        if (vc[10] != "") {
          std::vector<std::string> links = split(vc[10], ';');
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
        ofs << "Can't reach from Catalog:" << file_name << std::endl;
      }
      std::string str = grammar_folder + file_name;
      CGrammarReader reader(str);
      if (!reader.load())
        ofs << "Can't load grammar file:" << file_name << std::endl;
      else reader.check(ofs);

    }
  //std::wstring search_path = FromUtf8(grammar_folder);
  //search_path += L"*.tsv";
  //WIN32_FIND_DATA fd;
  //HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
  //if (hFind != INVALID_HANDLE_VALUE)
  //  do {
  //    if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
  //      std::string file_name = grammar_folder;
  //      std::string str = ToUtf8(fd.cFileName);
  //      if (std::find(processed.begin(), processed.end(), str) == processed.end()) {
  //        // file not reachable from Catalog
  //        ofs << "Can't reach from Catalog:" << str << std::endl;
  //      }

  //      file_name += str;
  //      CGrammarReader reader(file_name);
  //      if (!reader.load())
  //        ofs << "Can't load grammar file:" << file_name << std::endl;
  //      else reader.check(ofs);
  //    }
  //  } while (::FindNextFile(hFind, &fd));
  //  ::FindClose(hFind);
}


void CompareWithAdobe(std::string& adobe_file, std::string& grammar_folder, std::ofstream& ofs) {

}
