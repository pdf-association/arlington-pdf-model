///////////////////////////////////////////////////////////////////////////////
/// @file
/// @brief CPDFFile class declaration
///
/// Contains data and functionality about a specific PDF file needed to support
/// various predicate calculations.
///
/// @copyright
/// Copyright 2022 PDF Association, Inc. https://www.pdfa.org
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
/// @author Peter Wyatt, PDF Association
///
///////////////////////////////////////////////////////////////////////////////

#ifndef PDFFile_h
#define PDFFile_h
#pragma once

#include "ASTNode.h"
#include "ArlingtonPDFShim.h"
#include "ArlingtonTSVGrammarFile.h"

#include <string>
#include <vector>
#include <filesystem>
#include <iostream>

using namespace ArlingtonPDFShim;
namespace fs = std::filesystem;

class CPDFFile
{
private:
    /// @brief PDF filename
    fs::path                pdf_filename;

    /// @brief PDF SDK object reference
    ArlingtonPDFSDK&        pdfsdk;

    /// @brief Physical file size (in bytes). int for simplicity.
    int                     filesize_bytes;

    /// @brief PDF version from the file header (raw from PDF)
    std::string             pdf_header_version;

    /// @brief PDF version from the Document Catalog (raw from PDF)
    std::string             pdf_catalog_version;

    /// @brief Forced PDF version from command line or empty()
    std::string             forced_version;

    /// @brief Don't round off PDF versions - do an exact version compare (--force exact)
    bool                    exact_version_compare;

    /// @brief Latest PDF version found in the PDF file (based on Arlington SinceVersion field). Initialized to "1.0".
    std::string             latest_feature_version;

    /// @brief Arlington TSV file of latest PDF version found in the PDF file (could also be free text).
    std::string             latest_feature_arlington;

    /// @brief Key related to latest PDF version found in the PDF file (could also be free text)
    std::string             latest_feature_key;

    /// @brief whether PDF file uses xref stream or a traditional trailer
    bool                    has_xref_stream;

    /// @brief flags whether a predicate expression involved fn:Deprecate(...)
    bool                    deprecated;

    /// @brief flags whether a predicate expression was fully implemented.
    /// Only ever set to false in predicate implementations.
    /// Only initialized to true at the start of ProcessPredicate()
    bool                    fully_implemented;

    /// @brief Trailer dictionary
    ArlPDFTrailer*          trailer;

    /// @brief the value of the trailer /Size key (i.e. maximum object number + 1)
    int                     trailer_size;

    /// @brief Document Catalog dictionary
    ArlPDFDictionary*       doccat;

    /// @brief List of names of extensions being supported. Default = empty list
    std::vector<std::string>    extensions;

    /// @brief Method to check if a key value is within a prescribed set of values
    bool check_key_value(ArlPDFDictionary* dict, const std::wstring& key, const std::vector<std::wstring> values);

    /// @brief Split an Arlington key path (e.g. Catalog::Names::Dests) into a vector of keys
    std::vector<std::string> split_key_path(std::string key);

    /// @brief  Gets the object mentioned by an Arlington path
    ArlPDFObject* get_object_for_path(ArlPDFObject* parent, const std::vector<std::string>& arlpath);

    /// @brief Convert a basic PDF object into an AST-Node equivalent
    ASTNode* convert_basic_object_to_ast(ArlPDFObject *obj);

    double convert_node_to_double(const ASTNode* node);

    ASTNode* fn_BeforeVersion(const ASTNode* ver_node, const ASTNode* thing);
    ASTNode* fn_Deprecated(const ASTNode* dep_ver, const ASTNode* thing);
    ASTNode* fn_IsPDFVersion(const ASTNode* ver_node, const ASTNode* thing);
    ASTNode* fn_SinceVersion(const ASTNode* ver_node, const ASTNode* thing);
    ASTNode* fn_Extension(const ASTNode* extn, const ASTNode* value);
    bool fn_ArraySortAscending(ArlPDFObject* parent, const ASTNode *arr_key, const ASTNode* step);
    bool fn_BitClear(ArlPDFObject* obj, const ASTNode* bit_node);
    bool fn_BitSet(ArlPDFObject* obj, const ASTNode* bit_node);
    bool fn_BitsClear(ArlPDFObject* obj, const ASTNode* low_bit_node, const ASTNode* high_bit_node);
    bool fn_BitsSet(ArlPDFObject* obj, const ASTNode* low_bit_node, const ASTNode* high_bit_node);
    bool fn_FontHasLatinChars(ArlPDFObject* obj);
    bool fn_ImageIsStructContentItem(ArlPDFObject* obj);
    bool fn_InMap(ArlPDFObject* obj, const ASTNode* map);
    bool fn_IsAssociatedFile(ArlPDFObject* obj);
    bool fn_IsEncryptedWrapper();
    bool fn_IsLastInArray(ArlPDFObject* parent, ArlPDFObject* obj, const ASTNode* key);
    bool fn_IsPDFTagged();
    bool fn_IsPresent(ArlPDFObject* parent, std::string& key);
    bool fn_MustBeDirect(ArlPDFObject* parent, ArlPDFObject* obj, const ASTNode* arg);
    bool fn_NoCycle(ArlPDFObject* obj, const std::string& key);
    bool fn_NotStandard14Font(ArlPDFObject* parent);
    bool fn_PageContainsStructContentItems(ArlPDFObject* obj);
    bool fn_Contains(ArlPDFObject* obj, const ASTNode* key, const ASTNode* value);
    ASTNode* fn_PageProperty(ArlPDFObject* parent, ASTNode* pg, ASTNode* pg_key);
    ASTNode* fn_RequiredValue(ArlPDFObject* parent, const ASTNode* condition, const ASTNode* value);
    ASTNode* fn_DefaultValue(const ASTNode* condition, const ASTNode* value);
    double fn_RectHeight(ArlPDFObject* parent, const ASTNode* key);
    double fn_RectWidth(ArlPDFObject* parent, const ASTNode* key);
    int  fn_ArrayLength(ArlPDFObject* parent, const ASTNode* key);
    int  fn_NumberOfPages();
    int  fn_StreamLength(ArlPDFObject* parent, const ASTNode* key);
    int  fn_StringLength(ArlPDFObject* parent, const ASTNode* key);
    int  fn_FileSize() { return filesize_bytes; }

public:
    /// @brief PDF version being used (always a valid version, default is "2.0"). PUBLIC
    std::string             pdf_version;

    CPDFFile(const fs::path& pdf_file, ArlingtonPDFSDK& pdf_sdk, const std::string& forced_ver, const std::vector<std::string>& extns);

    ~CPDFFile() { /* destructor */ delete doccat; }

    /// @brief returns true iff the PDF file uses cross-reference streams
    bool uses_xref_stream() { return has_xref_stream; };

    /// @brief Returns the PDF files trailer dictionary or nullptr
    ArlPDFTrailer* get_trailer() { return trailer; };

    /// @returns the trailer /Size key or -1
    int get_trailer_size() { return trailer_size; }

    /// @brief PDF version to use when processing a PDF file (always a valid version)
    std::string  check_and_get_pdf_version(std::ostream& ofs);

    /// @brief Set the PDF version for an encountered feature so we can track latest version used
    void set_feature_version(const std::string& ver, const std::string& arl, const std::string& key);

    /// @brief returns the latest feature version encountered so far as a human readable string.
    std::string get_latest_feature_version_info();

    /// @brief whether a version override is being forced by --force
    bool is_forced_version() { return (forced_version.size() > 0); }

    /// @brief returns the list of currently support extensions. Could be an empty vector.
    std::vector<std::string> get_extensions() { return extensions; }

    /// @brief Calculates an Arlington predicate expression
    ASTNode* ProcessPredicate(ArlPDFObject* parent, ArlPDFObject* obj, const ASTNode* in_ast, const int key_idx, const ArlTSVmatrix& tsv_data, const int type_idx, int depth = 0);

    void ClearPredicateStatus() { deprecated = false; fully_implemented = true; };
    bool PredicateWasDeprecated() { return deprecated; };
    bool PredicateWasFullyProcessed() { return fully_implemented; };
};

#endif // PDFFile_h
