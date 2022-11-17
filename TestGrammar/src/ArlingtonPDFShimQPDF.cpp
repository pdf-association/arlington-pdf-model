///////////////////////////////////////////////////////////////////////////////
/// @file 
/// @brief  Arlington QPDF SDK shim layer
///
/// A wafer-thin shim layer to isolate the QPDF SDK library from the rest of the
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
/// @author Peter Wyatt, PDF Association
///
///////////////////////////////////////////////////////////////////////////////

#include "ArlingtonPDFShim.h"
#ifdef ARL_PDFSDK_QPDF

#include <string>
#include <cassert>
#include <algorithm>
#include "utils.h"

/// @brief QPDF uses some C++17 deprecated features so try silence warnings
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING

#pragma warning(push)
#pragma warning(disable:4996)
#include "qpdf/QPDF.hh"
#pragma warning(pop)

using namespace ArlingtonPDFShim;

void* ArlingtonPDFSDK::ctx = nullptr;


struct qpdf_context {
    QPDF*               qpdf_ctx  = nullptr;
    ArlPDFTrailer*      pdf_trailer = nullptr;
    ArlPDFDictionary*   pdf_catalog = nullptr;

    ~qpdf_context() {
    }
};


/// @brief Initialize the PDF SDK. May throw exceptions.
void ArlingtonPDFSDK::initialize()
{
    assert(ctx == nullptr);

    // Assign to void context
    qpdf_context* qctx = new qpdf_context;
    qctx->qpdf_ctx = new QPDF();
    ctx = qctx;
}


/// @brief  Shutdown the PDF SDK
void ArlingtonPDFSDK::shutdown()
{
    qpdf_context* qctx = (qpdf_context*)ctx;
    if (qctx->qpdf_ctx != nullptr)
        delete qctx->qpdf_ctx;
    if (qctx != nullptr)
        delete qctx;
    ctx = nullptr;
}


/// @brief  Returns human readable version string for PDF SDK that is being used
/// @return version string
std::string ArlingtonPDFSDK::get_version_string()
{
    return "QPDF " QPDF_VERSION;
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
    qpdf_context* qctx = (qpdf_context*)ctx;
    assert(qctx->qpdf_ctx != nullptr);
    assert(!pdf_filename.empty());

    if (password.size() > 0)
        qctx->qpdf_ctx->processFile(pdf_filename.string().c_str(), ToUtf8(password).c_str());
    else
        qctx->qpdf_ctx->processFile(pdf_filename.string().c_str());

    auto t = qctx->qpdf_ctx->getTrailer();
    QPDFObjectHandle* trailer = &t;

    if (trailer->isDictionary()) {
        qctx->pdf_trailer = new ArlPDFTrailer(trailer, 
                                            trailer->hasKey("/Type"),
                                            qctx->qpdf_ctx->isEncrypted(), 
                                            false
                                     );
        auto r = qctx->qpdf_ctx->getRoot();
        qctx->pdf_catalog = new ArlPDFDictionary(qctx->pdf_trailer, &r, false);
        return true;
    }
    return false;
}



/// @brief Close a previously opened PDF file. Frees all memory for a file so multiple PDFs don't accumulate leaked memory.
void ArlingtonPDFSDK::close_pdf() {
    assert(ctx != nullptr);
    auto qpdf_ctx = (qpdf_context*)ctx;

    delete qpdf_ctx->pdf_catalog;
    qpdf_ctx->pdf_catalog = nullptr;

    delete qpdf_ctx->pdf_trailer;
    qpdf_ctx->pdf_trailer = nullptr;
}



/// @brief  Gets the PDF trailer dictionary-like object
/// 
/// @returns   PDF trailer dictionary
ArlPDFTrailer* ArlingtonPDFSDK::get_trailer() {
    assert(ctx != nullptr);
    qpdf_context* qctx = (qpdf_context*)ctx;
    assert(qctx->qpdf_ctx != nullptr);
    return qctx->pdf_trailer;
}



/// @brief  Gets the PDF document catalog 
/// 
/// @returns   PDF trailer dictionary
ArlPDFDictionary* ArlingtonPDFSDK::get_document_catalog() {
    assert(ctx != nullptr);
    qpdf_context* qctx = (qpdf_context*)ctx;
    assert(qctx->qpdf_ctx != nullptr);
    return qctx->pdf_catalog;
}



/// @brief  Gets the PDF version of the current PDF file as a string of length 3
/// 
/// @returns   PDF version string (always length 3)
std::string ArlingtonPDFSDK::get_pdf_version() {
    assert(ctx != nullptr);
    qpdf_context* qctx = (qpdf_context*)ctx;
    assert(qctx->qpdf_ctx != nullptr);
    auto v = qctx->qpdf_ctx->getPDFVersion();
    return v;
}


/// @brief  Gets the PDF version of the current PDF file as an integer * 10
/// 
/// @returns   PDF version multiplied by 10
int ArlingtonPDFSDK::get_pdf_version_number() {
    assert(ctx != nullptr);
    qpdf_context* qctx = (qpdf_context*)ctx;
    assert(qctx->qpdf_ctx != nullptr);
    auto v = qctx->qpdf_ctx->getVersionAsPDFVersion();
    return (v.getMajor() * 10) + v.getMinor();
}



/// @brief  Gets the number of pages in the PDF file
/// 
/// @param[in] trailer   trailer of the PDF
/// 
/// @returns   number of pages in the PDF or -1 on error
int ArlingtonPDFSDK::get_pdf_page_count() {
    assert(ctx != nullptr);
    qpdf_context* qctx = (qpdf_context*)ctx;
    assert(qctx->qpdf_ctx != nullptr);
    return 99999; /// @todo - QPDF page count
}



/// @brief   generates unique identifier for every object
/// @return  for indirect objects it returns the unique identifier (object number)
std::string ArlPDFObject::get_hash_id()
{
    assert(object != nullptr);
    return std::to_string(((QPDFObjectHandle*)object)->getObjectID()) + "_" + std::to_string(((QPDFObjectHandle*)object)->getGeneration());
}


/// @brief  Returns the PDF object type of a QPDF object
/// @return PDFObjectType enum value
PDFObjectType determine_object_type(QPDFObjectHandle *obj)
{
    if (obj == nullptr)
        return PDFObjectType::ArlPDFObjTypeNull;

    PDFObjectType retval;

    switch (obj->getTypeCode())
    {
        case qpdf_object_type_e::ot_boolean:
            retval = PDFObjectType::ArlPDFObjTypeBoolean;
            break;
        case qpdf_object_type_e::ot_integer:
        case qpdf_object_type_e::ot_real:
            retval = PDFObjectType::ArlPDFObjTypeNumber;    // Integer or Real (or bitmask)
            break;
        case qpdf_object_type_e::ot_string:
            retval = PDFObjectType::ArlPDFObjTypeString;    // Any type of string
            break;
        case qpdf_object_type_e::ot_name:
            retval = PDFObjectType::ArlPDFObjTypeName;
            break;
        case qpdf_object_type_e::ot_array:
            retval = PDFObjectType::ArlPDFObjTypeArray;     // incl. rectangle or matrix
            break;
        case qpdf_object_type_e::ot_dictionary:
            retval = PDFObjectType::ArlPDFObjTypeDictionary;
            break;
        case qpdf_object_type_e::ot_stream:
            retval = PDFObjectType::ArlPDFObjTypeStream;
            break;
        case qpdf_object_type_e::ot_null:
            retval = PDFObjectType::ArlPDFObjTypeNull;
            break;
        default:
            retval = PDFObjectType::ArlPDFObjTypeUnknown;
			assert(false && "bad QPDF object type!");
            break;
    }
    return retval;
}



/// @brief Constructor taking a parent PDF object and a PDF SDK generic pointer of an object
ArlPDFObject::ArlPDFObject(ArlPDFObject *parent, void* obj, const bool can_delete) :
    object(obj), deleteable(can_delete)
{
    assert(object != nullptr);
    QPDFObjectHandle* pdf_obj = (QPDFObjectHandle*)obj;
    auto obj_type = pdf_obj->getTypeCode();
    assert((obj_type != qpdf_object_type_e::ot_uninitialized) && (obj_type != qpdf_object_type_e::ot_reserved));
    is_indirect = pdf_obj->isIndirect();

    /// @todo Resolve the indirect reference to a terminating object
    //if (is_indirect)
    //    pdf_obj = qpdf_resolve_indirect(pdf_obj);

    // Object can be invalid (e.g. no valid object in PDF file or infinite loop of indirect references) 
    /// so substitute a null object as constructors cannot return nullptr
    if (pdf_obj == nullptr) {
        auto n = QPDFObjectHandle::newNull();
        pdf_obj = &n;
    }

    // Proceed to populate class data
    type = determine_object_type(pdf_obj);
    obj_nbr = pdf_obj->getObjectID();
    gen_nbr = pdf_obj->getGeneration();
    if ((parent != nullptr) && (obj_nbr == 0)) {
        // Populate with parents object & generation number but as negative to indicate parent
        obj_nbr = parent->get_object_number();
        if (obj_nbr > 0) obj_nbr *= -1;
        gen_nbr = parent->get_generation_number();
        if (gen_nbr > 0) gen_nbr *= -1;
    }
    object = pdf_obj;
}



/// @brief Checks if keys are already sorted and, if not, then sorts and caches
void ArlPDFObject::sort_keys()
{
    if (sorted_keys.empty()) {
        assert(((QPDFObjectHandle*)object)->isDictionary());
        auto dict = ((QPDFObjectHandle*)object)->getDict();

        // Get all the keys in the dictionary
        for (auto& k : dict.getKeys()) {
            sorted_keys.push_back(ToWString(k));
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
    QPDFObjectHandle *obj = (QPDFObjectHandle *)object;
    assert(obj->isBool());
    bool retval = obj->getBoolValue();
    return retval;
}


/// @brief  Returns true if a PDF numeric object is an integer
/// @return Returns true if an integer value, false if real value
bool ArlPDFNumber::is_integer_value()
{
    assert(object != nullptr);
    QPDFObjectHandle *obj = (QPDFObjectHandle *)object;
    bool retval = obj->isInteger();
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
    return retval;
}


/// @brief  Returns the bytes of a PDF string object
/// @return The bytes of a PDF string object (can be zero length)
std::wstring ArlPDFString::get_value()
{
    assert(object != nullptr);
    QPDFObjectHandle *obj = (QPDFObjectHandle *)object;
    assert(obj->isString());
    std::wstring retval = ToWString(obj->getStringValue());
    return retval;
}


/// @returns  Returns true if a PDF string object was a hex string
bool ArlPDFString::is_hex_string()
{
    assert(object != nullptr);
    QPDFObjectHandle* obj = (QPDFObjectHandle*)object;
    assert(obj->isString());
    return false; /// @todo - how to know if hex string in QPDF??
}


/// @brief  Returns the name of a PDF name object as a string
/// @return The string representation of a PDF name object (can be zero length)
std::wstring ArlPDFName::get_value()
{
    assert(object != nullptr);
    QPDFObjectHandle *obj = (QPDFObjectHandle *)object;
    assert(obj->isString());
    std::wstring retval = ToWString(obj->getName());
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
    return retval;
}


/// @brief  Returns the i-th array element from a PDF array object
/// @param  idx[in] the array index [0 ... n-1]
/// @return the object at array element index
ArlPDFObject* ArlPDFArray::get_value(const int idx)
{
    assert(object != nullptr);
    assert(idx >= 0);
    QPDFObjectHandle *obj = (QPDFObjectHandle *)object;
    assert(obj->isArray());
    auto e = obj->getArrayItem(idx);
    QPDFObjectHandle* elem = &e;
    ArlPDFObject *retval = new ArlPDFObject(this, elem);
    return retval;
}


/// @brief Returns the number of keys in a PDF dictionary
/// @return Number of keys (>= 0)
int ArlPDFDictionary::get_num_keys()
{
    assert(object != nullptr);
    QPDFObjectHandle *obj = (QPDFObjectHandle *)object;
    assert(obj->isDictionary());
    auto  dict = obj->getDictAsMap();
    int retval = (int)dict.size();
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
    std::string s = ToUtf8(key); 
    bool retval = obj->hasKey(s);
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
    ArlPDFObject* retval = nullptr;
    std::string s = ToUtf8(key);
    if (obj->hasKey(s)) {
        auto o = obj->getKey(s); 
        QPDFObjectHandle* keyobj = &o;
        if (keyobj->isInitialized())
            retval = new ArlPDFObject(this, keyobj);
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

    sort_keys();
    std::wstring retval = L"";
    // Get the i-th sorted key name, allowing for no keys in a dictionary 
    if ((sorted_keys.size() > 0) && (index < sorted_keys.size()))
        retval = sorted_keys[index];

    QPDFObjectHandle *obj = (QPDFObjectHandle *)object;
    assert(obj->isDictionary());
    auto dict = obj->getDictAsMap();
    if (index < dict.size()) {
        int i = 0;
        for (const auto& [ky, val] : dict)
            if (index == i++) {
                retval = ToWString(ky);
                break;
            }
    }
    return retval;
}



ArlPDFDictionary* ArlPDFStream::get_dictionary() {
    assert(object != nullptr);
    QPDFObjectHandle* obj = (QPDFObjectHandle*)object;
    assert(obj->isStream());
    ArlPDFDictionary* retval = (ArlPDFDictionary *)obj;  /// @todo is this correct????
    return retval;
}



#endif // ARL_PDFSDK_QPDF
