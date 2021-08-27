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


/// @brief Validates the Arlington "Type" field (column 2) 
///  - fn:SinceVersion(x.y,type)
///  - fn:Deprecated(x.y,type)
///  - fn:BeforeVersion(x.y,type)
///  - fn:IsPDFVersion(x.y,type)
bool TypePredicateProcessor::ValidateSyntax() {
    std::regex      r_Types("fn:(SinceVersion|IsDeprecated|BeforeVersion|IsPDFVersion)\\(([1-9]\\.[0-9])\\,([a-z\\-]+)\\)");

    // Nothing to do?
    if (tsv_field.find("fn:") == std::string::npos)
        return true;

    std::vector<std::string> type_list = split(tsv_field, ';');
    for (auto t : type_list) {
        std::smatch     m;
        if (std::regex_search(tsv_field, m, r_Types) && m.ready() && (m.size() == 4)) {
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

/// @brief Implements predicate support for the Arlington "Type" field (column 2) 
/// - fn:SinceVersion(x.y,type)
/// - fn:Deprecated(x.y,type)
/// - fn:BeforeVersion(x.y,type)
/// - fn:IsPDFVersion(1.0,type)
/// 
/// @param[in]  pdf_version   A PDF version as a string (1.0, 1.1, ... 1.7, 2.0)
/// 
/// @returns a list of Arlington types WITHOUT any predicates after being processed
std::string TypePredicateProcessor::Process(const std::string pdf_version) {
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



/// @brief Validates predicates in the Arlington "IndirectReference" field (column 6) 
/// - fn:MustBeDirect()
/// - fn:MustBeDirect(fn:IsPresent(key))
bool IndirectRefPredicateProcessor::ValidateSyntax() {
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

bool IndirectRefPredicateProcessor::Process(ArlPDFObject* obj) {
    return true; /// @todo
}
