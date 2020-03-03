///////////////////////////////////////////////////////////////////////////////
// Main.cpp
// Copyright (c) 2020 Normex, Pdfix. All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
 
#include <exception>
#include <iostream>
#include <string>

#include "Pdfix.h"
#include "PdfToHtml.h"
#include "OcrTesseract.h"

#include "Initialization.hpp"
#include "ParseObjects.hpp"

void show_help() {
  std::cout << "TestGrammar by Normex s.r.o. (c) 2020" << std::endl;
  std::cout << "Validates PDF file against grammar defined by list of CSV files. more info:https://github.com/romantoda/PDF20_Grammar" << std::endl;
  std::cout << std::endl;
  std::cout << "to validate pdf file:" << std::endl;
  std::cout << "  testgrammar <input_file> <grammar_folder> <report_file>" << std::endl;
  std::cout << std::endl;
  std::cout << "    input_file      - full pathname to input pdf" << std::endl;
  std::cout << "    output_folder   - folder with csv files representing PDF 2.0 Grammar" << std::endl;
  std::cout << "    report_file     - file for storing results" << std::endl;
  std::cout << std::endl;
  std::cout << "to checks grammar itself:" << std::endl;
  std::cout << "  testgrammar -v <grammar_folder> <report_file>" << std::endl;
  std::cout << "    output_folder   - folder with csv files representing PDF 2.0 Grammar" << std::endl;
  std::cout << "    report_file     - file for storing results" << std::endl;
}

int main(int argc, char* argv[]) {
 
  if (argc == 1) {
    show_help();
    return 0;
  }

  if (strcmp(argv[1], "/?") == 0) {
    show_help();
    return 0;
  }

  try {
    std::string a1, a2, a3;
    auto i = 1;
    if (argc > i) a1 = argv[i++];
    if (argc > i) a2 = argv[i++];
    if (argc > i) a3 = argv[i++];

    std::string grammar_folder = check_folder_path(a2);

    std::ofstream ofs;
    std::string save_path = a3; //"w:\\report.txt";
    ofs.open(save_path);
    
    // check grammar itself?
    if (a1 == "-v") {

      // collecting all csv starting from Catalog
      std::vector<std::string> processed;
      std::vector<std::string> to_process;
      to_process.push_back("Catalog.csv");
      while (!to_process.empty()) {
        std::string gfile = to_process.back();
        to_process.pop_back();
        if (std::find(processed.begin(), processed.end(), gfile) == processed.end()) {
          processed.push_back(gfile);
          std::string gf = grammar_folder + gfile;
          CGrammarReader reader(gf);
          reader.load();
          const std::vector<std::vector<std::string>>& data = reader.get_data();
          for (int i = 1; i < data.size(); i++) {
            std::vector<std::string> vc = data[i];
            // does link exists ?
            if (vc[10] != "")
              to_process.push_back(vc[10]+".csv");
          }
        }
      }

      std::wstring search_path = FromUtf8(grammar_folder);
      search_path += L"*.csv";
      WIN32_FIND_DATA fd;
      HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
      if (hFind != INVALID_HANDLE_VALUE) 
        do {
          if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            std::string file_name = grammar_folder;
            std::string str = ToUtf8(fd.cFileName);
            if (std::find(processed.begin(), processed.end(), str) == processed.end()) {
              // file not reachable from Catalog
              ofs << "Can't reach from Catalog:" << str << std::endl;
            }

            file_name += str;
            CGrammarReader reader(file_name);
            if (!reader.load())
              ofs << "Can't load grammar file:" << file_name << std::endl;
            else reader.check(ofs);
          }
        } while (::FindNextFile(hFind, &fd));
      ::FindClose(hFind);
      return 0;
    }
    
    // check pdf file
    std::wstring email = L"roman.toda@gmail.com";                                     // authorization email   
    std::wstring key = L"2C8ihBCkvEz4LSLbG";                                      // authorization license key
    std::string input_file = a1;

    Initialization(email, key);
    ParsePdsObjects(input_file, grammar_folder, save_path);
    GetPdfix()->Destroy();
  }
  catch (std::exception& ex) {
    std::cout << ex.what() << std::endl;
    return 1;
  }
  return 0;
}
