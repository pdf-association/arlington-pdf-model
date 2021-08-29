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
#include <string>
#include <queue>
#include <map>
#include <cctype>
#include <iostream>

#include "utils.h"
#include "CheckGrammar.h"
#include "ArlingtonTSVGrammarFile.h"
#include "PredicateProcessor.h"
#include "TestGrammarVers.h"

namespace fs = std::filesystem;

template<typename charT>

struct my_equal {
    my_equal(const std::locale& loc) : loc_(loc) {}
    bool operator()(charT ch1, charT ch2) {
        return std::toupper(ch1, loc_) == std::toupper(ch2, loc_);
    }
private:
    const std::locale& loc_;
};

// find substring (case insensitive)
template<typename T>
int ci_find_substr(const T& str1, const T& str2, const std::locale& loc = std::locale())
{
    typename T::const_iterator it = std::search(str1.begin(), str1.end(),
        str2.begin(), str2.end(), my_equal<typename T::value_type>(loc));
    if (it != str1.end()) 
        return it - str1.begin();
    else 
        return -1; // not found
}


/// @brief  Checks the validity of a single Arlington PDF Model TSV file:
/// - correct # of columns (TAB separated)
/// - correct headings (first line)
/// - correct basic types 1st column
/// 
/// @param[in]      reader 
/// @param[in,out]  report_stream  open output stream to report errors
/// 
/// @return trues if Arlington TSV file is valid, false if there were any errors
bool check_grammar(CArlingtonTSVGrammarFile& reader, bool verbose, std::ostream& report_stream)
{
    bool                        retval = true;
    auto                        data_list = reader.get_data();
    std::vector<std::string>    keys_list;
    std::vector<std::string>    vars_list;

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

    // SLOW!!! Use heavy regexes to reduce predicates to nothing if they are well formed...
    if (verbose) {
        for (auto& vc : data_list)
            for (auto& col : vc) {
                if (col.find("fn:") != std::string::npos)
                    ValidationByConsumption(reader.get_tsv_name(), col, report_stream);

                // Locate all local variables (@xxx) to see if they are also keys in this object
                // Variables in other objects (yyy::@xxx) are NOT checked
                std::smatch  m;
                const std::regex   r_KeyValue("[^:]@([a-zA-Z0-9_]+)");
                if (std::regex_search(col, m, r_KeyValue) && m.ready() && (m.size() > 0)) {
                    for (int i = 1; i < m.size(); i += 2)
                        vars_list.push_back(m[i].str());    
                }
            }
    }

    for (auto& vc : data_list) {
        keys_list.push_back(vc[TSV_KEYNAME]);

        KeyPredicateProcessor *key_validator = new KeyPredicateProcessor(vc[TSV_KEYNAME]);
        if (!key_validator->ValidateRowSyntax()) {
            report_stream << "Error: KeyName field validation error " << reader.get_tsv_name() << " for key " << vc[TSV_KEYNAME] << std::endl;
            retval = false;
        }
        delete key_validator;

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

        LinkPredicateProcessor* links_validator = new LinkPredicateProcessor(vc[TSV_LINK]);
        if (!links_validator->ValidateRowSyntax()) {
            report_stream << "Error: Link field validation error " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << ": " << vc[TSV_LINK] << std::endl;
            retval = false;
        }
        delete links_validator;

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
                    if (std::find(CArlingtonTSVGrammarFile::arl_non_complex_types.begin(), 
                                  CArlingtonTSVGrammarFile::arl_non_complex_types.end(), 
                                  type) != CArlingtonTSVGrammarFile::arl_non_complex_types.end()) {
                        // type is a simple type - Links NOT expected
                        if (links[j] != "[]")  
                            report_stream << "Error: basic type " << type << " should not be linked in " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << ": " << links[j] << std::endl;
                    }
                    else if (std::find(CArlingtonTSVGrammarFile::arl_complex_types.begin(),
                                       CArlingtonTSVGrammarFile::arl_complex_types.end(),
                                       type) != CArlingtonTSVGrammarFile::arl_complex_types.end()) {
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
        } // if TSV_LINKS

        if (vc[TSV_DEFAULTVALUE] != "") {
            if (types.size() != default_val.size()) {
                report_stream << "Error: wrong # of types vs. # of DefaultValue " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << std::endl;
                retval = false;
            }
        }

        if (vc[TSV_POSSIBLEVALUES] != "") {
            if (types.size() != possible_vals.size()) {
                report_stream << "Error: wrong # of types vs. # of PossibleValues " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << std::endl;
                retval = false;
            }
        }

        report_stream.flush();
    } // for

    // Check if all local variables (@xxx) match a key in this object definition
    if (vars_list.size() > 0) {
        for (auto v : vars_list) 
            if (std::find(keys_list.begin(), keys_list.end(), v) == keys_list.end())
                report_stream << "Warning: referenced variable @" << v << " not a key in " << reader.get_tsv_name() << std::endl;
    }

    // Check for duplicate keys in this TSV file
    auto it = std::unique(keys_list.begin(), keys_list.end());
    if (it != keys_list.end()) {
        report_stream << "Error: duplicate keys in " << reader.get_tsv_name() << " for key " << *it << std::endl;
        retval = false;
    }
    
    // Check "*" wildcard key - must be last (duplicate keys already checked above)
    if (std::find(keys_list.begin(), keys_list.end(), "*") != keys_list.end()) 
        if (keys_list[keys_list.size() - 1] != "*") {
            report_stream << "Error: wildcard key '*' in " << reader.get_tsv_name() << " was not last key" << std::endl;
            retval = false;
        }

    // Check for integer-repeating wildcards: start at zero, adjacent, increasing by 1 each time
    std::regex      r_intWild("([0-9]+)\\*");
    bool            started_seq = false;
    int             next_int = 0;
    for (auto& k : keys_list) {
        std::smatch     m;
        if (std::regex_search(k, m, r_intWild) && m.ready() && (m.size() == 2)) {
            if (next_int != atoi(m[1].str().c_str())) {
                report_stream << "Error: integer+wildcard key " << k << " in " << reader.get_tsv_name() << " did not have expected integer " << next_int << std::endl;
                retval = false;
            } 
            started_seq = true;
            next_int++;
        }
        else if (started_seq) {
            report_stream << "Error: integer+wildcard key in " << reader.get_tsv_name() << " were not adjacent for integer " << next_int << std::endl;
            retval = false;
        }
    } // for

    return retval;
}


/// @brief   Validate an entire Arlington PDF Model TSV folder for holistic links
/// 
/// @param[in] grammar_folder   folder containing a set of TSV files
/// @param[in] ofs              open output stream 
void ValidateGrammarFolder(const fs::path& grammar_folder, bool verbose, std::ostream& ofs) {
    // collecting all tsv starting from Trailer (traditional and XRefStream)
    std::vector<std::string>  processed;
    std::vector<std::string>  to_process;
    fs::path                  gf;

    ofs << "BEGIN - Arlington Validation Report - TestGrammar " << TestGrammar_VERSION << std::endl;
    ofs << "Arlington TSV data: " << fs::absolute(grammar_folder).lexically_normal() << std::endl;
    if (verbose)
        ofs << "Predicate reduction by regular expression is being attempted." << std::endl;

    // Dual entry points into the latest Arlington grammars.
    // Will report errors in PDF sets prior to PDF 1.5.
    to_process.push_back("FileTrailer.tsv");
    to_process.push_back("XRefStream.tsv");

    // Build the full list of all referenced grammar files mentioned in "Links" fields (after stripping off all predicates)
    while (!to_process.empty()) { 
        std::string gfile = to_process.back();
        to_process.pop_back();
        if (std::find(processed.begin(), processed.end(), gfile) == processed.end()) {
            processed.push_back(gfile);
            gf = grammar_folder / gfile;
            CArlingtonTSVGrammarFile reader(gf);
            reader.load();
            const ArlTSVmatrix &data = reader.get_data();
            for (auto& vc : data) {
                std::string all_links = remove_link_predicates(vc[TSV_LINK]);
                if (all_links != "") {
                    std::vector<std::string> links = split(all_links, ';');
                    for (auto type_link : links)
                        if ((type_link != "") && (type_link != "[]")) 
                            if ((type_link[0] == '[') && type_link[type_link.size()-1] == ']') {
                                std::vector<std::string> direct_links = split(type_link.substr(1, type_link.size() - 2), ','); // strip [ and ] then split by COMMA
                                for (auto lnk : direct_links)
                                    if (lnk != "") 
                                        to_process.push_back(lnk + ".tsv");
                            }
                            else 
                                ofs << "Error: " << gfile << " has bad link '" << type_link << "' - missing enclosing [ ]" << std::endl;
                }
            } // for
        } // if
    } // while

    // Iterate across all physical files in the folder
    for (const auto& entry : fs::directory_iterator(grammar_folder)) {
        if (entry.is_regular_file() && entry.path().extension().string() == ".tsv") {
            const auto tsv = entry.path().filename().string();
            if (std::find(processed.begin(), processed.end(), tsv) == processed.end()) 
                ofs << "Error: can't reach " << tsv << " from Trailer or XRefStream" << std::endl;
            gf = grammar_folder / tsv;
            CArlingtonTSVGrammarFile reader(gf);
            if (!reader.load())
                ofs << "Error: can't load Arlington TSV grammar file " << gf << std::endl;
            else 
                check_grammar(reader, verbose, ofs);
        } // if
    } // for

    ofs << "END" << std::endl;
}
