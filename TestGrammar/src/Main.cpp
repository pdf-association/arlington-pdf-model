///////////////////////////////////////////////////////////////////////////////
// Main.cpp
// Copyright (c) 2020 Normex, Pdfix. All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
 
#include <exception>
#include <iostream>
#include <string>

#include "Pdfix.h"

#include "Initialization.hpp"
#include "ParseObjects.h"
//#include "GrammarFile.h"
#include "CheckGrammar.h"


void show_help() {
  std::cout << "TestGrammar by Normex s.r.o. (c) 2020" << std::endl;
  std::cout << "Validates PDF file against grammar defined by list of TSV files. more info:https://github.com/romantoda/PDF20_Grammar" << std::endl;
  std::cout << std::endl;
  std::cout << "to validate single pdf file:" << std::endl;
  std::cout << "  testgrammar <input_file> <grammar_folder> <report_file>" << std::endl;
  std::cout << std::endl;
  std::cout << "    input_file      - full pathname to input pdf" << std::endl;
  std::cout << "    grammar_folder  - folder with tsv files representing PDF 2.0 Grammar" << std::endl;
  std::cout << "    report_file     - file for storing results" << std::endl;
  std::cout << "to validate folder with pdf files:" << std::endl;
  std::cout << "  testgrammar <input_folder> <grammar_folder> <report_folder>" << std::endl;
  std::cout << std::endl;
  std::cout << "    input_folder      - folder with pdf files" << std::endl;
  std::cout << "    grammar_folder  - folder with tsv files representing PDF 2.0 Grammar" << std::endl;
  std::cout << "    report_folder     - folder for storing results" << std::endl;
  std::cout << std::endl;
  std::cout << "to checks grammar itself:" << std::endl;
  std::cout << "  testgrammar -v <grammar_folder> <report_file>" << std::endl;
  std::cout << "    grammar_folder  - folder with tsv files representing PDF 2.0 Grammar" << std::endl;
  std::cout << "    report_file     - file for storing results" << std::endl;
  std::cout << "to compare with Adobe's grammar:" << std::endl;
  std::cout << "  testgrammar -c <grammar_folder> <report_file> <adobe_grammar_file>" << std::endl;
  std::cout << "    grammar_folder      - folder with tsv files representing PDF 2.0 Grammar" << std::endl;
  std::cout << "    report_file         - file for storing results" << std::endl;
  std::cout << "    adobe_grammar_file  - ????" << std::endl;
}

int main(int argc, char* argv[]) {
  clock_t tStart = clock();

  if (argc == 1) {
    show_help();
    return 0;
  }

  if (strcmp(argv[1], "/?") == 0) {
    show_help();
    return 0;
  }

  try {
    std::string a1, a2, a3, a4;
    auto i = 1;
    if (argc > i) a1 = argv[i++];
    if (argc > i) a2 = argv[i++];
    if (argc > i) a3 = argv[i++];
    if (argc > i) a4 = argv[i++];

    std::string grammar_folder = check_folder_path(a2);

    std::string save_path = a3; //"w:\\report.txt";

    // check grammar itself?
    if (a1 == "-v") {
      std::ofstream ofs;
      ofs.open(save_path);
      CheckGrammar(grammar_folder, ofs);
      return 0;
    }

    // init PDFix
    std::string input_file = a1;

    Initialization();
    if (a1 == "-c") {
      std::ofstream ofs;
      ofs.open(save_path);
      CompareWithAdobe(a4, grammar_folder, ofs);
    }
    else {
      Pdfix* pdfix = GetPdfix();
      PdfDoc* doc = nullptr;
      auto single_pdf = [&](const std::string &file_name, std::string report_file_name) {
        std::wstring open_file = FromUtf8(file_name);
        doc = pdfix->OpenDoc(open_file.c_str(), L"");
        if (!doc)
          throw std::runtime_error(std::to_string(pdfix->GetErrorType()));
        //acquire catalog
        PdsObject* root = doc->GetRootObject();
        if (!root)
          throw std::runtime_error(std::to_string(pdfix->GetErrorType()));
        //open report file
        std::ofstream ofs;
        ofs.open(report_file_name);

        CParsePDF parser(doc, grammar_folder, ofs);
        parser.parse_object(root, "Catalog", "Catalog");
        ofs << "Time:" << (double)(clock() - tStart) / CLOCKS_PER_SEC << std::endl;
        ofs.close();
        doc->Close();
      };

      if (folder_exists(input_file)) {
        std::wstring search_path = FromUtf8(input_file);
        search_path += L"*.pdf";
        WIN32_FIND_DATA fd;
        HANDLE hFind = ::FindFirstFile(search_path.c_str(), &fd);
        if (hFind != INVALID_HANDLE_VALUE)
          do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
              std::string str = input_file + ToUtf8(fd.cFileName);
              single_pdf(str, save_path+ ToUtf8(fd.cFileName)+".txt");
            }
          } while (::FindNextFile(hFind, &fd));
          ::FindClose(hFind);

      }
      else single_pdf(input_file, save_path);


      pdfix->Destroy();
    }

  }
  catch (std::exception& ex) {
    std::cout << ex.what() << std::endl;
    return 1;
  }

  return 0;
}
