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

/// @file 
/// A wafer-thin shim layer to isolate the PDFix SDK library from the rest of the
/// Arlington PDF Model proof-of-concept C++ application. Performance overhead issues 
/// are considered irrelevant. See https://pdfix.github.io/pdfix_sdk_builds/en/6.1.0/html/.

#include "ArlingtonPDFShim.h"

#ifdef ARL_PDFSDK_PDFIX
#include <string>
#include <cassert>

#include "Pdfix.h"

using namespace ArlingtonPDFShim;
using namespace PDFixSDK;

Pdfix_statics;

void* ArlingtonPDFSDK::ctx;

struct pdfix_context {
    Pdfix* pdfix;
    PdfDoc* doc = nullptr;
    ~pdfix_context() {
        if (doc != nullptr) 
            doc->Close();
        pdfix->Destroy();
    }
};

/// @brief Initialize the PDF SDK. May throw exceptions.
void ArlingtonPDFSDK::initialize(bool enable_debugging)
{
    assert(ctx == nullptr);

    // initialize Pdfix
    std::wstring email = L"PDF Assoc. SafeDocs";
    std::wstring license_key = L"jgrrknzeuaDobhTt";

    if (!Pdfix_init(Pdfix_MODULE_NAME))
        throw std::runtime_error("Pdfix: Initialization failed");

    Pdfix* pdfix = GetPdfix();
    if (!pdfix)
        throw std::runtime_error("Pdfix: GetPdfix failed");

    if (pdfix->GetVersionMajor() != PDFIX_VERSION_MAJOR ||
        pdfix->GetVersionMinor() != PDFIX_VERSION_MINOR ||
        pdfix->GetVersionPatch() != PDFIX_VERSION_PATCH)
        throw std::runtime_error("Pdfix: Incompatible version");

    if (!pdfix->GetAccountAuthorization()->Authorize(email.c_str(), license_key.c_str()))
        throw std::runtime_error("Pdfix: Authorization failed");

    // Global namespaced flag to control debugging output
    ArlingtonPDFShim::debugging = enable_debugging;

    // Assign to void context
    auto pdfix_ctx = new pdfix_context;
    pdfix_ctx->pdfix = pdfix;

    ctx = pdfix_ctx;
}


/// @brief  Shutdown the PDF SDK
void ArlingtonPDFSDK::shutdown()
{
    if (ctx != nullptr) {
      auto pdfix_ctx = (pdfix_context*)ctx;
      delete (pdfix_ctx);
      ctx = nullptr;
    }
}


/// @brief  Returns human readable version string for PDF SDK that is being used
/// @return version string
std::string ArlingtonPDFSDK::get_version_string()
{
    assert(ctx != nullptr);
    Pdfix* pdfix = ((pdfix_context*)ctx)->pdfix;
    return "PDFix v" + std::to_string(pdfix->GetVersionMajor()) + "."
                     + std::to_string(pdfix->GetVersionMinor()) + "."
                     + std::to_string(pdfix->GetVersionPatch());
}


/// @brief   Opens a PDF file (no password) and locates trailer dictionary
/// @param   pdf_filename[in] PDF filename
/// @return  handle to PDF trailer dictionary or nullptr if trailer is not locatable
ArlPDFTrailer *ArlingtonPDFSDK::get_trailer(std::filesystem::path pdf_filename)
{
    assert(ctx != nullptr);
    auto pdfix_ctx = (pdfix_context*)ctx;
    if (pdfix_ctx->doc) {
      pdfix_ctx->doc->Close();
      pdfix_ctx->doc = nullptr;
    }

    pdfix_ctx->doc = pdfix_ctx->pdfix->OpenDoc(pdf_filename.wstring().data(), L"");

    if (pdfix_ctx->doc != nullptr)
    {
      PdsDictionary* trailer = pdfix_ctx->doc->GetTrailerObject();
      if (trailer != nullptr)
      {
        ArlPDFTrailer* trailer_obj = new ArlPDFTrailer(trailer);

        // if /Type key exists, then assume working with XRefStream
        PdsObject* type_key = trailer->Get(L"Type");
        trailer_obj->set_xrefstm(type_key != nullptr);

        int id = trailer->GetId();
        PdsObject* root_key = trailer->Get(L"Root");
        if (root_key != nullptr) {
          id = root_key->GetId();
          PdsObject* info_key = trailer->Get(L"Info");
          if (info_key != nullptr) {
            id = info_key->GetId();
          }
          else {
            if (ArlingtonPDFShim::debugging) {
              std::cout << __FUNCTION__ << " trailer Info key could not be found!" << std::endl;
            }
          }
          return trailer_obj;
        }
        if (ArlingtonPDFShim::debugging) {
          std::cout << __FUNCTION__ << " trailer Root key could not be found!" << std::endl;
        }
      }
      if (ArlingtonPDFShim::debugging) {
        std::cout << __FUNCTION__ << " trailer could not be found!" << std::endl;
      }
    }
    return nullptr;
}



/// @brief  Gets the PDF version of the PDF file
/// 
/// @param trailer   trailer of the PDF
/// 
/// @returns   PDF version string 
std::string ArlingtonPDFSDK::get_pdf_version(ArlPDFTrailer* trailer) {
    assert(ctx != nullptr);
    assert(trailer != nullptr);
    return "2.0"; /// @todo - how to get PDF version from PDFix??
}


/// @brief constructor
/// @param[in] obj 
ArlPDFObject::ArlPDFObject(void* obj) : object(obj)
{
    is_indirect = false;
    if (object == nullptr)
        return;
    is_indirect = (get_object_number() != 0);
}


/// @brief destructor
ArlPDFObject::~ArlPDFObject() {

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

    PdsObject *obj = (PdsObject *)object;
    PDFObjectType retval;

    switch (obj->GetObjectType())
    {
        case kPdsBoolean:
            retval = PDFObjectType::ArlPDFObjTypeBoolean;
            break;
        case kPdsNumber:
            retval = PDFObjectType::ArlPDFObjTypeNumber;    // Integer or Real (or bitmask)
            break;
        case kPdsString:
            retval = PDFObjectType::ArlPDFObjTypeString;    // Any type of string
            break;
        case kPdsName:
            retval = PDFObjectType::ArlPDFObjTypeName;
            break;
        case kPdsArray:
            retval = PDFObjectType::ArlPDFObjTypeArray;     // incl. rectangle or matrix
            break;
        case kPdsDictionary:
            retval = PDFObjectType::ArlPDFObjTypeDictionary;
            break;
        case kPdsStream:
            retval = PDFObjectType::ArlPDFObjTypeStream;
            break;
        case kPdsNull:
            retval = PDFObjectType::ArlPDFObjTypeNull;
            break;
        case kPdsReference:
            retval = PDFObjectType::ArlPDFObjTypeReference;
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
    if (ArlingtonPDFShim::debugging) {
        std::wcout << __FUNCTION__ << "(" << object << "): " << (is_indirect ? "true" : "false") << std::endl;
    }
    return is_indirect;
}

/// @brief   generates unique identifier for every object
/// @return  for indirect objects it returns the unique identifier (object number)
std::string ArlPDFObject::get_hash_id()
{
  assert(object != nullptr);
  return std::to_string(((PdsObject*)object)->GetId());
}


/// @brief  Returns the object number or 0 if a direct object
/// @return the object number or 0 if a direct object
int ArlPDFObject::get_object_number()
{
    assert(object != nullptr);
    int retval = ((PdsObject*)object)->GetId();
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
    assert(((PdsObject *)object)->GetObjectType() == kPdsBoolean);
    PdsBoolean* obj = (PdsBoolean *)object;
    bool retval = obj->GetValue();
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
    assert(((PdsObject*)object)->GetObjectType() == kPdsNumber);
    PdsNumber* obj = (PdsNumber*)object;
    bool retval = obj->IsIntegerValue();
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
    assert(((PdsObject*)object)->GetObjectType() == kPdsNumber);
    PdsNumber* obj = (PdsNumber*)object;
    assert(obj->IsIntegerValue());
    int retval = obj->GetIntegerValue();
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
    assert(((PdsObject*)object)->GetObjectType() == kPdsNumber);
    PdsNumber* obj = (PdsNumber*)object;
    double retval = obj->GetValue();
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
    assert(((PdsObject*)object)->GetObjectType() == kPdsString);
    PdsString* obj = (PdsString*)object;
    std::wstring retval = obj->GetText();
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
    assert(((PdsObject*)object)->GetObjectType() == kPdsName);
    PdsName* obj = (PdsName*)object;
    std::wstring retval = obj->GetText();
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
    assert(((PdsObject*)object)->GetObjectType() == kPdsArray);
    PdsArray* obj = (PdsArray*)object;
    int retval = obj->GetNumObjects();
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
    assert(((PdsObject*)object)->GetObjectType() == kPdsArray);
    PdsArray* obj = (PdsArray*)object;
    PdsObject* type_obj = obj->Get(idx);
    ArlPDFObject* retval = nullptr;
    if (type_obj!=nullptr)
     retval = new ArlPDFObject(type_obj);

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
    assert(((PdsObject*)object)->GetObjectType() == kPdsDictionary);
    PdsDictionary* obj = (PdsDictionary*)object;
    int retval = obj->GetNumKeys();
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
    assert(((PdsObject*)object)->GetObjectType() == kPdsDictionary);
    PdsDictionary* obj = (PdsDictionary*)object;
    bool retval = obj->Known(key.c_str());
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
    assert(((PdsObject*)object)->GetObjectType() == kPdsDictionary);
    PdsDictionary* obj = (PdsDictionary*)object;

    PdsObject* type_obj = obj->Get(key.c_str());
    ArlPDFObject* retval = nullptr;
    if (type_obj != nullptr)
      retval = new ArlPDFObject(type_obj);

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
    assert(((PdsObject*)object)->GetObjectType() == kPdsDictionary);
    PdsDictionary* obj = (PdsDictionary*)object;
    std::wstring retval = obj->GetKey(index);
    if (ArlingtonPDFShim::debugging) {
        std::wcout << __FUNCTION__ << "(" << index << "): '" << retval << "'" << std::endl;
    }
    return retval;
}

/// @brief  Gets the dictionary associated with the PDF stream
/// @return the PDF dictionary object
ArlPDFDictionary* ArlPDFStream::get_dictionary()
{
  assert(object != nullptr);
  assert(((PdsObject*)object)->GetObjectType() == kPdsStream);
  PdsStream* obj = (PdsStream*)object;
  PdsDictionary* stm_dict = obj->GetStreamDict();
  assert(stm_dict != nullptr);
  ArlPDFDictionary* retval = new ArlPDFDictionary(stm_dict);
  if (ArlingtonPDFShim::debugging) {
    std::wcout << __FUNCTION__ << retval << std::endl;
  }
  return retval;
}


/// @brief Returns the number of keys in a PDF stream
/// @return Number of keys (>= 0)
//int ArlPDFStream::get_num_keys()
//{
//    assert(object != nullptr);
//    assert(((PdsObject*)object)->GetObjectType() == kPdsStream);
//    PdsStream* obj = (PdsStream*)object;
//    PdsDictionary* stm_dict = obj->GetStreamDict();
//    assert(stm_dict != nullptr);
//    int retval = stm_dict->GetNumKeys();
//    if (ArlingtonPDFShim::debugging) {
//        std::wcout << __FUNCTION__ << "(" << object << "): " << retval << std::endl;
//    }
//    return retval;
//}
//
//
///// @brief  Checks whether a PDF stream object has a specific key
///// @param key the key name
///// @return true if the dictionary has the specified key
//bool ArlPDFStream::has_key(std::wstring key)
//{
//    assert(object != nullptr);
//    assert(((PdsObject*)object)->GetObjectType() == kPdsStream);
//    PdsStream* obj = (PdsStream*)object;
//    PdsDictionary* stm_dict = obj->GetStreamDict();
//    assert(stm_dict != nullptr);
//    bool retval = stm_dict->Known(key.c_str());
//    if (ArlingtonPDFShim::debugging) {
//        std::wcout << __FUNCTION__ << "(" << key << "): " << (retval ? "true" : "false") << std::endl;
//    }
//    return retval;
//}
//
//
///// @brief  Gets the object associated with the key from a PDF stream
///// @param key the key name
///// @return the PDF object value of key
//ArlPDFObject* ArlPDFStream::get_value(std::wstring key)
//{
//    assert(object != nullptr);
//    assert(((PdsObject*)object)->GetObjectType() == kPdsStream);
//    PdsStream* obj = (PdsStream*)object;
//    PdsDictionary* stm_dict = obj->GetStreamDict();
//    assert(stm_dict != nullptr);
//    ArlPDFObject * retval = new ArlPDFObject(stm_dict->Get(key.c_str()));
//    if (ArlingtonPDFShim::debugging) {
//        std::wcout << __FUNCTION__ << "(" << key << "): " << retval << std::endl;
//    }
//    return retval;
//}
//
//
///// @brief Returns the key name of i-th stream key
///// @param index[in] dictionary key index
///// @return Key name
//std::wstring ArlPDFStream::get_key_name_by_index(int index)
//{
//    assert(object != nullptr);
//    assert(index >= 0);
//    assert(((PdsObject*)object)->GetObjectType() == kPdsStream);
//    PdsStream* obj = (PdsStream*)object;
//    PdsDictionary* stm_dict = obj->GetStreamDict();
//    assert(stm_dict != nullptr);
//    std::wstring retval = stm_dict->GetKey(index);
//    if (ArlingtonPDFShim::debugging) {
//        std::wcout << __FUNCTION__ << "(" << index << "): '" << retval << "'" << std::endl;
//    }
//    return retval;
//}

#endif // ARL_PDFSDK_PDFIX
