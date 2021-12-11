///////////////////////////////////////////////////////////////////////////////
// CheckGrammar.cpp
// Copyright 2020 PDF Association, Inc. https://www.pdfa.org
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

#include <exception>
#include <iterator>
#include <algorithm>
#include <regex>
#include <queue>
#include <map>
#include <cctype>
#include <iostream>
#include <cassert>

#include "utils.h"
#include "CheckGrammar.h"
#include "ArlingtonTSVGrammarFile.h"
#include "PredicateProcessor.h"
#include "TestGrammarVers.h"

namespace fs = std::filesystem;

/// @brief When validating, need to know context of TSV (array, dict, stream, name-tree, number-tree)
typedef struct _ValidationContext  {
    std::string     tsv_name;
    std::string     type;
} ValidationContext;


/// @brief For debugging ease, make the root of an entire predicate static
static ASTNode* pred_root = nullptr;



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
        report_stream << reader.get_tsv_name() << ":" << std::endl;

    if (data_list.empty()) {
        report_stream << "Error: empty Arlington TSV grammar file: " << reader.get_tsv_name() << std::endl;
        return false;
    }

    if (reader.header_list.size() < 12) {
        report_stream << "Error: wrong number of columns in TSV file: " << reader.get_tsv_name() << std::endl;
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
        report_stream << "Error: wrong column headers for file: " << reader.get_tsv_name() << std::endl;
        retval = false;
    }

    bool has_reqd_inheritable = false;
    for (auto& vc : data_list) {
        // Add key of current row to a list to later check for duplicates
        keys_list.push_back(vc[TSV_KEYNAME]);

        for (auto& col : vc) {

            // Check brackets are all balanced
            if (std::count(std::begin(col), std::end(col), '[') != std::count(std::begin(col), std::end(col), ']'))
                report_stream << "Error: mismatched number of open '[' and close ']' set brackets '" << col << "' for " << reader.get_tsv_name() << std::endl;

            if (std::count(std::begin(col), std::end(col), '(') != std::count(std::begin(col), std::end(col), ')'))
                report_stream << "Error: mismatched number of open '(' and close ')' brackets '" << col << "' for " << reader.get_tsv_name() << std::endl;

            // Locate all local variables (@xxx) to see if they are also keys in this object
            // Variables in other objects (yyy::@xxx) are purposely NOT checked
            std::smatch  m;
            const std::regex   r_LocalKeyValue("[^:]@([a-zA-Z0-9_]+)");
            if (std::regex_search(col, m, r_LocalKeyValue) && m.ready() && (m.size() > 0))
                for (int i = 1; i < (int)m.size(); i += 2)
                    vars_list.push_back(m[i].str());

            // Try and parse each predicate after isolating
            std::vector<std::string>  list = split(col, ';');
            for (auto& fn : list)
                if (fn.find("fn:") != std::string::npos) {
                    std::string s = fn;
                    if ((s[0] == '[') && (s[s.size() - 1] == ']'))    // Strip enclosing [ and ]
                        s = s.substr(1, s.size() - 2);

                    while (!s.empty()) {
                        // Sometimes the comma separated lists have whitespace between terms
                        if (s[0] == ' ') {
                            s = s.substr(1, s.size() - 1);
                            assert((s[0] != '&') && (s[0] != '|') && (s[0] != 'm'));  // BAD: need SPACE before &&, ||, mod
                        }
                        // if (verbose)
                        //    report_stream << "In:  '" << s << "'" << std::endl;
                        assert(pred_root == nullptr);
                        pred_root = new ASTNode();
                        s = LRParsePredicate(s, pred_root);
                        // if (verbose)
                        //    report_stream << "AST: " << *pred_root << std::endl;
                        assert(pred_root->valid());
                        delete pred_root;
                        pred_root = nullptr;
                        //if (verbose)
                        //    report_stream << std::endl;
                        if (!s.empty())
                            if ((s[0] == ',') || (s[0] == '[') || (s[0] == ']') || (s[0] == ';') || (s[0] == ' '))
                                s = s.substr(1, s.size() - 1);
                    } // while
                }
        } // for col

        KeyPredicateProcessor *key_validator = new KeyPredicateProcessor(vc[TSV_KEYNAME]);
        if (!key_validator->ValidateRowSyntax()) {
            report_stream << "Error: KeyName field validation error " << reader.get_tsv_name() << " for key " << vc[TSV_KEYNAME] << std::endl;
            retval = false;
        }
        delete key_validator;

        TypePredicateProcessor* type_validator = new TypePredicateProcessor(vc[TSV_TYPE]);
        if (!type_validator->ValidateRowSyntax()) {
            report_stream << "Error: Type field validation error " << reader.get_tsv_name() << " for key " << vc[TSV_TYPE] << std::endl;
            retval = false;
        }
        delete type_validator;

        SinceVersionPredicateProcessor* sincever_validator = new SinceVersionPredicateProcessor(vc[TSV_SINCEVERSION]);
        if (!sincever_validator->ValidateRowSyntax()) {
            report_stream << "Error: SinceVersion field validation error " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << ": " << vc[TSV_SINCEVERSION] << std::endl;
            retval = false;
        }
        delete sincever_validator;

        DeprecatedInPredicateProcessor* depver_validator = new DeprecatedInPredicateProcessor(vc[TSV_DEPRECATEDIN]);
        if (!depver_validator->ValidateRowSyntax()) {
            report_stream << "Error: DeprecatedIn field validation error " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << ": " << vc[TSV_DEPRECATEDIN] << std::endl;
            retval = false;
        }
        delete depver_validator;

        RequiredPredicateProcessor* reqd_validator = new RequiredPredicateProcessor(vc[TSV_REQUIRED]);
        if (!reqd_validator->ValidateRowSyntax()) {
            report_stream << "Error: Required field validation error " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << ": " << vc[TSV_REQUIRED] << std::endl;
            retval = false;
        }
        delete reqd_validator;

        IndirectRefPredicateProcessor* ir_validator = new IndirectRefPredicateProcessor(vc[TSV_INDIRECTREF]);
        if (!ir_validator->ValidateRowSyntax()) {
            report_stream << "Error: IndirectRef field validation error " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << ": " << vc[TSV_INDIRECTREF] << std::endl;
            retval = false;
        }
        delete ir_validator;

        InheritablePredicateProcessor* inherit_validator = new InheritablePredicateProcessor(vc[TSV_INHERITABLE]);
        if (!inherit_validator->ValidateRowSyntax()) {
            report_stream << "Error: Inheritable field validation error " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << ": " << vc[TSV_INHERITABLE] << std::endl;
            retval = false;
        }
        delete inherit_validator;
        if ((vc[TSV_INHERITABLE] == "TRUE") && (vc[TSV_REQUIRED] != "FALSE"))
            has_reqd_inheritable = true;

        DefaultValuePredicateProcessor* dv_validator = new DefaultValuePredicateProcessor(vc[TSV_DEFAULTVALUE]);
        if (!dv_validator->ValidateRowSyntax()) {
            report_stream << "Error: DefaultValue field validation error " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << ": " << vc[TSV_DEFAULTVALUE] << std::endl;
            retval = false;
        }
        delete dv_validator;

        PossibleValuesPredicateProcessor* pv_validator = new PossibleValuesPredicateProcessor(vc[TSV_POSSIBLEVALUES]);
        if (!pv_validator->ValidateRowSyntax()) {
            report_stream << "Error: PossibleValues field validation error " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << ": " << vc[TSV_POSSIBLEVALUES] << std::endl;
            retval = false;
        }
        delete pv_validator;

        SpecialCasePredicateProcessor* sc_validator = new SpecialCasePredicateProcessor(vc[TSV_SPECIALCASE]);
        if (!sc_validator->ValidateRowSyntax()) {
            report_stream << "Error: SpecialCase field validation error " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << ": " << vc[TSV_SPECIALCASE] << std::endl;
            retval = false;
        }
        delete sc_validator;

        LinkPredicateProcessor* links_validator = new LinkPredicateProcessor(vc[TSV_LINK]);
        if (!links_validator->ValidateRowSyntax()) {
            report_stream << "Error: Link field validation error " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << ": " << vc[TSV_LINK] << std::endl;
            retval = false;
        }
        delete links_validator;

        // TSV_NOTE

        // CHECK INTER-COLUMN CONSISTENCY
        // Various columns support multiple types by separating with ";" (SEMI-COLON)
        std::vector<std::string>    types = split(vc[TSV_TYPE], ';');
        std::vector<std::string>    links = split(vc[TSV_LINK], ';');
        std::vector<std::string>    default_val = split(vc[TSV_DEFAULTVALUE], ';');
        std::vector<std::string>    possible_vals = split(vc[TSV_POSSIBLEVALUES], ';');

        if (vc[TSV_LINK] != "") {
            if (links.size() != types.size()) {
                report_stream << "Error: wrong # of Types vs. # of links " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << std::endl;
                retval = false;
            }
            else {
                // Assumes same number of elements in both Types and Links vectors!
                // Check basic types which must NOT have any Links and complex types which REQUIRE Links.
                int j = -1;
                for (auto& type : types) {
                    j++;
                    type = remove_type_predicates(type);
                    if (std::find(std::begin(v_ArlNonComplexTypes), std::end(v_ArlNonComplexTypes), type) != std::end(v_ArlNonComplexTypes)) {
                        // type is a simple type - Links NOT expected
                        if (links[j] != "[]")
                            report_stream << "Error: basic type " << type << " should not be linked in " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << ": " << links[j] << std::endl;
                    }
                    else if (std::find(std::begin(v_ArlComplexTypes), std::end(v_ArlComplexTypes), type) != std::end(v_ArlComplexTypes)) {
                        // type is a complex type - Links are REQUIRED
                        if (links[j] == "[]")
                            report_stream << "Error: complex type " << type << " is unlinked in " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << std::endl;
                    }
                    else {
                        // unexpected type!
                        report_stream << "Error: unexpected type " << type << " in " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << std::endl;
                    }
                } // for
            }
        }

        if (vc[TSV_DEFAULTVALUE] != "")
            if (types.size() != default_val.size()) {
                report_stream << "Error: wrong # of types vs. # of DefaultValue " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << std::endl;
                retval = false;
            }

        if (vc[TSV_POSSIBLEVALUES] != "")
            if (types.size() != possible_vals.size()) {
                report_stream << "Error: wrong # of types vs. # of PossibleValues " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << std::endl;
                retval = false;
            }

        report_stream.flush();
    } // for

    // Check if all local variables (@xxx) match a key in this object definition
    if (vars_list.size() > 0) {
        for (auto& v : vars_list)
            if (std::find(std::begin(keys_list), std::end(keys_list), v) == std::end(keys_list))
                report_stream << "Warning: referenced variable @" << v << " not a key in " << reader.get_tsv_name() << std::endl;
    }

    // Check for duplicate keys in this TSV file
    auto it = std::unique(std::begin(keys_list), std::end(keys_list));
    if (it != std::end(keys_list)) {
        report_stream << "Error: duplicate keys in " << reader.get_tsv_name() << " for key " << *it << std::endl;
        retval = false;
    }

    // Check that if at least one key that was inheritable and possibly required, then also a Parent key that is a dictionary
    // Not assuming page tree as this is more flexible (for future). Predicates in "Required" field are NOT processed
    if (has_reqd_inheritable) {
        if (std::find(std::begin(keys_list), std::end(keys_list), "Parent") == std::end(keys_list)) {
            report_stream << "Error: at least one required inheritable key in " << reader.get_tsv_name() << " but no Parent key" << std::endl;
            retval = false;
        }
        else {
            for (auto& vc : data_list)
                if ((vc[TSV_KEYNAME] == "Parent") && (vc[TSV_TYPE] != "dictionary")) {
                    report_stream << "Error: at least one required inheritable key in " << reader.get_tsv_name() << " but Parent key is not a dictionary" << std::endl;
                    retval = false;
                }
        }
    }

    // Check "*" wildcard key - must be last (duplicate keys already checked above)
    if (std::find(std::begin(keys_list), std::end(keys_list), "*") != std::end(keys_list))
        if (keys_list[keys_list.size() - 1] != "*") {
            report_stream << "Error: wildcard key '*' in " << reader.get_tsv_name() << " was not last key" << std::endl;
            retval = false;
        }

    bool ambiguous = false;
    bool valid_array = check_valid_array_definition(reader.get_tsv_name(), keys_list, report_stream, &ambiguous);

    if ((arl_type == "array") && !valid_array) {
        report_stream << "Error: array definition file '" << reader.get_tsv_name() << "' did not validate as an array!" << std::endl;
        retval = false;
    }
    
    if ((arl_type != "array") && (arl_type != "name-tree") && (arl_type != "number-tree") && valid_array && !ambiguous) {
        // Dictionary or stream
        report_stream << "Error: " << arl_type << " definition file '" << reader.get_tsv_name() << "' appears to be an array!" << std::endl;
        // This is not a requirement that all arrays are explicitly named as such, but is otherwise highly confusing!
        if (reader.get_tsv_name().find("Array") != std::string::npos) {
            report_stream << "Warning: non-array definition file '" << reader.get_tsv_name() << "' is named inappropriately?" << std::endl;
        }
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

    ofs << "BEGIN - Arlington Validation Report - TestGrammar " << TestGrammar_VERSION << std::endl;
    ofs << "Arlington TSV data: " << fs::absolute(grammar_folder).lexically_normal() << std::endl;

#ifdef ARL_PARSER_TESTING
    ofs << "ARL_PARSER_TESTING #defined so processing hardcoded predicates only." << std::endl;

    std::vector<std::string> parse_test_str = {
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
            ofs << "In:  '" << s << "'" << std::endl;
            assert(pred_root == nullptr);
            pred_root = new ASTNode();
            s = LRParsePredicate(s, pred_root);
            ofs << "AST: " << *pred_root << std::endl;
            ofs << "AST valid: " << (pred_root->valid() ? "true" : "false!") << std::endl;
            assert(pred_root->valid());
            delete pred_root;
            pred_root = nullptr;
            if (!s.empty())
                if ((s[0] == ',') || (s[0] == '[') || (s[0] == ']') || (s[0] == ';'))
                    s = s.substr(1, s.size() - 1);
        } while (!s.empty());
    }
    return;
#endif // ARL_PARSER_TESTING

    if (verbose)
        ofs << "Predicate reduction by regular expression is being attempted." << std::endl;

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
                    std::string all_links = remove_link_predicates(vc[TSV_LINK]);
                    if (all_links != "") {
                        std::vector<std::string> links = split(all_links, ';');

                        std::string all_types = remove_type_predicates(vc[TSV_TYPE]);
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
                                                ofs << "Error: " << vcxt1.tsv_name << " has simple type '" << type_link << "' when link " << lnk << "is present" << std::endl;
                                            }

                                            // Name- and number-tree nodes can be any type so ignore false warnings
                                            if ((vcxt1.type != "name-tree") && (vcxt1.type != "number-tree")) {
                                                if ((lnk.find("Array") != std::string::npos) && (vcxt1.type != "array")) {
                                                    ofs << "Warning: in " << gf.stem() << ", " << lnk << " filename contains 'Array' but is linked as " << vcxt1.type << std::endl;
                                                }
                                                if ((lnk.find("Dict") != std::string::npos) && (vcxt1.type != "dictionary")) {
                                                    ofs << "Warning: in " << gf.stem() << ", " << lnk << " filename contains 'Dict' but is linked as " << vcxt1.type << std::endl;
                                                }
                                                if ((lnk.find("Stream") != std::string::npos) && (vcxt1.type != "stream") && (lnk != "ArrayOfStreamsGeneral")) {
                                                    ofs << "Warning: in " << gf.stem() << ", " << lnk << " filename contains 'Stream' but is linked as " << vcxt1.type << std::endl;
                                                }
                                            }
                                            to_process.push_back(vcxt1);
                                        }
                                }
                                else {
                                    ofs << "Error: " << vcxt.tsv_name << " has bad link '" << type_link << "' - missing enclosing [ ]" << std::endl;
                                }
                            }
                        } // for
                    }
                } // for
            }
            else
                ofs << "Error: linked file " << gf.stem() << " failed to load!" << std::endl;
        }
    } // while

    if (verbose) {
        ofs << std::endl;
        for (auto& a : processed)
            ofs << "Info: " << a.tsv_name << " as " << a.type << std::endl;
        ofs << std::endl;
    }

    // Iterate across all physical files in the folder to append anything that exists but is so far unreferenced
    for (const auto& entry : fs::directory_iterator(grammar_folder)) {
        if (entry.is_regular_file() && entry.path().extension().string() == ".tsv") {
            const auto tsv = entry.path().stem().string();

            // std::find_if() matching lambda function - same TSV file ONLY since can't know type
            auto is_tsv_match = [&tsv](ValidationContext a) { return (a.tsv_name == tsv); };
            auto result = std::find_if(std::begin(processed), std::end(processed), is_tsv_match);

            if (result == std::end(processed)) {
                ofs << "Error: can't reach " << tsv << " from Trailer or XRefStream (assumed as dictionary)" << std::endl;
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
            ofs << "Error: can't load Arlington TSV grammar file " << gf << " as " << p.type << std::endl;
        }
        else {
            check_grammar(reader, p.type, verbose, ofs);
        }
    } // for

    ofs << "END" << std::endl;
}
