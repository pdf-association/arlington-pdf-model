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
// Contributors: Roman Toda, Normex. 
//
///////////////////////////////////////////////////////////////////////////////

/// @file
/// A wafer-thin shim layer to isolate the PDFix SDK library from the rest of the 
/// Arlington PDF Model proof-of-concept C++ application. Replace just this C++ file 
/// for alternate PDF SDKs. Performance overhead issues are considered irrelevant. 

#include "ArlingtonPDFShim.h"

#ifdef ARL_PDFSDK_PDFIUM
#include <string>
#include <cassert>


//pdfium
#include "core/fxcodec/fx_codec.h"
#include "core/fpdfapi/page/cpdf_pagemodule.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fpdfapi/page/cpdf_docpagedata.h"
#include "core/fpdfapi/render/cpdf_docrenderdata.h"

#include "core/fxge/cfx_gemodule.h"
#include "core/fpdfapi/parser/cpdf_object.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_boolean.h"
#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_name.h"
#include "core/fpdfapi/parser/cpdf_number.h"
#include "core/fpdfapi/parser/cpdf_reference.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fpdfapi/parser/cpdf_string.h"
#include "core/fpdfapi/parser/fpdf_parser_decode.h"
#include "core/fxcrt/fx_memory.h"

using namespace ArlingtonPDFShim;

void* ArlingtonPDFSDK::ctx;

struct pdfium_context {
  CPDF_Document* document = nullptr;
  ~pdfium_context() {
    if (document)
      delete (document);
    //CPDF_PageModule::Destroy();
    //CFX_GEModule::Destroy();
  }
};

/// @brief Initialize the PDF SDK. May throw exceptions.
void ArlingtonPDFSDK::initialize(bool enable_debugging)
{
    assert(ctx == nullptr);

    auto pdfium_ctx = new pdfium_context;

    FXMEM_InitializePartitionAlloc();
    CFX_GEModule::Create(nullptr);
    CPDF_PageModule::Create();
    ctx = pdfium_ctx;
    ArlingtonPDFShim::debugging = enable_debugging;
}

/// @brief  Shutdown the PDF SDK
void ArlingtonPDFSDK::shutdown()
{
    //destroy pdfium
    auto pdfium_ctx = (pdfium_context*)ctx;
    if (pdfium_ctx->document != nullptr) {
      delete pdfium_ctx->document;
      pdfium_ctx->document = nullptr;
    }

    CPDF_PageModule::Destroy();
    CFX_GEModule::Destroy();
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
/// 
/// @param[in]   pdf_filename PDF filename
/// 
/// @returns  handle to PDF trailer dictionary or nullptr if trailer is not locatable
ArlPDFTrailer *ArlingtonPDFSDK::get_trailer(std::filesystem::path pdf_filename)
{
    assert(ctx != nullptr);
    auto pdfium_ctx = (pdfium_context*)ctx;
    
    // close previously opened document
    if (pdfium_ctx->document != nullptr) {
      delete(pdfium_ctx->document);
	    pdfium_ctx->document = nullptr;
	  }

    RetainPtr<IFX_SeekableReadStream> file_access = IFX_SeekableReadStream::CreateFromFilename(pdf_filename.string().c_str());
    if (!file_access)
      return nullptr;

    pdfium_ctx->document = new CPDF_Document(std::make_unique<CPDF_DocRenderData>(), std::make_unique<CPDF_DocPageData>());
    ByteString password;
    CPDF_Parser::Error err_code = pdfium_ctx->document->LoadDoc(file_access, password);
    if (err_code) {
      delete(pdfium_ctx->document);
	    pdfium_ctx->document = nullptr;
      return nullptr;
	  }

    const CPDF_Dictionary* trailr = pdfium_ctx->document->GetParser()->GetTrailer();
    if (trailr != NULL) {
        ArlPDFTrailer* trailer_obj = new ArlPDFTrailer((void*)trailr);
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

/// @brief  Gets the PDF version of the current PDF file
/// 
/// @param[in] trailer   trailer of the PDF
/// 
/// @returns   PDF version string 
std::string ArlingtonPDFSDK::get_pdf_version(ArlPDFTrailer* trailer) {
    assert(ctx != nullptr);
    assert(trailer != nullptr);
    auto pdfium_ctx = (pdfium_context*)ctx;
    assert(pdfium_ctx->document != nullptr);
    int ver = pdfium_ctx->document->GetParser()->GetFileVersion();
    // ver = PDF header version x 10 (so PDF 1.3 = 13)
    char version_str[4];
    snprintf(version_str, 4, "%1.1f", ver / 10.0);
    return version_str;

}

ArlPDFObject::ArlPDFObject(void* obj):object(obj)
{
  is_indirect = false;
  if (object == nullptr)
    return;
  CPDF_Object* pdf_obj = (CPDF_Object*)object;
  if (pdf_obj->GetType() == CPDF_Object::kReference) {
    object = pdf_obj->GetDirect(); 
    is_indirect = true;
  } 
}

ArlPDFObject::~ArlPDFObject()
{
  if (object == nullptr)
    return;
  CPDF_Object* pdf_obj = (CPDF_Object*)object;
  if (is_indirect) {
    /// @todo Release object or free memory ???
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
    case CPDF_Object::kBoolean:
            retval = PDFObjectType::ArlPDFObjTypeBoolean; 
            break;
    case CPDF_Object::kNumber:
            retval = PDFObjectType::ArlPDFObjTypeNumber;    // Integer or Real (or bitmask)
            break;
        case CPDF_Object::kString:
            retval = PDFObjectType::ArlPDFObjTypeString;    // Any type of string
            break;
        case CPDF_Object::kName:
            retval = PDFObjectType::ArlPDFObjTypeName;
            break;
        case CPDF_Object::kArray:
            retval = PDFObjectType::ArlPDFObjTypeArray;     // incl. rectangle or matrix
            break;
        case CPDF_Object::kDictionary:
            retval = PDFObjectType::ArlPDFObjTypeDictionary;
            break;
        case CPDF_Object::kStream:
            retval = PDFObjectType::ArlPDFObjTypeStream;
            break;
        case CPDF_Object::kNullobj:
            retval = PDFObjectType::ArlPDFObjTypeNull;
            break;
        case CPDF_Object::kReference:
            object = obj->GetDirect();
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
    assert(((CPDF_Object *)object)->GetType() == CPDF_Object::kBoolean);
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
    assert(((CPDF_Object*)object)->GetType() == CPDF_Object::kNumber);
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
    assert(((CPDF_Object*)object)->GetType() == CPDF_Object::kNumber);
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
    assert(((CPDF_Object*)object)->GetType() == CPDF_Object::kNumber);
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
    assert(((CPDF_Object*)object)->GetType() == CPDF_Object::kString);
    CPDF_String* obj = ((CPDF_String*)object);
    std::wstring retval = obj->GetUnicodeText().c_str(); 
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
    assert(((CPDF_Object*)object)->GetType() == CPDF_Object::kName);
    CPDF_Name* obj = ((CPDF_Name*)object);
    std::wstring retval = obj->GetUnicodeText().c_str();  
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
    assert(((CPDF_Object*)object)->GetType() == CPDF_Object::kArray);
    CPDF_Array* obj = ((CPDF_Array*)object);
    int retval = obj->size(); 
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
    assert(((CPDF_Object*)object)->GetType() == CPDF_Object::kArray);
    CPDF_Array* obj = ((CPDF_Array*)object);

    ArlPDFObject* retval = nullptr;
    CPDF_Object* type_key = obj->GetObjectAt(idx);
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
    assert(((CPDF_Object*)object)->GetType() == CPDF_Object::kDictionary);
    CPDF_Dictionary* obj = ((CPDF_Dictionary*)object);
    int retval = obj->size();
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
    assert(((CPDF_Object*)object)->GetType() == CPDF_Object::kDictionary);
    CPDF_Dictionary* obj = ((CPDF_Dictionary*)object);
      
    bool retval = obj->KeyExist(WideString(key.c_str()).ToUTF8());
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
    assert(((CPDF_Object*)object)->GetType() == CPDF_Object::kDictionary);
    ArlPDFObject* retval = nullptr;
    CPDF_Dictionary* obj = ((CPDF_Dictionary*)object);

    CPDF_Object* type_key = obj->GetObjectFor(WideString(key.c_str()).ToUTF8());
    if (type_key != NULL)
        retval = new ArlPDFObject(type_key);

    if (ArlingtonPDFShim::debugging) {
        std::wcout << __FUNCTION__ << "(" << key << "): " << retval << std::endl;
    }
    return retval;
}


/// @brief Returns the key name of i-th dictionary key
/// 
/// @param[in] index dictionary key index
/// 
/// @returns Key name
std::wstring ArlPDFDictionary::get_key_name_by_index(int index)
{
    assert(object != nullptr);
    assert(index >= 0);
    assert(((CPDF_Object*)object)->GetType() == CPDF_Object::kDictionary);
    CPDF_Dictionary* obj = ((CPDF_Dictionary*)object); 

    std::wstring retval;
    const auto& keys = obj->GetKeys();
    if (index < keys.size())
      retval = PDF_DecodeText(keys[index].raw_span()).c_str();

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
    assert(((CPDF_Object*)object)->GetType() == CPDF_Object::kStream);
    assert(((CPDF_Object*)object)->GetType() == CPDF_Object::kStream);
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

#endif // ARL_PDFSDK_PDFIUM
