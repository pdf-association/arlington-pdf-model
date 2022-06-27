///////////////////////////////////////////////////////////////////////////////
/// @file 
/// @brief CParsePDF class declaration
/// 
/// Reads the whole PDF starting from specific object and validating against grammar
/// provided via an Arlington TSV file set
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

#ifndef ParseObjects_h
#define ParseObjects_h
#pragma once

#include <string>
#include <filesystem>
#include <map>
#include <iostream>
#include <string>
#include <queue>
#include <cassert>

#include "ArlingtonTSVGrammarFile.h"
#include "ArlingtonPDFShim.h"
#include "utils.h"

using namespace ArlingtonPDFShim;

class CParsePDF
{
private:
    /// @brief Remembering processed PDF objects (and how they were validated).
    ///        Storing hash_id of object as key and link with which we validated the object as the value.
    std::map<std::string, std::string>      mapped;

    /// @brief the Arlington PDF model (cache of loaded TSV grammar files)
    std::map<std::string, std::unique_ptr<CArlingtonTSVGrammarFile>>  grammar_map;

    /// @brief simulating recursive processing of the ArlPDFObjects
    struct queue_elem {
        ArlPDFObject* object;
        std::string   link;
        std::string   context;

        queue_elem(ArlPDFObject* o, const std::string &l, const std::string &c)
            : object(o), link(l), context(c)
            { /* constructor */ assert(o != nullptr); }
    };

    /// @brief The list of PDF objects to process
    std::queue<queue_elem>  to_process;

    /// @brief The folder with an Arlington TSV file set
    fs::path                grammar_folder;

    /// @brief Output stream to write results to. Already open
    std::ostream            &output;

    /// @brief Terse output. Otherwise output can make "... | sort | uniq | ..." Linux CLI pipelines difficult
    ///        Details of specific PDF objects (such as object numbers) are not output.
    bool                    terse;

    /// @brief Ensures the context line for the PDF is shown for error, warning or info messages
    bool                    context_shown;

    /// @brief File size (in bytes) - needed for predicate processing
    std::uintmax_t          filesize_bytes;

    /// @brief PDF version from the file header (never empty during file processing)
    std::string             pdf_header_version;

    /// @brief PDF version from the Document Catalog (can be empty)
    std::string             pdf_catalog_version;

    /// @brief PDF version to use when processing a PDF file (never empty during file processing)
    std::string             pdf_version;

    const ArlTSVmatrix& get_grammar(const std::string& link);

public:
    CParsePDF(const fs::path& tsv_folder, std::ostream &ofs, bool terser_output)
        : grammar_folder(tsv_folder), output(ofs), terse(terser_output), filesize_bytes(0)
        { /* constructor */ }

    void add_parse_object(ArlPDFObject* object, const std::string& link, const std::string& context);
    void set_pdf_file_size(const fs::path& pdf_file);
    std::uintmax_t get_pdf_file_size();
    void parse_object(const std::string &pdf_hdr_ver, ArlPDFObject* doc_cat_ver_obj);

private:
    void parse_name_tree(ArlPDFDictionary* obj, const std::string &links, std::string context, bool root = true);
    void parse_number_tree(ArlPDFDictionary* obj, const std::string &links, std::string context, bool root = true);

    int get_type_index_for_object(ArlPDFObject* obj, const std::string& types);
    std::string get_type_string_for_object(ArlPDFObject* obj);
    std::string get_link_for_object(ArlPDFObject* obj, const std::string &links_string, std::string &obj_name);
    std::string get_linkset_for_object_type(ArlPDFObject* obj, const std::string &types, const std::string &links);
    bool is_required_key(ArlPDFObject* obj, const std::string& reqd, const std::string& pdf_vers = "2.0");

    void check_basics(ArlPDFObject* obj, int key_idx, const ArlTSVmatrix& tsv_data, const fs::path &grammar_file, const std::string &context);
    bool check_possible_values(ArlPDFObject* object, int key_idx, const ArlTSVmatrix& tsv_data, const int index, std::wstring &real_str_value);
    ArlPDFObject* find_via_inheritance(ArlPDFDictionary* obj, const std::wstring& key, int depth = 0);
    bool check_predicates(ArlPDFObject* object, int key_idx, const ArlTSVmatrix& tsv_data, const fs::path& grammar_file, const std::string& context, const std::string& col);
};

#endif // ParseObjects_h
