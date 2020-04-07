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

std::string get_full_csv_file(std::string &grammar_file, const std::string &csv_name) {
  std::string file_name = get_path_dir(grammar_file);
  file_name += "/";
  if (csv_name.front() == '['  && csv_name.back() == ']')
    file_name += csv_name.substr(1, csv_name.size() - 2);
  else
    file_name += csv_name;
  file_name += ".csv";
  return file_name;
};


// choose one from provided links to validate further
// the decision is made based on 
// - 
int SelectOne(PdsObject* obj, std::string &grammar_file, std::vector<std::string> &links) {
  int to_ret = -1;
  for (int i = 0; i < links.size(); i++) {
    auto lnk = links[i];
    CGrammarReader reader(get_full_csv_file(grammar_file, lnk));
    if (!reader.load())
      continue;

    const std::vector<std::vector<std::string>> &data_list = reader.get_data();
    if (data_list.empty())
      continue;

    int to_ret = i;
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

          if (vec[6]!="" && vec[6] != ToUtf8(str_value)) {
            to_ret = -1;
            if (lnk == "StructElem") {
              to_ret = -1;
            }
            break;
          }
        }
      }
    }

    // if all required are there - return this position in list of links
    if (to_ret != -1)
      return to_ret;
  }
  return to_ret;
}


void ProcessObject(PdsObject* obj,
  std::ostream& ss,
  std::map<PdsObject*, int>& mapped,
  std::string grammar_file,
  std::string context,
  std::string indent
);


void ProcessNameTree(PdsDictionary* obj,
  std::ostream& ss,
  std::map<PdsObject*, int>& mapped,
  std::string grammar_file,
  std::string links,
  std::string context,
  std::string indent
) {
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

          std::vector<std::string> direct_links = split(links.substr(1, links.size() - 2), ',');
          int position = 0;
          if (direct_links.size() > 1) {
            position = SelectOne(item, grammar_file, direct_links);
          }
          if (position == -1)
            ss << indent << "Cannot find proper link to validate " << ToUtf8(str) << std::endl;
          else {
            std::string as;
            if (direct_links.size() > 1)
              as = "(as " + direct_links[position] + ")";
            //todo: co ked sa dostanem do situacie ze uz som inner_objraz validoval ale pod inou linkou??
            ProcessObject(item, ss, mapped, get_full_csv_file(grammar_file, direct_links[position]), context + "->" + ToUtf8(str) + as, indent);
            //ProcessObject(item, ss, mapped, grammar_file, context + "->" + ToUtf8(str), indent);
          }
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
          ProcessNameTree(item, ss, mapped, grammar_file, links, context, indent);
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
}

void ProcessNumberTree(PdsDictionary* obj,
  std::ostream& ss,
  std::map<PdsObject*, int>& mapped,
  std::string grammar_file,
  std::string links,
  std::string context,
  std::string indent
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
          std::vector<std::string> direct_links = split(links.substr(1, links.size() - 2), ',');
          int position = 0;
          if (direct_links.size() > 1) {
            position = SelectOne(item, grammar_file, direct_links);
          }
          if (position == -1)
            ss << indent << "Cannot find proper link to validate " << std::to_string(key) << std::endl;
          else {
            std::string as;
            if (direct_links.size() > 1)
              as = "(as " + direct_links[position] + ")";
            //todo: co ked sa dostanem do situacie ze uz som inner_objraz validoval ale pod inou linkou??
            ProcessObject(item, ss, mapped, get_full_csv_file(grammar_file, direct_links[position]), context + "->" + std::to_string(key) + as, indent);
            //ProcessObject(item, ss, mapped, grammar_file, context + "->" + std::to_string(key), indent);
          }
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
          ProcessNumberTree(item, ss, mapped, grammar_file,links, context, indent);
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
void ProcessObject(PdsObject* obj,
  std::ostream& ss,
  std::map<PdsObject*, int>& mapped,
  std::string grammar_file,
  std::string context,
  std::string indent
) {

  //if (obj->GetObjectType() != kPdsDictionary &&
  //  obj->GetObjectType() != kPdsStream &&
  //  obj->GetObjectType() != kPdsArray)
  //  return;

  auto found = mapped.find(obj);
  if (found != mapped.end()) {
    //    ss << context << " already processed" <<std::endl;
    found->second++;
    return;
  }

  ss << indent << context << std::endl;
  indent += "  ";
  mapped.insert(std::make_pair(obj, 1));

  CGrammarReader reader(grammar_file);
  if (!reader.load()) {
    ss << indent << "Error: Can't load grammar file:" << grammar_file << std::endl;
    return;
  }
  const std::vector<std::vector<std::string>> &data_list = reader.get_data();
  if (data_list.empty()) {
    ss << indent << "Error: Empty grammar file:" << grammar_file << std::endl;
    return;
  }

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
            ss << indent << "Error: not indirect ";
            ss << context << "->" << vec[0];
            ss << "(" << grammar_file << ")" << std::endl;
            //ss << "<<" << obj->GetId() << " 0 obj>> (" << grammar_file << ") ";
            //ss << vec[0] << " not indirect " << std::endl;
          }

          // check type
          int index = get_type_index(inner_obj, vec[1]);
          if (index == -1) {
            ss << indent << "Error:  wrong type ";
            ss << context << "->" << vec[0];
            ss << "(" << grammar_file << ")";
            ss << " should be:" << vec[1] << " and is " << inner_obj->GetObjectType() << std::endl;
          }

          // possible value, could be one of many 
          // if we have more values and also more links, then we switch context (follow the provided link) 
          // based on the value
          if (vec[8] != "") {
            std::wstring str_value;
            str_value.resize(dictObj->GetText(key.c_str(), nullptr, 0));
            dictObj->GetText(key.c_str(), (wchar_t*)str_value.c_str(), (int)str_value.size());
            std::vector<std::string> options = split(vec[8], ';');
            std::vector<std::string>::iterator it = std::find(options.begin(), options.end(), ToUtf8(str_value));
            if (it == options.end()) {
              ss << indent << "Error:  wrong value ";
              ss << context << "->" << vec[0];
              ss << "(" << grammar_file << ")";
              ss << " should be:" << vec[8] << " and is " << ToUtf8(str_value) << std::endl;
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
          //todo: mozno treba osetrit aj typ pripadne moze mat viac liniek??
          if (vec[0] == "*" && vec[10] != "") {
            ProcessObject(inner_obj, ss, mapped, get_full_csv_file(grammar_file, vec[10]), context + "->" + ToUtf8(key), indent);
            break;
          }
    }
    // check presence of required values
    for (auto& vec : data_list)
      if (vec[4] == "TRUE") {
        PdsObject *inner_obj = dictObj->Get(utf8ToUtf16(vec[0]).c_str());
        if (inner_obj == nullptr) {
          ss << indent << "Error:  required key doesn't exist ";
          ss << context << "->" << vec[0];
          ss << "(" << grammar_file << ")" << std::endl;
        }
      }

    // now go through containers and process them with new grammar_file
    //todo: ked sme ich nepretestovali skor (*) 
    for (auto& vec : data_list)
      if (vec.size() >= 11 && vec[10] != "") {
        PdsObject *inner_obj = dictObj->Get(utf8ToUtf16(vec[0]).c_str());
        if (inner_obj != nullptr) {
          int index = get_type_index(inner_obj, vec[1]);
          if (index == -1) {
            ss << indent << "Cannot validate - wrong type:"<<vec[0]<< std::endl;
            break;
          }
          std::vector<std::string> opt = split(vec[1], ';');
          std::vector<std::string> links = split(vec[10], ';');
          if (links[index] == "[]") 
            continue;

          if (opt[index] == "NUMBER TREE" && inner_obj->GetObjectType() == kPdsDictionary) {
            ProcessNumberTree((PdsDictionary*)inner_obj, ss, mapped, grammar_file,links[index], context + "->" + vec[0], indent);
          }
          else
            if (opt[index] == "NAME TREE" && inner_obj->GetObjectType() == kPdsDictionary) {
              ProcessNameTree((PdsDictionary*)inner_obj, ss, mapped, grammar_file,links[index], context + "->" + vec[0], indent);
            }
            else if (inner_obj->GetObjectType() == kPdsStream) {
              ProcessObject(((PdsStream*)inner_obj)->GetStreamDict(), ss, mapped, get_full_csv_file(grammar_file,links[index]), context + "->" + vec[0], indent);
            }
            else
              if ((inner_obj->GetObjectType() == kPdsDictionary || inner_obj->GetObjectType() == kPdsArray)) {
                std::vector<std::string> direct_links = split(links[index].substr(1, links[index].size() - 2), ',');
                int position = 0;
                if (direct_links.size() > 1) {
                  position = SelectOne(inner_obj, grammar_file, direct_links);
                  //if (direct_links[position] == "StructElem") {
                  //  as = as + "XX";
                  //}
                }
                if (position == -1)
                  ss << indent << "Cannot find proper link to validate "<< vec[0] << std::endl;
                else {
                  std::string as;
                  if (direct_links.size() > 1)
                    as = "(as " + direct_links[position] + ")";
                    //todo: co ked sa dostanem do situacie ze uz som inner_objraz validoval ale pod inou linkou??
                  ProcessObject(inner_obj, ss, mapped, get_full_csv_file(grammar_file, direct_links[position]), context + "->" + vec[0] + as, indent);
                }
              }
        }
      }
    return;
  }

  if (obj->GetObjectType() == kPdsArray) {
    bool to_ret = true;
    PdsArray* arrayObj = (PdsArray*)obj;
    for (int i = 0; i < arrayObj->GetNumObjects(); ++i) {
      PdsObject* item = arrayObj->Get(i);
      for (auto& vec : data_list)

        //todo: mozno treba osetrit aj typ, moze byt viac liniek
        if (vec[0] == "*" && vec[10] != "" && vec[10] != "[]") {
          std::vector<std::string> direct_links = split(vec[10].substr(1, vec[10].size() - 2), ',');
         int position=0;
         if (direct_links.size()>1)
            position = SelectOne(item, grammar_file, direct_links);
          if (position == -1)
            ss << indent << "Error: Cannot find proper link to validate [" << std::to_string(i) << "] " << std::endl;
          else
            ProcessObject(item, ss, mapped, 
              get_full_csv_file(grammar_file, direct_links[position]), 
              context + "[" + std::to_string(i) + "](as " + direct_links[position] + ")", indent);
          break;
        }
    }
    return;
  }

  ss << indent << "Error: can't process ";
  ss << context;
  ss << "(" << grammar_file << ")" << std::endl;
  return;
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
  ProcessObject(root, ofs, mapped, grammar_file, "Catalog", "");
  ofs.close();

  doc->Close();
}
//! [ParseObjects_cpp]
