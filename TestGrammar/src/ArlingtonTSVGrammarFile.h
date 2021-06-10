///////////////////////////////////////////////////////////////////////////////
// ArlingtonTSVGrammarFile.h
// Copyright 2020-2021 PDF Association, Inc. https://www.pdfa.org
//
// This material is based upon work supported by the Defense Advanced
// Research Projects Agency (DARPA) under Contract No. HR001119C0079.
// Any opinions, findings and conclusions or recommendations expressed
// in this material are those of the author(s) and do not necessarily
// reflect the views of the Defense Advanced Research Projects Agency
// (DARPA). Approved for public release.
//
// SPDX-License-Identifier: Apache-2.0
// Contributors: Roman Toda, Frantisek Forgac, Normex; Peter Wyatt
//
///////////////////////////////////////////////////////////////////////////////

/*!
  A class to read the Arlington TSV-based grammar data from a TSV file.
*/

#pragma once

#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;

// Arlington TSV column titles and numbers
enum ArlingtonTSVColumns {
     TSV_KEYNAME         = 0,     // "*" means any
     TSV_TYPE            = 1,     // in alphabetical order of basic_types, "," separated
     TSV_SINCEVERSION    = 2,     // 1.0, 1.1, ..., 2.0
     TSV_DEPRECATEDIN    = 3,     // blank or 1.0, 1.1, ..., 2.0
     TSV_REQUIRED        = 4,     // TRUE or FALSE
     TSV_INDIRECTREF     = 5,     // TRUE or FALSE
     TSV_INHERITABLE     = 6,
     TSV_DEFAULTVALUE    = 7,
     TSV_POSSIBLEVALUES  = 8,
     TSV_SPECIALCASE     = 9,     // ignore for now...
     TSV_LINK            = 10,    // ";" separated list of "[xxx]"
     TSV_NOTES           = 11     // free text
};


class CArlingtonTSVGrammarFile
{
public:
    /// @brief All Arlington pre-defined types (alphabetically sorted)
    const std::vector<std::string>  arl_all_types =
        {
            "array",
            "bitmask",
            "boolean",
            "date",
            "dictionary",
            "integer",
            "matrix",
            "name",
            "name-tree",
            "null",
            "number",
            "number-tree",
            "rectangle",
            "stream",
            "string",
            "string-ascii",
            "string-byte",
            "string-text"
         };

    /// @brief Arlingon pre-defined types which REQUIRE a Link - aka "Complex types" (alphabetically sorted)
    const std::vector<std::string>  arl_complex_types =
    {
        "array",
        "dictionary",
        "name-tree",
        "number-tree",
        "stream"
    };

    /// @brief Arlington pre-defined types that must NOT have Links - aka "Non-complex types" (alphabetically sorted)
    const std::vector<std::string>  arl_non_complex_types =
    {
        "array",
        "bitmask",
        "boolean",
        "date",
        "integer",
        "matrix",
        "name",
        "null",
        "number",
        "rectangle",
        "string",
        "string-ascii",
        "string-byte",
        "string-text"
    };

    fs::path                              tsv_file_name;
    std::vector<std::vector<std::string>> data_list;
    std::vector<std::string>              header_list;

    CArlingtonTSVGrammarFile(fs::path tsv_name) :
        tsv_file_name(tsv_name)
    { /* constructor */ }

    /// Function to fetch data from a TSV File
    bool load();

    // Returns the name of the TSV file (without path or extension)
    std::string get_tsv_name();

    // Returns the folder containing the TSV files (without any filename or extension)
    fs::path    get_tsv_dir();

    /// Returns the raw TSV data
    const std::vector<std::vector<std::string>>& get_data();
};
