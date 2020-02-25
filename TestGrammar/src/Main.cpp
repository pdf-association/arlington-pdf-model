///////////////////////////////////////////////////////////////////////////////
// Main.cpp
// Copyright (c) 2020 Normex, Pdfix. All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
 
#include <exception>
#include <iostream>
#include "Pdfix.h"
#include "PdfToHtml.h"
#include "OcrTesseract.h"

#include "Initialization.hpp"
#include "ParseObjects.hpp"

extern std::wstring GetAbsolutePath(const std::wstring& path);

int main()
{
  std::wstring email = L"roman.toda@gmail.com";                                     // authorization email   
  std::wstring key = L"2C8ihBCkvEz4LSLbG";                                      // authorization license key
  
  std::wstring resources_dir = GetAbsolutePath(L"../resources");
  std::wstring output_dir = GetAbsolutePath(L"../output");

  std::wstring open_path = resources_dir + L"\\test.pdf";        // source PDF document

  try {
    Initialization(email, key);
    ParsePdsObjects(email, key, open_path, output_dir + L"/ParsePdsObjects.txt");
  }
  catch (std::exception& ex) {
    std::cout << "Error: " << ex.what() << std::endl;
	  return 1;
  }
  return 0;
}

