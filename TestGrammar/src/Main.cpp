///////////////////////////////////////////////////////////////////////////////
// Main.cpp
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

#if defined __linux__
#include <cstring>
#endif

#include <filesystem>
#include "Pdfix.h"

#include "Initialization.hpp"
#include "ParseObjects.h"
#include "CheckGrammar.h"
#include "CheckGrammar.h"
#include "TestGrammarVers.h"

void show_help() {
  std::cout << "TestGrammar ver." << TestGrammar_VERSION << std::endl;
  std::cout << "Validates PDF file against grammar defined by list of TSV files." << std::endl;
  std::cout << std::endl;
  std::cout << "to validate single pdf file:" << std::endl;
  std::cout << "  testgrammar <input_file> <grammar_folder> <report_file>" << std::endl;
  std::cout << "    input_file      - full pathname to input pdf" << std::endl;
  std::cout << "    grammar_folder  - folder with tsv files representing PDF 2.0 Grammar" << std::endl;
  std::cout << "    report_file     - file for storing results" << std::endl;
  std::cout << std::endl;
  std::cout << "to validate folder with pdf files:" << std::endl;
  std::cout << "  testgrammar <input_folder> <grammar_folder> <report_folder>" << std::endl;
  std::cout << "    input_folder      - folder with pdf files" << std::endl;
  std::cout << "    grammar_folder  - folder with tsv files representing PDF 2.0 Grammar" << std::endl;
  std::cout << "    report_folder     - folder for storing results" << std::endl;
  std::cout << std::endl;
  std::cout << "to checks grammar itself:" << std::endl;
  std::cout << "  testgrammar -v <grammar_folder> <report_file>" << std::endl;
  std::cout << "    grammar_folder  - folder with tsv files representing PDF 2.0 Grammar" << std::endl;
  std::cout << "    report_file     - file for storing results" << std::endl;
  std::cout << std::endl;
  std::cout << "to compare with Adobe's grammar (not implemented yet):" << std::endl;
  std::cout << "  testgrammar -c <grammar_folder> <report_file> <adobe_grammar_file>" << std::endl;
  std::cout << "    grammar_folder      - folder with tsv files representing PDF 2.0 Grammar" << std::endl;
  std::cout << "    report_file         - file for storing results" << std::endl;
  std::cout << "    adobe_grammar_file  - ????" << std::endl;
}

#ifdef _WIN32
int wmain(int argc, wchar_t* argv[]) {
#elif defined __linux__
int main(int argc, char* argv[]) {
#endif
  //clock_t tStart = clock();
  if (argc == 1) {
    show_help();
    return 0;
  }

  try {
    std::wstring a1, a2, a3, a4;
    auto i = 1;

#ifdef _WIN32
    if (argc > i) a1 = argv[i++];
    if (argc > i) a2 = argv[i++];
    if (argc > i) a3 = argv[i++];
    if (argc > i) a4 = argv[i++];
#elif defined __linux__
    if (argc > i) a1 = utf8ToUtf16(argv[i++]);
    if (argc > i) a2 = utf8ToUtf16(argv[i++]);
    if (argc > i) a3 = utf8ToUtf16(argv[i++]);
    if (argc > i) a4 = utf8ToUtf16(argv[i++]);
#endif

    if (a1== L"/?") {
      show_help();
      return 0;
    }

    std::string grammar_folder = check_folder_path(ToUtf8(a2));
    std::wstring save_path = a3; //"w:\\report.txt";

    // check grammar itself?
    if (a1 == L"-v") {
      std::ofstream ofs;
      ofs.open(ToUtf8(save_path));
      CheckGrammar(grammar_folder, ofs);
      ofs.close();
      return 0;
    }

    // init PDFix
    std::wstring input_file = a1;

    Initialization();
    if (a1 == L"-c") {
      std::ofstream ofs;
      ofs.open(ToUtf8(save_path));
      CompareWithAdobe(a4, grammar_folder, ofs);
        ofs.close();
    }
    else {
      Pdfix* pdfix = GetPdfix();
      PdfDoc* doc = nullptr;
      auto single_pdf = [&](const std::wstring& file_name, std::wstring report_file_name) {
        std::wstring open_file = file_name;
        //open report file
        std::ofstream ofs;
        ofs.open(ToUtf8(report_file_name));
        ofs << "BEGIN - TestGrammar ver." << TestGrammar_VERSION << std::endl;

        doc = pdfix->OpenDoc(open_file.c_str(), L"");
        if (doc != nullptr) {
          //acquire catalog
          PdsObject* root = doc->GetRootObject();
          if (root != nullptr) {
            CParsePDF parser(doc, grammar_folder, ofs);
            parser.parse_object(root, "Catalog", "Catalog");
          }
          else ofs << "Failed to open:" << ToUtf8(file_name) << std::endl;
          doc->Close();
        }
        else ofs << "Failed to acquire Catalog in:" << ToUtf8(file_name) << std::endl;
        ofs << "END" << std::endl;
        ofs.close();
      };

      if (folder_exists(input_file)) {
        const std::filesystem::path p(input_file);
        for (const auto& entry : std::filesystem::directory_iterator(p))
          if (entry.is_regular_file() && entry.path().extension().wstring() == L".pdf") {
            const auto file_name = entry.path().filename().wstring();
            std::wstring str = input_file + file_name;
            single_pdf(str, save_path + file_name + L".txt");
          }
      }
      else
        single_pdf(input_file, save_path);

      pdfix->Destroy();
    }
  }
  catch (std::exception & ex) {
    std::cout << ex.what() << std::endl;
    return 1;
  }

  return 0;
}
