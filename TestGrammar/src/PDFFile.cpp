///////////////////////////////////////////////////////////////////////////////
/// @file
/// @brief CPDFFile class definition
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

#include "PDFFile.h"
#include "ArlingtonPDFShim.h"
#include "ArlPredicates.h"
#include "utils.h"

#include <cassert>
#include <vector>
#include <math.h>

using namespace ArlingtonPDFShim;
namespace fs = std::filesystem;

/// @brief \#define PP_FN_DEBUG to get verbose debugging of issues to std::cout when calculating predicate functions
#undef PP_FN_DEBUG

/// @brief Constructor. Calculates some details about the PDF file
CPDFFile::CPDFFile(const fs::path& pdf_file, ArlingtonPDFSDK& pdf_sdk, std::string& forced_ver)
    : pdf_filename(pdf_file), pdfsdk(pdf_sdk), has_xref_stream(false), doccat(nullptr), latest_feature_version("1.0")
{
    if (!forced_ver.empty()) {
        forced_version = forced_ver;
    }

    // Get physical file size, reduced to an int for simplicity
    filesize_bytes = (int)fs::file_size(pdf_filename);

    trailer = pdfsdk.get_trailer(pdf_file.wstring());
    if (trailer != nullptr) {
        has_xref_stream = trailer->get_xrefstm();

        // Get PDF version from file header.  No sanity checking is done.
        pdf_header_version = pdfsdk.get_pdf_version(trailer);

        // Get the Document Catalog Version, if it exists. No sanity checking is done.
        if (trailer->has_key(L"Root")) {
            ArlPDFObject* dc = trailer->get_value(L"Root");
            if (dc != nullptr) {
                if ((dc->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary)) {
                    doccat = (ArlPDFDictionary*)dc;
                    ArlPDFObject* doc_cat_ver_obj = doccat->get_value(L"Version");

                    if (doc_cat_ver_obj != nullptr) {
                        if (doc_cat_ver_obj->get_object_type() == PDFObjectType::ArlPDFObjTypeName) {
                            std::wstring doccat_ver = ((ArlPDFName*)doc_cat_ver_obj)->get_value();
                            pdf_catalog_version = ToUtf8(doccat_ver);
                        }
                        delete doc_cat_ver_obj;
                    }
                }
                else {
                    delete dc; // DocCatalog was not a dictionary (wtf?) so can delete
                }
            }
        }
    }
}


/// @brief Convert a key which is assumed to be an integer array index
/// @returns -1 on error or the integer array index (>= 0)
int CPDFFile::key_to_array_index(std::string& key) {
    int i;
    try {
        i = std::stoi(key);
        if (i < 0)
            i = -1;
    }
    catch (...) {
        i = -1;
    }
    return i;
}


/// @brief Split an Arlington key path (e.g. Catalog::Names::Dests) into a vector of keys
/// @param[in]   key  an Arlington key which might be a key path
std::vector<std::string> CPDFFile::split_key_path(std::string key) {
    std::vector<std::string>    keys;

    auto sep = key.find("::");
    if (sep != std::string::npos) {
        // Have a multi-part path with "::" separators between keys
        do {
            keys.push_back(key.substr(0, sep - 1));
            key = key.substr(sep + 2);
            sep = key.find("::");
        } while (sep != std::string::npos);
    }
    else {
        // No "::" path separator
        keys.push_back(key);
    }
    return keys;
}

/// @brief Processes an AST-Node by recursively descending and calculating the left and right predicates.
///
/// @param[in]  obj              PDF object related to the predicate
/// @param[in]  in_ast           input AST tree
/// @param[in]  tsv_data         the row of TSV data that is being processed
/// @param[in]  key_idx          the index into the Arlington 'Key' field of the TSV data (>=0)
/// @param[in]  type_idx         the index into the Arlington 'Type' field of 'Key' field of the TSV data  (>=0)
/// @param[out] fully_processed  whether or not predicate processing fully completed all processing
/// @param[in]  depth            depth counter of recursion
/// 
/// @returns   Output AST 
ASTNode* CPDFFile::ProcessPredicate(ArlPDFObject* obj, const ASTNode* in_ast, const int key_idx, const ArlTSVmatrix& tsv_data, const int type_idx, bool* fully_processed, int depth) {
    assert(obj != nullptr);
    assert(in_ast != nullptr);
    assert(key_idx >= 0);
    assert(type_idx >= 0);
    assert(fully_processed != nullptr);

    ASTNode* out = new ASTNode;
    ASTNode* out_left = nullptr;
    ASTNode* out_right = nullptr;

#ifdef PP_DEBUG
    std::cout << std::string(depth * 2, ' ') << "In:  " << *in_ast << std::endl;
#endif

    if (in_ast->arg[0] != nullptr) {
        out_left = ProcessPredicate(obj, in_ast->arg[0], key_idx, tsv_data, type_idx, fully_processed, ++depth);
#ifdef PP_DEBUG
        if (out_left != nullptr) { std::cout << std::string(depth * 2, ' ') << " Out-Left:  " << *out_left << std::endl; }
#endif 
    }

    if (in_ast->arg[1] != nullptr) {
        out_right = ProcessPredicate(obj, in_ast->arg[1], key_idx, tsv_data, type_idx, fully_processed, ++depth);
#ifdef PP_DEBUG
        if (out_right != nullptr) { std::cout << std::string(depth * 2, ' ') << " Out-Right:  " << *out_right << std::endl; }
#endif 
    }

    switch (in_ast->type) {
    case ASTNodeType::ASTNT_ConstPDFBoolean:
    case ASTNodeType::ASTNT_ConstString:
    case ASTNodeType::ASTNT_ConstInt:
    case ASTNodeType::ASTNT_ConstNum:
    case ASTNodeType::ASTNT_Key:
        // Primitive type so out = in
        out->type = in_ast->type;
        out->node = in_ast->node;
        break;

    case ASTNodeType::ASTNT_Predicate:
    {
        if (in_ast->node == "fn:ArrayLength(") {
            out->type = ASTNodeType::ASTNT_ConstInt;
            out->node = std::to_string(fn_ArrayLength(obj));
        }
        else if (in_ast->node == "fn:ArraySortAscending(") {
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_ArraySortAscending(obj) ? "true" : "false";
        }
        else if (in_ast->node == "fn:BeforeVersion(") {
            delete out;
            out = fn_BeforeVersion(out_left, out_right);  
        }
        else if (in_ast->node == "fn:BitClear(") {
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_BitClear(obj, out_left) ? "true" : "false";
        }
        else if (in_ast->node == "fn:BitSet(") {
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_BitSet(obj, out_left) ? "true" : "false";
        }
        else if (in_ast->node == "fn:BitsClear(") {
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_BitsClear(obj, out_left, out_right) ? "true" : "false";
        }
        else if (in_ast->node == "fn:BitsSet(") {
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_BitsSet(obj, out_left, out_right) ? "true" : "false";
        }
        else if (in_ast->node == "fn:DefaultValue(") {
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = "true"; /// @todo ????
            *fully_processed = false; /// @todo 
        }
        else if (in_ast->node == "fn:Deprecated(") {
            delete out;
            out = fn_Deprecated(out_left, out_right);  
        }
        else if (in_ast->node == "fn:Eval(") {
            assert(out_left != nullptr);
            out->type = out_left->type;
            out->node = out_left->node;
        }
        else if (in_ast->node == "fn:FileSize(") {
            out->type = ASTNodeType::ASTNT_ConstInt;
            out->node = std::to_string(fn_FileSize());
        }
        else if (in_ast->node == "fn:FontHasLatinChars(") {
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_FontHasLatinChars(obj) ? "true" : "false";
        }
        else if (in_ast->node == "fn:Ignore(") {
            /// @todo - is this correct?
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = "true";
            *fully_processed = false; /// @todo 
        }
        else if (in_ast->node == "fn:ImageIsStructContentItem(") {
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_ImageIsStructContentItem(obj) ? "true" : "false";
            *fully_processed = false; /// @todo 
        }
        else if (in_ast->node == "fn:ImplementationDependent(") {
            /// @todo - is this correct?
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = "true";
        }
        else if (in_ast->node == "fn:InMap(") {
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_InMap(obj, out_left) ? "true" : "false";
            *fully_processed = false; /// @todo 
        }
        else if (in_ast->node == "fn:IsAssociatedFile(") {
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_IsAssociatedFile(obj) ? "true" : "false";
            *fully_processed = false; /// @todo 
        }
        else if (in_ast->node == "fn:IsEncryptedWrapper(") {
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_IsEncryptedWrapper() ? "true" : "false";
        }
        else if (in_ast->node == "fn:IsLastInNumberFormatArray(") {
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_IsLastInNumberFormatArray(obj) ? "true" : "false";
            *fully_processed = false; /// @todo 
        }
        else if (in_ast->node == "fn:IsMeaningful(") {
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_IsMeaningful(obj, out_left) ? "true" : "false";
        }
        else if (in_ast->node == "fn:IsPDFTagged(") {
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_IsPDFTagged() ? "true" : "false";
        }
        else if (in_ast->node == "fn:IsPDFVersion(") {
            delete out;
            out = fn_IsPDFVersion(out_left, out_right);     
        }
        else if (in_ast->node == "fn:IsPresent(") {
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = "true"; /// @todo /////////////////////////////////////////////////////
            *fully_processed = false; /// @todo 
        }
        else if (in_ast->node == "fn:IsRequired(") {
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = "false"; /// @todo /////////////////////////////////////////////////////
            *fully_processed = false; /// @todo 
        }
        else if (in_ast->node == "fn:KeyNameIsColorant(") {
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = "true"; /// @todo /////////////////////////////////////////////////////
            *fully_processed = false; /// @todo 
        }
        else if (in_ast->node == "fn:MustBeDirect(") {
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_MustBeDirect(obj, out_left) ? "true" : "false";
        }
        else if (in_ast->node == "fn:MustBeIndirect(") {
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_MustBeIndirect(obj, out_left) ? "true" : "false";
        }
        else if (in_ast->node == "fn:NoCycle(") {
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_NoCycle(obj) ? "true" : "false";
            *fully_processed = false; /// @todo 
        }
        else if (in_ast->node == "fn:NotInMap(") {
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_NotInMap(obj, out_left) ? "true" : "false";
            *fully_processed = false; /// @todo 
        }
        else if (in_ast->node == "fn:NotPresent(") {
            assert(out_left->type == ASTNodeType::ASTNT_Key);
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_NotPresent(obj, out_left->node) ? "true" : "false";
        }
        else if (in_ast->node == "fn:NotStandard14Font(") {
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_NotStandard14Font(obj) ? "true" : "false";
        }
        else if (in_ast->node == "fn:NumberOfPages(") {
            out->type = ASTNodeType::ASTNT_ConstInt;
            out->node = std::to_string(fn_NumberOfPages());
            *fully_processed = false; /// @todo 
        }
        else if (in_ast->node == "fn:PageContainsStructContentItems(") {
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_PageContainsStructContentItems(obj) ? "true" : "false";
            *fully_processed = false; /// @todo 
        }
        else if (in_ast->node == "fn:PageProperty(") {
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_PageProperty(obj, out_right) ? "true" : "false";
            *fully_processed = false; /// @todo 
        }
        else if (in_ast->node == "fn:RectHeight(") {
            out->type = ASTNodeType::ASTNT_ConstNum;
            out->node = std::to_string(fn_RectHeight(obj));
        }
        else if (in_ast->node == "fn:RectWidth(") {
            out->type = ASTNodeType::ASTNT_ConstNum;
            out->node = std::to_string(fn_RectWidth(obj));
        }
        else if (in_ast->node == "fn:RequiredValue(") {
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_RequiredValue(obj, out_left, out_right) ? "true" : "false";
            *fully_processed = false; /// @todo 
            delete out_left;
            delete out_right;
            out_left = out_right = nullptr;
        }
        else if (in_ast->node == "fn:SinceVersion(") {
            delete out;
            out = fn_SinceVersion(out_left, out_right);     
        }
        else if (in_ast->node == "fn:StreamLength(") {
            out->type = ASTNodeType::ASTNT_ConstInt;
            out->node = std::to_string(fn_StreamLength(obj));
        }
        else if (in_ast->node == "fn:StringLength(") {
            out->type = ASTNodeType::ASTNT_ConstInt;
            out->node = std::to_string(fn_StringLength(obj));
        }
        else {
            assert(false && "unrecognized predicate function!");
            *fully_processed = false;  
            delete out;
            delete out_left;
            delete out_right;
            return nullptr;
        }
    }
    break;

    case ASTNodeType::ASTNT_MathComp:
        // Math comparison operators - cannot be start of an AST!
        assert((out_left != nullptr) && (out_right != nullptr));
        out->type = ASTNodeType::ASTNT_ConstPDFBoolean;

        if (in_ast->node == "==") {
            // equality - could be numeric, logical, etc.
            out->node = (out_left->node == out_right->node) ? "true" : "false";
        }
        else if (in_ast->node == "!=") {
            // inequality - could be numeric, logical, etc.
            out->node = (out_left->node != out_right->node) ? "true" : "false";
        }
        else {
            double left = 0.0;
            double right = 0.0;

            try {
                left = std::stod(out_left->node);
            }
            catch (...) {
#ifdef PP_DEBUG
                std::cout << "left side floating point exception for " << out_left->node << "!" << std::endl;
#endif
                delete out;
                delete out_left;
                delete out_right;
                return nullptr;
            }
            try {
                right = std::stod(out_right->node);
            }
            catch (...) {
#ifdef PP_DEBUG
                std::cout << "right side floating point exception for " << out_right->node << "!" << std::endl;
#endif
                delete out;
                delete out_left;
                delete out_right;
                return nullptr;
            }

            if (in_ast->node == "<=") {
                // less than or equal to (numeric only)
                out->node = (left <= right) ? "true" : "false";
            }
            else if (in_ast->node == "<") {
                // less than (numeric only)
                out->node = (left < right) ? "true" : "false";
            }
            else if (in_ast->node == ">=") {
                // greater than or equal to (numeric only)
                out->node = (left >= right) ? "true" : "false";
            }
            else if (in_ast->node == ">") {
                // greater than (numeric)
                out->node = (left > right) ? "true" : "false";
            }
            else {
                assert(false && "unexpected math comparison!");
                delete out;
                delete out_left;
                delete out_right;
                return nullptr;
            }
        }
        break;

    case ASTNodeType::ASTNT_MathOp:
    {
        // Math operators: +, -, *, mod
        assert((out_left != nullptr) && (out_right != nullptr));
        double left = std::stod(out_left->node);
        double right = std::stod(out_right->node);

        // Work out typing - integer vs number
        if ((in_ast->arg[0]->type == ASTNodeType::ASTNT_ConstInt) && (in_ast->arg[1]->type == ASTNodeType::ASTNT_ConstInt))
            out->type = ASTNodeType::ASTNT_ConstInt;
        else
            out->type = ASTNodeType::ASTNT_ConstNum;

        if (in_ast->node == " + ") {
            // addition
            if (out->type == ASTNodeType::ASTNT_ConstInt)
                out->node = std::to_string(int(left + right));
            else
                out->node = std::to_string(left + right);
        }
        else if (in_ast->node == " - ") {
            // subtraction
            if (out->type == ASTNodeType::ASTNT_ConstInt)
                out->node = std::to_string(int(left - right));
            else
                out->node = std::to_string(left - right);
        }
        else if (in_ast->node == " * ") {
            // multiply
            if (out->type == ASTNodeType::ASTNT_ConstInt)
                out->node = std::to_string(int(left * right));
            else
                out->node = std::to_string(left * right);
        }
        else if (in_ast->node == " mod ") {
            // modulo
            out->type = ASTNodeType::ASTNT_ConstInt;
            out->node = std::to_string(int(left) % int(right));
        }
        else {
            assert(false && "unexpected math operator!");
            delete out;
            delete out_left;
            delete out_right;
            return nullptr;
        }
    }
    break;

    case ASTNodeType::ASTNT_LogicalOp:
    {
        // Logical operators - should have 2 operands (left, right) but due to version-based predicates
        // this can reduce to just one in which case the output is just the non-nullptr boolean.
        if ((out_left != nullptr) && (out_right == nullptr)) {
            assert(out_left->type == ASTNodeType::ASTNT_ConstPDFBoolean);
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = out_left->node;
            break;
        }
        else if ((out_left == nullptr) && (out_right != nullptr)) {
            assert(out_right->type == ASTNodeType::ASTNT_ConstPDFBoolean);
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = out_right->node;
            break;
        }

        assert((out_left != nullptr) && (out_right != nullptr));
        assert((out_left->type == ASTNodeType::ASTNT_ConstPDFBoolean) && (out_right->type == ASTNodeType::ASTNT_ConstPDFBoolean));
        if (in_ast->node == " && ") {
            // logical AND
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = ((out_left->node == "true") && (out_right->node == "true")) ? "true" : "false";
        }
        else if (in_ast->node == " || ") {
            // logical OR
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = ((out_left->node == "true") || (out_right->node == "true")) ? "true" : "false";
        }
        else {
            assert(false && "unexpected logical operator!");
            delete out;
            delete out_left;
            delete out_right;
            return nullptr;
        }
    }
    break;

    case ASTNodeType::ASTNT_KeyValue: // @keyname
    {
        assert(in_ast->node[0] == '@');
        std::string key = in_ast->node.substr(1); // strip the "@"

        bool self_refer = (tsv_data[key_idx][TSV_KEYNAME] == key);

        if (!self_refer) {
            /// @todo - need to get the parent object
            out->type = ASTNodeType::ASTNT_Key;
            out->node = key;
            break;
        }

        // self-reference: obj is thus the correct object
        PDFObjectType obj_type = obj->get_object_type();
        switch (obj_type) {
        case PDFObjectType::ArlPDFObjTypeName:
        {
            out->type = ASTNodeType::ASTNT_Key;
            out->node = ToUtf8(((ArlPDFName*)obj)->get_value());
        }
        break;

        case PDFObjectType::ArlPDFObjTypeNumber:
            if (((ArlPDFNumber*)obj)->is_integer_value()) {
                out->type = ASTNodeType::ASTNT_ConstInt;
                out->node = std::to_string(((ArlPDFNumber*)obj)->get_integer_value());
            }
            else {
                out->type = ASTNodeType::ASTNT_ConstNum;
                out->node = std::to_string(((ArlPDFNumber*)obj)->get_value());
            }
            break;

        case PDFObjectType::ArlPDFObjTypeBoolean:
        {
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = std::to_string(((ArlPDFBoolean*)obj)->get_value());
        }
        break;

        case PDFObjectType::ArlPDFObjTypeString:
        {
            out->type = ASTNodeType::ASTNT_ConstString;
            out->node = ToUtf8(((ArlPDFString*)obj)->get_value());
        }
        break;

        case PDFObjectType::ArlPDFObjTypeDictionary:
        {
            ArlPDFObject* val = ((ArlPDFDictionary*)obj)->get_value(ToWString(key));
            if (val != nullptr) {
                PDFObjectType val_type = val->get_object_type();
                /// @todo 
            }
            else {
                delete out;
                delete out_left;
                delete out_right;
                return nullptr;
            }
        }
        break;

        case PDFObjectType::ArlPDFObjTypeStream:
        {
            ArlPDFObject* val = ((ArlPDFStream*)obj)->get_dictionary()->get_value(ToWString(key));
            if (val != nullptr) {
                PDFObjectType val_type = val->get_object_type();
                /// @todo 
            }
            else {
                delete out;
                delete out_left;
                delete out_right;
                return nullptr;
            }
        }
        break;

        case PDFObjectType::ArlPDFObjTypeArray:
            try {
                int idx = std::stoi(key);
                ArlPDFObject* val = ((ArlPDFArray*)obj)->get_value(idx);
                if (val != nullptr) {
                    PDFObjectType val_type = val->get_object_type();
                    /// @todo 
                }
                else {
                    delete out;
                    delete out_left;
                    delete out_right;
                    return nullptr;
                }
            }
            catch (...) {  // Conversion of key as an array index integer failed!
                delete out;
                delete out_left;
                delete out_right;
                return nullptr;
            }
            break;

        case PDFObjectType::ArlPDFObjTypeNull:
        case PDFObjectType::ArlPDFObjTypeUnknown:
        case PDFObjectType::ArlPDFObjTypeReference:
        default:
            assert(false && "unexpected key-value type!");
            delete out;
            delete out_left;
            delete out_right;
            return nullptr;
        } // switch obj_type
    }
    break;

    case ASTNodeType::ASTNT_Unknown:
    case ASTNodeType::ASTNT_Type:
    default:
        // Likely a parsing error!
        delete out;
        delete out_left;
        delete out_right;
        assert(false && "unexpected AST node while recursing!");
        return nullptr;
    } // switch

    if (out != nullptr) {
        out->arg[0] = out_left;
        out->arg[1] = out_right;
#ifdef PP_DEBUG
        std::cout << std::string(depth * 2, ' ') << "Out: " << *out << std::endl;
#endif
        assert(out->valid());
    }
    else {
        delete out_left;
        delete out_right;
#ifdef PP_DEBUG
        std::cout << std::string(depth * 2, ' ') << "Out: nullptr" << std::endl;
#endif 
    }

    return out;
}


/// @brief   Check if the value of a key is in a dictionary and matches a given set
///
/// @param[in] dict     dictionary object
/// @param[in] key      the key name or array index
/// @param[in] values   a set of values to match
///
/// @returns true if the key value matches something in the values set
bool CPDFFile::check_key_value(ArlPDFDictionary* dict, const std::wstring& key, const std::vector<std::wstring> values)
{
    assert(dict != nullptr);
    ArlPDFObject* val_obj = dict->get_value(key);

    if (val_obj != nullptr) {
        std::wstring  val;
        switch (val_obj->get_object_type()) {
        case PDFObjectType::ArlPDFObjTypeString:
            val = ((ArlPDFString*)val_obj)->get_value();
            for (auto& i : values)
                if (val == i)
                    return true;
            break;

        case PDFObjectType::ArlPDFObjTypeName:
            val = ((ArlPDFName*)val_obj)->get_value();
            for (auto& i : values)
                if (val == i)
                    return true;
            break;

        default: /* fallthrough */
            break;
        } // switch
    }
    return false;
}


/// @brief Work out which PDF version to use between PDF header, DocCatalog::Version and command line.
/// Updates pdf_version field. Always returns a valid PDF version. Default version is "2.0".
/// 
/// @param[in,out] ofs    output stream for messages
/// @returns              Always a valid 3-char version string ("1.0", "1.1", ..., "2.0")
std::string CPDFFile::get_pdf_version(std::ostream& ofs)
{
    bool hdr_ok = ((pdf_header_version.size() == 3)  && FindInVector(v_ArlPDFVersions, pdf_header_version));
    bool cat_ok = ((pdf_catalog_version.size() == 3) && FindInVector(v_ArlPDFVersions, pdf_catalog_version));

    pdf_version.clear();

    if (hdr_ok)
        ofs << COLOR_INFO << "Header is version PDF " << pdf_header_version << COLOR_RESET;
    if (cat_ok)
        ofs << COLOR_INFO << "Document Catalog/Version is version PDF " << pdf_catalog_version << COLOR_RESET;

    if (hdr_ok && cat_ok) {
        // Choose latest version. Rely on ASCII for version computation
        if (pdf_catalog_version[0] > pdf_header_version[0]) {
            pdf_version = pdf_catalog_version;
        }
        else if (pdf_catalog_version[0] < pdf_header_version[0]) {
            ofs << COLOR_ERROR << "Document Catalog major version is earlier than PDF header version! Ignoring." << COLOR_RESET;
        }
        else { // major version digit is the same. Check minor digit
            if (pdf_catalog_version[2] > pdf_header_version[2]) {
                pdf_version = pdf_catalog_version;
            }
            else if (pdf_catalog_version[2] < pdf_header_version[2]) {
                ofs << COLOR_ERROR << "Document Catalog minor version is earlier than PDF header version! Ignoring." << COLOR_RESET;
                pdf_version = pdf_header_version;
            }
            else // versions are the same so fall through
                pdf_version = pdf_header_version;
        }
    }
    else if (cat_ok && !hdr_ok) {
        // Catalog::Version is only valid version
        pdf_version = pdf_catalog_version;
    }
    else if (!cat_ok && hdr_ok) {
        // Header is only valid version
        pdf_version = pdf_header_version;
    }
    else {
        // Both must be bad - assume latest version
        ofs << COLOR_ERROR << "Both Document Catalog and header versions are invalid. Assuming PDF 2.0." << COLOR_RESET;
        pdf_version = "2.0";
    }

    if (!forced_version.empty()) {
        ofs << COLOR_INFO << "Command line forced to version PDF " << forced_version << COLOR_RESET;
        pdf_version = forced_version;
    }
    assert(!pdf_version.empty());
    assert(FindInVector(v_ArlPDFVersions, pdf_version));

    return pdf_version;
}


/// @brief Set the PDF version for an encountered feature so we can track latest version used in a PDF file
///
/// @param[in]  ver   a valid PDF version from Arlington representing a feature we have just encountered
/// @param[in]  arl   the Arlington TSV file of the feature we have just encountered
/// @param[in]  key   the key (or array index) of the feature we have just encountered
void CPDFFile::set_feature_version(std::string ver, std::string arl, std::string key) {
    assert((ver.size() == 3) && FindInVector(v_ArlPDFVersions, ver));
    assert((latest_feature_version.size() == 3) && FindInVector(v_ArlPDFVersions, latest_feature_version));

    // Convert to 10 * PDF version
    int pdf_v = (ver[0] - '0') * 10 + (ver[2] - '0');
    int latest_v = (latest_feature_version[0] - '0') * 10 + (latest_feature_version[2] - '0');

    if (pdf_v > latest_v) {
        latest_feature_version = ver;
        latest_feature_arlington = arl;
        latest_feature_key = key;
    }
}


/// @brief returns the latest feature version details encountered so far as a human readable string. 
std::string CPDFFile::get_latest_feature_version_info()
{
    std::string s = "version PDF " + latest_feature_version;
    if (!latest_feature_arlington.empty()) {
        s = s + " (" + latest_feature_arlington;
        if (latest_feature_key.size() > 0) {
            s = s + "/" + latest_feature_key;
        }
        s = s + ")";
    }
    return s;
};


/// @brief Returns the length of a PDF array object
int CPDFFile::fn_ArrayLength(ArlPDFObject* obj) {
    assert(obj != nullptr);
    if (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeArray) {
        ArlPDFArray* arr = (ArlPDFArray*)obj;
        return arr->get_num_elements();
    }
#ifdef PP_FN_DEBUG
    std::cout << "fn_ArrayLength() was not an array!" << std::endl;
#endif
    return -1;
}

/// @brief Checks to see if the elements in a PDF array object are sorted. 
/// Unsortable elements return false.
bool CPDFFile::fn_ArraySortAscending(ArlPDFObject* obj) {
    assert(obj != nullptr);

    if (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeArray) {
        ArlPDFArray* arr = (ArlPDFArray*)obj;
        if (arr->get_num_elements() > 0) {
            // Make sure all array elements are a consistent numeric type
            PDFObjectType obj_type = arr->get_value(0)->get_object_type();
            if (obj_type == PDFObjectType::ArlPDFObjTypeNumber) {
                double       last_elem_val = ((ArlPDFNumber*)arr->get_value(0))->get_value();
                double       this_elem_val;
                for (int i = 1; i < arr->get_num_elements(); i++) {
                    obj_type = arr->get_value(i)->get_object_type();
                    if (obj_type == PDFObjectType::ArlPDFObjTypeNumber) {
                        this_elem_val = ((ArlPDFNumber*)arr->get_value(i))->get_value();
                        if (last_elem_val > this_elem_val)
                            return false; // was not sorted!
                        last_elem_val = this_elem_val;
                    }
                    else {
#ifdef PP_FN_DEBUG
                        std::cout << "fn_ArraySortAscending() had inconsistent types!" << std::endl;
#endif
                        return false; // inconsistent array element types
                    }
                } // for
                return true;
            }
            else {
#ifdef PP_FN_DEBUG
                std::cout << "fn_ArraySortAscending() was not a numeric array!" << std::endl;
#endif
                return false; // not a numeric array
            }
        }
        else
            return true; // empty array is always sorted by definition
    }
#ifdef PP_FN_DEBUG
    std::cout << "fn_ArraySortAscending() was not an array!" << std::endl;
#endif
    return false; // wasn't an array
}


/// @brief Checks if a single bit in a PDF integer object is clear. 
bool CPDFFile::fn_BitClear(ArlPDFObject* obj, const ASTNode* bit_node) {
    assert(obj != nullptr);

    assert((bit_node != nullptr) && (bit_node->type == ASTNodeType::ASTNT_ConstInt));
    int bit = std::stoi(bit_node->node);
    assert((bit >= 1) && (bit <= 32));

    PDFObjectType obj_type = obj->get_object_type();
    if (obj_type == PDFObjectType::ArlPDFObjTypeNumber) {
        ArlPDFNumber* num_obj = (ArlPDFNumber*)obj;
        if (num_obj->is_integer_value()) {
            int bitmask = 1 << (bit - 1);
            int val = num_obj->get_integer_value();
            return ((val & bitmask) == 0);
        }
        else {
#ifdef PP_FN_DEBUG
            std::cout << "fn_BitClear() was not an integer!" << std::endl;
#endif
            return false;  // wasn't an integer
        }
    }
    else {
#ifdef PP_FN_DEBUG
        std::cout << "fn_BitClear() was not a number!" << std::endl;
#endif
        return false; // wasn't a number
    }
}


/// @brief Checks if a single bit in a PDF integer object is set.
bool CPDFFile::fn_BitSet(ArlPDFObject* obj, const ASTNode* bit_node) {
    assert(obj != nullptr);

    assert((bit_node != nullptr) && (bit_node->type == ASTNodeType::ASTNT_ConstInt));
    int bit = std::stoi(bit_node->node);
    assert((bit >= 1) && (bit <= 32));

    PDFObjectType obj_type = obj->get_object_type();
    if (obj_type == PDFObjectType::ArlPDFObjTypeNumber) {
        ArlPDFNumber* num_obj = (ArlPDFNumber*)obj;
        if (num_obj->is_integer_value()) {
            int bitmask = 1 << (bit - 1);
            int val = num_obj->get_integer_value();
            return ((val & bitmask) == 1);
        }
        else {
#ifdef PP_FN_DEBUG
            std::cout << "fn_BitSet() was not an integer!" << std::endl;
#endif
            return false;  // wasn't an integer
        }
    }
    else {
#ifdef PP_FN_DEBUG
        std::cout << "fn_BitSet() was not a number!" << std::endl;
#endif
        return false; // wasn't a number
    }
}


/// @brief Checks if multiple bits in a PDF integer object are clear. 
bool CPDFFile::fn_BitsClear(ArlPDFObject* obj, const ASTNode* low_bit_node, const ASTNode* high_bit_node) {
    assert(obj != nullptr);

    assert((low_bit_node != nullptr) && (low_bit_node->type == ASTNodeType::ASTNT_ConstInt));
    int low_bit = std::stoi(low_bit_node->node);
    assert((low_bit >= 1) && (low_bit <= 32));

    assert((high_bit_node != nullptr) && (high_bit_node->type == ASTNodeType::ASTNT_ConstInt));
    int high_bit = std::stoi(high_bit_node->node);
    assert((high_bit >= 1) && (high_bit <= 32));

    assert(low_bit <= high_bit);

    PDFObjectType obj_type = obj->get_object_type();
    if (obj_type == PDFObjectType::ArlPDFObjTypeNumber) {
        ArlPDFNumber* num_obj = (ArlPDFNumber*)obj;
        if (num_obj->is_integer_value()) {
            int val = num_obj->get_integer_value();
            bool all_bits_clear = true;
            for (int bit = low_bit; bit <= high_bit; bit++) {
                int bitmask = 1 << (bit - 1);
                all_bits_clear = all_bits_clear && ((val & bitmask) == 0);
            }
            return all_bits_clear;
        }
        else {
#ifdef PP_FN_DEBUG
            std::cout << "fn_BitsClear() was not an integer!" << std::endl;
#endif
            return false;  // wasn't an integer
        }
    }
    else {
#ifdef PP_FN_DEBUG
        std::cout << "fn_BitsClear() was not a number!" << std::endl;
#endif
        return false; // wasn't a number
    }
}


/// @brief Checks if multiple bits in a PDF integer object are set.
bool CPDFFile::fn_BitsSet(ArlPDFObject* obj, const ASTNode* low_bit_node, const ASTNode* high_bit_node) {
    assert(obj != nullptr);

    assert((low_bit_node != nullptr) && (low_bit_node->type == ASTNodeType::ASTNT_ConstInt));
    int low_bit = std::stoi(low_bit_node->node);
    assert((low_bit >= 1) && (low_bit <= 32));

    assert((high_bit_node != nullptr) && (high_bit_node->type == ASTNodeType::ASTNT_ConstInt));
    int high_bit = std::stoi(high_bit_node->node);
    assert((high_bit >= 1) && (high_bit <= 32));

    assert(low_bit < high_bit);

    PDFObjectType obj_type = obj->get_object_type();
    if (obj_type == PDFObjectType::ArlPDFObjTypeNumber) {
        ArlPDFNumber* num_obj = (ArlPDFNumber*)obj;
        if (num_obj->is_integer_value()) {
            int val = num_obj->get_integer_value();
            bool all_bits_set = true;
            for (int bit = low_bit; bit <= high_bit; bit++) {
                int bitmask = 1 << (bit - 1);
                all_bits_set = all_bits_set && ((val & bitmask) == 1);
            }
            return all_bits_set;
        }
        else {
#ifdef PP_FN_DEBUG
            std::cout << "fn_BitsSet() was not an integer!" << std::endl;
#endif
            return false;  // wasn't an integer
        }
    }
    else {
#ifdef PP_FN_DEBUG
        std::cout << "fn_BitsSet() was not a number!" << std::endl;
#endif
        return false; // wasn't a number
    }
}


/// @brief Just assume that all fonts in PDF files have at least 1 Latin character. 
/// Used by FontDescriptors for CapHeight: e.g. fn:IsRequired(fn:FontHasLatinChars())
/// 
/// @param[in]  obj assumed to be a font descriptor dictionary
/// 
/// @returns true if the font object has Latin characters, false otherwise
bool CPDFFile::fn_FontHasLatinChars(ArlPDFObject* obj) {
    assert(obj != nullptr);

    // Check to make sure obj is likely a Font Descritor dictionary
    if (obj->get_object_type() != PDFObjectType::ArlPDFObjTypeDictionary) {
#ifdef PP_FN_DEBUG
        std::cout << "fn_FontHasLatinChars() object was not a dictionary!" << std::endl;
#endif
        return false;
    }

    if (!((ArlPDFDictionary*)obj)->has_key(L"Type")) {
#ifdef PP_FN_DEBUG
        std::cout << "fn_FontHasLatinChars() dictionary did not have a /Type key!" << std::endl;
#endif
        return false;
    }

    ArlPDFObject* t = ((ArlPDFDictionary*)obj)->get_value(L"Type");
    if ((t == nullptr) || (t->get_object_type() != PDFObjectType::ArlPDFObjTypeName)) {
#ifdef PP_FN_DEBUG
        std::cout << "fn_FontHasLatinChars() dictionary /Type key was not name!" << std::endl;
#endif
        return false;
    }

    if (((ArlPDFName*)t)->get_value() != L"FontDescriptor") {
#ifdef PP_FN_DEBUG
        std::cout << "fn_FontHasLatinChars() dictionary /Type key was not FontDescriptor!" << std::endl;
#endif
        delete t;
        return false;
    }

    delete t;
    return true;
}


/// @brief 
///  e.g. XObjectImage.tsv has fn:IsRequired(fn:ImageIsStructContentItem())
bool CPDFFile::fn_ImageIsStructContentItem(ArlPDFObject* obj) {
    assert(obj != nullptr);
    return true; /// @todo
}


/// @brief Returns true if obj is in the specified map
///  e.g. fn:InMap(RichMediaContent::Assets)
/// 
/// @param[in]  obj   a PDF object
/// @param[in]  map   the name of a PDF map
/// @returns  true if obj is in the specified map, false otherwise
bool CPDFFile::fn_InMap(ArlPDFObject* obj, ASTNode* map) {
    assert(obj != nullptr);
    assert(map != nullptr);
    assert(map->type == ASTNodeType::ASTNT_Key);
    return true; /// @todo
}

/// @brief Check if obj (which should be a File Specification dicionary) is in DocCat::AF array (of File Specification dicionaries)
bool CPDFFile::fn_IsAssociatedFile(ArlPDFObject* obj) {
    assert(obj != nullptr);

    // Check to make sure obj is likely a File Specification dictionary
    if (obj->get_object_type() != PDFObjectType::ArlPDFObjTypeDictionary) {
#ifdef PP_FN_DEBUG
        std::cout << "fn_IsAssociatedFile() object was not a dictionary!" << std::endl;
#endif
        return false;
    }
    auto obj_hash = obj->get_hash_id();

    // Locate 'obj' in the AF array based on matching hashes...
    ArlPDFObject* af = doccat->get_value(L"AF");
    if ((af != nullptr) && (af->get_object_type() == PDFObjectType::ArlPDFObjTypeArray)) {
        /// walk AF array of File Specification dictionaries looking for 'obj'
        ArlPDFArray* af_arr = (ArlPDFArray*)af;
        for (int i = 0; i < af_arr->get_num_elements(); i++) {
            ArlPDFObject* afile = af_arr->get_value(i);
            if ((afile != nullptr) && (afile->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary)) {
                if (obj_hash == afile->get_hash_id()) {
                    // Found it!
                    delete afile;
                    delete af;
                    return true;
                }
            }
            delete afile;
        }
        delete af;
    }

    return false;
}


/// @brief Check if PDF has an unencrytped wrapper as per clause 7.6.7 in ISO 32000-2:2020
/// 1. DocCatalog::Collection dictionary exists
/// 2. DocCatalog::Collection View key == /H 
/// 3. there is a FileSpec dictionary in the DocCatalog::Names::EmbeddedFiles name 
///    tree where the AFRelationship key == EncryptedPayload
/// 4. the same FileSpec dictionary is also in DocCatalog::AF array
bool CPDFFile::fn_IsEncryptedWrapper() {
    bool retval = false;

    ArlPDFObject* collection = doccat->get_value(L"Collection");
    if ((collection != nullptr) && (collection->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary)) {
        ArlPDFObject* view = ((ArlPDFDictionary*)collection)->get_value(L"View");
        if ((view != nullptr) && (view->get_object_type() == PDFObjectType::ArlPDFObjTypeName)) {
            if (((ArlPDFName*)view)->get_value() == L"H") {
                ArlPDFObject* names = doccat->get_value(L"Names");
                if ((names != nullptr) && (names->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary)) {
                    ArlPDFObject* embedded_files = ((ArlPDFDictionary*)names)->get_value(L"EmbeddedFiles");
                    if ((embedded_files != nullptr) && (embedded_files->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary)) {
                        /// @todo - walk EmbeddedFiles name tree
                        ArlPDFObject* af = doccat->get_value(L"AF");
                        if ((af != nullptr) && (af->get_object_type() == PDFObjectType::ArlPDFObjTypeArray)) {
                            /// walk AF array of File Specification dictionaries
                            ArlPDFArray *af_arr = (ArlPDFArray*)af;
                            for (int i = 0; i < af_arr->get_num_elements(); i++) {
                                ArlPDFObject* afile = af_arr->get_value(i);
                                if ((afile != nullptr) && (afile->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary)) {
                                    ArlPDFObject* af_rel = ((ArlPDFDictionary*)afile)->get_value(L"AFRelationship");
                                    if ((af_rel != nullptr) && (af_rel->get_object_type() == PDFObjectType::ArlPDFObjTypeName)) {
                                        if (((ArlPDFName*)af_rel)->get_value() == L"EncryptedPayload") {
                                            delete af_rel;
                                            retval = true;
                                            break;
                                        }
                                    }
                                    delete af_rel;
                                }
                                delete afile;
                            } // for
                        }
                        delete af;
                    }
                    delete embedded_files;
                }
                delete names;
            }
        }
        delete view;
    }
    delete collection;

    return retval;
}


/// @brief 
bool CPDFFile::fn_IsLastInNumberFormatArray(ArlPDFObject* obj) {
    assert(obj != nullptr);
    return false; /// @todo
}


/// @brief determines the meaningfulness of a PDF object according to ISO 32000-2:2020
///  e.g. fn:IsMeaningful(\@Subtype==Polyline) or fn:IsMeaningful(fn:IsPresent(CL))
/// 
/// @returns true if obj is "meaningful", false otherwise
bool CPDFFile::fn_IsMeaningful(ArlPDFObject* obj, ASTNode *arg) {
    assert(obj != nullptr);
    assert(arg != nullptr);
    assert(arg->type != ASTNodeType::ASTNT_ConstPDFBoolean);
    UNREFERENCED_FORMAL_PARAM(obj);
    return (arg->node == "true") ? true : false;
}


/// @brief determine if PDF file is a Tagged PDF via DocCat::MarkInfo::Marked == true
bool CPDFFile::fn_IsPDFTagged() {
    assert(doccat != nullptr);
    bool retval = false;

    ArlPDFObject *mi = doccat->get_value(L"MarkInfo");
    if ((mi != nullptr) && (mi->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary)) {
        ArlPDFObject* marked = ((ArlPDFDictionary *)mi)->get_value(L"Marked");
        if ((marked != nullptr) && (marked->get_object_type() == PDFObjectType::ArlPDFObjTypeBoolean)) {
            retval = ((ArlPDFBoolean*)marked)->get_value();
        }
        delete marked;
    }
    delete mi;
    return retval;
}


/// @brief If obj is a dictionary, check if key is present.  
/// If obj is an array, check if element at location key (an integer) is present.
/// Returns result via arg is_present.
/// 
/// @returns true if the function worked and *is_present was determined. 
bool CPDFFile::fn_IsPresent(ArlPDFObject* obj, std::string& key, bool* is_present) {
    assert(obj != nullptr);
    assert(is_present != nullptr);

    *is_present = false;
    switch (obj->get_object_type()) {
        case PDFObjectType::ArlPDFObjTypeArray: {
            ArlPDFArray* arr = (ArlPDFArray*)obj;
            try {
                int idx = std::stoi(key);
                *is_present = (arr->get_value(idx) != nullptr);
                return true;
            }
            catch (...) {
                return false; // key wasn't an integer
            }
        }
        case PDFObjectType::ArlPDFObjTypeDictionary: {
            ArlPDFDictionary* dict = (ArlPDFDictionary*)obj;
            *is_present = (dict->get_value(ToWString(key)) != nullptr);
            return true;
        }
        default:
            /// is this correct? Streams??
            break;
    } // switch
    return false;
}


/// @brief
bool CPDFFile::fn_KeyNameIsColorant(std::wstring& key, std::vector<std::wstring>& colorants) {
    for (auto& k : colorants)
        if (k == key)
            return true;
    return false;
}


/// @brief Checks if obj is an direct reference (i.e. NOT indirect)
/// e.g. fn:MustBeDirect() or fn:MustBeDirect(fn:IsPresent(Encrypt))
///
/// @param[in]  obj  PDF object which must be direct
/// @param[in]  arg  optional conditional AST
bool CPDFFile::fn_MustBeDirect(ArlPDFObject* obj, ASTNode *arg) {
    assert(obj != nullptr);
    if (arg == nullptr) {
        return !obj->is_indirect_ref();
    }
    else {
        assert(arg->type == ASTNodeType::ASTNT_ConstPDFBoolean);
        if (arg->node == "true")
            return !obj->is_indirect_ref();
        else
            return false;
    }
}


/// @brief Ensures an obj is an indirect reference
/// e.g. fn:MustBeIndirect(fn:BeforeVersion(2.0))
///
/// @param[in]  obj  PDF object which must be indirect
/// @param[in]  arg  optional conditional AST
bool CPDFFile::fn_MustBeIndirect(ArlPDFObject* obj, ASTNode* arg) {
    assert(obj != nullptr);
    assert(obj != nullptr);
    if (arg == nullptr) {
        return obj->is_indirect_ref();
    }
    else {
        assert(arg->type == ASTNodeType::ASTNT_ConstPDFBoolean);
        if (arg->node == "true")
            return obj->is_indirect_ref();
        else
            return false;
    }
}


/// @brief Checks to make sure that there are no cycles in obj
bool CPDFFile::fn_NoCycle(ArlPDFObject* obj) {
    assert(obj != nullptr);
    return true; /// @todo 
}


/// @brief Returns true if obj is NOT in map, false otherwise
/// e.g. fn:NotInMap(Catalog::Names::EmbeddedFiles)
/// @returns true if obj is NOT in map, false otherwise
bool CPDFFile::fn_NotInMap(ArlPDFObject* obj, ASTNode *map) {
    return !fn_InMap(obj, map);
}


/// @brief Determines if a key or array element index is NOT present in a PDF object
/// 
/// @param[in] obj   PDF object (dictionary or array)
/// @param[in] key   key (or integer array index if obj is an array)
/// 
/// @returns true if 'key' is NOT present in 'obj', false otherwise
bool CPDFFile::fn_NotPresent(ArlPDFObject* obj, std::string& key) {
    assert(obj != nullptr);
    assert(!key.empty());

    PDFObjectType t = obj->get_object_type();
    if (t == PDFObjectType::ArlPDFObjTypeDictionary) {
        ArlPDFDictionary* dict = (ArlPDFDictionary*)obj;
        return !dict->has_key(ToWString(key)); 
    }
    else if (t == PDFObjectType::ArlPDFObjTypeArray) {
        ArlPDFArray* arr = (ArlPDFArray*)obj;
        int idx = key_to_array_index(key); 
        return (idx < 0) || (idx >= arr->get_num_elements());
    }
    return false;
}


/// @brief PDF Standard 14 font names from ISO 32000
static const std::vector<std::wstring> Std14Fonts = {
    L"Times-Roman",
    L"Helvetica",
    L"Courier",
    L"Symbol",
    L"Times-Bold",
    L"Helvetica-Bold",
    L"Courier-Bold",
    L"ZapfDingbats",
    L"Times-Italic",
    L"Helvetica-Oblique",
    L"Courier-Oblique",
    L"Times-BoldItalic",
    L"Helvetica-BoldOblique",
    L"Courier-BoldOblique"
};

/// @brief
ASTNode* CPDFFile::fn_NotStandard14Font(ArlPDFObject* parent) {
    assert(parent != nullptr);

    if (parent->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary) {
        ArlPDFDictionary* dict = (ArlPDFDictionary*)parent;
        if (check_key_value(dict, L"Type", { L"Font" }) &&
            check_key_value(dict, L"Subtype", { L"Type1" }) &&
            !check_key_value(dict, L"BaseFont", Std14Fonts)) {
            ASTNode* node = new ASTNode;
            node->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            node->node = std::to_string(true);
            return node;
        }
        else
            return nullptr; // wasn't a Type 1 font dictionary
    }
    return nullptr;
}


/// @brief Used by PageObject.tsv for requiredness condition on StructParents key:
/// - fn:IsRequired(fn:PageContainsStructContentItems())
/// 
/// Need to check if the numeric value of the PDF object is a valid index into the 
/// trailer::DocCat::StructTreeRoot::ParentTree number tree
/// 
/// @param[in] obj   the StructParent/StructParents object
/// 
/// @returns
bool CPDFFile::fn_PageContainsStructContentItems(ArlPDFObject* obj) {
    assert(obj != nullptr);

    if (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeNumber) {
        if (((ArlPDFNumber*)obj)->is_integer_value()) {
            int val = ((ArlPDFNumber*)obj)->get_integer_value();
            if (val >= 0) {
                /// @todo 
                return true;
            }
            else {
#ifdef PP_FN_DEBUG
                std::cout << "fn_PageContainsStructContentItems() was less than zero!" << std::endl;
#endif
                return false;
            }
        }
        else {
#ifdef PP_FN_DEBUG
            std::cout << "fn_PageContainsStructContentItems() was not an integer!" << std::endl;
#endif
            return false; // not an integer
        }
    }
#ifdef PP_FN_DEBUG
    std::cout << "fn_PageContainsStructContentItems() was not number!" << std::endl;
#endif
    return false;  // not a number object
}


/// @brief Used by Target.tsv for A key: 
/// - fn:PageProperty(\@P,Annots)
/// - fn:Eval(\@A==fn:PageProperty(\@P,Annots::NM))
/// 
/// @param[in] pg       a PDF page object
/// @param[in] pg_key   a key of a PDF page object
/// 
/// @returns ????
bool CPDFFile::fn_PageProperty(ArlPDFObject* pg, ASTNode* pg_key) {
    assert(pg != nullptr);
    assert(pg_key != nullptr);
    assert(pg_key->type == ASTNodeType::ASTNT_Key);

    if (pg->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary) {
        ArlPDFDictionary* pg_dict = (ArlPDFDictionary*)pg;
        /// @todo - does not work for "Annots::NM". Also not checking if pg is REALLY a page dict (/Type/Page)
        return (pg_dict->has_key(ToWString(pg_key->node)));
    }
#ifdef PP_FN_DEBUG
    std::cout << "fn_PageProperty() page was not a dictionary!" << std::endl;
#endif
    return false;
}


/// @brief Returns the height of a PDF rectangle (>=0.0).
/// @returns -1.0 on error
double CPDFFile::fn_RectHeight(ArlPDFObject* obj) {
    assert(obj != nullptr);

    if (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeArray) {
        ArlPDFArray* rect = (ArlPDFArray*)obj;
        if (rect->get_num_elements() == 4) {
            for (int i = 0; i < 4; i++)
                if (rect->get_value(i)->get_object_type() != PDFObjectType::ArlPDFObjTypeNumber)
                    return -1.0; // not all rect array elements were numbers;
            double lly = ((ArlPDFNumber*)rect->get_value(1))->get_value();
            double ury = ((ArlPDFNumber*)rect->get_value(3))->get_value();
            double height = round(fabs(ury - lly));
            return height;
        }
        else {
#ifdef PP_FN_DEBUG
            std::cout << "fn_RectHeight() was not a 4 element array!" << std::endl;
#endif
            return -1.0; // not a 4 element array
        }
    }
#ifdef PP_FN_DEBUG
    std::cout << "fn_RectHeight() was not an array!" << std::endl;
#endif
    return -1.0; // not an array
}


/// @brief Returns the width of a PDF rectangle (>=0.0).
/// @returns -1.0 on error
double CPDFFile::fn_RectWidth(ArlPDFObject* obj) {
    assert(obj != nullptr);

    if (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeArray) {
        ArlPDFArray* rect = (ArlPDFArray*)obj;
        if (rect->get_num_elements() == 4) {
            for (int i = 0; i < 4; i++)
                if (rect->get_value(i)->get_object_type() != PDFObjectType::ArlPDFObjTypeNumber)
                    return -1.0; // not all rect array elements were numbers;
            double llx = ((ArlPDFNumber*)rect->get_value(0))->get_value();
            double urx = ((ArlPDFNumber*)rect->get_value(2))->get_value();
            double width = fabs(urx - llx);
            return width;
        }
        else {
#ifdef PP_FN_DEBUG
            std::cout << "fn_RectWidth() was not a 4 element array!" << std::endl;
#endif
            return -1.0; // not a 4 element array
        }
    }
#ifdef PP_FN_DEBUG
    std::cout << "fn_RectWidth() was not an array!" << std::endl;
#endif
    return -1.0; // not an array
}


/// @brief  Determines if value of a PDF object is as specified based on a boolean condition.
/// e.g. fn:RequiredValue(\@CFM==AESV2,128)
/// 
/// @param[in] obj          PDF object
/// @param[in] condition    already reduced AST node tree that is true/false
/// @param[in] value        can be any primitive PDF type (int, real, name, string-*, boolean)
/// 
/// @returns true if obj has the same as 'value', false otherwise
bool CPDFFile::fn_RequiredValue(ArlPDFObject* obj, ASTNode* condition, ASTNode* value) {
    assert(obj != nullptr);

    /// @todo: temporary code until @-value for non-self referenced keys works
    /// e.g. fn:Deprecated(2.0,fn:RequiredValue(\@V<2,2))
    if (condition == nullptr) {
        return false;
    }

    assert(condition != nullptr);
    assert(condition->type == ASTNodeType::ASTNT_ConstPDFBoolean);
    assert(value != nullptr);

    if (condition->node == "false") {
        // Condition not met so value of obj can be anything (no need to check)
        return false;
    }
    else {
        // Condition is met so value of obj MUST BE 'value'
        PDFObjectType obj_type = obj->get_object_type();
        switch (obj_type) {
        case PDFObjectType::ArlPDFObjTypeName:
            if (value->type == ASTNodeType::ASTNT_Key) {
                return (value->node == ToUtf8(((ArlPDFName*)obj)->get_value()));
            }
            break;

        case PDFObjectType::ArlPDFObjTypeNumber:
            if ((value->type == ASTNodeType::ASTNT_ConstInt) && ((ArlPDFNumber*)obj)->is_integer_value()) {
                return (value->node == std::to_string(((ArlPDFNumber*)obj)->get_integer_value()));
            }
            else if (value->type == ASTNodeType::ASTNT_ConstNum) {
                return (value->node == std::to_string(((ArlPDFNumber*)obj)->get_value()));
            }
            break;

        case PDFObjectType::ArlPDFObjTypeBoolean:
            if (value->type == ASTNodeType::ASTNT_ConstPDFBoolean) {
                bool b = ((ArlPDFBoolean*)obj)->get_value();
                if ((value->node == "true") && b) {
                    return true;
                }
                else if ((value->node == "false") && !b) {
                    return true;
                }
                // else fall through
            }
            break;

        case PDFObjectType::ArlPDFObjTypeString:
            if (value->type == ASTNodeType::ASTNT_ConstString) {
                return (value->node == ToUtf8(((ArlPDFString*)obj)->get_value()));
            }
            break;

        case PDFObjectType::ArlPDFObjTypeDictionary:
        case PDFObjectType::ArlPDFObjTypeStream:
        case PDFObjectType::ArlPDFObjTypeArray:
        case PDFObjectType::ArlPDFObjTypeNull:
        case PDFObjectType::ArlPDFObjTypeUnknown:
        case PDFObjectType::ArlPDFObjTypeReference:
        default:
            assert(false && "unexpected fn:RequiredValue value!");
            return false;
        } // switch obj_type
    }

    return false;
}


/// @brief Stream Length is according to Length key value, not actual stream data. 
/// @returns Length of the PDF stream object or -1 on error.
int CPDFFile::fn_StreamLength(ArlPDFObject* obj) {
    assert(obj != nullptr);

    if (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeStream) {
        ArlPDFStream* stm_obj = (ArlPDFStream*)obj;
        ArlPDFObject* len_obj = stm_obj->get_dictionary()->get_value(L"Length");
        if (len_obj->get_object_type() == PDFObjectType::ArlPDFObjTypeNumber) {
            ArlPDFNumber* len_num_obj = (ArlPDFNumber*)len_obj;
            if (len_num_obj->is_integer_value()) {
                return len_num_obj->get_integer_value();
            }
            else {
#ifdef PP_FN_DEBUG
                std::cout << "fn_StreamLength() Length was not an integer (was a float)!" << std::endl;
#endif
                return -1; // stream Length key was a float!
            }
        }
        else {
#ifdef PP_FN_DEBUG
            std::cout << "fn_StreamLength() Length was not a number!" << std::endl;
#endif
            return -1; // stream Length key was not an number
        }
    }
#ifdef PP_FN_DEBUG
    std::cout << "fn_StreamLength() was not a stream!" << std::endl;
#endif
    return -1; // not a stream
}


/// @brief Returns length of a PDF string object (>= 0). 
/// @returns Length of the PDF string object or -1 if an error.
int CPDFFile::fn_StringLength(ArlPDFObject* obj) {
    assert(obj != nullptr);

    if (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeString) {
        ArlPDFString* str_obj = (ArlPDFString*)obj;
        int len = (int)str_obj->get_value().size();
        return len;
    }
#ifdef PP_FN_DEBUG
    std::cout << "fn_StringLength() was not a string!" << std::endl;
#endif
    return -1; // not a string
}


/// @brief BeforeVersion means a feature was introduced prior to specific PDF version:
///   - fn:IsRequired(fn:BeforeVersion(1.3))
///   - fn:Eval((\@Colors>=1) && fn:BeforeVersion(1.3,fn:Eval(\@Colors<=4)))
/// 
/// @param[in] ver_node  version from Arlington PDF model
/// @param[in] thing     (optional) the feature that was introduced
/// 
/// @returns ASTNode or nullptr if after a PDF version
ASTNode* CPDFFile::fn_BeforeVersion(const ASTNode* ver_node, const ASTNode* thing) {
    assert(pdf_version.size() == 3);
    assert(FindInVector(v_ArlPDFVersions, pdf_version));

    assert(ver_node != nullptr);
    assert(ver_node->type == ASTNodeType::ASTNT_ConstNum);
    assert(ver_node->node.size() == 3);
    assert(FindInVector(v_ArlPDFVersions, ver_node->node));

    // Convert to 10 * PDF version
    int pdf_v = (pdf_version[0] - '0') * 10 + (pdf_version[2] - '0');
    int arl_v = (ver_node->node[0] - '0') * 10 + (ver_node->node[2] - '0');

    if (thing != nullptr) {
        if (pdf_v < arl_v) {
            ASTNode* out = new ASTNode;
            out->type = thing->type;
            out->node = thing->node;
            return out;
        }
    }
    else { // thing == nullptr
        ASTNode* out = new ASTNode;
        out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
        out->node = (pdf_v < arl_v) ? "true" : "false";
        return out;
    }
    return nullptr;
}


/// @brief SinceVersion means a feature was introduced in a specific PDF version
/// 
/// @param[in] ver_node  version when feature 'thing' was introduced from Arlington PDF model
/// @param[in] thing     (optional) the feature that was introduced
/// 
/// @returns AST-Node tree or nullptr if before a PDF version
ASTNode* CPDFFile::fn_SinceVersion(const ASTNode* ver_node, const ASTNode* thing) {
    assert(pdf_version.size() == 3);
    assert(FindInVector(v_ArlPDFVersions, pdf_version));

    assert(ver_node != nullptr);
    assert(ver_node->type == ASTNodeType::ASTNT_ConstNum);
    assert(ver_node->node.size() == 3);
    assert(FindInVector(v_ArlPDFVersions, ver_node->node));

    // Convert to 10 * PDF version
    int pdf_v = (pdf_version[0] - '0') * 10 + (pdf_version[2] - '0');
    int arl_v = (ver_node->node[0] - '0') * 10 + (ver_node->node[2] - '0');

    if (thing != nullptr) {
        if (pdf_v >= arl_v) {
            ASTNode* out = new ASTNode;
            out->type = thing->type;
            out->node = thing->node;
            return out;
        }
    }
    else { // thing == nullptr
        ASTNode* out = new ASTNode;
        out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
        out->node = (pdf_v >= arl_v) ? "true" : "false";
        return out;
    }
    return nullptr;
}


/// @brief IsPDFVersion means a feature was introduced for only a specific PDF version:
///   - fn:IsRequired(fn:IsPDFVersion(1.0))
///   - fn:IsPDFVersion(1.0,fn:BitsClear(2,32))
///   - fn:IsPDFVersion(1.2,ActionNOP)
/// 
/// @param[in] ver_node  version when feature 'thing' was introduced from Arlington PDF model
/// @param[in] thing     (optional) the feature that was introduced
/// 
/// @returns AST-Node tree or nullptr if not a specific PDF version
ASTNode* CPDFFile::fn_IsPDFVersion(const ASTNode* ver_node, const ASTNode* thing) {
    assert(pdf_version.size() == 3);
    assert(FindInVector(v_ArlPDFVersions, pdf_version));

    assert(ver_node != nullptr);
    assert(ver_node->type == ASTNodeType::ASTNT_ConstNum);
    assert(ver_node->node.size() == 3);
    assert(FindInVector(v_ArlPDFVersions, ver_node->node));

    // Convert to 10 * PDF version
    int pdf_v = (pdf_version[0] - '0') * 10 + (pdf_version[2] - '0');
    int arl_v = (ver_node->node[0] - '0') * 10 + (ver_node->node[2] - '0');
    if (thing != nullptr) {
        if (pdf_v == arl_v) {
            ASTNode* out = new ASTNode;
            out->type = thing->type;
            out->node = thing->node;
            return out;
        }
    }
    else { // thing == nullptr
        ASTNode* out = new ASTNode;
        out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
        out->node = (pdf_v == arl_v) ? "true" : "false";
        return out;
    }
    return nullptr;
}


/// @brief Deprecated predicate. If PDF version is BEFORE Arlington's deprecated version, then return 
/// whatever it was, otherwise return nullptr (meaning `thing` shouldn't exist as it has been deprecated).
///
/// @param[in] dep_ver  version when deprecated from the Arlington PDF model (1st arg to predicate)
/// @param[in] thing    thing that was deprecated (2nd arg to predicate) which itself may have been 
///                     a predicate and thus already reduced to an ASTNode or nullptr
/// 
/// @returns AST-Node tree or nullptr if at or after a PDF version
ASTNode* CPDFFile::fn_Deprecated(const ASTNode* dep_ver, const ASTNode* thing) {
    assert(pdf_version.size() == 3);
    assert(FindInVector(v_ArlPDFVersions, pdf_version));

    assert(dep_ver != nullptr);
    assert(dep_ver->type == ASTNodeType::ASTNT_ConstNum);
    assert(dep_ver->node.size() == 3);
    assert(FindInVector(v_ArlPDFVersions, dep_ver->node));

    // Convert to 10 * PDF version
    int pdf_v = (pdf_version[0] - '0') * 10 + (pdf_version[2] - '0');
    int arl_v = (dep_ver->node[0] - '0') * 10 + (dep_ver->node[2] - '0');

    if ((pdf_v < arl_v) && (thing != nullptr)) {
        ASTNode* out = new ASTNode;
        out->type = thing->type;
        out->node = thing->node;
        return out;
    }
    return nullptr;
}


/// @brief Returns the number of pages in the PDF file or -1 on error
/// @returns Number of pages in the PDF file or -1 on error
int CPDFFile::fn_NumberOfPages() {
    return 99999; /// @todo PDFix = PdfDoc::GetNumPages()
}
