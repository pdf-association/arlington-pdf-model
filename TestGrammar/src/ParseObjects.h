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
// Contributors: Roman Toda, Frantisek Forgac, Normex. Peter Wyatt
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
    // remembering processed objects (and how they were validated)
    std::map<ArlPDFObject*, std::string>      mapped;
  
    // the Arlington PDF model (cache of loaded grammar files)
    std::map<std::string, std::unique_ptr<CArlingtonTSVGrammarFile>>  grammar_map;

    // simulating recursive processing of the ArlPDFObjects
    struct queue_elem {
        ArlPDFObject* object;
        std::string   link;
        std::string   context;
        
        // Constructor
        queue_elem(ArlPDFObject* o, const std::string &l, std::string &c)
            : object(o), link(l), context(c)
        { /* */ }
    };

    std::queue<queue_elem>  to_process;
    std::filesystem::path   grammar_folder;
    std::ostream            &output;
  
    const std::vector<std::vector<std::string>>* get_grammar(const std::string& link);

public:
    CParsePDF(const std::filesystem::path& tsv_folder, std::ostream &ofs)
        : grammar_folder(tsv_folder), output(ofs)
    { /* constructor */ }

    void add_parse_object(ArlPDFObject* object, const std::string& link, std::string context);
    void parse_object();
    void parse_name_tree(ArlPDFDictionary* obj, const std::string &links, std::string context);
    void parse_number_tree(ArlPDFDictionary* obj, const std::string &links, std::string context);

    std::string select_one(ArlPDFObject* obj, const std::string &links_string, std::string &obj_name);
    std::string get_link_for_type(ArlPDFObject* obj, const std::string &types, const std::string &links);
    int get_type_index(ArlPDFObject*obj, std::string types);
    std::string get_type_string(ArlPDFObject*obj);
    void check_basics(ArlPDFObject * ArlPDFObject, const std::vector<std::string> &vec, const std::string &grammar_file);
    bool check_possible_values(ArlPDFObject* object, const std::string& possible_value_str, int index, std::wstring& real_str_value);
};

#endif // ParseObjects_h
