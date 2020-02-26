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
  std::cout << "  testgrammar <input_file> <grammar_folder>" << std::endl;
  std::cout << std::endl;
  std::cout << "    input_file      - full pathname to input pdf" << std::endl;
  std::cout << "    output_folder   - folder with csv files representing PDF 2.0 Grammar" << std::endl;
  std::cout << std::endl;
  std::cout << "to checks grammar itself:" << std::endl;
  std::cout << "  testgrammar -v <grammar_folder>" << std::endl;
  std::cout << "    output_folder   - folder with csv files representing PDF 2.0 Grammar" << std::endl;
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
    std::string a1, a2;
    auto i = 1;
    if (argc > i) a1 = argv[i++];
    if (argc > i) a2 = argv[i++];

    std::wstring grammar_folder = FromUtf8(check_folder_path(a2));

    std::ofstream ofs;
    std::wstring save_path = L"w:\\report.txt";
    ofs.open(ToUtf8(save_path));
    
    // check grammar itself?
    if (a1 == "-v") {
      std::wstring search_path = grammar_folder;
      search_path += L"*.csv";
      WIN32_FIND_DATA fd;
      HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
      if (hFind != INVALID_HANDLE_VALUE) 
        do {
          if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            std::wstring file_name = grammar_folder;
            file_name += fd.cFileName;
            CGrammarReader reader(file_name);
            if (!reader.load())
              ofs << "Can't load grammar file:" << ToUtf8(file_name) << std::endl;
            else reader.check(ofs);
          }
        } while (::FindNextFile(hFind, &fd));
      ::FindClose(hFind);
      return 0;
    }
    
    // check pdf file
    std::wstring email = L"roman.toda@gmail.com";                                     // authorization email   
    std::wstring key = L"2C8ihBCkvEz4LSLbG";                                      // authorization license key
    std::wstring input_file = FromUtf8(a1);

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
