////////////////////////////////////////////////////////////////////////////////////////////////////
// ParseObjects.cpp
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

const std::vector<std::vector<std::string>>* CParsePDF::get_grammar(const std::string &link) {
  auto it = grammar_map.find(link);
  if (it == grammar_map.end()) {
    std::string grammar_file = grammar_folder + link + ".tsv";
    std::unique_ptr<CGrammarReader> reader(new CGrammarReader(grammar_file));
    reader->load();
    //if (data_list->empty())
    //  continue;
    const std::vector<std::vector<std::string>>* to_ret = &reader->get_data();
    grammar_map.insert(std::make_pair(link, std::move(reader)));
    return to_ret;
  }
  return &it->second->get_data();
}


bool CParsePDF::check_possible_values(PdsObject* object, const std::string& possible_value_str, int index, std::wstring &real_str_value) {
  double num_value;

  if (object->GetObjectType() == kPdsBoolean) {
    if (((PdsBoolean*)object)->GetValue())
      real_str_value = L"TRUE";
    else real_str_value = L"FALSE";
  }
  if (object->GetObjectType() == kPdsNumber) {
    num_value = ((PdsNumber*)object)->GetValue();
    real_str_value = std::to_wstring(num_value);
  }
  if (object->GetObjectType() == kPdsName) {
    real_str_value.resize(((PdsName*)object)->GetText(nullptr, 0));
    ((PdsName*)object)->GetText((wchar_t*)real_str_value.c_str(), (int)real_str_value.size());
  }
  if (object->GetObjectType() == kPdsString) {
    real_str_value.resize(((PdsString*)object)->GetText(nullptr, 0));
    ((PdsString*)object)->GetText((wchar_t*)real_str_value.c_str(), (int)real_str_value.size());
  }

  std::vector<std::string> options;
  std::string def = possible_value_str;
  if (def[0] == '[') {
    std::vector<std::string> all_defaults = split(possible_value_str, ';');
    def = all_defaults[index];
    def = def.substr(1, def.size() - 2);
  }
  bool is_value = (def.find("value") != std::string::npos) || (def.find("Value") != std::string::npos);
  bool is_interval = (def.find("<") != std::string::npos) /*&& def.find("<") != std::string::npos*/;
  if (def != "" && !is_value && !is_interval) {
    options = split(def, ',');
    bool found = false;
    for (auto opt : options)
      if (object->GetObjectType() == kPdsNumber) {
        try {
          auto double_val = std::stod(opt);
          if (num_value == double_val) {
            found = true;
            break;
          }
        }
        catch (...) {
          break;
        }
      }
      else
        if (opt == ToUtf8(real_str_value)) {
          found = true;
          break;
        }
    if (!found)
      return false;
  }
  return true;
}


// choose one from provided links to validate further
// the decision is made based on 
// - 
std::string CParsePDF::select_one(PdsObject* obj, const std::string &links_string, std::string &obj_name) {
  if (links_string == "[]" || links_string == "")
    return "";

  std::vector<std::string> links = split(links_string.substr(1, links_string.size() - 2), ',');
  if (links.size() == 1)
    return links[0];

  int to_ret = -1;
  for (auto i = 0; i < (int)links.size(); i++) {
    const auto lnk = links[i];
    const std::vector<std::vector<std::string>>* data_list = get_grammar(lnk);

    to_ret = i;
    // first need to go through "required" - that may fail the validation right away
    if (obj->GetObjectType() == kPdsDictionary || obj->GetObjectType() == kPdsStream || obj->GetObjectType() == kPdsArray) {
      // are all "required" fields has to be present
      // and if required value is defined then has to match with value
      for (auto j = 1; j < (int)data_list->size(); j++) {
        auto &vec = data_list->at(j);
        if (vec[4] == "TRUE") {
          PdsObject* inner_object = nullptr;

          //required value exists?
          if (obj->GetObjectType() == kPdsArray) {
            if (j-1 >= ((PdsArray*)obj)->GetNumObjects()) {
              to_ret = -1;
              break;
            }
            inner_object = ((PdsArray*)obj)->Get(j - 1);
          }
          else {
            PdsDictionary* dictObj = (PdsDictionary*)obj;
            if (obj->GetObjectType() == kPdsStream)
              dictObj = ((PdsStream*)obj)->GetStreamDict();

            if (!dictObj->Known(utf8ToUtf16(vec[0]).c_str())) {
              to_ret = -1;
              break;
            }
            inner_object = dictObj->Get(utf8ToUtf16(vec[0]).c_str());
          }

          //have required object, let's check possible values
          if (inner_object==nullptr)  {
            to_ret = -1;
            break;
          }
          int index = get_type_index(inner_object, vec[1]);
          if (index == -1 ) {
            to_ret = -1;
            break;
          }
          std::wstring str_value;
          if (vec[7] != "" && !check_possible_values(inner_object, vec[7], index, str_value)) {
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

// one line could have two types "array;dictionary" 
// function returns index into the "array" based on obj's type
// returns -1 if type doesn't exist in string
int CParsePDF::get_type_index(PdsObject *obj, std::string types) {
  std::vector<std::string> opt = split(types, ';');
  for (auto i = 0; i < (int)opt.size(); i++) {
    if ((obj->GetObjectType() == kPdsBoolean) && (opt[i] == "BOOLEAN"))
      return i;
    if ((obj->GetObjectType() == kPdsNumber) && ((opt[i] == "NUMBER") || (opt[i] == "INTEGER")))
      return i;
    if ((obj->GetObjectType() == kPdsName) && (opt[i] == "NAME"))
      return i;
    if ((obj->GetObjectType() == kPdsNull) && (opt[i] == "NULL"))
      return i;
    if ((obj->GetObjectType() == kPdsStream) && (opt[i] == "STREAM"))
      return i;
    if ((obj->GetObjectType() == kPdsString) && ((opt[i] == "STRING") || (opt[i] == "DATE") || (opt[i] == "STRING-BYTE") || (opt[i] == "STRING-TEXT") || (opt[i] == "STRING-ASCII")))
      return i;
    if ((obj->GetObjectType() == kPdsArray) && ((opt[i] == "ARRAY") || (opt[i] == "RECTANGLE")))
      return i;
    if ((obj->GetObjectType() == kPdsDictionary) && ((opt[i] == "DICTIONARY") || (opt[i] == "NUMBER-TREE") || (opt[i] == "NAME-TREE")))
      return i;
  }
  return -1;
}

std::string CParsePDF::get_type_string(PdsObject *obj) {
  if (obj==nullptr)
    return "UNKNOWN";
  
  if (obj->GetObjectType() == kPdsBoolean)
    return "BOOLEAN";
  if (obj->GetObjectType() == kPdsNumber)
    return "NUMBER";
  if (obj->GetObjectType() == kPdsName)
    return "NAME";
  if (obj->GetObjectType() == kPdsNull)
    return "NULL OBJECT";
  if (obj->GetObjectType() == kPdsStream)
    return "STREAM";
  if (obj->GetObjectType() == kPdsString)
    return "STRING";
  if (obj->GetObjectType() == kPdsArray)
    return "ARRAY";
  if (obj->GetObjectType() == kPdsDictionary)
    return "DICTIONARY";
  
  return "UNDEFINED";
}


// validating basic information on container (stream, arrray, dictionary)
// going through all objects and check
// - type
// - indirect
// - possible value
void CParsePDF::check_basics(PdsObject *object, const std::vector<std::string> &vec, const std::string &grammar_file) {
  // is indirect when needed ?

  if ((vec[5] == "TRUE") && (object->GetId() == 0)) {
    output << "Error: not indirect:";
    output << vec[0] << "(" << grammar_file << ")" << std::endl;
  }

  // check type
  int index = get_type_index(object, vec[1]);
  if (index == -1 /*&& vec[1]!="ANY"*/) {
    int index2 = get_type_index(object, vec[1]);
    output << "Error:  wrong type:";
    output << vec[0] << "(" << grammar_file << ")";
    output << " should be:" << vec[1] << " and is " << get_type_string(object)<< std::endl;
  }

  // possible value, could be one of many 
  // could be a pattern array;name --- [];[name1,name2]
  // could be single reference -- name1,name2
  // we should cober also sigle reference in brackets [name1,name2]
  if (vec[7] != "" && index!=-1) {
    std::wstring str_value;
    if (!check_possible_values(object,vec[7],index, str_value))
    {
      output << "Error:  wrong value:";
      output << vec[0] << "(" << grammar_file << ")";
      output << " should be:" << vec[7] << " and is " << ToUtf8(str_value) << std::endl;
    }

    //std::wstring str_value;
    //double num_value;

    //if (object->GetObjectType() == kPdsBoolean) {
    //  if (((PdsBoolean*)object)->GetValue())
    //    str_value = L"TRUE";
    //  else str_value = L"FALSE";
    //}
    //if (object->GetObjectType() == kPdsNumber) {
    //  num_value = ((PdsNumber*)object)->GetValue();
    //  //      str_value = std::to_wstring(((PdsNumber*)object)->GetValue());
    //}
    //if (object->GetObjectType() == kPdsName) {
    //  str_value.resize(((PdsName*)object)->GetText(nullptr, 0));
    //  ((PdsName*)object)->GetText((wchar_t*)str_value.c_str(), (int)str_value.size());
    //}
    //if (object->GetObjectType() == kPdsString) {
    //  str_value.resize(((PdsString*)object)->GetText(nullptr, 0));
    //  ((PdsString*)object)->GetText((wchar_t*)str_value.c_str(), (int)str_value.size());
    //}

    //std::vector<std::string> options;
    //std::string def = vec[7];
    //if (def[0] == '[') {
    //  std::vector<std::string> all_defaults = split(vec[7], ';');
    //  def = all_defaults[index];
    //  def = def.substr(1, def.size() - 2);
    //}
    //bool is_value = (def.find("value") != std::string::npos) || (def.find("Value") != std::string::npos);
    //bool is_interval = def.find("<") != std::string::npos;
    //if (def != "" && !is_value && !is_interval) {
    //  options = split(def, ',');
    //  bool found = false;
    //  for (auto opt : options)
    //    if (object->GetObjectType() == kPdsNumber) {
    //      try {
    //        auto double_val = std::stod(opt);
    //        if (num_value == double_val) {
    //          found = true;
    //          break;
    //        }
    //      }
    //      catch (...) {
    //        break;
    //      }
    //    }
    //    else
    //      if (opt == ToUtf8(str_value)) {
    //        found = true;
    //        break;
    //      }
    //  if (!found) {
    //    output << "Error:  wrong value:";
    //    output << vec[0] << "(" << grammar_file << ")";
    //    output << " should be:" << vec[7] << " and is " << ToUtf8(str_value) << std::endl;
    //  }
    //}
  }
}


void CParsePDF::parse_name_tree(PdsDictionary* obj, const std::string &links, std::string context) {
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

void CParsePDF::parse_number_tree(PdsDictionary* obj, const std::string &links, std::string context) {
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

void CParsePDF::parse_object(PdsObject *object, const std::string &link, std::string context)
{
  if (link == "")
    return;

  auto found = mapped.find(object);
  if (found != mapped.end()) {
    //    output << context << " already Processed" <<std::endl;
    //found->second ++;
    if (found->second != link) {
      output << "Error: object validated in two different context first:" << found->second;
      output << " second:" << link <<  " in:" << context << std::endl;
    }
    return;
  }

  output << context << std::endl;
  context = "  " + context;

  mapped.insert(std::make_pair(object, link));

  std::string grammar_file = grammar_folder + link + ".tsv";
  const std::vector<std::vector<std::string>>* data_list = get_grammar(link);

  // validating as dictionary:
  // going through all objects in dictionary 
  // checking basics (type,possiblevalue, indirect)
  // then check presence of required keys
  // then recursively calling validation for each container with link to other grammar file
  if (object->GetObjectType() == kPdsDictionary || object->GetObjectType() == kPdsStream) {
    PdsDictionary* dictObj = (PdsDictionary*)object;
    //validate values first, then Process containers
    if (object->GetObjectType() == kPdsStream)
      dictObj = ((PdsStream*)object)->GetStreamDict();

    for (int i = 0; i < (dictObj->GetNumKeys()); i++) {
      std::wstring key;
      key.resize(dictObj->GetKey(i, nullptr, 0));
      dictObj->GetKey(i, (wchar_t*)key.c_str(), (int)key.size());

      // checking basis (type,possiblevalue, indirect)
      PdsObject *inner_obj = dictObj->Get(key.c_str());
      // might have wrong/malformed object. Key exists but value not
      if (inner_obj != nullptr) {
        bool found = false;
        for (auto& vec : *data_list)
          if (vec[0] == ToUtf8(key)) {
            check_basics(inner_obj, vec, grammar_file);
            found = true;
            break;
          }
        // we didn't find the key, there may be * we can use to validate
        if (!found)
          for (auto& vec : *data_list)
            if (vec[0] == "*" && vec[9] != "") {
              std::string lnk = get_link_for_type(inner_obj, vec[1], vec[9]);
              std::string as = ToUtf8(key);
              std::string direct_link = select_one(inner_obj, lnk, as);
              parse_object(inner_obj, direct_link, context + "->" + as);
              break;
            }
      }
      else {
        // malformed file ?
      }
    }

    // check presence of required values
    for (auto& vec : *data_list)
      if (vec[4] == "TRUE" && vec[0] != "*") {
        PdsObject *inner_obj = dictObj->Get(utf8ToUtf16(vec[0]).c_str());
        if (inner_obj == nullptr) {
          output << "Error:  required key doesn't exist:";
          output << vec[0] << "(" << grammar_file << ")" << std::endl;
        }
      }

    // now go through containers and Process them with new grammar_file
    for (auto& vec : *data_list)
      if (vec.size() >= 10 && vec[9] != "") {
        PdsObject *inner_obj = dictObj->Get(utf8ToUtf16(vec[0]).c_str());
        if (inner_obj != nullptr) {
          int index = get_type_index(inner_obj, vec[1]);
          //error already reported before
          if (index == -1)
            break;
          std::vector<std::string> opt = split(vec[1], ';');
          std::vector<std::string> links = split(vec[9], ';');
          if (links[index] == "[]")
            continue;

          if (opt[index] == "NUMBER-TREE" && inner_obj->GetObjectType() == kPdsDictionary) {
            parse_number_tree((PdsDictionary*)inner_obj, links[index], context + "->" + vec[0]);
          }
          else
            if (opt[index] == "NAME-TREE" && inner_obj->GetObjectType() == kPdsDictionary) {
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
      if (item!=nullptr)
        for (auto& vec : *data_list)
          if (vec[0] == std::to_string(i) || vec[0] == "*") {
            // checking basics of the element
            check_basics(item, vec, grammar_file);
            std::string lnk = get_link_for_type(item, vec[1], vec[9]);
            std::string as = "[" + std::to_string(i) + "]";
            std::string direct_link = select_one(item, lnk, as);
            //if element does have a link - process it
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
