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

extern std::string ToUtf8(const std::wstring& wstr);
extern std::string GetAbsolutePath(const std::string& path);

std::wstring utf8ToUtf16(const std::string& utf8Str)
{
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
  return conv.from_bytes(utf8Str);
}

#ifdef GetObject
#undef GetObject
#endif

// ProcessObject gets the value of the object.
void ProcessObject(PdsObject* obj, std::ostream& ss, std::map<PdsObject*, int>& mapped, std::string grammar_file) {

  auto found = mapped.find(obj);
  if (found != mapped.end()) {
    found->second++;
    return;
  }
  
  std::string file_name = GetAbsolutePath("../../csv/");
  file_name += grammar_file;
  file_name += ".csv";
  CGrammarReader reader(file_name,ss);
  std::vector<std::vector<std::string> > data_list = reader.get_data();
  if (data_list.empty())
    ss << "Can't load grammar file:" << grammar_file << std::endl;



  auto get_obj_type_str = [](PdsObject *obj) {
    std::string str;
    switch (obj->GetObjectType()) {
    case kPdsBoolean: str = "BOOLEAN"; break;
    case kPdsNumber: str = "NUMBER"; break;
    case kPdsName: str = "NAME"; break;
    case kPdsString: str = "STRING"; break;
    case kPdsStream: str = "STREAM"; break;
    case kPdsArray: str = "ARRAY"; break;
    case kPdsDictionary: str = "DICTIONARY"; break;
    default: str = "NONE"; break;
    }
    return str;
  };

  mapped.insert(std::make_pair(obj, 1));
  if (obj->GetObjectType() == kPdsDictionary) {
    PdsDictionary* dictObj = (PdsDictionary*)obj;

    //validate values first, the process containers
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
            ss << "<<" << obj->GetId() << " 0 obj>> (" << grammar_file << ") ";
            ss << vec[0] << " not indirect " << std::endl;
          }

          // proper type ?
          if ((vec[1] != get_obj_type_str(inner_obj))) {
            ss << "<<" << obj->GetId() << " 0 obj>> (" << grammar_file << ") ";
            ss << "key:" << vec[0];
            ss << " wrong type should be:" << vec[1] << " and is " << get_obj_type_str(inner_obj) << std::endl;
          }

          // possible value ?
          if (vec[8] != "") {
            std::string str;
            str.resize(dictObj->GetString(key.c_str(), nullptr, 0));
            dictObj->GetString(key.c_str(), (char*)str.c_str(), (int)str.size());
            if (str != vec[8]) {
              ss << "<<" << obj->GetId() << " 0 obj>> (" << grammar_file << ") ";
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
          ss << "<<" << obj->GetId() << " 0 obj>> (" << grammar_file << ") ";
          ss << vec[0] << " is required but doesn't exist " << std::endl;
        }
      }

    // now do through containers and process them with new grammar_file
    for (std::vector<std::string> vec : data_list)
      if (vec.size()>=11 && vec[10] != "") {
        PdsObject *inner_obj = dictObj->Get(utf8ToUtf16(vec[0]).c_str());
        if (inner_obj != nullptr)
          ProcessObject(inner_obj, ss, mapped, vec[10]);
      }
  }
  else {
    ss << "<<" << obj->GetId() << " 0 obj>> (" << grammar_file << ") ";
    ss << " is not dictionary! " << std::endl;
  }
}

// Iterates all documents bookmars.
void ParsePdsObjects(
  const std::wstring& email,                           // authorization email   
  const std::wstring& license_key,                     // authorization license key
  const std::wstring& open_path,                       // source PDF document
  const std::wstring& save_path                        // output document
  ) {
  // initialize Pdfix
  if (!Pdfix_init(Pdfix_MODULE_NAME))
    throw std::runtime_error("Pdfix initialization fail");

  Pdfix* pdfix = GetPdfix();
  if (!pdfix)
    throw std::runtime_error("GetPdfix fail");
  if (!pdfix->Authorize(email.c_str(), license_key.c_str()))
    throw std::runtime_error(std::to_string(GetPdfix()->GetErrorType()));

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
  ProcessObject(root, ofs, mapped, "Catalog");
  ofs.close();

  doc->Close();
  pdfix->Destroy();
}
//! [ParseObjects_cpp]
