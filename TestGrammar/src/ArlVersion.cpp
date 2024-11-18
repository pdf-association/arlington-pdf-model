///////////////////////////////////////////////////////////////////////////////
/// @file
/// @brief ArlVersion class definition
///
/// @copyright
/// Copyright 2022 PDF Association, Inc. https://www.pdfa.org
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
/// @author Peter Wyatt, PDF Association
///
///////////////////////////////////////////////////////////////////////////////

#include "ArlVersion.h"
#include "ArlingtonTSVGrammarFile.h"
#include "ArlPredicates.h"
#include "LRParsePredicate.h"
#include "PredicateProcessor.h"
#include "utils.h"
#include "PDFFile.h"

#include <memory>
#include <algorithm>
#include <cassert>
#include <regex>

using namespace ArlingtonPDFShim;

/// @brief "SinceVersion" field extension predicate regex (version-less)
/// - m[1] = name of extension
const std::regex  r_ExtensionOnly("^fn:Extension\\((" + ArlKeyBase + ")\\)");


/// @brief "SinceVersion" field version-based extension predicate regex
/// - m[1] = name of extension
/// - m[2] = PDF version
const std::regex  r_ExtensionVersion("^fn:Extension\\((" + ArlKeyBase + ")\\,(" + ArlPDFVersion + ")\\)");


/// @brief "SinceVersion" field version-based extension predicate regex
/// - m[1] = name of extension
/// - m[2] = PDF version for extension
/// - m[3] = PDF version without extension
const std::regex  r_EvalExtensionVersion("^fn:Eval\\(fn:Extension\\((" + ArlKeyBase + ")\\," + ArlPDFVersion + "\\) \\|\\| " + ArlPDFVersion + "\\)");


bool object_matches_Arlington(ArlingtonPDFShim::ArlPDFObject* pdfObj, const std::string arlType) {
    // Determine the Arlington equivalent for the PDF Object
    assert(pdfObj != nullptr);
    switch (pdfObj->get_object_type())
    {
    case PDFObjectType::ArlPDFObjTypeNumber:
    {
        ArlPDFNumber* numobj = (ArlPDFNumber*)pdfObj;
        if (numobj->is_integer_value())
            return (arlType == "integer") || (arlType == "bitmask");
        else
            return (arlType == "number");
    }
    break;
    case PDFObjectType::ArlPDFObjTypeBoolean:    return (arlType == "boolean"); break;
    case PDFObjectType::ArlPDFObjTypeName:       return (arlType == "name"); break;
    case PDFObjectType::ArlPDFObjTypeNull:       return (arlType == "null"); break;
    case PDFObjectType::ArlPDFObjTypeStream:     return (arlType == "stream"); break;
    case PDFObjectType::ArlPDFObjTypeString:     return (arlType == "date") || (arlType == "string") || (arlType == "string-ascii") || (arlType == "string-byte") || (arlType == "string-text");; break;
    case PDFObjectType::ArlPDFObjTypeArray:      return (arlType == "array") || (arlType == "rectangle") || (arlType == "matrix"); break;
    case PDFObjectType::ArlPDFObjTypeDictionary: return (arlType == "dictionary") || (arlType == "name-tree") || (arlType == "number-tree"); break; 
    case PDFObjectType::ArlPDFObjTypeReference:
        assert(false && "ArlPDFObjTypeReference for object_matches_Arlington()");
        break;
    default:
        assert(false && "unexpected type for object_matches_Arlington()");
        break;
    }
    return false;
}

/// @brief Constructor to handle version complexities
///
/// @param[in] obj          PDF object
/// @param[in] vec          the row from the Arlington TSV file (including all predicates and complexity ([];[];[]))
/// @param[in] pdf_ver      PDF version multiplied by 10
/// @param[in] extns        a list of extension names to support
ArlVersion::ArlVersion(ArlPDFObject* obj, std::vector<std::string> vec, const int pdf_ver, const std::vector<std::string>& extns)
    : arl_version(0), version_reason(ArlVersionReason::Unknown), arl_type_index(-1)
{
    tsv = vec;
    supported_extensions = extns; // copy all the extensions being supported

    wildcard_extn = false;
    for (auto& e : supported_extensions)
        if (e == "*") {
            wildcard_extn = true;
            break;
        }

    // Determine the Arlington equivalent for the PDF Object
    assert(obj != nullptr);
    switch (obj->get_object_type())
    {
    case PDFObjectType::ArlPDFObjTypeNumber:
    {
        ArlPDFNumber* numobj = (ArlPDFNumber*)obj;
        if (numobj->is_integer_value())
            arl_type_of_pdf_object = "integer";         // or "bitmask"
        else
            arl_type_of_pdf_object = "number";
    }
    break;
    case PDFObjectType::ArlPDFObjTypeBoolean:     arl_type_of_pdf_object = "boolean"; break;
    case PDFObjectType::ArlPDFObjTypeName:        arl_type_of_pdf_object = "name"; break;
    case PDFObjectType::ArlPDFObjTypeNull:        arl_type_of_pdf_object = "null"; break;
    case PDFObjectType::ArlPDFObjTypeStream:      arl_type_of_pdf_object = "stream"; break;     // or "name-tree" or "number-tree"
    case PDFObjectType::ArlPDFObjTypeString:      arl_type_of_pdf_object = "string"; break;     // or "date" or "string-*"...
    case PDFObjectType::ArlPDFObjTypeArray:       arl_type_of_pdf_object = "array"; break;      // or "rectangle" or "matrix"
    case PDFObjectType::ArlPDFObjTypeDictionary:  arl_type_of_pdf_object = "dictionary"; break; // or "name-tree" or "number-tree"
    case PDFObjectType::ArlPDFObjTypeReference:
        assert(false && "ArlPDFObjTypeReference for ArlVersion()");
        arl_type_of_pdf_object = "null";
        break;
    default:
        assert(false && "unexpected type for ArlVersion()");
        arl_type_of_pdf_object = "null";
        break;
    }
    assert(FindInVector(v_ArlAllTypes, arl_type_of_pdf_object));

    // Set the PDF version being tested
    assert((pdf_ver >= 10) && ((pdf_ver <= 17) || (pdf_ver == 20)));
    pdf_version = pdf_ver;

    // Determine the type we will match from Arlington TSV 'Type' field data.
    // The Type field is complex ([];[];[]) and can have version predicates!
    // - try exact match first
    // - if object was integer look for bitmask
    // - if object was array look for rectangle and matrix
    // - name-trees and number-trees support dicts, arrays and streams
    // - if object was string look for date or string-*
    std::string arl_types = vec[TSV_TYPE];
    bool found = false;
    std::vector<std::string>  arl_t = split(arl_types, ';');
    for (int i = 0; i < (int)arl_t.size(); i++) {
        std::string t = arl_t[i];
        if ((t.size() > 0) && (t[0] == '['))
            t = t.substr(1, t.size() - 2);  // strip enclosing "[...]"
        if ((t == "number") && (arl_type_of_pdf_object == "integer")) {
            // Can always use integer in place of a number
            arl_type = arl_type_of_pdf_object = "number";
            version_reason = ArlVersionReason::OK;
            arl_type_index = i;
            found = true;
            break;
        }
        else if (t.find(arl_type_of_pdf_object) != std::string::npos) {
            if (t == arl_type_of_pdf_object) {
                // Found an exact match without any version predicates
                arl_type = arl_type_of_pdf_object;
                version_reason = ArlVersionReason::OK;
                arl_type_index = i;
                found = true;
                break;
            }
            else  // Found an exact match but wrapped in version predicates so fallthrough
                break;
        }
    } // for

    if (!found) {
        std::smatch     m;
        for (int i = 0; i < (int)arl_t.size() && !found; i++) {
            std::string t = arl_t[i];
            if (t.find("fn:") != std::string::npos) {
                // Predicate - remove...
                if (std::regex_search(t, m, r_Types) && m.ready() && (m.size() == 7)) {
                    // Needs to be synchronised with PredicateProcessor::ReduceTypeElement() / ValidateTypeSyntax()
                    // 
                    // m[1] = predicate function name (no "fn:" or '(')
                    // 
                    // If a version-based predicate:
                    //    m[2] = PDF version "x.y"
                    //    m[3] = Arlington pre-defined type
                    // Else if extension predicate:
                    //    m[5] = extension name 
                    //    m[6] = Arlington pre-defined type
                    if (m[1].str() == "SinceVersion") {
                        arl_version = string_to_pdf_version(m[2].str());
                        if (pdf_version >= arl_version)
                            version_reason = ArlVersionReason::OK;
                        else
                            version_reason = ArlVersionReason::Before_fnSinceVersion;
                        assert(FindInVector(v_ArlAllTypes, m[3].str()));
                        t = m[3];
                    }
                    else if (m[1].str() == "Deprecated") {
                        arl_version = string_to_pdf_version(m[2].str());
                        if (pdf_version >= arl_version)
                            version_reason = ArlVersionReason::Is_fnDeprecated;
                        else
                            version_reason = ArlVersionReason::OK;
                        assert(FindInVector(v_ArlAllTypes, m[3].str()));
                        t = m[3];
                    }
                    else if (m[1].str() == "IsPDFVersion") {
                        arl_version = string_to_pdf_version(m[2].str());
                        if (pdf_version == arl_version)
                            version_reason = ArlVersionReason::OK;
                        else
                            version_reason = ArlVersionReason::Not_fnIsPDFVersion;
                        assert(FindInVector(v_ArlAllTypes, m[3].str()));
                        t = m[3];
                    }
                    else if (m[1].str() == "BeforeVersion") {
                        arl_version = string_to_pdf_version(m[2].str());
                        if (pdf_version < arl_version)
                            version_reason = ArlVersionReason::OK;
                        else
                            version_reason = ArlVersionReason::After_fnBeforeVersion;
                        assert(FindInVector(v_ArlAllTypes, m[3].str()));
                        t = m[3];
                    }
                    else if (m[4].str() == "Extension") {
                        assert(FindInVector(v_ArlAllTypes, m[6].str()));
                        if (wildcard_extn || FindInVector(extns, m[5].str()))
                            t = m[6];
                        // Extensions don't specify a version so fake to match PDF
                        arl_version = pdf_version;
                    }
                }
                else {
                    assert(false && "unexpected predicate in Type field!");
                }
            }

            // 't' should now be cleaned of predicates
            if ((arl_type_of_pdf_object == t) ||
                ((arl_type_of_pdf_object == "integer")    && (t == "bitmask")) ||
                ((arl_type_of_pdf_object == "array")      && (t =="rectangle")) ||
                ((arl_type_of_pdf_object == "array")      && (t == "matrix")) ||
                ((arl_type_of_pdf_object == "dictionary") && (t == "name-tree")) ||
                ((arl_type_of_pdf_object == "stream")     && (t == "name-tree")) ||
                ((arl_type_of_pdf_object == "array")      && (t == "name-tree")) ||
                ((arl_type_of_pdf_object == "dictionary") && (t == "number-tree")) ||
                ((arl_type_of_pdf_object == "stream")     && (t == "number-tree")) ||
                ((arl_type_of_pdf_object == "array")      && (t == "number-tree")) ||
                ((arl_type_of_pdf_object == "string")     && (t == "date")) ||
                ((arl_type_of_pdf_object == "string")     && (t.find("string-") != std::string::npos))) {
                arl_type_index = i;
                arl_type = t;
                found = true;
                if (version_reason == ArlVersionReason::Unknown)
                    version_reason = ArlVersionReason::OK;
            }
        } // for
    } // if !found

    // Override predicates with SinceVersion and DeprecatedIn fields
    int since_ver = 0;
    if (FindInVector(v_ArlPDFVersions, vec[TSV_SINCEVERSION])) {
        // Simple PDF version
        since_ver = string_to_pdf_version(vec[TSV_SINCEVERSION]);
        if (found && (pdf_version < since_ver)) {
            arl_version = since_ver;
            version_reason = ArlVersionReason::Before_fnSinceVersion;
        }
    }
    else {
        // Predicate-based "SinceVersion" field with fn:Extension(...), fn:Extension(...,x.y) or a fn:Eval which evaluates
        // to a PDF version 
        assert(vec[TSV_SINCEVERSION].find("fn:") != std::string::npos);

        std::smatch       m;
        if (std::regex_search(vec[TSV_SINCEVERSION], m, r_ExtensionVersion) && m.ready() && (m.size() >= 3)) {
            // - m[1] = name of extension
            // - m[2] = PDF version
            int tsv_ver = string_to_pdf_version(m[2].str());
            if (FindInVector(extns, m[1].str()) && (pdf_ver >= tsv_ver))
                since_ver = tsv_ver;
        }
        else if (std::regex_search(vec[TSV_SINCEVERSION], m, r_ExtensionOnly) && m.ready() && (m.size() == 2)) {
            // m[1] = extension name
            if (FindInVector(extns, m[1].str()))
                since_ver = pdf_ver;
        }
        else if (std::regex_search(vec[TSV_SINCEVERSION], m, r_EvalExtensionVersion) && m.ready() && (m.size() == 4)) {
            /// - m[1] = name of extension
            /// - m[2] = PDF version for extension
            /// - m[3] = PDF version without extension
            int tsv_ver1 = string_to_pdf_version(m[2].str());
            int tsv_ver2 = string_to_pdf_version(m[3].str());
            if (FindInVector(extns, m[1].str()) && (pdf_ver >= tsv_ver1))
                since_ver = tsv_ver1;
            else
                since_ver = tsv_ver2;
        }
        else {
            assert(false && "unexpected SinceVersion predicate!");
        }
    }

    if (found && (vec[TSV_DEPRECATEDIN] != "")) {
        int deprecated_ver = string_to_pdf_version(vec[TSV_DEPRECATEDIN]);
        if (pdf_version >= deprecated_ver) {
            arl_version = deprecated_ver;
            version_reason = ArlVersionReason::Is_fnDeprecated;
        }
        if ((deprecated_ver == since_ver) && (pdf_version != deprecated_ver)) {
            arl_version = deprecated_ver;
            version_reason = ArlVersionReason::Not_fnIsPDFVersion;
        }
    }

    // Fix-up / sanity logic due to predicates vs SinceVersion / DeprecatedIn fields
    if (found && (version_reason == ArlVersionReason::Is_fnDeprecated) && (arl_version > pdf_version)) {
        found = true;
        version_reason = ArlVersionReason::OK;
    }

    // Regex may have matched and we prematurely set a reason, but still not found
    if (!found)
        version_reason = ArlVersionReason::Unknown;

    assert((found && (arl_type.size() > 0) && (arl_type_index >= 0)) || (!found && (arl_type.size() == 0) && (arl_type_index < 0)));
    assert((found && (version_reason != ArlVersionReason::Unknown)) || (!found && (version_reason == ArlVersionReason::Unknown)));
}



/// @returns true if the current key is an unsupported extension and not part of an official PDF specification.
/// This effectively means that a key will be reported as an undocumented key if this method returns true.
bool  ArlVersion::is_unsupported_extension() {
    if (FindInVector(v_ArlPDFVersions, tsv[TSV_SINCEVERSION])) {
        // Simple PDF version
        return false;
    }
    else {
        // Predicate-based "SinceVersion" field in the forms of:
        // - fn:Eval((fn:Extension(ADBE_Extn3,1.7) && fn:Extension(ISO_19005_3,1.7)) || 2.0)
        // - fn:Eval(fn:Extension(ISO_19005_3,1.7) || 2.0)
        // - fn:Extension(AAPL,1.2)
        // - fn:Extension(AAPL)
        assert(tsv[TSV_SINCEVERSION].find("fn:") != std::string::npos);

        std::smatch       m;
        if (std::regex_search(tsv[TSV_SINCEVERSION], m, r_EvalExtensionVersion) && m.ready() && (m.size() == 4)) {
            /// - m[1] = name of extension
            /// - m[2] = PDF version for extension
            /// - m[3] = PDF version without extension
            int tsv_ver1 = string_to_pdf_version(m[2].str());
            int tsv_ver2 = string_to_pdf_version(m[3].str());
            return !(((FindInVector(supported_extensions, m[1].str()) || wildcard_extn) && (pdf_version >= tsv_ver1)) || (pdf_version >= tsv_ver2));
        }
        else if (std::regex_search(tsv[TSV_SINCEVERSION], m, r_ExtensionVersion) && m.ready() && (m.size() >= 3)) {
            // m[1] = extension name
            // m[2] = PDF version "x.y"
            int tsv_ver = string_to_pdf_version(m[2].str());
            return !((FindInVector(supported_extensions, m[1].str()) || wildcard_extn) && (pdf_version >= tsv_ver));
        }
        else if (std::regex_search(tsv[TSV_SINCEVERSION], m, r_ExtensionOnly) && m.ready() && (m.size() == 2)) {
            // m[1] = extension name
            return !(FindInVector(supported_extensions, m[1].str()) || wildcard_extn);
        }
        else {
            assert(false && "unexpected SinceVersion predicate!");
        }
    }
    return true;
}



/// @brief Return an appropriate reduced Arlington Link set AFTER processing predicates for the current PDF object and PDF version.
/// Thus deprecated links will be processed away based on the PDF version and NOT returned.
///
/// @param[in] arl_links   a raw Arlington 'Links' field, including complex ([];[];[]) and predicates
///
/// @returns a reduced set (vector) of Arlington Links appropriate for the type of PDF object and PDF version.
/// Or empty vector if nothing appropriate.
std::vector<std::string>  ArlVersion::get_appropriate_linkset(std::string arl_links) {
    std::vector<std::string>      retval;

    if ((arl_type_index < 0) || (arl_links == ""))
        return retval; // empty vector of strings (i.e. no Links)

    std::vector<std::string>    links = split(arl_links, ';');
    assert(arl_type_index < (int)links.size());
    std::string                 appropriate_links = links[arl_type_index];
    assert(appropriate_links[0] == '[');
    appropriate_links = appropriate_links.substr(1, appropriate_links.size() - 2); // strip '[' and ']'

    // Special case for performance
    if (appropriate_links.find("fn:") == std::string::npos) {
        // No predicates so split on COMMA and return
        retval = split(appropriate_links, ',');
        return retval;
    }

    std::string s = appropriate_links;

    while (s.size() > 0) {
        if (s.rfind("fn:", 0) == 0) {
            std::smatch     m;

            // next Link starts with "fn:"
            if (std::regex_search(s, m, r_startsWithSinceVersionExtension) && m.ready() && (m.size() == 4)) {
                    // m[1] = PDF version "x.y" --> convert to integer as x*10 + y
                    // m[2] = extension name
                    // m[3] = Arlington link
                    // int arl_v = string_to_pdf_version(m[1].str());
                if (FindInVector(supported_extensions, m[2].str()) || wildcard_extn)
                    retval.push_back(m[3]);     // m[2] = Arlington link
                s = m.suffix();
                if (s[0] == ',')
                    s = s.substr(1);            // skip COMMA
            }
            else if (std::regex_search(s, m, r_startsWithIsPDFVersionExtension) && m.ready() && (m.size() == 4)) {
                // m[1] = PDF version "x.y" --> convert to integer as x*10 + y
                // m[2] = extension name
                // m[3] = Arlington link
                // int arl_v = string_to_pdf_version(m[1].str());
                if (FindInVector(supported_extensions, m[2].str()) || wildcard_extn)
                    retval.push_back(m[3]);     // m[2] = Arlington link
                s = m.suffix();
                if (s[0] == ',')
                    s = s.substr(1);            // skip COMMA
            }
            else if (std::regex_search(s, m, r_startsWithSinceVersion) && m.ready() && (m.size() == 3)) {
                // m[1] = PDF version "x.y" --> convert to integer as x*10 + y
                int arl_v = string_to_pdf_version(m[1].str());
                if (pdf_version >= arl_v)
                    retval.push_back(m[2]);     // m[2] = Arlington link
                s = m.suffix();
                if (s[0] == ',')
                    s = s.substr(1);            // skip COMMA
            }
            else if (std::regex_search(s, m, r_startsWithBeforeVersion) && m.ready() && (m.size() == 3)) {
                // m[1] = PDF version "x.y" --> convert to integer as x*10 + y
                int arl_v = string_to_pdf_version(m[1].str());
                if (pdf_version < arl_v)
                    retval.push_back(m[2]);     // m[2] = Arlington link
                s = m.suffix();
                if (s[0] == ',')
                    s = s.substr(1);            // skip COMMA
            }
            else if (std::regex_search(s, m, r_startsWithIsPDFVersion) && m.ready() && (m.size() == 3)) {
                // m[2] = PDF version "x.y" --> convert to integer as x*10 + y
                int arl_v = string_to_pdf_version(m[1].str());
                if (pdf_version == arl_v)
                    retval.push_back(m[2]);     // m[2] = Arlington link
                s = m.suffix();
                if (s[0] == ',')
                    s = s.substr(1);            // skip COMMA
            }
            else if (std::regex_search(s, m, r_startsWithDeprecated) && m.ready() && (m.size() == 3)) {
                // m[2] = PDF version "x.y" --> convert to integer as x*10 + y
                int arl_v = string_to_pdf_version(m[1].str());
                if (pdf_version < arl_v)
                    retval.push_back(m[2]);     // m[2] = Arlington link
                s = m.suffix();
                if (s[0] == ',')
                    s = s.substr(1);            // skip COMMA
            }
            else if (std::regex_search(s, m, r_startsWithLinkExtension) && m.ready() && (m.size() == 3)) {
                // m[1] = named extension
                // m[2] = link 
                if (FindInVector(supported_extensions, m[1].str()) || wildcard_extn)
                    retval.push_back(m[2]);     // m[2] = Arlington link
                s = m.suffix();
                if (s[0] == ',')
                    s = s.substr(1);            // skip COMMA
            }
            else {
                assert(false && "unexpected predicate in Arlington Links!");
                s.clear();
            }
        }
        else {
            // does NOT start with "fn:"
            // copy link (up to next COMMA) to output
            auto comma = s.find(',');
            std::string l;
            if (comma != std::string::npos) {
                l = s.substr(0, comma);
                s = s.substr(comma + 1);
            }
            else {
                l = s;
                s.clear();
            }
            retval.push_back(l);
        }
    } // while

    return retval;
}


/// @brief Return the full Arlington Link set AFTER blindly removing predicates (i.e. ignore current PDF version).
///
/// @param[in] arl_links   a raw Arlington 'Links' field, including complex ([];[];[]) and predicates
///
/// @returns a simplified but full set (vector) of Arlington Links appropriate for the type of PDF object.
/// Or empty vector if nothing appropriate.
std::vector<std::string>  ArlVersion::get_full_linkset(std::string arl_links) {
    std::vector<std::string>      retval;

    if ((arl_type_index < 0) || (arl_links == ""))
        return retval; // empty vector of strings (i.e. no Links)

    // Brute force removal of all predicates
    std::string s = remove_type_link_predicates(arl_links);

    std::vector<std::string>    links = split(s, ';');
    assert(arl_type_index < (int)links.size());
    std::string                 full_links = links[arl_type_index];
    assert(full_links[0] == '[');
    full_links = full_links.substr(1, full_links.size() - 2); // strip '[' and ']'
    retval = split(full_links, ',');
    return retval;
}
