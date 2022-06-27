///////////////////////////////////////////////////////////////////////////////
/// @file 
/// @brief  Arlington PDFium SDK shim layer
/// 
/// A wafer-thin shim layer to isolate the pdfium SDK library from the rest of the
/// Arlington PDF Model proof-of-concept C++ application. Performance and memory
/// overhead issues are considered irrelevant.
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
/// @author Roman Toda, Normex
/// @author Peter Wyatt, PDF Association
///
///////////////////////////////////////////////////////////////////////////////

#include "ArlingtonPDFShim.h"

#ifdef ARL_PDFSDK_PDFIUM
#include <algorithm>
#include <string>
#include <vector>
#include <cassert>


//pdfium
#include "core/include/fxcodec/fx_codec.h"
#include "core/include/fxge/fx_ge.h"
#include "core/include/fpdfapi/fpdf_parser.h"
#include "core/include/fpdfapi/fpdf_module.h"

using namespace ArlingtonPDFShim;

void* ArlingtonPDFSDK::ctx = nullptr;

struct pdfium_context {
    CPDF_Parser*        parser;
    CPDF_ModuleMgr*     moduleMgr;
    CCodec_ModuleMgr*   codecModule;

    pdfium_context() {
        /* Default constructor */
        parser = nullptr;
        CPDF_ModuleMgr::Create();
        codecModule = CCodec_ModuleMgr::Create();
        moduleMgr = CPDF_ModuleMgr::Get();
        moduleMgr->SetCodecModule(codecModule);
        // moduleMgr->InitPageModule();
        // moduleMgr->InitRenderModule();
        // moduleMgr->LoadEmbeddedGB1CMaps();
        // moduleMgr->LoadEmbeddedJapan1CMaps();
        // moduleMgr->LoadEmbeddedCNS1CMaps();
        // moduleMgr->LoadEmbeddedKorea1CMaps();
    };

    ~pdfium_context() {
        /* Destructor */
/// @todo pdfium-based Linux release builds always segfault on exit!!
///       NULL-pointer dereference in CFX_Plex::FreeDataChain (fx_basic_plex.cpp:24)
#if !defined(__linux__) && !defined(DEBUG)
        if (parser != nullptr) {
            parser->CloseParser();
            delete(parser);
        }
        if (codecModule != nullptr)
            codecModule->Destroy();
        if (moduleMgr != nullptr)
            moduleMgr->Destroy();
#endif
    };
};

/// @brief Initialize the PDF SDK. May throw exceptions.
void ArlingtonPDFSDK::initialize(bool enable_debugging)
{
    assert(ctx == nullptr);
    auto pdfium_ctx = new pdfium_context;
    ctx = pdfium_ctx;
    ArlingtonPDFShim::debugging = enable_debugging;
}

/// @brief  Shutdown the PDF SDK
void ArlingtonPDFSDK::shutdown()
{
    if (ctx != nullptr) {
        delete((pdfium_context*)ctx);
        ctx = nullptr;
    }
}

/// @brief  Returns human readable version string for PDF SDK that is being used
/// @returns version string
std::string ArlingtonPDFSDK::get_version_string()
{
    assert(ctx != nullptr);
    return "pdfium";
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
    if (pdfium_ctx->parser != nullptr) {
        pdfium_ctx->parser->CloseParser();
        delete(pdfium_ctx->parser);
    }
    pdfium_ctx->parser = new CPDF_Parser;

    //pdfium_ctx->parser->SetPassword(password);
    FX_DWORD err_code = pdfium_ctx->parser->StartParse((FX_LPCSTR)pdf_filename.string().c_str());
    if (err_code) {
        delete pdfium_ctx->parser;
        pdfium_ctx->parser = nullptr;
        return nullptr;
    }

    CPDF_Dictionary* trailr = pdfium_ctx->parser->GetTrailer();
    if (trailr != NULL) {
        ArlPDFTrailer* trailer_obj = new ArlPDFTrailer(trailr);
        // if /Type key exists, then assume working with XRefStream
        ArlPDFObject* type_key= trailer_obj->get_value(L"Type");
        trailer_obj->set_xrefstm(type_key != nullptr);
        delete type_key;
        return trailer_obj;
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
    assert(pdfium_ctx->parser != nullptr);
    int ver = pdfium_ctx->parser->GetFileVersion();
    // ver = PDF header version x 10 (so PDF 1.3 = 13)
    char version_str[4];
    snprintf(version_str, 4, "%1.1f", ver / 10.0);
    return version_str;
}


CPDF_Object* pdfium_is_indirect_valid(CPDF_Object* obj) {
    assert(obj != nullptr);
    assert(obj->GetType() == PDFOBJ_REFERENCE);
    FX_DWORD    obj_num = ((CPDF_Reference*)obj)->GetRefObjNum();
    CPDF_Object* pdf_ir = ((pdfium_context*)ArlingtonPDFSDK::ctx)->parser->GetDocument()->GetIndirectObject(obj_num);
    return pdf_ir;
}


/// @brief Constructor taking a PDF SDK generic pointer for storage
ArlPDFObject::ArlPDFObject(void* obj) : object(obj)
{
    assert(object != nullptr);
    CPDF_Object* pdf_obj = (CPDF_Object*)object;
    int obj_type = pdf_obj->GetType();
    assert(obj_type != PDFOBJ_INVALID);
    is_indirect = (obj_type == PDFOBJ_REFERENCE);
}


/// @brief Destructor for all objects
ArlPDFObject::~ArlPDFObject()
{
    sorted_keys.clear(); 
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
            {
                FX_DWORD r = ((CPDF_Reference*)object)->GetRefObjNum();
                CPDF_Object* ir_obj = ((pdfium_context*)ArlingtonPDFSDK::ctx)->parser->GetDocument()->GetIndirectObject(r);
                //??? delete object; or ((CPDF_Object*)object)->Release(); ??
                object = ir_obj;
                retval = get_object_type(); 
            }
            break;
        case PDFOBJ_INVALID: /* fallthrough */
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
    if (((CPDF_Object*)object)->GetType() != PDFOBJ_REFERENCE) {
        return std::to_string(((CPDF_Object*)object)->GetObjNum()) + "_" + std::to_string(((CPDF_Object*)object)->GetGenNum());
    }
    else {
        CPDF_Reference* r = (CPDF_Reference*)object;
        return std::to_string(r->GetRefObjNum()) + "_" + std::to_string(r->GetGenNum());
    }
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


/// @brief Checks if keys are already sorted and, if not, then sorts and caches
void ArlPDFObject::sort_keys()
{
    if (sorted_keys.empty()) {
        assert(((CPDF_Object*)object)->GetType() == PDFOBJ_DICTIONARY);
        CPDF_Dictionary* dict = ((CPDF_Dictionary*)object);

        // Get all the keys in the dictionary
        FX_POSITION pos = dict->GetStartPos();
        while (pos) {
            CFX_ByteString keyName;
            (void)dict->GetNextElement(pos, keyName);
            std::wstring key = (FX_LPCWSTR)keyName.UTF8Decode();
            sorted_keys.push_back(key);
        }
        // Sort the keys
        if (sorted_keys.size() > 1)
            std::sort(sorted_keys.begin(), sorted_keys.end());
    }
}


/// @brief   Returns the value of a PDF boolean object
/// @return  Returns true or false
bool ArlPDFBoolean::get_value()
{
    assert(object != nullptr);
    assert(((CPDF_Object *)object)->GetType() == PDFOBJ_BOOLEAN);
    CPDF_Boolean* obj = ((CPDF_Boolean*)object);
    bool retval = (obj->GetInteger() != 0);
    if (ArlingtonPDFShim::debugging) {
        std::wcout << __FUNCTION__ << "(" << object << "): " << (retval ? "true" : "false") << std::endl;
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
    if (type_key != nullptr) {
        int t = type_key->GetType();
        assert(t != PDFOBJ_INVALID);
        retval = new ArlPDFObject(type_key);
    }

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

    CFX_ByteString bstr = CFX_ByteString::FromUnicode(key.c_str());
    CPDF_Object* type_key = obj->GetElement(bstr);
    if (type_key != NULL)
        retval = new ArlPDFObject(type_key);

    if (ArlingtonPDFShim::debugging) {
        std::wcout << __FUNCTION__ << "(" << key << "): " << retval << std::endl;
    }
    return retval;
}


/// @brief Returns the key name of i-th dictionary key. Keys need to be
/// alphabetically sorted so that output order matches other PDF SDKs (PDFix)
/// 
/// @param[in] index dictionary key index
/// 
/// @returns Key name
std::wstring ArlPDFDictionary::get_key_name_by_index(int index)
{
    assert(object != nullptr);
    assert(index >= 0);
    std::wstring     retval;

    sort_keys();
    // Get the i-th sorted key name, allowing for no keys in a dictionary 
    if ((!sorted_keys.empty()) && (index < sorted_keys.size()))
        retval = sorted_keys[index];

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

#endif // ARL_PDFSDK_PDFIUM
