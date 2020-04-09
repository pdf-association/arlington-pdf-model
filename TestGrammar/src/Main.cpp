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
  std::cout << "to validate pdf file:" << std::endl;
  std::cout << "  testgrammar <input_file> <grammar_folder> <report_file>" << std::endl;
  std::cout << std::endl;
  std::cout << "    input_file      - full pathname to input pdf" << std::endl;
  std::cout << "    grammar_folder  - folder with tsv files representing PDF 2.0 Grammar" << std::endl;
  std::cout << "    report_file     - file for storing results" << std::endl;
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

    std::ofstream ofs;
    std::string save_path = a3; //"w:\\report.txt";
    ofs.open(save_path);

    // check grammar itself?
    if (a1 == "-v") {
      CheckGrammar(grammar_folder, ofs);
      return 0;
    }

    // init PDFix
    std::string input_file = a1;

    Initialization();
    if (a1 == "-c") CompareWithAdobe(a4, grammar_folder, ofs);
    else {
      Pdfix* pdfix = GetPdfix();
      PdfDoc* doc = nullptr;

      //open pdf file
      std::wstring open_file = FromUtf8(input_file);
      doc = pdfix->OpenDoc(open_file.c_str(), L"");
      if (!doc)
        throw std::runtime_error(std::to_string(pdfix->GetErrorType()));
      //acquire catalog
      PdsObject* root = doc->GetRootObject();
      if (!root)
        throw std::runtime_error(std::to_string(pdfix->GetErrorType()));
      //open report file
      std::ofstream ofs;
      ofs.open(save_path);

      CParsePDF parser(doc, grammar_folder, ofs);
      parser.parse_object(root,"Catalog", "Catalog");
      ofs.close();
      doc->Close();
      pdfix->Destroy();
    }

  }
  catch (std::exception& ex) {
    std::cout << ex.what() << std::endl;
    return 1;
  }
  return 0;
}
