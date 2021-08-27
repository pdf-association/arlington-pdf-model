///////////////////////////////////////////////////////////////////////////////
// PredicateProcessor.cpp
// Copyright 2021 PDF Association, Inc. https://www.pdfa.org
//
// This material is based upon work supported by the Defense Advanced
// Research Projects Agency (DARPA) under Contract No. HR001119C0079.
// Any opinions, findings and conclusions or recommendations expressed
// in this material are those of the author(s) and do not necessarily
// reflect the views of the Defense Advanced Research Projects Agency
// (DARPA). Approved for public release.
//
// SPDX-License-Identifier: Apache-2.0
// Contributors: Peter Wyatt, PDF Association
// 
///////////////////////////////////////////////////////////////////////////////


#include <exception>
#include <iterator>
#include <algorithm>
#include <regex>
#include <string>
#include <cassert>
#include "PredicateProcessor.h"
#include "utils.h"


/// @brief Validates an Arlington "Key" field (column 1) 
/// - No COMMAs or SEMI-COLONs
/// - any alphanumeric or "." or "-" or "_"
/// - any integer (array index)
/// - wildcard "*" - must be last row (cannot be checked here)
/// - integer + "*" for a repeating set of N array elements - all rows (cannot be checked here)
bool KeyPredicateProcessor::ValidateRowSyntax() {
    std::regex      r_Keys("(\\*|[0-9]+|[0-9]+\\*|[a-zA-Z0-9\\-\\._]+)");
    if (std::regex_search(tsv_field,r_Keys)) {
        return true;
    }
    return false;
}


/// @brief Validates an Arlington "Type" field (column 2) 
///  - fn:SinceVersion(x.y,type)
///  - fn:Deprecated(x.y,type)
///  - fn:BeforeVersion(x.y,type)
///  - fn:IsPDFVersion(x.y,type)
bool TypePredicateProcessor::ValidateRowSyntax() {
    // Nothing to do?
    if (tsv_field.find("fn:") == std::string::npos)
        return true;

    std::regex      r_Types("fn:(SinceVersion|IsDeprecated|BeforeVersion|IsPDFVersion)\\(([1-9]\\.[0-9])\\,([a-z\\-]+)\\)");

    std::vector<std::string> type_list = split(tsv_field, ';');
    for (auto t : type_list) {
        std::smatch     m;
        if (std::regex_search(t, m, r_Types) && m.ready() && (m.size() == 4)) {
            // m[1] = predicate function name (no "fn:")

            // m[2] = PDF version "x.y"
            bool valid = false;
            for (auto v : CArlingtonTSVGrammarFile::arl_pdf_versions) {
                if (m[2] == v) {
                    valid = true;
                    break;
                }
            }
            if (!valid)
                return false;

            // m[3] = Arlington type
            valid = false;
            for (auto t : CArlingtonTSVGrammarFile::arl_all_types) {
                if (m[3] == t) {
                    valid = true;
                    break;
                }
            }
            if (!valid)
                return false;
        }
    } // for    
    return true;
}

/// @brief Reduces an Arlington "Type" field (column 2) based on a PDF version
/// - [];[];[]
/// - fn:SinceVersion(x.y,type)
/// - fn:Deprecated(x.y,type)
/// - fn:BeforeVersion(x.y,type)
/// - fn:IsPDFVersion(1.0,type)
/// 
/// @param[in]  pdf_version   A PDF version as a string (1.0, 1.1, ... 1.7, 2.0)
/// 
/// @returns a list of Arlington types WITHOUT any predicates. NEVER an empty string "".
std::string TypePredicateProcessor::ReduceRow(const std::string pdf_version) {
    if (tsv_field.find("fn:") == std::string::npos)
        return tsv_field;

    std::regex      r_Types("fn:(SinceVersion|IsDeprecated|BeforeVersion|IsPDFVersion)\\(([1-9]\\.[0-9])\\,([a-z\\-]+)\\)");
    std::string     to_ret = "";

    assert(pdf_version.size() == 3);

    std::vector<std::string> type_list = split(tsv_field, ';');
    for (auto t : type_list) {
        std::smatch     m;
        if (std::regex_search(t, m, r_Types) && m.ready() && (m.size() == 4)) {
            // m[1] = predicate function name (no "fn:")
            // m[2] = PDF version "x.y" --> convert to integer as x*10 + y
            int pdf_v = pdf_version[0] * 10 + pdf_version[2];
            assert(m[2].str().size() == 3);
            int arl_v = m[2].str()[0] * 10 + m[2].str()[2];
            if (((m[1] == "SinceVersion")  && (pdf_v >= arl_v)) ||
                ((m[1] == "BeforeVersion") && (pdf_v <  arl_v)) ||
                ((m[1] == "IsPDFVersion")  && (pdf_v == arl_v)) ||
                ((m[1] == "IsDeprecated")  && (pdf_v <  arl_v))) {
                // m[3] = Arlington type
                for (auto a : CArlingtonTSVGrammarFile::arl_all_types) {
                    if (m[3] == a) {
                        if (to_ret == "")
                            to_ret = a;
                        else
                            to_ret += ";" + a;
                        break;
                    }
                }
            }
        }
        else {
            // No predicate so just add...
            if (to_ret == "")
                to_ret = t;
            else
                to_ret += ";" + t;
        }
    } // for

    assert(to_ret != "");
    assert(to_ret.find("fn:") == std::string::npos);
    return to_ret;
}


/// @brief Validates an Arlington "SinceVersion" field (column 3) 
/// - only "1.0" or "1.1" or ... or "1.7 or "2.0"
bool SinceVersionPredicateProcessor::ValidateRowSyntax() {
    if (tsv_field.size() == 3) {
        for (auto v : CArlingtonTSVGrammarFile::arl_pdf_versions)
            if (tsv_field == v) 
                return true;
    }
    return false;
}


/// @brief Determines if the current Arlington row is valid based on the "SinceVersion" field (column 3) 
/// 
/// @returns true if this row is valid for the specified by PDF version. false otherwise
bool SinceVersionPredicateProcessor::ReduceRow(const std::string pdf_version) {
    // PDF version "x.y" --> convert to integer as x*10 + y
    assert(tsv_field.size() == 3);
    assert(pdf_version.size() == 3);

    int pdf_v = pdf_version[0] * 10 + pdf_version[2];
    int tsv_v = tsv_field[0]   * 10 + tsv_field[2];
    return (tsv_v <= pdf_v);
};


/// @brief Validates an Arlington "DeprecatedIn" field (column 4) 
/// - only "", "1.0" or "1.1" or ... or "1.7 or "2.0"
bool DeprecatedInPredicateProcessor::ValidateRowSyntax() {
    if (tsv_field == "")
        return true;
    else if (tsv_field.size() == 3) {
        for (auto v : CArlingtonTSVGrammarFile::arl_pdf_versions)
            if (tsv_field == v)
                return true;
    }
    return false;
}


/// @brief Determines if the current Arlington row is valid based on the "DeprecatedIn" field (column 4) 
/// 
/// @returns true if this row is valid for the specified by PDF version. false otherwise
bool DeprecatedInPredicateProcessor::ReduceRow(const std::string pdf_version) {
    if (tsv_field == "")
        return true;

    // PDF version "x.y" --> convert to integer as x*10 + y
    assert(tsv_field.size() == 3);
    assert(pdf_version.size() == 3);

    int pdf_v = pdf_version[0] * 10 + pdf_version[2];
    int tsv_v = tsv_field[0]   * 10 + tsv_field[2];
    return (tsv_v >= pdf_v);
};


/// @brief Validates an Arlington "Required" field (column 5) 
/// - either TRUE, FALSE or fn:IsRequired(...)
/// - inner can be very flexible, including logical " && " and " || " expressions:
///   . fn:BeforeVersion(x.y), fn:IsPDFVersion(x.y)
///   . fn:IsPresent(key) or fn:NotPresent(key)
///   . @key==... or @key!=...
///   . use of Arlington-PDF-Path "::", "parent::"
///   . various highly specialized predicates: fn:IsEncryptedWrapper(), fn:NotStandard14Font(), ...
bool RequiredPredicateProcessor::ValidateRowSyntax() {
    if ((tsv_field == "TRUE") || (tsv_field == "FALSE"))
        return true;
    else if ((tsv_field.find("fn:IsRequired(") == 0) && (tsv_field[tsv_field.size()-1] == ')')) {
        std::string inner = tsv_field.substr(14, tsv_field.size()-15);
        //////////////////////////////////////////////////////////////////////////////
        /// @todo
        //////////////////////////////////////////////////////////////////////////////
        return true;
    }
    return false;
}


/// @brief Reduces an Arlington "Required" field (column 5) for a given PDF version and PDF object 
/// - either TRUE, FALSE or fn:IsRequired(...)
/// - inner can be very flexible, including logical && and || expressions:
///   . fn:BeforeVersion(x.y), fn:IsPDFVersion(x.y)
///   . fn:IsPresent(key) or fn:NotPresent(key)
///   . @key==... or @key!=...
///   . use of Arlington-PDF-Path "::", "parent::"
///   . various highly specialized predicates: fn:IsEncryptedWrapper(), fn:NotStandard14Font(), ...
/// 
/// @returns true if field is required for the given PDF version and PDF object
bool RequiredPredicateProcessor::ReduceRow(const std::string pdf_version, ArlPDFObject* obj) {
    if (tsv_field == "TRUE")
        return true;
    else if (tsv_field == "FALSE")
        return false;
    else {
        //////////////////////////////////////////////////////////////////////////////
        /// @todo
        //////////////////////////////////////////////////////////////////////////////
        return false;
    }
}


/// @brief Validates an Arlington "IndirectReference" field (column 6) 
/// - [];[];[]
/// - fn:MustBeDirect()
/// - fn:MustBeDirect(fn:IsPresent(key))
bool IndirectRefPredicateProcessor::ValidateRowSyntax() {
    // Nothing to do?
    if (tsv_field.find("fn:") == std::string::npos)
        return true;

    std::vector<std::string> indirect_list = split(tsv_field, ';');
    for (auto ir : indirect_list) {
        if ((ir != "TRUE") && (ir != "FALSE") && (ir != "fn:MustBeDirect()") && (ir != "fn:MustBeDirect(fn:IsPresent(Encrypt))"))
            return false;
    } // for    
    return true;
}


/// @brief Validates an Arlington "Inheritable" field (column 7) 
/// - only TRUE or FALSE 
bool InheritablePredicateProcessor::ValidateRowSyntax() {
    if ((tsv_field == "TRUE") || (tsv_field == "FALSE"))
        return true;
    return false;
}


/// @brief Validates an Arlington "Inheritable" field (column 7) 
/// - only TRUE or FALSE 
/// 
/// @returns true if the row is inheritable, false otherwise
bool InheritablePredicateProcessor::ReduceRow() {
    if (tsv_field == "TRUE") 
        return true;
    else 
        return false;
}



/// @brief Validates an Arlington "Links" field (column 11) 
///  - fn:SinceVersion(x.y,link)
///  - fn:Deprecated(x.y,link)
///  - fn:BeforeVersion(x.y,link)
///  - fn:IsPDFVersion(x.y,link)
bool LinkPredicateProcessor::ValidateRowSyntax() {
    // Nothing to do?
    if (tsv_field.find("fn:") == std::string::npos)
        return true;

    std::regex      r_Links("fn:(SinceVersion|IsDeprecated|BeforeVersion|IsPDFVersion)\\(([1-9]\\.[0-9])\\,([a-z_0-9]+)\\)");

    std::vector<std::string> link_list = split(tsv_field, ';');
    for (auto lnk : link_list) {
        std::smatch     m;
        if (std::regex_search(lnk, m, r_Links) && m.ready() && (m.size() == 4)) {
            // m[1] = predicate function name (no "fn:")

            // m[2] = PDF version "x.y"
            bool valid = false;
            for (auto v : CArlingtonTSVGrammarFile::arl_pdf_versions) {
                if (m[2] == v) {
                    valid = true;
                    break;
                }
            }
            if (!valid)
                return false;

            // m[3] = Arlington link (TSV filename)
        }
    } // for    
    return true;
}

/// @brief Reduces an Arlington "Links" field (column 11) based on a PDF version 
///  - fn:SinceVersion(x.y,link)
///  - fn:Deprecated(x.y,link)
///  - fn:BeforeVersion(x.y,link)
///  - fn:IsPDFVersion(x.y,link)
/// 
/// @returns an Arlington Links field with all predicates removed. May be empty string "".
std::string LinkPredicateProcessor::ReduceRow(const std::string pdf_version) {
    std::regex      r_Links("fn:(SinceVersion|IsDeprecated|BeforeVersion|IsPDFVersion)\\(([1-9]\\.[0-9])\\,([a-z_0-9]+)\\)");

    // Nothing to do?
    if (tsv_field.find("fn:") == std::string::npos)
        return tsv_field;

    std::string to_ret = "";
    std::vector<std::string> link_list = split(tsv_field, ';');
    for (auto lnk : link_list) {
        std::smatch     m;
        if (std::regex_search(lnk, m, r_Links) && m.ready() && (m.size() == 4)) {
            // m[1] = predicate function name (no "fn:")
            // m[2] = PDF version "x.y" --> convert to integer as x*10 + y
            int pdf_v = pdf_version[0] * 10 + pdf_version[2];
            assert(m[2].str().size() == 3);
            int arl_v = m[2].str()[0] * 10 + m[2].str()[2];
            if (((m[1] == "SinceVersion")  && (pdf_v >= arl_v)) ||
                ((m[1] == "BeforeVersion") && (pdf_v <  arl_v)) ||
                ((m[1] == "IsPDFVersion")  && (pdf_v == arl_v)) ||
                ((m[1] == "IsDeprecated")  && (pdf_v <  arl_v))) {
                // m[3] = Arlington link
                if (to_ret == "")
                    to_ret = m[3];
                else
                    to_ret += ";" + m[3].str();
            }
        }
        else {
            if (to_ret == "")
                to_ret = lnk;
            else
                to_ret += ";" + lnk;
        }
    } // for    

    assert(to_ret.find("fn:") == std::string::npos);
    return to_ret;
}
