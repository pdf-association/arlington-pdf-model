///////////////////////////////////////////////////////////////////////////////
// ParseObjects.h
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

#ifndef ParseObjects_h
#define ParseObjects_h

///
/// Read the whole PDF starting from specific object and validating against
/// the Arlington PDF grammar provided via the set of TSV files
///
#pragma once

#include <string>
#include <filesystem>
#include <map>
#include <iostream>
#include <string>
#include <queue>

#include "ArlingtonTSVGrammarFile.h"
#include "ArlingtonPDFShim.h"
#include "utils.h"

using namespace ArlingtonPDFShim;

class CParsePDF
{
private:
    /// @brief Remembering processed PDF objects (and how they were validated).
    //  Storing hash_id of object and link with which we validated the object.
    std::map<std::string, std::string>      mapped;
  
    /// @brief the Arlington PDF model (cache of loaded TSV grammar files)
    std::map<std::string, std::unique_ptr<CArlingtonTSVGrammarFile>>  grammar_map;

    /// @brief simulating recursive processing of the ArlPDFObjects
    struct queue_elem {
        ArlPDFObject* object;
        std::string   link;
        std::string   context;
        
        // Constructor
        queue_elem(ArlPDFObject* o, const std::string &l, std::string &c)
            : object(o), link(l), context(c)
        { /* */ }
    };

    /// @brief The list of PDF objects to process
    std::queue<queue_elem>  to_process;

    /// @brief The folder with an Arlington TSV file set
    std::filesystem::path   grammar_folder;

    /// @brief Output stream to write results to. Already open
    std::ostream            &output;
  
    const ArlTSVmatrix& get_grammar(const std::string& link);

public:
    CParsePDF(const std::filesystem::path& tsv_folder, std::ostream &ofs)
        : grammar_folder(tsv_folder), output(ofs)
        { /* constructor */ }

    void add_parse_object(ArlPDFObject* object, const std::string& link, std::string context);
    void parse_object();
    void parse_name_tree(ArlPDFDictionary* obj, const std::string &links, std::string context);
    void parse_number_tree(ArlPDFDictionary* obj, const std::string &links, std::string context);

    std::string select_one(ArlPDFObject* obj, const std::string &links_string, std::string &obj_name);
    std::string get_link_for_object(ArlPDFObject* obj, const std::string &types, const std::string &links);
    int get_type_index_for_object(ArlPDFObject* obj, const std::string &types);
    std::string get_type_string(ArlPDFObject* obj);
    void check_basics(ArlPDFObject* obj, int key_idx, const ArlTSVmatrix& tsv_data, const fs::path &grammar_file);
    bool check_possible_values(ArlPDFObject* object, int key_idx, const ArlTSVmatrix& tsv_data, const int index, std::wstring &real_str_value);
    bool is_required_key(ArlPDFObject* obj, const std::string &reqd);
};

#endif // ParseObjects_h
