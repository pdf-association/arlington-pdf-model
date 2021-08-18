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

#include <iostream>
#include <filesystem>

/// A wafer thin shim layer to isolate a specific C/C++ PDF SDK library from the Arlington
/// PDF Model proof-of-concept C++ application. By replacing the matching .cpp file, 
/// any PDF SDK library should be easily integrateable without propogating changes
/// throughout the PoC code base. Performance issues are considered irrelevant. 

namespace ArlingtonPDFShim {

    // Enable verbose debugging
    static bool    debugging = false;

    // All the various types of PDF Object
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

    // Human readable equivalent of the above enum
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
        PDFObjectType get_object_type();
        int   get_object_number();
        bool  is_indirect_ref();
        std::string get_hash_id();
    };

    class ArlPDFBoolean : public ArlPDFObject {
        using ArlPDFObject::ArlPDFObject;
    public:
        bool get_value();
    };

    class ArlPDFNumber : public ArlPDFObject {
        using ArlPDFObject::ArlPDFObject;
    public:
        bool   is_integer_value();
        int    get_integer_value();
        double get_value();
    };

    class ArlPDFString : public ArlPDFObject {
        using ArlPDFObject::ArlPDFObject;
    public:
        std::wstring get_value();
    };

    class ArlPDFName : public ArlPDFObject {
        using ArlPDFObject::ArlPDFObject;
    public:
        std::wstring get_value();
    };

    class ArlPDFNull : public ArlPDFObject {
        using ArlPDFObject::ArlPDFObject;
    };

    class ArlPDFArray : public ArlPDFObject {
        using ArlPDFObject::ArlPDFObject;
    public:
        int get_num_elements();
        ArlPDFObject* get_value(int idx);
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
    };

    class ArlPDFStream : public ArlPDFObject {
        using ArlPDFObject::ArlPDFObject;
    public:
      ArlPDFDictionary* get_dictionary();
        //// For keys by name...
        //bool          has_key(std::wstring key);
        //ArlPDFObject* get_value(std::wstring key);
        //// For iterating keys...
        //int get_num_keys();
        //std::wstring get_key_name_by_index(int index);
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
    };

    class ArlingtonPDFSDK {
    public:
        // Untyped PDF SDK context object 
        static void    *ctx;

        // Constructor
        explicit ArlingtonPDFSDK()
        { /* constructor */
          ctx = nullptr;
        };

        // Initialize the PDF SDK. Throws exceptions on error.
        void initialize(bool enable_debugging);

        // Shutdown the PDF SDK
        void shutdown();

        // Get human-readable version string
        std::string get_version_string();

        // Open a PDF file (no password) and returns trailer object. 
        ArlPDFTrailer *get_trailer(std::filesystem::path pdf_filename);
    };

} // namespace

#endif // ArlingtonPDFShim_h
