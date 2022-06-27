///////////////////////////////////////////////////////////////////////////////
/// @file 
/// @brief A class to read the Arlington TSV-based grammar data from a TSV file.
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

#ifndef ArlingtonTSVGrammarFile_h
#define ArlingtonTSVGrammarFile_h
#pragma once

#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;


/// @brief  Representation of raw Arlington TSV string data (rows and columns)
typedef std::vector<std::vector<std::string>> ArlTSVmatrix;


/// @brief Arlington TSV column (field) titles and numbers
enum ArlingtonTSVColumns {
     TSV_KEYNAME         = 0,     // "*" means any
     TSV_TYPE            = 1,     // in alphabetical order of basic_types, ";" separated
     TSV_SINCEVERSION    = 2,     // 1.0, 1.1, ..., 2.0
     TSV_DEPRECATEDIN    = 3,     // blank or 1.0, 1.1, ..., 2.0
     TSV_REQUIRED        = 4,     // TRUE or FALSE or predicates
     TSV_INDIRECTREF     = 5,     // TRUE or FALSE or predicates
     TSV_INHERITABLE     = 6,
     TSV_DEFAULTVALUE    = 7,
     TSV_POSSIBLEVALUES  = 8,     // predicates!!
     TSV_SPECIALCASE     = 9,     // predicates!!
     TSV_LINK            = 10,    // ";" separated list of "[xxx]" with predicates
     TSV_NOTES           = 11     // free text
};


class CArlingtonTSVGrammarFile
{
private:
    fs::path                    tsv_file_name;
    ArlTSVmatrix                data_list;

public:
    /// @brief All Arlington pre-defined types (alphabetically sorted)
    static const std::vector<std::string>  arl_all_types;

    /// @brief Arlingon pre-defined types which REQUIRE a Link - aka "Complex types" (alphabetically sorted)
    static const std::vector<std::string>  arl_complex_types;

    /// @brief Arlington pre-defined types that must NOT have Links - aka "Non-complex types" (alphabetically sorted)
    static const std::vector<std::string>  arl_non_complex_types;

    // @brief Arlington PDF versions
    static const std::vector<std::string>  arl_pdf_versions;

    /// @brief TSV header row - public only so can validating all Arlington grammar files
    std::vector<std::string>    header_list;

    CArlingtonTSVGrammarFile(fs::path tsv_name) :
        tsv_file_name(tsv_name)
        { /* constructor */ }

    /// @brief Function to fetch data from a TSV File
    bool load();

    /// @brief  Returns the name of the TSV file (without path or extension)
    std::string get_tsv_name();

    /// @brief Returns the folder containing the TSV files (without any filename or extension)
    fs::path    get_tsv_dir();

    /// @brief Returns a reference to the raw TSV data from the TSV file
    const ArlTSVmatrix& get_data();
};

#endif // ArlingtonTSVGrammarFile_h
