////////////////////////////////////////////////////////////////////////////////////////////////////
// ParseObjects.cpp
// Copyright (c) 2020 Normex, Pdfix. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
  Reading the whole PDF starting from specific object and validating against grammar provided via tsv file
*/

#pragma once

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
#include "ParseObjects.h"

#include "Pdfix.h"
#include "utils.h"

using namespace PDFixSDK;

#ifdef GetObject
#undef GetObject
#endif

//std::string get_full_csv_file(std::string &grammar_file, const std::string &csv_name) {
//  std::string file_name = get_path_dir(grammar_file);
//  file_name += "/";
//  if (csv_name.front() == '['  && csv_name.back() == ']')
//    file_name += csv_name.substr(1, csv_name.size() - 2);
//  else
//    file_name += csv_name;
//  file_name += ".csv";
//  return file_name;
//};


// choose one from provided links to validate further
// the decision is made based on 
// - 
// if we had to choose one from more, we update context with (as ChosenLink)
std::string CParsePDF::select_one(PdsObject* obj, const std::string &links_string, std::string &obj_name) {
  if (links_string == "[]" || links_string == "")
    return "";

  std::vector<std::string> links = split(links_string.substr(1, links_string.size() - 2), ',');
  if (links.size() == 1)
    return links[0];

  int to_ret = -1;
  for (int i = 0; i < links.size(); i++) {
    auto lnk = links[i];
    std::string grammar_file = grammar_folder + lnk + ".tsv";
    CGrammarReader reader(grammar_file);
    if (!reader.load())
      continue;

    const std::vector<std::vector<std::string>> &data_list = reader.get_data();
    if (data_list.empty())
      continue;

    to_ret = i;
    // first need to go through "required" - that may fail the validation right away
    if (obj->GetObjectType() == kPdsDictionary || obj->GetObjectType() == kPdsStream) {
      // are all "required" fields has to be present
      // and if required value is defined then has to match with value
      PdsDictionary* dictObj = (PdsDictionary*)obj;
      for (int j = 1; j < data_list.size(); j++) {
        auto& vec = data_list[j];
        if (vec[4] == "TRUE") {
          std::wstring str_value;
          //required value eists?
          if (!dictObj->Known(utf8ToUtf16(vec[0]).c_str())) {
            to_ret = -1;
            break;
          }
          str_value.resize(dictObj->GetText(utf8ToUtf16(vec[0]).c_str(), nullptr, 0));
          dictObj->GetText(utf8ToUtf16(vec[0]).c_str(), (wchar_t*)str_value.c_str(), (int)str_value.size());

          if (vec[6] != "" && vec[6] != ToUtf8(str_value)) {
            to_ret = -1;
            break;
          }
        }
      }
    }

    // if all required are there - return this position in list of links
    if (to_ret != -1) {
      obj_name += "(as " + links[to_ret] + ")";
      return links[to_ret];
    }
  }
  output << "Error: Can't select any link from " << links_string <<" to validate provided object:" << obj_name <<  std::endl;
  return "";
}

// returns specific link for provided object (decision is made by type)
// for: array;number as types and [ArrayOfSomething];[] as links and if obj is array the returned value is [ArrayOfSomething]
std::string CParsePDF::get_link_for_type(PdsObject* obj, const std::string &types, const std::string &links) {
  int index = get_type_index(obj, types);
  if (index == -1)
    return "[]";
  std::vector<std::string> lnk = split(links, ';');
  return lnk[index];
}

void CParsePDF::parse_name_tree(PdsDictionary* obj, std::string links, std::string context) {
  // todo check if Kids doesn't exist together with names etc..
  PdsObject *kids_obj = obj->Get(L"Kids");
  PdsObject *names_obj = obj->Get(L"Names");
  PdsObject *limits_obj = obj->Get(L"Limits");
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
          std::string as = ToUtf8(str);
          std::string direct_link = select_one(item, links, as);
          parse_object(item, direct_link, context+ "->" + as);
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
        if (item != nullptr)
          parse_name_tree(item, links, context);
        else {
          //error kid isn't dictionary 
        }
      }
    }
    else {
      //error kids isn't array
    }
  }
}

void CParsePDF::parse_number_tree(PdsDictionary* obj, std::string links, std::string context) {
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
          std::string as = std::to_string(key);
          std::string direct_link = select_one(item, links, as);
          parse_object(item, direct_link, context + "->" +  as);
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
        if (item != nullptr)
          parse_number_tree(item, links, context);
        else {
          //error kid isn't dictionary 
        }
      }
    }
    else {
      //error kids isn't array
    }
  }
}

// one line could have two types "array;dictionary" 
// function returns index into the "array" based on obj's type
// returns -1 if type doesn't exist in string
int CParsePDF::get_type_index(PdsObject *obj, std::string types) {
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

void CParsePDF::parse_object(PdsObject *object, std::string link, std::string context)
{
  if (link == "")
    return;

  auto found = mapped.find(object);
  if (found != mapped.end()) {
    //    output << context << " already Processed" <<std::endl;
    found->second++;
    return;
  }

  output << context << std::endl;
  context = "  " + context;

  mapped.insert(std::make_pair(object, 1));

  std::string grammar_file = grammar_folder + link + ".tsv";

  CGrammarReader reader(grammar_file);
  if (!reader.load()) {
    output << "Error: Can't load grammar file:" << grammar_file << std::endl;
    return;
  }
  const std::vector<std::vector<std::string>> &data_list = reader.get_data();
  if (data_list.empty()) {
    output << "Error: Empty grammar file:" << grammar_file << std::endl;
    return;
  }

  // validating as dictionary:
  // going through all objects in dictionary 
  // checking types from the grammar
  // checking pooutputible values
  // checking if indirect if required
  // if we change context based on value we do it
  // then check presence of required keys
  // then recursively calling validation for each container with link to other grammar file
  if (object->GetObjectType() == kPdsDictionary || object->GetObjectType() == kPdsStream) {
    PdsDictionary* dictObj = (PdsDictionary*)object;
    //validate values first, then Process containers
    for (int i = 0; i < (dictObj->GetNumKeys()); i++) {
      std::wstring key;
      key.resize(dictObj->GetKey(i, nullptr, 0));
      dictObj->GetKey(i, (wchar_t*)key.c_str(), (int)key.size());

      //check Type, pooutputible values, indirect
      // todo: ked mame key ktory neni v gramatike
      PdsObject *inner_obj = dictObj->Get(key.c_str());
      bool found = false;
      for (auto& vec : data_list)
        if (vec[0] == ToUtf8(key)) {
          // is indirect when needed ?
          if ((vec[5] == "TRUE") && (inner_obj->GetId() == 0)) {
            output << "Error: not indirect:";
            output << vec[0] << "(" << grammar_file << ")" << std::endl;
          }

          // check type
          int index = get_type_index(inner_obj, vec[1]);
          if (index == -1) {
            output << "Error:  wrong type:";
            output << vec[0] << "(" << grammar_file << ")";
            output << " should be:" << vec[1] << " and is " << inner_obj->GetObjectType() << std::endl;
          }

          // possible value, could be one of many 
          // could be a pattern array;name --- [];[name1,name2]
          // could be single reference -- name1;name2
          if (vec[8] != "") {
            std::wstring str_value;
            str_value.resize(dictObj->GetText(key.c_str(), nullptr, 0));
            dictObj->GetText(key.c_str(), (wchar_t*)str_value.c_str(), (int)str_value.size());
            std::vector<std::string> options = split(vec[8], ';');
            std::vector<std::string>::iterator it = std::find(options.begin(), options.end(), ToUtf8(str_value));
            if (it == options.end()) {
              output << "Error:  wrong value:";
              output << vec[0] << "(" << grammar_file << ")";
              output << " should be:" << vec[8] << " and is " << ToUtf8(str_value) << std::endl;
            }
          }
          found = true;
          break;
        }
      // we didn't find key, there may be * we can use to validate
      if (!found)
        for (auto& vec : data_list)
          if (vec[0] == "*" && vec[10] != "") {
            std::string lnk = get_link_for_type(inner_obj, vec[1], vec[10]);
            std::string as = ToUtf8(key);
            std::string direct_link = select_one(inner_obj, lnk, as);
            parse_object(inner_obj, direct_link, context + "->" + as);
            break;
          }
    }
    // check presence of required values
    for (auto& vec : data_list)
      if (vec[4] == "TRUE" && vec[0] != "*") {
        PdsObject *inner_obj = dictObj->Get(utf8ToUtf16(vec[0]).c_str());
        if (inner_obj == nullptr) {
          output << "Error:  required key doesn't exist:";
          output << vec[0] << "(" << grammar_file << ")" << std::endl;
        }
      }

    // now go through containers and Process them with new grammar_file
    //todo: ked sme ich nepretestovali skor (*) 
    for (auto& vec : data_list)
      if (vec.size() >= 11 && vec[10] != "") {
        PdsObject *inner_obj = dictObj->Get(utf8ToUtf16(vec[0]).c_str());
        if (inner_obj != nullptr) {
          int index = get_type_index(inner_obj, vec[1]);
          //already tested before
          if (index == -1)
            break;
          std::vector<std::string> opt = split(vec[1], ';');
          std::vector<std::string> links = split(vec[10], ';');
          if (links[index] == "[]")
            continue;

          if (opt[index] == "NUMBER TREE" && inner_obj->GetObjectType() == kPdsDictionary) {
            parse_number_tree((PdsDictionary*)inner_obj, links[index], context + "->" + vec[0]);
          }
          else
            if (opt[index] == "NAME TREE" && inner_obj->GetObjectType() == kPdsDictionary) {
              parse_name_tree((PdsDictionary*)inner_obj, links[index], context + "->" + vec[0]);
            }
            else if (inner_obj->GetObjectType() == kPdsStream) {
              std::string as = vec[0];
              std::string direct_link = select_one(((PdsStream*)inner_obj)->GetStreamDict(), links[index], as);
              parse_object(((PdsStream*)inner_obj)->GetStreamDict(), direct_link, context + "->" + as);
            }
            else
              if ((inner_obj->GetObjectType() == kPdsDictionary || inner_obj->GetObjectType() == kPdsArray)) {
                std::string as = vec[0];
                std::string direct_link = select_one(inner_obj, links[index], as);
                parse_object(inner_obj, direct_link, context + "->" + as);
              }
        }
      }
    return;
  }

  if (object->GetObjectType() == kPdsArray) {
    bool to_ret = true;
    PdsArray* arrayObj = (PdsArray*)object;
    for (int i = 0; i < arrayObj->GetNumObjects(); ++i) {
      PdsObject* item = arrayObj->Get(i);
      for (auto& vec : data_list)
        if (vec[0] == "*") {
          std::string lnk = get_link_for_type(item, vec[1], vec[10]);
          std::string as = "[" + std::to_string(i) + "]";
          std::string direct_link = select_one(item, lnk, as);
          parse_object(item, direct_link, context + as);
          break;
        }
    }
    return;
  }

  output << "Error: can't process:";
  output << "(" << grammar_file << ")" << std::endl;
  return;
}
