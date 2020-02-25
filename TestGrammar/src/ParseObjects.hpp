////////////////////////////////////////////////////////////////////////////////////////////////////
// ParseObjects.cpp
// Copyright (c) 2020 Normex, Pdfix. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
  Reading the whole PDF starting from catalog and validating against grammar provided via cvs file
*/

#pragma once

//! [ParseObjects_cpp]
#include <string>
#include <map>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <memory>
#include <algorithm> 
#include <codecvt>
#include "GrammarFile.hpp"

#include "Pdfix.h"
#include "utils.h"

#ifdef GetObject
#undef GetObject
#endif

// ProcessObject gets the value of the object.
void ProcessObject(PdsObject* obj, std::ostream& ss, std::map<PdsObject*, int>& mapped, std::wstring grammar_file) {

  auto found = mapped.find(obj);
  if (found != mapped.end()) {
    found->second++;
    return;
  }
  
  CGrammarReader reader(grammar_file);
  if (!reader.load()) {
    ss << "Can't load grammar file:" << grammar_file.c_str() << std::endl;
    return;
  }
  std::vector<std::vector<std::string> > data_list = reader.get_data();
  if (data_list.empty())
    ss << "Empty grammar file:" << grammar_file.c_str() << std::endl;

  //auto get_obj_type_str = [](PdsObject *obj) {
  //  std::string str;
  //  switch (obj->GetObjectType()) {
  //  case kPdsBoolean: str = "BOOLEAN"; break;
  //  case kPdsNumber: str = "NUMBER"; break;
  //  case kPdsName: str = "NAME"; break;
  //  case kPdsString: str = "STRING"; break;
  //  case kPdsStream: str = "STREAM"; break;
  //  case kPdsArray: str = "ARRAY"; break;
  //  case kPdsDictionary: str = "DICTIONARY"; break;
  //  default: str = "NONE"; break;
  //  }
  //  return str;
  //};

  mapped.insert(std::make_pair(obj, 1));
  if (obj->GetObjectType() == kPdsDictionary) {
    PdsDictionary* dictObj = (PdsDictionary*)obj;

    //validate values first, then process containers
    for (int i = 0; i < (dictObj->GetNumKeys()); i++) {
      std::wstring key;
      key.resize(dictObj->GetKey(i, nullptr, 0));
      dictObj->GetKey(i, (wchar_t*)key.c_str(), (int)key.size());

      //check Type, possible values, indirect
      PdsObject *inner_obj = dictObj->Get(key.c_str());
      for (std::vector<std::string> vec : data_list)
        if (vec[0] == ToUtf8(key)) {

          // is indirect when needed ?
          if ((vec[5] == "TRUE") && (inner_obj->GetId() == 0)) {
            ss << "<<" << obj->GetId() << " 0 obj>> (" << grammar_file.c_str() << ") ";
            ss << vec[0] << " not indirect " << std::endl;
          }

          // proper type ?
          bool is_type_ok = false;
          switch (inner_obj->GetObjectType()) {
          case kPdsBoolean: is_type_ok = (vec[1] == "BOOLEAN"); break;
          case kPdsNumber: is_type_ok = (vec[1] == "NUMBER") || (vec[1] == "INTEGER"); break;
          case kPdsName: is_type_ok = (vec[1] == "NAME"); break;
          case kPdsString: is_type_ok = (vec[1] == "STRING") || (vec[1] == "DATE"); break;
          case kPdsStream: is_type_ok = (vec[1] == "STREAM"); break;
          case kPdsArray: is_type_ok = (vec[1] == "ARRAY" || (vec[1] == "RECTANGLE")); break;
          case kPdsDictionary: is_type_ok = (vec[1] == "DICTIONARY"); break;
          }
          if (!is_type_ok) {
            ss << "<<" << obj->GetId() << " 0 obj>> (" << grammar_file.c_str() << ") ";
            ss << "key:" << vec[0];
            ss << " wrong type should be:" << vec[1] << " and is " << inner_obj->GetObjectType() << std::endl;
          }

          // possible value
          //todo: one of many
          if (vec[8] != "") {
            std::string str;
            str.resize(dictObj->GetString(key.c_str(), nullptr, 0));
            dictObj->GetString(key.c_str(), (char*)str.c_str(), (int)str.size());
            if (str != vec[8]) {
              ss << "<<" << obj->GetId() << " 0 obj>> (" << grammar_file.c_str() << ") ";
              ss << "key:" << vec[0];
              ss << " wrong value should be:" << vec[8] << " and is " << str << std::endl;
            }
          }

          break;
        }
    }
    // check presence of required values
    for (std::vector<std::string> vec : data_list)
      if (vec[4] == "TRUE") {
        PdsObject *inner_obj = dictObj->Get(utf8ToUtf16(vec[0]).c_str());
        if (inner_obj == nullptr) {
          ss << "<<" << obj->GetId() << " 0 obj>> (" << grammar_file.c_str() << ") ";
          ss << vec[0] << " is required but doesn't exist " << std::endl;
        }
      }

    // now do through containers and process them with new grammar_file
    //todo: arrays also
    //todo: what if there are 2 links?
    for (std::vector<std::string> vec : data_list)
      if (vec.size()>=11 && vec[10] != "") {
        PdsObject *inner_obj = dictObj->Get(utf8ToUtf16(vec[0]).c_str());
        if (inner_obj != nullptr) {
          std::wstring file_name = get_path_dir(grammar_file);
          file_name += FromUtf8(vec[10]);
          file_name += L".csv";
          ProcessObject(inner_obj, ss, mapped, file_name);
        }
      }
  }
  else {
    ss << "<<" << obj->GetId() << " 0 obj>> (" << grammar_file.c_str() << ") ";
    ss << " is not dictionary! " << std::endl;
  }
}

// Iterates all documents 
void ParsePdsObjects(
  const std::wstring& open_path,                       // source PDF document
  const std::wstring& grammar_folder,                  // folder for grammar files
  const std::wstring& save_path                        // report file
  ) {
  Pdfix* pdfix = GetPdfix();

  PdfDoc* doc = nullptr;
  doc = pdfix->OpenDoc(open_path.c_str(), L"");
  if (!doc)
    throw std::runtime_error(std::to_string(GetPdfix()->GetErrorType()));

  PdsObject* root = doc->GetRootObject();
  if (!root)
    throw std::runtime_error(std::to_string(GetPdfix()->GetErrorType()));

  std::map<PdsObject*, int> mapped;

  std::ofstream ofs;
  ofs.open(ToUtf8(save_path));
  std::wstring grammar_file = grammar_folder + L"Catalog.csv";
  ProcessObject(root, ofs, mapped, grammar_file);
  ofs.close();

  doc->Close();
}
//! [ParseObjects_cpp]
