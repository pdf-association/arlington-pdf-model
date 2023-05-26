///////////////////////////////////////////////////////////////////////////////
/// @file 
/// @brief Arlington PDF SDK shim layer
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
/// @author Peter Wyatt, PDF Association
//
///////////////////////////////////////////////////////////////////////////////

#ifndef ArlingtonPDFShim_h
#define ArlingtonPDFShim_h
#pragma once

#include <iostream>
#include <filesystem>
#include <vector>
#include <memory>
#include <cassert>

/// @brief Choose which PDF SDK you want to use. Some may have more functionality than others.
/// This is set in CMakeLists.txt or the TestGrammar | Properties | Preprocessor dialog for Visual Studio
#if !defined(ARL_PDFSDK_PDFIUM) && !defined(ARL_PDFSDK_PDFIX) && !defined(ARL_PDFSDK_QPDF)
#error Select the PDF SDK by defining one of: ARL_PDFSDK_PDFIUM, ARL_PDFSDK_PDFIX or ARL_PDFSDK_QPDF
#endif



/// @brief \#define MARK_STRINGS_WHEN_ENCRYPTED to replace all string values when encrypted with standard text
#ifndef MARK_STRINGS_WHEN_ENCRYPTED
#undef MARK_STRINGS_WHEN_ENCRYPTED
const std::wstring UNSUPPORTED_ENCRYPTED_STRING_MARKER = L"<!unsupported encryption!>";
#endif // MARK_STRINGS_WHEN_ENCRYPTED


/// @namespace ArlingtonPDFShim
/// A wafer thin shim layer to isolate a specific C/C++ PDF SDK library from the Arlington
/// PDF Model proof-of-concept C++ application. By replacing the matching .cpp file,
/// any PDF SDK library should be easily integrateable without propogating changes
/// throughout the PoC code base. Performance issues are considered irrelevant.
namespace ArlingtonPDFShim {
    /// @enum PDFObjectType 
    /// All the various types of PDF Object
    enum class PDFObjectType {
        ArlPDFObjTypeUnknown    = 0,
        ArlPDFObjTypeBoolean,
        ArlPDFObjTypeNumber,    // Integer or Real
        ArlPDFObjTypeString,    // Any type of string
        ArlPDFObjTypeName,
        ArlPDFObjTypeArray,
        ArlPDFObjTypeDictionary,
        ArlPDFObjTypeStream,
        ArlPDFObjTypeNull,
        ArlPDFObjTypeReference  // Indirect reference
    };

    /// @brief Human readable equivalent of PDFObjectType
    const std::string PDFObjectType_strings[] = {
        "????",
        "boolean",
        "number",
        "string",
        "name",
        "array",
        "dictionary",
        "stream",
        "null",
        "Indirect Reference"
    };

    /// @brief a PDF object ID comprising object and generation numbers
    typedef struct _object_id {
        int object_num;         // valid if != 0. Negative means direct in another object
        int generation_num;     // valid if >= 0

        _object_id() : 
            object_num(0), generation_num(-1)
            { /* default constructor to invalid values */ };

    } object_id;

    /// @class ArlPDFObject
    /// Base class PDF object
    class ArlPDFObject {
    protected:
        /// @brief the underlying PDF object type
        PDFObjectType   type;

        /// @brief pointer to PDF SDK dependent data object
        void*           object;

        /// @brief PDF object identifier. Or direct in a parent if negative.
        object_id       obj_id;

        /// @brief PDF object identifier of parent. Or direct in a parent if negative.
        object_id       parent_id;

        /// @brief true iff is an indirect reference
        bool            is_indirect;

        /// @brief deleteable underlying PDF SDK object (NO for trailer, doccat)
        bool            deleteable;
        
        /// @brief Sort all dictionary keys so guaranteed same order across PDF SDKs
        std::vector<std::wstring>   sorted_keys;

        /// @brief Checks if keys are sorted and, if not, then sorts
        virtual void sort_keys();

    public:
        ArlPDFObject(const bool can_delete = true) :
            object(nullptr), type(PDFObjectType::ArlPDFObjTypeUnknown), is_indirect(false), deleteable(can_delete)
            { /* default constructor */ };

        explicit ArlPDFObject(ArlPDFObject* container, void* obj, const bool can_delete = true);

        ~ArlPDFObject()
            { /* default destructor */ sorted_keys.clear(); assert(deleteable); }
        
        PDFObjectType get_object_type() { return type; };
        int   get_object_number()       { return obj_id.object_num; };
        int   get_generation_number()   { return obj_id.generation_num; };
        bool  has_valid_parent()        { return ((parent_id.object_num != 0) && (parent_id.generation_num >= 0)); };
        bool  is_indirect_ref()         { return is_indirect; };
        bool  is_deleteable()           { return deleteable; };
        void  force_deleteable()        { deleteable = true; };
        std::string get_hash_id();

        /// @brief output operator <<
        friend std::ostream& operator << (std::ostream& ofs, const ArlPDFObject& obj) {
            if (obj.object != nullptr) {
                if (obj.obj_id.object_num > 0)
                    ofs << "obj " << obj.obj_id.object_num << " " << obj.obj_id.generation_num;
                else if (obj.obj_id.object_num < 0)
                    ofs << "container obj " << abs(obj.obj_id.object_num) << " " << abs(obj.obj_id.generation_num);
                else
                    ofs << "direct-obj";
            }
            return ofs;
        };
    };

    /// @class ArlPDFBoolean
    /// PDF Boolean object
    class ArlPDFBoolean : public ArlPDFObject {
    public:
        ArlPDFBoolean(ArlPDFObject* container, void* obj) : ArlPDFObject(container, obj)
            { /* constructor */ type = PDFObjectType::ArlPDFObjTypeBoolean; };

        friend std::ostream& operator << (std::ostream& ofs, const ArlPDFBoolean& obj) {
            ofs << "boolean " << (ArlPDFObject&)obj;
            return ofs;
        };

        bool get_value();
    };

    /// @class ArlPDFNumber
    /// PDF Number object
    class ArlPDFNumber : public ArlPDFObject {
    public:
        ArlPDFNumber(ArlPDFObject* container, void* obj) : ArlPDFObject(container, obj)
            { /* constructor */ type = PDFObjectType::ArlPDFObjTypeNumber; };

        bool   is_integer_value();
        int    get_integer_value();
        double get_value();
        friend std::ostream& operator << (std::ostream& ofs, const ArlPDFNumber& obj) {
            ofs << "number " << (ArlPDFObject&)obj;
            return ofs;
        };
    };

    /// @class ArlPDFString
    /// PDF string object
    class ArlPDFString : public ArlPDFObject {
    public:
        ArlPDFString(ArlPDFObject* container, void* obj) : ArlPDFObject(container, obj)
            { /* constructor */ type = PDFObjectType::ArlPDFObjTypeString; };

        std::wstring get_value();
        bool is_hex_string();

        friend std::ostream& operator << (std::ostream& ofs, const ArlPDFString& obj) {
            ofs << "string " << (ArlPDFObject&)obj;
            return ofs;
        };
    };

    /// @class ArlPDFName
    /// PDF Name object
    class ArlPDFName : public ArlPDFObject {
    public:
        ArlPDFName(ArlPDFObject* container, void* obj) : ArlPDFObject(container, obj)
            { /* constructor */ type = PDFObjectType::ArlPDFObjTypeString; };

        std::wstring get_value();

        friend std::ostream& operator << (std::ostream& ofs, const ArlPDFName& obj) {
            ofs << "name " << (ArlPDFObject&)obj;
            return ofs;
        };
    };

    /// @class ArlPDFNull
    /// PDF null object
    class ArlPDFNull : public ArlPDFObject {
    public:
        ArlPDFNull(ArlPDFObject* container, void* obj) : ArlPDFObject(container, obj)
            { /* constructor */ type = PDFObjectType::ArlPDFObjTypeNull; };

        friend std::ostream& operator << (std::ostream& ofs, const ArlPDFNull& obj) {
            ofs << "null " << (ArlPDFObject&)obj;
            return ofs;
        };
    };

    /// @class ArlPDFArray
    /// PDF Array object
    class ArlPDFArray : public ArlPDFObject {
    public:
        ArlPDFArray(ArlPDFObject* container, void* obj) : ArlPDFObject(container, obj)
            { /* constructor */ type = PDFObjectType::ArlPDFObjTypeArray; };

        int get_num_elements();
        ArlPDFObject* get_value(const int idx);

        friend std::ostream& operator << (std::ostream& ofs, const ArlPDFArray& obj) {
            ofs << "array " << (ArlPDFObject&)obj;
            return ofs;
        };
    };

    /// @class ArlPDFDictionary
    /// PDF Dictionary object
    class ArlPDFDictionary : public ArlPDFObject {
    public:
        ArlPDFDictionary(ArlPDFObject* container, void* obj, const bool can_delete = true) : ArlPDFObject(container, obj, can_delete)
            { /* constructor */ type = PDFObjectType::ArlPDFObjTypeDictionary; };

        // For keys by name...
        bool          has_key(std::wstring key);
        ArlPDFObject* get_value(std::wstring key);

        // For iterating keys...
        int get_num_keys();
        std::wstring get_key_name_by_index(const int index);

        bool has_duplicate_keys();
        std::vector<std::string>& get_duplicate_keys();

        friend std::ostream& operator << (std::ostream& ofs, const ArlPDFDictionary& obj) {
            ofs << "dictionary " << (ArlPDFObject&)obj;
            return ofs;
        };
    };

    /// @class ArlPDFStream
    /// PDF stream object
    class ArlPDFStream : public ArlPDFObject {
    public:
        ArlPDFStream(ArlPDFObject* container, void* obj) : ArlPDFObject(container, obj)
            { /* constructor */ type = PDFObjectType::ArlPDFObjTypeStream; };

        ArlPDFDictionary* get_dictionary();

        friend std::ostream& operator << (std::ostream& ofs, const ArlPDFStream& obj) {
            ofs << "stream " << (ArlPDFObject&)obj;
            return ofs;
        };
    };

    /// @class ArlPDFTrailer
    /// The trailer object of a PDF document (file)
    class ArlPDFTrailer : public ArlPDFDictionary {
    protected:
        /// @brief Whether it is XRefStream or conventional trailer
        const bool            has_xrefstm;

        /// @brief PDF is encrypted
        const bool            has_encryption;

        /// @brief If unsupported encryption (standard or PKI) is in place (means all string checks will warn)
        const bool            has_unsupported_encryption;

    public:
        ArlPDFTrailer(void* obj, const bool has_xref, const bool encrypted, const bool unsupport_enc) : ArlPDFDictionary(nullptr, obj, false),
            has_xrefstm(has_xref), has_encryption(encrypted), has_unsupported_encryption(unsupport_enc)
            { /* constructor */ };

        ~ArlPDFTrailer() 
            { /* destructor */ };

        bool is_xrefstm() { return has_xrefstm; };
        bool is_encrypted() { return has_encryption; };
        bool is_unsupported_encryption() { return has_unsupported_encryption; };

        friend std::ostream& operator << (std::ostream& ofs, const ArlPDFTrailer& obj) {
            ofs << "trailer " << (ArlPDFDictionary&)obj << (obj.has_encryption ? (obj.has_unsupported_encryption ? " with unsupported encryption"  : " encrypted") : "") << (obj.has_xrefstm ? " with XRefStm " : "");
            return ofs;
        }
    };



    /// @class ArlingtonPDFSDK
    /// Arlington PDF SDK
    class ArlingtonPDFSDK {
    public:
        /// @brief Untyped PDF SDK context object. Needs casting appropriately
        static void* ctx;

        /// @brief PDF SDK constructor
        explicit ArlingtonPDFSDK()
            { /* constructor */ ctx = nullptr; };

        /// @brief Initialize the PDF SDK. Can throw exceptions on error.
        void initialize();

        /// @brief Shutdown the PDF SDK
        void shutdown();

        /// @brief Get human-readable name and version string of PDF SDK
        std::string get_version_string();

        /// @brief Open a PDF file (optional password) 
        bool open_pdf(const std::filesystem::path& pdf_filename, const std::wstring& password);

        /// @brief Close a previously opened PDF and free all memory and resources
        void close_pdf();

        /// @brief Returns trailer dictionary-like object of an already opened PDF. DO NOT DELETE.
        ArlPDFTrailer*  get_trailer();
         
        /// @brief Returns document catalog (Trailer::Root) of an already opened PDF. DO NOT DELETE.
        ArlPDFDictionary* get_document_catalog();

        /// @brief Get the PDF version of an already opened PDF file as a string of length 3
        std::string get_pdf_version();

        /// @brief Get the PDF version of an already opened PDF file as an integer * 10
        int get_pdf_version_number();

        /// @brief Get number of pages (>= 0) in the already opened PDF. -1 on error.
        int get_pdf_page_count();
    };

}; // namespace

#endif // ArlingtonPDFShim_h
