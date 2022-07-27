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


/// @brief Initialize the PDF SDK. May throw exceptions.
void ArlingtonPDFSDK::initialize()
{
    assert(ctx == nullptr);

    // Assign to void context
    ctx = new QPDF();
}


/// @brief  Shutdown the PDF SDK
void ArlingtonPDFSDK::shutdown()
{
    if (ctx != nullptr)
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


/// @brief  Gets the PDF version of the current PDF file
/// 
/// @param trailer   trailer of the PDF
/// 
/// @returns   PDF version string 
std::string ArlingtonPDFSDK::get_pdf_version(ArlPDFTrailer* trailer) {
    assert(ctx != nullptr);
    assert(trailer != nullptr);

    QPDF* qpdfctx = (QPDF*)ctx;
    return qpdfctx->getPDFVersion();
}


/// @brief  Gets the number of pages in the PDF file
/// 
/// @param[in] trailer   trailer of the PDF
/// 
/// @returns   number of pages in the PDF or -1 on error
int ArlingtonPDFSDK::get_pdf_page_count(ArlPDFTrailer* trailer) {
    assert(ctx != nullptr);
    assert(trailer != nullptr);
    return 99999;
}




/// @brief Constructor
/// 
/// @param[in] obj   the object
ArlPDFObject::ArlPDFObject(void* obj) :
    object(obj), obj_nbr(-1), gen_nbr(-1) 
{
    is_indirect = false;
    if (object == nullptr)
        return;
    QPDFObjectHandle* pdf_obj = (QPDFObjectHandle*)object;
    is_indirect = pdf_obj->isIndirect();
}


/// @brief destructor
ArlPDFObject::~ArlPDFObject() {
    sorted_keys.clear();
}


/// @brief   generates unique identifier for every object
/// @return  for indirect objects it returns the unique identifier (object number)
std::string ArlPDFObject::get_hash_id()
{
    assert(object != nullptr);
    return std::to_string(((QPDFObjectHandle*)object)->getObjectID()) + "_" + std::to_string(((QPDFObjectHandle*)object)->getGeneration());
}


/// @brief  Returns the PDF object type of an object
/// @return PDFObjectType enum value
PDFObjectType ArlPDFObject::get_object_type()
{
    if (object == nullptr)
        return PDFObjectType::ArlPDFObjTypeNull;

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
    return retval;
}


/// @brief   Indicates if an object is an indirect reference
/// @return  true if an indirect reference. false otherwise (direct object)
bool ArlPDFObject::is_indirect_ref()
{
    assert(object != nullptr);
    bool retval = (((QPDFObjectHandle *)object)->isIndirect());
    return retval;
}


/// @brief  Returns the object number or 0 if a direct object
/// @return the object number or 0 if a direct object
int ArlPDFObject::get_object_number()
{
    assert(object != nullptr);
    int retval = ((QPDFObjectHandle *)object)->getObjectID();
    return retval;
}


/// @brief Checks if keys are already sorted and, if not, then sorts and caches
void ArlPDFObject::sort_keys()
{
    if (sorted_keys.empty()) {
        assert(((PdsObject*)object)->GetObjectType() == kPdsDictionary);
        PdsDictionary* obj = (PdsDictionary*)object;
        int numKeys = obj->GetNumKeys();
        // Get all the keys in the dictionary
        for (int i = 0; i < numKeys; i++) {
            sorted_keys.push_back(obj->GetKey(index));
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
ArlPDFObject* ArlPDFArray::get_value(int idx)
{
    assert(object != nullptr);
    assert(idx >= 0);
    QPDFObjectHandle *obj = (QPDFObjectHandle *)object;
    assert(obj->isArray());
    QPDFObjectHandle *elem = new QPDFObjectHandle;
    *elem = obj->getArrayItem(idx);
    ArlPDFObject *retval = new ArlPDFObject(elem);
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
        QPDFObjectHandle* keyobj = new QPDFObjectHandle;
        *keyobj = obj->getKey(s);
        if (keyobj->isInitialized())
            ArlPDFObject* retval = new ArlPDFObject(keyobj);
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
    // Get the i-th sorted key name, allowing for no keys in a dictionary 
    if ((!sorted_keys.empty()) && (index < sorted_keys.size()))
        retval = sorted_keys[index];

/*****
    std::wstring retval = L"";
    QPDFObjectHandle *obj = (QPDFObjectHandle *)object;
    assert(obj->isDictionary());
    std::map<std::string, QPDFObjectHandle>  dict = obj->getDictAsMap();
    if (index < dict.size()) {
        int i = 0;
        for (const auto& [ky, val] : dict)
            if (index == i++) {
                retval = ToWString(ky);
                break;
            }
    }
****/
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
