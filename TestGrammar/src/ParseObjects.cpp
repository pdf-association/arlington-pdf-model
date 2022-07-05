///////////////////////////////////////////////////////////////////////////////
/// @file 
/// @brief CParsePDF class definition
/// 
/// @copyright 
/// Copyright 2020-2022 PDF Association, Inc. https://www.pdfa.org
/// SPDX-License-Identifier: Apache-2.0
/// 
/// @remark
/// This material is based upon work supported by the Defense Advanced
/// Research Projects Agency (DARPA) under Contract No. HR001119C0079.
/// Any opinions, findings and conclusions or recommendations expressed
/// in this material are those of the author(s) and do not necessarily
/// reflect the views of the Defense Advanced Research Projects Agency
/// (DARPA). Approved for public release.
///
/// @author Roman Toda, Normex
/// @author Frantisek Forgac, Normex
/// @author Peter Wyatt, PDF Association
///
///////////////////////////////////////////////////////////////////////////////

#include "ParseObjects.h"
#include "ArlingtonTSVGrammarFile.h"
#include "ArlPredicates.h"
#include "ASTNode.h"
#include "PredicateProcessor.h"
#include "utils.h"
#include "PDFFile.h"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <memory>
#include <algorithm>
#include <codecvt>
#include <math.h>
#include <cassert>
#include <regex>
#include <algorithm>

using namespace ArlingtonPDFShim;
namespace fs = std::filesystem;


/// @brief Locates & reads in a single Arlington TSV grammar file. The input data is not altered or validated.
///
/// @param[in] link   the stub name of an Arlington TSV grammar file from the TSV data (i.e. without folder or ".tsv" extension)
///
/// @returns          a row/column matrix (vector of vector) of raw strings directly from the TSV file
const ArlTSVmatrix& CParsePDF::get_grammar(const std::string &link)
{
    auto it = grammar_map.find(link);
    if (it == grammar_map.end())
    {
        fs::path grammar_file = grammar_folder;
        grammar_file /= link + ".tsv";
        std::unique_ptr<CArlingtonTSVGrammarFile> reader(new CArlingtonTSVGrammarFile(grammar_file));
        reader->load();
        const ArlTSVmatrix& to_ret = reader->get_data();
        grammar_map.insert(std::make_pair(link, std::move(reader)));
        return to_ret;
    }
    return it->second->get_data();
}


/// @brief Checks the Arlington PossibleValues field (column 9) for the provided PDF object.
///
/// @todo Arlington predicates in the PossibleValues are NOT fully supported
///
/// @param[in] object                a valid PDF object
/// @param[in] key_idx               >= 0. Index into the TSV data for thsi key/array element.
/// @param[in] tsv_data              full Arlington TSV data for this PDF object.
/// @param[in] type_idx              >= 0. Index into Types field supporting complex types ([];[];[])
/// @param[out] real_str_value       the key value converted to a string representation
///
/// @returns true if and only if PDF object has the correct type and a valid value according to PossibleValues.
bool CParsePDF::check_possible_values(ArlPDFObject* object, int key_idx, const ArlTSVmatrix &tsv_data, const int type_idx, std::wstring &real_str_value) {
    assert(object != nullptr);
    assert(key_idx >= 0);
    assert(key_idx < (int)tsv_data.size());
    assert(type_idx >= 0);

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
            break; // Fallthrough
    } // switch

    // Need to cope with wildcard keys "*" in TSV data as key_idx might be beyond rows in tsv_data
    if (key_idx >= (int)tsv_data.size()) {
        if (tsv_data[0][TSV_KEYNAME] == "*")
            key_idx = 0;
        else if (tsv_data[tsv_data.size() - 1][TSV_KEYNAME] == "*")
            key_idx = (int)tsv_data.size() - 1;
        else {
            /// @todo Arlington has more complex repetition patterns which are not supported yet
            output << COLOR_WARNING << "Arlington complex repetition patterns are not supported yet." << COLOR_RESET;
            return false;
        }
    }

    if (tsv_data[key_idx][TSV_POSSIBLEVALUES] == "") {
        return true; // no PossibleValues defined at all in Arlington so any value is OK
    }

    /// Possible Value might be something complex with multiple predicates and empty options
    /// e.g.: [a,fn:A(b),c];[];[d,fn:C(xx,fn:B(yyy,e)),f]
    PossibleValuesPredicateProcessor pv = PossibleValuesPredicateProcessor(pdfc, tsv_data[key_idx][TSV_POSSIBLEVALUES]);

     bool fully_processed = true;
    bool b = pv.ReduceRow(object, key_idx, tsv_data, type_idx, &fully_processed);
    return b;
}


/// @brief Supports Arlington vec[TSV_REQUIRED] field which is either TRUE, FALSE or predicates. This method
///        calculates predicates
///  e.g. fn:IsRequired(fn:IsPresent(Solidities)); fn:IsRequired(\@SubFilter==adbe.pkcs7.s3 || \@SubFilter==adbe.pkcs7.s4)
///
/// @returns true if the key is required. false if the key is not required or the predicate is too complex
bool CParsePDF::is_required_key(ArlPDFObject* obj, const std::string &reqd) {
    assert(obj != nullptr);

    if (reqd == "TRUE")
        return true;
    else if (reqd == "FALSE")
        return false;
    else {
        const std::regex  r_isRequired("^fn:IsRequired\\((.+)\\)");     // $1 = inner
        const std::regex  r_isPresent("fn:IsPresent\\(([^\\)]+)\\)");   // $1 = inner
        const std::regex  r_notPresent("fn:NotPresent\\(([^\\)]+)\\)"); // $1 = inner
        const std::regex  r_keyValueEquals("@(.+)==(.+)");              // $1 = LHS, "==", $2 = RHS
        const std::regex  r_keyValueNotEquals("@(.+)!=(.+)");           // $1 = LHS, "!=", $2 = RHS
        std::string     inner;
        std::string     key;
        std::smatch     m;
        ArlPDFObject*   val;

        // Strip off outer predicate "^fn:IsRequired( ... )"
        inner = std::regex_replace(reqd, r_isRequired, "$1");

        /// @todo don't currently support multi-term predicates or paths to other objects, so assume not required
        if ((inner.find("&&") != std::string::npos) || (inner.find("||") != std::string::npos) ||
            (inner.find("::") != std::string::npos) || (inner.find("fn:") != std::string::npos))
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
                default: 
                    assert(false && "unexpected object type for fn:IsRequired(fn:IsPresent(...))"); 
                    break;
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
                default: 
                    assert(false && "unexpected object type for fn:IsRequired(fn:NotPresent(...))");
                    break;
            }
        }
        else if (std::regex_search(inner, m, r_keyValueEquals) && m.ready() && (m.size() == 3)) {
            key = m[1];
            switch (obj_type) {
                case PDFObjectType::ArlPDFObjTypeDictionary:
                    val = ((ArlPDFDictionary*)obj)->get_value(ToWString(key));
                    if (val != nullptr) {
                        /// @todo check if val->get_value() == m[2]
                        delete val;
                    }
                    return false;
                case PDFObjectType::ArlPDFObjTypeStream: 
                    {
                        ArlPDFDictionary* stm_dict = ((ArlPDFStream*)obj)->get_dictionary();
                        assert(stm_dict != nullptr);
                        val = stm_dict->get_value(ToWString(key));
                        delete stm_dict;
                        if (val != nullptr) {
                            /// @todo check if val->get_value() == m[2]
                            delete val;
                        }
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
                        delete val;
                    }
                    return false;
                case PDFObjectType::ArlPDFObjTypeStream: 
                    {
                        ArlPDFDictionary* stm_dict = ((ArlPDFStream*)obj)->get_dictionary();
                        assert(stm_dict != nullptr);
                        val = stm_dict->get_value(ToWString(key));
                        delete stm_dict;
                        if (val != nullptr) {
                            /// @todo check if val->get_value() != m[2]
                            delete val;
                        }
                        return false;
                    }
                default: 
                    assert(false && "unexpected object type for fn:IsRequired(@key!=...))"); 
                    break;
            }
        }
    }
    // fallthrough == too complex. Assume not required.
    return false;
}


/// @brief define SCORING_DEBUG to see scoring inside get_link_for_object()
#undef SCORING_DEBUG


///@brief  Choose a specific link for a PDF object from a provided set of Arlington links to validate further.
/// Select a link with all required values and with matching possible values.
/// Sometimes required values are missing, are inherited, etc.
/// Scoring mechanism is used (lower score = better, like golf):
///   +6 = required key is completely missing
///   +4 = required key is present, but has a different (incorrect) type (does happen: name vs string)
///   -1 = required key value doesn't match with a possible value due to a predicate (TODO - currently unsupported)
///   -2 = required key value but can be anything (so easier match)
///   -4 = required key (not /Type or /Subtype) and value matches precisely
///  -20 = required key is key is /Type or /Subtype AND possible value matches precisely
/// -3*N = all N keys in the object were required keys AND present
///
/// Arlington grammar file with the lowest score is our selected link (like golf).
///
/// @param[in]  obj          the PDF object in question
/// @param[in]  links_string set of Arlington links to try (predicates are SAFE)
/// @param[in]  obj_name     the path of the PDF object in the PDF file
///
/// @returns a single Arlington link that is the best match for the given PDF object. Or "" if no link.
std::string CParsePDF::get_link_for_object(ArlPDFObject* obj, const std::string &links_string, std::string &obj_name) {
    assert(obj != nullptr);

    if (links_string == "[]" || links_string == "")
        return "";

#if defined(SCORING_DEBUG)
    std::cout << std::endl << "Deciding for " << obj_name << " using " << links_string << std::endl;
#endif

    // Remove all predicates from Links before spliting by COMMA
    std::string s = remove_type_link_predicates(links_string);
    std::vector<std::string> links = split(s.substr(1, s.size() - 2), ',');
    assert(links.size() > 0);
    if (links.size() == 1) {
#if defined(SCORING_DEBUG)
        std::cout << "Only option is " << links[0] << std::endl << std::endl;
#endif
        return links[0];
    }

    int  to_ret = -1;
    int  min_score = 1000;

    // checking all links to see which one is suitable for provided object
    auto obj_type = obj->get_object_type();
    for (auto i = 0; i < (int)links.size(); i++) {
#if defined(SCORING_DEBUG)
        std::cout << "Scoring " << links[i] << std::endl;
#endif
        const ArlTSVmatrix& data_list = get_grammar(links[i]);

        auto j = 0;
        auto link_score = 0;
        if ((obj_type == PDFObjectType::ArlPDFObjTypeDictionary) ||
            (obj_type == PDFObjectType::ArlPDFObjTypeStream) ||
            (obj_type == PDFObjectType::ArlPDFObjTypeArray)) {
            bool all_required_keys = true;
            for (auto& vec : data_list) {
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
                                delete stmDictObj;
                            }
                            break;
                        default:
                            assert(false && "Unexpected object type!");
                            break;
                    } // switch

                    // have a Required inner object, check "Possible Values" and compute score
                    if (inner_object != nullptr) {
                        int index = get_type_index_for_object(inner_object, vec[TSV_TYPE]);
                        if (index != -1) {
                            if (vec[TSV_POSSIBLEVALUES] != "") {
                                std::wstring   str_value;  // inner_object value from PDF as string
                                if (check_possible_values(inner_object, j-1, data_list, index, str_value)) {
#if defined(SCORING_DEBUG)
                                    std::cout << "Required key " << vec[TSV_KEYNAME] << " is present and value '" << ToUtf8(str_value) << "' matched" << std::endl;
#endif
                                    if (vec[TSV_KEYNAME] == "Type" || vec[TSV_KEYNAME] == "Subtype")
                                        link_score += -20; // Type or Subtype key exists with a correct value
                                    else
                                        link_score += -4; // another required key (not Type or Subtype) exists with a correct value
                                }
                                else if (vec[TSV_POSSIBLEVALUES].find("fn:") != std::string::npos) {
#if defined(SCORING_DEBUG)
                                    std::cout << "Required key " << vec[TSV_KEYNAME] << " is present but predicates in " << vec[TSV_POSSIBLEVALUES] << std::endl;
#endif
                                    link_score += -1; /// @todo required key exists, but not a match, likely because of predicates
                                }
                                else {
#if defined(SCORING_DEBUG)
                                    std::cout << "Required key " << vec[TSV_KEYNAME] << " is present but incorrect value '" << ToUtf8(str_value) << "'" << std::endl;
#endif
                                    link_score += +4; // required key exists, but NOT a correct value (no predicates so explicitly wrong)
                                }
                            }
                            else {
#if defined(SCORING_DEBUG)
                                std::cout << "Required key " << vec[TSV_KEYNAME] << " is present and anything is OK" << std::endl;
#endif
                                // Required key exists, but no specified PossibleValues so anything is OK
                                link_score += -2;  // another required key (not Type or Subtype)
                            }
                        } else {
#if defined(SCORING_DEBUG)
                            std::cout << "Required key " << vec[TSV_KEYNAME] << " is present but wrong type!" << std::endl;
#endif
                            link_score += +4; // required key exists but is wrong type (explicitly incorrect)
                        }
                        delete inner_object;
                    } 
                    else {
#if defined(SCORING_DEBUG)
                        std::cout << "Required key " << vec[TSV_KEYNAME] << " is missing!" << std::endl;
#endif
                        link_score += +6; // required key is missing!
                        all_required_keys = false;
                    }
                } // if a required key
                else
                    all_required_keys = false;
            } // for each key in grammar file

            // All keys were required and present (common for arrays) - bonus x nbr of keys
            if (all_required_keys) {
#if defined(SCORING_DEBUG)
                std::cout << "All required keys were present!" << std::endl;
#endif
                link_score += -3 * (int)data_list.size();
            }
        } // if (dict || stream || array)

#if defined(SCORING_DEBUG)
        std::cout << links[i] << " scored " << link_score << ", min so far is " << min_score << std::endl;
#endif

        // remembering the lowest score
        if (min_score > link_score) {
            to_ret = i;
            min_score = link_score;
        }
    } // for

    // lowest score wins (if we had any matches for required keys)
    if (to_ret >= 0) {
        obj_name += " (as " + links[to_ret] + ")";
#if defined(SCORING_DEBUG)
        std::cout << "Winner was " << obj_name << " from " << links_string << std::endl << std::endl;
#endif
        return links[to_ret];
    }

    output << COLOR_ERROR << "can't select any link from \"" << links_string <<"\" to validate provided object " << obj_name;
    if (!terse)
        output << " (" << *obj << ")";
    output << COLOR_RESET;
    return "";
}


///@brief Returns a specific set of links for a provided PDF object. Decision is made by matching the type of PDF object.
///       Result includes predicates (if present)
///
/// e.g. if "array;number" is Types and [ArrayOfSomething];[] as Links, then if obj is an array the returned value is "[ArrayOfSomething]"
///
/// @param[in] obj      PDF object
/// @param[in] types    Arlington "Types" field from TSV data e.g. "array;dictionary;number". Predicates must be REMOVED.
/// @param[in] links    Arlington "Links" field from TSV data e.g. "[SomeArray];[SomeDictA,SomeDictB];[]". 
///
/// @returns Arlington link set (TSV file names) or "". Predicates are RETAINED.
std::string CParsePDF::get_linkset_for_object_type(ArlPDFObject* obj, const std::string &types, const std::string &links) {
    assert(obj != nullptr);
    assert(types.size() > 0);  // A Type is always required
    assert(links.size() >= 0); // Links are not required for fundamental types

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

    std::string reduced_types = types;

    // Only do predicate overhead if there are predicates
    if (types.find("fn:") != std::string::npos) {
        /// Reduce the predicates using PDF version
        TypePredicateProcessor type_reducer(pdfc, types);
        reduced_types = type_reducer.ReduceRow();
    } 

    PDFObjectType t = obj->get_object_type();
    std::vector<std::string> opt = split(reduced_types, ';');
    for (auto i = 0; i < (int)opt.size(); i++) {
        switch (t) {
            case PDFObjectType::ArlPDFObjTypeBoolean:
                if (opt[i] == "boolean")
                    return i;
                break;
            case PDFObjectType::ArlPDFObjTypeNumber:
                if ((opt[i] == "number") || (opt[i] == "integer") || (opt[i] == "bitmask"))
                    return i;
                break;
            case PDFObjectType::ArlPDFObjTypeName:
                if (opt[i] == "name")
                    return i;
                break;
            case PDFObjectType::ArlPDFObjTypeNull:
                if (opt[i] == "null")
                    return i;
                break;
            case PDFObjectType::ArlPDFObjTypeStream:
                if (opt[i] == "stream")
                    return i;
                break;
            case PDFObjectType::ArlPDFObjTypeString:
                if ((opt[i] == "string") || (opt[i] == "date") || (opt[i] == "string-byte") || (opt[i] == "string-text") || (opt[i] == "string-ascii"))
                    return i;
                break;
            case PDFObjectType::ArlPDFObjTypeArray:
                if ((opt[i] == "array") || (opt[i] == "rectangle") || (opt[i] == "matrix"))
                    return i;
                break;
            case PDFObjectType::ArlPDFObjTypeDictionary:
                if ((opt[i] == "dictionary") || (opt[i] == "number-tree") || (opt[i] == "name-tree"))
                    return i;
                break;
            case PDFObjectType::ArlPDFObjTypeReference:
                assert(false && "PDFObjectType::ArlPDFObjTypeReference");
                break;
            case PDFObjectType::ArlPDFObjTypeUnknown:
            default:
                assert(false); 
                break;
        } // switch
    } // for
    return -1;
}


/// @brief  Determines the Arlington type for a given PDF object.
///         Note that for some types there are ambiguities (e.g. an integer could also be a bitmask).
///
/// @param[in] obj PDF object
///
/// @return approximated Arlington type as a string
std::string CParsePDF::get_type_string_for_object(ArlPDFObject *obj) {
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


/// @brief  Recursively looks for 'key' via inheritance (i.e. through "Parent" keys)
///
/// @param[in] obj
/// @param[in] key        the key to find
/// @param[in] depth      recursive depth (in case of malformed PDFs to stop infinite loops!)
///
/// @returns nullptr if 'key' is NOT located via inheritance, otherwise the PDF object which matches BY KEYNAME!
ArlPDFObject* CParsePDF::find_via_inheritance(ArlPDFDictionary* obj, const std::wstring& key, int depth) {
    assert(obj != nullptr);
    if (depth > 250) {
        output << COLOR_ERROR << "recursive inheritance depth of " << depth << " exceeded for " << ToUtf8(key) << COLOR_RESET;
        return nullptr;
    }
    ArlPDFObject* parent = obj->get_value(L"Parent");
    if ((parent != nullptr) && (parent->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary)) {
        ArlPDFDictionary* parent_dict = (ArlPDFDictionary*)parent;
        ArlPDFObject* key_obj = parent_dict->get_value(key);
        if (key_obj == nullptr)
            key_obj = find_via_inheritance(parent_dict, key, depth+1);
        delete parent;
        return key_obj;
    }
    return nullptr;
}


/// @brief Validate basic information of a PDF object (stream, arrray, dictionary) including:
///   - type
///   - indirect
///   - possible value
///
/// @param[in]   object        the PDF object to check
/// @param[in]   key_idx       >= 0. Index into TSV data for this PDF object
/// @param[in]   tsv_data      the full Arlington PDF model definition read from a TSV
/// @param[in]   grammar_file  the name Arlington PDF model filename (object name) used for error messages
/// @param[in]   context       context (PDF path)
void CParsePDF::check_basics(ArlPDFObject *object, int key_idx, const ArlTSVmatrix& tsv_data, const fs::path &grammar_file, const std::string &context) {
    assert(object != nullptr);
    assert(key_idx >= 0);
    auto obj_type = object->get_object_type();

    // Need to cope with wildcard keys "*" in TSV data as key_idx might be beyond rows in tsv_data
    if (key_idx >= (int)tsv_data.size()) {
        if (tsv_data[0][TSV_KEYNAME] == "*")
            key_idx = 0;
        else if (tsv_data[tsv_data.size()-1][TSV_KEYNAME] == "*")
            key_idx = (int)tsv_data.size() - 1;
        else {
            /// @todo Arlington has more complex repetition patterns which are not supported yet
            if (!context_shown) {
                context_shown = true;
                output << context << std::endl;
            }
            output << COLOR_WARNING << "Arlington complex repetition patterns are not supported yet." << COLOR_RESET;
            return;
        }
    }

    // Treat null object as though the key is not present (i.e. don't report an error)
    if ((tsv_data[key_idx][TSV_INDIRECTREF] == "TRUE") &&
        (!object->is_indirect_ref() && (obj_type != PDFObjectType::ArlPDFObjTypeNull) && (obj_type != PDFObjectType::ArlPDFObjTypeReference))) {
        if (!context_shown) {
            context_shown = true;
            output << context << std::endl;
        }
        output << COLOR_ERROR << "not an indirect reference as required: " << tsv_data[key_idx][TSV_KEYNAME] << " (" << grammar_file.stem() << ")" << COLOR_RESET;
    }

    // check type. "null" is always valid and same as not present so ignore.
    int index = get_type_index_for_object(object, tsv_data[key_idx][TSV_TYPE]);
    if ((obj_type != PDFObjectType::ArlPDFObjTypeNull) && (index == -1)) {
        if (!context_shown) {
            context_shown = true;
            output << context << std::endl;
        }
        output << COLOR_ERROR << "wrong type: " << tsv_data[key_idx][TSV_KEYNAME] << " (" << grammar_file.stem() << ")";
        output << " should be: " << tsv_data[key_idx][TSV_TYPE] << " and is " << get_type_string_for_object(object) << " (" << *object << ")";
        output << COLOR_RESET;
    }

    // Possible Values: could be one of many
    // - simple: just a name, number, string, etc.
    // - complex type with SEMI-COLONS: [];[];[]
    if ((tsv_data[key_idx][TSV_POSSIBLEVALUES] != "") && (index != -1)) {
        std::wstring str_value;
        if (!check_possible_values(object, key_idx, tsv_data, index, str_value)) {
            if (tsv_data[key_idx][TSV_POSSIBLEVALUES].find("fn:") == std::string::npos) {
                // No predicates - so an error...
                if (!context_shown) {
                    context_shown = true;
                    output << context << std::endl;
                }
                output << COLOR_ERROR << "wrong value: " << tsv_data[key_idx][TSV_KEYNAME] << " (" << grammar_file.stem() << ")";
                output << " should be: " << tsv_data[key_idx][TSV_TYPE] << " " << tsv_data[key_idx][TSV_POSSIBLEVALUES] << " and is ";
                output << get_type_string_for_object(object) << "==" << ToUtf8(str_value) << " (" << *object << ")";
                output << COLOR_RESET;
            }
            else {
                PossibleValuesPredicateProcessor pv(pdfc, tsv_data[key_idx][TSV_POSSIBLEVALUES]);
                bool fully_processed = true;
                bool pv_reduced = pv.ReduceRow(object, key_idx, tsv_data, index, &fully_processed);
                if (!pv_reduced) {
                    if (!context_shown) {
                        context_shown = true;
                        output << context << std::endl;
                    }
                    // If predicates ARE fully processed then we know it is a wrong value.
                    // If predicates are partially processed then just a warning
                    if (!fully_processed) {
                        output << COLOR_WARNING << "possibly wrong value (some predicates NOT supported): " << tsv_data[key_idx][TSV_KEYNAME] << " (" << grammar_file.stem() << ")";
                    }
                    else {
                        output << COLOR_ERROR << "wrong value: " << tsv_data[key_idx][TSV_KEYNAME] << " (" << grammar_file.stem() << ")";
                    }
                    output << " should be: " << tsv_data[key_idx][TSV_TYPE] << " " << tsv_data[key_idx][TSV_POSSIBLEVALUES] << " and is ";
                    output << get_type_string_for_object(object) << "==" << ToUtf8(str_value) << " (" << *object << ")" << COLOR_RESET;
                }
            }
        }
    }
}


/// @brief Processes a PDF name tree
///
/// @param[in]     obj
/// @param[in]     links   set of Arlington links (predicates are SAFE)
/// @param[in,out] context
/// @param[in]     root    true if the root node of a Name tree
void CParsePDF::parse_name_tree(ArlPDFDictionary* obj, const std::string &links, std::string context, bool root) {
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
                    std::string  direct_link = get_link_for_object(obj2, links, as);
                    add_parse_object(obj2, direct_link, context+ "->[" + as + "]");
                    delete obj1;
                    continue;
                }
                else {
                    // Error: name tree Names array did not have pairs of entries (obj2 == nullptr)
                    output << COLOR_ERROR << "name tree Names array element #" << i << " - missing 2nd element in a pair for " << strip_leading_whitespace(context) << COLOR_RESET;
                }
            }
            else {
                // Error: 1st in the pair was not OK
                if (obj1 == nullptr)
                    output << COLOR_ERROR << "name tree Names array element #" << i << " - 1st element in a pair returned null for " << strip_leading_whitespace(context) << COLOR_RESET;
                else {
                    output << COLOR_ERROR << "name tree Names array element #" << i << " - 1st element in a pair was not a string for " << strip_leading_whitespace(context);
                    if (!terse)
                        output << " (" << *obj1 << ")";
                    output << COLOR_RESET;
                }
            }
            delete obj1;
        }
    }
    else {
        // Table 36 Names: "Root and leaf nodes only; required in leaf nodes; present in the root node
        //                  if and only if Kids is not present"
        if (root && (kids_obj == nullptr)) {
            if (names_obj == nullptr)
                output << COLOR_ERROR << "name tree Names object was missing when Kids was also missing for " << strip_leading_whitespace(context);
            else
                output << COLOR_ERROR << "name tree Names object was not an array when Kids was also missing for " << strip_leading_whitespace(context);
            if (!terse)
                output << " (" << *obj << ")";
            output << COLOR_RESET;
        }
    }
    delete names_obj;

    if (kids_obj != nullptr) {
        if (kids_obj->get_object_type() == PDFObjectType::ArlPDFObjTypeArray) {
            ArlPDFArray* array_obj = (ArlPDFArray*)kids_obj;
            for (int i = 0; i < array_obj->get_num_elements(); i++) {
                ArlPDFObject* item = array_obj->get_value(i);
                if ((item != nullptr) && (item->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary))
                    parse_name_tree((ArlPDFDictionary*)item, links, context, false);
                else {
                    // Error: individual kid isn't dictionary in PDF name tree
                    output << COLOR_ERROR << "name tree Kids array element number #" << i << " was not a dictionary for " << strip_leading_whitespace(context);
                    if (!terse && (item != nullptr))
                        output << " (" << *item << ")";
                    output << COLOR_RESET;
                }
                delete item;
            }
        }
        else {
            // error: Kids isn't array in PDF name tree
            output << COLOR_ERROR << "name tree Kids object was not an array for " << strip_leading_whitespace(context) << COLOR_RESET;
        }
        delete kids_obj;
    }
}


/// @brief Processes a PDF number tree
///
/// @param[in]     obj
/// @param[in]     links    set of Arlington links (Predicates are SAFE!)
/// @param[in,out] context
/// @param[in]     root   true if the root node of a Name tree
void CParsePDF::parse_number_tree(ArlPDFDictionary* obj, const std::string &links, std::string context, bool root) {
    ArlPDFObject *kids_obj   = obj->get_value(L"Kids");
    ArlPDFObject *nums_obj   = obj->get_value(L"Nums");
    // ArlPDFObject *limits_obj = obj->get_value(L"Limits");

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
                            std::string  direct_link = get_link_for_object(obj2, links, as);
                            add_parse_object(obj2, direct_link, context + "->[" + as + "]");
                        }
                        else {
                            // Error: every even entry in a number tree Nums array are supposed be objects
                            output << COLOR_ERROR << "number tree Nums array element #" << i << " was null for " << strip_leading_whitespace(context) << COLOR_RESET;
                        }
                    }
                    else {
                        // Error: every odd entry in a number tree Nums array are supposed be integers
                        output << COLOR_ERROR << "number tree Nums array element #" << i << " was not an integer for " << strip_leading_whitespace(context);
                        if (!terse)
                            output << " (" << *obj1 << ")";
                        output << COLOR_RESET;
                    }
                    delete obj1;
                }
                else {
                    // Error: one of the pair of objects was not OK in PDF number tree
                    output << COLOR_ERROR << "number tree Nums array was invalid for " << strip_leading_whitespace(context) << COLOR_RESET;
                }
            } // for
        }
        else {
            // Error: Nums isn't an array in PDF number tree
            output << COLOR_ERROR << "number tree Nums object was not an array for " << strip_leading_whitespace(context) << COLOR_RESET;
        }
        delete nums_obj;
    }
    else {
        // Table 37 Nums: "Root and leaf nodes only; shall be required in leaf nodes;
        //                 present in the root node if and only if Kids is not present
        if (root && (kids_obj == nullptr)) {
            output << COLOR_ERROR << "number tree Nums object was missing when Kids was also missing for " << strip_leading_whitespace(context);
            if (!terse)
                output << " (" << *obj << ")";
            output << COLOR_RESET;
        }
    }

    if (kids_obj != nullptr) {
        if (kids_obj->get_object_type() == PDFObjectType::ArlPDFObjTypeArray) {
            ArlPDFArray* array_obj = (ArlPDFArray*)kids_obj;
            for (int i = 0; i < array_obj->get_num_elements(); i++) {
                ArlPDFObject* item = array_obj->get_value(i);
                if ((item != nullptr) && (item->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary)) {
                    parse_number_tree((ArlPDFDictionary*)item, links, context, false);
                }
                else {
                    // Error: individual kid isn't dictionary in PDF number tree
                    output << COLOR_ERROR << "number tree Kids array element number #" << i << " was not a dictionary for " << context;
                    if (!terse && (item != nullptr))
                        output << " (" << *item << ")";
                    output << COLOR_RESET;
                }
                delete item;
            }
        }
        else {
            // Error: Kids isn't array in PDF number tree
            output << COLOR_ERROR << "number tree Kids object was not an array for " << context;
            if (!terse)
                output << " (" << *kids_obj << ")";
            output << COLOR_RESET;
        }
        delete kids_obj;
    }
}


/// @brief Queues a PDF object for processing against an Arlington link, and with a PDF path context
///
/// @param[in]     object   PDF object (not nullptr)
/// @param[in]     link     Arlington link (TSV filename)
/// @param[in,out] context  current content (PDF path)
void CParsePDF::add_parse_object(ArlPDFObject* object, const std::string& link, const std::string& context) {
    assert(object != nullptr);
    to_process.emplace(object, link, context);
}

/// @brief Iteratively parse PDF objects from the to_process queue
///
/// @param[in] pdf   PDF file object
void CParsePDF::parse_object(CPDFFile &pdf)
{
    pdfc = &pdf;
    std::string ver = pdfc->get_pdf_version(output); // may produce messages
    output << "Processing file as PDF " << ver << std::endl;

    counter = 0;

    while (!to_process.empty()) {
        context_shown = false;

        counter++;
        queue_elem elem = to_process.front();
        to_process.pop();
        if (elem.link == "") {
            delete elem.object;
            continue;
        }

        // Need to clean up elem.link due to predicates "fn:SinceVersion(x,y,...)"
        elem.link = remove_type_link_predicates(elem.link);

        if (!terse) {
            output << std::setw(8) << counter << ": " << elem.context << std::endl;
            context_shown = true;
        }
        elem.context = "  " + elem.context;

        assert(elem.object != nullptr);
        if (elem.object->is_indirect_ref()) {
            auto hash = elem.object->get_hash_id();
            auto found = mapped.find(hash);
            if (found != mapped.end()) {
                // "_Universal..." objects match anything so ignore them.
                if ((found->second != elem.link) &&
                    (((elem.link != "_UniversalDictionary") && (elem.link != "_UniversalArray")) &&
                    ((found->second != "_UniversalDictionary") && (found->second != "_UniversalArray")))) {
                    if (!context_shown) {
                        output << elem.context << std::endl;
                        context_shown = true;
                    }
                    output << COLOR_ERROR << "object validated in two different contexts. First: \"" << found->second << "\"; second: \"" << elem.link << "\"";
                    output << COLOR_RESET;
                }
                delete elem.object;
                continue;
            }
            // remember visited object with a link used for validation
            mapped.insert(std::make_pair(hash, elem.link));
        }

        fs::path  grammar_file = grammar_folder;
        grammar_file /= elem.link + ".tsv";
        const ArlTSVmatrix &data_list = get_grammar(elem.link);

        // Validating as dictionary:
        // - going through all objects in dictionary
        // - checking basics (Type, PossibleValue, indirect)
        // - then check presence of required keys
        // - then recursively calling validation for each container with link to other grammar file
        assert(elem.object != nullptr);
        auto obj_type = elem.object->get_object_type();

        if ((obj_type == PDFObjectType::ArlPDFObjTypeDictionary) || (obj_type == PDFObjectType::ArlPDFObjTypeStream)) {
            ArlPDFDictionary* dictObj;

            // validate values first, then process containers
            if (obj_type == PDFObjectType::ArlPDFObjTypeStream)
                dictObj = ((ArlPDFStream*)elem.object)->get_dictionary();
            else
                dictObj = (ArlPDFDictionary*)elem.object;

            auto dict_num_keys = dictObj->get_num_keys();
            for (int i = 0; i < dict_num_keys; i++) {
                std::wstring key = dictObj->get_key_name_by_index(i);
                ArlPDFObject* inner_obj = dictObj->get_value(key);

                // might have wrong/malformed object. Key exists, but value does not
                if (inner_obj != nullptr) {
                    bool is_found = false;
                    int key_idx = -1;
                    for (auto& vec : data_list) {
                        key_idx++;
                        if (vec[TSV_KEYNAME] == ToUtf8(key)) {
                            check_basics(inner_obj, key_idx, data_list, elem.link, elem.context);
                            is_found = true;
                            delete inner_obj;
                            break;
                        }
                    }

                    // Metadata streams are allowed anywhere
                    if ((!is_found) && (key == L"Metadata")) {
                        add_parse_object(inner_obj, "Metadata", elem.context + "->Metadata");
                        output << COLOR_INFO << "found a Metadata key in " << strip_leading_whitespace(elem.context) << COLOR_RESET;
                        is_found = true;
                    }

                    // AF (Associated File) objects are allowed anywhere
                    if ((!is_found) && (key == L"AF")) {
                        add_parse_object(inner_obj, "FileSpecification", elem.context + "->AF");
                        output << COLOR_INFO << "found an Associated File AF key in " << strip_leading_whitespace(elem.context) << COLOR_RESET;
                        is_found = true;
                    }

                    // we didn't find the key, there may be wildcard key (*) that will validate
                    if (!is_found)
                        for (auto& vec : data_list)
                            if (vec[TSV_KEYNAME] == "*") {
                                if (vec[TSV_LINK] != "") {
                                    // wildcard is a complex type so recurse
                                    std::string lnk = get_linkset_for_object_type(inner_obj, vec[TSV_TYPE], vec[TSV_LINK]);
                                    std::string as = ToUtf8(key);
                                    std::string direct_link = get_link_for_object(inner_obj, lnk, as);
                                    add_parse_object(inner_obj, direct_link, elem.context + "->" + as);
                                }
                                else {
                                    delete inner_obj;
                                }
                                is_found = true;
                                break;
                            }

                    // Still didn't find the key - report as an extension 
                    if (!is_found) {
                        std::string k = ToUtf8(key);
                        if (!context_shown) {
                            output << elem.context << std::endl;
                            context_shown = true;
                        }
                        if (is_second_class_pdf_name(k))
                            output << COLOR_INFO << "second class key '" << k << "' is not defined in Arlington for \"" << elem.link << "\"";
                        else if (is_third_class_pdf_name(k))
                            output << COLOR_INFO << "third class key '" << k << "' found in \"" << elem.link << "\"";
                        else
                            output << COLOR_INFO << "unknown key '" << k << "' is not defined in Arlington for \"" << elem.link << "\"";
                        output << COLOR_RESET;
                        delete inner_obj;
                    }
                }
                else {
                    // malformed PDF or parsing limitation in PDF SDK?
                    if (!context_shown) {
                        output << elem.context << std::endl;
                        context_shown = true;
                    }
                    output << COLOR_ERROR << "could not get value for key '" << ToUtf8(key) << "' (\"" << elem.link << "\")" << COLOR_RESET;
                }
            } // for

            // check presence of required values in object first, then parents if inheritable
            for (auto& vec : data_list)
                if (is_required_key(elem.object, vec[TSV_REQUIRED]) && (vec[TSV_KEYNAME] != "*")) {
                    ArlPDFObject* inner_obj = dictObj->get_value(ToWString(vec[TSV_KEYNAME]));
                    if (inner_obj == nullptr) {
                        if (vec[TSV_INHERITABLE] == "FALSE") {
                            if (!context_shown) {
                                output << elem.context << std::endl;
                                context_shown = true;
                            }
                            output << COLOR_ERROR << "non-inheritable required key doesn't exist: " << vec[TSV_KEYNAME] << " (\"" << elem.link << "\")";
                            output << " (" << *dictObj << ")";
                            output << COLOR_RESET;
                        } else {
                            inner_obj = find_via_inheritance(dictObj, ToWString(vec[TSV_KEYNAME]));
                            if (inner_obj == nullptr) {
                                if (!context_shown) {
                                    output << elem.context << std::endl;
                                    context_shown = true;
                                }
                                output << COLOR_ERROR << "inheritable required key doesn't exist: " << vec[TSV_KEYNAME] << " (\"" << elem.link << "\")";
                                output << " (" << *dictObj << ")";
                                output << COLOR_RESET;
                            }
                            else {
                                delete inner_obj;
                            }
                        }
                    }
                    else {
                        delete inner_obj;
                    }
                }

            // now go through containers and process them with new elem.link
            for (auto& vec : data_list) {
                if (vec.size() >= TSV_NOTES && vec[TSV_LINK] != "") {
                    ArlPDFObject* inner_obj = dictObj->get_value(ToWString(vec[TSV_KEYNAME]));
                    if (inner_obj != nullptr) {
                        int index = get_type_index_for_object(inner_obj, vec[TSV_TYPE]);
                        if (index == -1) {  // error already reported before
                            delete inner_obj;
                            break;
                        }
                        std::vector<std::string> opt   = split(vec[TSV_TYPE], ';');
                        std::vector<std::string> links = split(vec[TSV_LINK], ';');
                        if (links[index] == "[]") {
                            delete inner_obj;
                            continue;
                        }

                        opt[index] = remove_type_link_predicates(opt[index]);

                        auto inner_obj_type = inner_obj->get_object_type();
                        if ((opt[index] == "number-tree") && (inner_obj_type == PDFObjectType::ArlPDFObjTypeDictionary)) {
                            parse_number_tree((ArlPDFDictionary*)inner_obj, links[index], elem.context + "->" + vec[TSV_KEYNAME]);
                            delete inner_obj;
                        }
                        else if ((opt[index] == "name-tree") && (inner_obj_type == PDFObjectType::ArlPDFObjTypeDictionary)) {
                            parse_name_tree((ArlPDFDictionary*)inner_obj, links[index], elem.context + "->" + vec[TSV_KEYNAME]);
                            delete inner_obj;
                        }
                        else if ((inner_obj_type == PDFObjectType::ArlPDFObjTypeDictionary) ||
                                 (inner_obj_type == PDFObjectType::ArlPDFObjTypeStream) ||
                                 (inner_obj_type == PDFObjectType::ArlPDFObjTypeArray)) {
                            std::string as = vec[TSV_KEYNAME];
                            std::string direct_link = get_link_for_object(inner_obj, links[index], as);
                            add_parse_object(inner_obj, direct_link, elem.context + "->" + as);
                        }
                        else {
                            delete inner_obj;
                        }
                    }
                }
                continue;
            } // for

            if (obj_type == PDFObjectType::ArlPDFObjTypeStream)
                delete dictObj; // Only delete for streams
        } 
        else if (obj_type == PDFObjectType::ArlPDFObjTypeArray) {
            ArlPDFArray*    arrayObj = (ArlPDFArray*)elem.object;

            // Preview the Arlington array definition for wildcards and to see if this really is an array
            // Also determine minimum required number of elements in the array.
            constexpr auto ALL_ARRAY_ELEMS = 99999;
            int first_wildcard = ALL_ARRAY_ELEMS; // bigger than any TSV row count
            int first_notreqd  = ALL_ARRAY_ELEMS; // bigger than any TSV row count
            std::vector<std::string>    key_list;

            for (int i = 0; i < (int)data_list.size(); i++) {
                key_list.push_back(data_list[i][TSV_KEYNAME]);
                if ((first_notreqd > i) && (data_list[i][TSV_REQUIRED] != "TRUE"))
                    first_notreqd = i;

                if (first_wildcard > i)
                    if (data_list[i][TSV_KEYNAME].find("*") != std::string::npos)
                        first_wildcard = i;
            } // for

            // Use null-stream to suppress messages - should have used "--validate" first anyway
            bool ambiguous;
            if (!check_valid_array_definition(elem.link, key_list, cnull, &ambiguous)) {
                if (!context_shown) {
                    output << elem.context << std::endl;
                    context_shown = true;
                }
                output << COLOR_ERROR << "PDF array object encountered, but using Arlington dictionary \"" << elem.link << "\"" << COLOR_RESET;
                delete elem.object;
                continue;
            }

            for (int i = 0; i < arrayObj->get_num_elements(); i++) {
                ArlPDFObject* item = arrayObj->get_value(i);
                if (item != nullptr) {
                    if ((first_wildcard == 0) && (data_list[0][TSV_KEYNAME] == "*")) {
                        // All array elements will match wildcard
                        check_basics(item, 0, data_list, elem.link, elem.context);
                        if (data_list[0][TSV_LINK] != "") {
                            std::string t = remove_type_link_predicates(data_list[0][TSV_TYPE]);
                            std::string lnk = get_linkset_for_object_type(item, t, data_list[0][TSV_LINK]);
                            std::string as = "[" + std::to_string(i) + "]";
                            std::string direct_link = get_link_for_object(item, lnk, as);
                            add_parse_object(item, direct_link, elem.context + as);
                        }
                        else {
                            delete item;
                        }
                    }
                    else if ((first_wildcard > i) && (i < (int)data_list.size())) {
                        // No wildcards to this array element
                        assert(data_list[i][TSV_KEYNAME] == std::to_string(i));
                        check_basics(item, i, data_list, elem.link, elem.context);
                        if (data_list[i][TSV_LINK] != "") {
                            std::string t = remove_type_link_predicates(data_list[i][TSV_TYPE]);
                            std::string lnk = get_linkset_for_object_type(item, t, data_list[i][TSV_LINK]);
                            std::string as = "[" + std::to_string(i) + "]";
                            std::string direct_link = get_link_for_object(item, lnk, as);
                            add_parse_object(item, direct_link, elem.context + as);
                        }
                        else {
                            delete item;
                        }
                    }
                    else {
                        /// @todo  Support array wildcards fully (integer + *)
                        delete item;
                    }
                }
            } // for-each array element

            auto array_size = arrayObj->get_num_elements();
            if ((first_notreqd == ALL_ARRAY_ELEMS) && (first_wildcard == ALL_ARRAY_ELEMS) && ((int)data_list.size() != array_size)) {
                if (!context_shown) {
                    output << elem.context << std::endl;
                    context_shown = true;
                }
                output << COLOR_ERROR << "array length incorrect for \"" << elem.link << "\"";
                output << ", wanted " << data_list.size() << ", got " << array_size << " (" << *arrayObj << ")";
                output << COLOR_RESET;
            }

            if ((first_notreqd != ALL_ARRAY_ELEMS) && (first_notreqd > array_size)) {
                if (!context_shown) {
                    output << elem.context << std::endl;
                    context_shown = true;
                }
                output << COLOR_ERROR << "array \"" << elem.link << "\" requires " << first_notreqd << " elements, but only had " << array_size;
                output << " (" << *arrayObj << ")";
                output << COLOR_RESET;
            }
        }
        else {
            if (!context_shown) {
                output << elem.context << std::endl;
                context_shown = true;
            }
            output << COLOR_ERROR << "unexpected object type " << PDFObjectType_strings[(int)obj_type] << " for \"" << elem.link << "\"";
            output << " (" << *elem.object << ")";
            output << COLOR_RESET;
        }
        delete elem.object;
    } // while queue not empty

    // Clean up
    pdfc = nullptr;
}
