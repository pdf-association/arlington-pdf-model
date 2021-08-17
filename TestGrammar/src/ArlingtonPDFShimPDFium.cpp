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

void* ArlingtonPDFSDK::ctx;

struct pdfium_context {
  CPDF_Parser* parser=nullptr;
  CCodec_ModuleMgr* codecModule;
  ~pdfium_context() {
    if (parser) delete(parser);
    codecModule->Destroy();
  }
};

/// @brief Initialize the PDF SDK. May throw exceptions.
void ArlingtonPDFSDK::initialize(bool enable_debugging)
{
    assert(ctx == nullptr);

    auto pdfium_ctx = new pdfium_context;
    CPDF_ModuleMgr::Create();
    auto moduleMgr = CPDF_ModuleMgr::Get();
    pdfium_ctx->codecModule = CCodec_ModuleMgr::Create();
    moduleMgr->SetCodecModule(pdfium_ctx->codecModule);
    //moduleMgr->InitPageModule();
    //moduleMgr->LoadEmbeddedGB1CMaps();
    //moduleMgr->LoadEmbeddedJapan1CMaps();
    //moduleMgr->LoadEmbeddedCNS1CMaps();
    //moduleMgr->LoadEmbeddedKorea1CMaps();
    pdfium_ctx->parser = nullptr;

    ctx = pdfium_ctx;
    ArlingtonPDFShim::debugging = enable_debugging;
}

/// @brief  Shutdown the PDF SDK
void ArlingtonPDFSDK::shutdown()
{
  //destroy pdfium
  auto pdfium_ctx = (pdfium_context*)ctx;
  delete(pdfium_ctx);
  CPDF_ModuleMgr::Destroy();
  ctx = nullptr;
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
    auto pdfium_ctx = (pdfium_context*)ctx;
    
    // close previously opened document
    if (pdfium_ctx->parser)
      delete(pdfium_ctx->parser);
    pdfium_ctx->parser = new CPDF_Parser;

    //pdfium_ctx->parser->SetPassword(password);
    FX_DWORD err_code = pdfium_ctx->parser->StartParse((FX_LPCSTR)pdf_filename.string().c_str());
    if (err_code)
      return nullptr;

    CPDF_Dictionary* trailr = pdfium_ctx->parser->GetTrailer();
    if (trailr != NULL) {
        ArlPDFTrailer* trailer_obj = new ArlPDFTrailer(trailr);
        // if /Type key exists, then assume working with XRefStream
        ArlPDFObject* type_key= trailer_obj->get_value(L"Type");
        trailer_obj->set_xrefstm(type_key != nullptr);

        //FX_DWORD  GetObjNum() const
        //FX_DWORD  GetGenNum() const
        int id = trailr->GetObjNum();
        CPDF_Object* root_key = (CPDF_Object*)trailer_obj->get_value(L"Root");
        if (root_key != NULL) {
            id = root_key->GetObjNum();
        
            CPDF_Object* info_key = (CPDF_Object*)trailer_obj->get_value(L"Info");
            if (info_key != NULL) {
                id = info_key->GetObjNum();
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
    return nullptr;
}


ArlPDFObject::ArlPDFObject(void* obj):object(obj)
{
  is_indirect = false;
  if (object == nullptr)
    return;
  CPDF_Object* pdf_obj = (CPDF_Object*)object;
  if (pdf_obj->GetType() == PDFOBJ_REFERENCE) {
    object = ((pdfium_context*)ArlingtonPDFSDK::ctx)->parser->GetDocument()->GetIndirectObject(((CPDF_Reference*)object)->GetRefObjNum());
    is_indirect = true;
  } 
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
             object = ((pdfium_context*)ArlingtonPDFSDK::ctx)->parser->GetDocument()->GetIndirectObject(((CPDF_Reference*)object)->GetRefObjNum());
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
  return std::to_string(((CPDF_Object*)object)->GetObjNum()) + "_"+ std::to_string(((CPDF_Object*)object)->GetGenNum());
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
    CPDF_Boolean* obj = ((CPDF_Boolean*)object);
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
    CPDF_Number* obj = ((CPDF_Number*)object);
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
    CPDF_Number* obj = ((CPDF_Number*)object);
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
    CPDF_Number* obj = ((CPDF_Number*)object);
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
    CPDF_String* obj = ((CPDF_String*)object);
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
    CPDF_Name* obj = ((CPDF_Name*)object);
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
    CPDF_Array* obj = ((CPDF_Array*)object);
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
    CPDF_Array* obj = ((CPDF_Array*)object);

    ArlPDFObject* retval = nullptr;
    CPDF_Object* type_key = obj->GetElement(idx);
    if (type_key !=nullptr)
      retval = new ArlPDFObject(type_key);

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
    CPDF_Dictionary* obj = ((CPDF_Dictionary*)object);
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
    CPDF_Dictionary* obj = ((CPDF_Dictionary*)object);
      
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
    ArlPDFObject* retval = nullptr;
    CPDF_Dictionary* obj = ((CPDF_Dictionary*)object);

    CPDF_Object* type_key = obj->GetElement(CFX_ByteString::FromUnicode(key.c_str()));
    if (type_key != NULL)
        retval = new ArlPDFObject(type_key);

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
    CPDF_Dictionary* obj = ((CPDF_Dictionary*)object); 

    FX_POSITION pos = obj->GetStartPos();
    std::wstring retval;
    while (pos) {
      CFX_ByteString keyName;
      CPDF_Object* nextObj = obj->GetNextElement(pos, keyName);
      if (index == 0) {
        retval = (FX_LPCWSTR)keyName.UTF8Decode();
        break;
      }
      index--;
    }

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
    assert(((CPDF_Object*)object)->GetType() == PDFOBJ_STREAM);
    CPDF_Stream* obj = ((CPDF_Stream*)object); 
    CPDF_Dictionary* stm_dict = obj->GetDict();
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
//    assert(((CPDF_Object*)object)->GetType() == PDFOBJ_STREAM);
//    CPDF_Stream* obj = ((CPDF_Stream*)object); 
//    CPDF_Dictionary* stm_dict = obj->GetDict();
//    assert(stm_dict != nullptr);
//    int retval = stm_dict->GetCount();
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
//    assert(((CPDF_Object*)object)->GetType() == PDFOBJ_STREAM);
//    CPDF_Stream* obj = ((CPDF_Stream*)object); 
//    CPDF_Dictionary* stm_dict = obj->GetDict();
//    assert(stm_dict != nullptr);
//    bool retval = stm_dict->KeyExist(CFX_ByteString::FromUnicode(key.c_str()));
//    if (ArlingtonPDFShim::debugging) {
//        std::wcout << __FUNCTION__ << "(" << key << "): " << (retval ? "true" : "false") << std::endl;
//    }
//    return retval;
//}


///// @brief  Gets the object associated with the key from a PDF stream
///// @param key the key name
///// @return the PDF object value of key
//ArlPDFObject* ArlPDFStream::get_value(std::wstring key)
//{
//    assert(object != nullptr);
//    assert(((CPDF_Object*)object)->GetType() == PDFOBJ_STREAM);
//    CPDF_Stream* obj = ((CPDF_Stream*)object); 
//    CPDF_Dictionary* stm_dict = obj->GetDict();
//    assert(stm_dict != nullptr);
//    ArlPDFObject * retval = new ArlPDFObject(stm_dict->GetElement(CFX_ByteString::FromUnicode(key.c_str())));
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
//    assert(((CPDF_Object*)object)->GetType() == PDFOBJ_STREAM);
//    CPDF_Stream* obj = ((CPDF_Stream*)object); 
//    CPDF_Dictionary* stm_dict = obj->GetDict();
//    assert(stm_dict != nullptr);
//
//    int pos_index = 0;
//    std::wstring retval;
//    FX_POSITION pos = stm_dict->GetStartPos();
//    while (pos) {
//      CFX_ByteString keyName;
//      CPDF_Object* nextObj = stm_dict->GetNextElement(pos, keyName);
//      if (pos_index == index) {
//        retval = (FX_LPCWSTR)keyName.UTF8Decode();
//        break;
//      }
//      pos_index++;
//    }
//
//    if (ArlingtonPDFShim::debugging) {
//        std::wcout << __FUNCTION__ << "(" << index << "): '" << retval << "'" << std::endl;
//    }
//    return retval;
//}
