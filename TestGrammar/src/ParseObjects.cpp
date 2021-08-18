///////////////////////////////////////////////////////////////////////////////
// Copyright 2020-2021 PDF Association, Inc. https://www.pdfa.org
//
// This material is based upon work supported by the Defense Advanced
// Research Projects Agency (DARPA) under Contract No. HR001119C0079.
// Any opinions, findings and conclusions or recommendations expressed
// in this material are those of the author(s) and do not necessarily
// reflect the views of the Defense Advanced Research Projects Agency
// (DARPA). Approved for public release.
//
// SPDX-License-Identifier: Apache-2.0
// Contributors: Roman Toda, Frantisek Forgac, Normex. Peter Wyatt, PDF Association
// 
///////////////////////////////////////////////////////////////////////////////

/*!
  Reading the whole PDF starting from specific object and validating against grammar provided via tsv file
*/

#include <map>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <memory>
#include <algorithm> 
#include <codecvt>
#include <math.h>

#include "ArlingtonTSVGrammarFile.h"
#include "ParseObjects.h"
#include "ArlingtonPDFShim.h"
#include "utils.h"

using namespace ArlingtonPDFShim;
namespace fs = std::filesystem;

/// @brief 
/// @param link[in,out] 
/// @return 
const std::vector<std::vector<std::string>>* CParsePDF::get_grammar(const std::string &link) 
{
    auto it = grammar_map.find(link);
    if (it == grammar_map.end()) 
    {
        fs::path grammar_file = grammar_folder;
        grammar_file /= link + ".tsv";
        std::unique_ptr<CArlingtonTSVGrammarFile> reader(new CArlingtonTSVGrammarFile(grammar_file));
        reader->load();
        const std::vector<std::vector<std::string>>* to_ret = &reader->get_data();
        grammar_map.insert(std::make_pair(link, std::move(reader)));
        return to_ret;
    }
    return &it->second->get_data();
}

/// @brief
/// @param object                valid PDFix object
/// @param possible_value_str    string of possible values from TSV data. Cannot be NULL.
/// @param index                 >= 0
/// @param real_str_value        
///
/// @returns true iff PDF object has the correct type and a valid value.
bool CParsePDF::check_possible_values(ArlPDFObject* object, const std::string& possible_value_str, int index, std::wstring &real_str_value) {
    double num_value;

    switch (object->get_object_type())
    {
        case PDFObjectType::ArlPDFObjTypeBoolean:
            if (((ArlPDFBoolean*)object)->get_value())
                real_str_value = L"true";
            else 
                real_str_value = L"false";
            break;
        
        case PDFObjectType::ArlPDFObjTypeNumber: 
            {
                ArlPDFNumber *numobj = (ArlPDFNumber*)object;
                if (numobj->is_integer_value()) {
                    int ivalue = numobj->get_integer_value();
                    real_str_value = std::to_wstring(ivalue);
                } 
                else {
                    num_value = numobj->get_value();
                    real_str_value = std::to_wstring(num_value);
                }
            }
            break;
   
        case PDFObjectType::ArlPDFObjTypeName:
            real_str_value = ((ArlPDFName*)object)->get_value();
            break;
  
        case PDFObjectType::ArlPDFObjTypeString:
            real_str_value = ((ArlPDFString*)object)->get_value();
            break;

        default: 
            break;
    } // switch

    std::vector<std::string>    options;
    std::string                 possible_vals = possible_value_str;
    if (possible_vals[0] == '[') {
        std::vector<std::string>    all_defaults = split(possible_value_str, ';');
        possible_vals = all_defaults[index];
        possible_vals = possible_vals.substr(1, possible_vals.size() - 2);
    }

    if (possible_vals != "") {
        options = split(possible_vals, ',');
        bool found = false;
        int options_tested = 0;
        for (auto opt : options) {
            std::string function;
            opt = extract_function(opt, function);

            if (opt == "") 
                continue;

            options_tested++;
            if ((object->get_object_type() == PDFObjectType::ArlPDFObjTypeNumber) && !((ArlPDFNumber*)object)->is_integer_value()) {
                try {
                    num_value = ((ArlPDFNumber*)object)->get_value();
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
            } else
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
std::string CParsePDF::select_one(ArlPDFObject* obj, const std::string &links_string, std::string &obj_name) {
    if (links_string == "[]" || links_string == "")
        return "";

    std::vector<std::string> links = split(links_string.substr(1, links_string.size() - 2), ',');
    if (links.size() == 1)
        return links[0];

    int to_ret = -1;
    int min_score = 1000;
    // checking all links to see which one is suitable for provided object 
    for (auto i = 0; i < (int)links.size(); i++) {
        std::string     function;
        auto            lnk = extract_function(links[i], function);
        const std::vector<std::vector<std::string>>* data_list = get_grammar(lnk);

        auto j = 0;
        auto link_score = 0;
        PDFObjectType obj_type = obj->get_object_type();
        if ((obj_type == PDFObjectType::ArlPDFObjTypeDictionary) ||
            (obj_type == PDFObjectType::ArlPDFObjTypeStream) ||
            (obj_type == PDFObjectType::ArlPDFObjTypeArray)) {
            // are all "required" fields has to be present
            // and if required value is defined then has to match with possible value
            for (auto& vec : *data_list) {
                j++;
                // only checking required keys
                if (vec[TSV_REQUIRED] != "TRUE")
                    continue;

                ArlPDFObject* inner_object = nullptr;
                // required value exists?
                switch (obj_type) {
                case PDFObjectType::ArlPDFObjTypeArray:
                    if (j-1 < ((ArlPDFArray*)obj)->get_num_elements())
                        inner_object = ((ArlPDFArray*)obj)->get_value(j - 1);
                    break;
                case PDFObjectType::ArlPDFObjTypeDictionary:
                    {
                        ArlPDFDictionary* dictObj = (ArlPDFDictionary*)obj;
                        if (dictObj->has_key(utf8ToUtf16(vec[TSV_KEYNAME])))
                            inner_object = dictObj->get_value(utf8ToUtf16(vec[TSV_KEYNAME]));
                    }
                    break;
                case PDFObjectType::ArlPDFObjTypeStream:
                    {
                        ArlPDFDictionary* stmDictObj = ((ArlPDFStream*)obj)->get_dictionary();
                        if (stmDictObj->has_key(utf8ToUtf16(vec[TSV_KEYNAME])))
                            inner_object = stmDictObj->get_value(utf8ToUtf16(vec[TSV_KEYNAME]));
                    }
                    break;
                }

                // have required object, let's check possible values and compute score
                if (inner_object != nullptr) {
                    int index = get_type_index(inner_object, vec[TSV_TYPE]);
                    if (index != -1 ) {
                        std::wstring str_value;
                        if (vec[TSV_POSSIBLEVALUES] != "" && !check_possible_values(inner_object, vec[TSV_POSSIBLEVALUES], index, str_value))
                            if (vec[TSV_KEYNAME] == "Type" || vec[TSV_KEYNAME] == "Subtype")
                                link_score += 5;
                            else 
                                link_score++;
                    } else 
                        link_score++;
                } else 
                    link_score++;
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
    if (obj->get_object_number() != 0)
        output << " for object #" << obj->get_object_number();
    output << std::endl;
    return "";
}

// returns specific link for provided object (decision is made by type)
// for: array;number as types and [ArrayOfSomething];[] as links and if obj is array the returned value is [ArrayOfSomething]
std::string CParsePDF::get_link_for_type(ArlPDFObject* obj, const std::string &types, const std::string &links) {
    int  index = get_type_index(obj, types);
    if (index == -1)
        return "[]";
    std::vector<std::string> lnk = split(links, ';');
    if (index >= (int)lnk.size())  // for ArrayOfDifferences: types is "INTEGER;NAME", links is "" and we get buffer overflow in lnk!
        return "";
    return lnk[index];
}


/// @brief  Searches the Arlington TSV "Type" column to find a match for a PDF object.
///         Each "Type" is an alphabetically-sorted, semi-colon separated list, possibly with predicates. 
///         Predicates are NOT evaluated!
/// @param obj[in]    the PDF object
/// @param types[in]  alphabetically-sorted, semi-colon separated string of known Arlington types 
/// @return  array index into the Arlington TSV data or -1 if no match
int CParsePDF::get_type_index(ArlPDFObject *obj, std::string types) {
    types = types + ";";
    std::vector<std::string> opt = split(types, ';');
    for (auto i = 0; i < (int)opt.size(); i++) {
        switch (obj->get_object_type()) {
        case PDFObjectType::ArlPDFObjTypeBoolean: 
            if (opt[i].find("boolean") != std::string::npos) 
                return i;
            break;
        case PDFObjectType::ArlPDFObjTypeNumber: 
            if ((opt[i].find("number") != std::string::npos) || 
                (opt[i].find("integer") != std::string::npos) || 
                (opt[i].find("bitmask") != std::string::npos))
                return i;
            break;
        case PDFObjectType::ArlPDFObjTypeName: 
            if (opt[i].find("name") != std::string::npos) 
                return i;
            break;
        case PDFObjectType::ArlPDFObjTypeNull: 
            if (opt[i].find("null") != std::string::npos)
                return i;
            break;
        case PDFObjectType::ArlPDFObjTypeStream: 
            if (opt[i].find("stream") != std::string::npos)
                return i;
            break;
        case PDFObjectType::ArlPDFObjTypeString: 
            if ((opt[i].find("string") != std::string::npos) || 
                (opt[i].find("date") != std::string::npos) || 
                (opt[i].find("string-byte") != std::string::npos) ||
                (opt[i].find("string-text") != std::string::npos) || 
                (opt[i].find("string-ascii") != std::string::npos))
                return i;
            break;
        case PDFObjectType::ArlPDFObjTypeArray: 
            if ((opt[i].find("array") != std::string::npos) || 
                (opt[i].find("rectangle") != std::string::npos) || 
                (opt[i].find("matrix") != std::string::npos))
                return i;
            break;
        case PDFObjectType::ArlPDFObjTypeDictionary: 
            if ((opt[i].find("dictionary") != std::string::npos) || 
                (opt[i].find("number-tree") != std::string::npos) || 
                (opt[i].find("name-tree") != std::string::npos))
                return i;
            break;
        default: break;
        } // switch
    } // for
    return -1;
}


/// @brief  Determines the Arlington type for a given PDF object.
///         Note that for some types there are ambiguities (e.g. an
///         integer could also be a bitmask).
/// @param obj[in] PDF object   
/// @return approximate Arlington type string 
std::string CParsePDF::get_type_string(ArlPDFObject *obj) {
    if (obj == nullptr)
        return "NULLPTR";

    switch (obj->get_object_type())
    { 
    case PDFObjectType::ArlPDFObjTypeNumber:
    {
        ArlPDFNumber *numobj = (ArlPDFNumber *)obj;
        if (numobj->is_integer_value())
            return "integer";                                           // or "bitmask"
        else
            return "number";
    }
    case PDFObjectType::ArlPDFObjTypeBoolean:     return "boolean";
    case PDFObjectType::ArlPDFObjTypeName:        return "name";
    case PDFObjectType::ArlPDFObjTypeNull:        return "null";
    case PDFObjectType::ArlPDFObjTypeStream:      return "stream";
    case PDFObjectType::ArlPDFObjTypeString:      return "string";      // or "date" or "string-*"
    case PDFObjectType::ArlPDFObjTypeArray:       return "array";       // or "rectangle" or "matrix"
    case PDFObjectType::ArlPDFObjTypeDictionary:  return "dictionary";  // or "name-tree" or "number-tree"
    case PDFObjectType::ArlPDFObjTypeReference:   return "INDIRECT-REF";
    default:                                      return "UNDEFINED";
  }
}


// validating basic information on container (stream, arrray, dictionary)
// going through all objects and check
// - type
// - indirect
// - possible value
void CParsePDF::check_basics(ArlPDFObject *object, const std::vector<std::string> &vec, const std::string &grammar_file) {

    auto obj_type = object->get_object_type();
    if (ArlingtonPDFShim::debugging) {
        std::cout << __FUNCTION__ << "(" << vec[TSV_KEYNAME] << ":" << vec[TSV_TYPE] << ", " << fs::path(grammar_file).stem() << ")" << std::endl;
    }    

    // Treat null object as though the key is not present (i.e. don't report an error)
    //todo: is that good condition?
    if ((vec[TSV_INDIRECTREF] == "TRUE") && 
        (!object->is_indirect_ref() && obj_type != PDFObjectType::ArlPDFObjTypeNull && obj_type != PDFObjectType::ArlPDFObjTypeReference)) {
        output << "Error: not an indirect reference as required: " << vec[TSV_KEYNAME] << " (" << fs::path(grammar_file).stem() << ")" << std::endl;
    }

    // check type. "null" is always valid and same as not present so ignore.
    int index = get_type_index(object, vec[TSV_TYPE]);
    if ((obj_type != PDFObjectType::ArlPDFObjTypeNull) && (index == -1) /*&& vec[TSV_TYPE]!="ANY"*/) {
        output << "Error: wrong type: " << vec[TSV_KEYNAME] << " (" << fs::path(grammar_file).stem() << ")";
        output << " should be: " << vec[TSV_TYPE] << " and is " << get_type_string(object);
        if (object->get_object_number() != 0) {
            output << " for object #" << object->get_object_number();
        }
        output << std::endl;
    }

    // possible value, could be one of many 
    // could be a pattern array;name --- [];[name1,name2]
    // could be single reference -- name1,name2
    // we should cover also single reference in brackets [name1,name2]
    if ((vec[TSV_POSSIBLEVALUES] != "") && (index != -1)) {
        std::wstring str_value;
        if (!check_possible_values(object, vec[TSV_POSSIBLEVALUES], index, str_value)) {
            output << "Error: wrong value: " << vec[TSV_KEYNAME] << " (" << fs::path(grammar_file).stem() << ")";
            output << " should be: " << vec[TSV_TYPE] << " (" << vec[TSV_POSSIBLEVALUES] << ") and is ";
            output << get_type_string(object) << " (" << ToUtf8(str_value) << ")";
            if (object->get_object_number() != 0) {
                output << " for object #" << object->get_object_number();
            }
            output << std::endl;
        }
    }
}


/// @brief  Parses a PDF name tree 
/// @param obj[in] 
/// @param links[in] 
/// @param context[in] 
void CParsePDF::parse_name_tree(ArlPDFDictionary* obj, const std::string &links, std::string context) {
    /// @todo check if Kids doesn't exist together with names etc.
    ArlPDFObject *kids_obj   = obj->get_value(L"Kids");
    ArlPDFObject *names_obj  = obj->get_value(L"Names");
    //ArlPDFObject *limits_obj = obj->get_value(L"Limits");
    if ((names_obj != nullptr) && (names_obj->get_object_type() == PDFObjectType::ArlPDFObjTypeArray)) {
        ArlPDFArray *array_obj = (ArlPDFArray*)names_obj;
        for (int i = 0; i < array_obj->get_num_elements(); i += 2) {
            // Pairs of entries: name (string), value. value has to be further validated
            ArlPDFObject* obj1 = array_obj->get_value(i);
            
            if ((obj1 != nullptr) && (obj1->get_object_type() == PDFObjectType::ArlPDFObjTypeString)) {
                ArlPDFObject* obj2 = array_obj->get_value(i + 1);

                if (obj2 != nullptr) {
                    std::wstring str = ((ArlPDFString*)obj1)->get_value();
                    std::string  as = ToUtf8(str);
                    std::string  direct_link = select_one(obj2, links, as);
                    add_parse_object(obj2, direct_link, context+ "->" + as);
                    continue;
                }
            }
            // Error: one of the pair of objects was not OK
        }
    }
    else {
        // Error: Names isn't an array in PDF name tree
    }

    if (kids_obj != nullptr) {
        if (kids_obj->get_object_type() == PDFObjectType::ArlPDFObjTypeArray) {
            ArlPDFArray* array_obj = (ArlPDFArray*)kids_obj;
            for (int i = 0; i < array_obj->get_num_elements(); i++) {
                ArlPDFObject* item = array_obj->get_value(i);
                if ((item != nullptr) && (item->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary))
                    parse_name_tree((ArlPDFDictionary*)item, links, context);
                else {
                    // Error: individual kid isn't dictionary in PDF name tree
                }
            }
        }
        else {
            // error: Kids isn't array in PDF name tree
        }
    }
}


/// @brief Parses a PDF number tree
/// @param obj[in] 
/// @param links[in] 
/// @param context[in] 
void CParsePDF::parse_number_tree(ArlPDFDictionary* obj, const std::string &links, std::string context) {
    /// @todo check if Kids doesn't exist together with names etc..
    ArlPDFObject *kids_obj   = obj->get_value(L"Kids");
    ArlPDFObject *nums_obj   = obj->get_value(L"Nums");
    //ArlPDFObject *limits_obj = obj->get_value(L"Limits");

    if (nums_obj != nullptr) {
        if (nums_obj->get_object_type() == PDFObjectType::ArlPDFObjTypeArray) {
            ArlPDFArray *array_obj = (ArlPDFArray*)nums_obj;
            for (int i = 0; i < array_obj->get_num_elements(); i += 2) {
                // Pairs of entries: number, value. value has to be validated
                ArlPDFObject* obj1 = array_obj->get_value(i);

                if ((obj1 != nullptr) && (obj1->get_object_type() == PDFObjectType::ArlPDFObjTypeNumber)) {
                    if (((ArlPDFNumber*)obj1)->is_integer_value()) {
                        ArlPDFObject* obj2 = array_obj->get_value(i + 1);

                        if (obj2 != nullptr) {
                            int val = ((ArlPDFNumber*)obj1)->get_integer_value();
                            std::string  as = std::to_string(val);
                            std::string  direct_link = select_one(obj2, links, as);
                            add_parse_object(obj2, direct_link, context + "->" + as);
                            continue;
                        }
                    }
                }
                // Error: one of the pair of objects was not OK in PDF number tree
            }
        }
    }
    else {
        // Error: Nums isn't an array in PDF number tree
    }

    if (kids_obj != nullptr) {
        if (kids_obj->get_object_type() == PDFObjectType::ArlPDFObjTypeArray) {
            ArlPDFArray* array_obj = (ArlPDFArray*)kids_obj;
            for (int i = 0; i < array_obj->get_num_elements(); i++) {
                ArlPDFObject* item = array_obj->get_value(i);
                if ((item != nullptr) && (item->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary))
                    parse_name_tree((ArlPDFDictionary*)item, links, context);
                else {
                    // Error: individual kid isn't dictionary in PDF number tree
                }
            }
        }
        else {
            // Error: Kids isn't array in PDF number tree
        }
    }
}


/// @brief 
/// @param object 
/// @param link 
/// @param context 
void CParsePDF::add_parse_object(ArlPDFObject* object, const std::string& link, std::string context) {
  to_process.emplace(object, link, context);
}



/// @brief 
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

        if (elem.object->is_indirect_ref()) {
          auto found = mapped.find(elem.object->get_hash_id());
          if (found != mapped.end()) {
            //  output << elem.context << " already Processed" << std::endl;
            // "_Universal..." objects match anything so ignore them.

            // remove predicates to match clean elem.link
            found->second = extract_function(found->second, function);

            if ((found->second != elem.link) &&
              (((elem.link != "_UniversalDictionary") && (elem.link != "_UniversalArray")) &&
                ((found->second != "_UniversalDictionary") && (found->second != "_UniversalArray"))))
            {
              output << "Error: object validated in two different contexts. First: " << found->second;
              output << "; second: " << elem.link << " in: " << strip_leading_whitespace(elem.context) << std::endl;
            }
            continue;
          }
          // remember visited object with a link used for validation
          mapped.insert(std::make_pair(elem.object->get_hash_id(), elem.link));
        }

        output << elem.context << std::endl;
        elem.context = "  " + elem.context;

        fs::path  grammar_file = grammar_folder;
        grammar_file /= elem.link + ".tsv";
        const std::vector<std::vector<std::string>>* data_list = get_grammar(elem.link);

        // Validating as dictionary:
        // - going through all objects in dictionary 
        // - checking basics (Type, PossibleValue, indirect)
        // - then check presence of required keys
        // - then recursively calling validation for each container with link to other grammar file
        PDFObjectType obj_type = elem.object->get_object_type();

        if (obj_type == PDFObjectType::ArlPDFObjTypeDictionary || obj_type == PDFObjectType::ArlPDFObjTypeStream) {
            ArlPDFDictionary* dictObj; //= (ArlPDFDictionary*)elem.object;
            
            //validate values first, then Process containers
            if (elem.object->get_object_type() == PDFObjectType::ArlPDFObjTypeStream)
                dictObj = ((ArlPDFStream*)elem.object)->get_dictionary();
            else 
                dictObj = (ArlPDFDictionary*)elem.object;

            for (int i = 0; i < (dictObj->get_num_keys()); i++) {
                std::wstring key = dictObj->get_key_name_by_index(i);

                // checking basis (type,possiblevalue, indirect)
                ArlPDFObject* inner_obj = dictObj->get_value(key);

                // might have wrong/malformed object. Key exists but value not
                if (inner_obj != nullptr) {
                    bool is_found = false;
                    for (auto& vec : *data_list)
                        if (vec[TSV_KEYNAME] == ToUtf8(key)) {
                            check_basics(inner_obj, vec, grammar_file.filename().string());
                            is_found = true;
                            break;
                        }
                        // std::cout << "Found? /" << ToUtf8(key) << ": " << (found ? "true" : "false") << std::endl;
                        // we didn't find the key, there may be * we can use to validate
                        if (!is_found)
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
            for (auto& vec : *data_list) {
                if (vec[TSV_REQUIRED] == "TRUE" && vec[TSV_KEYNAME] != "*") {
                    ArlPDFObject* inner_obj = dictObj->get_value(utf8ToUtf16(vec[TSV_KEYNAME]));
                    if (inner_obj == nullptr) {
                        if (vec[TSV_INHERITABLE] == "FALSE") {
                            output << "Error: non-inheritable required key doesn't exist: " << vec[TSV_KEYNAME] << " (" << fs::path(grammar_file).stem() << ")" << std::endl;
                        } else {
                            //@todo support inheritance
                            output << "Error: required key doesn't exist (inheritance not checked): " << vec[TSV_KEYNAME] << " (" << fs::path(grammar_file).stem() << ")" << std::endl;
                        }
                    }
                }
            } // for

            // now go through containers and Process them with new grammar_file
            for (auto& vec : *data_list) {
                if (vec.size() >= TSV_NOTES && vec[TSV_LINK] != "") {
                    //std::wstring wstr = utf8ToUtf16(vec[KEY_COLUMN]);
                    //auto exists = dictObj->Known(wstr.c_str());
                    ArlPDFObject* inner_obj = dictObj->get_value(utf8ToUtf16(vec[TSV_KEYNAME]));
                    if (inner_obj != nullptr) {
                        int index = get_type_index(inner_obj, vec[TSV_TYPE]);
                        //error already reported before
                        if (index == -1)
                            break;
                        std::vector<std::string> opt = split(vec[TSV_TYPE], ';');
                        std::vector<std::string> links = split(vec[TSV_LINK], ';');
                        if (links[index] == "[]")
                            continue;

                        std::string fn;
                        opt[index] = extract_function(opt[index], fn);

                        if ((opt[index] == "number-tree") && (inner_obj->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary)) {
                            parse_number_tree((ArlPDFDictionary*)inner_obj, links[index], elem.context + "->" + vec[TSV_KEYNAME]);
                        }
                        else if ((opt[index] == "name-tree") && (inner_obj->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary)) {
                                parse_name_tree((ArlPDFDictionary*)inner_obj, links[index], elem.context + "->" + vec[TSV_KEYNAME]);
                        }
                        else if ((inner_obj->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary) || 
                                 (inner_obj->get_object_type() == PDFObjectType::ArlPDFObjTypeStream) ||
                                 (inner_obj->get_object_type() == PDFObjectType::ArlPDFObjTypeArray)) {
                            std::string as = vec[TSV_KEYNAME];
                            std::string direct_link = select_one(inner_obj, links[index], as);
                            add_parse_object(inner_obj, direct_link, elem.context + "->" + as);
                        }
                    }
                }
                continue;
            } // for
        } else if (obj_type == PDFObjectType::ArlPDFObjTypeArray) {
            ArlPDFArray* arrayObj = (ArlPDFArray*)elem.object;
            for (int i = 0; i < arrayObj->get_num_elements(); ++i) {
                ArlPDFObject* item = arrayObj->get_value(i);
                if (item != nullptr) {
                    for (auto& vec : *data_list) {
                        if (vec[TSV_KEYNAME] == std::to_string(i) || vec[TSV_KEYNAME] == "*") {
                            // checking basics of the element
                            check_basics(item, vec, grammar_file.filename().string());
                            if (vec[TSV_LINK] != "") {
                                std::string lnk = get_link_for_type(item, vec[TSV_TYPE], vec[TSV_LINK]);
                                std::string as = "[" + std::to_string(i) + "]";
                                std::string direct_link = select_one(item, lnk, as);
                                //if element does have a link - process it
                                add_parse_object(item, direct_link, elem.context + as);
                            }
                            break;
                        }
                    } // for
                }
            } // for array elements
            continue;
        } // if obj_tyoe
    }
}
