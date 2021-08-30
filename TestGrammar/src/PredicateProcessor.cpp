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

/// @brief Integer - only optinoal leading negative sign supported
const std::string ArlInt = "(\\-)?[0-9]+";

/// @brief Number (requires at least 1 decimal place either side of decimal point)
const std::string ArlNum = ArlInt + "\\.[0-9]+";

/// @brief Arlington key or array index regex, including path separator "::" and wildcards
/// Examples: SomeKey, 3, parent::SomeKey, SomeKeyA::someKeyB::3, 
const std::string  ArlKey      = "([a-zA-Z0-9_\\.\\-]+\\:\\:)?([a-zA-Z0-9_\\.\\-]+\\:\\:)?[a-zA-Z0-9_\\.\\-\\*]+";
const std::string  ArlKeyValue = "([a-zA-Z0-9_\\.\\-]+\\:\\:)?([a-zA-Z0-9_\\.\\-]+\\:\\:)?@[a-zA-Z0-9_\\.\\-\\*]+";

/// @brief Arlington PDF version regex (1.0, 1.1, ... 1.7, 2.0)
const std::string  ArlPDFVersion = "(1\\.0|1\\.1|1\\.2|1\\.3|1\\.4|1\\.5|1\\.6|1\\.7|2\\.0)";

/// @brief Arlington Type or Link (TSV filename)
const std::string ArlTypeOrLink = "[a-zA-Z0-9_\\-\\.]+";

/// @brief Arlington math comparisons - currently NOT required to have SPACE either side
const std::string ArlMathComp = "(==|!=|>=|<=|>|<)";

/// @brief Arlington math operators - required explicit SPACE either side
const std::string ArlMathOp   = " (mod|\\*|\\+|\\-) ";

/// @brief Arlington logical operators. Require SPACE either side. Also expect bracketed expressions either side or predicate:
/// e.g. ...) || (... or ...) || fn:...  
const std::string ArlLogicalOp = " (&&|\\|\\|) ";
const std::regex  r_LogicalBracketing("(..)(&&|\\|\\|)(..)");

/// @brief Arlington PDF boolean keywords
const std::string ArlBooleans = "(true|false)";

/// @brief Arlington predicate with zero or one parameter
const std::string ArlPredicate0Arg  = "fn:[a-zA-Z14]+\\(\\)";
const std::string ArlPredicate1Arg  = "fn:[a-zA-Z14]+\\(" + ArlKey + "\\)";
const std::string ArlPredicate1ArgV = "fn:[a-zA-Z14]+\\(" + ArlKeyValue + "\\)";

/// @brief Ordered list of regex matches that should reduce well-formed predicates down to nothing (i.e. an empty string)
/// @todo Mathematical expressions are currently NOT supported (+, -, *, /, multi-term expressions, etc).
const std::vector<std::regex> AllPredicateFunctions = {
    // Bracketed expression components
    std::regex("\\(" + ArlKeyValue + ArlMathComp + ArlPredicate0Arg + "\\)"),
    std::regex("\\(" + ArlKeyValue + ArlMathComp + ArlPredicate1Arg + "\\)"),
    std::regex("\\(" + ArlPredicate1Arg + ArlMathComp + ArlPredicate1Arg + "\\)"),
    std::regex("\\(" + ArlPredicate1ArgV + ArlMathComp + ArlInt + "\\)"),
    std::regex("\\(" + ArlPredicate1Arg + ArlMathComp + ArlInt + "\\)"),
    std::regex("\\(" + ArlKeyValue + "==" + ArlBooleans + "\\)"),
    std::regex("\\(" + ArlKeyValue + ArlMathComp + ArlKeyValue + "\\)"),
    std::regex("\\(" + ArlKeyValue + ArlMathComp + ArlKey + "\\)"),
    std::regex("\\(" + ArlKey + ArlMathComp + ArlKeyValue + "\\)"),
    std::regex("\\(" + ArlKeyValue + " mod (90|8)==0\\)"),
    // IsRequired is always outer function and starting for "Required" field
    std::regex("^fn:IsRequired\\(.*\\)"),
    // single PDF version arguments
    std::regex("fn:SinceVersion\\(" + ArlPDFVersion + "\\)"),
    std::regex("fn:IsPDFVersion\\(" + ArlPDFVersion + "\\)"),
    std::regex("fn:BeforeVersion\\(" + ArlPDFVersion + "\\)"),
    std::regex("fn:Deprecated\\(" + ArlPDFVersion + "\\)"),
    // 2 arguments: PDF version and type/link
    //  - Mostly not required as pre-processed via remove_type_predicates()
    std::regex("fn:IsPDFVersion\\(1.0,fn:BitsClear\\(" + ArlInt + "," + ArlInt + "\\)\\)"),
    std::regex("fn:SinceVersion\\(2.0,fn:BitSet\\(" + ArlInt + "\\)\\)"),
    std::regex("fn:SinceVersion\\(" + ArlPDFVersion + ",fn:BitsClear\\(" + ArlInt + "," + ArlInt + "\\)\\)"),
    // Single integer arguments
    std::regex("fn:BitClear\\(" + ArlInt + "\\)"),
    std::regex("fn:BitSet\\(" + ArlInt + "\\)"),
    // 2 integer arguments
    std::regex("fn:BitsClear\\(" + ArlInt + "," + ArlInt + "\\)"),
    std::regex("fn:BitsSet\\(" + ArlInt + "," + ArlInt + "\\)"),
    // Parameterless predicates - RUINS math expressions!
    std::regex(ArlPredicate0Arg),
    // single key / array index arguments - RUINS math expressions!
    std::regex("fn:RectHeight\\(" + ArlKey + "\\)"),
    std::regex("fn:RectWidth\\(" + ArlKey + "\\)"),
    std::regex("fn:StringLength\\(" + ArlKey + "," + ArlKeyValue + ArlMathOp + ArlInt + "\\)"),
    std::regex("fn:StringLength\\(" + ArlKey + "\\)" + ArlMathComp + ArlInt),
    std::regex("fn:ArrayLength\\(" + ArlKey + "\\)" + ArlMathComp + ArlInt),
    std::regex("fn:ArrayLength\\(" + ArlKey + "\\) " + ArlMathComp + " fn:ArrayLength\\(" + ArlKey + "\\)"),
    std::regex("\\(fn:ArrayLength\\(" + ArlKey + "\\) mod 2\\)==0"),
    std::regex("fn:Ignore\\(" + ArlKey + "\\)"),
    std::regex("fn:InMap\\(" + ArlKey + "\\)"),
    std::regex("fn:NotInMap\\(" + ArlKey + "\\)"),
    std::regex("fn:IsPageNumber\\(" + ArlKeyValue + "\\)"),
    std::regex("fn:IsPresent\\(" + ArlKey + "\\)"),
    std::regex("fn:NotPresent\\(" + ArlKey + "\\)"),
    std::regex("fn:MustBeDirect\\(" + ArlKey + "\\)"),
    // More complex...
    std::regex("fn:IsPresent\\(" + ArlKey + "," + ArlKey + "\\)"),
    std::regex("fn:StringLength\\(" + ArlKey + "," + ArlKeyValue + ArlMathOp + ArlKey + "\\)" + ArlMathComp + ArlInt),
    std::regex("fn:Required\\(" + ArlKeyValue + ArlMathComp + ArlKey + "," + ArlKey + "\\)"),
    // unbracketed expression components
    std::regex(ArlKeyValue + "==" + ArlBooleans),
    std::regex(ArlKeyValue + ArlMathComp + ArlKeyValue),
    std::regex(ArlKeyValue + ArlMathComp + ArlKey),
    std::regex(ArlKey + ArlMathComp + ArlKeyValue),
    // Logical operators after math operations stripped away
    std::regex("\\(" + ArlLogicalOp + "\\)"),
    std::regex(ArlLogicalOp),
    // predicates with complex arguments (incl. nested functions) do last as previous regexes should have soaked up everything
    std::regex(ArlPredicate0Arg),
    std::regex("^fn:Ignore"),
    std::regex("^fn:IsMeaningful"),
    std::regex("^fn:IsRequired"),
    std::regex("^fn:NotPresent"),
    std::regex("^fn:Eval")
};


/// @brief  Validates an Arlington predicate by regex-match search & replace-with-nothing removal.
///         VERY INEFFICIENT and VERY SLOW!!
/// 
/// @param[in] fn    the Arlington input containing predicates 
/// @return          true if the predicate is reduced to the empty string, false otherwise
bool ValidationByConsumption(const std::string& tsv_file, const std::string& fn, std::ostream& ofs)
{
    bool ret_val = true;
    bool show_tsv = false;
    std::regex    bad_result("[^a-zA-Z0-9_. \\-\\,\\(\\)]");
    std::vector<std::string>  list = split(fn, ';');

    for (auto& l : list) {
        std::string s = l;
        if ((s[0] == '[') && (s[s.size()-1] == ']'))    // Strip [ and ]
            s = s.substr(1, s.size()-2);

        // Keeps the type/link value so nested other predicates still match
        s = remove_type_predicates(s);

        // Logical expression - expect bracketed expressions either side or predicate:
        /// e.g. ...) || (... or ...) || fn:...
        if ((s.find("&&") != std::string::npos) || (s.find("||") != std::string::npos)) {
            std::smatch  m;
            if (!std::regex_search(s, m, r_LogicalBracketing)) {
                if (!show_tsv) {
                    ofs << "   " << tsv_file << ":" << std::endl;
                        show_tsv = true;
                }
                ofs << "Error: bad logical expression bracketing: '" << l << "'" << std::endl;
            } 
            else {
                for (int i = 1; i < m.size() ; i += 4)
                    if ((m[i] != ") ") || ((m[i+2] != " (") && (m[i+2] != " f"))) {
                        if (!show_tsv) {
                            ofs << "   " << tsv_file << ":" << std::endl;
                            show_tsv = true;
                        }
                        ofs << "Error: incorrect logical expression bracketing: '" << l << "'" << std::endl;
                    }
            }
        }

        for (auto r : AllPredicateFunctions) {
            s = std::regex_replace(s, r, "");
            // Remove any leading COMMA that was potentially between predicates that just got stripped
            if ((s.size() > 0) && (s[0] == ','))
                s = s.substr(1, s.size() - 1);
        }
        if (std::regex_search(s, bad_result)) {
            if (!show_tsv) {
                ofs << "   " << tsv_file << ":" << std::endl;
                show_tsv = true;
            }
            ofs << "\tIn:  '" << l << "'" << std::endl;
            ofs << "\tOut: '" << s << "'" << std::endl;
            ret_val = false;
        }
    }
    return ret_val;
}


/// @brief Validates an Arlington "Key" field (column 1) 
/// - No COMMAs or SEMI-COLONs
/// - any alphanumeric or "." or "-" or "_"
/// - any integer (array index)
/// - wildcard "*" - must be last row (cannot be checked here)
/// - integer + "*" for a repeating set of N array elements - all rows (cannot be checked here)
bool KeyPredicateProcessor::ValidateRowSyntax() {
    std::regex      r_Keys("(\\*|[0-9]+|[0-9]+\\*|[a-zA-Z0-9\\-\\._]+)");
    if (std::regex_search(tsv_field,r_Keys)) 
        return true;
    return false;
}


const std::regex r_Types("fn:(SinceVersion|Deprecated|BeforeVersion|IsPDFVersion)\\(" + ArlPDFVersion + "\\,([a-z\\-]+)\\)");

/// @brief Validates an Arlington "Type" field (column 2) 
/// Arlington types are all lowercase.
///  - fn:SinceVersion(x.y,type)
///  - fn:Deprecated(x.y,type)
///  - fn:BeforeVersion(x.y,type)
///  - fn:IsPDFVersion(x.y,type)
bool TypePredicateProcessor::ValidateRowSyntax() {
    // Nothing to do?
    if (tsv_field.find("fn:") == std::string::npos)
        return true;


    std::vector<std::string> type_list = split(tsv_field, ';');
    for (auto& t : type_list) {
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
            for (auto arlt : CArlingtonTSVGrammarFile::arl_all_types) {
                if (m[3] == arlt) {
                    valid = true;
                    break;
                }
            }
            if (!valid)
                return false;
        }
        else if (t.find("fn:") == std::string::npos) {
            bool valid = false;
            for (auto& arlt : CArlingtonTSVGrammarFile::arl_all_types) {
                if (t == arlt) {
                    valid = true;
                    break;
                }
            }
            if (!valid)
                return false;
        }
        else
            return false;
    } // for    
    return true;
}

/// @brief Reduces an Arlington "Type" field (column 2) based on a PDF version
/// Arlington types are always lowercase
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

    std::string     to_ret = "";

    assert(pdf_version.size() == 3);

    std::vector<std::string> type_list = split(tsv_field, ';');
    for (auto& t : type_list) {
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
                ((m[1] == "Deprecated")    && (pdf_v <  arl_v))) {
                // m[3] = Arlington type
                for (auto& a : CArlingtonTSVGrammarFile::arl_all_types) {
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
        for (auto& v : CArlingtonTSVGrammarFile::arl_pdf_versions)
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
        for (auto& v : CArlingtonTSVGrammarFile::arl_pdf_versions)
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
/// - inner can be very flexible expressions, including logical operators " && " and " || ":
///   . fn:BeforeVersion(x.y), fn:IsPDFVersion(x.y)
///   . fn:IsPresent(key) or fn:NotPresent(key)
///   . @key==value or @key!=value
///   . use of Arlington-PDF-Path key syntax "::", "parent::"
///   . various highly specialized predicates: fn:IsEncryptedWrapper(), fn:NotStandard14Font(), ...
bool RequiredPredicateProcessor::ValidateRowSyntax() {
    if ((tsv_field == "TRUE") || (tsv_field == "FALSE"))
        return true;
    else if ((tsv_field.find(";") != std::string::npos) ||
             (tsv_field.find("[") != std::string::npos) ||
             (tsv_field.find("]") != std::string::npos))
        return false;
    else if ((tsv_field.find("fn:IsRequired(") == 0) && (tsv_field[tsv_field.size()-1] == ')')) {
        std::string expr = tsv_field.substr(14, tsv_field.size()-15);
        //////////////////////////////////////////////////////////////////////////////
        /// @todo
        //////////////////////////////////////////////////////////////////////////////
        return true;
    } 
    return false;
}


/// @brief Reduces an Arlington "Required" field (column 5) for a given PDF version and PDF object 
/// - either TRUE, FALSE or fn:IsRequired(...)
/// - NO SEMI-COLONs or [ ]
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
        std::string expr = tsv_field.substr(14, tsv_field.size() - 15);
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
    for (auto& ir : indirect_list) {
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



const std::regex  r_Links("fn:(SinceVersion|Deprecated|BeforeVersion|IsPDFVersion)\\(" + ArlPDFVersion + "\\,([a-zA-Z0-9_.\\-]+)\\)");

/// @brief Validates an Arlington "Links" field (column 11) 
///  - fn:SinceVersion(x.y,link)
///  - fn:Deprecated(x.y,link)
///  - fn:BeforeVersion(x.y,link)
///  - fn:IsPDFVersion(x.y,link)
bool LinkPredicateProcessor::ValidateRowSyntax() {
    // Nothing to do?
    if (tsv_field.find("fn:") == std::string::npos)
        return true;

    std::vector<std::string> link_list = split(tsv_field, ';');
    for (auto& lnk : link_list) {
        std::smatch     m;
        if (std::regex_search(lnk, m, r_Links) && m.ready() && (m.size() == 4)) {
            // m[1] = predicate function name (no "fn:")

            // m[2] = PDF version "x.y"
            bool valid = false;
            for (auto& v : CArlingtonTSVGrammarFile::arl_pdf_versions) {
                if (m[2] == v) {
                    valid = true;
                    break;
                }
            }
            if (!valid)
                return false;

            // m[3] = Arlington link (TSV filename)
        }
        else if (lnk.find("fn:") != std::string::npos)
            return false;
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
                ((m[1] == "Deprecated")  && (pdf_v <  arl_v))) {
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


//////////////////////////////////////////////////////////////////////////////////////////////
//
//  Predicate implementations
//   - return true if predicate makes sense, false otherwise
//   - 1st argument is the PDF object in question
//   - first set of arguments are function specific
//   - last argument(s) are the return (out) parameters
//
//////////////////////////////////////////////////////////////////////////////////////////////


/// @brief   Check if the value of a key is in a dictionary and matches a given set
/// @param[in] dict     dictionary object
/// @param[in] key      the key name or array index
/// @param[in] values   a set of values to match
/// @return true if the key value matches something in the values set
bool check_key_value(ArlPDFDictionary* dict, const std::wstring& key, const std::vector<std::wstring> values)
{
    assert(dict != nullptr);
    ArlPDFObject *val_obj = dict->get_value(key);

    if (val_obj != nullptr) {
        std::wstring  val;
        switch (val_obj->get_object_type()) {
            case PDFObjectType::ArlPDFObjTypeString:
                val = ((ArlPDFString*)val_obj)->get_value();
                for (auto& i : values)
                    if (val == i)
                        return true;
                break;
            case PDFObjectType::ArlPDFObjTypeName:
                val = ((ArlPDFName *)val_obj)->get_value();
                for (auto& i : values)
                    if (val == i)
                        return true;
                break;
        } // switch
    }
    return false;
}


bool fn_ArrayLength(ArlPDFObject* obj, int *arr_len) {
    assert(obj != nullptr);
    assert(arr_len != nullptr);
    if (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeArray) {
        ArlPDFArray *arr = (ArlPDFArray *)obj;
        *arr_len = arr->get_num_elements();
        return true;
    }
    return false;
}


bool fn_ArraySortAscending(ArlPDFObject* obj, bool *sorted) {
    assert(obj != nullptr);
    assert(sorted != nullptr);
    *sorted = false;

    if (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeArray) {
        ArlPDFArray* arr = (ArlPDFArray*)obj;
        if (arr->get_num_elements() > 0) {
            // Make sure all array elements are a consistent numeric type
            PDFObjectType obj_type = arr->get_value(0)->get_object_type();
            if (obj_type == PDFObjectType::ArlPDFObjTypeNumber) {
                double       last_elem_val = ((ArlPDFNumber *)arr->get_value(0))->get_value();
                double       this_elem_val;
                for (int i = 1; i < arr->get_num_elements(); i++) {
                    obj_type = arr->get_value(i)->get_object_type();
                    if (obj_type == PDFObjectType::ArlPDFObjTypeNumber) {
                        this_elem_val = ((ArlPDFNumber*)arr->get_value(i))->get_value();
                        if (last_elem_val > this_elem_val) 
                            return false; // was not sorted!
                        last_elem_val = this_elem_val;
                    }
                    else
                        return false; // inconsistent array element types
                }
                *sorted = true;
                return true;
            }
            else
               return false; // not a numeric array 
        }
        else {
            *sorted = true;
            return true; // empty array is always sorted by definition
        }
    }
    return false; // wasn't an array
}


bool fn_BitClear(ArlPDFObject* obj, int bit, bool *bit_was_clear) {
    assert(obj != nullptr);
    assert((bit >= 1) && (bit <= 32));
    assert(bit_was_clear != nullptr);

    *bit_was_clear = false;
    PDFObjectType obj_type = obj->get_object_type();
    if (obj_type == PDFObjectType::ArlPDFObjTypeNumber) {
        ArlPDFNumber *num_obj = (ArlPDFNumber *)obj;
        if (num_obj->is_integer_value()) {
            int bitmask = 1 << (bit - 1);
            int val = num_obj->get_integer_value();
            *bit_was_clear = ((val & bitmask) == 0);
            return true;
        }
        else
            return false;  // wasn't an integer
    }
    else
        return false; // wasn't a number
}


bool fn_BitSet(ArlPDFObject* obj, int bit, bool* bit_was_set) {
    assert(obj != nullptr);
    assert((bit >= 1) && (bit <= 32));
    assert(bit_was_set != nullptr);

    *bit_was_set = false;
    PDFObjectType obj_type = obj->get_object_type();
    if (obj_type == PDFObjectType::ArlPDFObjTypeNumber) {
        ArlPDFNumber* num_obj = (ArlPDFNumber *)obj;
        if (num_obj->is_integer_value()) {
            int bitmask = 1 << (bit - 1);
            int val = num_obj->get_integer_value();
            *bit_was_set = ((val & bitmask) == 1);
            return true;
        }
        else
            return false;  // wasn't an integer
    }
    else
        return false; // wasn't a number
}


bool fn_BitsClear(ArlPDFObject* obj, int low_bit, int high_bit, bool *all_bits_clear) {
    assert(obj != nullptr);
    assert((low_bit >= 1) && (low_bit <= 32));
    assert((high_bit >= 1) && (high_bit <= 32));
    assert(low_bit < high_bit);

    *all_bits_clear = false;
    PDFObjectType obj_type = obj->get_object_type();
    if (obj_type == PDFObjectType::ArlPDFObjTypeNumber) {
        ArlPDFNumber* num_obj = (ArlPDFNumber*)obj;
        if (num_obj->is_integer_value()) {
            int val = num_obj->get_integer_value();
            for (int bit = low_bit; bit <= high_bit; bit ++) {
                int bitmask = 1 << (bit - 1);
                *all_bits_clear = *all_bits_clear && ((val & bitmask) == 0);
            }
            return true;
        }
        else
            return false;  // wasn't an integer
    }
    else
        return false; // wasn't a number
}


bool fn_BitsSet(ArlPDFObject* obj, int low_bit, int high_bit, bool* all_bits_set) {
    assert(obj != nullptr);
    assert((low_bit >= 1) && (low_bit <= 32));
    assert((high_bit >= 1) && (high_bit <= 32));
    assert(low_bit < high_bit);

    *all_bits_set = false;
    PDFObjectType obj_type = obj->get_object_type();
    if (obj_type == PDFObjectType::ArlPDFObjTypeNumber) {
        ArlPDFNumber* num_obj = (ArlPDFNumber*)obj;
        if (num_obj->is_integer_value()) {
            int val = num_obj->get_integer_value();
            for (int bit = low_bit; bit <= high_bit; bit++) {
                int bitmask = 1 << (bit - 1);
                *all_bits_set = *all_bits_set && ((val & bitmask) == 1);
            }
            return true;
        }
        else
            return false;  // wasn't an integer
    }
    else
        return false; // wasn't a number
}


bool fn_CreatedFromNamePageObj(ArlPDFObject* obj) {
    assert(obj != nullptr);
    return false; /// @todo
}


bool fn_Eval(ArlPDFObject* obj) {
    assert(obj != nullptr);
    return false; /// @todo
}

bool fn_FileSize(size_t limit) {
    assert(limit > 0);
    return false; /// @todo
}


bool fn_FontHasLatinChars(ArlPDFObject* obj) {
    assert(obj != nullptr);
    return false; /// @todo
}


bool fn_GetPageNumber(ArlPDFObject* obj) {
    assert(obj != nullptr);
    return false; /// @todo
}


bool fn_Ignore() {
    return true;
}


bool fn_ImageIsStructContentItem(ArlPDFObject* obj) {
    return false; /// @todo
}


bool fn_ImplementationDependent() {
    return true;
}


bool fn_InMap(ArlPDFObject* obj) {
    assert(obj != nullptr);
    /// @todo
    return false; /// @todo
}


bool fn_IsAssociatedFile(ArlPDFObject* obj) {
    assert(obj != nullptr);
    /// @todo Need to see if obj is in trailer::Catalog::AF (array of File Specification dicionaries)
    return false; /// @todo
}


bool fn_IsEncryptedWrapper(ArlPDFObject* obj) {
    assert(obj != nullptr);
    return false; /// @todo
}


bool fn_IsLastInNumberFormatArray(ArlPDFObject* obj) {
    assert(obj != nullptr);
    return false; /// @todo
}


bool fn_IsMeaningful(ArlPDFObject* obj) {
    assert(obj != nullptr);
    return false; /// @todo
}


bool fn_IsPDFTagged(ArlPDFObject* obj) {
    assert(obj != nullptr);
    /// @todo Need to see trailer::Catalog::StructTreeRoot exists
    return false; /// @todo
}


bool fn_IsPageNumber(ArlPDFObject* obj) {
    assert(obj != nullptr);
    return false; /// @todo
}


bool fn_IsPresent(ArlPDFObject* obj, std::string& key, bool *is_present) {
    assert(obj != nullptr);
    assert(is_present != nullptr);

    *is_present = false;
    switch (obj->get_object_type()) {
        case PDFObjectType::ArlPDFObjTypeArray: {
            ArlPDFArray* arr = (ArlPDFArray*)obj;
            try {
                int idx = std::stoi(key);
                *is_present = (arr->get_value(idx) != nullptr);
                return true;
            }
            catch (...) {
                return false; // key wasn't a number
            }
            }
        case PDFObjectType::ArlPDFObjTypeDictionary: {
            ArlPDFDictionary *dict = (ArlPDFDictionary*)obj;
            *is_present = (dict->get_value(ToWString(key)) != nullptr);
            return true;
            }
    } // switch
    return false; 
}


bool fn_KeyNameIsColorant(std::wstring& key, std::vector<std::wstring>& colorants) {
    for (auto& k : colorants)
        if (k == key)
            return true;
    return false; 
}


bool fn_MustBeDirect(ArlPDFObject* obj) {
    assert(obj != nullptr);
    return !obj->is_indirect_ref();
}


bool fn_NoCycle(ArlPDFObject* obj, std::string key) {
    assert(obj != nullptr);
    /// @todo Starting at obj, recursively look at key to ensure there is no loop
    return false; /// @todo
}


bool fn_NotInMap(ArlPDFObject* obj, std::string& pdf_path) {
    assert(obj != nullptr);
    /// @todo Look up map and then look up a reference to this obj
    return false; /// @todo
}


bool fn_NotPresent(ArlPDFObject* obj, std::string& key, bool* is_not_present) {
    bool ret_val = fn_IsPresent(obj, key, is_not_present);
    *is_not_present = !(*is_not_present);
    return ret_val;
}


/// @brief PDF Standard 14 font names
static const std::vector<std::wstring> Std14Fonts = { 
    L"Times-Roman", 
    L"Helvetica", 
    L"Courier", 
    L"Symbol",  
    L"Times-Bold", 
    L"Helvetica-Bold", 
    L"Courier-Bold", 
    L"ZapfDingbats", 
    L"Times-Italic",
    L"Helvetica-Oblique", 
    L"Courier-Oblique", 
    L"Times-BoldItalic", 
    L"Helvetica-BoldOblique", 
    L"Courier-BoldOblique" 
};

bool fn_NotStandard14Font(ArlPDFObject* parent, bool *not_std14font) {
    assert(parent != nullptr);
    assert(not_std14font != nullptr);

    *not_std14font = false;
    if (parent->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary) {
        ArlPDFDictionary *dict = (ArlPDFDictionary*)parent;
        if (check_key_value(dict, L"Type", {L"Font"}) && check_key_value(dict, L"Subtype", {L"Type1"}) && !check_key_value(dict, L"BaseFont", Std14Fonts)) {
            *not_std14font = true;
            return true;
        }
        else
            return false; // wasn't a Type 1 font dictionary
    }
    return false; 
}


bool fn_NumberOfPages(int *num_pages) {
    assert(num_pages != nullptr);

    *num_pages = -1;
    /// @todo Return the total number of pages in the PDF
    return false; /// @todo
}


// Object is a StructParent integer
bool fn_PageContainsStructContentItems(ArlPDFObject* obj, bool *struct_content_items) {
    assert(obj != nullptr);
    assert(struct_content_items != nullptr);

    *struct_content_items = false;
    if (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeNumber) {
        if (((ArlPDFNumber*)obj)->is_integer_value()) {
            int val = ((ArlPDFNumber*)obj)->get_integer_value();
            if (val >= 0) {
                /// @todo Need to see if this is a valid index into trailer::Catalog::StructTreeRoot::ParentTree (number tree)  
                *struct_content_items = true;
                return true; 
            }
            else
                return false; // cannot have negative value
        }
        else
            return false; // not an integer
    }
    return false;  // not a number object
}


bool fn_RectHeight(ArlPDFObject* obj, double *height) {
    assert(obj != nullptr);
    assert(height != nullptr);

    *height = -1;
    if (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeArray) {
        ArlPDFArray* rect = (ArlPDFArray*)obj;
        if (rect->get_num_elements() == 4) {
            for (int i = 0; i < 4; i++)
                if (rect->get_value(i)->get_object_type() != PDFObjectType::ArlPDFObjTypeNumber)
                    return false; // not all rect array elements were numbers;
            double lly = ((ArlPDFNumber*)rect->get_value(1))->get_value();
            double ury = ((ArlPDFNumber*)rect->get_value(3))->get_value();
            *height = round(fabs(ury - lly));
            return true;
        }
        else
            return false; // not a 4 element array
    }
    return false; // not an array
}


bool fn_RectWidth(ArlPDFObject* obj, double *width) {
    assert(obj != nullptr);
    assert(width != nullptr);

    *width = -1;
    if (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeArray) {
        ArlPDFArray *rect = (ArlPDFArray*)obj;
        if (rect->get_num_elements() == 4) {
            for (int i = 0; i < 4; i++)
                if (rect->get_value(i)->get_object_type() != PDFObjectType::ArlPDFObjTypeNumber)
                    return false; // not all rect array elements were numbers;
            double llx = ((ArlPDFNumber *)rect->get_value(0))->get_value();
            double urx = ((ArlPDFNumber *)rect->get_value(2))->get_value();
            *width = round(fabs(urx - llx));
            return true;
        }
        else
            return false; // not a 4 element array
    }
    return false; // not an array
}


bool fn_RequiredValue(ArlPDFObject* obj, std::string& expr, std::string& value, bool *was_req_val) {
    assert(obj != nullptr);
    assert(was_req_val != nullptr);

    *was_req_val = false;
    return false; /// @todo
}


bool fn_StreamLength(ArlPDFObject* obj, size_t *stm_len) {
    assert(obj != nullptr);
    assert(stm_len != nullptr);

    *stm_len = -1;
    if (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeStream) {
        ArlPDFStream* stm_obj = (ArlPDFStream*)obj;
        ArlPDFObject* len_obj = stm_obj->get_dictionary()->get_value(L"Length");
        if (len_obj->get_object_type() == PDFObjectType::ArlPDFObjTypeNumber) {
            ArlPDFNumber* len_num_obj = (ArlPDFNumber*)len_obj;
            if (len_num_obj->is_integer_value()) {
                *stm_len = len_num_obj->get_integer_value();
                return true;
            }
            else
                return false; // stream Length key was a float!
        }
        else
            return false; // stream Length key was not an number
    }
    return false; // not a stream
}


bool fn_StringLength(ArlPDFObject* obj, size_t *str_len) {
    assert(obj != nullptr);
    assert(str_len != nullptr);

    *str_len = -1;
    if (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeString) {
        ArlPDFString *str_obj = (ArlPDFString *)obj;
        std::wstring s = str_obj->get_value();
        *str_len = s.size();
        return true;
    }
    return false; // not a string
}
