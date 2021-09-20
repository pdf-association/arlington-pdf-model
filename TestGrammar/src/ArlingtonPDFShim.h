///////////////////////////////////////////////////////////////////////////////
// ArlingtonPDFShim.h
// Copyright 2020-2021 PDF Association, Inc. https://www.pdfa.org
//
// This material is based upon work supported by the Defense Advanced
// Research Projects Agency (DARPA) under Contract No. HR001119C0079.
// Any opinions, findings and conclusions or recommendations expressed
// in this material are those of the author(s) and do not necessarily
// reflect the views of the Defense Advanced Research Projects Agency
// (DARPA). Approved for public release.
//
// SPDX-License-Identifier: Apache-2.0
// Contributors: Peter Wyatt, PDF Association
//
///////////////////////////////////////////////////////////////////////////////

#ifndef ArlingtonPDFShim_h
#define ArlingtonPDFShim_h
#pragma once

/// @brief Choose which PDF SDK you want to use. Some may have more functionality than others. pdfium is default
#if !defined(ARL_PDFSDK_PDFIUM) && !defined(ARL_PDFSDK_PDFIX) && !defined(ARL_PDFSDK_QPDF)
#define ARL_PDFSDK_PDFIUM
#undef ARL_PDFSDK_PDFIX
#undef ARL_PDFSDK_QPDF
#endif

#include <iostream>
#include <filesystem>

/// @namespace A wafer thin shim layer to isolate a specific C/C++ PDF SDK library from the Arlington
/// PDF Model proof-of-concept C++ application. By replacing the matching .cpp file,
/// any PDF SDK library should be easily integrateable without propogating changes
/// throughout the PoC code base. Performance issues are considered irrelevant.
namespace ArlingtonPDFShim {

    /// @brief Enable verbose debugging
    static bool    debugging = false;

    /// @brief All the various types of PDF Object
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

    /// @brief Human readable equivalent of the above enum
    const std::string PDFObjectType_strings[] = {
        "ArlPDFObjTypeUnknown",
        "ArlPDFObjTypeBoolean",
        "ArlPDFObjTypeNumber",
        "ArlPDFObjTypeString",
        "ArlPDFObjTypeName",
        "ArlPDFObjTypeArray",
        "ArlPDFObjTypeDictionary",
        "ArlPDFObjTypeStream",
        "ArlPDFObjTypeNull",
        "ArlPDFObjTypeReference"
    };

    // Base class PDF object
    class ArlPDFObject {
    protected:
        void* object;
        bool is_indirect;
    public:
        explicit ArlPDFObject(void* obj);
        ~ArlPDFObject();
        PDFObjectType get_object_type();
        int   get_object_number();
        bool  is_indirect_ref();
        std::string get_hash_id();

        /// @brief output operator <<
        friend std::ostream& operator << (std::ostream& ofs, const ArlPDFObject& obj) {
            if (obj.object != nullptr) {
                int           obj_num  = const_cast<ArlPDFObject&>(obj).get_object_number();
                if (obj_num > 0)
                    ofs << "obj " << obj_num;
                else
                    ofs << "direct-obj";
            }
            return ofs;
        };
    };

    class ArlPDFBoolean : public ArlPDFObject {
        using ArlPDFObject::ArlPDFObject;
    public:
        bool get_value();
        friend std::ostream& operator << (std::ostream& ofs, const ArlPDFBoolean& obj) {
            ofs << "Boolean " << (ArlPDFObject)obj;
            return ofs;
        };
    };

    class ArlPDFNumber : public ArlPDFObject {
        using ArlPDFObject::ArlPDFObject;
    public:
        bool   is_integer_value();
        int    get_integer_value();
        double get_value();
        friend std::ostream& operator << (std::ostream& ofs, const ArlPDFNumber& obj) {
            ofs << "Number " << (ArlPDFObject)obj;
            return ofs;
        };
    };

    class ArlPDFString : public ArlPDFObject {
        using ArlPDFObject::ArlPDFObject;
    public:
        std::wstring get_value();
        friend std::ostream& operator << (std::ostream& ofs, const ArlPDFString& obj) {
            ofs << "String " << (ArlPDFObject)obj;
            return ofs;
        };
    };

    class ArlPDFName : public ArlPDFObject {
        using ArlPDFObject::ArlPDFObject;
    public:
        std::wstring get_value();
        friend std::ostream& operator << (std::ostream& ofs, const ArlPDFName& obj) {
            ofs << "Name " << (ArlPDFObject)obj;
            return ofs;
        };
    };

    class ArlPDFNull : public ArlPDFObject {
        using ArlPDFObject::ArlPDFObject;
        friend std::ostream& operator << (std::ostream& ofs, const ArlPDFNull& obj);
    };

    class ArlPDFArray : public ArlPDFObject {
        using ArlPDFObject::ArlPDFObject;
    public:
        int get_num_elements();
        ArlPDFObject* get_value(int idx);
        friend std::ostream& operator << (std::ostream& ofs, const ArlPDFArray& obj) {
            ofs << "Array " << (ArlPDFObject)obj;
            return ofs;
        };
    };

    class ArlPDFDictionary : public ArlPDFObject {
        using ArlPDFObject::ArlPDFObject;
    public:
        // For keys by name...
        bool          has_key(std::wstring key);
        ArlPDFObject* get_value(std::wstring key);
        // For iterating keys...
        int get_num_keys();
        std::wstring get_key_name_by_index(int index);
        friend std::ostream& operator << (std::ostream& ofs, const ArlPDFDictionary& obj) {
            ofs << "Dictionary " << (ArlPDFObject)obj;
            return ofs;
        };
    };

    class ArlPDFStream : public ArlPDFObject {
        using ArlPDFObject::ArlPDFObject;
    public:
        ArlPDFDictionary* get_dictionary();
        friend std::ostream& operator << (std::ostream& ofs, const ArlPDFStream& obj) {
            ofs << "Stream " << (ArlPDFObject)obj;
            return ofs;
        };
    };

    // The trailer object of a PDF document (file)
    class ArlPDFTrailer : public ArlPDFDictionary {
        using ArlPDFDictionary::ArlPDFDictionary;
    protected:
        // Whether it is XRefStream or normal trailer
        bool    is_xrefstm = false;
    public:
        void set_xrefstm(bool is_xrefstream) { is_xrefstm = is_xrefstream; };
        bool get_xrefstm() { return is_xrefstm; };
        friend std::ostream& operator << (std::ostream& ofs, const ArlPDFTrailer& obj);
    };

    class ArlingtonPDFSDK {
    public:
        /// @brief Untyped PDF SDK context object. Needs casting appropriately
        static void    *ctx;

        /// @brief PDF SDK constructor
        explicit ArlingtonPDFSDK()
            { /* constructor */ ctx = nullptr; };

        /// @brief Initialize the PDF SDK. Throws exceptions on error.
        void initialize(bool enable_debugging);

        /// @brief Shutdown the PDF SDK
        void shutdown();

        /// @brief Get human-readable version string of PDF SDK
        std::string get_version_string();

        /// @brief Open a PDF file (no password) and returns trailer object.
        ArlPDFTrailer *get_trailer(std::filesystem::path pdf_filename);

        /// @brief Get the PDF version of a PDF file
        std::string get_pdf_version(ArlPDFTrailer* trailer);
    };

}; // namespace

#endif // ArlingtonPDFShim_h
