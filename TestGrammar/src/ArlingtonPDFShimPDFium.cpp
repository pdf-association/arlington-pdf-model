///////////////////////////////////////////////////////////////////////////////
// ArlingtonPDFShim.cpp
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

/// A wafer-thin shim layer to isolate the PDFix SDK library from the rest of the 
/// Arlington PDF Model proof-of-concept C++ application. Replace just this C++ file 
/// for alternate PDF SDKs. Performance overhead issues are considered irrelevant. 

#include <string>
#include <cassert>

#include "ArlingtonPDFShim.h"


//pdfium
#include "core/include/fxcodec/fx_codec.h"
#include "core/include/fxge/fx_ge.h"
#include "core/include/fpdfapi/fpdf_parser.h"
#include "core/include/fpdfapi/fpdf_module.h"

using namespace ArlingtonPDFShim;

//todo: have to be static of ArlObject
CPDF_Parser* temp_parser;


/// @brief Initialize the PDF SDK. May throw exceptions.
void ArlingtonPDFSDK::initialize(bool enable_debugging)
{
    assert(ctx == nullptr);
    CPDF_ModuleMgr::Create();
    auto moduleMgr = CPDF_ModuleMgr::Get();
    
    CPDF_Parser* parser = FX_NEW CPDF_Parser;
    temp_parser = parser;
    ctx = parser;
    
    ArlingtonPDFShim::debugging = enable_debugging;
}

/// @brief  Shutdown the PDF SDK
void ArlingtonPDFSDK::shutdown()
{
  //destroy pdfium
  CPDF_Parser* parser = (CPDF_Parser*)ctx;
  delete(parser);
  CPDF_ModuleMgr::Destroy();
  //CFX_GEModule::Destroy();
}

/// @brief  Returns human readable version string for PDF SDK that is being used
/// @return version string
std::string ArlingtonPDFSDK::get_version_string()
{
    assert(ctx != nullptr);
    return "pdfium";
    //Pdfix* pdfix = (Pdfix *)ctx;
    //return "PDFix v" + std::to_string(pdfix->GetVersionMajor()) + "." 
    //                 + std::to_string(pdfix->GetVersionMinor()) + "." 
    //                 + std::to_string(pdfix->GetVersionPatch());
}


/// @brief   Opens a PDF file (no password) and locates trailer dictionary
/// @param   pdf_filename[in] PDF filename
/// @return  handle to PDF trailer dictionary or nullptr if trailer is not locatable
ArlPDFTrailer *ArlingtonPDFSDK::get_trailer(std::filesystem::path pdf_filename)
{
    assert(ctx != nullptr);
    CPDF_Parser* parser = (CPDF_Parser*)ctx;
    //parser->SetPassword(password);
    FX_DWORD err_code = parser->StartParse((FX_LPCSTR)pdf_filename.string().c_str());
    if (err_code)
      return nullptr;

    //rt auto m_document = parser->GetDocument();
    ArlPDFTrailer* trailer_obj = new ArlPDFTrailer(parser->GetTrailer());
    return trailer_obj;
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

    CPDF_Object *obj = (CPDF_Object *)object;
    PDFObjectType retval;

    switch (obj->GetType())
    {
    case PDFOBJ_BOOLEAN:
            retval = PDFObjectType::ArlPDFObjTypeBoolean; 
            break;
    case PDFOBJ_NUMBER:
            retval = PDFObjectType::ArlPDFObjTypeNumber;    // Integer or Real (or bitmask)
            break;
        case PDFOBJ_STRING:
            retval = PDFObjectType::ArlPDFObjTypeString;    // Any type of string
            break;
        case PDFOBJ_NAME:
            retval = PDFObjectType::ArlPDFObjTypeName;
            break;
        case PDFOBJ_ARRAY:
            retval = PDFObjectType::ArlPDFObjTypeArray;     // incl. rectangle or matrix
            break;
        case PDFOBJ_DICTIONARY:
            retval = PDFObjectType::ArlPDFObjTypeDictionary;
            break;
        case PDFOBJ_STREAM:
            retval = PDFObjectType::ArlPDFObjTypeStream;
            break;
        case PDFOBJ_NULL:
            retval = PDFObjectType::ArlPDFObjTypeNull;
            break;
        case PDFOBJ_REFERENCE:
            //TODO: can't immediately change the indiret to direct
             object = temp_parser->GetDocument()->GetIndirectObject(((CPDF_Reference*)object)->GetRefObjNum());
             retval = get_object_type(); // PDFObjectType::ArlPDFObjTypeReference;
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
    bool retval = (((CPDF_Object*)object)->GetType() == PDFOBJ_REFERENCE);
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
    int retval = ((CPDF_Object*)object)->GetObjNum();
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
    assert(((CPDF_Object *)object)->GetType() == PDFOBJ_BOOLEAN);
    CPDF_Boolean* obj = ((CPDF_Boolean*)object); // ->AsBoolean();
    bool retval = obj->GetInteger()!=0;
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
    assert(((CPDF_Object*)object)->GetType() == PDFOBJ_NUMBER);
    CPDF_Number* obj = ((CPDF_Number*)object); // ->AsNumber();
    bool retval = obj->IsInteger();
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
    assert(((CPDF_Object*)object)->GetType() == PDFOBJ_NUMBER);
    CPDF_Number* obj = ((CPDF_Number*)object); // ->AsNumber();
    assert(obj->IsInteger());
    int retval = obj->GetInteger();
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
    assert(((CPDF_Object*)object)->GetType() == PDFOBJ_NUMBER);
    CPDF_Number* obj = ((CPDF_Number*)object); // ->AsNumber();
    double retval = obj->GetNumber();
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
    assert(((CPDF_Object*)object)->GetType() == PDFOBJ_STRING);
    CPDF_String* obj = ((CPDF_String*)object); // ->AsString();
    std::wstring retval = (FX_LPCWSTR)obj->GetString().UTF8Decode();
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
    assert(((CPDF_Object*)object)->GetType() == PDFOBJ_NAME);
    CPDF_Name* obj = ((CPDF_Name*)object); // ->AsName();
    std::wstring retval = (FX_LPCWSTR)obj->GetString().UTF8Decode();
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
    assert(((CPDF_Object*)object)->GetType() == PDFOBJ_ARRAY);
    CPDF_Array* obj = ((CPDF_Array*)object); // ->AsArray();
    int retval = obj->GetCount();
    if (ArlingtonPDFShim::debugging) {
        std::wcout << __FUNCTION__ << "(" << object << "): " << retval << std::endl;
    }
    return retval;
}


/// @brief  Returns the i-th array element from a PDF array object
/// @param idx the array index [0 ... n-1]
/// @return the object at array element index
ArlPDFObject* ArlPDFArray::get_value(int idx)
{
    assert(object != nullptr);
    assert(idx >= 0);
    assert(((CPDF_Object*)object)->GetType() == PDFOBJ_ARRAY);
    CPDF_Array* obj = ((CPDF_Array*)object); // ->AsArray();

    ArlPDFObject *retval = new ArlPDFObject(obj->GetElement(idx));
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
    assert(((CPDF_Object*)object)->GetType() == PDFOBJ_DICTIONARY);
    CPDF_Dictionary* obj = ((CPDF_Dictionary*)object); // ->AsDictionary();
    int retval = obj->GetCount();
    if (ArlingtonPDFShim::debugging) {
        std::wcout << __FUNCTION__ << "(" << object << "): " << retval << std::endl;
    }
    return retval;
}


/// @brief  Checks whether a PDF dictionary object has a specific key
/// @param key the key name
/// @return true if the dictionary has the specified key
bool ArlPDFDictionary::has_key(std::wstring key)
{
    assert(object != nullptr);
    assert(((CPDF_Object*)object)->GetType() == PDFOBJ_DICTIONARY);
    CPDF_Dictionary* obj = ((CPDF_Dictionary*)object); // ->AsDictionary();
      
    bool retval = obj->KeyExist(CFX_ByteString::FromUnicode(key.c_str()));
    if (ArlingtonPDFShim::debugging) {
        std::wcout << __FUNCTION__ << "(" << key << "): " << (retval ? "true" : "false") << std::endl;
    }
    return retval;
}


/// @brief  Gets the object associated with the key from a PDF dictionary
/// @param key the key name
/// @return the PDF object value of key
ArlPDFObject* ArlPDFDictionary::get_value(std::wstring key)
{
    assert(object != nullptr);
    assert(((CPDF_Object*)object)->GetType() == PDFOBJ_DICTIONARY);
    CPDF_Dictionary* obj = ((CPDF_Dictionary*)object); // ->AsDictionary();

    ArlPDFObject* retval = new ArlPDFObject(obj->GetElement(CFX_ByteString::FromUnicode(key.c_str())));
    if (ArlingtonPDFShim::debugging) {
        std::wcout << __FUNCTION__ << "(" << key << "): " << retval << std::endl;
    }
    return retval;
}


/// @brief Returns the key name of i-th dictionary key
/// @param index[in] dictionary key index 
/// @return Key name
std::wstring ArlPDFDictionary::get_key_name_by_index(int index)
{
    assert(object != nullptr);
    assert(index >= 0);
    assert(((CPDF_Object*)object)->GetType() == PDFOBJ_DICTIONARY);
    CPDF_Dictionary* obj = ((CPDF_Dictionary*)object); // ->AsDictionary();

    FX_POSITION pos = obj->GetStartPos();
    int pos_index = 0;
    std::wstring retval;
    while (pos) {
      CFX_ByteString keyName;
      CPDF_Object* nextObj = obj->GetNextElement(pos, keyName);
      if (pos_index == index) {
        retval = (FX_LPCWSTR)keyName.UTF8Decode();
        break;
      }
      pos_index++;
    }

    if (ArlingtonPDFShim::debugging) {
        std::wcout << __FUNCTION__ << "(" << index << "): '" << retval << "'" << std::endl;
    }
    return retval;
}


/// @brief Returns the number of keys in a PDF stream 
/// @return Number of keys (>= 0)
int ArlPDFStream::get_num_keys()
{
    assert(object != nullptr);
    assert(((CPDF_Object*)object)->GetType() == PDFOBJ_STREAM);
    CPDF_Stream* obj = ((CPDF_Stream*)object); // ->AsStream();
    CPDF_Dictionary* stm_dict = obj->GetDict();
    assert(stm_dict != nullptr);
    int retval = stm_dict->GetCount();
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
    assert(((CPDF_Object*)object)->GetType() == PDFOBJ_STREAM);
    CPDF_Stream* obj = ((CPDF_Stream*)object); // ->AsStream();
    CPDF_Dictionary* stm_dict = obj->GetDict();
    assert(stm_dict != nullptr);
    bool retval = stm_dict->KeyExist(CFX_ByteString::FromUnicode(key.c_str()));
    if (ArlingtonPDFShim::debugging) {
        std::wcout << __FUNCTION__ << "(" << key << "): " << (retval ? "true" : "false") << std::endl;
    }
    return retval;
}


/// @brief  Gets the object associated with the key from a PDF stream
/// @param key the key name
/// @return the PDF object value of key
ArlPDFObject* ArlPDFStream::get_value(std::wstring key)
{
    assert(object != nullptr);
    assert(((CPDF_Object*)object)->GetType() == PDFOBJ_STREAM);
    CPDF_Stream* obj = ((CPDF_Stream*)object); // ->AsStream();
    CPDF_Dictionary* stm_dict = obj->GetDict();
    assert(stm_dict != nullptr);
    ArlPDFObject * retval = new ArlPDFObject(stm_dict->GetElement(CFX_ByteString::FromUnicode(key.c_str())));
    if (ArlingtonPDFShim::debugging) {
        std::wcout << __FUNCTION__ << "(" << key << "): " << retval << std::endl;
    }
    return retval;
}


/// @brief Returns the key name of i-th stream key
/// @param index[in] dictionary key index 
/// @return Key name
std::wstring ArlPDFStream::get_key_name_by_index(int index)
{
    assert(object != nullptr);
    assert(index >= 0);
    assert(((CPDF_Object*)object)->GetType() == PDFOBJ_STREAM);
    CPDF_Stream* obj = ((CPDF_Stream*)object); // ->AsStream();
    CPDF_Dictionary* stm_dict = obj->GetDict();
    assert(stm_dict != nullptr);

    int pos_index = 0;
    std::wstring retval;
    FX_POSITION pos = stm_dict->GetStartPos();
    while (pos) {
      CFX_ByteString keyName;
      CPDF_Object* nextObj = stm_dict->GetNextElement(pos, keyName);
      if (pos_index == index) {
        retval = (FX_LPCWSTR)keyName.UTF8Decode();
        break;
      }
      pos_index++;
    }

    if (ArlingtonPDFShim::debugging) {
        std::wcout << __FUNCTION__ << "(" << index << "): '" << retval << "'" << std::endl;
    }
    return retval;
}
