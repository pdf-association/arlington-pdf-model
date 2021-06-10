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
// Contributors: Roman Toda, Frantisek Forgac, Normex
///////////////////////////////////////////////////////////////////////////////

#include <exception>
#include <iterator>
#include <queue>
#include <map>
#include <algorithm>
#include <cctype>
#include <iostream>

#include "Pdfix.h"
#include "utils.h"
#include "CheckGrammar.h"
#include "ArlingtonTSVGrammarFile.h"
#include "TestGrammarVers.h"

using namespace PDFixSDK;
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
  if (it != str1.end()) return it - str1.begin();
  else return -1; // not found
}

/// @brief  Checks the validity of a single Arlington PDF Model TSV file:
/// - correct # of columns (TAB separated)
/// - correct headings (first line)
/// - correct basic types 1st column
/// @param reader[in] 
/// @param report_stream 
/// @return true if TSV file is valid, false if there were errors
bool check_grammar(CArlingtonTSVGrammarFile& reader, std::ostream& report_stream)
{
    bool                        retval = true;
    std::vector<std::string>    processed;
    std::string                 function;
    auto                        data_list = reader.get_data();

    if (data_list.empty()) {
        report_stream << "Empty Arlington TSV grammar file: " << reader.get_tsv_name() << std::endl;
        return false;
    }

    if (reader.header_list.size() < 12) {
        report_stream << "Wrong number of columns for file: " << reader.get_tsv_name() << std::endl;
        return false;
    }

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
        report_stream << "Wrong column headers for file: " << reader.get_tsv_name() << std::endl;
        retval = false;
    }

    // check basic types (ignoring first line)
    // check existing link
    // check duplicate keys
    for (auto& vc : data_list) {
        if (std::find(processed.begin(), processed.end(), vc[TSV_KEYNAME]) == processed.end())
            processed.push_back(vc[TSV_KEYNAME]);
        else {
            report_stream << "Duplicate keys in " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << std::endl;
            retval = false;
        }

        // Various columns support multiple types by separating with ";" (SEMI-COLON)
        std::vector<std::string>    types = split(vc[TSV_TYPE], ';');
        std::vector<std::string>    links = split(vc[TSV_LINK], ';');
        std::vector<std::string>    possible_val = split(vc[TSV_POSSIBLEVALUES], ';');

        // if link exists check:
        // - number of links and number of types match
        // - each link follows pattern [];[];[];...
        // - each dictionary, array etc.. is linked
        // - each link actually exists as a TSV file
        if (vc[TSV_LINK] != "") {
            if (links.size() != types.size()) {
                report_stream << "Wrong # of Types vs. # of links " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << std::endl;
                retval = false;
            }

            for (size_t link_pos = 0; link_pos < links.size(); link_pos++) {
                // Check basic types which REQUIRE Links 
                if ((types.size() > link_pos) && (links[link_pos] == "[]") &&
                    ((types[link_pos] == "dictionary") || (types[link_pos] == "stream") || (types[link_pos] == "array") ||
                     (types[link_pos] == "name-tree") || (types[link_pos] == "number-tree"))) {
                    report_stream << "Type " << types[link_pos] << " not linked in " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << std::endl;
                    retval = false;
                } 

                /// @todo Check basic types which must NOT have any Links 

                // Each set of links for a type can be multiple links separated by "," (COMMA)
                std::vector<std::string> direct_links = split(links[link_pos].substr(1, links[link_pos].size() - 2), ',');

                // report all multiple links - all the places where a decision has to be made as to which one to validate
                // Note that the only predicates allowed in Links are: 'fn:SinceVersion', 'fn:BeforeVersion' and 'fn:IsPDFVersion'
                for (auto lnk : direct_links) {
                    if (lnk != "") {
                        fs::path        new_name = reader.get_tsv_dir();
                        auto            direct_lnk = extract_function(lnk, function);
                        new_name /= direct_lnk + ".tsv";
                        if (!fs::exists(new_name)) {
                            report_stream << "Link " << lnk << " doesn't exist in " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << std::endl;
                            retval = false;
                        } // if
                    } // if
                } // for direct_links
            } // for link_pos
        } // if

        // Check if each Type is a known Arlington pre-defined type
        for (auto type : types) {
            if ((std::find(reader.arl_all_types.begin(), reader.arl_all_types.end(), type)) == reader.arl_all_types.end()) {
                auto t = extract_function(type, function);
                if ((std::find(reader.arl_all_types.begin(), reader.arl_all_types.end(), t)) == reader.arl_all_types.end()) {
                    report_stream << "Unknown type " << type << " in " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << std::endl;
                    retval = false;
                }
            } 
        } 

        // Check pattern in possible values
        if (vc[TSV_POSSIBLEVALUES] != "") {
            if (types.size() != possible_val.size()) {
                report_stream << "Wrong # of types vs. # of possible values " << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << std::endl;
                retval = false;
            }
        }

        // check if an Arlington complex type has a Possible Value
        for (auto type : types) {
            auto it = std::find(reader.arl_complex_types.begin(), reader.arl_complex_types.end(), type);
            if ((it != reader.arl_complex_types.end()) && (vc[TSV_POSSIBLEVALUES] != "")) {
                int i;
                for(i = 0; types.at(i) != type; i++);
                if (possible_val[i] != "[]") {
                    report_stream << "Complex type has PossibleValue defined " << vc[TSV_POSSIBLEVALUES] << " in ";
                    report_stream << reader.get_tsv_name() << "/" << vc[TSV_KEYNAME] << std::endl;
                } // if
            } // if
        } // for

        report_stream.flush();
    }

    return retval;
}

/// @brief   Validate an entire Arlington PDF Model TSV folder
/// @param grammar_folder[in]   folder containing a set of TSV files
/// @param ofs[in]  output stream 
void ValidateGrammarFolder(const fs::path& grammar_folder, std::ostream& ofs) {
    // collecting all tsv starting from Catalog
    std::vector<std::string>  processed;
    std::vector<std::string>  to_process;
    fs::path                  gf;

    // Dual entry points into the latest Arlington grammars
    // Will report errors in PDF sets prior to PDF 1.5
    to_process.push_back("FileTrailer.tsv");
    to_process.push_back("XRefStream.tsv");

    while (!to_process.empty()) { 
        std::string gfile = to_process.back();
        to_process.pop_back();
        if (std::find(processed.begin(), processed.end(), gfile) == processed.end()) {
            processed.push_back(gfile);
            gf = grammar_folder / gfile;
            CArlingtonTSVGrammarFile reader(gf);
            reader.load();
            const std::vector<std::vector<std::string>>& data = reader.get_data();
            for (auto& vc : data) {
                if (vc[TSV_LINK] != "") {
                    std::vector<std::string> links = split(vc[TSV_LINK], ';');
                    for (auto type_link : links) {
                        std::vector<std::string> direct_links = split(type_link.substr(1, type_link.size() - 2), ',');
                        for (auto lnk : direct_links) {
                            if (lnk != "") {
                                std::string function;
                                auto direct_lnk = extract_function(lnk, function);
                                to_process.push_back(direct_lnk + ".tsv");
                            } 
                        } // for
                    } // for
                } // if
            } // for
        } // if
    } // while

    for (const auto& entry : fs::directory_iterator(grammar_folder)) {
        if (entry.is_regular_file() && entry.path().extension().string() == ".tsv") {
            const auto tsv = entry.path().filename().string();
            if (std::find(processed.begin(), processed.end(), tsv) == processed.end()) {
                ofs << "Can't reach " << tsv << "from Trailer or XRefStream" << std::endl;
            }
            gf = grammar_folder / tsv;
            CArlingtonTSVGrammarFile reader(gf);
            if (!reader.load())
                ofs << "Can't load Arlington TSV grammar file " << gf << std::endl;
            else 
                check_grammar(reader, ofs);
        } // if
    } // for
}
