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
  Reading the whole PDF starting from specific object and validating against grammar provided via Arlington TSV file set
*/

#include <map>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <memory>
#include <algorithm> 
#include <codecvt>
#include <math.h>
#include <assert.h>
#include <regex>
#include <algorithm>

#include "ArlingtonTSVGrammarFile.h"
#include "ParseObjects.h"
#include "ArlingtonPDFShim.h"
#include "utils.h"

using namespace ArlingtonPDFShim;
namespace fs = std::filesystem;

/// @brief Locates or reads in a single Arlington TSV grammar file. The input data is not altered or validated.
/// 
/// @param[in] link   the name of the Arlington TSV grammar file without ".tsv" extension
/// 
/// @returns          a row/column matrix (vector of vector) of raw strings directly from the TSV file
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


/// @brief Checks the Arlington PossibleValues field (column 9) for the provided PDF object.
/// 
/// @todo Arlington predicates in the PossibleValues are NOT currently supported
/// 
/// @param[in] object                a valid PDF object
/// @param[in] possible_value_str    string of possible values from Arlington TSV data. Cannot be NULL. 
/// @param[in] index                 >= 0. Index into PossibleValues if it is a complex type ([];[];[])
/// @param[out] real_str_value       the key value converted to a string representation
///
/// @returns true if and only if PDF object has the correct type and a valid value according to PossibleValues.
bool CParsePDF::check_possible_values(ArlPDFObject* object, const std::string& possible_value_str, int index, std::wstring &real_str_value) {
    double num_value;
    assert(object != nullptr);
    assert(index >= 0);

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

    /// possible_value_str might be something like: [a,fn:A(b),c];[];[d,fn:C(xx,fn:B(yyy,e)),f]

    if (possible_value_str == "") {
        return true; // no PossibleValues defined at all in Arlington so any value is OK   
    }    
    else {
        std::vector<std::string>    options = split(possible_value_str, ';');
        assert(index < options.size());

        std::string  possible_vals = options[index];
        if (possible_vals[0] == '[') {
            assert(possible_vals.size() >= 2);
            possible_vals = possible_vals.substr(1, possible_vals.size() - 2); // strip [ and ]
        }
    
        if (possible_vals == "") 
            return true; // no PossibleValues defined in Arlington for this type (based on index) so any value is OK 
        
        /// @todo Hack to remove certain predicates from the PossibleValues TSV data. Many predicates will remain!
        possible_vals = remove_type_predicates(possible_vals);
        if (possible_vals.find("fn:") != std::string::npos) {
            /// @todo Predicates remained in PossibleValues means we cannot make reliable decision
            return false;
        }

        // Safe to split on COMMAs as no predicates 
        options = split(possible_vals, ','); 
        assert(options.size() > 0);

        bool found = false;
        PDFObjectType obj_type = object->get_object_type();
        for (auto opt : options) {
            assert(opt != ""); 

            // Only floating point numbers need special processing
            if ((obj_type == PDFObjectType::ArlPDFObjTypeNumber) && !((ArlPDFNumber*)object)->is_integer_value()) {
                try {
                    num_value = ((ArlPDFNumber*)object)->get_value();
                    auto double_val = std::stod(opt);
                    // Double-precision comparison often fails because parsed PDF value is not precisely stored
                    // Old Adobe PDF specs used to recommend 5 digits so go +/- half of that
                    if (fabs(num_value - double_val) <= 0.000005) {
                        found = true;
                        break;
                    }
                }
                catch (...) {
                    // fallthrough to next opt in PossibleValues options list
                }
            }
            else if (opt == ToUtf8(real_str_value)) { // everything except floats can be simply string compared
                found = true;
                break;
            }
        } // for
        return found;
    }
    return false;
}


/// @brief Supports Arlington vec[TSV_REQUIRED] field which is either TRUE, FALSE or predicates.
///  e.g. fn:IsRequired(fn:IsPresent(Solidities)); fn:IsRequired(@SubFilter==adbe.pkcs7.s3 || @SubFilter==adbe.pkcs7.s4)
///
/// @returns true if the key is required. false if the key is not required or the predicate is too complex
bool CParsePDF::is_required_key(ArlPDFObject* obj, std::string reqd) {
    assert(obj != nullptr);

    if (reqd == "TRUE")
        return true;
    else if (reqd == "FALSE")
        return false;
    else {
        std::regex      r_isRequired("fn:IsRequired\\((.+)\\)");
        std::regex      r_isPresent("fn:IsPresent\\((.+)\\)");
        std::regex      r_notPresent("fn:NotPresent\\((.+)\\)");
        std::regex      r_keyValueEquals("@(.+)==(.+)");
        std::regex      r_keyValueNotEquals("@(.+)!=(.+)");
        std::string     inner;
        std::string     key;
        std::smatch     m;
        ArlPDFObject*   val;

        // Strip off outer predicate "fn:IsRequired( ... )"
        inner = std::regex_replace(reqd, r_isRequired, "$1");

        /// @todo don't currently support multi-term predicates or paths to other objects, so assume not required
        if ((inner.find("&&") != std::string::npos) || (inner.find("||") != std::string::npos) || (inner.find("::") != std::string::npos))
            return false;

        PDFObjectType obj_type = obj->get_object_type();
        if (std::regex_search(inner, r_isPresent)) {
            key = std::regex_replace(inner, r_isPresent, "$1");
            switch (obj_type) {
                case PDFObjectType::ArlPDFObjTypeDictionary:
                    val = ((ArlPDFDictionary*)obj)->get_value(ToWString(key));
                    return (val != nullptr);
                case PDFObjectType::ArlPDFObjTypeStream:
                    val = ((ArlPDFStream*)obj)->get_dictionary()->get_value(ToWString(key));
                    return (val != nullptr);
                default: assert(false && "unexpected object type for fn:IsRequired(fn:IsPresent(...))"); break;
            }
        }
        else if (std::regex_search(inner, r_notPresent)) {
            key = std::regex_replace(inner, r_notPresent, "$1");
            switch (obj_type) {
                case PDFObjectType::ArlPDFObjTypeDictionary:
                    val = ((ArlPDFDictionary*)obj)->get_value(ToWString(key));
                    return (val == nullptr);
                case PDFObjectType::ArlPDFObjTypeStream:
                    val = ((ArlPDFStream*)obj)->get_dictionary()->get_value(ToWString(key));
                    return (val == nullptr);
                default: assert(false && "unexpected object type for fn:IsRequired(fn:NotPresent(...))"); break;
            }
        }
        else if (std::regex_search(inner, m, r_keyValueEquals) && m.ready() && (m.size() == 3)) {
            key = m[1];
            switch (obj_type) {
                case PDFObjectType::ArlPDFObjTypeDictionary:
                    val = ((ArlPDFDictionary*)obj)->get_value(ToWString(key));
                    if (val != nullptr) {
                        /// @todo check if val->get_value() == m[2]
                    }
                    return false;
                case PDFObjectType::ArlPDFObjTypeStream:
                    val = ((ArlPDFStream*)obj)->get_dictionary()->get_value(ToWString(key));
                    if (val != nullptr) {
                        /// @todo check if val->get_value() == m[2]
                    }
                    return false;
                default: assert(false && "unexpected object type for fn:IsRequired(@key==...))"); break;
            }
        }
        else if (std::regex_search(inner, m, r_keyValueNotEquals) && m.ready() && (m.size() == 3)) {
            key = m[1];
            switch (obj_type) {
                case PDFObjectType::ArlPDFObjTypeDictionary:
                    val = ((ArlPDFDictionary*)obj)->get_value(ToWString(key));
                    if (val != nullptr) {
                        /// @todo check if val->get_value() != m[2]
                    }
                    return false;
                case PDFObjectType::ArlPDFObjTypeStream:
                    val = ((ArlPDFStream*)obj)->get_dictionary()->get_value(ToWString(key));
                    if (val != nullptr) {
                        /// @todo check if val->get_value() != m[2]
                    }
                    return false;
                default: assert(false && "unexpected object type for fn:IsRequired(@key!=...))"); break;
            }
        }
    }
    // fallthrough == too complex. Assume not required.
    return false;
}


///@brief  Choose a link for a PDF object from a provided set of Arlington links to validate further.
/// We select a link with all required values and with matching possible values. 
/// Sometimes required values are missing, are inherited etc. 
/// Scoring mechanism is used (lower score = better, like golf):
///  +4 = if required key is completely missing;
///  +2 = if required key is present, but has a different (incorrect) type;
///  +1 = if required key value doesn't match with a possible value 
///  -1 = if required key value doesn't match with a possible value due to a predicate
///  -2 = if required key value matches but is not /Type or /Subtype
/// -10 = if possible value matches and key is /Type or /Subtype.
/// Arlington grammar file with the lowest score is our selected link (like golf).
/// 
/// @param[in]  obj          the PDF object in question
/// @param[in]  links_string set of Arlington links to try with all predicates already REMOVED!!!
/// @param[in]  obj_name     the path of the PDF object in the PDF file
/// 
/// @returns a single Arlington link that is the best match for the given PDF object. Or "" if no link.
std::string CParsePDF::select_one(ArlPDFObject* obj, const std::string &links_string, std::string &obj_name) {
    assert(obj != nullptr);
    if (links_string == "[]" || links_string == "")
        return "";

    // Remove all predicates from Links before spliting by COMMA
    std::string s = remove_type_predicates(links_string);
    std::vector<std::string> links = split(s.substr(1, s.size() - 2), ',');
    assert(links.size() > 0);
    if (links.size() == 1)
        return links[0];

    int to_ret = -1;
    int min_score = 1000;
    // checking all links to see which one is suitable for provided object 
    for (auto i = 0; i < (int)links.size(); i++) {
        const std::vector<std::vector<std::string>>* data_list = get_grammar(links[i]);

        auto j = 0;
        auto link_score = 0;
        PDFObjectType obj_type = obj->get_object_type();
        if ((obj_type == PDFObjectType::ArlPDFObjTypeDictionary) ||
            (obj_type == PDFObjectType::ArlPDFObjTypeStream) ||
            (obj_type == PDFObjectType::ArlPDFObjTypeArray)) {
            // Are all "required" fields has to be present?
            // and if required value is defined then does it match with possible value?
            for (auto& vec : *data_list) {
                j++;
                if (is_required_key(obj, vec[TSV_REQUIRED])) {
                    ArlPDFObject* inner_object = nullptr;

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
                    } // switch

                    // have required object, let's check possible values and compute score
                    if (inner_object != nullptr) {
                        int index = get_type_index_for_object(inner_object, vec[TSV_TYPE]);
                        if (index != -1) {
                            if (vec[TSV_POSSIBLEVALUES] != "") {
                                std::wstring str_value;
                                if (check_possible_values(inner_object, vec[TSV_POSSIBLEVALUES], index, str_value)) {
                                    if (vec[TSV_KEYNAME] == "Type" || vec[TSV_KEYNAME] == "Subtype")
                                        link_score -= 10; // Type or Subtype key with a correct value!! 
                                    else 
                                        link_score -= 2; // another required key (not Type or Subtype) with a correct value!!
                                }
                                else if (str_value.find(L"fn:") != std::wstring::npos)
                                    link_score--; /// @todo required key present, but not a match, likely because of predicates
                                else
                                    link_score++; // required key present, but not a match (and no predicates)
                            }
                        } else 
                            link_score += 2; // required key exists but is wrong type
                    } else 
                        link_score += 4; // required key is missing
                } // if a required key
            } // for each key in grammar file
        } // if (dict || stream || array)

        // remembering the lowest score
        if (min_score > link_score) {
            to_ret = i;
            min_score = link_score;
        }
    }
    // lowest score wins (if we had any matches for required keys)
    if (to_ret != -1) {
        obj_name += " (as " + links[to_ret] + ")";
        return links[to_ret];
    }

    output << "Error: Can't select any link from " << links_string <<" to validate provided object: " << obj_name; 
    if (obj->get_object_number() != 0)
        output << " for object #" << obj->get_object_number();
    output << std::endl;
    return "";
}


///@brief Returns a specific set of links for a provided PDF object. Decision is made by matching the type of PDF object.
/// 
/// e.g. if "array;number" is Types and [ArrayOfSomething];[] as Links, then if obj is an array the returned value is "[ArrayOfSomething]"
/// 
/// @param[in] obj      PDF object
/// @param[in] types    Arlington "Types" field from TSV data e.g. "array;dictionary;number". Predicates must be REMOVED.
/// @param[in] links    Arlington "Links" field from TSV data e.g. "[SomeArray];[SomeDictA,SomeDictB];[]".  Predicates must be REMOVED.
/// 
/// @returns Arlington link (TSV file name) or ""
std::string CParsePDF::get_link_for_object(ArlPDFObject* obj, const std::string &types, const std::string &links) {
    assert(obj != nullptr);
    assert(types.size() > 0);
    assert(links.size() >= 0);

    int  index = get_type_index_for_object(obj, types);
    if (index == -1)
        return "[]"; // no match based on type of PDF object
    std::vector<std::string> lnk = split(links, ';');
    if (index >= (int)lnk.size())  // ArrayOfDifferences: types is "integer;name", links is "" and we get buffer overflow in lnk!
        return "[]";
    return lnk[index];
}


/// @brief  Searches the Arlington TSV "Type" column to find a match for a PDF object.
///         Each "Type" is an alphabetically-sorted, semi-colon separated list, possibly with predicates. 
///         Predicates are allowed in types.
/// 
/// @param[in] obj    PDF object
/// @param[in] types  alphabetically-sorted, semi-colon separated string of known Arlington types 
/// 
/// @returns  array index into the Arlington "Types" TSV data or -1 if no match
int CParsePDF::get_type_index_for_object(ArlPDFObject *obj, const std::string &types) {
    assert(obj != nullptr);
    assert(types.size() > 0);

    std::string t = remove_type_predicates(types);
    std::vector<std::string> opt = split(t, ';');
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
            default: assert(false); break;
        } // switch
    } // for
    return -1;
}


/// @brief  Determines the Arlington type for a given PDF object.
///         Note that for some types there are ambiguities (e.g. an integer could also be a bitmask).
/// 
/// @param obj[in] PDF object   
/// 
/// @return approximated Arlington type as a string 
std::string CParsePDF::get_type_string(ArlPDFObject *obj) {
    assert(obj != nullptr);

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
    case PDFObjectType::ArlPDFObjTypeReference:   assert(false && "ArlPDFObjTypeReference for CParsePDF::get_type_string()"); return "INDIRECT-REF";
    default:                                      assert(false && "unexpected type for CParsePDF::get_type_string()"); return "UNDEFINED";
  }
}


/// @brief Validate basic information on a PDF object (stream, arrray, dictionary) including:
///   - type
///   - indirect
///   - possible value
/// 
/// @param[in]   object  the PDF object to validate
/// @param[in]   vec     the Arlington PDF model definition read from a TSV
/// @param[in]   grammar_file  the name Arlington PDF model filename (object name) used for error messages
void CParsePDF::check_basics(ArlPDFObject *object, const std::vector<std::string> &vec, const fs::path &grammar_file) {
    assert(object != nullptr);
    auto obj_type = object->get_object_type();

    // Treat null object as though the key is not present (i.e. don't report an error)
    /// @todo: is that good condition?
    if ((vec[TSV_INDIRECTREF] == "TRUE") && 
        (!object->is_indirect_ref() && (obj_type != PDFObjectType::ArlPDFObjTypeNull) && (obj_type != PDFObjectType::ArlPDFObjTypeReference))) {
        output << "Error: not an indirect reference as required: " << vec[TSV_KEYNAME] << " (" << grammar_file.stem() << ")" << std::endl;
    }

    // check type. "null" is always valid and same as not present so ignore.
    int index = get_type_index_for_object(object, vec[TSV_TYPE]);
    if ((obj_type != PDFObjectType::ArlPDFObjTypeNull) && (index == -1) /*&& vec[TSV_TYPE]!="ANY"*/) {
        output << "Error: wrong type: " << vec[TSV_KEYNAME] << " (" << grammar_file.stem() << ")";
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
            output << "Error: wrong value: " << vec[TSV_KEYNAME] << " (" << grammar_file.stem() << ")";
            output << " should be: " << vec[TSV_TYPE] << " " << vec[TSV_POSSIBLEVALUES] << " and is ";
            output << get_type_string(object) << "==" << ToUtf8(str_value);
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
/// @param[in] object 
/// @param[in][in] link 
/// @param[in] context 
void CParsePDF::add_parse_object(ArlPDFObject* object, const std::string& link, std::string context) {
  to_process.emplace(object, link, context);
}



/// @brief Iteratively parse PDF objects
void CParsePDF::parse_object() 
{
    while (!to_process.empty()) {
        queue_elem elem = to_process.front();
        to_process.pop();
        if (elem.link == "")
            continue;

        // Need to clean up the elem.link due to declarative functions "fn:SinceVersion(x,y, ...)"
        elem.link = remove_link_predicates(elem.link);

        if (elem.object->is_indirect_ref()) {
          auto found = mapped.find(elem.object->get_hash_id());
          if (found != mapped.end()) {
            //  output << elem.context << " already Processed" << std::endl;
            // "_Universal..." objects match anything so ignore them.

            // remove predicates to match clean elem.link
            found->second = remove_link_predicates(found->second);

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

        if ((obj_type == PDFObjectType::ArlPDFObjTypeDictionary) || (obj_type == PDFObjectType::ArlPDFObjTypeStream)) {
            ArlPDFDictionary* dictObj; //= (ArlPDFDictionary*)elem.object;
            
            // validate values first, then process containers
            if (elem.object->get_object_type() == PDFObjectType::ArlPDFObjTypeStream)
                dictObj = ((ArlPDFStream*)elem.object)->get_dictionary();
            else 
                dictObj = (ArlPDFDictionary*)elem.object;

            for (int i = 0; i < (dictObj->get_num_keys()); i++) {
                std::wstring key = dictObj->get_key_name_by_index(i);

                // checking basis (type,possiblevalue, indirect)
                ArlPDFObject* inner_obj = dictObj->get_value(key);

                // might have wrong/malformed object. Key exists but value does not
                if (inner_obj != nullptr) {
                    bool is_found = false;
                    for (auto& vec : *data_list)
                        if (vec[TSV_KEYNAME] == ToUtf8(key)) {
                            check_basics(inner_obj, vec, grammar_file.filename());
                            is_found = true;
                            break;
                        }
                        // we didn't find the key, there may be * we can use to validate
                        if (!is_found)
                            for (auto& vec : *data_list)
                                if ((vec[TSV_KEYNAME] == "*") && (vec[TSV_LINK] != "")) {
                                    std::string lnk = get_link_for_object(inner_obj, vec[TSV_TYPE], vec[TSV_LINK]);
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
            } // for

            // check presence of required values in object first, then parents if inheritable
            for (auto& vec : *data_list) {
                if (is_required_key(elem.object, vec[TSV_REQUIRED]) && (vec[TSV_KEYNAME] != "*")) {
                    ArlPDFObject* inner_obj = dictObj->get_value(ToWString(vec[TSV_KEYNAME]));
                    if (inner_obj == nullptr) {
                        if (vec[TSV_INHERITABLE] == "FALSE") {
                            output << "Error: non-inheritable required key doesn't exist: " << vec[TSV_KEYNAME] << " (" << fs::path(grammar_file).stem() << ")" << std::endl;
                        } else {
                            ///@todo support inheritance
                            output << "Error: required key doesn't exist (inheritance not checked): " << vec[TSV_KEYNAME] << " (" << fs::path(grammar_file).stem() << ")" << std::endl;
                        }
                    }
                } 
            } // for

            // now go through containers and Process them with new grammar_file
            for (auto& vec : *data_list) {
                if (vec.size() >= TSV_NOTES && vec[TSV_LINK] != "") {
                    ArlPDFObject* inner_obj = dictObj->get_value(ToWString(vec[TSV_KEYNAME]));
                    if (inner_obj != nullptr) {
                        int index = get_type_index_for_object(inner_obj, vec[TSV_TYPE]);
                        //error already reported before
                        if (index == -1)
                            break;
                        std::vector<std::string> opt = split(vec[TSV_TYPE], ';');
                        std::vector<std::string> links = split(vec[TSV_LINK], ';');
                        if (links[index] == "[]")
                            continue;

                        opt[index] = remove_type_predicates(opt[index]);
                        links[index] = remove_link_predicates(links[index]);

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
                            check_basics(item, vec, grammar_file.filename());
                            if (vec[TSV_LINK] != "") {
                                std::string lnk = get_link_for_object(item, vec[TSV_TYPE], vec[TSV_LINK]);
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
