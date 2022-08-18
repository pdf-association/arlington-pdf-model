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
#include <map>
#include <iostream>
#include <queue>
#include <cassert>

#include "ArlingtonTSVGrammarFile.h"
#include "ArlingtonPDFShim.h"
#include "ArlVersion.h"
#include "PDFFile.h"
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

    /// @brief Data structure for recursive processing of the ArlPDFObjects
    /// @todo - lifetime management of recursive parent objects AND not blow out memory!
    struct queue_elem {
        ArlPDFObject* parent;   // PDF object of parent (can be null for trailer)
        ArlPDFObject* object;   // PDF object (e.g. of a key)
        std::string   link;     // Arlington TSV filename
        std::string   context;  // PDF DOM path

        queue_elem(ArlPDFObject* p, ArlPDFObject* o, const std::string &l, const std::string &c)
            : parent(p), object(o), link(l), context(c)
            { /* constructor */ assert(object != nullptr); assert(link.size() > 0); }
    };

    /// @brief The list of PDF objects to process
    std::queue<queue_elem>  to_process;

    /// @brief The folder with an Arlington TSV file set
    fs::path                grammar_folder;

    /// @brief Output stream to write results to. Already open
    std::ostream            &output;

    /// @brief Terse output. Otherwise output can make "... | sort | uniq | ..." Linux CLI pipelines difficult
    ///        Details of specific PDF objects (such as object numbers) are not output.
    bool                    debug_mode;

    /// @brief Terse output. Otherwise output can make "... | sort | uniq | ..." Linux CLI pipelines difficult
    ///        Details of specific PDF objects (such as object numbers) are not output.
    bool                    terse;

    /// @brief Ensures the context line for the PDF is shown for error, warning or info messages
    bool                    context_shown;

    /// @brief PDF class object for calculating predicates, versioning, etc.
    CPDFFile*               pdfc;

    /// @brief PDF version of file (multiplied by 10)
    int                     pdf_version;

    /// @brief Line counter of the PDF DOM for easier analysis and debugging
    unsigned int            counter;

    void show_context(queue_elem& e);

    /// @brief Locates & reads in a single Arlington TSV grammar file.
    const ArlTSVmatrix& get_grammar(const std::string& link);

    void parse_name_tree(ArlPDFDictionary* obj, const std::vector<std::string>& links, const std::string context, const bool root = true);
    void parse_number_tree(ArlPDFDictionary* obj, const std::vector<std::string>& links, const std::string context, const bool root = true);

    std::string recommended_link_for_object(ArlPDFObject* obj, const std::vector<std::string> links, const std::string obj_name);

    bool check_numeric_array(ArlPDFArray* arr, const int elems_to_check);
    void check_everything(ArlPDFObject* parent, ArlPDFObject* obj, const int key_idx, const ArlTSVmatrix& tsv_data, const std::string& grammar_file, const std::string& context, std::ostream& ofs);
    ArlPDFObject* find_via_inheritance(ArlPDFDictionary* obj, const std::wstring& key, const int depth = 0);

    /// @brief add an object to be checked
    void add_parse_object(ArlPDFObject* parent, ArlPDFObject* object, const std::string& link, const std::string& context);

public:
    CParsePDF(const fs::path& tsv_folder, std::ostream &ofs, const bool terser_output, const bool debug_output)
        : grammar_folder(tsv_folder), output(ofs), terse(terser_output), pdfc(nullptr), counter(0), context_shown(false), debug_mode(debug_output)
        { /* constructor */ }

    /// @brief add an object to be checked
    void add_root_parse_object(ArlPDFObject* object, const std::string& link, const std::string& context);

    /// @brief begin analysing a PDF file from a "root" object (most likely the trailer)
    bool parse_object(CPDFFile& pdf);
};

#endif // ParseObjects_h
