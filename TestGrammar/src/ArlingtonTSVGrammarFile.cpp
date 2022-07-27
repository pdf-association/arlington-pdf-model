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

#include <iterator>

#include "ArlingtonTSVGrammarFile.h"

/// @brief  Parses through a TSV file line by line and loads TSV data into data_list
/// @return returns false if TSV data is malformed, else returns true
bool CArlingtonTSVGrammarFile::load()
{
    std::ifstream     file(tsv_file_name, std::ios::in);
    std::string       line = "";

    // Check if the file exists and is OK for reading
    if (!file.is_open() || file.bad() || file.eof()) {
        file.close();
        return false;
    }

    // Iterate through each line (row) and split content using TAB delimiter
    while (getline(file, line))
    {
        ArlTSVRow                   vec;
        std::string::size_type      prev_pos = 0;
        std::string::size_type      pos = 0;

        while ((pos = line.find('\t', pos)) != std::string::npos) {
            std::string substring(line.substr(prev_pos, pos - prev_pos));
            vec.push_back(substring);
            prev_pos = ++pos;
        }

        vec.push_back(line.substr(prev_pos, pos - prev_pos)); // Last word

        // Check first header line - have to have 12 columns
        if (data_list.empty() && (vec.size() < TSV_NOTES)) {
            file.close();
            return false;
        }

        // Move header row separately, so data is pure
        if (header_list.empty())
            header_list = vec;
        else
            data_list.push_back(vec);
    } // while

    // Close the TSV file
    file.close();

    // Empty file?
    if (data_list.size() == 0)
        return false;

    return true;
}

/// @brief  Returns the name of the TSV without folder or file extension
/// @return just the TSV filename (no folder, no extension) as a string
std::string CArlingtonTSVGrammarFile::get_tsv_name()
{
    return tsv_file_name.stem().string();
}


/// @brief  Returns the folder containing the current TSV file
/// @return Folder of the current TSV fie
fs::path CArlingtonTSVGrammarFile::get_tsv_dir()
{
    return tsv_file_name.parent_path();
}



/// @brief   Returns raw TSV data as a vector of vector of strings
/// @return  internal data_list (vector of vector of strings)
const ArlTSVmatrix& CArlingtonTSVGrammarFile::get_data()
{
    return data_list;
}
