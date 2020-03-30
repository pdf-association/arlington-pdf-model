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
#include "GrammarFile.h"

#include "Pdfix.h"
#include "utils.h"

using namespace PDFixSDK;

#ifdef GetObject
#undef GetObject
#endif
bool ProcessObject(PdsObject* obj,
  std::ostream& ss,
  std::map<PdsObject*, int>& mapped,
  std::string grammar_file,
  std::string context
);


bool ProcessNameTree(PdsDictionary* obj,
  std::ostream& ss,
  std::map<PdsObject*, int>& mapped,
  std::string grammar_file,
  std::string context
) {
  // todo check if Kids doesn't exist together with names etc..
  PdsObject *kids_obj = obj->Get(L"Kids");
  PdsObject *names_obj = obj->Get(L"Names");
  PdsObject *limits_obj = obj->Get(L"Limits");
  bool to_ret = true;
  if (names_obj != nullptr) {
    if (names_obj->GetObjectType() == kPdsArray) {
      PdsArray* array_obj = (PdsArray*)names_obj;
      for (int i = 0; i < array_obj->GetNumObjects();) {
        // we have tupples "name", value. value has to be validated
        //todo check syntax here
        std::wstring str;
        str.resize(array_obj->GetText(i, nullptr, 0));
        array_obj->GetText(i, (wchar_t*)str.c_str(), (int)str.size());
        i++;
        PdsDictionary* item = array_obj->GetDictionary(i);
        i++;
        if (item != nullptr) {
          to_ret = ProcessObject(item, ss, mapped, grammar_file, context+ "->" + ToUtf8(str));
        }
        else {
          //error value isn't dictionary
        }

      }
    }
  }
  else {
    //error names isn't array
  }

  if (kids_obj != nullptr) {
    if (kids_obj->GetObjectType() == kPdsArray) {
      PdsArray* array_obj = (PdsArray*)kids_obj;
      for (int i = 0; i < array_obj->GetNumObjects(); ++i) {
        PdsDictionary* item = array_obj->GetDictionary(i);
        if (item != nullptr) {
          to_ret = to_ret && ProcessNameTree(item, ss, mapped, grammar_file, context);
        }
        else {
          //error kid isn't dictionary 
        }
      }
    }
    else { 
    //error kids isn't array
    }
  }
  return to_ret;
}

bool ProcessNumberTree(PdsDictionary* obj,
  std::ostream& ss,
  std::map<PdsObject*, int>& mapped,
  std::string grammar_file,
  std::string context
) {
  // todo check if Kids doesn't exist together with names etc..
  PdsObject *kids_obj = obj->Get(L"Kids");
  PdsObject *nums_obj = obj->Get(L"Nums");
  PdsObject *limits_obj = obj->Get(L"Limits");
  bool to_ret = true;
  if (nums_obj != nullptr) {
    if (nums_obj->GetObjectType() == kPdsArray) {
      PdsArray* array_obj = (PdsArray*)nums_obj;
      for (int i = 0; i < array_obj->GetNumObjects();) {
        // we have tupples number, value. value has to be validated
        //todo check syntax here
        int key = array_obj->GetInteger(i);
        i++;
        PdsDictionary* item = array_obj->GetDictionary(i);
        i++;
        if (item != nullptr) {
          to_ret = ProcessObject(item, ss, mapped, grammar_file, context + "->" + std::to_string(key));
        }
        else {
          //error value isn't dictionary
        }

      }
    }
  }
  else {
    //error names isn't array
  }

  if (kids_obj != nullptr) {
    if (kids_obj->GetObjectType() == kPdsArray) {
      PdsArray* array_obj = (PdsArray*)kids_obj;
      for (int i = 0; i < array_obj->GetNumObjects(); ++i) {
        PdsDictionary* item = array_obj->GetDictionary(i);
        if (item != nullptr) {
          to_ret = to_ret && ProcessNumberTree(item, ss, mapped, grammar_file, context);
        }
        else {
          //error kid isn't dictionary 
        }
      }
    }
    else {
      //error kids isn't array
    }

  }
  return to_ret;
}

// one line could have two types "array;dictionary" 
// function returns index into the "array" based on obj's type
// returns -1 if type doesn't exist in string
int get_type_index(PdsObject *obj, std::string types) {
  std::vector<std::string> opt = split(types, ';');
  for (int i = 0; i < opt.size(); i++) {
    if ((obj->GetObjectType() == kPdsBoolean) && (opt[i] == "BOOLEAN"))
      return i;
    if ((obj->GetObjectType() == kPdsNumber) && ((opt[i] == "NUMBER") || (opt[i] == "INTEGER")))
      return i;
    if ((obj->GetObjectType() == kPdsName) && (opt[i] == "NAME"))
      return i;
    if ((obj->GetObjectType() == kPdsStream) && (opt[i] == "STREAM"))
      return i;
    if ((obj->GetObjectType() == kPdsString) && ((opt[i] == "STRING") || (opt[i] == "DATE")))
      return i;
    if ((obj->GetObjectType() == kPdsArray) && ((opt[i] == "ARRAY") || (opt[i] == "RECTANGLE")))
      return i;
    if ((obj->GetObjectType() == kPdsDictionary) && ((opt[i] == "DICTIONARY") || (opt[i] == "NUMBER TREE") || (opt[i] == "NAME TREE")))
      return i;
  }
  return -1;
}

// ProcessObject gets the value of the object.
bool ProcessObject(PdsObject* obj, 
  std::ostream& ss, 
  std::map<PdsObject*, int>& mapped, 
  std::string grammar_file,
  std::string context
  ) {

  //if (obj->GetObjectType() != kPdsDictionary &&
  //  obj->GetObjectType() != kPdsStream &&
  //  obj->GetObjectType() != kPdsArray)
  //  return;

  auto found = mapped.find(obj);
  if (found != mapped.end()) {
//    ss << context << " already processed" <<std::endl;
    found->second++;
    return true;
  }
  
  ss << context << std::endl;
  mapped.insert(std::make_pair(obj, 1));

  CGrammarReader reader(grammar_file);
  if (!reader.load()) {
    ss << "Error: Can't load grammar file:" << grammar_file << std::endl;
    return false;
  }
  const std::vector<std::vector<std::string>> &data_list = reader.get_data();
  if (data_list.empty())
    ss << "Error: Empty grammar file:" << grammar_file << std::endl;

  auto get_full_csv_file = [=](std::string csv_name) {
    std::string file_name = get_path_dir(grammar_file);
    file_name += "/";
    file_name += csv_name.substr(1, csv_name.size() - 2);
    file_name += ".csv";
    return file_name;
  };


  //todo: what about stream and array
  
  // validating as dictionary:
  // going through all objects in dictionary 
  // checking types from the grammar
  // checking possible values
  // checking if indirect if required
  // if we change context based on value we do it
  // then check presence of required keys
  // then recursively calling validation for each container with link to other grammar file
  if (obj->GetObjectType() == kPdsDictionary || obj->GetObjectType() == kPdsStream) {
    bool to_ret = true;

    PdsDictionary* dictObj = (PdsDictionary*)obj;
    //validate values first, then process containers
    for (int i = 0; i < (dictObj->GetNumKeys()); i++) {
      std::wstring key;
      key.resize(dictObj->GetKey(i, nullptr, 0));
      dictObj->GetKey(i, (wchar_t*)key.c_str(), (int)key.size());

      //check Type, possible values, indirect
      // todo: ked mame key ktory neni v gramatike
      PdsObject *inner_obj = dictObj->Get(key.c_str());
      bool found = false;
      for (auto& vec : data_list)
        if (vec[0] == ToUtf8(key)) {
          // is indirect when needed ?
          if ((vec[5] == "TRUE") && (inner_obj->GetId() == 0)) {
            ss << "Error: not indirect ";
            ss << context << "->" << vec[0];
            ss <<"(" << grammar_file << ")" << std::endl;
            //ss << "<<" << obj->GetId() << " 0 obj>> (" << grammar_file << ") ";
            //ss << vec[0] << " not indirect " << std::endl;
          }

          // check type
          int index = get_type_index(inner_obj, vec[1]);
          if (index == -1) {
            ss << "Error:  wrong type ";
            ss << context << "->" << vec[0];
            ss << "(" << grammar_file << ")";
            ss << " should be:" << vec[1] << " and is " << inner_obj->GetObjectType() << std::endl;
          }

          // possible value, could be one of many 
          // if we have more values and also more links, then we switch context (follow the provided link) 
          // based on the value
          if (vec[8] != "") {
            std::wstring str;
            str.resize(dictObj->GetText(key.c_str(), nullptr, 0));
            dictObj->GetText(key.c_str(), (wchar_t*)str.c_str(), (int)str.size());

            std::vector<std::string> options = split(vec[8], ';');
            std::vector<std::string>::iterator it=std::find(options.begin(), options.end(), ToUtf8(str));
            if (it == options.end()) {
              ss << "Error:  wrong value ";
              ss << context << "->" << vec[0];
              ss << "(" << grammar_file << ")";
              ss << " should be:" << vec[8] << " and is " << ToUtf8(str) << std::endl;
              to_ret = false;
            }

            //else if (vec[10] != "") {
            //  // found value and we know we have links so we might need to switch the 
            //  // context in which we are (changing csv, while staying in the same dictionary
            //  int index = std::distance(options.begin(), it);
            //  std::vector<std::string> links = split(vec[10], ';');
            //  if (links.size() <= index) {
            //    ss << "Error:  can't find link based on value ";
            //    ss << context << "->" << vec[0];
            //    ss << "(" << grammar_file << ")" << std::endl;
            //  }
            //  else {
            //    mapped.erase(obj);
            //    ProcessObject(obj, ss, mapped, get_full_csv_file(links[index]), context);
            //  }
            //}
          }
          found = true;
          break;
        }
      // we didn't find key, there may be * we can use to validate
      if (!found)
        for (auto& vec : data_list)
          //todo: mozno treba osetrit aj typ
          if (vec[0] == "*" && vec[10] != "") {
            ProcessObject(inner_obj, ss, mapped, get_full_csv_file(vec[10]), context+"->"+ToUtf8(key));
            break;
          }
    }
    // check presence of required values
    for (auto& vec : data_list)
      if (vec[4] == "TRUE") {
        PdsObject *inner_obj = dictObj->Get(utf8ToUtf16(vec[0]).c_str());
        if (inner_obj == nullptr) {
          ss << "Error:  required key doesn't exists ";
          ss << context << "->" << vec[0];
          ss << "(" << grammar_file << ")" << std::endl;
        }
      }

    // now go through containers and process them with new grammar_file
    //todo: arrays also
    //todo: what if there are 2 links?
    //todo: dictionary bez linku
    //todo: ked sme ich nepretestovali skor (*) 
    for (auto& vec : data_list)
      if (vec.size()>=11 && vec[10] != "") {
        PdsObject *inner_obj = dictObj->Get(utf8ToUtf16(vec[0]).c_str());
        if (inner_obj != nullptr) {
          int index = get_type_index(inner_obj, vec[1]);
          std::vector<std::string> opt = split(vec[1], ';');
          std::vector<std::string> links = split(vec[10], ';');
          if (opt[index] == "NUMBER TREE" && inner_obj->GetObjectType() == kPdsDictionary) {
            to_ret = to_ret && ProcessNumberTree((PdsDictionary*)inner_obj, ss, mapped, get_full_csv_file(links[index]), context + "->" + vec[0]);
          }
          else
            if (opt[index] == "NAME TREE" && inner_obj->GetObjectType() == kPdsDictionary) {
              to_ret = to_ret && ProcessNameTree((PdsDictionary*)inner_obj, ss, mapped, get_full_csv_file(links[index]), context + "->" + vec[0]);
            }
            else if (inner_obj->GetObjectType() == kPdsStream) {
              to_ret = to_ret && ProcessObject(((PdsStream*)inner_obj)->GetStreamDict(), ss, mapped, get_full_csv_file(links[index]), context + "->" + vec[0]);
            }
            else
              if ((inner_obj->GetObjectType() == kPdsDictionary || inner_obj->GetObjectType() == kPdsArray))
                //rt moze byt bud empty alebo ich moze byt viac
                to_ret = to_ret && ProcessObject(inner_obj, ss, mapped, get_full_csv_file(links[index]), context + "->" + vec[0]);
        }
      }
    return to_ret;
  }

  if (obj->GetObjectType() == kPdsArray) {
    bool to_ret = true;
    PdsArray* arrayObj = (PdsArray*)obj;
    for (int i = 0; i < arrayObj->GetNumObjects(); ++i) {
      //PdsDictionary* item = arrayObj->GetDictionary(i);
      PdsObject* item = arrayObj->Get(i);
      for (auto& vec : data_list)
        //todo: mozno treba osetrit aj typ
        if (vec[0] == "*" && vec[10] != "" && vec[10] != "[]") {
          std::vector<std::string> direct_links = split(vec[10].substr(1, vec[10].size() - 2), ',');
          to_ret = false;
          for (auto lnk : direct_links)
            if (ProcessObject(item, ss, mapped, get_full_csv_file(lnk), context + "[" + std::to_string(i) + "]")) {
              ss << " XXXXX Validated as " << lnk << std::endl;
              to_ret = true;
              break;
            }
          break;
        }
    }
    return to_ret;
  }

  ss << "Error:  can't process ";
  ss << context;
  ss << "(" << grammar_file << ")" << std::endl;
  return true;
  //ss << "<<" << obj->GetId() << " 0 obj>> (" << grammar_file << ") ";
  //ss << " is not dictionary! " << std::endl;
}

// Iterates all documents 
void ParsePdsObjects(
  const std::string& open_path,                       // source PDF document
  const std::string& grammar_folder,                  // folder for grammar files
  const std::string& save_path                        // report file
  ) {
  Pdfix* pdfix = GetPdfix();
  PdfDoc* doc = nullptr;
  std::wstring open_file = FromUtf8(open_path);
  doc = pdfix->OpenDoc(open_file.c_str(), L"");
  if (!doc)
    throw std::runtime_error(std::to_string(GetPdfix()->GetErrorType()));

  PdsObject* root = doc->GetRootObject();
  if (!root)
    throw std::runtime_error(std::to_string(GetPdfix()->GetErrorType()));

  std::map<PdsObject*, int> mapped;

  std::ofstream ofs;
  ofs.open(save_path);
  std::string grammar_file = grammar_folder + "Catalog.csv";
  ProcessObject(root, ofs, mapped, grammar_file, "Catalog");
  ofs.close();

  doc->Close();
}
//! [ParseObjects_cpp]
