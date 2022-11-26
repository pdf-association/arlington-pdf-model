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
#include <cassert>
#include "utils.h"

// pdfium
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
    FX_DWORD            open_err_code;

    ArlPDFTrailer*      pdf_trailer;
    ArlPDFDictionary*   pdf_catalog;

    pdfium_context() {
        /* Default constructor */
        open_err_code = PDFPARSE_ERROR_SUCCESS;
        pdf_trailer = nullptr;
        pdf_catalog = nullptr;
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
void ArlingtonPDFSDK::initialize()
{
    assert(ctx == nullptr);
    auto pdfium_ctx = new pdfium_context;
    ctx = pdfium_ctx;
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


/// @brief   Opens a PDF file (optional password) 
///
/// @param[in]   pdf_filename PDF filename
/// @param[in]   password     optional password
///
/// @returns  true if PDF file was opened successfully. false othewise.
bool ArlingtonPDFSDK::open_pdf(const std::filesystem::path& pdf_filename, const std::wstring &password)
{
    assert(ctx != nullptr);
    assert(!pdf_filename.empty());
    auto pdfium_ctx = (pdfium_context*)ctx;

    // close any previously opened document
    if (pdfium_ctx->parser != nullptr) {
        pdfium_ctx->parser->CloseParser();
        delete pdfium_ctx->parser;
    }
    pdfium_ctx->parser = new CPDF_Parser;

    if (password.size() > 0)
        pdfium_ctx->parser->SetPassword(ToUtf8(password).c_str());

    pdfium_ctx->open_err_code = pdfium_ctx->parser->StartParse((FX_LPCSTR)pdf_filename.string().c_str());
    if ((pdfium_ctx->open_err_code != PDFPARSE_ERROR_SUCCESS) && (pdfium_ctx->open_err_code != PDFPARSE_ERROR_PASSWORD) && (pdfium_ctx->open_err_code != PDFPARSE_ERROR_HANDLER)) {
        delete pdfium_ctx->parser;
        pdfium_ctx->parser = nullptr;
        return false;
    }

    // make master trailer and document catalog dictionaries
    CPDF_Dictionary* trailr = pdfium_ctx->parser->GetTrailer();
    assert(trailr != NULL);
    if (trailr != NULL) {
        assert(trailr->GetType() == PDFOBJ_DICTIONARY);
        pdfium_ctx->pdf_trailer = new ArlPDFTrailer(trailr,
                                        pdfium_ctx->parser->IsXRefStream(),
                                        pdfium_ctx->parser->IsEncrypted(),
                                        (pdfium_ctx->open_err_code == PDFPARSE_ERROR_PASSWORD) || (pdfium_ctx->open_err_code == PDFPARSE_ERROR_HANDLER)
                                    );

        /// @todo pdfium has an array of other trailers but this is not set up when a rebuild is done. ???
        /// May be useful for testing if all trailers have the correct keys ???
        /******************
        CFX_ArrayTemplate<CPDF_Dictionary*>* other_trailers = pdfium_ctx->parser->GetOtherTrailers();
        std::cout << "Number of other trailers = " << other_trailers->GetSize() << std::endl;
        for (int i = 0; i < other_trailers->GetSize(); i++) {
            CPDF_Dictionary* t = other_trailers->GetAt(i);
            FX_POSITION pos = t->GetStartPos();
            int j = 0;
            while (pos) {
                CFX_ByteString key;
                CPDF_Object* o = t->GetNextElement(pos, key);
                std::cout << i << "[" << j++ << "] = " << std::string(o->GetString()) << std::endl;
            }
        }
        *******************/

        auto dc_dict = trailr->GetDict("Root");
        if (dc_dict != nullptr) {
            pdfium_ctx->pdf_catalog = new ArlPDFDictionary(pdfium_ctx->pdf_trailer, dc_dict, false);
            return true;
        }
    }

    return false;
}



/// @brief Close a previously opened PDF file. Frees all memory for a file so multiple PDFs don't accumulate leaked memory.
void ArlingtonPDFSDK::close_pdf() {
    assert(ctx != nullptr);
    auto pdfium_ctx = (pdfium_context*)ctx;

    pdfium_ctx->pdf_catalog->force_deleteable();
    delete pdfium_ctx->pdf_catalog;
    pdfium_ctx->pdf_catalog = nullptr;
    
    pdfium_ctx->pdf_trailer->force_deleteable();
    delete pdfium_ctx->pdf_trailer;
    pdfium_ctx->pdf_trailer = nullptr;

    if (pdfium_ctx->parser != nullptr) {
        pdfium_ctx->parser->CloseParser();
        delete pdfium_ctx->parser;
        pdfium_ctx->parser = nullptr;
    }
}



/// @brief   Returns the trailer dictionary-like object for an already opened PDF
///
/// @returns  handle to PDF trailer dictionary. nullptr on error.
ArlPDFTrailer* ArlingtonPDFSDK::get_trailer()
{
    assert(ctx != nullptr);
    auto pdfium_ctx = (pdfium_context*)ctx;
    assert(pdfium_ctx->pdf_trailer != nullptr);
    auto b = pdfium_ctx->parser->IsXRefStream();
    return pdfium_ctx->pdf_trailer;
}



/// @brief   Returns the document catalog for an already opened PDF
///
/// @returns  handle to document catalog. nullptr on error.
ArlPDFDictionary* ArlingtonPDFSDK::get_document_catalog()
{
    assert(ctx != nullptr);
    auto pdfium_ctx = (pdfium_context*)ctx;
    assert(pdfium_ctx->pdf_catalog != nullptr);
    return pdfium_ctx->pdf_catalog;
}




/// @brief  Gets the PDF version of the current PDF file as a string of length 3.
/// Note that for corrupted and invalid PDFs, this can be an out-of-range value!
/// e.g verapdf\corpus\veraPDF-corpus-staging\PDF_A-1b\6.1 File structure\6.1.2 File header\veraPDF test suite 6-1-2-t01-fail-b.pdf
///
/// @returns   PDF version string (always length 3)
std::string ArlingtonPDFSDK::get_pdf_version() {
    assert(ctx != nullptr);

    auto pdfium_ctx = (pdfium_context*)ctx;
    assert(pdfium_ctx->parser != nullptr);
    int ver = pdfium_ctx->parser->GetFileVersion();
    // ver = PDF header version x 10 (so PDF 1.3 = 13)
    char version_str[6];
    snprintf(version_str, 4, "%1.1f", ver / 10.0);
    return version_str;
}


/// @brief  Gets the PDF version of the current PDF file as an integer * 10
/// Note that for corrupted and invalid PDFs, this can be an out-of-range value!
/// e.g verapdf\corpus\veraPDF-corpus-staging\PDF_A-1b\6.1 File structure\6.1.2 File header\veraPDF test suite 6-1-2-t01-fail-b.pdf
///
/// @returns   PDF version multiplied by 10
int ArlingtonPDFSDK::get_pdf_version_number() {
    assert(ctx != nullptr);

    auto pdfium_ctx = (pdfium_context*)ctx;
    assert(pdfium_ctx->parser != nullptr);
    return pdfium_ctx->parser->GetFileVersion();
}


/// @brief  Gets the number of pages in the PDF file
///
/// @returns   number of pages in the PDF or -1 on error
int ArlingtonPDFSDK::get_pdf_page_count() {
    assert(ctx != nullptr);

    auto pdfium_ctx = (pdfium_context*)ctx;
    assert(pdfium_ctx->parser != nullptr);
    return pdfium_ctx->parser->GetDocument()->GetPageCount();
}


CPDF_Object* pdfium_resolve_indirect(const CPDF_Object* pdfium_obj) {
    assert(pdfium_obj != nullptr);
    FX_DWORD     obj_num;
    CPDF_Object* pdf_ir;
    int          i = 20;    // Maxiumum number of indirections via IRs allowed

    do {
        assert(pdfium_obj->GetType() == PDFOBJ_REFERENCE);
        obj_num = ((CPDF_Reference*)pdfium_obj)->GetRefObjNum();
        pdf_ir = ((pdfium_context*)ArlingtonPDFSDK::ctx)->parser->GetDocument()->GetIndirectObject(obj_num);
    } while ((pdf_ir != nullptr) && (pdf_ir->GetType() == PDFOBJ_REFERENCE) && (--i > 0));
    if (i > 0)
        return pdf_ir;
    else
        return nullptr; // too many indirections - may get converted to a CPDF_Null
}


/// @brief  Returns the PDF object type of an object
///
/// @param[in,out]  pdfium_obj   the pdfium indirect object which will get updated once it is resolved
///
/// @return PDFObjectType enum value for the resolved object
PDFObjectType determine_object_type(CPDF_Object* pdfium_obj)
{
    if (pdfium_obj == nullptr)
        return PDFObjectType::ArlPDFObjTypeNull;

    PDFObjectType retval;

    switch (pdfium_obj->GetType())
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
            pdfium_obj = pdfium_resolve_indirect(pdfium_obj);
            if (pdfium_obj == nullptr)
                retval = PDFObjectType::ArlPDFObjTypeNull;
            else
                retval = determine_object_type(pdfium_obj);
        }
        break;
    case PDFOBJ_INVALID: /* fallthrough */
    default:
        retval = PDFObjectType::ArlPDFObjTypeUnknown;
        assert(false && "Bad pdfium object type!");
        break;
    }
    return retval;
}


/// @brief Constructor taking a parent PDF object and a PDF SDK generic pointer of an object
ArlPDFObject::ArlPDFObject(ArlPDFObject *parent, void* obj, const bool can_delete) :
    object(obj), deleteable(can_delete)
{
    assert(object != nullptr);
    CPDF_Object* pdf_obj = (CPDF_Object*)obj;
    int obj_type = pdf_obj->GetType();
    assert(obj_type != PDFOBJ_INVALID);
    is_indirect = (obj_type == PDFOBJ_REFERENCE);

    // Resolve the indirect reference to a terminating object
    if (is_indirect)
        pdf_obj = pdfium_resolve_indirect(pdf_obj);

    // Object can be invalid (e.g. no valid object in PDF file or infinite loop of indirect references) 
    // so substitute a null object as constructors cannot return nullptr
    if (pdf_obj == nullptr) 
        pdf_obj = new CPDF_Null; /// @todo will leak 12 bytes as no distinguishig between explicit null in PDF and this error situation

    // Proceed to populate class data
    type = determine_object_type(pdf_obj);
    obj_nbr = pdf_obj->GetObjNum();
    gen_nbr = pdf_obj->GetGenNum();
    if ((parent != nullptr) && (obj_nbr == 0)) {
        // Populate with parents object & generation number but as negative to indicate parent. NOT for trailer as it is parentless!
        obj_nbr = parent->get_object_number();
        if (obj_nbr > 0) obj_nbr *= -1;
        gen_nbr = parent->get_generation_number();
        if (gen_nbr > 0) gen_nbr *= -1;
    }
    object = pdf_obj;
}



/// @brief   generates unique identifier for every object
/// @return  for indirect objects it returns the unique identifier (object number)
std::string ArlPDFObject::get_hash_id()
{
    assert(object != nullptr);
    if (((CPDF_Object*)object)->GetType() != PDFOBJ_REFERENCE) {
        return std::to_string(obj_nbr) + "_" + std::to_string(gen_nbr);
    }
    else {
        CPDF_Reference* r = (CPDF_Reference*)object;
        return std::to_string(r->GetRefObjNum()) + "_" + std::to_string(r->GetGenNum());
    }
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
    return retval;
}


/// @brief  Returns the bytes of a PDF string object
/// @returns The bytes of a PDF string object (can be zero length)
std::wstring ArlPDFString::get_value()
{
    assert(object != nullptr);
    assert(((CPDF_Object*)object)->GetType() == PDFOBJ_STRING);
    
    std::wstring retval;
    CPDF_String* obj = ((CPDF_String*)object);
    CFX_ByteString bs = obj->GetString();
    for (auto i = 0; i < bs.GetLength(); i++) {
        wchar_t b = bs.GetAt(i);
        retval = retval + b;
    }

#ifdef MARK_STRINGS_WHEN_ENCRYPTED
    // Make error messages slightly more understandable in the case of unsupported encryption
    // Note that this will then break any predicate checks for the always-unencrypted strings described in clause 7.6.2 
    assert(ArlingtonPDFSDK::ctx != nullptr);
    if (((pdfium_context*)ArlingtonPDFSDK::ctx)->unsupported_encryption)
        retval = UNSUPPORTED_ENCRYPTED_STRING_MARKER;
#endif // MARK_STRINGS_WHEN_ENCRYPTED

    return retval;
}


/// @returns  Returns true if a PDF string object was a hex string
bool ArlPDFString::is_hex_string()
{
    assert(object != nullptr);
    assert(((CPDF_Object*)object)->GetType() == PDFOBJ_STRING);

    CPDF_String* obj = ((CPDF_String*)object);
    return (obj->IsHex() != 0);
}


/// @brief  Returns the name of a PDF name object as a string
/// @return The string representation of a PDF name object (can be zero length)
std::wstring ArlPDFName::get_value()
{
    assert(object != nullptr);
    assert(((CPDF_Object*)object)->GetType() == PDFOBJ_NAME);
    CPDF_Name* obj = ((CPDF_Name*)object);
    std::wstring retval = (FX_LPCWSTR)obj->GetString().UTF8Decode();
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
    return retval;
}


/// @brief  Returns the i-th array element from a PDF array object
/// @param idx the array index [0 ... n-1]
/// @return the object at array element index
ArlPDFObject* ArlPDFArray::get_value(const int idx)
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
        retval = new ArlPDFObject(this, type_key);
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
    CPDF_Dictionary* dict = ((CPDF_Dictionary*)object);

    CFX_ByteString bstr = CFX_ByteString::FromUnicode(key.c_str());
    CPDF_Object* key_value = dict->GetElement(bstr);
    if (key_value != NULL) {
        int t = key_value->GetType();
        assert(t != PDFOBJ_INVALID);
        retval = new ArlPDFObject(this, key_value);
    }
    return retval;
}


/// @brief Returns the key name of i-th dictionary key. Keys need to be
/// alphabetically sorted so that output order matches other PDF SDKs (PDFix)
///
/// @param[in] index dictionary key index
///
/// @returns Key name
std::wstring ArlPDFDictionary::get_key_name_by_index(const int index)
{
    assert(object != nullptr);
    assert(index >= 0);
    std::wstring     retval;

    sort_keys();
    // Get the i-th sorted key name, allowing for no keys in a dictionary
    if ((!sorted_keys.empty()) && (index < (int)sorted_keys.size()))
        retval = sorted_keys[index];

    return retval;
}


/// @brief Returns true if the dictionary has one or more duplicate keys.
/// Note that pdfium has been modified to report this capability!!
/// @return true if the dictionary has one or more duplicate keys
bool ArlPDFDictionary::has_duplicate_keys()
{
    assert(object != nullptr);
    assert(((CPDF_Object*)object)->GetType() == PDFOBJ_DICTIONARY);
    CPDF_Dictionary* dict = ((CPDF_Dictionary*)object);
    return dict->HasDuplicateKeys();
}


/// @brief Returns the list of duplicate keys in the dictionary.
/// Note that pdfium has been modified to report this capability!!
/// @return List of duplicate keys in the dictionary
std::vector<std::string>& ArlPDFDictionary::get_duplicate_keys()
{
    assert(object != nullptr);
    assert(((CPDF_Object*)object)->GetType() == PDFOBJ_DICTIONARY);
    CPDF_Dictionary* dict = ((CPDF_Dictionary*)object);
    return dict->GetDuplicateKeys();
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
    ArlPDFDictionary* retval = new ArlPDFDictionary(this, stm_dict);
    return retval;
}

#endif // ARL_PDFSDK_PDFIUM
