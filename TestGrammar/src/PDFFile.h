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
    ArlingtonPDFSDK& pdfsdk;

    /// @brief Physical file size (in bytes). int for simplicity.
    int                     filesize_bytes;

    /// @brief PDF version from the file header (raw from PDF)
    std::string             pdf_header_version;

    /// @brief PDF version from the Document Catalog (raw from PDF)
    std::string             pdf_catalog_version;

    /// @brief Forced PDF version from command line
    std::string             forced_version;

    /// @brief whether PDF file uses xref stream or a traditional trailer
    bool                    has_xref_stream;

    /// @brief Trailer dictionary
    ArlPDFTrailer*          trailer;

    /// @brief Document Catalog dictionary
    ArlPDFDictionary*       doccat;

    /// @brief Method to check if a key value is within a prescribed set of values
    bool check_key_value(ArlPDFDictionary* dict, const std::wstring& key, const std::vector<std::wstring> values);

    /// @brief Convert a key to an array index
    int key_to_array_index(std::string& key);

    /// @brief Split an Arlington key path (e.g. Catalog::Names::Dests) into a vector of keys
    std::vector<std::string> split_key_path(std::string key);

public:
    /// @brief PDF version being used (always a valid version, default is "2.0")
    std::string             pdf_version;

    CPDFFile(const fs::path& pdf_file, ArlingtonPDFSDK& pdf_sdk, std::string& forced_ver);

    ~CPDFFile() { /* destructor */ delete doccat; }

    bool uses_xref_stream() { return has_xref_stream; };
    ArlPDFTrailer* get_trailer() { return trailer; };

    ASTNode* fn_BeforeVersion(const ASTNode* ver_node, const ASTNode* thing);
    ASTNode* fn_Deprecated(const ASTNode* dep_ver, const ASTNode* thing);
    ASTNode* fn_IsPDFVersion(const ASTNode* ver_node, const ASTNode* thing);
    ASTNode* fn_NotStandard14Font(ArlPDFObject* parent);
    ASTNode* fn_SinceVersion(const ASTNode* ver_node, const ASTNode* thing);
    bool fn_ArraySortAscending(ArlPDFObject* obj);
    bool fn_BitClear(ArlPDFObject* obj, const ASTNode* bit_node);
    bool fn_BitSet(ArlPDFObject* obj, const ASTNode* bit_node);
    bool fn_BitsClear(ArlPDFObject* obj, const ASTNode* low_bit_node, const ASTNode* high_bit_node);
    bool fn_BitsSet(ArlPDFObject* obj, const ASTNode* low_bit_node, const ASTNode* high_bit_node);
    bool fn_FontHasLatinChars(ArlPDFObject* obj);
    bool fn_Ignore() { return true; } // always true
    bool fn_ImageIsStructContentItem(ArlPDFObject* obj);
    bool fn_ImplementationDependent() { return true; } // always true
    bool fn_InMap(ArlPDFObject* obj, ASTNode* map);
    bool fn_IsAssociatedFile(ArlPDFObject* obj);
    bool fn_IsEncryptedWrapper();
    bool fn_IsLastInNumberFormatArray(ArlPDFObject* obj);
    bool fn_IsMeaningful(ArlPDFObject* obj, ASTNode* arg);
    bool fn_IsPDFTagged();
    bool fn_IsPresent(ArlPDFObject* obj, std::string& key, bool* is_present);
    bool fn_KeyNameIsColorant(std::wstring& key, std::vector<std::wstring>& colorants);
    bool fn_MustBeDirect(ArlPDFObject* obj);
    bool fn_NoCycle(ArlPDFObject* obj);
    bool fn_NotInMap(ArlPDFObject* obj, ASTNode* map);
    bool fn_NotPresent(ArlPDFObject* obj, std::string& key);
    bool fn_PageContainsStructContentItems(ArlPDFObject* obj);
    bool fn_PageProperty(ArlPDFObject* pg, ASTNode* pg_key);
    bool fn_RequiredValue(ArlPDFObject* obj, ASTNode* condition, ASTNode* value);
    double fn_RectHeight(ArlPDFObject* obj);
    double fn_RectWidth(ArlPDFObject* obj);
    int  fn_ArrayLength(ArlPDFObject* obj);
    int  fn_NumberOfPages();
    int  fn_StreamLength(ArlPDFObject* obj);
    int  fn_StringLength(ArlPDFObject* obj);
    int  fn_FileSize() { return filesize_bytes; }

    std::string& get_pdf_header_version()
        { return pdf_header_version; }

    std::string& get_pdf_catalog_version()
        { return pdf_catalog_version; }

    /// @brief PDF version to use when processing a PDF file (always a valid version)
    std::string  get_pdf_version(std::ostream& ofs);

    ASTNode* ProcessPredicate(ArlPDFObject* obj, const ASTNode* in_ast, const int key_idx, const ArlTSVmatrix& tsv_data, const int type_idx, bool* fully_processed, int depth = 0);
};

#endif // PDFFile_h
