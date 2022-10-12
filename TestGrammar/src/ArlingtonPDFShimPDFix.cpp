///////////////////////////////////////////////////////////////////////////////
/// @file
/// @brief  Arlington PDFix SDK shim layer
///
/// A wafer-thin shim layer to isolate the PDFix SDK library from the rest of the
/// Arlington PDF Model proof-of-concept C++ application. Performance and memory
/// overhead issues are considered irrelevant.
/// See https://pdfix.github.io/pdfix_sdk_builds/en/6.1.0/html/.
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

#ifdef ARL_PDFSDK_PDFIX
#include <algorithm>
#include <string>
#include <cassert>
#include <iostream>
#include <fstream>

#include "Pdfix.h"
#include "ArlPredicates.h"
#include "utils.h"

using namespace ArlingtonPDFShim;
using namespace PDFixSDK;

Pdfix_statics;

void* ArlingtonPDFSDK::ctx = nullptr;

struct pdfix_context {
    Pdfix*                  pdfix = nullptr;
    PdfDoc*                 doc = nullptr;
    std::filesystem::path   pdf_file;
    ArlPDFTrailer*          pdf_trailer = nullptr;
    ArlPDFDictionary*       pdf_catalog = nullptr;

    ~pdfix_context() {
        if (doc != nullptr)
            doc->Close();
        if (pdfix != nullptr)
            pdfix->Destroy();
    }
};



/// @brief Initialize the PDF SDK. May throw exceptions.
void ArlingtonPDFSDK::initialize()
{
    assert(ctx == nullptr);

    // initialize Pdfix
    std::wstring email = L"PDF Assoc. SafeDocs";
    std::wstring license_key = L"jgrrknzeuaDobhTt";

    if (!Pdfix_init(Pdfix_MODULE_NAME))
        throw std::runtime_error("Pdfix: Initialization failed for " Pdfix_MODULE_NAME);

    Pdfix* pdfix = GetPdfix();
    if (pdfix == nullptr)
        throw std::runtime_error("Pdfix: GetPdfix failed");

    if (pdfix->GetVersionMajor() != PDFIX_VERSION_MAJOR ||
        pdfix->GetVersionMinor() != PDFIX_VERSION_MINOR ||
        pdfix->GetVersionPatch() != PDFIX_VERSION_PATCH)
        throw std::runtime_error("Pdfix: Incompatible version");

    if (!pdfix->GetAccountAuthorization()->Authorize(email.c_str(), license_key.c_str()))
        throw std::runtime_error("Pdfix: Authorization failed");

    // Assign to void context
    auto pdfix_ctx = new pdfix_context;
    pdfix_ctx->pdfix = pdfix;
    ctx = pdfix_ctx;
}


/// @brief  Shutdown the PDFix SDK
void ArlingtonPDFSDK::shutdown()
{
    if (ctx != nullptr) {
        auto pdfix_ctx = (pdfix_context*)ctx;
        delete pdfix_ctx;
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


/// @brief   Opens a PDF file (optional password) 
/// 
/// @param[in]   pdf_filename PDF filename
/// @param[in]   password     optional password
/// 
/// @return  true if PDF can be opened, false otherwise
bool ArlingtonPDFSDK::open_pdf(const std::filesystem::path& pdf_filename, const std::wstring& password)
{
    assert(ctx != nullptr);
    assert(!pdf_filename.empty());
    auto pdfix_ctx = (pdfix_context*)ctx;
    if (pdfix_ctx->doc != nullptr) {
        pdfix_ctx->doc->Close();
        pdfix_ctx->doc = nullptr;
    }

    pdfix_ctx->pdf_file = pdf_filename;
    pdfix_ctx->doc = pdfix_ctx->pdfix->OpenDoc(pdf_filename.wstring().data(), password.data());

    if (pdfix_ctx->doc != nullptr) {
        PdsDictionary* trailer = pdfix_ctx->doc->GetTrailerObject();
        if (trailer != nullptr)
        {
            // if /Type key exists, then assume working with XRefStream
            PdsObject* type_key = trailer->Get(L"Type");

            // If /Encrypt key exists, then assume encrypted PDF
            // pdfix_ctx->doc->IsSecured()
            PdsObject* enc_key = trailer->Get(L"Encrypt");

            /// @todo - how to determine if supported or unsupported encryption

            pdfix_ctx->pdf_trailer = new ArlPDFTrailer(trailer, 
                                                (type_key != nullptr),  // has a xref stream
                                                (enc_key != nullptr),   // is encrypted
                                                false                   // is unsupported encryption
                                         );
            pdfix_ctx->pdf_catalog = new ArlPDFDictionary(pdfix_ctx->pdf_trailer, pdfix_ctx->doc->GetRootObject(), false);
            return true;
        }
    }
    // For ease of debugging, grab the err into a write-only variable
    auto err_msg = pdfix_ctx->pdfix->GetError();
    UNREFERENCED_FORMAL_PARAM(err_msg);
    return false;
}



/// @brief Close a previously opened PDF file. Frees all memory for a file so multiple PDFs don't accumulate leaked memory.
void ArlingtonPDFSDK::close_pdf() {
    assert(ctx != nullptr);
    auto pdfix_ctx = (pdfix_context*)ctx;

    pdfix_ctx->pdf_catalog->force_deleteable();
    delete pdfix_ctx->pdf_catalog;
    pdfix_ctx->pdf_catalog = nullptr;

    pdfix_ctx->pdf_trailer->force_deleteable();
    delete pdfix_ctx->pdf_trailer;
    pdfix_ctx->pdf_trailer = nullptr;

    if (pdfix_ctx->doc != nullptr) {
        pdfix_ctx->doc->Close();
        pdfix_ctx->doc = nullptr;
    }
}



/// @brief   Returns the trailer dictionary-like object
/// 
/// @return  handle to PDF trailer dictionary or nullptr if trailer is not locatable
ArlPDFTrailer* ArlingtonPDFSDK::get_trailer()
{
    assert(ctx != nullptr);
    auto pdfix_ctx = (pdfix_context*)ctx;
    return pdfix_ctx->pdf_trailer;
}



/// @brief   Returns the trailer dictionary-like object
/// 
/// @return  handle to PDF document catalog or nullptr if not locatable
ArlPDFDictionary* ArlingtonPDFSDK::get_document_catalog()
{
    assert(ctx != nullptr);
    auto pdfix_ctx = (pdfix_context*)ctx;
    return pdfix_ctx->pdf_catalog;
}



/// @brief  Gets the PDF version of the PDF file
///
/// @returns   PDF version string
std::string ArlingtonPDFSDK::get_pdf_version() {
    assert(ctx != nullptr);
    auto pdfix_ctx = (pdfix_context*)ctx;
    assert(pdfix_ctx->doc != nullptr);

    int hdr = pdfix_ctx->doc->GetVersion(); // https://pdfix.github.io/pdfix_sdk_builds/en/6.17.0/html/struct_pdf_doc.html#a2c758395b48f2c84ab7fcbdbd118f745
    std::string s = std::to_string(hdr / 10) + "." + std::to_string(hdr % 10);
    assert(FindInVector(v_ArlPDFVersions, s));
    return s;
}


/// @brief  Gets the number of pages in the PDF file
///
/// @returns   number of pages in the PDF or -1 on error
int ArlingtonPDFSDK::get_pdf_page_count() {
    assert(ctx != nullptr);
    auto pdfix_ctx = (pdfix_context*)ctx;

    if (pdfix_ctx->doc != nullptr)
        return pdfix_ctx->doc->GetNumPages();

    return -1; // error
}


PdsObject* pdfix_resolve_indirect(PdsObject* pdfix_obj) {
    assert(pdfix_obj != nullptr);
    int        obj_num;
    PdsObject* pdf_ir = pdfix_obj;

    do {
        assert(pdf_ir->GetObjectType() == kPdsReference);
        obj_num = pdf_ir->GetId();
        pdf_ir = ((pdfix_context*)ArlingtonPDFSDK::ctx)->doc->GetObjectById(obj_num);
    } while ((pdf_ir != nullptr) && (pdf_ir->GetObjectType() == kPdsReference));
    return pdf_ir;
}


/// @brief  Returns the PDF object type of an object
///
/// @param[in,out]  pdfix_obj   the pdfix indirect object which will get updated once it is resolved
///
/// @return PDFObjectType enum value for the resolved object
PDFObjectType determine_object_type(PdsObject* pdfix_obj)
{
    if (pdfix_obj == nullptr)
        return PDFObjectType::ArlPDFObjTypeNull;

    PDFObjectType retval;

    switch (pdfix_obj->GetObjectType())
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
            {
                // retval = PDFObjectType::ArlPDFObjTypeReference
                pdfix_obj = pdfix_resolve_indirect(pdfix_obj);
                if (pdfix_obj == nullptr)
                    retval = PDFObjectType::ArlPDFObjTypeNull;
                else
                    retval = determine_object_type(pdfix_obj);
            }
            break;
        default:
            retval = PDFObjectType::ArlPDFObjTypeUnknown;
            assert(false && "Bad PDFix object type!");
            break;
    }
    return retval;
}



/// @brief constructor
/// @param[in] parent    the parent object (so can get the object and generation numbers)
/// @param[in] obj       the object
ArlPDFObject::ArlPDFObject(ArlPDFObject *parent, void* obj, const bool can_delete) :
    object(obj), deleteable(can_delete)
{
    assert(object != nullptr);
    PdsObject* pdfix_obj = (PdsObject*)object;
    assert(pdfix_obj != nullptr);
    obj_nbr = pdfix_obj->GetId();
    gen_nbr = pdfix_obj->GetGenId(); 
    is_indirect = (obj_nbr != 0); // https://pdfix.github.io/pdfix_sdk_builds/en/6.17.0/html/struct_pds_object.html#a4103892417afc9f82e4bcc385940f4f8
    if (pdfix_obj->GetObjectType() == kPdsReference) {
        is_indirect = true;
        object = pdfix_resolve_indirect(pdfix_obj);
        assert(object != nullptr);
    }

    type = determine_object_type(pdfix_obj);

    if ((parent != nullptr) && (obj_nbr == 0)) {
        // Populate with parents object & generation number but as negative to indicate "direct inside parent"
        obj_nbr = -abs(parent->get_object_number());
        gen_nbr = -abs(parent->get_generation_number());
    }
}



/// @brief   generates unique identifier for every object
/// @return  for indirect objects it returns the unique identifier (object number)
std::string ArlPDFObject::get_hash_id()
{
  assert(object != nullptr);
  return std::to_string(obj_nbr) + "_" + std::to_string(gen_nbr);
}


/// @brief Checks if keys are already sorted and, if not, then sorts and caches
void ArlPDFObject::sort_keys()
{
    if (sorted_keys.empty()) {
        assert(((PdsObject*)object)->GetObjectType() == kPdsDictionary);
        PdsDictionary* obj = (PdsDictionary*)object;
        int numKeys = obj->GetNumKeys();
        // Get all the keys in the dictionary
        for (int i=0; i < numKeys; i++) {
            sorted_keys.push_back(obj->GetKey(i));
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
    assert(((PdsObject *)object)->GetObjectType() == kPdsBoolean);
    PdsBoolean* obj = (PdsBoolean *)object;
    bool retval = obj->GetValue();
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
    return retval;
}

/// @returns  Returns true if a PDF string object was a hex string
bool ArlPDFString::is_hex_string()
{
    assert(object != nullptr);
    assert(((PdsObject*)object)->GetObjectType() == kPdsString);
    PdsString* obj = (PdsString*)object;
    return false; /// @todo - how to know if hex string in PDFix??
}



/// @brief  Returns the name of a PDF name object as a string
/// @return The string representation of a PDF name object (can be zero length)
std::wstring ArlPDFName::get_value()
{
    assert(object != nullptr);
    assert(((PdsObject*)object)->GetObjectType() == kPdsName);
    PdsName* obj = (PdsName*)object;
    std::wstring retval = obj->GetText();
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
    return retval;
}


/// @brief  Returns the i-th array element from a PDF array object
/// @param idx the array index [0 ... n-1]
/// @return the object at array element index
ArlPDFObject* ArlPDFArray::get_value(const int idx)
{
    assert(object != nullptr);
    assert(idx >= 0);
    assert(((PdsObject*)object)->GetObjectType() == kPdsArray);
    PdsArray* obj = (PdsArray*)object;
    PdsObject* type_key = obj->Get(idx);
    ArlPDFObject* retval = nullptr;
    if (type_key != nullptr)
        retval = new ArlPDFObject(this, type_key);

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

    PdsObject* type_key = obj->Get(key.c_str());
    ArlPDFObject* retval = nullptr;
    if (type_key != nullptr)
        retval = new ArlPDFObject(this, type_key);

    return retval;
}


/// @brief Returns the key name of i-th dictionary key
/// @param index[in] dictionary key index
/// @return Key name
std::wstring ArlPDFDictionary::get_key_name_by_index(const int index)
{
    assert(object != nullptr);
    assert(index >= 0);
    assert(((PdsObject*)object)->GetObjectType() == kPdsDictionary);

    std::wstring retval;

    sort_keys();
    // Get the i-th sorted key name, allowing for no keys in a dictionary
    if ((!sorted_keys.empty()) && (index < (int)sorted_keys.size()))
        retval = sorted_keys[index];

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
    ArlPDFDictionary* retval = new ArlPDFDictionary(this, stm_dict);

    return retval;
}

#endif // ARL_PDFSDK_PDFIX
