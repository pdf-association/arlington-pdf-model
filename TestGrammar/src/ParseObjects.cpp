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
  Reading the whole PDF starting from specific object and validating against grammar
  provided via an Arlington TSV file set
*/

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

#include "ArlingtonTSVGrammarFile.h"
#include "ParseObjects.h"
#include "PredicateProcessor.h"
#include "utils.h"

using namespace ArlingtonPDFShim;
namespace fs = std::filesystem;

/// @brief Matches integer-only array indices (NO WILDCARDS!) for TSV_KEYNAME field
///        Note that some PDF keys are real number like e.g. "/1.2"
const std::regex r_KeyArrayIndices("^[0-9]+$");


/// @brief Locates or reads in a single Arlington TSV grammar file. The input data is not altered or validated.
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
/// @todo Arlington predicates in the PossibleValues are NOT currently supported
///
/// @param[in] object                a valid PDF object
/// @param[in] key_idx               >= 0. Index into the TSV data for thsi key/array element.
/// @param[in] tsv_data              full Arlington TSV data for this PDF object.
/// @param[in] index                 >= 0. Index into Types field supporting complex types ([];[];[])
/// @param[out] real_str_value       the key value converted to a string representation
///
/// @returns true if and only if PDF object has the correct type and a valid value according to PossibleValues.
bool CParsePDF::check_possible_values(ArlPDFObject* object, int key_idx, const ArlTSVmatrix &tsv_data, const int index, std::wstring &real_str_value) {
    double num_value;
    assert(object != nullptr);
    assert(key_idx >= 0);
    assert(key_idx < tsv_data.size());
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

    // Need to cope with wildcard keys "*" in TSV data as key_idx might be beyond rows in tsv_data
    if (key_idx >= tsv_data.size()) {
        if (tsv_data[0][TSV_KEYNAME] == "*")
            key_idx = 0;
        else if (tsv_data[tsv_data.size() - 1][TSV_KEYNAME] == "*")
            key_idx = (int)tsv_data.size() - 1;
        else {
            /// @todo Arlington has more complex repetition patterns which are not supported
            return false;
        }
    }

    /// possible_value_str might be something like: [a,fn:A(b),c];[];[d,fn:C(xx,fn:B(yyy,e)),f]

    if (tsv_data[key_idx][TSV_POSSIBLEVALUES] == "") {
        return true; // no PossibleValues defined at all in Arlington so any value is OK
    }
    else {
        std::vector<std::string>    options = split(tsv_data[key_idx][TSV_POSSIBLEVALUES], ';');
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


/// @brief Supports Arlington vec[TSV_REQUIRED] field which is either TRUE, FALSE or predicates. This method
///        calculates predicates
///  e.g. fn:IsRequired(fn:IsPresent(Solidities)); fn:IsRequired(@SubFilter==adbe.pkcs7.s3 || @SubFilter==adbe.pkcs7.s4)
///
/// @returns true if the key is required. false if the key is not required or the predicate is too complex
bool CParsePDF::is_required_key(ArlPDFObject* obj, const std::string &reqd, const std::string& pdf_vers) {
    assert(obj != nullptr);

    if (reqd == "TRUE")
        return true;
    else if (reqd == "FALSE")
        return false;
    else {
        const std::regex   r_isRequired("^fn:IsRequired\\((.+)\\)");     // $1 = inner
        const std::regex   r_isPresent("fn:IsPresent\\(([^\\)]+)\\)");   // $1 = inner
        const std::regex   r_notPresent("fn:NotPresent\\(([^\\)]+)\\)"); // $1 = inner
        std::regex      r_keyValueEquals("@(.+)==(.+)");                 // $1 = LHS, "==", $2 = RHS
        std::regex      r_keyValueNotEquals("@(.+)!=(.+)");              // $1 = LHS, "!=", $2 = RHS
        std::string     inner;
        std::string     key;
        std::smatch     m;
        ArlPDFObject*   val;

        // Strip off outer predicate "^fn:IsRequired( ... )"
        inner = std::regex_replace(reqd, r_isRequired, "$1");

        // Ensure PDF version supplied is valid
        assert(std::regex_search(pdf_vers, std::regex(ArlPDFVersion)));

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


/// @brief define SCORING_DEBUG to see scoring unside get_link_for_object()
//#define SCORING_DEBUG


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
    std::string s = remove_type_predicates(links_string);
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
    PDFObjectType obj_type = obj->get_object_type();
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
                            }
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
                    } else {
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

    output << "Error: can't select any link from " << links_string <<" to validate provided object" << obj_name;
    if (!terse)
        output << " (" << *obj << ")";
    output << std::endl;
    return "";
}


///@brief Returns a specific set of links for a provided PDF object. Decision is made by matching the type of PDF object.
///       Result includes predicates (if present)
///
/// e.g. if "array;number" is Types and [ArrayOfSomething];[] as Links, then if obj is an array the returned value is "[ArrayOfSomething]"
///
/// @param[in] obj      PDF object
/// @param[in] types    Arlington "Types" field from TSV data e.g. "array;dictionary;number". Predicates must be REMOVED.
/// @param[in] links    Arlington "Links" field from TSV data e.g. "[SomeArray];[SomeDictA,SomeDictB];[]".  Predicates must be REMOVED.
///
/// @returns Arlington link set (TSV file names) or "". Predicates are RETAINED.
std::string CParsePDF::get_linkset_for_object_type(ArlPDFObject* obj, const std::string &types, const std::string &links) {
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
        output << "Error: recursive inheritance depth of " << depth << " exceeded for " << ToUtf8(key) << std::endl;
        return nullptr;
    }
    ArlPDFObject* parent = obj->get_value(L"Parent");
    if ((parent != nullptr) && (parent->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary)) {
        ArlPDFDictionary* parent_dict = (ArlPDFDictionary*)parent;
        ArlPDFObject* key_obj = parent_dict->get_value(key);
        if (key_obj == nullptr)
            return find_via_inheritance(parent_dict, key, depth+1);
        return key_obj;
    }
    return nullptr;
}



/// @brief Validate basic information on a PDF object (stream, arrray, dictionary) including:
///   - type
///   - indirect
///   - possible value
///
/// @param[in]   object        the PDF object to check
/// @param[in]   key_idx       >= 0. Index into TSV data for this PDF object
/// @param[in]   tsv_data      the full Arlington PDF model definition read from a TSV
/// @param[in]   grammar_file  the name Arlington PDF model filename (object name) used for error messages
void CParsePDF::check_basics(ArlPDFObject *object, int key_idx, const ArlTSVmatrix& tsv_data, const fs::path &grammar_file) {
    assert(object != nullptr);
    assert(key_idx >= 0);
    auto obj_type = object->get_object_type();

    // Need to cope with wildcard keys "*" in TSV data as key_idx might be beyond rows in tsv_data
    if (key_idx >= tsv_data.size()) {
        if (tsv_data[0][TSV_KEYNAME] == "*")
            key_idx = 0;
        else if (tsv_data[tsv_data.size()-1][TSV_KEYNAME] == "*")
            key_idx = (int)tsv_data.size() - 1;
        else {
            /// @todo Arlington has more complex repetition patterns which are not supported
            return;
        }
    }

    // Treat null object as though the key is not present (i.e. don't report an error)
    if ((tsv_data[key_idx][TSV_INDIRECTREF] == "TRUE") &&
        (!object->is_indirect_ref() && (obj_type != PDFObjectType::ArlPDFObjTypeNull) && (obj_type != PDFObjectType::ArlPDFObjTypeReference))) {
        output << "Error: not an indirect reference as required: " << tsv_data[key_idx][TSV_KEYNAME] << " (" << grammar_file.stem() << ")" << std::endl;
    }

    // check type. "null" is always valid and same as not present so ignore.
    int index = get_type_index_for_object(object, tsv_data[key_idx][TSV_TYPE]);
    if ((obj_type != PDFObjectType::ArlPDFObjTypeNull) && (index == -1)) {
        output << "Error: wrong type: " << tsv_data[key_idx][TSV_KEYNAME] << " (" << grammar_file.stem() << ")";
        if (!terse)
            output << " should be: " << tsv_data[key_idx][TSV_TYPE] << " and is " << get_type_string_for_object(object) << " (" << *object << ")";
        output << std::endl;
    }

    // possible value, could be one of many
    // could be a pattern array;name -- [];[name1,name2]
    // could be single reference -- name1,name2
    // we should cover also single reference in brackets [name1,name2]
    if ((tsv_data[key_idx][TSV_POSSIBLEVALUES] != "") && (index != -1)) {
        std::wstring str_value;
        if (!check_possible_values(object, key_idx, tsv_data, index, str_value)) {
            if (tsv_data[key_idx][TSV_POSSIBLEVALUES].find("fn:") == std::string::npos) {
                // No predicates - so an error...
                output << "Error: wrong value: " << tsv_data[key_idx][TSV_KEYNAME] << " (" << grammar_file.stem() << ")";
            }
            else {
                /// @todo Predicates - so just a warning...
                output << "Warning: possibly wrong value (predicates NOT supported): " << tsv_data[key_idx][TSV_KEYNAME] << " (" << grammar_file.stem() << ")";
            }
            if (!terse) {
                output << " should be: " << tsv_data[key_idx][TSV_TYPE] << " " << tsv_data[key_idx][TSV_POSSIBLEVALUES] << " and is ";
                output << get_type_string_for_object(object) << "==" << ToUtf8(str_value) << " (" << *object << ")";
            }
            output << std::endl;
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
                    continue;
                }
                else {
                    // Error: name tree Names array did not have pairs of entries (obj2 == nullptr)
                    output << "Error: name tree Names array element #" << i << " - missing 2nd element in a pair" << std::endl;
                }
            }
            else {
                // Error: 1st in the pair was not OK
                if (obj1 == nullptr)
                    output << "Error: name tree Names array element #" << i << " - 1st element in a pair returned null" << std::endl;
                else {
                    output << "Error: name tree Names array element #" << i << " - 1st element in a pair was not a string";
                    if (!terse)
                        output << " (" << *obj1 << ")";
                    output << std::endl;
                }
            }
        }
    }
    else {
        // Table 36 Names: "Root and leaf nodes only; required in leaf nodes; present in the root node
        //                  if and only if Kids is not present"
        if (root && (kids_obj == nullptr)) {
            if (names_obj == nullptr)
                output << "Error: name tree Names object was missing when Kids was also missing";
            else
                output << "Error: name tree Names object was not an array when Kids was also missing";
            if (!terse)
                output << " (" << *obj << ")";
            output << std::endl;
        }
    }

    if (kids_obj != nullptr) {
        if (kids_obj->get_object_type() == PDFObjectType::ArlPDFObjTypeArray) {
            ArlPDFArray* array_obj = (ArlPDFArray*)kids_obj;
            for (int i = 0; i < array_obj->get_num_elements(); i++) {
                ArlPDFObject* item = array_obj->get_value(i);
                if ((item != nullptr) && (item->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary))
                    parse_name_tree((ArlPDFDictionary*)item, links, context, false);
                else {
                    // Error: individual kid isn't dictionary in PDF name tree
                    output << "Error: name tree Kids array element number #" << i << " was not a dictionary";
                    if (!terse && (item != nullptr))
                        output << " (" << *item << ")";
                    output << std::endl;
                }
            }
        }
        else {
            // error: Kids isn't array in PDF name tree
            output << "Error: name tree Kids object was not an array" << std::endl;
        }
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
                            std::string  direct_link = get_link_for_object(obj2, links, as);
                            add_parse_object(obj2, direct_link, context + "->[" + as + "]");
                        }
                        else {
                            // Error: every even entry in a number tree Nums array are supposed be objects
                            output << "Error: number tree Nums array element #" << i << " was null" << std::endl;
                        }
                    }
                    else {
                        // Error: every odd entry in a number tree Nums array are supposed be integers
                        output << "Error: number tree Nums array element #" << i << " was not an integer";
                        if (!terse)
                            output << " (" << *obj1 << ")";
                        output << std::endl;
                    }
                }
                else {
                    // Error: one of the pair of objects was not OK in PDF number tree
                    output << "Error: number tree Nums array was invalid" << std::endl;
                }
            } // for
        }
        else {
            // Error: Nums isn't an array in PDF number tree
            output << "Error: number tree Nums object was not an array" << std::endl;
        }
    }
    else {
        // Table 37 Nums: "Root and leaf nodes only; shall be required in leaf nodes;
        //                 present in the root node if and only if Kids is not present
        if (root && (kids_obj == nullptr)) {
            output << "Error: number tree Nums object was missing when Kids was also missing";
            if (!terse)
                output << " (" << *obj << ")";
            output << std::endl;
        }
    }

    if (kids_obj != nullptr) {
        if (kids_obj->get_object_type() == PDFObjectType::ArlPDFObjTypeArray) {
            ArlPDFArray* array_obj = (ArlPDFArray*)kids_obj;
            for (int i = 0; i < array_obj->get_num_elements(); i++) {
                ArlPDFObject* item = array_obj->get_value(i);
                if ((item != nullptr) && (item->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary))
                    parse_number_tree((ArlPDFDictionary*)item, links, context, false);
                else {
                    // Error: individual kid isn't dictionary in PDF number tree
                    output << "Error: number tree Kids array element number #" << i << " was not a dictionary";
                    if (!terse && (item != nullptr))
                        output << " (" << *item << ")";
                    output << std::endl;
                }
            }
        }
        else {
            // Error: Kids isn't array in PDF number tree
            output << "Error: number tree Kids object was not an array";
            if (!terse)
                output << " (" << *kids_obj << ")";
            output << std::endl;
        }
    }
}


/// @brief Queues a PDF object for processing against an Arlington link, and with a PDF path context
///
/// @param[in]     object   PDF object (not nullptr)
/// @param[in]     link     Arlington link (TSV filename)
/// @param[in,out] context  current content (PDF path)
void CParsePDF::add_parse_object(ArlPDFObject* object, const std::string& link, std::string context) {
    to_process.emplace(object, link, context);
}



/// @brief Iteratively parse PDF objects from the to_process queue
void CParsePDF::parse_object()
{
    while (!to_process.empty()) {
        auto elem = to_process.front();
        to_process.pop();
        if (elem.link == "")
            continue;

        // Need to clean up the elem.link due to predicates "fn:SinceVersion(x,y, ...)"
        elem.link = remove_link_predicates(elem.link);

        if (elem.object->is_indirect_ref()) {
            auto hash = elem.object->get_hash_id();
            auto found = mapped.find(hash);
            if (found != mapped.end()) {
                // "_Universal..." objects match anything so ignore them.
                if ((found->second != elem.link) &&
                      (((elem.link != "_UniversalDictionary") && (elem.link != "_UniversalArray")) &&
                       ((found->second != "_UniversalDictionary") && (found->second != "_UniversalArray")))) {
                    output << "Error: object validated in two different contexts. First: " << found->second << "; second: " << elem.link;
                    if (!terse)
                        output << " in: " << strip_leading_whitespace(elem.context) << " " << found->first;
                    output << std::endl;
                }
                continue;
            }
            // remember visited object with a link used for validation
            mapped.insert(std::make_pair(hash, elem.link));
        }

        output << elem.context << std::endl;
        elem.context = "  " + elem.context;

        fs::path  grammar_file = grammar_folder;
        grammar_file /= elem.link + ".tsv";
        const ArlTSVmatrix &data_list = get_grammar(elem.link);

        // Validating as dictionary:
        // - going through all objects in dictionary
        // - checking basics (Type, PossibleValue, indirect)
        // - then check presence of required keys
        // - then recursively calling validation for each container with link to other grammar file
        PDFObjectType obj_type = elem.object->get_object_type();

        if ((obj_type == PDFObjectType::ArlPDFObjTypeDictionary) || (obj_type == PDFObjectType::ArlPDFObjTypeStream)) {
            ArlPDFDictionary* dictObj;

            // validate values first, then process containers
            if (elem.object->get_object_type() == PDFObjectType::ArlPDFObjTypeStream)
                dictObj = ((ArlPDFStream*)elem.object)->get_dictionary();
            else
                dictObj = (ArlPDFDictionary*)elem.object;

            for (int i = 0; i < (dictObj->get_num_keys()); i++) {
                std::wstring key = dictObj->get_key_name_by_index(i);
                ArlPDFObject* inner_obj = dictObj->get_value(key);

                // might have wrong/malformed object. Key exists, but value does not
                if (inner_obj != nullptr) {
                    bool is_found = false;
                    int key_idx = -1;
                    for (auto& vec : data_list) {
                        key_idx++;
                        if (vec[TSV_KEYNAME] == ToUtf8(key)) {
                            check_basics(inner_obj, key_idx, data_list, elem.link);
                            is_found = true;
                            break;
                        }
                    }

                    // Metadata streams are allowed anywhere
                    if (!is_found)
                        if (key == L"Metadata") {
                            add_parse_object(inner_obj, "Metadata", elem.context + "->Metadata");
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
                                is_found = true;
                                break;
                            }

                    // Still didn't find the key - report as an extension if not terse.
                    if (!is_found && !terse) {
                        std::string k = ToUtf8(key);
                        if (is_second_or_third_class_pdf_name(k))
                            output << "Warning: second/third class key '" << k << "' is not defined in Arlington for " << elem.link << std::endl;
                        else
                            output << "Warning: unknown key '" << k << "' is not defined in Arlington for " << elem.link << std::endl;
                    }
                }
                else {
                    // malformed PDF or parsing limitation in PDF SDK?
                    output << "Error: could not get value for key '" << ToUtf8(key) << "' (" << elem.link << ")" << std::endl;
                }
            } // for

            // check presence of required values in object first, then parents if inheritable
            for (auto& vec : data_list)
                if (is_required_key(elem.object, vec[TSV_REQUIRED]) && (vec[TSV_KEYNAME] != "*")) {
                    ArlPDFObject* inner_obj = dictObj->get_value(ToWString(vec[TSV_KEYNAME]));
                    if (inner_obj == nullptr) {
                        if (vec[TSV_INHERITABLE] == "FALSE") {
                            output << "Error: non-inheritable required key doesn't exist: " << vec[TSV_KEYNAME] << " (" << elem.link << ")";
                            if (!terse)
                                output << " (" << *dictObj << ")";
                            output << std::endl;
                        } else {
                            /// @todo support inheritance
                            inner_obj = find_via_inheritance(dictObj, ToWString(vec[TSV_KEYNAME]));
                            if (inner_obj == nullptr) {
                                output << "Error: inheritable required key doesn't exist: " << vec[TSV_KEYNAME] << " (" << elem.link << ")";
                                if (!terse)
                                    output << " (" << *dictObj << ")";
                                output << std::endl;
                            }
                        }
                    }
                }

            // now go through containers and process them with new elem.link
            for (auto& vec : data_list) {
                if (vec.size() >= TSV_NOTES && vec[TSV_LINK] != "") {
                    ArlPDFObject* inner_obj = dictObj->get_value(ToWString(vec[TSV_KEYNAME]));
                    if (inner_obj != nullptr) {
                        int index = get_type_index_for_object(inner_obj, vec[TSV_TYPE]);
                        if (index == -1)   // error already reported before
                            break;
                        std::vector<std::string> opt   = split(vec[TSV_TYPE], ';');
                        std::vector<std::string> links = split(vec[TSV_LINK], ';');
                        if (links[index] == "[]")
                            continue;

                        opt[index]   = remove_type_predicates(opt[index]);

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
                            std::string direct_link = get_link_for_object(inner_obj, links[index], as);
                            add_parse_object(inner_obj, direct_link, elem.context + "->" + as);
                        }
                    }
                }
                continue;
            } // for
        } else if (obj_type == PDFObjectType::ArlPDFObjTypeArray) {
            ArlPDFArray*    arrayObj = (ArlPDFArray*)elem.object;

            // Preview the Arlington array definition for wildcards and to see if this really is an array
            // Also determine minimum required number of elements in the array.
#define ALL_ARRAY_ELEMS 99999
            int             first_wildcard = ALL_ARRAY_ELEMS; // bigger than any TSV row count
            int             first_notreqd = ALL_ARRAY_ELEMS; // bigger than any TSV row count
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
            if (!check_valid_array_definition(elem.link, key_list, cnull)) {
                output << "Error: PDF array object encountered, but using Arlington dictionary " << elem.link << std::endl;
                continue;
            }

            for (int i = 0; i < arrayObj->get_num_elements(); i++) {
                ArlPDFObject* item = arrayObj->get_value(i);
                if (item != nullptr)
                    if ((first_wildcard == 0) && (data_list[0][TSV_KEYNAME] == "*")) {
                        // All array elements will match wildcard
                        check_basics(item, 0, data_list, elem.link);
                        if (data_list[0][TSV_LINK] != "") {
                            std::string t = remove_type_predicates(data_list[0][TSV_TYPE]);
                            std::string lnk = get_linkset_for_object_type(item, t, data_list[0][TSV_LINK]);
                            std::string as = "[" + std::to_string(i) + "]";
                            std::string direct_link = get_link_for_object(item, lnk, as);
                            add_parse_object(item, direct_link, elem.context + as);
                        }
                    }
                    else if ((first_wildcard > i) && (i < (int)data_list.size())) {
                        // No wildcards to this array element
                        assert(data_list[i][TSV_KEYNAME] == std::to_string(i));
                        check_basics(item, i, data_list, elem.link);
                        if (data_list[i][TSV_LINK] != "") {
                            std::string t = remove_type_predicates(data_list[i][TSV_TYPE]);
                            std::string lnk = get_linkset_for_object_type(item, t, data_list[i][TSV_LINK]);
                            std::string as = "[" + std::to_string(i) + "]";
                            std::string direct_link = get_link_for_object(item, lnk, as);
                            add_parse_object(item, direct_link, elem.context + as);
                        }
                    }
                    /// @todo  Support array wildcards fully (<integer>*)
            } // for-each array element

            if ((first_notreqd == ALL_ARRAY_ELEMS) && (first_wildcard == ALL_ARRAY_ELEMS) && (data_list.size() != arrayObj->get_num_elements())) {
                output << "Error: array length incorrect for " << elem.link;
                if (!terse)
                    output << ", wanted " << data_list.size() << ", got " << arrayObj->get_num_elements() << " (" << *arrayObj << ")";
                output << std::endl;
            }

            if ((first_notreqd != ALL_ARRAY_ELEMS) && (first_notreqd > arrayObj->get_num_elements())) {
                output << "Error: array " << elem.link << " requires " << first_notreqd << " elements, but only had " << arrayObj->get_num_elements();
                if (!terse)
                    output << " (" << *arrayObj << ")";
                output << std::endl;
            }
        }
        else {
            output << "Error: unexpected object type " << PDFObjectType_strings[(int)obj_type] << " for " << elem.link;
            if (!terse)
                output << " (" << *elem.object << ")";
            output << std::endl;
        }
    } // while queue not empty
}
