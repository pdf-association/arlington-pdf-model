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
#include "LRParsePredicate.h"
#include "utils.h"

#include <cassert>
#include <vector>
#include <set>
#include <limits>
#include <climits>
#include <bitset>
#include <math.h>

using namespace ArlingtonPDFShim;
namespace fs = std::filesystem;

/// @brief \#define PP_FN_DEBUG to get verbose debugging to std::cout when calculating predicate functions
#undef PP_FN_DEBUG

/// @brief \#define PP_AST_DEBUG to get verbose debugging to std::cout when processing the predicate AST Tree
#undef PP_AST_DEBUG


/// @brief Constructor. Calculates some details about the PDF file
CPDFFile::CPDFFile(const fs::path& pdf_file, ArlingtonPDFSDK& pdf_sdk, const std::string& forced_ver, const std::vector<std::string>& extns)
    : pdf_filename(pdf_file), pdfsdk(pdf_sdk), trailer_size(INT_MAX),
      latest_feature_version("1.0"), deprecated(false), fully_implemented(true), exact_version_compare(false)
{
    if (forced_ver.size() > 0) {
        if (forced_ver == "exact")
            exact_version_compare = true;
        else
            forced_version = forced_ver;
    }

    // Copy across the list of supported extensions
    extensions = extns;

    // Get physical file size, reduced to an int for simplicity
    filesize_bytes = (int)fs::file_size(pdf_filename);

    // Get PDF version from file header.  No sanity checking is done.
    pdf_header_version = pdfsdk.get_pdf_version();

    auto trailer = pdfsdk.get_trailer();
    if (trailer != nullptr) {
        // Get the trailer Size key
        if (trailer->has_key(L"Size")) {
            ArlPDFObject* sz = trailer->get_value(L"Size");
            if (sz != nullptr) {
                if (((sz->get_object_type() == PDFObjectType::ArlPDFObjTypeNumber)) && ((ArlPDFNumber*)sz)->is_integer_value()) {
                    trailer_size = ((ArlPDFNumber*)sz)->get_integer_value();
                }
                delete sz;
            }
        }

        // Get the Document Catalog Version, if it exists. No sanity checking is done.
        {
            auto doccat = pdfsdk.get_document_catalog();
            ArlPDFObject* doc_cat_ver_obj = doccat->get_value(L"Version");

            if (doc_cat_ver_obj != nullptr) {
                if (doc_cat_ver_obj->get_object_type() == PDFObjectType::ArlPDFObjTypeName) {
                    std::wstring doccat_ver = ((ArlPDFName*)doc_cat_ver_obj)->get_value();
                    pdf_catalog_version = ToUtf8(doccat_ver);
                }
                delete doc_cat_ver_obj;
            }
        }
    }
}


/// @brief Split an Arlington key path (e.g. Catalog::Names::Dests) into a vector of keys.
/// 
/// @param[in]   key  an Arlington key which might be a key path
/// 
/// @returns     a vector of each key in the path
std::vector<std::string> CPDFFile::split_key_path(std::string key) 
{
    std::vector<std::string>    keys;

    auto sep = key.find("::");
    if (sep != std::string::npos) {
        // Have a multi-part path with "::" separators between keys
        do {
            keys.push_back(key.substr(0, sep));
            key = key.substr(sep + 2);
            sep = key.find("::");
        } while (sep != std::string::npos);
        keys.push_back(key);
    }
    else {
        // No "::" path separator
        keys.push_back(key);
    }

    // Only the FINAL portion of a path can have the '@' for value-of 
    for (size_t i = 0; i < keys.size() - 1; i++) {
        assert(keys[i][0] != '@');
    }

    // "parent" and "trailer" are pre-defined and can only be in the very 1st portion. 
    for (size_t i = 1; i < keys.size(); i++) {
        assert((keys[i] != "parent") && (keys[i] != "trailer"));
    }

    return keys;
}



/// @brief  Gets the object mentioned by an Arlington path.
/// 
/// @param[in]   parent           a parent object (such that a single path is IN this object)
/// @param[in]   arlpath          the Arlington path broken down as a vector
/// 
/// @returns   the object for the path or nullptr if it doesn't exist
ArlPDFObject* CPDFFile::get_object_for_path(ArlPDFObject* parent, const std::vector<std::string>& arlpath) {
    assert(parent != nullptr);
    assert(arlpath.size() > 0);

    std::vector<std::string> path = arlpath;
    ArlPDFObject*            obj = parent;
    bool                     delete_obj = false;
    PDFObjectType            obj_type;

    auto path_len = path.size();
    if (path[path_len - 1][0] == '@')           // Remove any '@' from last portion so it reverts to a key name / array index
        path[path_len - 1] = path[path_len - 1].substr(1);

    // some special case handling
    if ((path_len >= 2) && (path[0] == "trailer") && (path[1] == "Catalog")) {
        obj = pdfsdk.get_document_catalog();
        path.erase(path.begin());
        path.erase(path.begin());
    }
    else if ((path_len >= 1) && (path[0] == "trailer")) {
        obj = pdfsdk.get_trailer();
        path.erase(path.begin());
    }

    do {
        if (path[0] == "parent") {
            ///  @todo  "parent::key" or "parent::parent::key" is not supported...
            if (delete_obj) 
                delete obj;
            fully_implemented = false;
            return nullptr;
        }

        obj_type = obj->get_object_type();
        switch (obj_type) {
            case PDFObjectType::ArlPDFObjTypeArray:
                {
                    ArlPDFObject* a;
                    if (path[0] != "*") {
                        int idx = key_to_array_index(path[0]);
                        a = ((ArlPDFArray*)obj)->get_value(idx);
                    }
                    else
                        a = ((ArlPDFArray*)obj)->get_value(0);
                    if (a == nullptr) {
                        if (delete_obj) 
                            delete obj;
                        return nullptr;
                    }
                    if (delete_obj) 
                        delete obj;
                    obj = a;
                    delete_obj = true;
                }
                break;
            case PDFObjectType::ArlPDFObjTypeDictionary:
                {
                    ArlPDFObject* a;
                    if (path[0] != "*")
                        a = ((ArlPDFDictionary*)obj)->get_value(ToWString(path[0]));
                    else {
                        auto key = ((ArlPDFDictionary*)obj)->get_key_name_by_index(0);
                        a = ((ArlPDFDictionary*)obj)->get_value(key);
                    }
                    if (a == nullptr) {
                        if (delete_obj) 
                            delete obj;
                        return nullptr;
                    }
                    if (delete_obj) 
                        delete obj;
                    obj = a;
                    delete_obj = true;
                }
                break;
            case PDFObjectType::ArlPDFObjTypeStream:
                {
                    ArlPDFDictionary* dict = ((ArlPDFStream*)obj)->get_dictionary();
                    if (dict != nullptr) {
                        ArlPDFObject* a;
                        if (path[0] != "*")
                            a = dict->get_value(ToWString(path[0]));
                        else {
                            auto key = dict->get_key_name_by_index(0);
                            a = dict->get_value(key);
                        }
                        delete dict;
                        if (delete_obj)
                            delete obj;
                        if (a == nullptr) 
                            return nullptr;
                        obj = a;
                        delete_obj = true;
                    }
                }
                break;
            default:
                if (delete_obj) 
                    delete obj;
                return nullptr;
        } // switch
        assert(obj != nullptr);
        path.erase(path.begin()); // remove the head off the path and iterate
    } while (path.size() > 0);

    assert(delete_obj); // THIS WILL MAKE MEMORY MANAGEMENT REALLY BAD!!!
    return obj;
}


/// @brief Convert an integer or double node to numeric representation.
/// Internally throws and catches exceptions.
/// 
/// @param[in] node   AST Node - should be integer or double
/// 
/// @returns double or NaN (std::numeric_limits<double>::quiet_NaN())
double CPDFFile::convert_node_to_double(const ASTNode* node) {
    assert(node != nullptr);
    assert((node->type == ASTNodeType::ASTNT_ConstNum) || (node->type == ASTNodeType::ASTNT_ConstInt));
    double val = 0.0;

    try {
        val = std::stod(node->node);
    }
    catch (...) {
#ifdef PP_AST_DEBUG
        std::cout << "floating point exception for " << node->node << "!" << std::endl;
#endif
        val = std::numeric_limits<double>::quiet_NaN();
    }
    return val;
}


/// @brief Processes an AST-Node by recursively descending and calculating the left and right predicates.
/// Because keys referenced in predicates can be missing in a PDF this gets complicated...
/// 
/// If a key is not present in a PDF then an expression referencing that key (such as "@key" or fn:Predicate(key)) 
/// cannot be determined. In this case nullptr is returned (as an ASTNode*) representing the indeterminate nature. 
/// If the key IS present in the PDF then predicates, formulae, comparisons, etc.
/// can be performed and a valid ASTNode* is returned. An obvious exception to this rule is fn:IsPresent() and there
/// are a few others (e.g. numeric predicates such as fn:XxxLength() which will return -1 on such error).
/// 
/// When doing logical operators, a nullptr operand (meaning an indeterminate result) can be further processed by 
/// evaluating the other half of the expression for OR (" || ") - there is NO short-circuit boolean evaluation here. 
/// But this is not possible with AND (" && ") since both sides need to exist. Mathematical operations and comparisons 
/// also cannot be processed if either of the operands is indeterminate.
/// 
/// Optional arguments vs indeterminate arguments can be identified by examining in_ast->arg[x]. If it is nullptr
/// then the optional argument was NOT present. If in_ast->arg[x] is not nullptr, but one or both of out_left or 
/// out_right variables are nullptr then this indicates indeterminism.
///
/// @param[in]  parent           parent PDF object (e.g. the dictionary which contains 'obj' as an entry or array as element)
/// @param[in]  obj              PDF object related to the predicate. Never nullptr.
/// @param[in]  in_ast           input AST tree. Never nullptr.
/// @param[in]  tsv_data         the row of TSV data that is being processed
/// @param[in]  key_idx          the index into the Arlington 'Key' field of the TSV data (>=0)
/// @param[in]  type_idx         the index into the Arlington 'Type' field of 'Key' field of the TSV data  (>=0)
/// @param[in]  depth            depth counter for recursion (visual indentation) (>=0)
/// @param[in]  use_default_values  true if Default Values should be used when a key-value (@Key) is not present
/// 
/// @returns   Output AST (always valid) or nullptr if indeterminate 
ASTNode* CPDFFile::ProcessPredicate(ArlPDFObject* parent, ArlPDFObject* obj, const ASTNode* in_ast, const int key_idx, const ArlTSVmatrix& tsv_data, const int type_idx, int depth, const bool use_default_values)
{
    assert(parent != nullptr);
    assert(obj != nullptr);
    assert(in_ast != nullptr);
    assert(in_ast->valid());
    assert(key_idx >= 0);
    assert(type_idx >= 0);

    ASTNode* out = new ASTNode;
    ASTNode* out_left = nullptr;
    ASTNode* out_right = nullptr;

#ifdef PP_AST_DEBUG
    std::cout << std::string(depth * 2, ' ') << "In:  " << *in_ast << std::endl;
#endif

    if (depth == 0) {
        // reset deprecation & implementation detection at the start of possible recursion
        fully_implemented = true;
        deprecated = false; 
    }

    if (in_ast->arg[0] != nullptr) {
        bool current_processing_state = fully_implemented;
        fully_implemented = true;
        out_left = ProcessPredicate(parent, obj, in_ast->arg[0], key_idx, tsv_data, type_idx, depth + 1, use_default_values);
        fully_implemented = current_processing_state && fully_implemented;
#ifdef PP_AST_DEBUG
        if (out_left != nullptr) { std::cout << std::string(depth * 2, ' ') << " Out-Left:  " << *out_left << std::endl; }
        // Force calls to PDF SDK to make sure everything is OK
        (void)obj->get_object_type();
        (void)parent->get_object_type();
#endif 
    }

    if (in_ast->arg[1] != nullptr) {
        bool current_processing_state = fully_implemented;
        fully_implemented = true;
        out_right = ProcessPredicate(parent, obj, in_ast->arg[1], key_idx, tsv_data, type_idx, depth + 1, use_default_values);
        fully_implemented = current_processing_state && fully_implemented;
#ifdef PP_AST_DEBUG
        if (out_right != nullptr) { std::cout << std::string(depth * 2, ' ') << " Out-Right:  " << *out_right << std::endl; }
        // Force calls to PDF SDK to make sure everything is OK
        (void)obj->get_object_type();
        (void)parent->get_object_type();
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
        // Predicates can take up to 2 arguments: out_left, out_right.
        // If there is one argument only, then assert(out_right == nullptr)
        // Arguments have been reduced by the recursion calls above, but in some
        // cases (PDF file errors) ASTNodes might end up as nullptr.
        // Assertions are used where this implementation assumes the current usage 
        // of predicates in the current Arlington PDF model
        //
        //    grep -Po "fn:<predicate-name>\([^\t]*\)" *
        //
        if (in_ast->node == "fn:AlwaysUnencrypted(") {
            // no arguments
            assert(out_left == nullptr);
            assert(out_right == nullptr);
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = (fn_AlwaysUnencrypted(obj) ? "true" : "false");
        }
        else if (in_ast->node == "fn:ArrayLength(") {
            // 1 argument: name of key (or an integer array index) which is an array, could be indeterminate
            assert(out_right == nullptr);
            int len = fn_ArrayLength(parent, out_left);
            if (len >= 0) {
                // Valid length
                out->type = ASTNodeType::ASTNT_ConstInt;
                out->node = std::to_string(len);
            }
            else {
                // Invalid length - most likely key not present...
                delete out;
                out = nullptr;
            }
        }
        else if (in_ast->node == "fn:ArraySortAscending(") {
            // 2 arguments: name of key key (or an integer array index) which is the array, step size
            assert(out_left != nullptr);
            assert(out_right != nullptr);
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = (fn_ArraySortAscending(parent, out_left, out_right) ? "true" : "false");
        }
        else if (in_ast->node == "fn:BeforeVersion(") {
            // 1st arg is required (a PDF version). 2nd arg is optional.
            delete out;
            out = fn_BeforeVersion(out_left, out_right);
        }
        else if (in_ast->node == "fn:BitClear(") {
            // 1 argument required: bit number 1-32. NEVER indeterminate.
            assert(out_left != nullptr);
            assert(out_right == nullptr);
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_BitClear(obj, out_left) ? "true" : "false";
        }
        else if (in_ast->node == "fn:BitSet(") {
            // 1 argument required: bit number 1-32. NEVER indeterminate.
            assert(out_left != nullptr);
            assert(out_right == nullptr);
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_BitSet(obj, out_left) ? "true" : "false";
        }
        else if (in_ast->node == "fn:BitsClear(") {
            // 2 arguments: low bit, high bit. NEVER indeterminate.
            assert(out_left != nullptr);
            assert(out_right != nullptr);
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_BitsClear(obj, out_left, out_right) ? "true" : "false";
        }
        else if (in_ast->node == "fn:BitsSet(") {
            // 2 arguments: low bit, high bit. NEVER indeterminate.
            assert(out_left != nullptr);
            assert(out_right != nullptr);
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_BitsSet(obj, out_left, out_right) ? "true" : "false";
        }
        else if (in_ast->node == "fn:DefaultValue(") {
            // 2 arguments: condition, what the default value should be when condition is true
            // 2nd argument is never indeterminate.
            delete out;
            out = fn_DefaultValue(out_left, out_right);
        }
        else if (in_ast->node == "fn:Deprecated(") {
            // 2 arguments: version, what was deprecated in the version (1st argument)
            delete out;
            out = fn_Deprecated(out_left, out_right);
        }
        else if (in_ast->node == "fn:Eval(") {
            // 1 argument, which is the reduced expression. Arg can be nullptr due to things such as missing keys
            assert(out_right == nullptr);
            // Just strip this off...
            if (out_left != nullptr) {
                out->type = out_left->type;
                out->node = out_left->node;
            }
            else {
                delete out;
                out = nullptr;
            }
        }
        else if (in_ast->node == "fn:Extension(") {
            // 1 or 2 arguments: extension name (required), optional value (when used in fields except "SinceVersion")
            delete out;
            out = fn_Extension(out_left, out_right);
        }
        else if (in_ast->node == "fn:FileSize(") {
            // no arguments
            assert(out_left == nullptr);
            assert(out_right == nullptr);
            out->type = ASTNodeType::ASTNT_ConstInt;
            out->node = std::to_string(fn_FileSize());
        }
        else if (in_ast->node == "fn:FontHasLatinChars(") {
            // no arguments
            assert(out_left == nullptr);
            assert(out_right == nullptr);
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_FontHasLatinChars(obj) ? "true" : "false";
        }
        else if (in_ast->node == "fn:HasProcessColorants(") {
            // one argument - an array object of names
            assert(out_left != nullptr);
            assert(out_right == nullptr);
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_HasProcessColorants(parent, out_left) ? "true" : "false";
        }
        else if (in_ast->node == "fn:HasSpotColorants(") {
            // one argument - an array object of names
            assert(out_left != nullptr);
            assert(out_right == nullptr);
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_HasSpotColorants(parent, out_left) ? "true" : "false";
        }
        else if (in_ast->node == "fn:Ignore(") {
            /// @todo - implement ignoring things...
            // 1 argument which is the condition for ignoring, which can be nullptr due to reduction/indeterminism
            assert(out_right == nullptr);
            // just reduce to true as we will still report issues
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = "true";
        }
        else if (in_ast->node == "fn:ImageIsStructContentItem(") {
            // no arguments
            assert(out_left == nullptr);
            assert(out_right == nullptr);
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_ImageIsStructContentItem(obj) ? "true" : "false";
        }
        else if (in_ast->node == "fn:ImplementationDependent(") {
            // no arguments
            assert(out_left == nullptr);
            assert(out_right == nullptr);
            // just return true
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = "true";
        }
        else if (in_ast->node == "fn:InMap(") {
            // 1 argument which is the key name key (or an integer array index) of the map, which can be nullptr due to reduction/indeterminism
            assert(out_left != nullptr);
            assert(out_right == nullptr);
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_InMap(obj, out_left) ? "true" : "false";
        }
        else if (in_ast->node == "fn:IsAssociatedFile(") {
            // no arguments
            assert(out_left == nullptr);
            assert(out_right == nullptr);
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_IsAssociatedFile(obj) ? "true" : "false";
        }
        else if (in_ast->node == "fn:IsEncryptedWrapper(") {
            // no arguments
            assert(out_left == nullptr);
            assert(out_right == nullptr);
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_IsEncryptedWrapper() ? "true" : "false";
        }
        else if (in_ast->node == "fn:IsFieldName(") {
            // one argument: key-value
            assert(out_left != nullptr);
            assert(out_right == nullptr);
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_IsFieldName(obj) ? "true" : "false";
        }
        else if (in_ast->node == "fn:IsHexString(") {
            // no arguments
            assert(out_left == nullptr);
            assert(out_right == nullptr);
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_IsHexString(obj) ? "true" : "false";
        }
        else if (in_ast->node == "fn:IsLastInNumberFormatArray(") {
            // 1 argument which is the key name key (or an integer array index) of an array. COULD be indeterminate.
            assert(out_right == nullptr);
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_IsLastInArray(parent, obj, out_left) ? "true" : "false";
        }
        else if (in_ast->node == "fn:IsMeaningful(") {
            // 1 argument which is a condition under which something is "meaningful"
            assert(out_right == nullptr);
            // everything is meaningful when we are checking
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = "true";
        }
        else if (in_ast->node == "fn:IsPDFTagged(") {
            // no arguments
            assert(out_left == nullptr);
            assert(out_right == nullptr);
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_IsPDFTagged() ? "true" : "false";
        }
        else if (in_ast->node == "fn:IsPDFVersion(") {
            // 2 arguments: version, and whatever exists only in a single PDF version. COULD be indeterminate
            delete out;
            out = fn_IsPDFVersion(out_left, out_right);
        }
        else if (in_ast->node == "fn:IsPresent(") {
            // Need to check in_ast->arg[] to see if 1 or 2 argument version first:
            // If 1 argument: condition that has already been reduced to true/false, or a key name, or could be 
            // nullptr/indeterminate (e.g. missing key in an expression). In that case the result is a boolean
            // false.
            // If 2 arguments: 2nd argument (condition) only applies if the 1st argument resolved to true. But due
            // to missing keys the 1st argument could have resolved to nullptr in which case the result is a nullptr.
            //
            // Note that key names here can be integers (array index), wildcard '*' or integer+'*'!! 
            if ((in_ast->arg[0] != nullptr) && (in_ast->arg[1] != nullptr)) {
                // 2 argument version
                bool l = false;
                if (out_left != nullptr) {
                    if ((out_left->type == ASTNodeType::ASTNT_Key) || (out_left->type == ASTNodeType::ASTNT_ConstInt))
                        l = fn_IsPresent(parent, out_left->node);
                    else {
                        // Was probably a condition...
                        assert(out_left->type == ASTNodeType::ASTNT_ConstPDFBoolean);
                        l = (out_left->node == "true");
                    }
                }
                if (l) {
                    out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                    out->node = "false";
                    if (out_right != nullptr) {
                        assert(out_right->type == ASTNodeType::ASTNT_ConstPDFBoolean);
                        out->node = out_right->node;
                    }
                }
                else {
                    // 1st argument didn't exist/wasn't true so ignore 2nd argument. NOT FALSE!!!
                    delete out;
                    out = nullptr;
                }
            }
            else {
                // 1 argument version
                assert(out_right == nullptr);
                out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                out->node = "false";
                if (out_left != nullptr) {
                    if ((out_left->type == ASTNodeType::ASTNT_Key) || (out_left->type == ASTNodeType::ASTNT_ConstInt))
                        out->node = (fn_IsPresent(parent, out_left->node) ? "true" : "false");
                    else {
                        assert(out_left->type == ASTNodeType::ASTNT_ConstPDFBoolean);
                        out->node = out_left->node;
                    }
                }
            }
        }
        else if (in_ast->node == "fn:IsRequired(") {
            // 1 argument: condition that has already been reduced to true/false, or could be nullptr (e.g. missing key)
            if (out_left != nullptr) {
                assert(out_right == nullptr);
                assert(out_left->type == ASTNodeType::ASTNT_ConstPDFBoolean);
                out->node = out_left->node;
            }
            else
                out->node = "false";
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
        }
        else if (in_ast->node == "fn:KeyNameIsColorant(") {
            // no arguments
            assert(out_left == nullptr);
            assert(out_right == nullptr);
            // assume everything is a valid colorant
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = "true";
        }
        else if (in_ast->node == "fn:MustBeDirect(") {
            // optional 1 argument, which is a key/array index, an expression (reduced, possibly to nothing), or nothing
            assert(out_right == nullptr);
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            if (in_ast->arg[0] == nullptr) {
                // fn:MustBeDirect() - no arguments
                out->node = "true";
            }
            else {
                // there was an argument but may have been reduced to nullptr due to missing key, etc.
                if (out_left != nullptr)
                    out->node = fn_MustBeDirect(parent, obj, out_left) ? "true" : "false";
                else {
                    // indeterminate... (was an argument that got reduced to nullptr)
                    delete out;
                    out = nullptr;
                }
            }
        }
        else if (in_ast->node == "fn:MustBeIndirect(") {
            // optional 1 argument, which is a key/array index, an expression (reduced, possibly to nothing), or nothing
            assert(out_right == nullptr); 
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            if (in_ast->arg[0] == nullptr) {
                // fn:MustBeIndirect()  - no arguments
                out->node = "true";
            }
            else {
                // there was an argument but may have been reduced to nullptr due to missing key, etc.
                if (out_left != nullptr)
                    out->node = fn_MustBeDirect(parent, obj, out_left) ? "false" : "true";
                else {
                    // indeterminate... (was an argument that got reduced to nullptr)
                    delete out;
                    out = nullptr;
                }
            }
        }
        else if (in_ast->node == "fn:NoCycle(") {
            // no arguments
            assert(out_left == nullptr);
            assert(out_right == nullptr);
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_NoCycle(obj, tsv_data[key_idx][TSV_KEYNAME]) ? "true" : "false";
        }
        else if (in_ast->node == "fn:Not(") {
            // 1 argument: invert the condition (could have been reduced to indeterminate)
            assert(out_right == nullptr);
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            if (out_left != nullptr) {
                assert(out_left->type == ASTNodeType::ASTNT_ConstPDFBoolean);
                out->node = (out_left->node == "false") ? "true" : "false";
            }
            else {
                delete out;
                out = nullptr;
            }
        }
        else if (in_ast->node == "fn:NotStandard14Font(") {
            // no arguments
            assert(out_left == nullptr);
            assert(out_right == nullptr);
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_NotStandard14Font(obj) ? "true" : "false";
        }
        else if (in_ast->node == "fn:NumberOfPages(") {
            // no arguments
            assert(out_left == nullptr);
            assert(out_right == nullptr);
            out->type = ASTNodeType::ASTNT_ConstInt;
            out->node = std::to_string(fn_NumberOfPages());
        }
        else if (in_ast->node == "fn:PageContainsStructContentItems(") {
            // no arguments
            assert(out_left == nullptr);
            assert(out_right == nullptr);
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_PageContainsStructContentItems(obj) ? "true" : "false";
        }
        else if (in_ast->node == "fn:PageProperty(") {
            // 2 arguments: the page, a key (NEVER an array index!) on that page. Either could be nullptr! 
            delete out;
            out = fn_PageProperty(parent, out_left, out_right);
        }
        else if (in_ast->node == "fn:RectHeight(") {
            // 1 argument: key or integer array index of the rectangle. Could be indeterminate.
            assert(out_right == nullptr);
            out->type = ASTNodeType::ASTNT_ConstNum;
            out->node = std::to_string(fn_RectHeight(parent, out_left));
        }
        else if (in_ast->node == "fn:RectWidth(") {
            // 1 argument: key or integer array index of the rectangle. Could be indeterminate.
            assert(out_right == nullptr);
            out->type = ASTNodeType::ASTNT_ConstNum;
            out->node = std::to_string(fn_RectWidth(parent, out_left));
        }
        else if (in_ast->node == "fn:RequiredValue(") {
            delete out;
            out = fn_RequiredValue(obj, out_left, out_right);
        }
        else if (in_ast->node == "fn:SinceVersion(") {
            // 2 args: version, and thing that was introduced
            delete out;
            out = fn_SinceVersion(out_left, out_right);
        }
        else if (in_ast->node == "fn:StreamLength(") {
            // 1 argument: key name or integer array index of the stream
            assert(out_left != nullptr);
            assert(out_right == nullptr);
            out->type = ASTNodeType::ASTNT_ConstInt;
            int len = fn_StreamLength(parent, out_left);
            if (len >= 0) {
                // Valid length
                out->type = ASTNodeType::ASTNT_ConstInt;
                out->node = std::to_string(len);
            }
            else {
                // Invalid length - most likely key not present...
                delete out;
                out = nullptr;
            }
        }
        else if (in_ast->node == "fn:StringLength(") {
            // 1 argument: key name or integer array index of the string
            assert(out_right == nullptr);
            out->type = ASTNodeType::ASTNT_ConstInt;
            int len = fn_StringLength(parent, out_left);
            if (len >= 0) {
                // Valid length
                out->type = ASTNodeType::ASTNT_ConstInt;
                out->node = std::to_string(len);
            }
            else {
                // Invalid length - most likely key not present...
                delete out;
                out = nullptr;
            }
        }
        else if (in_ast->node == "fn:Contains(") {
            // 2 arguments: key name or integer array index and a value, but either may have been reduced
            if (out_left == nullptr) {
                delete out_right;
                out_right = nullptr;
            }
            out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            out->node = fn_Contains(obj, out_left, out_right) ? "true" : "false";
        }
        else {
            assert(false && "unrecognized predicate function!");
            fully_implemented = false;
            delete out;
            out = nullptr;
        }
    }
    break;

        case ASTNodeType::ASTNT_MathComp:
            {
                // Math/logic comparison operators - cannot be start of an AST!
                // Should have 2 operands (left, right) but due to predicate reduction this can reduce to just 
                // one in which case the output is nullptr also, since cannot make any comparison.
                out->type = ASTNodeType::ASTNT_ConstPDFBoolean;

                if ((out_left == nullptr) || (out_right == nullptr)) {
                    delete out;
                    out = nullptr;
                    break;
                }
                else if (in_ast->node == "==") {
                    // equality - could be numeric, logical, etc.
                    if (out_left->type == out_right->type) {
                        out->node = (out_left->node == out_right->node) ? "true" : "false";
                        break;
                    }
                    // else fallthrough and up-convert to doubles for math op
                }
                else if (in_ast->node == "!=") {
                    // inequality - could be numeric, logical, etc.
                    if (out_left->type == out_right->type) {
                        out->node = (out_left->node != out_right->node) ? "true" : "false";
                        break;
                    }
                    // else fallthrough and up-convert to doubles for math op
                }

                // Numeric comparisons between an integer and a real - promote to real
                double left  = convert_node_to_double(out_left);
                double right = convert_node_to_double(out_right);

                if ((left == std::numeric_limits<double>::quiet_NaN()) || (right == std::numeric_limits<double>::quiet_NaN())) {
                    delete out;
                    out = nullptr;
                    break;
                }

                if (in_ast->node == "==") {
                    // equality with tolerance (numeric only)
                    out->node = (fabs(left - right) <= ArlNumberTolerance) ? "true" : "false";
                }
                else if (in_ast->node == "!=") {
                    // inequality with tolerance(numeric only)
                    out->node = (fabs(left - right) > ArlNumberTolerance) ? "true" : "false";
                }
                else if (in_ast->node == "<=") {
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
                    out = nullptr;
                }
            }
            break;

        case ASTNodeType::ASTNT_MathOp:
                {
                // Math operators: "+", " - ", "*", " mod " (SPACEs either side on some)
                // Math operators should have 2 operands (left, right) but due to reductions,
                // this can reduce to just one in which case the output is just the non-nullptr value.
                // If both got reduced then reduce to "true".
                if ((out_left != nullptr) && (out_right == nullptr)) {
                    out->type = out_left->type;
                    out->node = out_left->node;
                    break;
                }
                else if ((out_left == nullptr) && (out_right != nullptr)) {
                    out->type = out_right->type;
                    out->node = out_right->node;
                    // swap nodes to be valid()
                    out_left = out_right;
                    out_right = nullptr;
                    break;
                }
                else if ((out_left == nullptr) && (out_right == nullptr)) {
                    out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                    out->node = "true";
                    break;
                }

                assert((out_left != nullptr) && (out_right != nullptr));
                double left = std::stod(out_left->node);
                double right = std::stod(out_right->node);

                // Work out typing - integer vs number
                if ((out_left->type == ASTNodeType::ASTNT_ConstInt) && (out_right->type == ASTNodeType::ASTNT_ConstInt))
                    out->type = ASTNodeType::ASTNT_ConstInt;
                else
                    out->type = ASTNodeType::ASTNT_ConstNum;

                if ((in_ast->node == "+") || (in_ast->node == " + ")) { // addition
                    if (out->type == ASTNodeType::ASTNT_ConstInt)
                        out->node = std::to_string(int(left + right));
                    else
                        out->node = std::to_string(left + right);
                }
                else if ((in_ast->node == "-") || (in_ast->node == " - ")) { // subtraction (NEVER unary negation)
                    if (out->type == ASTNodeType::ASTNT_ConstInt)
                        out->node = std::to_string(int(left - right));
                    else
                        out->node = std::to_string(left - right);
                }
                else if ((in_ast->node == "*") || (in_ast->node == " * ")) { // multiply
                    if (out->type == ASTNodeType::ASTNT_ConstInt)
                        out->node = std::to_string(int(left * right));
                    else
                        out->node = std::to_string(left * right);
                }
                else if (in_ast->node == " mod ") { // modulo
                    out->type = ASTNodeType::ASTNT_ConstInt;
                    out->node = std::to_string(int(left) % int(right));
                }
                else {
                    assert(false && "unexpected math operator!");
                    delete out;
                    out = nullptr;
                }
            }
            break;

        case ASTNodeType::ASTNT_LogicalOp:
            {
                // Logical operators - should have 2 operands (left, right) but due to reductions,
                // this can reduce to just one in which case the output is just the non-nullptr boolean.
                // If both got reduced then reduce to "true".
                if ((out_left != nullptr) && (out_right == nullptr)) {
                    assert(out_left->type == ASTNodeType::ASTNT_ConstPDFBoolean);
                    out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                    out->node = out_left->node;
                    break;
                }
                else if ((out_left == nullptr) && (out_right != nullptr) && (out_right->type == ASTNodeType::ASTNT_ConstPDFBoolean)) {
                    out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                    out->node = out_right->node;
                    // swap nodes
                    out_left = out_right;
                    out_right = nullptr;
                    break;
                }
                else if ((out_left == nullptr) && (out_right == nullptr)) {
                    out->type = ASTNodeType::ASTNT_ConstPDFBoolean;
                    out->node = "true";
                    break;
                }

                if (((out_left == nullptr) || (out_left->type == ASTNodeType::ASTNT_ConstNum)) || (out_right->type == ASTNodeType::ASTNT_ConstNum)) {
                    // Coming from SinceVersion field: fn:Eval(fn:Extension(PDF_VT2,1.6) || 2.0) type expression
                    assert(in_ast->node == " || ");
                    out->type = ASTNodeType::ASTNT_ConstNum;
                    if (out_left != nullptr)
                        out->node = out_left->node;
                    else
                        out->node = out_right->node;
                    delete out_left;
                    out_left = nullptr;
                    delete out_right;
                    out_right = nullptr;
                    break;
                }
                else {
                    assert((out_left->type == ASTNodeType::ASTNT_ConstPDFBoolean) && (out_right->type == ASTNodeType::ASTNT_ConstPDFBoolean));
                    if (in_ast->node == " && ") {
                        // logical AND - if only 1 arg then it is that arg
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
                        out = nullptr;
                    }
                }
            }
            break;

        case ASTNodeType::ASTNT_KeyValue: // "@keyname" - key name or integer array index ("@1")
            {
                auto key_parts = split_key_path(in_ast->node);
                assert(key_parts[key_parts.size() -1 ][0] == '@');
                key_parts[key_parts.size() - 1] = key_parts[key_parts.size() - 1].substr(1);  // strip the '@' off

                // Object to get value from
                ArlPDFObject* val = nullptr;
                bool delete_val = false;

                // To debug a specific predicate, uncomment and modify the following code. Add breakpoint to the 2nd line.
                // if (key_parts[key_parts.size() - 1] == "FontName")
                //    delete_val = delete_val;

                // Optimize for simple self-reference (where @key and current key are the same)
                bool self_refer = (key_parts.size() == 1) && (tsv_data[key_idx][TSV_KEYNAME] == key_parts[key_parts.size() - 1]);
                if (!self_refer) {
                    val = get_object_for_path(parent, key_parts);
                    delete_val = true;
                }
                else 
                    val = obj;  // Self-reference

                // Don't have a value from the PDF for "@Key", try getting "DefaultValue" for "Key" from Arlington.
                // Only want to use Default Values for SpecialCase processing. When processing Required field
                // this should not required - it would indicate a logical error in the PDF specification! 
                // See Issue #30: https://github.com/pdf-association/arlington-pdf-model/issues/30#issuecomment-1276804889
                if ((val == nullptr) && (key_parts.size() == 1)) {
                    bool got_dv = false;
                    if (use_default_values) {
                        for (int i = 0; i < (int)tsv_data.size(); i++)
                            if ((tsv_data[i][TSV_KEYNAME] == key_parts[0]) && (tsv_data[i][TSV_DEFAULTVALUE] != "")) {
                                delete out;
                                out = new ASTNode;
                                std::string s = LRParsePredicate(tsv_data[i][TSV_DEFAULTVALUE], out);
                                assert(s.size() == 0);
                                assert(out->valid());
                                got_dv = true;
                                break;
                            }
                    }
                    if (!got_dv) {
                        // Return nullptr if @key doesn't exist
                        delete out;
                        out = nullptr;
                    }
                }
                else {
                    ASTNode *tmp = convert_basic_object_to_ast(val);
                    if ((tmp == nullptr) && (in_ast->node.find("parent::") == std::string::npos)) {
                        // @Key reference was to a complex PDF object (array, dictionary, stream) - or PDF null object
                        // Re-instate the key (without the '@') so containing predicate can handle. See fn_Contains().
                        /// @todo - does not support Arlington paths with "parent::"
                        assert(out != nullptr); 
                        out->type = ASTNodeType::ASTNT_Key;
                        out->node = key_parts[0];
                        for (int i = 1; i < (int)key_parts.size(); i++)
                            out->node = out->node + "::" + key_parts[i];
                    }
                    else {
                        assert((tmp == nullptr) || tmp->valid());
                        delete out;
                        out = tmp;
                    }
                    assert((out == nullptr) || out->valid());
                }
                if (delete_val)
                    delete val;
            }
            break;

        case ASTNodeType::ASTNT_Unknown:
        case ASTNodeType::ASTNT_Type:
        default:
            // Likely a parsing error!
            assert(false && "unexpected AST node while recursing!");
            fully_implemented = false;
            delete out;
            out = nullptr;
            break;
    } // switch

    if (out != nullptr) {
        out->arg[0] = out_left;
        out->arg[1] = out_right;
#ifdef PP_AST_DEBUG
        std::cout << std::string(depth * 2, ' ') << "Out: " << *out << std::endl;
#endif
        assert(out->valid());
    }
    else {
        delete out_left;
        delete out_right;
#ifdef PP_AST_DEBUG
        std::cout << std::string(depth * 2, ' ') << "Out: nullptr" << std::endl;
#endif 
    }

    return out;
}


/// @brief Convert a basic PDF object (boolean, name, number, string) into an AST equivalent node.
/// Complex objects (array, dictionary, stream) reduce to a boolean "true" (meaning object exists).
/// The PDF null object reduces to the boolean "false" (meaning object doesn't exist)
/// 
/// @param[in] obj   PDF object. Can be nullptr.
/// 
/// @returns AST-Node equivalent data structure or nullptr.
ASTNode* CPDFFile::convert_basic_object_to_ast(ArlPDFObject* obj) 
{
    if (obj == nullptr)
        return nullptr;

    PDFObjectType obj_type = obj->get_object_type();
    ASTNode* ast_obj = new ASTNode;

    switch (obj_type) {
    case PDFObjectType::ArlPDFObjTypeName:
        ast_obj->type = ASTNodeType::ASTNT_Key;
        ast_obj->node = ToUtf8(((ArlPDFName*)obj)->get_value());
        return ast_obj;

    case PDFObjectType::ArlPDFObjTypeNumber:
        if (((ArlPDFNumber*)obj)->is_integer_value()) {
            ast_obj->type = ASTNodeType::ASTNT_ConstInt;
            ast_obj->node = std::to_string(((ArlPDFNumber*)obj)->get_integer_value());
        }
        else {
            ast_obj->type = ASTNodeType::ASTNT_ConstNum;
            ast_obj->node = std::to_string(((ArlPDFNumber*)obj)->get_value());
        }
        return ast_obj;

    case PDFObjectType::ArlPDFObjTypeBoolean:
        ast_obj->type = ASTNodeType::ASTNT_ConstPDFBoolean;
        ast_obj->node = ((ArlPDFBoolean*)obj)->get_value() ? "true" : "false";
        return ast_obj;

    case PDFObjectType::ArlPDFObjTypeString:
        ast_obj->type = ASTNodeType::ASTNT_ConstString;
        ast_obj->node = ToUtf8(((ArlPDFString*)obj)->get_value());
        return ast_obj;

    case PDFObjectType::ArlPDFObjTypeStream:
    case PDFObjectType::ArlPDFObjTypeArray:
    case PDFObjectType::ArlPDFObjTypeDictionary:
        // Caller needs to manage!
        // Either knowing it exists or handling somehow else
        break;

    case PDFObjectType::ArlPDFObjTypeNull:
        // PDF null object same as not existing - return nullptr
        break;

    case PDFObjectType::ArlPDFObjTypeReference:
    case PDFObjectType::ArlPDFObjTypeUnknown:
    default:
        assert(false && "unexpected object type for conversion to AST-Node!");
        break;
    } // switch obj_type
    delete ast_obj;
    return nullptr;
}


/// @brief   Check if the value of a key is in a dictionary and matches a given set
///
/// @param[in] dict     dictionary object
/// @param[in] key      the key name or array index
/// @param[in] values   a set of values to match. "*" will be interpreted as wildcard and will match anything for certain kinds of PDF objects.
///
/// @returns true if the key value matches something in the values set
bool CPDFFile::check_key_value(ArlPDFDictionary* dict, const std::wstring& key, const std::vector<std::wstring> values)
{
    assert(dict != nullptr);
    assert(key.find(L"::") == std::string::npos);
    assert(key.find('*') == std::string::npos);

    ArlPDFObject* val_obj = dict->get_value(key);

    if (val_obj != nullptr) {
        std::wstring  val;
        switch (val_obj->get_object_type()) {
        case PDFObjectType::ArlPDFObjTypeString:
            val = ((ArlPDFString*)val_obj)->get_value();
            delete val_obj;
            for (auto& v : values)
                if (val == v)
                    return true;
            break;

        case PDFObjectType::ArlPDFObjTypeName:
            val = ((ArlPDFName*)val_obj)->get_value();
            delete val_obj;
            for (auto& v : values)
                if ((val == v) || (v == L"*"))      // Support wildcard matching for PDF names in Arlington
                    return true;
            break;

        case PDFObjectType::ArlPDFObjTypeNumber:
            if (((ArlPDFNumber*)val_obj)->is_integer_value()) {
                int i = ((ArlPDFNumber*)val_obj)->get_integer_value();
                delete val_obj;
                val = ToWString(std::to_string(i));
                for (auto& v : values)
                    if (val == v) 
                        return true;
            }
            else {
                double d = ((ArlPDFNumber*)val_obj)->get_value();
                delete val_obj;
                // Cannot compare doubles as strings due to unknown precision
                for (auto& v : values)
                    if (fabs(std::stod(v) - d) <= ArlNumberTolerance)
                        return true;
            }
            break;

        default: /* fallthrough */
            delete val_obj;
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
std::string CPDFFile::check_and_get_pdf_version(std::ostream& ofs)
{
    bool hdr_ok = ((pdf_header_version.size() == 3)  && FindInVector(v_ArlPDFVersions, pdf_header_version));
    bool cat_ok = ((pdf_catalog_version.size() == 3) && FindInVector(v_ArlPDFVersions, pdf_catalog_version));

    pdf_version.clear();

    if (hdr_ok)
        ofs << COLOR_INFO << "Header is version PDF " << pdf_header_version << COLOR_RESET;
    if (cat_ok)
        ofs << COLOR_INFO << "Document Catalog/Version is PDF " << pdf_catalog_version << COLOR_RESET;

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

    // See if XRefStream is wrong for final PDF version (i.e. before PDF 1.5)
    if (get_ptr_to_trailer()->is_xrefstm()) {
        if ((pdf_version[0] == '1') && (pdf_version[2] < '5'))
            ofs << COLOR_ERROR << "XRefStream is present in PDF " << pdf_version << " before introduction in PDF 1.5." << COLOR_RESET;
        else if ((pdf_header_version[0] == '1') && (pdf_header_version[2] < '5'))
            ofs << COLOR_WARNING << "XRefStream is present in file with header %PDF-" << pdf_header_version << " and Document Catalog Version of PDF " << pdf_catalog_version << COLOR_RESET;
    }

    // To reduce lots of false warnings, snap transparency-aware PDF to 1.7
    if (!exact_version_compare && (forced_version.size() == 0) && ((pdf_version == "1.4") || (pdf_version == "1.5") || (pdf_version == "1.6"))) {
        ofs << COLOR_INFO << "Rounding up PDF " << pdf_version << " to PDF 1.7" << COLOR_RESET;
        pdf_version = "1.7";
    }

    // Hard force to any version - expect lots of messages if this is wrong!!
    if (forced_version.size() > 0) {
        ofs << COLOR_INFO << "Command line forced to PDF " << forced_version << COLOR_RESET;
        pdf_version = forced_version;
    }

    assert(pdf_version.size() > 0);
    assert(FindInVector(v_ArlPDFVersions, pdf_version));

    return pdf_version;
}


/// @brief Set the PDF version for an encountered feature so we can track latest version used in a PDF file
///
/// @param[in]  ver   a valid PDF version from Arlington representing a feature we have just encountered
/// @param[in]  arl   the Arlington TSV file of the feature we have just encountered
/// @param[in]  key   the key (or array index) of the feature we have just encountered
void CPDFFile::set_feature_version(const std::string& ver, const std::string& arl, const std::string& key) 
{
    // Avoid processing extensions
    if ((ver.size() == 3) && FindInVector(v_ArlPDFVersions, ver)) {
        // Convert to 10 * PDF version
        int pdf_v    = string_to_pdf_version(ver);
        int latest_v = string_to_pdf_version(latest_feature_version);

        if (pdf_v > latest_v) {
            latest_feature_version = ver;
            latest_feature_arlington = arl;
            latest_feature_key = key;
        }
    }
}


/// @brief returns the latest feature version details encountered so far as a human readable string. 
std::string CPDFFile::get_latest_feature_version_info()
{
    std::string s = " PDF " + latest_feature_version;
    if (latest_feature_arlington.size() > 0) {
        s = s + " (" + latest_feature_arlington;
        if (latest_feature_key.size() > 0) {
            s = s + "/" + latest_feature_key;
        }
        s = s + ")";
    }
    return s;
};



/// @brief Asserts that a PDF string object is always to be unencrypted
/// 
/// @param[in] obj  PDF string object
/// 
/// @returns false if the object is not an unencrypted string 
bool CPDFFile::fn_AlwaysUnencrypted(ArlPDFObject* obj) {
    assert(obj != nullptr);
    if (obj->get_object_type() != PDFObjectType::ArlPDFObjTypeString)
        return false;

    ArlPDFString *str = (ArlPDFString*)obj;
    std::wstring  val = str->get_value();

    fully_implemented = false; /// @todo - how to determine if a string is encrypted or unencrypted???
    return true;
}



/// @brief Asserts that a PDF string object is a valid PDF partial Field Name according to clause 12.7.4.2.
/// This means it needs to be a non-empty string and not contain a PERIOD (".").
/// 
/// @param[in] obj  PDF string object
/// 
/// @returns false if the object is not a valid PDF field name
bool CPDFFile::fn_IsFieldName(ArlPDFObject* obj) {
    assert(obj != nullptr);
    if (obj->get_object_type() != PDFObjectType::ArlPDFObjTypeString)
        return false;

    ArlPDFString* str = (ArlPDFString*)obj;
    std::wstring s = str->get_value();
    if ((s.size() > 0) && (s.find('.') == std::string::npos)) {
        return true;
    }
    return false;
}



/// @brief Asserts that a PDF string object was expressed as a hexadecimal string
/// 
/// @param[in] obj  PDF string object
/// 
/// @returns false if the object was not a hexadecimal string 
bool CPDFFile::fn_IsHexString(ArlPDFObject* obj) {
    assert(obj != nullptr);
    if (obj->get_object_type() != PDFObjectType::ArlPDFObjTypeString)
        return false;

    ArlPDFString* str = (ArlPDFString*)obj;
#if !defined(ARL_PDFSDK_PDFIUM) 
    fully_implemented = false; /// @todo - how to determine if a string is hex for PDFix and other PDF SDKs???
#endif
    return str->is_hex_string();
}



/// @brief Returns the length of a PDF array object that is a key (or array element) 
/// of another object. If key is indeterminate (nullptr), then returns -1.
///
/// @param[in]  parent the parent PDF object to look up keys
/// @param[in]  key    a relative or absolute key
/// 
/// @returns -1 on error or the array length (>= 0)
int CPDFFile::fn_ArrayLength(ArlPDFObject* parent, const ASTNode* key) {
    assert(parent != nullptr);
    int retval = -1;

    if (key != nullptr) {
        auto key_parts = split_key_path(key->node);
        ArlPDFObject* a = get_object_for_path(parent, key_parts);
        if ((a != nullptr) && (a->get_object_type() == PDFObjectType::ArlPDFObjTypeArray))
            retval = ((ArlPDFArray*)a)->get_num_elements();
        delete a;
    }
    return retval;
}


/// @brief Confirms if the elements in a PDF array object are sorted in ascending order.
/// Unsortable elements return false.
/// 
/// @param[in]  parent   the parent PDF array object 
/// @param[in]  arr_key  the key name or an integer array index of the array to be tested
/// @param[in]  step     the step for the array indices. MUST be an integer.
/// 
/// @returns true if array is sorted in ascending order, false otherwise.
bool CPDFFile::fn_ArraySortAscending(ArlPDFObject* parent, const ASTNode* arr_key, const ASTNode* step) {
    assert(parent != nullptr);
    assert(arr_key != nullptr);
    assert((arr_key->type == ASTNodeType::ASTNT_Key) || (arr_key->type == ASTNodeType::ASTNT_ConstInt));
    assert(step != nullptr);
    assert(step->type == ASTNodeType::ASTNT_ConstInt);

    bool retval = false;
    int step_idx = key_to_array_index(step->node);
    assert(step_idx >= 0);

    auto key_parts = split_key_path(arr_key->node);
    ArlPDFObject* obj = get_object_for_path(parent, key_parts);

    if ((obj != nullptr) && (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeArray)) {
        ArlPDFArray* arr = (ArlPDFArray*)obj;
        if (arr->get_num_elements() > 0) {
            // Make sure all array elements are numeric 
            ArlPDFObject* first_elem = arr->get_value(0);
            assert(first_elem != nullptr);
            if (first_elem->get_object_type() == PDFObjectType::ArlPDFObjTypeNumber) {
                ArlPDFNumber* elem = (ArlPDFNumber*)first_elem;
                double       last_elem_val = elem->get_value();
                delete elem;
                double       this_elem_val;
                // Need to check every N-th 
                for (int i = step_idx; i < arr->get_num_elements(); i += step_idx) {
                    elem = (ArlPDFNumber*)arr->get_value(i);
                    if ((elem != nullptr) && (elem->get_object_type() == PDFObjectType::ArlPDFObjTypeNumber)) {
                        this_elem_val = elem->get_value();
                        if (last_elem_val > this_elem_val) {
                            delete elem;
                            delete obj;
                            return false; // was not sorted!
                        }
                        last_elem_val = this_elem_val;
                    }
                    else {
#ifdef PP_FN_DEBUG
                        std::cout << "fn_ArraySortAscending() had non-numeric types!" << std::endl;
#endif
                        delete elem;
                        delete obj;
                        return false; // inconsistent array element types
                    }
                    delete elem;
                } // for
                retval = true;
            }
            else {
#ifdef PP_FN_DEBUG
                std::cout << "fn_ArraySortAscending() was not a numeric array!" << std::endl;
#endif
            }
        }
        else
            retval = true; // empty array is always sorted by definition
    }
#ifdef PP_FN_DEBUG
    std::cout << "fn_ArraySortAscending() was not an array!" << std::endl;
#endif
    delete obj;
    return retval; // wasn't an array
}


/// @brief Checks if a single bit (1-32 inclusive) in a PDF integer object is clear (0). 
/// 
/// @param[in]   obj       a PDF integer object
/// @param[in]   bit_node  AST being the bit number (i.e. an integer 1-32 inclusive)
/// 
/// @returns true iff specified bit was clear (0), false otherwise (incl. errors)
bool CPDFFile::fn_BitClear(ArlPDFObject* obj, const ASTNode* bit_node) 
{
    assert(obj != nullptr);

    assert((bit_node != nullptr) && (bit_node->type == ASTNodeType::ASTNT_ConstInt));
    int bit = std::stoi(bit_node->node); // no need to try/catch as assume Arlington model is valid!
    assert((bit >= 1) && (bit <= 32));
    bit--; // change to 0-31 inclusive

    PDFObjectType obj_type = obj->get_object_type();
    if (obj_type == PDFObjectType::ArlPDFObjTypeNumber) {
        ArlPDFNumber* num_obj = (ArlPDFNumber*)obj;
        if (num_obj->is_integer_value()) {
            std::bitset<32> bitmask = 0;
            bitmask.set(bit, true);
            std::bitset<32> val = num_obj->get_integer_value();
            // std::cout << "BitClear(" << bit + 1 << ")\tVal = " << val << ", bitmask = " << bitmask << std::endl;
            return ((val & bitmask) == 0);
        }
        else {
#ifdef PP_FN_DEBUG
            std::cout << "fn_BitClear() object was not an integer!" << std::endl;
#endif
        }
    }
    else {
#ifdef PP_FN_DEBUG
        std::cout << "fn_BitClear() object was not a number!" << std::endl;
#endif
    }
    return false; 
}


/// @brief Checks if a single bit (1-32 inclusive) in a PDF integer object is set (1).
/// 
/// @param[in]   obj       a PDF integer object
/// @param[in]   bit_node  AST being the bit number (i.e. an integer 1-32 inclusive)
/// 
/// @returns true iff specified bit was set (1), false otherwise (incl. errors)
bool CPDFFile::fn_BitSet(ArlPDFObject* obj, const ASTNode* bit_node) 
{
    assert(obj != nullptr);

    assert((bit_node != nullptr) && (bit_node->type == ASTNodeType::ASTNT_ConstInt));
    int bit = std::stoi(bit_node->node); // no need to try/catch as assume Arlington model is valid!
    assert((bit >= 1) && (bit <= 32));
    bit--; // change to 0-31 inclusive

    PDFObjectType obj_type = obj->get_object_type();
    if (obj_type == PDFObjectType::ArlPDFObjTypeNumber) {
        ArlPDFNumber* num_obj = (ArlPDFNumber*)obj;
        if (num_obj->is_integer_value()) {
            std::bitset<32> bitmask = 0;
            bitmask.set(bit, true);
            std::bitset<32> val = num_obj->get_integer_value();
            // std::cout << "BitSet(" << bit+1 << ")\tVal = " << val << ", bitmask = " << bitmask << std::endl;
            return ((val & bitmask) == bitmask);
        }
        else {
#ifdef PP_FN_DEBUG
            std::cout << "fn_BitSet() object was not an integer!" << std::endl;
#endif
        }
    }
    else {
#ifdef PP_FN_DEBUG
        std::cout << "fn_BitSet() object was not a number!" << std::endl;
#endif
    }
    return false;
}


/// @brief Checks if multiple bits (inclusive range 1-32) in a PDF integer object are all clear (0). 
/// Use fn:BitClear() for a single bit as it is more efficient (but this method will still work).
/// 
/// @param[in]   obj             a PDF integer object
/// @param[in]   low_bit_node    AST being the low bit number (1-32 inclusive). 
/// @param[in]   high_bit_node   AST being the high bit number (1-32 inclusive). 
/// 
/// @returns true iff specified bits were all clear (0), false otherwise (incl. errors)
bool CPDFFile::fn_BitsClear(ArlPDFObject* obj, const ASTNode* low_bit_node, const ASTNode* high_bit_node) 
{
    assert(obj != nullptr);

    assert((low_bit_node != nullptr) && (low_bit_node->type == ASTNodeType::ASTNT_ConstInt));
    int low_bit = std::stoi(low_bit_node->node);
    assert((low_bit >= 1) && (low_bit <= 32));

    assert((high_bit_node != nullptr) && (high_bit_node->type == ASTNodeType::ASTNT_ConstInt));
    int high_bit = std::stoi(high_bit_node->node);
    assert((high_bit >= 1) && (high_bit <= 32));

    assert(low_bit <= high_bit);
    low_bit--;  // 0-31 inclusive
    high_bit--; // 0-31 inclusive

    PDFObjectType obj_type = obj->get_object_type();
    if (obj_type == PDFObjectType::ArlPDFObjTypeNumber) {
        ArlPDFNumber* num_obj = (ArlPDFNumber*)obj;
        if (num_obj->is_integer_value()) {
            std::bitset<32> val = num_obj->get_integer_value();
            std::bitset<32> bitmask = 0;
            for (int bit = low_bit; bit <= high_bit; bit++)
                bitmask.set(bit, true);
            // std::cout << "BitsClear(" << low_bit + 1 << "," << high_bit + 1 << ")\tVal = " << val << ", bitmask = " << bitmask << std::endl;
            return ((val & bitmask) == 0);
        }
        else {
#ifdef PP_FN_DEBUG
            std::cout << "fn_BitsClear() object was not an integer!" << std::endl;
#endif
        }
    }
    else {
#ifdef PP_FN_DEBUG
        std::cout << "fn_BitsClear() object was not a number!" << std::endl;
#endif
    }
    return false;
}


/// @brief Checks if multiple bits (inclusive range 1-32) in a PDF integer object are all set (1). 
/// Use fn:BitSet() for a single bit as it is more efficient, even though this method will work.
/// 
/// @param[in]   obj       a PDF integer object
/// @param[in]   low_bit_node    AST being the low bit number (1-32 inclusive). 
/// @param[in]   high_bit_node   AST being the high bit number (1-32 inclusive). 
/// 
/// @returns true iff all bits were set (1), false otherwise (incl. errors)
bool CPDFFile::fn_BitsSet(ArlPDFObject* obj, const ASTNode* low_bit_node, const ASTNode* high_bit_node) 
{
    assert(obj != nullptr);

    assert((low_bit_node != nullptr) && (low_bit_node->type == ASTNodeType::ASTNT_ConstInt));
    int low_bit = std::stoi(low_bit_node->node);
    assert((low_bit >= 1) && (low_bit <= 32));

    assert((high_bit_node != nullptr) && (high_bit_node->type == ASTNodeType::ASTNT_ConstInt));
    int high_bit = std::stoi(high_bit_node->node);
    assert((high_bit >= 1) && (high_bit <= 32));

    assert(low_bit < high_bit);
    low_bit--;  // 0-31 inclusive
    high_bit--; // 0-31 inclusive

    PDFObjectType obj_type = obj->get_object_type();
    if (obj_type == PDFObjectType::ArlPDFObjTypeNumber) {
        ArlPDFNumber* num_obj = (ArlPDFNumber*)obj;
        if (num_obj->is_integer_value()) {
            std::bitset<32> val = num_obj->get_integer_value();
            std::bitset<32> bitmask = 0;
            for (int bit = low_bit; bit <= high_bit; bit++)
                bitmask.set(bit, true);
            // std::cout << "BitsSet(" << low_bit+1 << "," << high_bit+1 << ")\tVal = " << val << ", bitmask = " << bitmask << std::endl;
            return ((val & bitmask) == bitmask);
        }
        else {
#ifdef PP_FN_DEBUG
            std::cout << "fn_BitsSet() was not an integer!" << std::endl;
#endif
        }
    }
    else {
#ifdef PP_FN_DEBUG
        std::cout << "fn_BitsSet() was not a number!" << std::endl;
#endif
    }
    return false;
}


/// @brief Determines if the specified extension is currently support or not 
/// 
/// @param[in]  extn   the name of the extension (required)
/// @param[in]  value  optional value that is added when the extension is supported
/// 
/// @returns true if the extension is being support, false otherwise
ASTNode* CPDFFile::fn_Extension(const ASTNode* extn, const ASTNode* value) {
    assert(extn != nullptr);
    assert(extn->type == ASTNodeType::ASTNT_Key); // extension names look like keys

    bool extn_supported = false;
    for (auto& e : extensions)
        if ((extn->node == e) || (e == "*")) {
            extn_supported = true;
            break;
        }

    if (extn_supported) {
        ASTNode* retval = new ASTNode;
        if (value != nullptr) {
            retval->type = value->type;
            retval->node = value->node;
        }
        else {
            retval->type = ASTNodeType::ASTNT_ConstPDFBoolean;
            retval->node = "true";
        }
        return retval;
    }
    return nullptr;
}


/// @brief Just assume that all fonts in PDF files have at least 1 Latin character. 
/// Used by FontDescriptors for CapHeight: e.g. fn:IsRequired(fn:FontHasLatinChars())
/// 
/// @param[in]  obj assumed to be a font descriptor dictionary
/// 
/// @returns true if the font object has Latin characters, false otherwise
bool CPDFFile::fn_FontHasLatinChars(ArlPDFObject* obj) 
{
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
        delete t;
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



/// @brief Checks to see if the PDF array object of names contains at least one process colorant name.
/// Process colorant names are Cyan, Magenta, Yellow, Black.
/// 
/// @param[in] parent    PDF parent object
/// @param[in] obj_ref   key name or an integer array index of a PDF array object of names
/// 
/// @returns true if the array contains a process colorant name
bool CPDFFile::fn_HasProcessColorants(ArlPDFObject *parent, const ASTNode* obj_ref) {
    assert(obj_ref != nullptr);
    assert((obj_ref->type == ASTNodeType::ASTNT_Key) || (obj_ref->type == ASTNodeType::ASTNT_ConstInt));
    auto key_parts = split_key_path(obj_ref->node);
    auto obj = get_object_for_path(parent, key_parts);

    if ((obj == nullptr) || (obj->get_object_type() != PDFObjectType::ArlPDFObjTypeArray))
        return false;

    auto arr = (ArlPDFArray*)obj;
    for (int i = 0; i < arr->get_num_elements(); i++) {
        auto o = arr->get_value(i);
        if ((o != nullptr) && (o->get_object_type() == PDFObjectType::ArlPDFObjTypeName)) {
            auto nm = ((ArlPDFName*)o)->get_value();
            if ((nm == L"Cyan") || (nm == L"Magenta") || (nm == L"Yellow") || (nm == L"Black")) {
                delete o;
                return true;
            }
        }
        delete o;
    }
    return false;
}



/// @brief Checks to see if the PDF array object of names contains at least one spot colorant name.
/// For this app, a spot colorant name is considered any name other than the CMYK process colorants.
/// 
/// @param[in] parent    PDF parent object
/// @param[in] obj_ref   key name or an integer array index to a PDF array object of names
/// 
/// @returns true if the array contains a spot colorant name
bool CPDFFile::fn_HasSpotColorants(ArlPDFObject* parent, const ASTNode* obj_ref) {
    assert(obj_ref != nullptr);
    assert((obj_ref->type == ASTNodeType::ASTNT_Key) || (obj_ref->type == ASTNodeType::ASTNT_ConstInt));
    auto key_parts = split_key_path(obj_ref->node);
    auto obj = get_object_for_path(parent, key_parts);

    if ((obj == nullptr) || (obj->get_object_type() != PDFObjectType::ArlPDFObjTypeArray))
        return false;

    auto arr = (ArlPDFArray*)obj;
    for (int i = 0; i < arr->get_num_elements(); i++) {
        auto o = arr->get_value(i);
        if ((o != nullptr) && (o->get_object_type() == PDFObjectType::ArlPDFObjTypeName)) {
            auto nm = ((ArlPDFName*)o)->get_value();
            if ((nm != L"Cyan") && (nm != L"Magenta") && (nm != L"Yellow") && (nm != L"Black") && (nm.size() > 0)) {
                delete o;
                return true;
            }
        }
        delete o;
    }
    return false;
}



/// @brief Checks if a PDF image object is a structure content item
///  e.g. XObjectImage.tsv has fn:IsRequired(fn:ImageIsStructContentItem())
/// 
/// @param[in]  obj assumed to be an Image XObject
/// 
/// @returns always true if object is an image
bool CPDFFile::fn_ImageIsStructContentItem(ArlPDFObject* obj) 
{
    assert(obj != nullptr);

    // Check to make sure obj is an Image XObject
    if (obj->get_object_type() != PDFObjectType::ArlPDFObjTypeDictionary) {
#ifdef PP_FN_DEBUG
        std::cout << "fn_ImageIsStructContentItem() object was not a dictionary!" << std::endl;
#endif
        return false;
    }

    if (!((ArlPDFDictionary*)obj)->has_key(L"Subtype")) {
#ifdef PP_FN_DEBUG
        std::cout << "fn_ImageIsStructContentItem() dictionary did not have a /Subtype key!" << std::endl;
#endif
        return false;
    }

    ArlPDFObject* t = ((ArlPDFDictionary*)obj)->get_value(L"Subtype");
    if ((t == nullptr) || (t->get_object_type() != PDFObjectType::ArlPDFObjTypeName)) {
#ifdef PP_FN_DEBUG
        std::cout << "fn_ImageIsStructContentItem() dictionary /Subtype key was not name!" << std::endl;
#endif
        delete t;
        return false;
    }

    if (((ArlPDFName*)t)->get_value() != L"Image") {
#ifdef PP_FN_DEBUG
        std::cout << "fn_ImageIsStructContentItem() dictionary /Subtype key was not Image!" << std::endl;
#endif
        delete t;
        return false;
    }

    delete t;
    return true;
}


/// @brief Returns true if obj is in the specified map
///  e.g. fn:InMap(RichMediaContent::Assets)
/// 
/// @param[in]  obj   a PDF object
/// @param[in]  map   the name or an integer array index of a PDF map
/// 
/// @returns  true iff obj is in the specified map, false otherwise
bool CPDFFile::fn_InMap(ArlPDFObject* obj, const ASTNode* map) {
    assert(obj != nullptr);
    assert(map != nullptr);
    assert((map->type == ASTNodeType::ASTNT_Key) || (map->type == ASTNodeType::ASTNT_ConstInt));

    auto keys = split_key_path(map->node);

    // Assumptions about code below
    assert((keys.size() == 3) || (keys.size() == 4)); 
    assert(keys[keys.size() - 1][0] != '@');            // No "@key" as the final key

    if (keys[0] != "trailer") {
        // don't support "parent::" or anything else
        fully_implemented = false;
        return false;
    }

    ArlPDFObject* o = nullptr;

    if (keys[0] == "trailer") {
        if (keys[1] == "Catalog") {
            assert((keys[2] == "Dests") || (keys[2] == "Names"));
            auto doccat = pdfsdk.get_document_catalog();
            o = doccat->get_value(ToWString(keys[2]));
            if ((o != nullptr) && (keys.size() == 4)) {
                ArlPDFObject* o1 = ((ArlPDFDictionary*)o)->get_value(ToWString(keys[3]));
                if ((o1 == nullptr) || (o1->get_object_type() != PDFObjectType::ArlPDFObjTypeDictionary)) {
                    delete o1;
                    delete o;
                    return false;
                }
                delete o;
                o = o1;
            }
        }
        else {
            auto t = pdfsdk.get_trailer();
            o = t->get_value(ToWString(keys[1]));
        }
    }

    bool retval = false;
    if ((o != nullptr) && (o->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary)) {
        /// @todo - need to process name-tree/number-tree maps to see if object is present
        /// based on matching hash IDs. For now assume it matches just because there the map exists!
        fully_implemented = false;
        retval = true;
    }

    delete o;
    return retval;
}


/// @brief Check if obj (which should be a File Specification dicionary) is in 
///  DocCat::AF array (of File Specification dicionaries)
/// 
/// @param[in]  obj   a PDF object
/// 
/// @returns  true iff obj is an Associated File, false otherwise
bool CPDFFile::fn_IsAssociatedFile(ArlPDFObject* obj)
{
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
    auto doccat = pdfsdk.get_document_catalog();
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


/// @brief Check if PDF has an unencrytped wrapper as per clause 7.6.7 in ISO 32000-2:2020:
/// 
/// 1. DocCatalog::Collection dictionary exists
/// 2. DocCatalog::Collection View key == /H 
/// 3. there is a FileSpec dictionary in the DocCatalog::Names::EmbeddedFiles name 
///    tree where the AFRelationship key == EncryptedPayload
/// 4. the same FileSpec dictionary is also in DocCatalog::AF array
bool CPDFFile::fn_IsEncryptedWrapper() 
{
    bool retval = false;

    auto doccat = pdfsdk.get_document_catalog();
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
                    } // if EmbeddedFiles
                    delete embedded_files;
                } // if Names
                delete names;
            } // if H
        } // if View
        delete view;
    }
    delete collection;

    return retval;
}


/// @brief Checks if obj is the last item in parent (assumed to be an array).
/// Check is based on HashID.
///  
/// @param[in]  parent   parent object (assumed array)
/// @param[in]  obj      the object (assumed to be a NumberFormat dictionary)
/// @param[in]  key      key name. Must be "parent"
/// 
/// @returns true iff obj is the last number in the format array, false otherwise
bool CPDFFile::fn_IsLastInArray(ArlPDFObject* parent, ArlPDFObject* obj, const ASTNode* key)
{
    assert(parent != nullptr);
    assert(obj != nullptr);
  
    if (((key->type != ASTNodeType::ASTNT_Key) && (key->type != ASTNodeType::ASTNT_ConstInt)) || (key->node != "parent")) {
        assert(false && "fn_IsLastInArray only supports 'parent' key!");
        return false;
    }

    if (parent->get_object_type() != PDFObjectType::ArlPDFObjTypeArray) {
#ifdef PP_FN_DEBUG
        std::cout << "fn_IsLastInArray() parent was not an array!" << std::endl;
#endif
        return false;
    }

    int arr_size = ((ArlPDFArray*)parent)->get_num_elements();
    if (arr_size < 1) {
#ifdef PP_FN_DEBUG
        std::cout << "fn_IsLastInArray() parent array was too small!" << std::endl;
#endif
        return false;
    }

    if (obj->get_object_type() != PDFObjectType::ArlPDFObjTypeDictionary) {
#ifdef PP_FN_DEBUG
        std::cout << "fn_IsLastInArray() object was not a dictionary!" << std::endl;
#endif
        return false;
    }

    // Get the last object in the array
    ArlPDFObject* last_obj = ((ArlPDFArray*)parent)->get_value(arr_size - 1);
    bool retval = (last_obj->get_hash_id() == obj->get_hash_id());
    delete last_obj;
    return retval;
}


/// @brief determine if PDF file is a Tagged PDF via DocCat::MarkInfo::Marked == true
bool CPDFFile::fn_IsPDFTagged() 
{
    bool retval = false;

    auto doccat = pdfsdk.get_document_catalog();
    if (doccat != nullptr) {
        ArlPDFObject* mi = doccat->get_value(L"MarkInfo");
        if ((mi != nullptr) && (mi->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary)) {
            ArlPDFObject* marked = ((ArlPDFDictionary*)mi)->get_value(L"Marked");
            if ((marked != nullptr) && (marked->get_object_type() == PDFObjectType::ArlPDFObjTypeBoolean)) {
                retval = ((ArlPDFBoolean*)marked)->get_value();
            }
            delete marked;
        }
        delete mi;
    }
    return retval;
}


/// @brief If obj is a dictionary, check if 'key' is present.  
/// If obj is an array, check if there is an array element at location 'key' (an integer).
///  cf. grep -Pho "IsPresent\([^)]*\)" * | sort | uniq
/// 
/// @param[in]   parent   parent PDF object
/// @param[in]   key      an Arlington PDF key expression (could be multi-part!)
/// 
/// @returns true if key is present, false otherwise
bool CPDFFile::fn_IsPresent(ArlPDFObject* parent, std::string& key) 
{
    assert(parent != nullptr);
    assert(key.size() > 0);
    assert(key.find('@') == std::string::npos); // NEVER have the value of a key

    auto key_parts = this->split_key_path(key);
    assert(key_parts.size() > 0);
    assert(key_parts[key_parts.size() - 1][0] != '@');

    ArlPDFObject* a = get_object_for_path(parent, key_parts);
    bool retval = (a != nullptr);
    delete a;
    return retval;
}


/// @brief Checks if obj is an direct reference (i.e. NOT indirect)
/// e.g. fn:MustBeDirect(), fn:MustBeDirect(ID::0) or fn:MustBeDirect(fn:IsPresent(Encrypt))
///
/// @param[in]  parent  parent PDF object which might be referenced for other objects direct
/// @param[in]  obj  PDF object which must be direct
/// @param[in]  arg  optional conditional AST
bool CPDFFile::fn_MustBeDirect(ArlPDFObject* parent, ArlPDFObject* obj, const ASTNode *arg)
{
    assert(obj != nullptr);
    bool retval = false;
    if (arg == nullptr) {
        retval = !obj->is_indirect_ref();
    }
    else {
        if (arg->type == ASTNodeType::ASTNT_ConstPDFBoolean) {
            // A reduced predicate expression that was true...
            if (arg->node == "true")
                retval = !obj->is_indirect_ref();
        }
        else if ((arg->type == ASTNodeType::ASTNT_Key) || (arg->type == ASTNodeType::ASTNT_ConstInt)) {
            // Look up key and reduce to true (present) or false (not present). NOT value-of-a-key (@keyname)!
            auto key_parts = split_key_path(arg->node);
            ArlPDFObject* val = get_object_for_path(parent, key_parts);
            if (val != nullptr) 
                retval = !val->is_indirect_ref();
            delete val;
        }
        else {
            assert(false && "unexpected argument to fn:MustBeDirect");
        }
    }
    return retval;
}


/// @brief Checks to make sure that there are no cycles in obj by looping through referencing 'key'.
/// Cycles are detected by comparing object hash IDs.
/// 
/// @param[in]    obj    the PDF object. Must be a dictionary
/// @param[in]    key    the key to follow which must be acyclic
/// 
/// @returns true if there are no cycles, false if cycles are detected or nodes are not dictionaries
bool CPDFFile::fn_NoCycle(ArlPDFObject* obj, const std::string &key) {
    assert(obj != nullptr);
    assert(key.size() > 0);

    if (obj->get_object_type() != PDFObjectType::ArlPDFObjTypeDictionary)
        return false;

    std::wstring              wkey = ToWString(key);
    std::set<std::string>     obj_hash_list;
    ArlPDFDictionary*         node = (ArlPDFDictionary*)((ArlPDFDictionary*)obj)->get_value(wkey);

    obj_hash_list.insert(obj->get_hash_id());
    while ((node != nullptr) && (node->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary)) {
        auto node_hash = node->get_hash_id();
        auto already_seen = obj_hash_list.insert(node_hash);
        if (already_seen.second) { // Found a matching object so a cycle is present 
            delete node;
            return false;
        }
        ArlPDFDictionary* tmp = (ArlPDFDictionary*)node->get_value(wkey);
        delete node;
        node = tmp;
    };
    delete node;
    return true; 
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
bool CPDFFile::fn_NotStandard14Font(ArlPDFObject* parent) {
    assert(parent != nullptr);

    if (parent->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary) {
        ArlPDFDictionary* dict = (ArlPDFDictionary*)parent;
        if (check_key_value(dict, L"Type", { L"Font" }) &&
            check_key_value(dict, L"Subtype", { L"Type1" }) &&
            !check_key_value(dict, L"BaseFont", Std14Fonts)) {
            return true;
        }
    }
    return false;
}


/// @brief Used by PageObject.tsv for requiredness condition on StructParents key:
/// - fn:IsRequired(fn:PageContainsStructContentItems())
/// 
/// Need to check if the numeric value of the PDF object is a valid index into the 
/// trailer::DocCat::StructTreeRoot::ParentTree number tree
/// 
/// @param[in]   obj              the StructParent/StructParents object
/// 
/// @returns
bool CPDFFile::fn_PageContainsStructContentItems(ArlPDFObject* obj) {
    assert(obj != nullptr);

    if (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeNumber) {
        if (((ArlPDFNumber*)obj)->is_integer_value()) {
            int val = ((ArlPDFNumber*)obj)->get_integer_value();
            if (val >= 0) {
                /// @todo - check if integer value in trailer::DocCat::StructTreeRoot::ParentTree number tree
                fully_implemented = false;
                return true;
            }
            else {
#ifdef PP_FN_DEBUG
                std::cout << "fn_PageContainsStructContentItems() was less than zero!" << std::endl;
#endif
            }
        }
        else {
#ifdef PP_FN_DEBUG
            std::cout << "fn_PageContainsStructContentItems() was not an integer!" << std::endl;
#endif
        }
    }
#ifdef PP_FN_DEBUG
    std::cout << "fn_PageContainsStructContentItems() was not number!" << std::endl;
#endif
    return false;  
}


/// @brief Used by Target.tsv for A key: 
/// - fn:PageProperty(\@P,Annots)
/// - fn:Eval(\@A==fn:PageProperty(\@P,Annots::NM))
/// 
/// @param[in] parent   a parent PDF page object
/// @param[in] pg       a reference to a PDF page object as an ASTNode
/// @param[in] pg_key   a key of a PDF page object as an ASTNode
/// 
/// @returns a new ASTNode representing the value of the specified key on the specified page or nullptr on error
ASTNode* CPDFFile::fn_PageProperty(ArlPDFObject* parent, ASTNode* pg, ASTNode* pg_key) {
    assert(parent != nullptr);

    if ((pg == nullptr) || (pg_key == nullptr))
        return nullptr;

    assert(pg->type == ASTNodeType::ASTNT_KeyValue); 
    assert(pg_key->type == ASTNodeType::ASTNT_Key);  // never an integer array index!

    auto pg_parts = split_key_path(pg->node);
    ArlPDFObject* pg_obj = get_object_for_path(parent, pg_parts);
    if ((pg_obj != nullptr) && (pg_obj->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary)) {
        auto pg_key_parts = split_key_path(pg_key->node);
        ArlPDFObject* pg_key_obj = get_object_for_path(parent, pg_key_parts);
        if (pg_key_obj != nullptr) {
            ASTNode* retval = convert_basic_object_to_ast(pg_key_obj);
            if (retval == nullptr) {
                // Referenced page property was a complex PDF object (array, dictionary, stream) or null object
                /// @todo - handle complex PDF object references for fn_PageProperty
            }
            delete pg_key_obj;
            delete pg_obj;
            return retval;
        }
    }
#ifdef PP_FN_DEBUG
    std::cout << "fn_PageProperty() page was not a dictionary!" << std::endl;
#endif
    delete pg_obj;
    return nullptr;
}


/// @brief Returns the height of a PDF rectangle (>=0.0).
/// 
/// @param[in]   parent
/// @param[in]   key or or an integer array index of rectangle
/// 
/// @returns -1.0 on error
double CPDFFile::fn_RectHeight(ArlPDFObject* parent, const ASTNode* key) {
    assert(parent != nullptr);

    if (key == nullptr)
        return -1.0;

    assert((key->type == ASTNodeType::ASTNT_Key) || (key->type == ASTNodeType::ASTNT_ConstInt));

    auto key_parts = split_key_path(key->node);
    ArlPDFObject* r = get_object_for_path(parent, key_parts);
    if ((r != nullptr) && (r->get_object_type() == PDFObjectType::ArlPDFObjTypeArray)) {
        ArlPDFArray* rect = (ArlPDFArray*)r;
        if (rect->get_num_elements() >= 4) {
            ArlPDFNumber* v[4] = { nullptr, nullptr, nullptr, nullptr };
            for (int i = 0; i < 4; i++) {
                v[i] = (ArlPDFNumber*)rect->get_value(i);
                if ((v[i] == nullptr) || (v[i]->get_object_type() != PDFObjectType::ArlPDFObjTypeNumber)) {
                    for (int j = 0; j <= i; j++)
                        delete v[j];
                    delete r;
                    return -1.0; // not all rect array elements were numbers;
                }
            }
            double lly = v[1]->get_value();
            double ury = v[3]->get_value();
            double height = round(fabs(ury - lly));
            for (int i = 0; i < 4; i++)
                delete v[i];
            delete r;
            return height;
        }
        else {
#ifdef PP_FN_DEBUG
            std::cout << "fn_RectHeight() was not a 4 element array!" << std::endl;
#endif
        }
    }
#ifdef PP_FN_DEBUG
    std::cout << "fn_RectHeight() was not an array!" << std::endl;
#endif
    delete r;
    return -1.0; // not an array
}


/// @brief Returns the width of a PDF rectangle (>=0.0).
/// 
/// @param[in]   parent
/// @param[in]   key or or an integer array index of rectangle
/// 
/// @returns -1.0 on error
double CPDFFile::fn_RectWidth(ArlPDFObject* parent, const ASTNode* key) {
    assert(parent != nullptr);

    if (key == nullptr)
        return -1.0;

    assert((key->type == ASTNodeType::ASTNT_Key) || (key->type == ASTNodeType::ASTNT_ConstInt));

    auto key_parts = split_key_path(key->node);
    ArlPDFObject* r = get_object_for_path(parent, key_parts);
    if ((r != nullptr) && (r->get_object_type() == PDFObjectType::ArlPDFObjTypeArray)) {
        ArlPDFArray* rect = (ArlPDFArray*)r;
        if (rect->get_num_elements() >= 4) {
            ArlPDFNumber* v[4] = { nullptr, nullptr, nullptr, nullptr };
            for (int i = 0; i < 4; i++) {
                v[i] = (ArlPDFNumber*)rect->get_value(i);
                if ((v[i] == nullptr) || (v[i]->get_object_type() != PDFObjectType::ArlPDFObjTypeNumber)) {
                    for (int j = 0; j <= i; j++)
                        delete v[j];
                    delete r;
                    return -1.0; // not all rect array elements were numbers;
                }
            }
            double llx = v[0]->get_value();
            double urx = v[2]->get_value();
            double width = fabs(urx - llx);
            for (int i = 0; i < 4; i++)
                delete v[i];
            delete r;
            return width;
        }
        else {
#ifdef PP_FN_DEBUG
            std::cout << "fn_RectWidth() was not a 4 element array!" << std::endl;
#endif
        }
    }
#ifdef PP_FN_DEBUG
    std::cout << "fn_RectWidth() was not an array!" << std::endl;
#endif
    delete r;
    return -1.0; // not an array
}


/// @brief  Determines if value of a PDF object is as specified based on a boolean condition.
/// e.g. fn:RequiredValue(\@CFM==AESV2,128)
/// 
/// @param[in] obj          PDF object
/// @param[in] condition    already reduced AST node tree that is true/false
/// @param[in] value        can be any primitive PDF type (int, real, name, string-*, boolean)
/// 
/// @returns a new ASTNode of value (copied!) or nullptr
ASTNode* CPDFFile::fn_RequiredValue(ArlPDFObject* obj, const ASTNode* condition, const ASTNode* value) {
    assert(obj != nullptr);
    assert(value != nullptr);

    if (condition == nullptr) {
        ASTNode* retval = new ASTNode;
        retval->type = value->type;
        retval->node = value->node;
        return retval;
    }

    assert(condition != nullptr);
    assert(condition->type == ASTNodeType::ASTNT_ConstPDFBoolean);

    if (condition->node == "false") {
        // Condition not met so value of obj can be this value (no need to check anything)
        ASTNode* retval = new ASTNode;
        retval->type = value->type;
        retval->node = value->node;
        return retval;
    }
    else {
        // Condition is met so value of obj MUST BE 'value'
        PDFObjectType obj_type = obj->get_object_type();
        switch (obj_type) {
        case PDFObjectType::ArlPDFObjTypeName:
            if (value->type == ASTNodeType::ASTNT_Key) {
                if (value->node != ToUtf8(((ArlPDFName*)obj)->get_value())) {
                    return nullptr;
                }
            }
            break;

        case PDFObjectType::ArlPDFObjTypeNumber:
            if ((value->type == ASTNodeType::ASTNT_ConstInt) && ((ArlPDFNumber*)obj)->is_integer_value()) {
                if (value->node != std::to_string(((ArlPDFNumber*)obj)->get_integer_value()))
                    return nullptr;
            }
            else if (value->type == ASTNodeType::ASTNT_ConstNum) {
                if (value->node != std::to_string(((ArlPDFNumber*)obj)->get_value()))
                    return nullptr;
            }
            break;

        case PDFObjectType::ArlPDFObjTypeBoolean:
            if (value->type == ASTNodeType::ASTNT_ConstPDFBoolean) {
                bool b = ((ArlPDFBoolean*)obj)->get_value();
                if ((value->node == "true") && !b) {
                    return nullptr;
                }
                else if ((value->node == "false") && b) {
                    return nullptr;
                }
                // else fall through
            }
            break;

        case PDFObjectType::ArlPDFObjTypeString:
            if (value->type == ASTNodeType::ASTNT_ConstString) {
                if (value->node != ToUtf8(((ArlPDFString*)obj)->get_value()))
                    return nullptr;
            }
            break;

        default:
            assert(false && "unexpected fn:RequiredValue value!");
            return nullptr;
        } // switch obj_type
    }

    ASTNode* retval = new ASTNode;
    retval->type = value->type;
    retval->node = value->node;
    return retval;
}


/// @brief  Determines a conditional defaut value based on a boolean condition.
/// e.g. fn:Eval(fn:DefaultValue(\@StateModel=='Marked','Unmarked') || fn:DefaultValue(\@StateModel=='Review','None'))
/// 
/// @param[in] condition    already reduced AST node tree that is true/false
/// @param[in] value        can be any primitive PDF type (int, real, name, string-*, boolean)
/// 
/// @returns a new ASTNode of value or nullptr if false/error
ASTNode* CPDFFile::fn_DefaultValue(const ASTNode* condition, const ASTNode* value) {
    assert(value != nullptr);
    assert(condition != nullptr);
    assert(condition->type == ASTNodeType::ASTNT_ConstPDFBoolean);

    if (condition->node == "false")  // Condition was not met 
        return nullptr;

    ASTNode* retval = new ASTNode;
    retval->type = value->type;
    retval->node = value->node;
    return retval;
}


/// @brief Stream Length is according to /Length key value, not actual stream data. 
/// 
/// @param[in] parent    parent object
/// @param[in] key       key name or integer array index of a stream
/// 
/// @returns Length of the PDF stream object or -1 on error.
int CPDFFile::fn_StreamLength(ArlPDFObject* parent, const ASTNode* key) {
    assert(parent != nullptr);
    if (key == nullptr)
        return -1;
    assert((key->type == ASTNodeType::ASTNT_Key) || (key->type == ASTNodeType::ASTNT_ConstInt));

    auto key_parts = split_key_path(key->node);
    ArlPDFObject* o = get_object_for_path(parent, key_parts);
    if ((o != nullptr) && (o->get_object_type() == PDFObjectType::ArlPDFObjTypeStream)) {
        ArlPDFDictionary* dict = ((ArlPDFStream*)o)->get_dictionary();
        ArlPDFObject* len_obj = dict->get_value(L"Length");
        delete dict;
        delete o;
        if ((len_obj != nullptr) && (len_obj->get_object_type() == PDFObjectType::ArlPDFObjTypeNumber)) {
            ArlPDFNumber* len_num_obj = (ArlPDFNumber*)len_obj;
            int   retval = -1;
            if (len_num_obj->is_integer_value()) 
                retval = len_num_obj->get_integer_value();
            else {
#ifdef PP_FN_DEBUG
                std::cout << "fn_StreamLength() Length was not an integer (was a float)!" << std::endl;
#endif
            }
            delete len_obj;
            return retval;
        }
        else {
#ifdef PP_FN_DEBUG
            std::cout << "fn_StreamLength() Length was not a number!" << std::endl;
#endif
        }
    }
#ifdef PP_FN_DEBUG
    std::cout << "fn_StreamLength() was not a stream!" << std::endl;
#endif
    return -1; 
}


/// @brief Returns length of a PDF string object (>= 0). 
/// 
/// @param[in] parent    parent object
/// @param[in] key       key name or integer array index of a PDF string
/// 
/// @returns Length of the PDF string object or -1 if an error.
int CPDFFile::fn_StringLength(ArlPDFObject* parent, const ASTNode* key) {
    assert(parent != nullptr);
    if (key == nullptr)
        return - 1;

    assert((key->type == ASTNodeType::ASTNT_Key) || (key->type == ASTNodeType::ASTNT_ConstInt));

    auto key_parts = split_key_path(key->node);
    ArlPDFObject* o = get_object_for_path(parent, key_parts);
    if ((o != nullptr) && (o->get_object_type() == PDFObjectType::ArlPDFObjTypeString)) {
        ArlPDFString* str_obj = (ArlPDFString*)o;
        int len = (int)str_obj->get_value().size();
        delete o;
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

    // Convert to 10 * PDF version
    int pdf_v = string_to_pdf_version(pdf_version);
    int arl_v = string_to_pdf_version(ver_node->node);

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

    // Convert to 10 * PDF version
    int pdf_v = string_to_pdf_version(pdf_version);
    int arl_v = string_to_pdf_version(ver_node->node);

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

    // Convert to 10 * PDF version
    int pdf_v = string_to_pdf_version(pdf_version);
    int arl_v = string_to_pdf_version(ver_node->node);
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

    // Convert to 10 * PDF version
    int pdf_v = string_to_pdf_version(pdf_version);
    int arl_v = string_to_pdf_version(dep_ver->node);

    if (!deprecated)
        deprecated = (pdf_v >= arl_v);

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
    return pdfsdk.get_pdf_page_count();
}


/// @brief Looks up a value in an object. Returns true if it is present
///  
/// @param[in] obj       PDF object
/// @param[in] key       a key or integer array index
/// @param[in] value     the value we are looking for
/// 
/// @returns true if obj contains value (e.g. equal if a name, in an array if array)
bool CPDFFile::fn_Contains(ArlPDFObject* obj, const ASTNode* key, const ASTNode* value) {
    assert(obj != nullptr);
    
    // May have been recusively reduced (e.g. don't exist in the PDF)
    if ((key == nullptr) || (value == nullptr))
        return false;

    assert((key->type == ASTNodeType::ASTNT_Key) || (key->type == ASTNodeType::ASTNT_Key));
    bool retval = false;

    if (key->type == value->type)
        retval = (key->node == value->node);
    else {
        switch (obj->get_object_type()) {
        case PDFObjectType::ArlPDFObjTypeArray:
        {
            ArlPDFArray* arr = (ArlPDFArray*)obj;
            for (int i = 0; (i < arr->get_num_elements()) && !retval; i++) {
                ArlPDFObject* elem = arr->get_value(i);
                ASTNode* v = convert_basic_object_to_ast(elem);
                if (v == nullptr) {
                    // Array reference was another complex PDF object (array, dictionary, stream) or null
                    /// @todo - handle complex nested references for fn_Contains
                }
                else if (v->type == value->type)
                    retval = (v->node == value->node);
                delete v;
                delete elem;
            }
        }
        break;
        case PDFObjectType::ArlPDFObjTypeBoolean:
        case PDFObjectType::ArlPDFObjTypeString:
        case PDFObjectType::ArlPDFObjTypeName:
        {
            ASTNode* v = convert_basic_object_to_ast(obj);
            if ((v != nullptr) && (v->type == value->type))
                retval = (v->node == value->node);
            delete v;
        }
        break;
        case PDFObjectType::ArlPDFObjTypeNull:
            break;
        case PDFObjectType::ArlPDFObjTypeNumber:        // Need to support number/integer
        case PDFObjectType::ArlPDFObjTypeDictionary:
        case PDFObjectType::ArlPDFObjTypeStream:
        default:
            assert(false && "unexpected PDF object type in fn_Contains!");
            break;
        } // switch
    }

    return retval;
}