///////////////////////////////////////////////////////////////////////////////
// ArlingtonPDFShimQPDF.cpp
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
// Contributors: Peter Wyatt
//
///////////////////////////////////////////////////////////////////////////////

/// A wafer-thin shim layer to isolate the QPDF SDK library from the rest of the
/// Arlington PDF Model proof-of-concept C++ application. Replace just this C++ file
/// for alternate PDF SDKs. Performance overhead issues are considered irrelevant.

#include "ArlingtonPDFShim.h"
#ifdef ARL_PDFSDK_QPDF

// QPDF uses some C++17 deprecated features so silence warnings
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS 
#define _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNINGS

#include <string>
#include <cassert>
#include <codecvt>
#include <locale>

#include <qpdf/QPDF.hh>
#include <qpdf/QPDFObjectHandle.hh>

using namespace ArlingtonPDFShim;


/// @brief Initialize the PDF SDK. May throw exceptions.
void ArlingtonPDFSDK::initialize(bool enable_debugging)
{
    assert(ctx == nullptr);

    // Global namespaced flag to control debugging output
    ArlingtonPDFShim::debugging = enable_debugging;

    // Assign to void context
    ctx = new QPDF();
}


/// @brief  Shutdown the PDF SDK
void ArlingtonPDFSDK::shutdown()
{
    delete ctx;
    ctx = nullptr;
}


/// @brief  Returns human readable version string for PDF SDK that is being used
/// @return version string
std::string ArlingtonPDFSDK::get_version_string()
{
    return QPDF::QPDFVersion();
}


/// @brief   Opens a PDF file (no password) and locates trailer dictionary
/// @param   pdf_filename[in] PDF filename
/// @return  handle to PDF trailer dictionary or nullptr if trailer is not locatable
ArlPDFTrailer *ArlingtonPDFSDK::get_trailer(std::filesystem::path pdf_filename)
{
    assert(ctx != nullptr);

    QPDF* qpdfctx = (QPDF*)ctx;
    qpdfctx->processFile(pdf_filename.string().c_str(), nullptr);

    QPDFObjectHandle *trailer = new QPDFObjectHandle;
    *trailer = qpdfctx->getTrailer();

    if (trailer->isDictionary())
    {
        ArlPDFTrailer* trailer_obj = new ArlPDFTrailer(trailer);

        // if /Type key exists, then assume working with XRefStream
        trailer_obj->set_xrefstm(trailer->hasKey("/Type"));

        int id = trailer->getObjectID();
        QPDFObjectHandle root_key = qpdfctx->getRoot();
        QPDFObject::object_type_e ot = root_key.getTypeCode();
        id = root_key.getObjectID();

        QPDFObjectHandle info_key = trailer->getKey("/Info");
        ot = info_key.getTypeCode();
        id = info_key.getObjectID();

        return trailer_obj;
    }
    return nullptr;
}


/// @brief  Returns the PDF object type of an object
/// @return PDFObjectType enum value
PDFObjectType ArlPDFObject::get_object_type()
{
    if (object == nullptr) {
        if (ArlingtonPDFShim::debugging) {
            std::cout << __FUNCTION__ << "(nullptr): PDFObjectType::ArlPDFObjTypeNull" << std::endl;
        }
        return PDFObjectType::ArlPDFObjTypeNull;
    }

    QPDFObjectHandle *obj = (QPDFObjectHandle *)object;
    PDFObjectType retval;

    switch (obj->getTypeCode())
    {
        case QPDFObject::object_type_e::ot_boolean:
            retval = PDFObjectType::ArlPDFObjTypeBoolean;
            break;
        case QPDFObject::object_type_e::ot_integer:
        case QPDFObject::object_type_e::ot_real:
            retval = PDFObjectType::ArlPDFObjTypeNumber;    // Integer or Real (or bitmask)
            break;
        case QPDFObject::object_type_e::ot_string:
            retval = PDFObjectType::ArlPDFObjTypeString;    // Any type of string
            break;
        case QPDFObject::object_type_e::ot_name:
            retval = PDFObjectType::ArlPDFObjTypeName;
            break;
        case QPDFObject::object_type_e::ot_array:
            retval = PDFObjectType::ArlPDFObjTypeArray;     // incl. rectangle or matrix
            break;
        case QPDFObject::object_type_e::ot_dictionary:
            retval = PDFObjectType::ArlPDFObjTypeDictionary;
            break;
        case QPDFObject::object_type_e::ot_stream:
            retval = PDFObjectType::ArlPDFObjTypeStream;
            break;
        case QPDFObject::object_type_e::ot_null:
            retval = PDFObjectType::ArlPDFObjTypeNull;
            break;
        default:
            retval = PDFObjectType::ArlPDFObjTypeUnknown;
            break;
    }
    if (ArlingtonPDFShim::debugging) {
        std::cout << __FUNCTION__ << "(" << object << "): " << PDFObjectType_strings[(int)retval] << std::endl;
    }
    return retval;
}


/// @brief   Indicates if an object is an indirect reference
/// @return  true if an indirect reference. false otherwise (direct object)
bool ArlPDFObject::is_indirect_ref()
{
    assert(object != nullptr);
    bool retval = (((QPDFObjectHandle *)object)->isIndirect());
    if (ArlingtonPDFShim::debugging) {
        std::wcout << __FUNCTION__ << "(" << object << "): " << (retval ? "true" : "false") << std::endl;
    }
    return retval;
}


/// @brief  Returns the object number or 0 if a direct object
/// @return the object number or 0 if a direct object
int ArlPDFObject::get_object_number()
{
    assert(object != nullptr);
    int retval = ((QPDFObjectHandle *)object)->getObjectID();
    if (ArlingtonPDFShim::debugging) {
        std::wcout << __FUNCTION__ << "(" << object << "): " << retval << std::endl;
    }
    return retval;
}


/// @brief   Returns the value of a PDF boolean object
/// @return  Returns true or false
bool ArlPDFBoolean::get_value()
{
    assert(object != nullptr);
    QPDFObjectHandle *obj = (QPDFObjectHandle *)object;
    assert(obj->isBool());
    bool retval = obj->getBoolValue();
    if (ArlingtonPDFShim::debugging) {
        std::wcout << __FUNCTION__ << "(" << object << "): " << retval << std::endl;
    }
    return retval;
}


/// @brief  Returns true if a PDF numeric object is an integer
/// @return Returns true if an integer value, false if real value
bool ArlPDFNumber::is_integer_value()
{
    assert(object != nullptr);
    QPDFObjectHandle *obj = (QPDFObjectHandle *)object;
    bool retval = obj->isInteger();
    if (ArlingtonPDFShim::debugging) {
        std::wcout << __FUNCTION__ << "(" << object << "): " << (retval ? "true" : "false") << std::endl;
    }
    return retval;
}


/// @brief  Returns the integer value of a PDF integer object
/// @return The integer value bounded by compiler
int ArlPDFNumber::get_integer_value()
{
    assert(object != nullptr);
    QPDFObjectHandle *obj = (QPDFObjectHandle *)object;
    assert(obj->isInteger());
    int retval = obj->getIntValueAsInt();
    if (ArlingtonPDFShim::debugging) {
        std::wcout << __FUNCTION__ << "(" << object << "): " << retval << std::endl;
    }
    return retval;
}


/// @brief  Returns the value of a PDF numeric object as a double,
///         regardless if it is an integer or real in the PDF file
/// @return Double precision value bounded by compiler
double ArlPDFNumber::get_value()
{
    assert(object != nullptr);
    QPDFObjectHandle *obj = (QPDFObjectHandle *)object;
    assert(obj->isInteger() || obj->isReal());
    double retval = obj->getNumericValue();
    if (ArlingtonPDFShim::debugging) {
        std::wcout << __FUNCTION__ << "(" << object << "): " << retval << std::endl;
    }
    return retval;
}


/// @brief  Returns the bytes of a PDF string object
/// @return The bytes of a PDF string object (can be zero length)
std::wstring ArlPDFString::get_value()
{
    assert(object != nullptr);
    QPDFObjectHandle *obj = (QPDFObjectHandle *)object;
    assert(obj->isString());
    std::wstring retval = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(obj->getStringValue());
    if (ArlingtonPDFShim::debugging) {
        std::wcout << __FUNCTION__ << "(" << object << "): '" << retval << "'" << std::endl;
    }
    return retval;
}


/// @brief  Returns the name of a PDF name object as a string
/// @return The string representation of a PDF name object (can be zero length)
std::wstring ArlPDFName::get_value()
{
    assert(object != nullptr);
    QPDFObjectHandle *obj = (QPDFObjectHandle *)object;
    assert(obj->isString());
    std::wstring retval = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(obj->getName());
    if (ArlingtonPDFShim::debugging) {
        std::wcout << __FUNCTION__ << "(" << object << "): '" << retval << "'" << std::endl;
    }
    return retval;
}


/// @brief  Returns the number of elements in a PDF array
/// @return The number of array elements (>= 0)
int ArlPDFArray::get_num_elements()
{
    assert(object != nullptr);
    QPDFObjectHandle *obj = (QPDFObjectHandle *)object;
    assert(obj->isArray());
    int retval = obj->getArrayNItems();
    if (ArlingtonPDFShim::debugging) {
        std::wcout << __FUNCTION__ << "(" << object << "): " << retval << std::endl;
    }
    return retval;
}


/// @brief  Returns the i-th array element from a PDF array object
/// @param  idx[in] the array index [0 ... n-1]
/// @return the object at array element index
ArlPDFObject* ArlPDFArray::get_value(int idx)
{
    assert(object != nullptr);
    assert(idx >= 0);
    QPDFObjectHandle *obj = (QPDFObjectHandle *)object;
    assert(obj->isArray());
    QPDFObjectHandle *elem = new QPDFObjectHandle;
    *elem = obj->getArrayItem(idx);
    ArlPDFObject *retval = new ArlPDFObject(elem);
    if (ArlingtonPDFShim::debugging) {
        std::wcout << __FUNCTION__ << "(" << idx << "): " << retval << std::endl;
    }
    return retval;
}


/// @brief Returns the number of keys in a PDF dictionary
/// @return Number of keys (>= 0)
int ArlPDFDictionary::get_num_keys()
{
    assert(object != nullptr);
    QPDFObjectHandle *obj = (QPDFObjectHandle *)object;
    assert(obj->isDictionary());
    std::map<std::string, QPDFObjectHandle>  dict = obj->getDictAsMap();
    int retval = (int)dict.size();
    if (ArlingtonPDFShim::debugging) {
        std::wcout << __FUNCTION__ << "(" << object << "): " << retval << std::endl;
    }
    return retval;
}


/// @brief  Checks whether a PDF dictionary object has a specific key
/// @param  key[in] the key name
/// @return true if the dictionary has the specified key
bool ArlPDFDictionary::has_key(std::wstring key)
{
    assert(object != nullptr);
    QPDFObjectHandle *obj = (QPDFObjectHandle *)object;
    assert(obj->isDictionary());
    std::string s = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(key);
    bool retval = obj->hasKey(s); 
    if (ArlingtonPDFShim::debugging) {
        std::cout << __FUNCTION__ << "(" << s << "): " << (retval ? "true" : "false") << std::endl;
    }
    return retval;
}


/// @brief  Gets the object associated with the key from a PDF dictionary
/// @param key the key name
/// @return the PDF object value of key
ArlPDFObject* ArlPDFDictionary::get_value(std::wstring key)
{
    assert(object != nullptr);
    QPDFObjectHandle *obj = (QPDFObjectHandle *)object;
    assert(obj->isDictionary());
    std::string s = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(key);
    if (obj->hasKey(s)) {
        QPDFObjectHandle* keyobj = new QPDFObjectHandle;
        *keyobj = obj->getKey(s);
        if (keyobj->isInitialized()) {
            ArlPDFObject* retval = new ArlPDFObject(keyobj);
            if (ArlingtonPDFShim::debugging) {
                std::cout << __FUNCTION__ << "(" << s << "): " << retval << std::endl;
            }
            return retval;
        }
    }
    return nullptr;
}


/// @brief Returns the key name of i-th dictionary key
/// @param index[in] dictionary key index
/// @return Key name
std::wstring ArlPDFDictionary::get_key_name_by_index(int index)
{
    assert(object != nullptr);
    assert(index >= 0);
    QPDFObjectHandle *obj = (QPDFObjectHandle *)object;
    assert(obj->isDictionary());
    std::map<std::string, QPDFObjectHandle>  dict = obj->getDictAsMap();
    if (index < dict.size()) {
        int i = 0;
        for (const auto& [ky, val] : dict) {
            if (index == i++) {
                if (ArlingtonPDFShim::debugging) {
                    std::cout << __FUNCTION__ << "(" << index << "): " << ky << std::endl;
                }
                return std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(ky);
            }
        }
    }
    if (ArlingtonPDFShim::debugging) {
        std::cout << __FUNCTION__ << "(" << index << "): nullptr"<< std::endl;
    }
    return nullptr;
}


/// @brief Returns the number of keys in a PDF stream
/// @return Number of keys (>= 0)
int ArlPDFStream::get_num_keys()
{
    assert(object != nullptr);
    QPDFObjectHandle *obj = (QPDFObjectHandle *)object;
    assert(obj->isStream());
    std::map<std::string, QPDFObjectHandle>  dict = obj->getDictAsMap();
    int retval = (int)dict.size();
    if (ArlingtonPDFShim::debugging) {
        std::wcout << __FUNCTION__ << "(" << object << "): " << retval << std::endl;
    }
    return retval;
}


/// @brief  Checks whether a PDF stream object has a specific key
/// @param key the key name
/// @return true if the dictionary has the specified key
bool ArlPDFStream::has_key(std::wstring key)
{
    assert(object != nullptr);
    QPDFObjectHandle *obj = (QPDFObjectHandle *)object;
    assert(obj->isStream());
    std::string s = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(key);
    bool retval = obj->hasKey(s);
    if (ArlingtonPDFShim::debugging) {
        std::cout << __FUNCTION__ << "(" << s << "): " << (retval ? "true" : "false") << std::endl;
    }
    return retval;
}


/// @brief  Gets the object associated with the key from a PDF stream
/// @param key the key name
/// @return the PDF object value of key
ArlPDFObject* ArlPDFStream::get_value(std::wstring key)
{
    assert(object != nullptr);
    QPDFObjectHandle *obj = (QPDFObjectHandle *)object;
    assert(obj->isStream());
    std::string s = std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(key);
    if (obj->hasKey(s)) {
        QPDFObjectHandle* keyobj = new QPDFObjectHandle;
        *keyobj = obj->getKey(s);
        if (keyobj->isInitialized()) {
            ArlPDFObject* retval = new ArlPDFObject(keyobj);
            if (ArlingtonPDFShim::debugging) {
                std::cout << __FUNCTION__ << "(" << s << "): " << retval << std::endl;
            }
            return retval;
        }
    }
    return nullptr;
}


/// @brief Returns the key name of i-th stream key
/// @param index[in] dictionary key index
/// @return Key name
std::wstring ArlPDFStream::get_key_name_by_index(int index)
{
    assert(object != nullptr);
    assert(index >= 0);
    QPDFObjectHandle *obj = (QPDFObjectHandle *)object;
    assert(obj->isStream());
    std::map<std::string, QPDFObjectHandle>  dict = obj->getDictAsMap();
    if (index < dict.size()) {
        int i = 0;
        for (const auto& [ky, val] : dict) {
            if (index == i++) {
                if (ArlingtonPDFShim::debugging) {
                    std::cout << __FUNCTION__ << "(" << index << "): " << ky << std::endl;
                }
                return std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(ky);
            }
        }
    }
    if (ArlingtonPDFShim::debugging) {
        std::cout << __FUNCTION__ << "(" << index << "): nullptr" << std::endl;
    }
    return nullptr;
}

#endif // ARL_PDFSDK_QPDF
