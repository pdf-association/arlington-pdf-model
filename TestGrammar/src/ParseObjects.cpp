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
#include <math.h>

#include "GrammarFile.h"
#include "ParseObjects.h"

#include "Pdfix.h"
#include "utils.h"

using namespace PDFixSDK;


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

//
// @param object                valid PDFix object
// @param possible_value_str    string of possible values from TSV data. Cannot be NULL.
// @param index                 >= 0
// @param real_str_value        
//
// @returns true iff PDF object has the correct type and a valid value.
bool CParsePDF::check_possible_values(PdsObject* object, const std::string& possible_value_str, int index, std::wstring &real_str_value) {
  double num_value;

  if (object->GetObjectType() == kPdsBoolean) {
    if (((PdsBoolean*)object)->GetValue())
      real_str_value = L"TRUE";
    else real_str_value = L"FALSE";
  } 
  else if (object->GetObjectType() == kPdsNumber) {
    num_value = ((PdsNumber*)object)->GetValue();
    real_str_value = std::to_wstring(num_value);
  } 
  else if(object->GetObjectType() == kPdsName) {
    real_str_value.resize(((PdsName*)object)->GetText(nullptr, 0));
    ((PdsName*)object)->GetText((wchar_t*)real_str_value.c_str(), (int)real_str_value.size());
  }
  else if (object->GetObjectType() == kPdsString) {
    real_str_value.resize(((PdsString*)object)->GetText(nullptr, 0));
    ((PdsString*)object)->GetText((wchar_t*)real_str_value.c_str(), (int)real_str_value.size());
  }

  std::vector<std::string> options;
  std::string possible_vals = possible_value_str;
  if (possible_vals[0] == '[') {
    std::vector<std::string> all_defaults = split(possible_value_str, ';');
    possible_vals = all_defaults[index];
    possible_vals = possible_vals.substr(1, possible_vals.size() - 2);
  }

  //bool is_value    = (possible_vals.find("value") != std::string::npos) || (possible_vals.find("Value") != std::string::npos);
  //bool is_interval = (def.find("<") != std::string::npos) /*&& def.find("<") != std::string::npos*/;
  if (possible_vals != "" /*&& !is_value *//*&& !is_interval*/) {
    options = split(possible_vals, ',');
    bool found = false;
    int options_tested = 0;
    for (auto opt : options) {
      std::string function;
      opt = extract_function(opt, function);

      if (opt == "") 
        continue;

      options_tested++;
      if (object->GetObjectType() == kPdsNumber ) {
        try {
          auto double_val = std::stod(opt);
          // Double-precision comparison often fails because parsed PDF value is not precisely stored
          // Old PDF specs used to recommend 5 digits so go +/- half of that
          if (fabs(num_value - double_val) <= 0.000005) {
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
    }
    if (!found && (options_tested > 0))
      return false;
  }
  return true;
}


// choose one from provided links to validate further
// we select a link with all required values and with matching possible values 
// sometimes required values are missing, are inherited etc. 
// we use scoring mechanism
// +1 = if required key is missing
// +1 = if required key is different type
// +1 = if required key value doesn't correspond with possible value
// +5 = if possible value doesn't match and key is Type or Subtype
// grammar file with the lowest score is our selected link
std::string CParsePDF::select_one(PdsObject* obj, const std::string &links_string, std::string &obj_name) {
  if (links_string == "[]" || links_string == "")
    return "";

  std::vector<std::string> links = split(links_string.substr(1, links_string.size() - 2), ',');
  if (links.size() == 1)
    return links[0];

  int to_ret = -1;
  int min_score = 1000;
   // checking all links to see which one is suitable for provided object 
  for (auto i = 0; i < (int)links.size(); i++) {
    std::string function;
    auto lnk = extract_function(links[i], function);
    const std::vector<std::vector<std::string>>* data_list = get_grammar(lnk);

    auto j = 0;
    auto link_score = 0;
    if (obj->GetObjectType() == kPdsDictionary || obj->GetObjectType() == kPdsStream || obj->GetObjectType() == kPdsArray) {
      // are all "required" fields has to be present
      // and if required value is defined then has to match with possible value
      for (auto& vec : *data_list) {
        j++;
        // only checking required keys
        if (vec[TSV_REQUIRED] != "TRUE")
          continue;

        PdsObject* inner_object = nullptr;
        //required value exists?
        if (obj->GetObjectType() == kPdsArray) {
          if (j-1 < ((PdsArray*)obj)->GetNumObjects())
            inner_object = ((PdsArray*)obj)->Get(j - 1);
        }
        else {
          PdsDictionary* dictObj = (PdsDictionary*)obj;
          if (obj->GetObjectType() == kPdsStream)
            dictObj = ((PdsStream*)obj)->GetStreamDict();
          if (dictObj->Known(utf8ToUtf16(vec[TSV_KEYNAME]).c_str()))
            inner_object = dictObj->Get(utf8ToUtf16(vec[TSV_KEYNAME]).c_str());
        }

        //have required object, let's check possible values and compute score
        if (inner_object != nullptr) {
          int index = get_type_index(inner_object, vec[TSV_TYPE]);
          if (index != -1 ) {
            std::wstring str_value;
            if (vec[TSV_POSSIBLEVALUES] != "" && !check_possible_values(inner_object, vec[TSV_POSSIBLEVALUES], index, str_value))
              if (vec[TSV_KEYNAME] == "Type" || vec[TSV_KEYNAME] == "Subtype")
                link_score += 5;
              else link_score++;
          } else link_score++;
        } else link_score++;
      } // for each key in grammar file
    }

    // remembering the lowest score
    if (min_score > link_score) {
      to_ret = i;
      min_score = link_score;
    }
  }
  // if all required are there - return this position in list of links
  if (to_ret != -1) {
    std::string function;
    auto lnk = extract_function(links[to_ret], function);
    obj_name += " (as " + lnk + ")";
    return links[to_ret];
  }

  output << "Error: Can't select any link from " << links_string <<" to validate provided object: " << obj_name; 
  if (obj->GetId() != 0)
    output << " for object " << obj->GetId();
  output << std::endl;
  return "";
}

// returns specific link for provided object (decision is made by type)
// for: array;number as types and [ArrayOfSomething];[] as links and if obj is array the returned value is [ArrayOfSomething]
std::string CParsePDF::get_link_for_type(PdsObject* obj, const std::string &types, const std::string &links) {
  int index = get_type_index(obj, types);
  if (index == -1)
    return "[]";
  std::vector<std::string> lnk = split(links, ';');
  if (index >= lnk.size())  // for ArrayOfDifferences: types is "INTEGER;NAME", links is "" and we get buffer overflow in lnk!
    return "";
  return lnk[index];
}

// one line could have two types "array;dictionary" 
// function returns index into the "array" based on obj's type
// returns -1 if type doesn't exist in string
int CParsePDF::get_type_index(PdsObject *obj, std::string types) {
  std::vector<std::string> opt = split(types, ';');
  for (auto i = 0; i < (int)opt.size(); i++) {
    std::string function;
    opt[i] = extract_function(opt[i], function);
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
  auto ToString = [&](PdsObject* obj) {
    switch (obj->GetObjectType()) {
    case kPdsBoolean:   return "Boolean";
    case kPdsNumber:    return "number";
    case kPdsName:      return "name";
    case kPdsNull:      return "null";
    case kPdsStream:    return "stream";
    case kPdsString:    return "string";
    case kPdsArray:     return "array";
    case kPdsDictionary:return "dictionary";
    case kPdsReference: return "indirect-ref";
    case kPdsUnknown:
    default:            return "!unknown!";
    }
  };

  // Treat null object as though the key is not present (i.e. don't report an error)
  if ((vec[TSV_INDIRECTREF] == "TRUE") && (object->GetId() == 0) && (object->GetObjectType() != kPdsNull)) {
    output << "Error: not indirect: ";
    output << vec[TSV_KEYNAME] << " (" << grammar_file << ")" << std::endl;
  }

  // check type. "null" is always valid and same as not present so ignore.
  int index = get_type_index(object, vec[TSV_TYPE]);
  if ((object->GetObjectType() != kPdsNull) && (index == -1) /*&& vec[TSV_TYPE]!="ANY"*/) {
    int index2 = get_type_index(object, vec[TSV_TYPE]);
    output << "Error: wrong type: " << vec[TSV_KEYNAME] << " (" << grammar_file << ")";
    output << " should be: " << vec[TSV_TYPE] << " and is " << get_type_string(object);
    if (object->GetId() != 0) {
      output << " for object " << object->GetId();
    }
    output << std::endl;
  }

  // possible value, could be one of many 
  // could be a pattern array;name --- [];[name1,name2]
  // could be single reference -- name1,name2
  // we should cover also single reference in brackets [name1,name2]
  if (vec[TSV_POSSIBLEVALUES] != "" && index!=-1) {
    std::wstring str_value;
    if (!check_possible_values(object, vec[TSV_POSSIBLEVALUES], index, str_value)) {
      output << "Error: wrong value: " << vec[TSV_KEYNAME] << " (" << grammar_file << ")";
      output << " should be: " << vec[TSV_TYPE] << " " << vec[TSV_POSSIBLEVALUES] << " and is ";
      output << ToString(object) << " (" << ToUtf8(str_value) << ")";
      if (object->GetId() != 0) {
        output << " for object " << object->GetId();
      }
      output << std::endl;
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
    //std::string def = vec[TSV_POSSIBLEVALUES];
    //if (def[0] == '[') {
    //  std::vector<std::string> all_defaults = split(vec[TSV_POSSIBLEVALUES], ';');
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
    //    output << "Error: wrong value: " << vec[TSV_KEYNAME] << " (" << grammar_file << ")";
    //    output << " should be:" << vec[TSV_POSSIBLEVALUES] << " and is " << ToUtf8(str_value);
    //    if (object->GetId() != 0) {
    //      output << " for object " << object->GetId();
    //    }
    //    output << std::endl;
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
          add_parse_object(item, direct_link, context+ "->" + as);
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
          add_parse_object(item, direct_link, context + "->" +  as);
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

void CParsePDF::add_parse_object(PdsObject* object, const std::string& link, std::string context) {
  to_process.emplace(object, link, context);
}

void CParsePDF::parse_object() 
{
  while (!to_process.empty()) {
    queue_elem elem = to_process.front();
    to_process.pop();
    if (elem.link == "")
      continue;

    // Need to clean up the elem.link due to declarative functions "fn:SinceVersion(x,y, ...)"
    std::string function;
    elem.link = extract_function(elem.link, function);
    
    auto found = mapped.find(elem.object);
    if (found != mapped.end()) {
      //  output << elem.context << " already Processed" << std::endl;
      // "_Universal..." objects match anything so ignore them.

      // remove declarative functions to match clean elem.link
      found->second = extract_function(found->second, function);

      if ((found->second != elem.link) && 
          (((elem.link != "_UniversalDictionary") && (elem.link != "_UniversalArray")) &&
           ((found->second != "_UniversalDictionary") && (found->second != "_UniversalArray")))) {
        output << "Error: object validated in two different contexts. First: " << found->second;
        output << "; second: " << elem.link << " in: " << elem.context << std::endl;
      }
      continue;
    }

    output << elem.context << std::endl;
    elem.context = "  " + elem.context;
    //remember visited object with a link used for validation
    mapped.insert(std::make_pair(elem.object, elem.link));

    std::string grammar_file = grammar_folder + elem.link + ".tsv";
    const std::vector<std::vector<std::string>>* data_list = get_grammar(elem.link);

    // validating as dictionary:
    // going through all objects in dictionary 
    // checking basics (Type, PossibleValue, indirect)
    // then check presence of required keys
    // then recursively calling validation for each container with link to other grammar file
    if (elem.object->GetObjectType() == kPdsDictionary || elem.object->GetObjectType() == kPdsStream) {
      PdsDictionary* dictObj = (PdsDictionary*)elem.object;
      //validate values first, then Process containers
      if (elem.object->GetObjectType() == kPdsStream)
        dictObj = ((PdsStream*)elem.object)->GetStreamDict();

      for (int i = 0; i < (dictObj->GetNumKeys()); i++) {
        std::wstring key;
        key.resize(dictObj->GetKey(i, nullptr, 0));
        dictObj->GetKey(i, (wchar_t*)key.c_str(), (int)key.size());

        // checking basis (type,possiblevalue, indirect)
        PdsObject* inner_obj = dictObj->Get(key.c_str());
        // std::cout << "Looking for " << elem.link << " /" << ToUtf8(key) << std::endl;
        // might have wrong/malformed object. Key exists but value not
        if (inner_obj != nullptr) {
          bool found = false;
          for (auto& vec : *data_list)
            if (vec[TSV_KEYNAME] == ToUtf8(key)) {
              check_basics(inner_obj, vec, grammar_file);
              found = true;
              break;
            }
          // std::cout << "Found? /" << ToUtf8(key) << ": " << (found ? "true" : "false") << std::endl;
          // we didn't find the key, there may be * we can use to validate
          if (!found)
            for (auto& vec : *data_list)
              if (vec[TSV_KEYNAME] == "*" && vec[TSV_LINK] != "") {
                std::string lnk = get_link_for_type(inner_obj, vec[TSV_TYPE], vec[TSV_LINK]);
                std::string as = ToUtf8(key);
                std::string direct_link = select_one(inner_obj, lnk, as);
                add_parse_object(inner_obj, direct_link, elem.context + "->" + as);
                break;
              }
        }
        else {
          // malformed file ?
          // std::cout << "NOT found " << ToUtf8(key) << std::endl;
        }
      }

      // check presence of required values
      for (auto& vec : *data_list)
        if (vec[TSV_REQUIRED] == "TRUE" && vec[TSV_KEYNAME] != "*") {
          PdsObject* inner_obj = dictObj->Get(utf8ToUtf16(vec[TSV_KEYNAME]).c_str());
          if (inner_obj == nullptr) {
            output << "Error: required key doesn't exist: " << vec[TSV_KEYNAME] << " (" << grammar_file << ")" << std::endl;
          }
        }

      auto id_r = dictObj->GetId();

      // now go through containers and Process them with new grammar_file
      for (auto& vec : *data_list)
        if (vec.size() >= TSV_NOTES && vec[TSV_LINK] != "") {
          //std::wstring wstr = utf8ToUtf16(vec[KEY_COLUMN]);
          //auto exists = dictObj->Known(wstr.c_str());
          PdsObject* inner_obj = dictObj->Get(utf8ToUtf16(vec[TSV_KEYNAME]).c_str());
          if (inner_obj != nullptr) {
            int index = get_type_index(inner_obj, vec[TSV_TYPE]);
            //error already reported before
            if (index == -1)
              break;
            std::vector<std::string> opt = split(vec[TSV_TYPE], ';');
            std::vector<std::string> links = split(vec[TSV_LINK], ';');
            if (links[index] == "[]")
              continue;

            std::string function;
            opt[index] = extract_function(opt[index], function);

            if (opt[index] == "NUMBER-TREE" && inner_obj->GetObjectType() == kPdsDictionary) {
              parse_number_tree((PdsDictionary*)inner_obj, links[index], elem.context + "->" + vec[TSV_KEYNAME]);
            }
            else
              if (opt[index] == "NAME-TREE" && inner_obj->GetObjectType() == kPdsDictionary) {
                parse_name_tree((PdsDictionary*)inner_obj, links[index], elem.context + "->" + vec[TSV_KEYNAME]);
              }
              else if (inner_obj->GetObjectType() == kPdsStream) {
                std::string as = vec[TSV_KEYNAME];
                std::string direct_link = select_one(((PdsStream*)inner_obj)->GetStreamDict(), links[index], as);
                add_parse_object(((PdsStream*)inner_obj)->GetStreamDict(), direct_link, elem.context + "->" + as);
              }
              else
                if ((inner_obj->GetObjectType() == kPdsDictionary || inner_obj->GetObjectType() == kPdsArray)) {
                  std::string as = vec[TSV_KEYNAME];
                  std::string direct_link = select_one(inner_obj, links[index], as);
                  add_parse_object(inner_obj, direct_link, elem.context + "->" + as);
                }
          }
        }
      continue;
    }

    if (elem.object->GetObjectType() == kPdsArray) {
      bool to_ret = true;
      PdsArray* arrayObj = (PdsArray*)elem.object;
      for (int i = 0; i < arrayObj->GetNumObjects(); ++i) {
        PdsObject* item = arrayObj->Get(i);
        if (item != nullptr) {
          for (auto& vec : *data_list) {
            if (vec[TSV_KEYNAME] == std::to_string(i) || vec[TSV_KEYNAME] == "*") {
              // checking basics of the element
              check_basics(item, vec, grammar_file);
              if (vec[TSV_LINK] != "") {
                std::string lnk = get_link_for_type(item, vec[TSV_TYPE], vec[TSV_LINK]);
                std::string as = "[" + std::to_string(i) + "]";
                std::string direct_link = select_one(item, lnk, as);
                //if element does have a link - process it
                add_parse_object(item, direct_link, elem.context + as);
              }
              break;
            }
          }
        }
      }
      continue;
    }
  }
}
