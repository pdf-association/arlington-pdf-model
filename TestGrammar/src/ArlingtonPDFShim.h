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

    /// @class ArlPDFObject
    /// Base class PDF object
    class ArlPDFObject {
    protected:
        /// @brief the underlying PDF object type
        PDFObjectType   type;

        /// @brief pointer to PDF SDK dependent data object
        void*           object;

        /// @brief PDF bject number from underlying PDF SDK. Or parent if negative.
        int             obj_nbr;

        /// @brief PDF generation number from underlying PDF SDK. Or parent if negative.
        int             gen_nbr;

        /// @brief true iff is an indirect reference
        bool            is_indirect;
        
        /// @brief Sort all dictionary keys so guaranteed same order across PDF SDKs
        std::vector<std::wstring>   sorted_keys;

        /// @brief Checks if keys are sorted and, if not, then sorts
        virtual void sort_keys();

    public:
        ArlPDFObject() :
            object(nullptr), obj_nbr(0), gen_nbr(0), type(PDFObjectType::ArlPDFObjTypeUnknown), is_indirect(false)
            { /* default constructor */ };

        explicit ArlPDFObject(ArlPDFObject* parent, void* obj);

        ~ArlPDFObject()
            { /* destructor */ sorted_keys.clear(); }
        
        PDFObjectType get_object_type() { return type; };
        int   get_object_number() { return obj_nbr;  };
        int   get_generation_number() { return gen_nbr; };
        bool  is_indirect_ref() { return is_indirect; };
        std::string get_hash_id();

        /// @brief output operator <<
        friend std::ostream& operator << (std::ostream& ofs, const ArlPDFObject& obj) {
            if (obj.object != nullptr) {
                if (obj.obj_nbr > 0)
                    ofs << "obj " << obj.obj_nbr << " " << obj.gen_nbr;
                else if (obj.obj_nbr < 0)
                    ofs << "parent obj " << abs(obj.obj_nbr) << " " << abs(obj.gen_nbr);
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
        ArlPDFBoolean(ArlPDFObject* parent, void* obj) : ArlPDFObject(parent, obj)
            { /* constructor */ type = PDFObjectType::ArlPDFObjTypeBoolean; };

        friend std::ostream& operator << (std::ostream& ofs, const ArlPDFBoolean& obj) {
            ofs << "boolean " << (ArlPDFObject)obj;
            return ofs;
        };

        bool get_value();
    };

    /// @class ArlPDFNumber
    /// PDF Number object
    class ArlPDFNumber : public ArlPDFObject {
    public:
        ArlPDFNumber(ArlPDFObject* parent, void* obj) : ArlPDFObject(parent, obj)
            { /* constructor */ type = PDFObjectType::ArlPDFObjTypeNumber; };

        bool   is_integer_value();
        int    get_integer_value();
        double get_value();
        friend std::ostream& operator << (std::ostream& ofs, const ArlPDFNumber& obj) {
            ofs << "number " << (ArlPDFObject)obj;
            return ofs;
        };
    };

    /// @class ArlPDFString
    /// PDF string object
    class ArlPDFString : public ArlPDFObject {
    public:
        ArlPDFString(ArlPDFObject* parent, void* obj) : ArlPDFObject(parent, obj)
            { /* constructor */ type = PDFObjectType::ArlPDFObjTypeString; };

        std::wstring get_value();
        bool is_hex_string();
        friend std::ostream& operator << (std::ostream& ofs, const ArlPDFString& obj) {
            ofs << "string " << (ArlPDFObject)obj;
            return ofs;
        };
    };

    /// @class ArlPDFName
    /// PDF Name object
    class ArlPDFName : public ArlPDFObject {
    public:
        ArlPDFName(ArlPDFObject* parent, void* obj) : ArlPDFObject(parent, obj)
            { /* constructor */ type = PDFObjectType::ArlPDFObjTypeString; };

        std::wstring get_value();
        friend std::ostream& operator << (std::ostream& ofs, const ArlPDFName& obj) {
            ofs << "name " << (ArlPDFObject)obj;
            return ofs;
        };
    };

    /// @class ArlPDFNull
    /// PDF null object
    class ArlPDFNull : public ArlPDFObject {
        using ArlPDFObject::ArlPDFObject;
    public:
        ArlPDFNull(ArlPDFObject* parent, void* obj) : ArlPDFObject(parent, obj)
            { /* constructor */ type = PDFObjectType::ArlPDFObjTypeNull; };

        friend std::ostream& operator << (std::ostream& ofs, const ArlPDFNull& obj) {
            ofs << "null " << (ArlPDFObject)obj;
            return ofs;
        };
    };

    /// @class ArlPDFArray
    /// PDF Array object
    class ArlPDFArray : public ArlPDFObject {
    public:
        ArlPDFArray(ArlPDFObject* parent, void* obj) : ArlPDFObject(parent, obj)
            { /* constructor */ type = PDFObjectType::ArlPDFObjTypeArray; };

        int get_num_elements();
        ArlPDFObject* get_value(const int idx);
        friend std::ostream& operator << (std::ostream& ofs, const ArlPDFArray& obj) {
            ofs << "array " << (ArlPDFObject)obj;
            return ofs;
        };
    };

    /// @class ArlPDFDictionary
    /// PDF Dictionary object
    class ArlPDFDictionary : public ArlPDFObject {
    public:
        ArlPDFDictionary(ArlPDFObject* parent, void* obj) : ArlPDFObject(parent, obj)
            { /* constructor */ type = PDFObjectType::ArlPDFObjTypeDictionary; };

        // For keys by name...
        bool          has_key(std::wstring key);
        ArlPDFObject* get_value(std::wstring key);

        // For iterating keys...
        int get_num_keys();
        std::wstring get_key_name_by_index(const int index);
        friend std::ostream& operator << (std::ostream& ofs, const ArlPDFDictionary& obj) {
            ofs << "dictionary " << (ArlPDFObject)obj;
            return ofs;
        };
    };

    /// @class ArlPDFStream
    /// PDF stream object
    class ArlPDFStream : public ArlPDFObject {
    public:
        ArlPDFStream(ArlPDFObject* parent, void* obj) : ArlPDFObject(parent, obj)
            { /* constructor */ type = PDFObjectType::ArlPDFObjTypeStream; };

        ArlPDFDictionary* get_dictionary();
        friend std::ostream& operator << (std::ostream& ofs, const ArlPDFStream& obj) {
            ofs << "stream " << (ArlPDFObject)obj;
            return ofs;
        };
    };

    /// @class ArlPDFTrailer
    /// The trailer object of a PDF document (file)
    class ArlPDFTrailer : public ArlPDFDictionary {
    protected:
        /// @brief Whether it is XRefStream or conventional trailer
        bool            is_xrefstm;

        /// @brief If unsupported encryption (standard or PKI) is in place (means string checks will fail)
        bool            unsupported_encryption;

    public:
        ArlPDFTrailer(void* obj) : ArlPDFDictionary(nullptr, obj),
            is_xrefstm(false), unsupported_encryption(false)
            { /* constructor */ };


        void set_xrefstm(const bool is_xrefstream) { is_xrefstm = is_xrefstream; };
        bool get_xrefstm() { return is_xrefstm; };
        void set_unsupported_encryption(const bool b) { unsupported_encryption = b; };
        bool is_unsupported_encryption() { return unsupported_encryption; };

        friend std::ostream& operator << (std::ostream& ofs, const ArlPDFTrailer& obj) {
            ofs << "trailer " << (ArlPDFDictionary)obj << (obj.is_xrefstm ? " with xref " : "");
            return ofs;
        }
    };

    /// @class ArlingtonPDFSDK
    /// Arlington PDF SDK
    class ArlingtonPDFSDK {
    public:
        /// @brief Untyped PDF SDK context object. Needs casting appropriately
        static void    *ctx;

        /// @brief PDF SDK constructor
        explicit ArlingtonPDFSDK()
            { /* constructor */ ctx = nullptr; };

        /// @brief Initialize the PDF SDK. Throws exceptions on error.
        void initialize();

        /// @brief Shutdown the PDF SDK
        void shutdown();

        /// @brief Get human-readable version string of PDF SDK
        std::string get_version_string();

        /// @brief Open a PDF file (no password) and returns trailer object.
        ArlPDFTrailer *get_trailer(std::filesystem::path pdf_filename);

        /// @brief Get the PDF version of a PDF file
        std::string get_pdf_version(ArlPDFTrailer* trailer);

        /// @brief Get number of pages in the PDF. -1 on error
        int get_pdf_page_count(ArlPDFTrailer* trailer);
    };

}; // namespace

#endif // ArlingtonPDFShim_h
