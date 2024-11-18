///////////////////////////////////////////////////////////////////////////////
/// @file 
/// @brief Validates an Arlington PDF model 
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

#include "CheckGrammar.h"
#include "ArlingtonTSVGrammarFile.h"
#include "ArlPredicates.h"
#include "PredicateProcessor.h"
#include "TestGrammarVers.h"
#include "LRParsePredicate.h"
#include "utils.h"

#include <exception>
#include <iterator>
#include <algorithm>
#include <regex>
#include <queue>
#include <map>
#include <cctype>
#include <iostream>
#include <cassert>

namespace fs = std::filesystem;


#ifdef DO_DOXYGEN
/// @def ARL_PARSER_TESTING 
/// @brief tests a small set of hard coded predicates
#define ARL_PARSER_TESTING
#else 
#undef ARL_PARSER_TESTING
#endif

/// @brief When validating, need to know context of TSV (array, dict, stream, name-tree, number-tree)
typedef struct _ValidationContext  {
    std::string     tsv_name;
    std::string     type;
} ValidationContext;


/// @brief For debugging ease, make the root of an entire predicate static
static ASTNode* pred_root = nullptr;


/// @brief Checks to make sure all keys and key-values referenced in an AST are also 
/// keys in the current TSV data object.
/// Does NOT check keys that use Arlington paths "::".
/// Recursive.
/// 
/// @param[in]  tsv_name           name of the Arlington TSV data file
/// @param[in]  tsv                the Arlington TSV data for the single PDF object 'tsv_name`
/// @param[in]  predicate          The predicate as stored in the TSV.
/// @param[in]  ast                a partial AST. Can be nullptr (from recurision).
/// @param[in,out] report_stream   stream to writer validation messages to
///  
void check_keys_in_predicate(const std::string& tsv_name, const ArlTSVmatrix& tsv, const std::string predicate, const ASTNode* ast, std::ostream& report_stream) {
    if (ast == nullptr)
        return;

    // Recursively descend down the AST
    check_keys_in_predicate(tsv_name, tsv, predicate, ast->arg[0], report_stream);
    check_keys_in_predicate(tsv_name, tsv, predicate, ast->arg[1], report_stream);

    std::string key;

    if (ast->type == ASTNodeType::ASTNT_KeyValue) {
        if (ast->node.find("::") != std::string::npos) {
            std::vector<std::string>    keys;
            std::string                 s = ast->node;
            auto sep = s.find("::");
            if (sep != std::string::npos) {
                // Have a multi-part path with "::" separators between keys
                do {
                    keys.push_back(s.substr(0, sep));
                    s = s.substr(sep + 2);
                    sep = s.find("::");
                } while (sep != std::string::npos);
                keys.push_back(s);
            }

            /// @todo not fully supported Arlington paths validation into different objects (e.g. parent:Key)
            report_stream << COLOR_WARNING << "Arlington key-value (@key) with path '::' in " << tsv_name << " " << predicate << " not fully validated" << COLOR_RESET;
            if ((keys[0] == "parent") || (keys[0] == "trailer"))
                return;
            // first key in path is testable - doesn't have '@'
            key = keys[0];
        }
        else
            key = ast->node.substr(1); // strip off '@'
    }
    else if (ast->type == ASTNodeType::ASTNT_Key) {
        if (ast->node.find("::") != std::string::npos) {
            std::vector<std::string>    keys;
            std::string                 s = ast->node;
            auto sep = s.find("::");
            if (sep != std::string::npos) {
                // Have a multi-part path with "::" separators between keys
                do {
                    keys.push_back(s.substr(0, sep));
                    s = s.substr(sep + 2);
                    sep = s.find("::");
                } while (sep != std::string::npos);
                keys.push_back(s);
            }

            /// @todo not fully supported Arlington paths validation into different objects (e.g. parent:Key)
            report_stream << COLOR_WARNING << "Arlington key with path '::' in " << tsv_name << " " << predicate << " not fully validated" << COLOR_RESET;
            if ((keys[0] == "parent") || (keys[0] == "trailer"))
                return;
            // first key in path is testable
            key = keys[0];
        }
        else
            key = ast->node;
    }
    else {
        // Not a key or key-value
        return;
    }

    assert(key.size() > 0);
    assert(key[0] != '@');

    bool found_key = false;
    for (auto& k : tsv)
        if (k[TSV_KEYNAME] == key) {
            found_key = true;
            break;
        }

    if (!found_key) {
        // Check to see if key (ast->node) was the name of an fn:Extension() or a value of an equality/inequality
        if ((predicate.find("fn:Extension(" + ast->node + ",") == std::string::npos) && 
            (predicate.find("==" + ast->node) == std::string::npos) &&
            (predicate.find("!=" + ast->node) == std::string::npos))
            report_stream << COLOR_WARNING << "key " << key << " was not found as a key in " << tsv_name << " in predicate " << predicate << COLOR_RESET;
    }
}


/// @brief  Checks the validity of a single Arlington PDF Model TSV file with knowledge of PDF type:
/// - correct # of columns (TAB separated)
/// - correct headings (first line)
/// - correct basic types 1st column
///
/// @param[in]      reader
/// @param[in]      arl_type       a complex Arlington Type for the TSV
/// @param[in]      verbose        true if verbose debug output is wanted
/// @param[in,out]  report_stream  open output stream to report errors
///
/// @return trues if Arlington TSV file is valid, false if there were any errors
bool check_grammar(CArlingtonTSVGrammarFile& reader, std::string& arl_type, bool verbose, std::ostream& report_stream)
{
    bool                        retval = true;
    auto&                       data_list = reader.get_data();
    std::vector<std::string>    keys_list;
    std::vector<std::string>    vars_list;

    if (verbose)
        report_stream << COLOR_INFO << reader.get_tsv_name() << ":" << COLOR_RESET;

    if (data_list.empty()) {
        report_stream << COLOR_ERROR << "empty Arlington TSV grammar file: " << reader.get_tsv_name() << COLOR_RESET;
        return false;
    }

    if (reader.header_list.size() < 12) {
        report_stream << COLOR_ERROR << "wrong number of columns in TSV file: " << reader.get_tsv_name() << COLOR_RESET;
        return false;
    }

    // Check column headers
    if ((reader.header_list[TSV_KEYNAME]          != "Key") ||
        (reader.header_list[TSV_TYPE]             != "Type") ||
        (reader.header_list[TSV_SINCEVERSION]     != "SinceVersion") ||
        (reader.header_list[TSV_DEPRECATEDIN]     != "DeprecatedIn") ||
        (reader.header_list[TSV_REQUIRED]         != "Required") ||
        (reader.header_list[TSV_INDIRECTREF]      != "IndirectReference") ||
        (reader.header_list[TSV_INHERITABLE]      != "Inheritable") ||
        (reader.header_list[TSV_DEFAULTVALUE]     != "DefaultValue") ||
        (reader.header_list[TSV_POSSIBLEVALUES]   != "PossibleValues") ||
        (reader.header_list[TSV_SPECIALCASE]      != "SpecialCase") ||
        (reader.header_list[TSV_LINK]             != "Link") ||
        (reader.header_list[TSV_NOTES]            != "Note")) {
        report_stream << COLOR_ERROR << "wrong column headers for file: " << reader.get_tsv_name() << COLOR_RESET;
        retval = false;
    }

    bool has_reqd_inheritable = false;
    bool attempt_to_parse_predicates = true;
    int key_idx = -1;
    for (auto& vc : data_list) {
        // Add key of current row to a list to later check for duplicates
        // Arrays can have repeating sets so need to check for <digit>+ASTERISK and remove ASTERISK
        key_idx++;
        if ((vc[TSV_KEYNAME].size() == 2) && isdigit(vc[TSV_KEYNAME][0]) && (vc[TSV_KEYNAME][1] == '*'))
            keys_list.push_back(vc[TSV_KEYNAME].substr(0,1));
        else
            keys_list.push_back(vc[TSV_KEYNAME]);

        for (auto& col : vc) {
            // Check brackets are all balanced
            if (std::count(std::begin(col), std::end(col), '[') != std::count(std::begin(col), std::end(col), ']')) {
                report_stream << COLOR_ERROR << "mismatched number of open '[' and close ']' set brackets '" << col << "' for " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << COLOR_RESET;
                retval = attempt_to_parse_predicates = false;
            }

            if (std::count(std::begin(col), std::end(col), '(') != std::count(std::begin(col), std::end(col), ')')) {
                report_stream << COLOR_ERROR << "mismatched number of open '(' and close ')' brackets '" << col << "' for " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << COLOR_RESET;
                retval = attempt_to_parse_predicates = false;
            }

            // Locate all local variables (\@xxx) to see if they are also keys in this object
            /// @todo Variables in other objects (yyy::\@xxx) are NOT checked
            const std::regex r_LocalKeyValue("[^:]@([a-zA-Z0-9_]+)");
            auto r_begin = std::sregex_iterator(col.begin(), col.end(), r_LocalKeyValue);
            auto r_end   = std::sregex_iterator();

            for (std::sregex_iterator it = r_begin; it != r_end; ++it) {
                std::smatch match = *it;
                if (!FindInVector(vars_list, match[1].str()))
                    vars_list.push_back(match[1].str());
            }

            if (attempt_to_parse_predicates) {
                // Try and parse each predicate after isolating
                std::vector<std::string>  list = split(col, ';');
                for (auto& fn : list)
                    if (fn.find("fn:") != std::string::npos) {
                        std::string s = fn;
                        if ((s[0] == '[') && (s[s.size() - 1] == ']'))    // Strip enclosing [ and ]
                            s = s.substr(1, s.size() - 2);

                        while (!s.empty()) {
                            // Sometimes the comma separated lists have whitespace between terms
                            //if (s[0] == ' ') {
                            //    s = s.substr(1, s.size() - 1);
                            //    assert((s[0] != '&') && (s[0] != '|') && (s[0] != 'm'));  // BAD: need SPACE before &&, ||, mod
                            //}
                            assert(pred_root == nullptr);
                            pred_root = new ASTNode();
                            s = LRParsePredicate(s, pred_root);
                            assert(pred_root->valid());
                            if ((fn.find("fn:Eval(") != std::string::npos) && verbose)
                                check_keys_in_predicate(reader.get_tsv_name(), data_list, fn, pred_root, report_stream);
                            delete pred_root;
                            pred_root = nullptr;
                            if (s.size() > 0)
                                if ((s[0] == ',') || (s[0] == '[') || (s[0] == ']') || (s[0] == ';') || (s[0] == ' '))
                                    s = s.substr(1, s.size() - 1);
                        } // while
                    }
            }
        } // for-each col in a TSV row

        PredicateProcessor validator(nullptr, data_list);
        if (!validator.ValidateKeySyntax(key_idx)) {
            report_stream << COLOR_ERROR << "KeyName field validation error " << reader.get_tsv_name() << " for key " << vc[TSV_KEYNAME] << COLOR_RESET;
            retval = false;
        }

        if (attempt_to_parse_predicates && !validator.ValidateTypeSyntax(key_idx)) {
            report_stream << COLOR_ERROR << "Type field validation error " << reader.get_tsv_name() << " for key " << vc[TSV_TYPE] << COLOR_RESET;
            retval = false;
        }

        if (attempt_to_parse_predicates && !validator.ValidateSinceVersionSyntax(key_idx)) {
            report_stream << COLOR_ERROR << "SinceVersion field validation error " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << ": " << vc[TSV_SINCEVERSION] << COLOR_RESET;
            retval = false;
        }

        if (attempt_to_parse_predicates && !validator.ValidateDeprecatedInSyntax(key_idx)) {
            report_stream << COLOR_ERROR << "DeprecatedIn field validation error " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << ": " << vc[TSV_DEPRECATEDIN] << COLOR_RESET;
            retval = false;
        }

        if (attempt_to_parse_predicates && !validator.ValidateRequiredSyntax(key_idx)) {
            report_stream << COLOR_ERROR << "Required field validation error " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << ": " << vc[TSV_REQUIRED] << COLOR_RESET;
            retval = false;
        }

        if (attempt_to_parse_predicates && !validator.ValidateIndirectRefSyntax(key_idx)) {
            report_stream << COLOR_ERROR << "IndirectRef field validation error " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << ": " << vc[TSV_INDIRECTREF] << COLOR_RESET;
            retval = false;
        }

        if (attempt_to_parse_predicates && !validator.ValidateInheritableSyntax(key_idx)) {
            report_stream << COLOR_ERROR << "Inheritable field validation error " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << ": " << vc[TSV_INHERITABLE] << COLOR_RESET;
            retval = false;
        }

        if ((vc[TSV_INHERITABLE] == "TRUE") && (vc[TSV_REQUIRED] != "FALSE"))
            has_reqd_inheritable = true;

        if (attempt_to_parse_predicates && !validator.ValidateDefaultValueSyntax(key_idx)) {
            report_stream << COLOR_ERROR << "DefaultValue field validation error " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << ": " << vc[TSV_DEFAULTVALUE] << COLOR_RESET;
            retval = false;
        }

        if (attempt_to_parse_predicates && !validator.ValidatePossibleValuesSyntax(key_idx)) {
            report_stream << COLOR_ERROR << "PossibleValues field validation error " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << ": " << vc[TSV_POSSIBLEVALUES] << COLOR_RESET;
            retval = false;
        }

        if (attempt_to_parse_predicates && !validator.ValidateSpecialCaseSyntax(key_idx)) {
            report_stream << COLOR_ERROR << "SpecialCase field validation error " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << ": " << vc[TSV_SPECIALCASE] << COLOR_RESET;
            retval = false;
        }

        if (attempt_to_parse_predicates && !validator.ValidateLinksSyntax(key_idx)) {
            report_stream << COLOR_ERROR << "Link field validation error " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << ": " << vc[TSV_LINK] << COLOR_RESET;
            retval = false;
        }

        // TSV_NOTE

        // CHECK INTER-COLUMN CONSISTENCY
        // Various columns support multiple types by separating with ";" (SEMI-COLON)
        std::vector<std::string>    types = split(vc[TSV_TYPE], ';');
        std::vector<std::string>    links = split(vc[TSV_LINK], ';');
        std::vector<std::string>    default_val = split(vc[TSV_DEFAULTVALUE], ';');
        std::vector<std::string>    possible_vals = split(vc[TSV_POSSIBLEVALUES], ';');
        std::vector<std::string>    specialcase_vals = split(vc[TSV_SPECIALCASE], ';');

        if (vc[TSV_LINK] != "") {
            if (links.size() != types.size()) {
                report_stream << COLOR_ERROR << "wrong # of Types vs. # of links " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << COLOR_RESET;
                retval = false;
            }
            else {
                // Assumes same number of elements in both Types and Links vectors!
                // Check basic types which must NOT have any Links and complex types which REQUIRE Links.
                int j = -1;
                for (auto& type : types) {
                    j++;
                    type = remove_type_link_predicates(type);
                    if (std::find(std::begin(v_ArlNonComplexTypes), std::end(v_ArlNonComplexTypes), type) != std::end(v_ArlNonComplexTypes)) {
                        // type is a simple type - Links NOT expected
                        if (links[j] != "[]")
                            report_stream << COLOR_ERROR << "basic type " << type << " should not be linked in " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << ": " << links[j] << COLOR_RESET;
                    }
                    else if (std::find(std::begin(v_ArlComplexTypes), std::end(v_ArlComplexTypes), type) != std::end(v_ArlComplexTypes)) {
                        // type is a complex type - Links are REQUIRED
                        if (links[j] == "[]")
                            report_stream << COLOR_ERROR << "complex type " << type << " is unlinked in " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << COLOR_RESET;
                    }
                    else {
                        // unexpected type!
                        report_stream << COLOR_ERROR << "unexpected type " << type << " in " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << COLOR_RESET;
                    }
                } // for
            }
        }

        if (vc[TSV_DEFAULTVALUE] != "") {
            if (types.size() != default_val.size()) {
                report_stream << COLOR_ERROR << "wrong # of types vs. # of DefaultValue " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << COLOR_RESET;
                retval = false;
            }
            // Check that each DefaultValue, if they exist, are also one of the PossibleValues and is also valid according to SpecialCase
            for (int i = 0; i < (int)types.size(); i++) {
                auto dv = validator.GetDefaultValue(key_idx, i);
                if ((dv != nullptr) && verbose) {
                    if (((int)possible_vals.size() > i) && (possible_vals[i] != "") && (possible_vals[i] != "[]")) {
                        /// @todo - check DefaultValue against PossibleValues for the i-th type. Need a PDF object so just report
                        /// report_stream << COLOR_WARNING << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << ":\t\tDefaultValue[" << i << "] = " << *dv << "\t\tPossibleValues[" << i << "] = " << possible_vals[i] << COLOR_RESET;
                    }
                    if (((int)specialcase_vals.size() > i) && (specialcase_vals[i] != "") && (specialcase_vals[i] != "[]")) {
                        /// @todo - check DefaultValue against SpecialCase field for the i-th type. Need a PDF object so just report
                        /// report_stream << COLOR_WARNING << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << ":\t\tDefaultValue[" << i << "] = " << *dv << "\t\tSpecialCase[" << i << "] = " << specialcase_vals[i] << COLOR_RESET;
                    }
                }
            }
        }

        if (vc[TSV_POSSIBLEVALUES] != "")
            if (types.size() != possible_vals.size()) {
                report_stream << COLOR_ERROR << "wrong # of types vs. # of PossibleValues " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << COLOR_RESET;
                retval = false;
            }

        if (vc[TSV_SPECIALCASE] != "")
            if (types.size() != specialcase_vals.size()) {
                report_stream << COLOR_ERROR << "wrong # of types vs. # of SpecialCase " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << COLOR_RESET;
                retval = false;
            }

        // Check alphabetical sorting of Types
        for (size_t i = 1; i < types.size(); i++) {
            // Strip off all version or extension predicates to extract only the Arlington data type
            std::string t0 = validator.ReduceTypeElement(types[i]);
            std::string t1 = validator.ReduceTypeElement(types[i - 1]);
            if ((t0.size() == 0) || (t1.size() == 0)) {
                report_stream << COLOR_ERROR << "Types " << vc[TSV_TYPE] << " had a processing error for " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << COLOR_RESET;
                retval = false;
            }
            else if (t0 < t1) {
                report_stream << COLOR_ERROR << "Types " << vc[TSV_TYPE] << " are not alphabetically sorted for " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << COLOR_RESET;
                retval = false;
            }
        }

        // Check versioning efficiency between SinceVersion field and all version-based predicates
        if (verbose && (vc[TSV_SINCEVERSION].size() == 3)) {
            int key_introduced_v = string_to_pdf_version(vc[TSV_SINCEVERSION]);
            for (size_t i = 0; i < vc.size(); i++) {
                int             pdf_ver;
                std::string     s;

                s = vc[i];
                auto ver_predicate = s.find("fn:SinceVersion(");
                while (ver_predicate != std::string::npos) {
                    pdf_ver = string_to_pdf_version(s.substr(ver_predicate + 16, 3));
                    if (pdf_ver <= key_introduced_v) {
                        report_stream << COLOR_INFO << "fn:SinceVersion() " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << " field " << ArlingtonTSVFieldNames[i];
                        report_stream << " was introduced in " << vc[TSV_SINCEVERSION] << " but " << s.substr(ver_predicate) << COLOR_RESET;
                    }
                    s = s.substr(ver_predicate + 19);
                    ver_predicate = s.find("fn:SinceVersion(");
                }

                s = vc[i];
                ver_predicate = s.find("fn:BeforeVersion(");
                while (ver_predicate != std::string::npos) {
                    pdf_ver = string_to_pdf_version(s.substr(ver_predicate + 17, 3));
                    if ((pdf_ver - 1) < key_introduced_v) {
                        report_stream << COLOR_INFO << "fn:BeforeVersion() " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << " field " << ArlingtonTSVFieldNames[i];
                        report_stream << " was introduced in " << vc[TSV_SINCEVERSION] << " but " << s.substr(ver_predicate) << COLOR_RESET;
                    }
                    s = s.substr(ver_predicate + 20);
                    ver_predicate = s.find("fn:BeforeVersion(");
                }

                s = vc[i];
                ver_predicate = s.find("fn:IsPDFVersion(");
                while (ver_predicate != std::string::npos) {
                    pdf_ver = string_to_pdf_version(s.substr(ver_predicate + 16, 3));
                    if (pdf_ver < key_introduced_v) {
                        report_stream << COLOR_INFO << "fn:IsPDFVersion() " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << " field " << ArlingtonTSVFieldNames[i];
                        report_stream << " was introduced in " << vc[TSV_SINCEVERSION] << " but " << s.substr(ver_predicate) << COLOR_RESET;
                    }
                    s = s.substr(ver_predicate + 19);
                    ver_predicate = s.find("fn:IsPDFVersion(");
                }
            }
        }
            
        report_stream.flush();
    } // for

    // Check if all local variables (@xxx) match a key in this object definition
    if (vars_list.size() > 0) {
        for (auto& v : vars_list)
            if (std::find(std::begin(keys_list), std::end(keys_list), v) == std::end(keys_list))
                report_stream << COLOR_ERROR << "referenced variable @" << v << " not a key in " << reader.get_tsv_name() << COLOR_RESET;
    }

    // Check for duplicate keys in this TSV file
    for (size_t i = 0; i < keys_list.size(); i++) {
        for (size_t j = 0; j < keys_list.size(); j++) {
            if ((i != j) && (keys_list[i] == keys_list[j])) {
                report_stream << COLOR_ERROR << "duplicate key in " << reader.get_tsv_name() << " for key #" << i << " " << keys_list[i] << COLOR_RESET;
                retval = false;
            }
        }
    }

    // Check that if at least one key that was inheritable and possibly required, then also a "Parent" key that is a dictionary
    // Not assuming page tree as this is more flexible (for future). Predicates in "Required" field are NOT processed
    if (has_reqd_inheritable) {
        if (std::find(std::begin(keys_list), std::end(keys_list), "Parent") == std::end(keys_list)) {
            report_stream << COLOR_ERROR << "at least one required inheritable key in " << reader.get_tsv_name() << " but no Parent key" << COLOR_RESET;
            retval = false;
        }
        else {
            for (auto& vc : data_list)
                if ((vc[TSV_KEYNAME] == "Parent") && (vc[TSV_TYPE] != "dictionary")) {
                    report_stream << COLOR_ERROR << "at least one required inheritable key in " << reader.get_tsv_name() << " but Parent key is not a dictionary" << COLOR_RESET;
                    retval = false;
                }
        }
    }

    // Check "*" wildcard key - must be last (duplicate keys already checked above)
    if (std::find(std::begin(keys_list), std::end(keys_list), "*") != std::end(keys_list)) {
        if (keys_list[keys_list.size() - 1] != "*") {
            report_stream << COLOR_ERROR << "wildcard key '*' in " << reader.get_tsv_name() << " was not last key" << COLOR_RESET;
            retval = false;
        }
    }

    // Array filenames match "ArrayOf*" or "*Array"
    auto filename = reader.get_tsv_name();
    bool valid_array_filename = (filename.rfind("ArrayOf", 0) == 0) || (filename.rfind("Array") != std::string::npos) || (filename.rfind("ColorSpace") != std::string::npos);
    if ((arl_type == "array") && !valid_array_filename) {
        report_stream << COLOR_ERROR << "array definition file '" << reader.get_tsv_name() << "' does not meet array file naming conventions!" << COLOR_RESET;
        retval = false;
    }

    bool pure_wildcard_only = false; // i.e. ambiguous - could be a dict or array
    bool valid_array_defn = check_valid_array_definition(reader.get_tsv_name(), keys_list, report_stream, &pure_wildcard_only);

    if ((arl_type == "array") && valid_array_filename && !valid_array_defn) {
        report_stream << COLOR_ERROR << "array definition file '" << reader.get_tsv_name() << "' did not validate as an array!" << COLOR_RESET;
        retval = false;
    }

    if ((arl_type != "array") && (arl_type != "name-tree") && (arl_type != "number-tree") && valid_array_defn && !pure_wildcard_only) {
        // Dictionary or stream
        report_stream << COLOR_ERROR << arl_type << " definition file '" << reader.get_tsv_name() << "' appears to be an array!" << COLOR_RESET;
        retval = false;
    }

    return retval;
}


/// @brief   Validate an entire Arlington PDF Model TSV folder for holistic links
///
/// @param[in] grammar_folder   folder containing a set of TSV files
/// @param[in] verbose          true if additional verbose debug output is wanted
/// @param[in] ofs              open output stream
void ValidateGrammarFolder(const fs::path& grammar_folder, bool verbose, std::ostream& ofs) {
    // collecting all tsv starting from Trailer (traditional and XRefStream)
    std::vector<ValidationContext>  processed;
    std::vector<ValidationContext>  to_process;
    ValidationContext               vcxt;
    fs::path                        gf;

    ofs << "BEGIN - Arlington Internal Grammar Validation Report - TestGrammar " << TestGrammar_VERSION << std::endl;
    ofs << "Arlington TSV data: " << fs::absolute(grammar_folder).lexically_normal() << std::endl;

#ifdef ARL_PARSER_TESTING
    UNREFERENCED_FORMAL_PARAM(verbose);
    ofs << COLOR_WARNING << "ARL_PARSER_TESTING was #defined so processing hardcoded predicates only!!" << COLOR_RESET;

    std::vector<std::string> parse_test_str = {
        "fn:Eval(UR3::Reference::0::@TransformMethod==UR3)",
        "fn:SinceVersion(1.2,string-byte)",
        "(fn:MustBeDirect(ID::0) && fn:MustBeDirect(ID::1))",
        "fn:Eval(fn:DefaultValue(@StateModel=='Marked','Unmarked') || fn:DefaultValue(@StateModel=='Review','None'))",
        "fn:IsRequired((fn:RectWidth(Rect)>0) || (fn:RectHeight(Rect)>0))",
        "fn:A((@c>=0) && (@b<=-1))",
        "fn:A(fn:B(xxx)==fn:C(@yy))",
        "fn:A()",
        "fn:A(123)",
        "fn:A(1.23,@x)",
        "fn:A((@x>0),true)",
        "fn:Eval((@O>=0) && (@O<=1))",
        "fn:Eval(fn:ArrayLength(DecodeParms)==fn:ArrayLength(Filter))",
        "fn:A((@c>=0) && (@b<=-1) || (xx!=yy))",
        "fn:Eval((@a>=1) && (@b<=2) || ((@c mod 3)==4))",
        "fn:Eval(((@a>=1) && (@b<=2)) || ((@c mod 3)==4))",
        "fn:Eval((RD::@0>=0) && (RD::@1>=0) && (RD::@2>=0) && (RD::@3>=0) && ((RD::@1+RD::@3)<fn:RectHeight(Rect)) && ((RD::@0+RD::@2)<fn:RectWidth(Rect)))"
    };

    for (auto& s : parse_test_str) {
        do {
            ofs << COLOR_INFO << std::endl << "In:  '" << s << "'" << COLOR_RESET;
            assert(pred_root == nullptr);
            pred_root = new ASTNode();
            s = LRParsePredicate(s, pred_root);
            ofs << COLOR_INFO << "AST: " << *pred_root << std::endl << "AST valid: " << (pred_root->valid() ? "true" : "false!") << COLOR_RESET;
            assert(pred_root->valid());
            delete pred_root;
            pred_root = nullptr;
            if (s.size() > 0)
                if ((s[0] == ',') || (s[0] == '[') || (s[0] == ']') || (s[0] == ';'))
                    s = s.substr(1, s.size() - 1);
        } while (!s.empty());
    }
    return;
#else
    // Multiple entry points into later Arlington grammars.
    vcxt.tsv_name = "FileTrailer";
    vcxt.type = "dictionary";
    to_process.push_back(vcxt);

    if (is_file(grammar_folder / "LinearizationParameterDict.tsv")) {
        vcxt.tsv_name = "LinearizationParameterDict";
        vcxt.type = "dictionary";
        to_process.push_back(vcxt);
    }

    // Avoid reporting errors in PDF sets prior to PDF 1.5
    if (is_file(grammar_folder / "XRefStream.tsv")) {
        vcxt.tsv_name = "XRefStream";
        vcxt.type = "stream";
        to_process.push_back(vcxt);
    }
    if (is_file(grammar_folder / "ObjectStream.tsv")) {
        vcxt.tsv_name = "ObjectStream";
        vcxt.type = "stream";
        to_process.push_back(vcxt);
    }

    // Build the full list of all referenced grammar files mentioned in "Links" fields (after stripping off all predicates)
    while (!to_process.empty()) {
        // Get and then remove last element from vector
        vcxt = to_process.back();
        to_process.pop_back();

        // std::find_if() matching lambda function - same TSV file AND same type of PDF object
        auto is_validation_match = [&vcxt](ValidationContext a) { 
            return (a.tsv_name == vcxt.tsv_name) && (a.type == vcxt.type);
        };

        // Have we already processed this Arlington grammar TSV file (vcxt)?
        if (std::find_if(std::begin(processed), std::end(processed), is_validation_match) == std::end(processed)) {
            processed.push_back(vcxt);

            gf = grammar_folder / (vcxt.tsv_name + ".tsv");
            CArlingtonTSVGrammarFile reader(gf);
            if (reader.load()) {
                const ArlTSVmatrix &data = reader.get_data();
                for (auto& vc : data) {
                    std::string all_links = remove_type_link_predicates(vc[TSV_LINK]);
                    if (all_links != "") {
                        std::vector<std::string> links = split(all_links, ';');

                        std::string all_types = remove_type_link_predicates(vc[TSV_TYPE]);
                        std::vector<std::string> types = split(all_types, ';');
                        
                        int idx = 0;
                        for (auto& type_link : links) {
                            ValidationContext vcxt1;
                            // Index the type
                            vcxt1.type = types[idx];
                            idx++;

                            if ((type_link != "") && (type_link != "[]")) {
                                if ((type_link[0] == '[') && (type_link[type_link.size()-1] == ']')) {
                                    std::vector<std::string> direct_links = split(type_link.substr(1, type_link.size() - 2), ','); // strip [ and ] then split by COMMA
                                    for (auto& lnk : direct_links)
                                        if (lnk != "") {
                                            vcxt1.tsv_name = lnk;

                                            if (std::find(std::begin(v_ArlComplexTypes), std::end(v_ArlComplexTypes), vcxt1.type) == std::end(v_ArlComplexTypes)) {
                                                ofs << COLOR_ERROR << vcxt1.tsv_name << " has simple type '" << type_link << "' when link " << lnk << " is present" << COLOR_RESET;
                                            }

                                            // Name- and number-tree nodes can be any type so ignore false warnings
                                            if ((vcxt1.type != "name-tree") && (vcxt1.type != "number-tree")) {
                                                if ((lnk.find("Array") != std::string::npos) && (vcxt1.type != "array")) {
                                                    ofs << COLOR_WARNING << "in " << gf.stem() << ", " << lnk << " filename contains 'Array' but is linked as " << vcxt1.type << COLOR_RESET;
                                                }
                                                if ((lnk.find("Dict") != std::string::npos) && (vcxt1.type != "dictionary")) {
                                                    ofs << COLOR_WARNING << "in " << gf.stem() << ", " << lnk << " filename contains 'Dict' but is linked as " << vcxt1.type << COLOR_RESET;
                                                }
                                                if ((lnk.find("Stream") != std::string::npos) && (vcxt1.type != "stream") && (lnk != "ArrayOfStreamsGeneral")) {
                                                    ofs << COLOR_WARNING << "in " << gf.stem() << ", " << lnk << " filename contains 'Stream' but is linked as " << vcxt1.type << COLOR_RESET;
                                                }
                                            }
                                            to_process.push_back(vcxt1);
                                        }
                                }
                                else {
                                    ofs << COLOR_ERROR << vcxt.tsv_name << " has bad link '" << type_link << "' - missing enclosing [ ]" << COLOR_RESET;
                                }
                            }
                        } // for
                    }
                } // for
            }
            else
                ofs << COLOR_ERROR << "linked file " << gf.stem() << " failed to load!" << COLOR_RESET;
        }
    } // while

    // Iterate across all physical files in the folder to append anything that exists but is so far unreferenced
    for (const auto& entry : fs::directory_iterator(grammar_folder)) {
        if (entry.is_regular_file() && entry.path().extension().string() == ".tsv") {
            const auto tsv = entry.path().stem().string();

            // std::find_if() matching lambda function - same TSV file ONLY since can't know type
            auto is_tsv_match = [&tsv](ValidationContext a) { return (a.tsv_name == tsv); };
            auto result = std::find_if(std::begin(processed), std::end(processed), is_tsv_match);

            if (result == std::end(processed)) {
                ofs << COLOR_ERROR << "can't reach " << tsv << " from Trailer or XRefStream (assumed as dictionary)" << COLOR_RESET;
                vcxt.tsv_name = tsv;
                vcxt.type = "dictionary"; // assumed!
                processed.push_back(vcxt);
            }
        }
    } // for

    // Now check everything...
    for (auto& p : processed) {
        gf = grammar_folder / (p.tsv_name + ".tsv");
        CArlingtonTSVGrammarFile reader(gf);
        if (!reader.load()) {
            ofs << COLOR_ERROR << "can't load Arlington TSV grammar file " << gf << " as " << p.type << COLOR_RESET;
        }
        else {
            check_grammar(reader, p.type, verbose, ofs);
        }
    } // for

    ofs << "END" << std::endl;
#endif // ARL_PARSER_TESTING
}
