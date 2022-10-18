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
#include "LRParsePredicate.h"
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

using namespace ArlingtonPDFShim;
namespace fs = std::filesystem;


/// @def \#define SCORING_DEBUG to see scoring inside recommended_link_for_object()
#undef SCORING_DEBUG


/// @def \#define CHECKS_DEBUG to see details of the checking of values, Possible Values, Special Cases, etc. for each object
#undef CHECKS_DEBUG


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


/// @brief Checks a rectangle or matrix to make sure all elements are numeric.
///
/// @param[in]  arr             any PDF array object
/// @param[in]  elems_to_check  the maximum number of elements to check (e.g. 4 for rectangle)
///
/// @returns true iff the first elems_to_check elements are all numeric
bool CParsePDF::check_numeric_array(ArlPDFArray* arr, const int elems_to_check) {
    bool retval = true;
    int  max_len = arr->get_num_elements();
    for (auto i = 0; i < std::min(elems_to_check, max_len); i++) {
        ArlPDFObject* elem = arr->get_value(i);
        retval = retval && ((elem != nullptr) && (elem->get_object_type() == PDFObjectType::ArlPDFObjTypeNumber));
        delete elem;
    }
    return retval;
}


///@brief  Choose a specific link for a PDF object from a provided set of Arlington links to validate further.
/// Select a link with as many required values with matching "Possible Values" as possible.
/// Sometimes required values are missing, are inherited, etc.
/// Scoring mechanism is used (lower score = better, like golf):
/// Arlington grammar file with the lowest score is our selected link (like golf).
///
/// @param[in]  obj          the PDF object in question
/// @param[in]  links        vector of Arlington 'Links' to try (predicates are SAFE)
/// @param[in]  obj_name     the path of the PDF object in the PDF file
///
/// @returns a single Arlington link that is the best match for the given PDF object. Or "" if no link.
std::string CParsePDF::recommended_link_for_object(ArlPDFObject* obj, const std::vector<std::string> links, const std::string obj_name) {
    assert(obj != nullptr);

    if (links.size() == 0) // Nothing to choose from
        return "";

    if (links.size() == 1)  // Choice of 1
        return links[0];

    auto obj_type = obj->get_object_type();

    int  to_ret = -1;
    int  min_score = 1000;

#if defined(SCORING_DEBUG)
    std::cout << "Deciding for " << *obj << " " << strip_leading_whitespace(obj_name) << " (" << PDFObjectType_strings[(int)obj_type] << ") between ";
    for (auto& l : links)
        std::cout << l << ",";
    std::cout << std::endl;
#endif

    // Checking each Link against obj to see which one is most suitable
    for (auto i = 0; i < (int)links.size(); i++) {
#if defined(SCORING_DEBUG)
        std::cout << "\tScoring " << links[i] << ": ";
#endif
        const ArlTSVmatrix& data_list = get_grammar(links[i]);

        int key_idx = -1;
        auto link_score = 0;
        if ((obj_type == PDFObjectType::ArlPDFObjTypeDictionary) ||
            (obj_type == PDFObjectType::ArlPDFObjTypeStream) ||
            (obj_type == PDFObjectType::ArlPDFObjTypeArray)) {
            int num_keys_matched = 0;
            bool a_required_key_was_bad = false;
            PredicateProcessor pp(pdfc, data_list);
            for (auto& vec : data_list) {
                key_idx++;
                ArlPDFObject* inner_object = nullptr;
                switch (obj_type) {
                    case PDFObjectType::ArlPDFObjTypeArray:
                        {
                            // vec[TSV_KEYNAME] should be an integer
                            int idx = key_to_array_index(vec[TSV_KEYNAME]);
                            if ((idx >= 0) && (idx < ((ArlPDFArray*)obj)->get_num_elements()))
                                inner_object = ((ArlPDFArray*)obj)->get_value(idx);
                        }
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
                        assert(false && "Unexpected object type in recommended_link_for_object()!");
                        break;
                } // switch

                bool reqd_key = false;
                bool deprecated_in_arl = false; 

                // have an inner object match key/array index, check "Possible Values" and compute score
                if (inner_object != nullptr) {
                    num_keys_matched++;
                    std::wstring   str_value;  // inner_object value from PDF as string (not used here)

                    // Get required-ness of key
                    ArlVersion inner_versioner(inner_object, vec, pdf_version, pdfc->get_extensions());
                    reqd_key = pp.IsRequired(obj, inner_object, key_idx, inner_versioner.get_arlington_type_index());

                    // Get deprecation of key
                    deprecated_in_arl = pp.IsDeprecated(key_idx);

                    if (inner_versioner.object_matched_arlington_type()) {
                        bool possible_values_ok = pp.ReducePVRow(obj, inner_object, key_idx, inner_versioner.get_arlington_type_index());

                        if (possible_values_ok) {
#if defined(SCORING_DEBUG)
                            std::cout << (reqd_key ? " Required" : " Optional");
                            std::cout << " key " << vec[TSV_KEYNAME] << " value matched";
                            if (str_value.size() > 0)
                                std::cout << " '" << ToUtf8(str_value) << "'";
                            std::cout << ".";
#endif
                            if ((vec[TSV_KEYNAME] == "Type") || (vec[TSV_KEYNAME] == "Subtype") || (vec[TSV_KEYNAME] == "S") || (vec[TSV_KEYNAME] == "Parent") || (vec[TSV_KEYNAME] == "TransformMethod"))
                                link_score += -80;     // A disambiguating key exists with a correct value
                            else if ((obj_type == PDFObjectType::ArlPDFObjTypeArray) && (vec[TSV_KEYNAME] == "0"))
                                link_score += (reqd_key ? -60 : -20); // Treat first element in an array that is not a wildcard as more important (e.g. disambiguate color spaces)
                            else
                                link_score += (reqd_key ? -10 : -4); // some other key with a correct value
                        }
                        else {
#if defined(SCORING_DEBUG)
                            std::cout << (reqd_key ? " Required" : " Optional");
                            std::cout << " key " << vec[TSV_KEYNAME] << " had wrong value";
                            if (str_value.size() > 0)
                                std::cout << " '" << ToUtf8(str_value) << "'";
                            std::cout << ".";
#endif
                            if ((vec[TSV_KEYNAME] == "Type") || (vec[TSV_KEYNAME] == "Subtype") || (vec[TSV_KEYNAME] == "S"))
                                link_score += +10;  // Type or Subtype key BUT with explicitly wrong value
                            else if ((obj_type == PDFObjectType::ArlPDFObjTypeArray) && (vec[TSV_KEYNAME] == "0"))
                                link_score += +7;   // Treat first element in an array that is not a wildcard as more important (e.g. disambiguate color spaces)
                            else
                                link_score += +5; // some other key but NOT a correct value
                            if (reqd_key)
                                a_required_key_was_bad = true;
                        }

                        if (deprecated_in_arl) // but key is deprecated...
                            link_score += +8;
                    }
                    else {
#if defined(SCORING_DEBUG)
                        std::cout << (reqd_key ? " Required" : " Optional");
                        std::cout << " key " << vec[TSV_KEYNAME] << " was wrong type.";
#endif
                        if ((vec[TSV_KEYNAME] == "Type") || (vec[TSV_KEYNAME] == "Subtype") || (vec[TSV_KEYNAME] == "S"))
                            link_score += +20; // disambiguating key exists with WRONG TYPE
                        else if ((obj_type == PDFObjectType::ArlPDFObjTypeArray) && (vec[TSV_KEYNAME] == "0"))
                            link_score += +3; // Treat first element in an array that is not a wildcard as more important, but WRONG TYPE
                        else
                            link_score += +1; // object type did not match to Arlington
                        if (reqd_key)
                            a_required_key_was_bad = true;
                    }
                    delete inner_object;
                }
                else {
                    if (reqd_key) {
#if defined(SCORING_DEBUG)
                        std::cout << " Required key " << vec[TSV_KEYNAME] << " missing.";
#endif
                        link_score += +12; // required key is missing!
                        a_required_key_was_bad = true;
                    }
                }
            } // for-each key in TSV

            // If all required keys were good then get a bonus weighting to definitions with less keys
            assert(num_keys_matched <= (int)data_list.size());
            if (!a_required_key_was_bad) {
#if defined(SCORING_DEBUG)
                std::cout << " All required keys good!";
#endif
                link_score += -8 * num_keys_matched;
            }
            link_score += (int)(-10.0 * num_keys_matched / data_list.size());

#if defined(SCORING_DEBUG)
           std::cout << " Number of keys matched = " << num_keys_matched << " of " << data_list.size() << ". Score = " << link_score << std::endl;
#endif

            // remembering the lowest score
            if (min_score > link_score) {
                to_ret = i;
                min_score = link_score;
            }
        } // if (dict || stream || array)
    } // for

    // lowest score wins
    if (to_ret >= 0) {
#if defined(SCORING_DEBUG)
        std::cout << "\tOutcome: " << *obj << " as " << links[to_ret] << " with score " << min_score << std::endl;
#endif
        return links[to_ret];
    }

    output << COLOR_ERROR << "can't select any Link to validate PDF object " << strip_leading_whitespace(obj_name) << " as " << PDFObjectType_strings[(int)obj_type];
    if (debug_mode)
        output << " (" << *obj << ")";
    output << COLOR_RESET;
    return "";
}


/// @brief  Recursively looks for 'key' via inheritance (i.e. through "/Parent" keys)
///
/// @param[in] obj
/// @param[in] key        the key to find
/// @param[in] depth      recursive depth (in case of malformed PDFs to stop infinite loops!)
///
/// @returns nullptr if 'key' is NOT located via inheritance, otherwise the PDF object which matches BY KEYNAME!
ArlPDFObject* CParsePDF::find_via_inheritance(ArlPDFDictionary* obj, const std::wstring& key, const int depth) {
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
key_obj = find_via_inheritance(parent_dict, key, depth + 1);
delete parent;
return key_obj;
    }
    return nullptr;
}


/// @brief Validate information of a PDF object (stream, arrray, dictionary) including:
///   - type
///   - indirect-ness
///   - possible value
///  Can output lots of error, warning and info messages!
///
/// @param[in]   parent        parent PDF object (e.g. the dictionary which contains object as a key/value)
/// @param[in]   object        the PDF object to check
/// @param[in]   key_index     >= 0. Row index into TSV data for this PDF object
/// @param[in]   tsv_data      the full Arlington PDF model data read from a TSV
/// @param[in]   grammar_file  the name Arlington PDF model filename used for error messages
/// @param[in]   context       context (PDF DOM path)
/// @param[in]   ofs           open output file stream (or cnull/cwnull for no output)
void CParsePDF::check_everything(ArlPDFObject* parent, ArlPDFObject* object, const int key_index, const ArlTSVmatrix& tsv_data, const std::string& grammar_file, const std::string& context, std::ostream& ofs) {
    assert(parent != nullptr);
    assert(object != nullptr);
    assert(key_index >= 0);
    auto obj_type = object->get_object_type();

    queue_elem fake_e(parent, object, grammar_file, context);

    // Need to cope with wildcard keys "*" or <digit>* for arrays in TSV data as key_index might be beyond rows in tsv_data[]
    int key_idx = key_index;
    if (key_index >= (int)tsv_data.size()) {
        if (tsv_data[tsv_data.size() - 1][TSV_KEYNAME] == "*") // pure wildcard (always last row)
            key_idx = (int)tsv_data.size() - 1;
        else
            key_idx = key_idx % ((int)tsv_data.size() - 1);
        assert((key_idx >= 0) && (key_idx < (int)tsv_data.size()));
    }

    // Process version predicates properly, so if PDF version is BEFORE SinceVersion then will get a wrong type error
    ArlVersion versioner(object, tsv_data[key_idx], pdf_version, pdfc->get_extensions());
    std::vector<std::string>  linkset = versioner.get_appropriate_linkset(tsv_data[key_idx][TSV_LINK]);
    std::string               arl_type = versioner.get_matched_arlington_type();

#ifdef CHECKS_DEBUG
    ofs << "Checking " << grammar_file << "/" << tsv_data[key_idx][TSV_KEYNAME] << " as " << versioner.get_object_arlington_type();
    if (versioner.object_matched_arlington_type())
        ofs << " vs " << arl_type;
    ofs << ": ";
#endif

    // Ignore null as this is the same as nonexistent
    if ((!versioner.object_matched_arlington_type() || (obj_type == PDFObjectType::ArlPDFObjTypeNull))) {
        if (obj_type != PDFObjectType::ArlPDFObjTypeNull) {
            show_context(fake_e);
            ofs << COLOR_ERROR << "wrong type: " << tsv_data[key_idx][TSV_KEYNAME] << " (" << grammar_file << ")";
            ofs << " should be " << tsv_data[key_idx][TSV_TYPE] << " in PDF " << std::fixed << std::setprecision(1) << (pdf_version / 10.0) << " and is " << versioner.get_object_arlington_type();
            if (debug_mode)
                ofs << " (" << *object << ")";
            ofs << COLOR_RESET;
        }
#ifdef CHECKS_DEBUG
        ofs << std::endl;
#endif
        return;
    }

    PredicateProcessor pp(pdfc, tsv_data);
    ReferenceType ir = pp.ReduceIndirectRefRow(parent, object, key_idx, versioner.get_arlington_type_index());

    // Also treat null object as though the key is nonexistent (i.e. don't report an error)
    if ((ir == ReferenceType::MustBeIndirect) && (!object->is_indirect_ref() &&
        (obj_type != PDFObjectType::ArlPDFObjTypeNull) && (obj_type != PDFObjectType::ArlPDFObjTypeReference))) {
        show_context(fake_e);
        ofs << COLOR_ERROR << "not an indirect reference as required: " << tsv_data[key_idx][TSV_KEYNAME] << " (" << grammar_file << ") ";
        ofs << "in PDF " << std::fixed << std::setprecision(1) << (pdf_version / 10.0) << COLOR_RESET;
    }

    // String-ify the value of the PDF object for potential output messages
    std::wstring        str_value;
    double              num_value;
    switch (object->get_object_type())
    {
    case PDFObjectType::ArlPDFObjTypeBoolean:
        if (((ArlPDFBoolean*)object)->get_value())
            str_value = L"true";
        else
            str_value = L"false";
        break;

    case PDFObjectType::ArlPDFObjTypeNumber:
            {
                ArlPDFNumber* numobj = (ArlPDFNumber*)object;
                if (numobj->is_integer_value()) {
                    long long ivalue = numobj->get_integer_value();
                    str_value = std::to_wstring(ivalue);
                    if ((arl_type == "bitmask") && (ivalue > 0xFFFFFFFF)) {
                        show_context(fake_e);
                        ofs << COLOR_WARNING << "bitmask was not a 32-bit value for key " << tsv_data[key_idx][TSV_KEYNAME] << " (" << grammar_file << ")" << COLOR_RESET;
                    }
                    if (((ivalue > 2147483647LL) || (ivalue < -2147483648LL)) && (pdf_version <= 17)) {
                        show_context(fake_e);
                        ofs << COLOR_WARNING << "integer value exceeds PDF 1.x integer range for " << tsv_data[key_idx][TSV_KEYNAME] << " (" << grammar_file << ")" << COLOR_RESET;
                    }
                }
                else {
                    num_value = numobj->get_value();
                    str_value = std::to_wstring(num_value);
                    if (arl_type == "bitmask") {
                        show_context(fake_e);
                        ofs << COLOR_WARNING << "bitmask was not an integer value for key " << tsv_data[key_idx][TSV_KEYNAME] << " (" << grammar_file << ")" << COLOR_RESET;
                    }
                }
            }
            break;

        case PDFObjectType::ArlPDFObjTypeName:
            str_value = ((ArlPDFName*)object)->get_value();
            if ((str_value.size() > 127) && (pdf_version <= 17)) {
                show_context(fake_e);
                ofs << COLOR_WARNING << "PDF 1.x names were limited to 127 bytes (was " << str_value.size() << ") for " << tsv_data[key_idx][TSV_KEYNAME] << " (" << grammar_file << ")" << COLOR_RESET;
            }
            break;

        case PDFObjectType::ArlPDFObjTypeString:
            {
                str_value = ((ArlPDFString*)object)->get_value();
                auto t = pdfc->get_ptr_to_trailer();
                // Warn if string starts with UTF-16LE byte-order-marker - DEPENDS ON PDF SDK!
                if ((str_value.size() >= 2) && (str_value[0] == 255) && (str_value[1] == 254) && !t->is_unsupported_encryption()) {
                    show_context(fake_e);
                    ofs << COLOR_WARNING << "string for key " << tsv_data[key_idx][TSV_KEYNAME] << " (" << grammar_file << ") starts with UTF-16LE byte order marker" << COLOR_RESET;
                }
                // Warn if an ASCII string contains bytes in the unprintable area of ASCII (based on C++ isprint())
                if ((arl_type == "string-ascii") && !t->is_unsupported_encryption()) {
                    bool pure_ascii = true;
                    for (size_t i = 0; i < str_value.size(); i++)
                        pure_ascii = pure_ascii && isprint(str_value[i]);
                    if (!pure_ascii) {
                        show_context(fake_e);
                        ofs << COLOR_WARNING << "ASCII string contained at least one unprintable byte for key " << tsv_data[key_idx][TSV_KEYNAME] << " (" << grammar_file << ")" << COLOR_RESET;
                    }
                }
                // If Arlington says it is a date string then check if PDF string complies
                if ((arl_type == "date") && (!is_valid_pdf_date_string(str_value))) {
                    show_context(fake_e);
                    if (!t->is_unsupported_encryption())
                        ofs << COLOR_ERROR << "invalid date string for key " << tsv_data[key_idx][TSV_KEYNAME] << " (" << grammar_file << "): \"" << ToUtf8(str_value) << "\"" << COLOR_RESET;
                    else
                        ofs << COLOR_WARNING << "possibly invalid date string for key " << tsv_data[key_idx][TSV_KEYNAME] << " (" << grammar_file << ") - unsupported encryption" << COLOR_RESET;
                }
            }
            break;

        case PDFObjectType::ArlPDFObjTypeArray:
            {
                // Arlington has both rectangles and matrices so confirm exact number of elements
                int arr_len = ((ArlPDFArray*)object)->get_num_elements();
                if (arl_type == "rectangle") {
                    if (arr_len != 4) {
                        show_context(fake_e);
                        ofs << COLOR_WARNING << "rectangle does not have exactly 4 elements for key " << tsv_data[key_idx][TSV_KEYNAME] << " (" << grammar_file << ") - had " << arr_len << COLOR_RESET;
                    }
                    if (!check_numeric_array((ArlPDFArray*)object, 4)) {
                        show_context(fake_e);
                        ofs << COLOR_ERROR << "rectangle does not have 4 numeric elements for key " << tsv_data[key_idx][TSV_KEYNAME] << " (" << grammar_file << ")" << COLOR_RESET;
                    }
                }
                if (arl_type == "matrix") {
                    if (arr_len != 6) {
                        show_context(fake_e);
                        ofs << COLOR_WARNING << "matrix does not have exactly 6 elements for key " << tsv_data[key_idx][TSV_KEYNAME] << " (" << grammar_file << ") - had " << arr_len << COLOR_RESET;
                    }
                    if (!check_numeric_array((ArlPDFArray*)object, 6)) {
                        show_context(fake_e);
                        ofs << COLOR_ERROR << "matrix does not have 6 numeric elements for key " << tsv_data[key_idx][TSV_KEYNAME] << " (" << grammar_file << ")" << COLOR_RESET;
                    }
                }
            }
            break;

        default:
            break; // Fallthrough
    } // switch

    bool checks_passed;
    // Check Arlington Special Case field
    checks_passed = pp.ReduceSCRow(parent, object, key_idx, versioner.get_arlington_type_index());
#ifdef CHECKS_DEBUG
    ofs << "SpecialCase = {" << (checks_passed ? "OK" : "not OK") << (pp.WasFullyImplemented() ? "" : ",partial implementation") << (pp.SomethingWasDeprecated() ? ",deprecated" : "") << "} ";
#endif
    if (!checks_passed) {
        show_context(fake_e);
        // If predicates ARE fully processed then we know it is the right or wrong value.
        // If predicates are partially processed then just a warning with additional output
        if (!pp.WasFullyImplemented())
            ofs << COLOR_WARNING << "special case possibly incorrect (some predicates NOT supported): " << tsv_data[key_idx][TSV_KEYNAME] << " (" << grammar_file << ")";
        else
            ofs << COLOR_ERROR << "special case not correct: " << tsv_data[key_idx][TSV_KEYNAME] << " (" << grammar_file << ")";
        ofs << " in PDF " << std::fixed << std::setprecision(1) << (pdf_version / 10.0);
        ofs << " should be: " << tsv_data[key_idx][TSV_TYPE] << " " << tsv_data[key_idx][TSV_SPECIALCASE];
        if (FindInVector(v_ArlNonComplexTypes, versioner.get_object_arlington_type())) {
            auto t = pdfc->get_ptr_to_trailer();
            if ((versioner.get_object_arlington_type().find("string") != std::string::npos) && t->is_unsupported_encryption()) {
                // Don't output encrypted strings
                ofs << " - string when unsupported encryption";
            }
            else {
                ofs << " and is " << versioner.get_object_arlington_type() << "==" << ToUtf8(str_value);
                if (debug_mode)
                    ofs << " (" << *object << ")";
            }
        }
        ofs << COLOR_RESET;
    }

    // Check value against Arlington PossibleValue field
    checks_passed = pp.ReducePVRow(parent, object, key_idx, versioner.get_arlington_type_index());
    
#ifdef CHECKS_DEBUG
    ofs << "PossibleValues = {" << (checks_passed ? "OK" : "not OK") << (pp.WasFullyImplemented() ? "" : ",partial implementation") << (pp.SomethingWasDeprecated() ? ",deprecated" : "") << "} ";
#endif
    if (!checks_passed) {
        show_context(fake_e);
        // If predicates ARE fully processed then we know it is the right or wrong value.
        // If predicates are partially processed then just a warning with additional output
        if (!pp.WasFullyImplemented())
            ofs << COLOR_WARNING << "possibly wrong value for possible values (some predicates NOT supported): " << tsv_data[key_idx][TSV_KEYNAME] << " (" << grammar_file << ")";
        else
            ofs << COLOR_ERROR << "wrong value for possible values: " << tsv_data[key_idx][TSV_KEYNAME] << " (" << grammar_file << ")";
        ofs << " should be: " << tsv_data[key_idx][TSV_TYPE] << " " << tsv_data[key_idx][TSV_POSSIBLEVALUES] << " in PDF " << std::fixed << std::setprecision(1) << (pdf_version / 10.0);
        if (FindInVector(v_ArlNonComplexTypes, versioner.get_object_arlington_type())) {
            auto t = pdfc->get_ptr_to_trailer();
            if ((versioner.get_object_arlington_type().find("string") != std::string::npos) && t->is_unsupported_encryption()) {
                // Don't output encrypted strings
                ofs << " - string when unsupported encryption";
            }
            else {
                ofs << " and is " << versioner.get_object_arlington_type() << "==" << ToUtf8(str_value);
                if (debug_mode)
                    ofs << " (" << *object << ")";
            }
        }
        ofs << COLOR_RESET;
    }
#ifdef CHECKS_DEBUG
    ofs << std::endl;
#endif
}


/// @brief Processes a PDF name tree
///
/// @param[in]     obj          PDF name tree object (dictionary)
/// @param[in]     links        set of Arlington links (predicates are SAFE)
/// @param[in,out] context
/// @param[in]     root         true if the root node of a Name tree
void CParsePDF::parse_name_tree(ArlPDFDictionary* obj, const std::vector<std::string> &links, const std::string context, const bool root) {
    ArlPDFObject *kids_obj   = obj->get_value(L"Kids");
    ArlPDFObject *names_obj  = obj->get_value(L"Names");
    //ArlPDFObject *limits_obj = obj->get_value(L"Limits");

    queue_elem fake_e(nullptr, obj, "name-tree", context);

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
                    std::string  best_link = recommended_link_for_object(obj2, links, as);
                    if (best_link.size() > 0)
                        add_parse_object(obj, obj2, best_link, context + "->[" + as + "]");
                    else
                        delete obj2;

                }
                else {
                    // Error: name tree Names array did not have pairs of entries (obj2 == nullptr)
                    show_context(fake_e);
                    output << COLOR_ERROR << "name tree Names array element #" << i << " - missing 2nd element in a pair for " << strip_leading_whitespace(context) << COLOR_RESET;
                }
            }
            else {
                // Error: 1st in the pair was not OK
                show_context(fake_e);
                if (obj1 == nullptr)
                    output << COLOR_ERROR << "name tree Names array element #" << i << " - 1st element in a pair returned null for " << strip_leading_whitespace(context) << COLOR_RESET;
                else {
                    output << COLOR_ERROR << "name tree Names array element #" << i << " - 1st element in a pair was not a string for " << strip_leading_whitespace(context);
                    if (debug_mode)
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
            show_context(fake_e);
            if (names_obj == nullptr)
                output << COLOR_ERROR << "name tree Names object was missing when Kids was also missing for " << strip_leading_whitespace(context);
            else
                output << COLOR_ERROR << "name tree Names object was not an array when Kids was also missing for " << strip_leading_whitespace(context);
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
                    show_context(fake_e);
                    output << COLOR_ERROR << "name tree Kids array element number #" << i << " was not a dictionary for " << strip_leading_whitespace(context);
                    if (debug_mode && (item != nullptr))
                        output << " (" << *item << ")";
                    output << COLOR_RESET;
                }
                delete item;
            }
        }
        else {
            // error: Kids isn't array in PDF name tree
            show_context(fake_e);
            output << COLOR_ERROR << "name tree Kids object was not an array for " << strip_leading_whitespace(context) << COLOR_RESET;
        }
        delete kids_obj;
    }
}


/// @brief Processes a PDF number tree
///
/// @param[in]     obj          PDF number tree object (dictionary)
/// @param[in]     links        set of Arlington links (Predicates are SAFE!)
/// @param[in,out] context
/// @param[in]     root         true if the root node of a Name tree
void CParsePDF::parse_number_tree(ArlPDFDictionary* obj, const std::vector<std::string>&links, const std::string context, const bool root) {
    ArlPDFObject *kids_obj   = obj->get_value(L"Kids");
    ArlPDFObject *nums_obj   = obj->get_value(L"Nums");
    // ArlPDFObject *limits_obj = obj->get_value(L"Limits");

    queue_elem fake_e(nullptr, obj, "number-tree", context);

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
                            std::string  best_link = recommended_link_for_object(obj2, links, as);
                            if (best_link.size() > 0)
                                add_parse_object(obj, obj2, best_link, context + "->[" + as + "]");
                            else
                                delete obj2;
                        }
                        else {
                            // Error: every even entry in a number tree Nums array are supposed be objects
                            show_context(fake_e);
                            output << COLOR_ERROR << "number tree Nums array element #" << i << " was null for " << strip_leading_whitespace(context) << COLOR_RESET;
                        }
                    }
                    else {
                        // Error: every odd entry in a number tree Nums array are supposed be integers
                        show_context(fake_e);
                        output << COLOR_ERROR << "number tree Nums array element #" << i << " was not an integer for " << strip_leading_whitespace(context);
                        if (debug_mode)
                            output << " (" << *obj1 << ")";
                        output << COLOR_RESET;
                    }
                    delete obj1;
                }
                else {
                    // Error: one of the pair of objects was not OK in PDF number tree
                    show_context(fake_e);
                    output << COLOR_ERROR << "number tree Nums array was invalid for " << strip_leading_whitespace(context) << COLOR_RESET;
                }
            } // for
        }
        else {
            // Error: Nums isn't an array in PDF number tree
            show_context(fake_e);
            output << COLOR_ERROR << "number tree Nums object was not an array for " << strip_leading_whitespace(context) << COLOR_RESET;
        }
        delete nums_obj;
    }
    else {
        // Table 37 Nums: "Root and leaf nodes only; shall be required in leaf nodes;
        //                 present in the root node if and only if Kids is not present
        if (root && (kids_obj == nullptr)) {
            show_context(fake_e);
            output << COLOR_ERROR << "number tree Nums object was missing when Kids was also missing for " << strip_leading_whitespace(context);
            output << COLOR_RESET;
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
                    show_context(fake_e);
                    output << COLOR_ERROR << "number tree Kids array element number #" << i << " was not a dictionary for " << strip_leading_whitespace(context);
                    if (debug_mode && (item != nullptr))
                        output << " (" << *item << ")";
                    output << COLOR_RESET;
                }
                delete item;
            }
        }
        else {
            // Error: Kids isn't array in PDF number tree
            show_context(fake_e);
            output << COLOR_ERROR << "number tree Kids object was not an array for " << strip_leading_whitespace(context);
            if (debug_mode)
                output << " (" << *kids_obj << ")";
            output << COLOR_RESET;
        }
        delete kids_obj;
    }
}


/// @brief Queues a PDF object for processing against an Arlington link, and with a PDF path context
///
/// @param[in]     parent       parent PDF object that contains object (nullptr for root objects)
/// @param[in]     object       PDF object (not nullptr)
/// @param[in]     link         Arlington link (TSV filename)
/// @param[in,out] context      current content (PDF path)
void CParsePDF::add_parse_object(ArlPDFObject* parent, ArlPDFObject* object, const std::string& link, const std::string& context) {
    to_process.emplace(parent, object, link, context);
}



/// @brief Queues a root PDF object for processing against an Arlington link, and with a PDF path context
///
/// @param[in]     object       PDF object (not nullptr)
/// @param[in]     link         Arlington link (TSV filename)
/// @param[in,out] context      current content (PDF path)
void CParsePDF::add_root_parse_object(ArlPDFObject* object, const std::string& link, const std::string& context) {
    to_process.emplace(nullptr, object, link, context);
}


/// @brief prints the context line to console if not already done so
/// 
/// @param[in] e    the element
void CParsePDF::show_context(queue_elem &e) {
    if (!context_shown) {
        output << COLOR_RESET_ANSI << std::setw(8) << counter << ": " << e.context;
        if (debug_mode)
            output << " (" << *e.object << ")";
        output << std::endl;
        context_shown = true;
    }
}



/// @brief Iteratively parse PDF objects from the to_process queue
///
/// @param[in] pdf   reference to the PDF file object
/// 
/// @returns true on success. false on fatal errors (not PDF errors!).
bool CParsePDF::parse_object(CPDFFile &pdf)
{
    pdfc = &pdf;
    std::string ver = pdfc->check_and_get_pdf_version(output); // will produce output messages

    output << COLOR_INFO << "Processing as PDF " << ver;
    auto extns = pdfc->get_extensions();
    if (extns.size() > 0) {
        output << " with extensions ";
        for (size_t i = 0; i < extns.size(); i++)
            output << extns[i] << ((i < (extns.size() - 1)) ? ", " : "");
    }
    output << COLOR_RESET;
    pdf_version = string_to_pdf_version(ver);

    counter = 0;

    while (to_process.size() > 0) {
        context_shown = false;

        queue_elem elem = to_process.front();
        to_process.pop();
        if (elem.link == "") {
            delete elem.object;
            continue;
        }

        // Ensure elem.link is clean of predicates "fn:SinceVersion(x,y,...)"
        assert(elem.link.find("fn:") == std::string::npos);

        // To debug: look at a full DOM tree and then do conditional breakpoints on counter==X
        counter++;
        if (!terse)
            show_context(elem);
        elem.context = "  " + elem.context; // ident for nested DOM display

        assert(elem.object != nullptr);
        if (elem.object->is_indirect_ref()) {
            auto hash = elem.object->get_hash_id();
            auto found = mapped.find(hash);
            if (found != mapped.end()) {
                // "_Universal..." objects match anything so ignore them.
                if ((found->second != elem.link) &&
                    (((elem.link != "_UniversalDictionary") && (elem.link != "_UniversalArray")) &&
                    ((found->second != "_UniversalDictionary") && (found->second != "_UniversalArray")))) {
                    show_context(elem);
                    output << COLOR_WARNING << "object ";
                    if (debug_mode)
                        output << *elem.object << " ";
                    output << "identified in two different contexts. Originally: " << found->second << "; second: " << elem.link << COLOR_RESET;
                }
                delete elem.object;
                continue;
            }
            // remember visited object with a link used for validation
            mapped.insert(std::make_pair(hash, elem.link));
        }

        fs::path  grammar_file = grammar_folder;
        grammar_file /= elem.link + ".tsv";
        const ArlTSVmatrix &tsv = get_grammar(elem.link);
        if (tsv.size() == 0) {
            output << COLOR_ERROR << "could not open " << grammar_file << COLOR_RESET;
            delete elem.object;
            return false;
        }

        // Validating as dictionary:
        // - going through all objects in dictionary
        // - checking basics (Type, PossibleValue, indirect)
        // - then check presence of required keys
        // - then recursively calling validation for each container with link to other grammar file
        auto obj_type = elem.object->get_object_type();

        // Check if object number is out-of-range as per trailer /Size
        // Allow for multiple indirections and thus negative object numbers
        if (abs(elem.object->get_object_number()) >= pdfc->get_trailer_size()) {
            show_context(elem);
            output << COLOR_ERROR << "object number " << abs(elem.object->get_object_number()) << " is illegal. trailer Size is " << pdfc->get_trailer_size() << COLOR_RESET;
        }

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
                std::string  key_utf8 = ToUtf8(key);
                ArlPDFObject* inner_obj = dictObj->get_value(key);
                bool kept_inner_obj = false;

                // might have wrong/malformed object. Key exists, but value does not.
                // NEVER any predicates in the Arlington 'Key' field
                if (inner_obj != nullptr) {
                    // Check if object number is out-of-range as per trailer /Size
                    if (inner_obj->get_object_number() >= pdfc->get_trailer_size()) {
                        show_context(elem);
                        output << COLOR_ERROR << "object number " << inner_obj->get_object_number() << " of key " << key_utf8 << " is illegal. trailer Size is " << pdfc->get_trailer_size() << COLOR_RESET;
                    }

                    bool is_found = false;
                    int key_idx = -1;
                    for (auto& vec : tsv) {
                        key_idx++;
                        /// Degenerate case of a PDF key called "/*" matching the Arlington dictionary wildcard!
                        if ((vec[TSV_KEYNAME] == key_utf8) && (vec[TSV_KEYNAME] != "*")) {
                            is_found = true;
                            check_everything(elem.object, inner_obj, key_idx, tsv, elem.link, elem.context, output);
                            pdf.set_feature_version(vec[TSV_SINCEVERSION], elem.link, key_utf8);

                            // Process version predicates properly (PDF version and object type aware)
                            ArlVersion versioner(inner_obj, vec, pdf_version, pdfc->get_extensions());

                            if (versioner.object_matched_arlington_type()) {
                                std::string arl_type = versioner.get_matched_arlington_type();
                                std::string as = elem.context + "->" + key_utf8;
                                std::vector<std::string>  full_linkset = versioner.get_full_linkset(vec[TSV_LINK]);
                                if (arl_type == "number-tree")
                                    parse_number_tree((ArlPDFDictionary*)inner_obj, full_linkset, as + " (as number-tree)");
                                else if (arl_type == "name-tree")
                                    parse_name_tree((ArlPDFDictionary*)inner_obj, full_linkset, as + " (as name-tree)");
                                else if (FindInVector(v_ArlComplexTypes, arl_type)) {
                                    std::string best_link = recommended_link_for_object(inner_obj, full_linkset, as);
                                    if (best_link.size() > 0) {
                                        if (vec[TSV_KEYNAME] != best_link)
                                            as = as + " (as " + best_link + ")";
                                        add_parse_object(dictObj, inner_obj, best_link, as); // DON'T DELETE inner_obj!
                                        kept_inner_obj = true;
                                    }
                                }
                                else // Arlington primitive type (integer, name, string, etc)
                                    assert(FindInVector(v_ArlNonComplexTypes, arl_type));
                            }
                            else {
                                // PDF object type is not according to Arlington for the exact named key!
                                // Already reported via check_basics() above.
                            }
                            // Report version mis-matches
                            ArlVersionReason reason = versioner.get_version_reason();
                            if ((reason != ArlVersionReason::OK) && (reason != ArlVersionReason::Unknown)) {
                                show_context(elem);
                                bool reason_shown = false;
                                if (reason == ArlVersionReason::After_fnBeforeVersion) {
                                    output << COLOR_INFO << "detected a dictionary key version-based feature after obsolescence in PDF";
                                    reason_shown = true;
                                }
                                else if (reason == ArlVersionReason::Before_fnSinceVersion) {
                                    output << COLOR_INFO << "detected a dictionary key version-based feature before official introduction in PDF ";
                                    reason_shown = true;
                                }
                                else if (reason == ArlVersionReason::Is_fnDeprecated) {
                                    output << COLOR_INFO << "detected a dictionary key version-based feature that was deprecated in PDF ";
                                    reason_shown = true;
                                }
                                else if (reason == ArlVersionReason::Not_fnIsPDFVersion) {
                                    output << COLOR_INFO << "detected a dictionary key version-based feature that was only in PDF ";
                                    reason_shown = true;
                                }
                                if (reason_shown) {
                                    output << std::fixed << std::setprecision(1) << (versioner.get_reason_version() / 10.0) << " (using PDF " << std::fixed << std::setprecision(1) << (pdf_version / 10.0);
                                    output << ") for " << elem.link << "/" << key_utf8 << COLOR_RESET;
                                }
                            }
                            if (versioner.is_unsupported_extension())
                                is_found = false;
                            break;
                        }
                    } // for-each Arlington row

                    // Metadata streams are allowed anywhere since PDF 1.4
                    if ((!is_found) && (key == L"Metadata")) {
                        add_parse_object(dictObj, inner_obj, "Metadata", elem.context + "->Metadata");
                        kept_inner_obj = true;
                        show_context(elem);
                        output << COLOR_INFO << "found a PDF 1.4 Metadata key" << COLOR_RESET;
                        pdf.set_feature_version("1.4", "Metadata", ""); // see clause 14.3
                        is_found = true;
                    }

                    // AF (Associated File) objects are allowed anywhere in PDF 2.0
                    if ((!is_found) && (key == L"AF")) {
                        add_parse_object(dictObj, inner_obj, "FileSpecification", elem.context + "->AF (as FileSpecification)");
                        kept_inner_obj = true;
                        show_context(elem);
                        output << COLOR_INFO << "found a PDF 2.0 Associated File AF key" << COLOR_RESET;
                        pdf.set_feature_version("2.0", "Associated File", "");
                        is_found = true;
                    }

                    // we didn't find the key, there may be wildcard key ("*") that will validate.
                    // Wildcards are always the last row so just check that.
                    if (!is_found) {
                        auto vec = tsv[tsv.size() - 1];
                        if (vec[TSV_KEYNAME] == "*") {
                            pdf.set_feature_version(vec[TSV_SINCEVERSION], elem.link, "dictionary wildcard");
                            // Process version predicates properly (PDF version and object type aware)
                            ArlVersion versioner(inner_obj, vec, pdf_version, pdfc->get_extensions());
                            if (versioner.object_matched_arlington_type()) {
                                std::string as = elem.context + "->" + key_utf8;
                                std::string arl_type = versioner.get_matched_arlington_type();
                                std::vector<std::string>  full_linkset = versioner.get_full_linkset(vec[TSV_LINK]);
                                if (arl_type == "number-tree")
                                    parse_number_tree((ArlPDFDictionary*)inner_obj, full_linkset, as + " (as number-tree)");
                                else if (arl_type == "name-tree")
                                    parse_name_tree((ArlPDFDictionary*)inner_obj, full_linkset, as + " (as name-tree)");
                                else if (FindInVector(v_ArlComplexTypes, arl_type)) {
                                    std::string best_link = recommended_link_for_object(inner_obj, full_linkset, as);
                                    if (best_link.size() > 0) {
                                        as = as + " (as " + best_link + ")";
                                        add_parse_object(dictObj, inner_obj, best_link, as); // DON'T DELETE inner_obj!
                                        kept_inner_obj = true;
                                    }
                                }
                                else // Arlington primitive type (integer, name, number, string, etc).
                                    assert(FindInVector(v_ArlNonComplexTypes, arl_type));
                                is_found = true;
                            }
                            else {
                                // PDF object type is not correct to Arlington for wildcard!
                                show_context(elem);
                                output << COLOR_ERROR << "wrong type for dictionary wildcard for " << elem.link << "/" << ToUtf8(key);
                                output << " in PDF " << std::fixed << std::setprecision(1) << (pdf_version / 10.0) << ": wanted " << vec[TSV_TYPE] << ", PDF was " << versioner.get_object_arlington_type() << COLOR_RESET;
                            }
                            // Report version mis-matches
                            ArlVersionReason reason = versioner.get_version_reason();
                            if ((reason != ArlVersionReason::OK) && (reason != ArlVersionReason::Unknown)) {
                                show_context(elem);
                                bool reason_shown = false;
                                if (reason == ArlVersionReason::After_fnBeforeVersion) {
                                    output << COLOR_INFO << "detected a dictionary wildcard version-based feature after obsolescence in PDF";
                                    reason_shown = true;
                                }
                                else if (reason == ArlVersionReason::Before_fnSinceVersion) {
                                    output << COLOR_INFO << "detected a dictionary wildcard version-based feature before official introduction in PDF ";
                                    reason_shown = true;
                                }
                                else if (reason == ArlVersionReason::Is_fnDeprecated) {
                                    output << COLOR_INFO << "detected a dictionary wildcard version-based feature that was deprecated in PDF ";
                                    reason_shown = true;
                                }
                                else if (reason == ArlVersionReason::Not_fnIsPDFVersion) {
                                    output << COLOR_INFO << "detected a dictionary wildcard version-based feature that was only in PDF ";
                                    reason_shown = true;
                                }
                                if (reason_shown) {
                                    output << std::fixed << std::setprecision(1) << (versioner.get_reason_version() / 10.0) << " (using PDF " << std::fixed << std::setprecision(1) << (pdf_version / 10.0);
                                    output << ") for " << elem.link << "/" << key_utf8 << COLOR_RESET;
                                }
                            }
                        } // last row was a wildcard
                    }

                    // Still didn't find the key - report as an extension
                    if (!is_found) {
                        show_context(elem);
                        if (is_second_class_pdf_name(key_utf8))
                            output << COLOR_INFO << "second class key '" << key_utf8 << "' is not defined in Arlington for ";
                        else if (is_third_class_pdf_name(key_utf8))
                            output << COLOR_INFO << "third class key '" << key_utf8 << "' found in ";
                        else
                            output << COLOR_INFO << "unknown key '" << key_utf8 << "' is not defined in Arlington for ";
                        output << elem.link << " in PDF " << std::fixed << std::setprecision(1) << (pdf_version / 10.0) << COLOR_RESET;
                    }
                }
                else {
                    // inner_objj == nullptr so malformed PDF or parsing limitation in PDF SDK?
                    show_context(elem);
                    output << COLOR_ERROR << "could not get value for key '" << key_utf8 << "' (" << elem.link << ")" << COLOR_RESET;
                }

                if (!kept_inner_obj)
                    delete inner_obj;
            } // for-each key in PDF object

            // Now process Arlington definition of the same PDF object
            PredicateProcessor req_pp(pdfc, tsv);
            int key_idx = -1;
            for (auto& vec : tsv) {
                key_idx++;
                // Check for missing required values in object, and parents if inheritable
                ArlVersion versioner(dictObj, vec, pdf_version, pdfc->get_extensions());
                bool required_key = req_pp.IsRequired(elem.object, dictObj, key_idx, versioner.get_arlington_type_index());

                if (required_key) {
                    assert(vec[TSV_KEYNAME].find('*') == std::string::npos); // wildcards should NEVER be required!
                    ArlPDFObject* inner_obj = dictObj->get_value(ToWString(vec[TSV_KEYNAME]));
                    if (inner_obj == nullptr) {
                        // Arlington 'Inheritable' field NEVER has predicates
                        assert(vec[TSV_INHERITABLE].find("fn:") == std::string::npos);
                        if (vec[TSV_INHERITABLE] == "FALSE") {
                            show_context(elem);
                            if (req_pp.WasFullyImplemented())
                                output << COLOR_ERROR << "non-inheritable required key does not exist: ";
                            else
                                output << COLOR_WARNING << "non-inheritable required key may not exist: ";
                            output << vec[TSV_KEYNAME] << " (" << elem.link << ") in PDF " << std::fixed << std::setprecision(1) << (pdf_version / 10.0);
                            if (debug_mode)
                                output << " (" << *dictObj << ")";
                            if ((vec[TSV_REQUIRED].find("fn:") != std::string::npos) || !req_pp.WasFullyImplemented())
                                output << " because " << vec[TSV_REQUIRED];
                            output << COLOR_RESET;
                        }
                        else {
                            assert(vec[TSV_INHERITABLE] == "TRUE");
                            inner_obj = find_via_inheritance(dictObj, ToWString(vec[TSV_KEYNAME]));
                            if (inner_obj == nullptr) {
                                show_context(elem);
                                if (req_pp.WasFullyImplemented())
                                    output << COLOR_ERROR << "inheritable required key does not exist: ";
                                else
                                    output << COLOR_WARNING << "inheritable required key may not exist: ";
                                output << vec[TSV_KEYNAME] << " (" << elem.link << ") in PDF " << std::fixed << std::setprecision(1) << (pdf_version / 10.0);
                                if (debug_mode)
                                    output << " (" << *dictObj << ")";
                                if ((vec[TSV_REQUIRED].find("fn:") != std::string::npos) || !req_pp.WasFullyImplemented())
                                    output << " because " << vec[TSV_REQUIRED];
                                output << COLOR_RESET;
                            }
                        }
                    }
                    delete inner_obj;
                }
                else if (!req_pp.WasFullyImplemented()) {
                    // Partial support is a warning as don't know if really required or not
                    show_context(elem);
                    output << COLOR_WARNING << "required key may not exist: " << vec[TSV_KEYNAME] << " (" << elem.link << ") in PDF " << std::fixed << std::setprecision(1) << (pdf_version / 10.0);
                    if (debug_mode)
                        output << " (" << *dictObj << ")";
                    output << " because " << vec[TSV_REQUIRED] << COLOR_RESET;
                }
            } // for-each Arlington row

            if (obj_type == PDFObjectType::ArlPDFObjTypeStream)
                delete dictObj; // Only delete for streams
        }
        else if (obj_type == PDFObjectType::ArlPDFObjTypeArray) {
            ArlPDFArray*    arrayObj = (ArlPDFArray*)elem.object;

            // Use null-stream to suppress messages - should have used "--validate" first anyway
            {
                std::vector<std::string> array_index_list;
                for (int i = 0; i < (int)tsv.size(); i++)
                    array_index_list.push_back(tsv[i][TSV_KEYNAME]);

                bool ambiguous;
                if (!check_valid_array_definition(elem.link, array_index_list, cnull, &ambiguous)) {
                    show_context(elem);
                    output << COLOR_ERROR << "PDF array object encountered, but using Arlington dictionary " << elem.link << COLOR_RESET;
                    delete elem.object;
                    continue;
                }
            }

            // Determine first row index that is optional (not Required field == "TRUE")
            int first_optional_idx  = -1;
            for (int i = 0; i < (int)tsv.size(); i++) {
                if (tsv[i][TSV_REQUIRED] != "TRUE") {
                    first_optional_idx = i;
                    break;
                }
            } // for

            // Determine (pure) wildcard status - array repeats handled separately
            int first_pure_wildcard = -1;
            if (tsv[tsv.size() - 1][TSV_KEYNAME] == "*")
                first_pure_wildcard = (int)tsv.size() - 1;

            int array_size = arrayObj->get_num_elements();

            // Are all required rows present?
            if ((first_optional_idx > 0) && (array_size < first_optional_idx)) {
                show_context(elem);
                output << COLOR_ERROR << "minimum required array length incorrect for " << elem.link;
                output << ": wanted " << first_optional_idx << ", got " << array_size;
                if (debug_mode)
                    output << " (" << *arrayObj << ")";
                output << " in PDF " << std::fixed << std::setprecision(1) << (pdf_version / 10.0) << COLOR_RESET;
            }

            // For array repeats, all rows need to be <single-digit> '*' - always starts with "0*" up to "9*"
            // Integer value of last row in TSV indicates the multiple of the length.
            int array_repeat_multiple = -1;
            bool is_array_repeat = false;
            if (tsv[0][TSV_KEYNAME] == "0*") {
                assert(first_pure_wildcard < 0);  // Should not have BOTH wildcard and array repeats
                assert(tsv[tsv.size() - 1][TSV_KEYNAME].size() == 2);
                array_repeat_multiple = (tsv[tsv.size() - 1][TSV_KEYNAME][0] - '0') + 1; // starts at "0*"
                assert((array_repeat_multiple >= 0) && (array_repeat_multiple <= 9));
                assert(tsv[tsv.size() - 1][TSV_KEYNAME][1] == '*');
                is_array_repeat = true;

                // If all rows required then array length must be an exact multiple of the repeat
                if (((array_size % array_repeat_multiple) != 0) && (first_optional_idx == -1)) {
                    show_context(elem);
                    output << COLOR_WARNING << "array length was not an exact multiple of " << array_repeat_multiple << " (was " << array_size << ") for " << elem.link;
                    output << " in PDF " << std::fixed << std::setprecision(1) << (pdf_version / 10.0) << COLOR_RESET;
                }
            }

            int last_idx = -1;
            for (int i = 0; i < array_size; i++) {
                ArlPDFObject* item = arrayObj->get_value(i);
                bool item_kept = false;
                if (item != nullptr) {
                    int idx = i; // TSV index

                    // Check if object number is out-of-range as per trailer /Size
                    // Allow for multiple indirections and thus negative object numbers
                    if (item->get_object_number() >= pdfc->get_trailer_size()) {
                        show_context(elem);
                        output << COLOR_ERROR << "object number " << item->get_object_number() << " of array element " << i << " is illegal. trailer Size is " << pdfc->get_trailer_size() << COLOR_RESET;
                    }

                    // Adjust for array repeats when only SOME rows are required (if last_idx was end of TSV array, then cycle back to start of TSV)
                    if ((array_repeat_multiple > 0) && (first_optional_idx != -1) && (last_idx >= ((int)tsv.size() - 1)))
                        idx = 0;

                    // Adjust for array repeats when all elements are required (so is always an exact multiple)
                    if ((array_repeat_multiple > 0) && (first_optional_idx == -1))
                        idx = idx % array_repeat_multiple;
                    
                    // Adjust for pure wildcards
                    if ((first_pure_wildcard != -1) && (idx > first_pure_wildcard))
                        idx = first_pure_wildcard;

                    assert(idx >= 0);
                    last_idx = idx;

                    // For array repeats when only SOME rows are required (i.e. first_optional_idx != -1), need to decide if PDF object 'item' 
                    // best matches the optional array element or if should cycle back around to match row 0 in TSV. Decide based
                    // on precise PDF object of 'item'.
                    if ((first_optional_idx != -1) && (idx >= first_optional_idx)) {
                        auto itm_type = item->get_object_type();
                        if ((tsv[tsv.size() - 1][TSV_TYPE].find(ArlingtonPDFShim::PDFObjectType_strings[(int)itm_type]) == std::string::npos) &&
                            (tsv[first_optional_idx][TSV_TYPE].find(ArlingtonPDFShim::PDFObjectType_strings[(int)itm_type]) != std::string::npos))
                            idx = 0;
                    }

                    if (idx < (int)tsv.size()) {
                        check_everything(arrayObj, item, idx, tsv, elem.link, elem.context, output);
                        std::string idx_s = "[" + std::to_string(i) + "]";
                        pdf.set_feature_version(tsv[idx][TSV_SINCEVERSION], elem.link, idx_s);
                        // Process version predicates properly (version aware)
                        ArlVersion versioner(item, tsv[idx], pdf_version, pdfc->get_extensions());
                        std::string arl_type = versioner.get_matched_arlington_type();
                        if (FindInVector(v_ArlComplexTypes, arl_type)) {
                            std::string as = elem.context + "[" + std::to_string(i);
                            std::vector<std::string>  full_linkset = versioner.get_full_linkset(tsv[idx][TSV_LINK]);
                            std::string best_link = recommended_link_for_object(item, full_linkset, as + "]");
                            if (best_link.size() > 0) {
                                as = as + " (as " + best_link + ")]";
                                add_parse_object(arrayObj, item, best_link, as);
                                item_kept = true;
                            }
                        }

                        // Report version mis-matches
                        ArlVersionReason reason = versioner.get_version_reason();
                        if ((reason != ArlVersionReason::OK) && (reason != ArlVersionReason::Unknown)) {
                            show_context(elem);
                            bool reason_shown = false;
                            if (reason == ArlVersionReason::After_fnBeforeVersion) {
                                output << COLOR_INFO << "detected an array version-based feature after obsolescence in PDF";
                                reason_shown = true;
                            }
                            else if (reason == ArlVersionReason::Before_fnSinceVersion) {
                                output << COLOR_INFO << "detected an array version-based feature before official introduction in PDF ";
                                reason_shown = true;
                            }
                            else if (reason == ArlVersionReason::Is_fnDeprecated) {
                                output << COLOR_INFO << "detected an array version-based feature that was deprecated in PDF ";
                                reason_shown = true;
                            }
                            else if (reason == ArlVersionReason::Not_fnIsPDFVersion) {
                                output << COLOR_INFO << "detected an array version-based feature that was only in PDF ";
                                reason_shown = true;
                            }
                            if (reason_shown)
                                output << std::fixed << std::setprecision(1) << (versioner.get_reason_version() / 10.0) << " (in PDF " << (pdf_version / 10.0) << ") for " << elem.link << "/" << i << COLOR_RESET;
                        }
                    }
                    else {
                        show_context(elem);
                        output << COLOR_INFO << "array was longer than needed in PDF " << std::fixed << std::setprecision(1) << (pdf_version / 10.0) << " for " << elem.link << "/" << i << COLOR_RESET;
                    }
                }
                if (!item_kept)
                    delete item;
            } // for-each array element
        }
        else {
            show_context(elem);
            output << COLOR_ERROR << "unexpected object type " << PDFObjectType_strings[(int)obj_type] << " for " << elem.link << " in PDF " << std::fixed << std::setprecision(1) << (pdf_version / 10.0) << COLOR_RESET;
        }
        if (elem.object->is_deleteable())
            delete elem.object;
    } // while queue not empty

    // Clean up
    pdfc = nullptr;
    return true;
}
