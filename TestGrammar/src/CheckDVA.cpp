///////////////////////////////////////////////////////////////////////////////
// CheckDVA.cpp
// Copyright 2020 PDF Association, Inc. https://www.pdfa.org
//
// This material is based upon work supported by the Defense Advanced
// Research Projects Agency (DARPA) under Contract No. HR001119C0079.
// Any opinions, findings and conclusions or recommendations expressed
// in this material are those of the author(s) and do not necessarily
// reflect the views of the Defense Advanced Research Projects Agency
// (DARPA). Approved for public release.
//
// SPDX-License-Identifier: Apache-2.0
// Contributors: Roman Toda, Frantisek Forgac, Normex. Peter Wyatt, PDF Association
//
///////////////////////////////////////////////////////////////////////////////

#include "ArlingtonPDFShim.h"

/// @file
/// Compares an Arlington PDF model to the Adobe DVA FormalRep as defined in a PDF

#include <exception>
#include <queue>
#include <map>
#include <string>

#include "CheckGrammar.h"
#include "ArlingtonTSVGrammarFile.h"
#include "TestGrammarVers.h"
#include "utils.h"

using namespace ArlingtonPDFShim;
namespace fs = std::filesystem;

/// @brief simulating recursive processing of the PDF Objects
struct to_process_elem {
    std::wstring  dva_link;
    std::wstring  dva_link2;
    std::string   link;
    to_process_elem(const std::wstring dva_lnk, std::string our_lnk)
        : dva_link(dva_lnk), dva_link2(L""), link(our_lnk)
        { /* constructor */ }
    to_process_elem(const std::wstring dva_lnk1, const std::wstring dva_lnk2, std::string our_lnk)
        : dva_link(dva_lnk1), dva_link2(dva_lnk2), link(our_lnk)
        { /* constructor */ }
};


/// @brief 
std::queue<to_process_elem> to_process_checks;


/// @brief  Process a single dictionary definition
/// 
/// @param[in] tsv_dir Arlington TSV directory
/// @param[in,out] ofs     already open output stream for report messages
/// @param[in] map 
void process_dict(const fs::path &tsv_dir, std::ostream& ofs, ArlPDFDictionary* map) {
    ArlPDFDictionary* map_dict = map;

    int count = 0;
    std::map<std::string, std::wstring>   mapped;

    to_process_checks.emplace(L"Catalog", "Catalog");

    to_process_checks.emplace(L"Font", L"FontType1", "FontType1");
    to_process_checks.emplace(L"Font", L"FontTrueType", "FontTrueType");
    to_process_checks.emplace(L"Font", L"FontMMType1", "FontMultipleMaster");
    to_process_checks.emplace(L"Font", L"FontType3", "FontType3");
    to_process_checks.emplace(L"Font", L"FontType0", "FontType0");
    to_process_checks.emplace(L"Font", L"FontCIDFontType0or2", "FontCIDType0");
    to_process_checks.emplace(L"Font", L"FontCIDFontType0or2", "FontCIDType2");

    //OPIDict
    to_process_checks.emplace(L"OPI1.3", "OPIVersion13");
    to_process_checks.emplace(L"OPI2.0", "OPIVersion20");

    to_process_checks.emplace(L"PagesOrPage",L"Pages", "PageTreeNode");
    to_process_checks.emplace(L"PagesOrPage",L"Page","PageObject");

    //Bead----BeadFirst,Bead
    to_process_checks.emplace(L"Bead_First", "BeadFirst");
    to_process_checks.emplace(L"Bead", "Bead");

    //OCGorOCMD----,
    to_process_checks.emplace(L"OCGorOCMD",L"OCG", "OptContentGroup");
    to_process_checks.emplace(L"OCGorOCMD",L"OCMD", "OptContentMembership");

    //Outline----OutlineItem,Outline
    to_process_checks.emplace(L"Outline", "OutlineItem");
    to_process_checks.emplace(L"Outlines", "Outline");

    //Pattern----PatternType1,PatternType2
    to_process_checks.emplace(L"Pattern",L"PatternType1", "PatternType1");
    to_process_checks.emplace(L"Pattern",L"PatternType2", "PatternType2");

    //XObject----XObjectFormType1,XObjectImage
    to_process_checks.emplace(L"XObject",L"XObjectForm", "XObjectFormType1");
    to_process_checks.emplace(L"XObject",L"XObjectImageBase", "XObjectImage");

    //Rendition----RenditionMedia,RenditionSelector
    to_process_checks.emplace(L"Rendition",L"MediaRendition", "RenditionMedia");
    to_process_checks.emplace(L"Rendition",L"SelectorRendition", "RenditionSelector");

    //SigRef----SignatureReferenceDocMDP,SignatureReferenceFieldMDP,SignatureReferenceIdentity,SignatureReferenceUR
    to_process_checks.emplace(L"SigRef",L"SigRefDocMDP", "SignatureReferenceDocMDP");
    to_process_checks.emplace(L"SigRef",L"SigRefFieldMDP", "SignatureReferenceFieldMDP");
    to_process_checks.emplace(L"SigRef",L"SigRefIdentity", "SignatureReferenceIdentity");
    to_process_checks.emplace(L"SigRef",L"SigRefUR", "SignatureReferenceUR");

    //Action----ActionGoTo,ActionGoToR,ActionGoToE,ActionGoToDp,ActionLaunch,ActionThread,ActionURI,ActionSound,ActionMovie,ActionHide,
    //ActionNamed,ActionSubmitForm,ActionResetForm,ActionImportData,ActionSetOCGState,ActionRendition,ActionTransition,ActionGoTo3DView,
    //ActionECMAScript,ActionRichMediaExecute
    to_process_checks.emplace(L"Action",L"ActionGoTo", "ActionGoTo");
    to_process_checks.emplace(L"Action",L"ActionGoToR", "ActionGoToR");
    to_process_checks.emplace(L"Action",L"ActionGoToE", "ActionGoToE");
    to_process_checks.emplace(L"Action",L"ActionLaunch", "ActionLaunch");
    to_process_checks.emplace(L"Action",L"ActionThread", "ActionThread");
    to_process_checks.emplace(L"Action",L"ActionURI", "ActionURI");
    to_process_checks.emplace(L"Action",L"ActionSound", "ActionSound");
    to_process_checks.emplace(L"Action",L"ActionMovie", "ActionMovie");
    to_process_checks.emplace(L"Action",L"ActionHide", "ActionHide");
    to_process_checks.emplace(L"Action",L"ActionNamed", "ActionNamed");
    to_process_checks.emplace(L"Action",L"ActionSubmitForm", "ActionSubmitForm");
    to_process_checks.emplace(L"Action",L"ActionResetForm", "ActionResetForm");
    to_process_checks.emplace(L"Action",L"ActionImportData", "ActionImportData");
    to_process_checks.emplace(L"Action",L"ActionSetOCGState", "ActionSetOCGState");
    to_process_checks.emplace(L"Action",L"ActionRendition", "ActionRendition");
    to_process_checks.emplace(L"Action",L"ActionTrans", "ActionTransition");
    to_process_checks.emplace(L"Action",L"ActionGoTo3DView", "ActionGoTo3DView");
    to_process_checks.emplace(L"Action",L"ActionJavaScript", "ActionECMAScript");

    to_process_checks.emplace(L"Annot", L"AnnotText", "AnnotText");
    to_process_checks.emplace(L"Annot", L"AnnotLink", "AnnotLink");
    to_process_checks.emplace(L"Annot", L"AnnotFreeText", "AnnotFreeText");
    to_process_checks.emplace(L"Annot", L"AnnotLine", "AnnotLine");
    to_process_checks.emplace(L"Annot", L"AnnotSquare", "AnnotSquare");
    to_process_checks.emplace(L"Annot", L"AnnotCircle", "AnnotCircle");
    to_process_checks.emplace(L"Annot", L"AnnotPolygon", "AnnotPolygon");
    to_process_checks.emplace(L"Annot", L"AnnotPolyLine", "AnnotPolyLine");
    to_process_checks.emplace(L"Annot", L"AnnotHighlight", "AnnotHighlight");
    to_process_checks.emplace(L"Annot", L"AnnotUnderline", "AnnotUnderline");
    to_process_checks.emplace(L"Annot", L"AnnotSquiggly", "AnnotSquiggly");
    to_process_checks.emplace(L"Annot", L"AnnotStrikeOut", "AnnotStrikeOut");
    to_process_checks.emplace(L"Annot", L"AnnotCaret", "AnnotCaret");
    to_process_checks.emplace(L"Annot", L"AnnotStamp", "AnnotStamp");
    to_process_checks.emplace(L"Annot", L"AnnotInk", "AnnotInk");
    to_process_checks.emplace(L"Annot", L"AnnotPopup", "AnnotPopup");
    to_process_checks.emplace(L"Annot", L"AnnotFileAttachment", "AnnotFileAttachment");
    to_process_checks.emplace(L"Annot", L"AnnotSound", "AnnotSound");
    to_process_checks.emplace(L"Annot", L"AnnotMovie", "AnnotMovie");
    to_process_checks.emplace(L"Annot", L"AnnotScreen", "AnnotScreen");
    to_process_checks.emplace(L"Annot", L"AnnotWidget", "AnnotWidget");
    to_process_checks.emplace(L"Annot", L"AnnotPrinterMark", "AnnotPrinterMark");
    to_process_checks.emplace(L"Annot", L"AnnotTrapNet", "AnnotTrapNetwork");
    to_process_checks.emplace(L"Annot", L"AnnotWatermark", "AnnotWatermark");
    to_process_checks.emplace(L"Annot", L"Annot3D", "Annot3D");

    while (!to_process_checks.empty()) {
        ArlPDFObject* tmp_obj = nullptr;

        to_process_elem elem = to_process_checks.front();
        if (ArlingtonPDFShim::debugging) {
            ofs << "Processing DVA " << ToUtf8(elem.dva_link) << "/" << ToUtf8(elem.dva_link2) << " vs Arlington '" << elem.link << "'" << std::endl;
        }
        to_process_checks.pop();
        if (elem.link == "")
            continue;

        elem.link = remove_link_predicates(elem.link);
        std::vector<std::string> links = split(elem.link, ',');
        if (links.size() > 1) {
            //for (auto lnk : links)
            //  to_process_checks.emplace(elem.dva_link, lnk);
            continue;
        }

        auto found = mapped.find(elem.link);
        if (found != mapped.end()) {
            //    output << context << " already Processed" <<std::endl;
            if (found->second != elem.dva_link) {
                //rt ofs << "TODO: link validated in 2 different contexts: " << ToUtf8(found->second) << " ";
                //   ofs << " second: " << elem.link << " in: " << elem.context << std::endl;
            }
            continue;
        }
        mapped.insert(std::make_pair(elem.link, elem.dva_link));

        // locate dict in DVA
        ArlPDFDictionary* dict = (ArlPDFDictionary*)map_dict->get_value(elem.dva_link);
        if (dict == nullptr) {
            ofs << "ERROR: Adobe DVA problem (dictionary not found): " << ToUtf8(elem.dva_link) << std::endl;
            continue;
        }

        // load Arlington definition (TSV file)
        std::unique_ptr<CArlingtonTSVGrammarFile> reader(new CArlingtonTSVGrammarFile(tsv_dir / (elem.link + ".tsv")));
        if (!reader->load()) {
            ofs << "ERROR: loading Arlington TSV file " << (tsv_dir / (elem.link + ".tsv")) << std::endl;
            continue;
        }
        const ArlTSVmatrix* data_list = &reader->get_data();
        ofs << std::endl << count++ << ": Comparing Arlington:" << elem.link << " vs DVA:" << ToUtf8(elem.dva_link) << std::endl;

        // what Arlington has and Adobe DVA doesn't
        for (auto& vec : *data_list) {
            ArlPDFDictionary* inner_obj = nullptr;

            // Arlington wildcard key name or array elements
            ///@todo support repeating array index sets in Arlington
            if (vec[TSV_KEYNAME] == "*") {
                tmp_obj = dict->get_value(L"GenericKey");
                if (tmp_obj != nullptr) {
                    switch (tmp_obj->get_object_type()) {
                        case PDFObjectType::ArlPDFObjTypeDictionary:
                        {
                            inner_obj = (ArlPDFDictionary*)tmp_obj;
                            if (inner_obj == nullptr) {
                                ArlPDFArray* inner_array = (ArlPDFArray*)dict->get_value(L"Array");
                                if ((inner_array == nullptr) || (inner_array->get_num_elements() != 1)) {
                                    ofs << "ERROR: Arlington wildcard key vs DVA Array - either not linked or multiple links: " << ToUtf8(elem.dva_link) << "/" << vec[TSV_KEYNAME] << std::endl;
                                }
                                else {
                                    tmp_obj = inner_array->get_value(0);
                                    if ((tmp_obj != nullptr) && (tmp_obj->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary)) {
                                        inner_obj = (ArlPDFDictionary*)tmp_obj;
                                    }
                                    else {
                                        ofs << "ERROR: Adobe DVA " << ToUtf8(elem.dva_link) <<"/GenericKey/Array[0] entry was not a dictionary" << std::endl;
                                    }
                                }
                            }
                        }
                        break;
                        default: {
                            ofs << "ERROR: Adobe DVA GenericKey dictionary expected but different object type found" << std::endl;
                        }
                    } // switch
                }
                else {
                    ofs << "Arlington wildcard in " << elem.link << " did not have matching GenericKey entry in DVA: " << ToUtf8(elem.dva_link) << std::endl;
                }
            } else {
                tmp_obj = dict->get_value(ToWString(vec[TSV_KEYNAME]));
                if ((tmp_obj != nullptr) && (tmp_obj->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary)) {
                    inner_obj = (ArlPDFDictionary*)tmp_obj;
                }
            }

            // could be in "ConcatWithFormalReps" (elements in array are names)
            if (inner_obj == nullptr) {
                ArlPDFArray* inner_array = (ArlPDFArray*)dict->get_value(L"ConcatWithFormalReps");
                if (inner_array != nullptr) {
                    ArlPDFObject *o = inner_array->get_value(0);
                    if ((o != nullptr) && (o->get_object_type() == PDFObjectType::ArlPDFObjTypeName)) {
                        ArlPDFName* nm = (ArlPDFName*)o;
                        std::wstring new_dva_value = nm->get_value();
                        ArlPDFDictionary *d = (ArlPDFDictionary*)map_dict->get_value(new_dva_value);
                        if (d != nullptr)
                            inner_obj = (ArlPDFDictionary*)d->get_value(ToWString(vec[TSV_KEYNAME]));
                        else {
                            ofs << "ERROR: DVA ConcatWithFormalReps target missing for " << ToUtf8(new_dva_value) << " - " << ToUtf8(elem.dva_link) << std::endl;
                        }
                    }
                } else if (elem.dva_link2 != L"") {
                    ArlPDFDictionary* d = (ArlPDFDictionary*)map_dict->get_value(elem.dva_link2);
                    inner_obj = (ArlPDFDictionary*)d->get_value(ToWString(vec[TSV_KEYNAME]));
                }
            }

            if (inner_obj == nullptr) {
                // Avoid reporting all the PDF 2.0 new stuff...
                if (vec[TSV_SINCEVERSION] != "2.0") {
                    ofs << "Missing key in DVA: " << ToUtf8(elem.dva_link) << "/" << vec[TSV_KEYNAME] << " (" << vec[TSV_SINCEVERSION] << ")" << std::endl;
                }
                continue;
            }
            else {
                // Arlington IndirectReference can also have predicate "fn:MustBeDirect(...)" or be complex ([];[];[];...)
                // Linux CLI:  cut -f 6 *.tsv | sort | uniq
                // Arlington field is UPPERCASE
                if (inner_obj->has_key(L"MustBeIndirect")) {
                    std::string indirect = "FALSE";
                    ArlPDFObject* indr = inner_obj->get_value(L"MustBeIndirect");
                    if ((indr != nullptr) && (indr->get_object_type() == PDFObjectType::ArlPDFObjTypeBoolean)) {
                        ArlPDFBoolean* indr_b = (ArlPDFBoolean*)indr;
                        if (indr_b->get_value())
                            indirect = "TRUE";
                        if (vec[TSV_INDIRECTREF] != indirect) {
                            ofs << "Indirect is different in DVA: " << ToUtf8(elem.dva_link) << "/" << vec[TSV_KEYNAME] << "==" << indirect;
                            ofs << " vs Arlington: " << elem.link << "/" << vec[TSV_KEYNAME] << "==" << vec[TSV_INDIRECTREF] << std::endl;
                        }
                    }
                    else {
                        ofs << "ERROR: DVA MustBeIndirect is not a Boolean " << ToUtf8(elem.dva_link) << std::endl;
                    }
                }
                else {
                    // Not reported as too noisy
                    // ofs << "DVA does not specify MustBeIndirect for " << ToUtf8(elem.dva_link) << std::endl;
                }

                // Arlington Required can also have predicate "fn:IsRequired(...)" or be complex ([];[];[];...)
                // Linux CLI:  cut -f 5 *.tsv | sort | uniq
                // Arlington field is UPPERCASE
                if (inner_obj->has_key(L"Required")) {
                    std::string required = "FALSE";
                    ArlPDFObject* req = inner_obj->get_value(L"Required");
                    if ((req != nullptr) && (req->get_object_type() == PDFObjectType::ArlPDFObjTypeBoolean)) {
                        ArlPDFBoolean *req_b = (ArlPDFBoolean *)req;
                        if (req_b->get_value())
                            required = "TRUE";
                        if (vec[TSV_REQUIRED] != required) {
                            ofs << "Required is different DVA: " << ToUtf8(elem.dva_link) << "/" << vec[TSV_KEYNAME] << "==" << required;
                            ofs << " vs Arlington: " << elem.link << "/" << vec[TSV_KEYNAME] << "==" << vec[TSV_REQUIRED] << std::endl;
                        }
                    }
                    else {
                        ofs << "ERROR: DVA Required is not a Boolean " << ToUtf8(elem.dva_link) << std::endl;
                    }
                }
                else {
                    ofs << "ERROR: DVA does not specify Required for " << ToUtf8(elem.dva_link) << std::endl;
                }

                // Arlington SinceVersion (1.0, 1.1, ..., 2.0)
                // Linux CLI: cut -f 3 *.tsv | sort | uniq
                if (inner_obj->has_key(L"PDFMajorVersion") && inner_obj->has_key(L"PDFMinorVersion")) {
                    ArlPDFObject *major = inner_obj->get_value(L"PDFMajorVersion");
                    ArlPDFObject* minor = inner_obj->get_value(L"PDFMinorVersion");
                    if ((major != nullptr) && (minor != nullptr) &&
                        (major->get_object_type() == PDFObjectType::ArlPDFObjTypeNumber) &&
                        (minor->get_object_type() == PDFObjectType::ArlPDFObjTypeNumber)) {
                        int   pdf_major = ((ArlPDFNumber*)major)->get_integer_value();
                        int   pdf_minor = ((ArlPDFNumber *)minor)->get_integer_value();
                        std::string ver = std::to_string(pdf_major) + "." + std::to_string(pdf_minor);
                        if (ver != vec[TSV_SINCEVERSION]) {
                            ofs << "SinceVersion is different in DVA: " << ToUtf8(elem.dva_link) << " (" << ver << ") ";
                            ofs << " vs Arlington: " << elem.link << "/" << vec[TSV_KEYNAME] << " (" << vec[TSV_SINCEVERSION] << ")" << std::endl;
                        }
                    }
                    else {
                        ofs << "ERROR: DVA PDFMajorVersion/PDFMinorVersion is invalid for " << ToUtf8(elem.dva_link) << std::endl;
                    }
                }

                // Check allowed Types
                tmp_obj = inner_obj->get_value(L"ValueType");
                if (tmp_obj == nullptr) {
                    ofs << "ERROR: No ValueType defined for DVA: " << ToUtf8(elem.dva_link) << "/" << vec[TSV_KEYNAME] << std::endl;
                }
                else if (tmp_obj->get_object_type() != PDFObjectType::ArlPDFObjTypeArray) {
                    ofs << "ERROR: ValueType is not an array for DVA: " << ToUtf8(elem.dva_link) << "/" << vec[TSV_KEYNAME] << std::endl;
                }
                else {
                    ArlPDFArray*                types_array = (ArlPDFArray*)tmp_obj;
                    std::vector<std::string>    types_our   = split(vec[TSV_TYPE], ';');

                    // Map Arlington types (always lowercase) to Adobe DVA types ("CosXxxx")
                    for (size_t i = 0; i < types_our.size(); i++) {
                        if (types_our[i] == "boolean")
                            types_our[i] = "CosBool";
                        else if (types_our[i] == "name")
                            types_our[i] = "CosName";
                        else if (types_our[i] == "number") {
                            types_our[i] = "CosFixed";
                            types_our.push_back("CosInteger");
                        }
                        else if (types_our[i] == "integer" || types_our[i] == "bitmask")
                            types_our[i] = "CosInteger";
                        else if (types_our[i] == "stream")
                            types_our[i] = "CosStream";
                        else if (types_our[i] == "array" || types_our[i] == "rectangle" || types_our[i] == "matrix")
                            types_our[i] = "CosArray";
                        else if (types_our[i] == "dictionary" || types_our[i] == "name-tree" || types_our[i] == "number-tree")
                            types_our[i] = "CosDict";
                        else if (types_our[i] == "string" || types_our[i] == "date" || types_our[i] == "string-byte" || types_our[i] == "string-text" || types_our[i] == "string-ascii")
                            types_our[i] = "CosString";
                    } // for

                    // DVA types are stored as names
                    std::vector<std::string> types_dva;
                    for (int i = 0; i < types_array->get_num_elements(); i++) {
                        std::wstring    new_dva_value;
                        ArlPDFObject    *obj = types_array->get_value(i);
                        if ((obj != nullptr) && (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeName)) {
                            ArlPDFName *nm = (ArlPDFName *)obj;
                            new_dva_value = nm->get_value();
                            for (size_t j = 0; j < types_our.size(); j++) {
                                if (types_our[j] == ToUtf8(new_dva_value)) {
                                    types_our[j] = "";
                                    new_dva_value = L"";
                                    break;
                                }
                            } // for
                            types_dva.push_back(ToUtf8(new_dva_value));
                        }
                        else {
                            ofs << "ERROR: DVA ValueType array element is not a name object" << std::endl;
                        }
                    } // for

                    std::string head = "==Key DVA: " + ToUtf8(elem.dva_link) +" vs Arlington: " + elem.link + "/" + vec[TSV_KEYNAME] + "\n";
                    std::string our("");
                    for (auto& tpe : types_our)
                        if (tpe != "") {
                            if (our == "")
                                our = tpe;
                            else
                                our += ", " + tpe;
                        }

                    if (our != "") {
                        ofs << head << "\tArlington: " << our << std::endl;
                        head = "";
                    }

                    our = "";
                    for (auto& tpe : types_dva)
                        if (tpe != "") {
                            if (our == "")
                                our = tpe;
                            else
                                our += ", " + tpe;
                        }

                    if (our != "") {
                        if (head != "")
                            ofs << head;
                        ofs << "\tDVA: " << our << std::endl;
                    }
                }

                // Check Arlington PossibleValue vs DVA Bounds
                tmp_obj = inner_obj->get_value(L"Bounds");
                if ((tmp_obj != nullptr) && (tmp_obj->get_object_type() != PDFObjectType::ArlPDFObjTypeDictionary)) {
                    ofs << "ERROR: Bounds is not a dictionary for DVA: " << ToUtf8(elem.dva_link) << "/" << vec[TSV_KEYNAME] << std::endl;
                }
                else if (vec[TSV_POSSIBLEVALUES] != "") {
                    ArlPDFDictionary *bounds_dict = (ArlPDFDictionary*)tmp_obj;
                    std::string possible = "";

                    if (bounds_dict == nullptr) {
                        ofs << "Bounds not defined in DVA for " << ToUtf8(elem.dva_link) << ", but PossibleValue defined in Arlington: ";
                        ofs << elem.link << "/" << vec[TSV_KEYNAME] << "==" << vec[TSV_POSSIBLEVALUES] << std::endl;
                    }
                    else {
                        tmp_obj = bounds_dict->get_value(L"Equals");
                        if ((tmp_obj != nullptr) && (tmp_obj->get_object_type() == PDFObjectType::ArlPDFObjTypeArray)) {
                            std::vector<std::string>    possible_dva;
                            ArlPDFArray*                possible_array = (ArlPDFArray*)tmp_obj;

                            // Arlington PossibleValues (column 9) can be COMMA-separated, complex ([a,fn:A(b),c];[d,fn:B(1,2,fn:C(3,4,e)),f];[g,h,i];...) which
                            // includes nested predicates that also use COMMAs. Sigh!
                            // Split by ";" first to remove predicates as they use COMMAs as argument separators.
                            /// @todo Spliting again by "," will not work properly as some predicates use COMMA!!! Hence garbled output sometimes.
                            /// For now use remove_type_predicates() which removes fn:SinceVersion and fn:Deprecated predicates only.
                            std::vector<std::vector<std::string>>   possible_our;
                            {
                                std::vector<std::string> pv_typed = split(vec[TSV_POSSIBLEVALUES], ';');
                                std::string s;
                                for (size_t i = 0; i < pv_typed.size(); i++) {
                                    s = remove_type_predicates(pv_typed[i]);  // removes fn:SinceVersion and fn:Deprecated predicates only
                                    if (s[0] == '[')
                                        s = s.substr(1, s.size() - 2); // strip off [ and ]
                                    pv_typed[i] = s;
                                    possible_our.push_back(split(pv_typed[i], ','));
                                } // for
                            }

                            ArlPDFObject*  obj;
                            for (int i = 0; i < possible_array->get_num_elements(); i++) {
                                std::wstring   new_dva_value = L"";

                                // Bounds array elements can be any basic type
                                // Convert to string for simplistic text comparison
                                obj = possible_array->get_value(i);
                                if (obj != nullptr) {
                                    switch (obj->get_object_type()) {
                                        case PDFObjectType::ArlPDFObjTypeBoolean: {
                                                ArlPDFBoolean* b = (ArlPDFBoolean*)obj;
                                                new_dva_value = (b->get_value() ? L"true" : L"false");
                                            }
                                            break;
                                        case PDFObjectType::ArlPDFObjTypeName: {
                                                ArlPDFName* nm = (ArlPDFName*)obj;
                                                new_dva_value = nm->get_value();
                                            }
                                            break;
                                        case PDFObjectType::ArlPDFObjTypeNumber: {
                                                ArlPDFNumber* num = (ArlPDFNumber*)obj;
                                                if (num->is_integer_value()) {
                                                    new_dva_value = std::to_wstring(num->get_integer_value());
                                                }
                                                else {
                                                    new_dva_value = std::to_wstring(num->get_value());
                                                }
                                            }
                                            break;
                                        case PDFObjectType::ArlPDFObjTypeString: {
                                                ArlPDFString* str = (ArlPDFString*)obj;
                                                new_dva_value = str->get_value();
                                            }
                                            break;
                                        default:
                                            ofs << "ERROR: DVA Bounds/Equal[" << i << "] was an unexpected type for " << ToUtf8(elem.dva_link) << std::endl;
                                            break;
                                    } // switch

                                    // Find if there is a match in PossibleValues
                                    if (new_dva_value != L"") {
                                        for (size_t j = 0; j < possible_our.size(); j++) { // split by ';'
                                            for (size_t k = 0; k < possible_our[j].size(); k++) { // split by ','
                                                if (possible_our[j][k] == ToUtf8(new_dva_value)) {
                                                    possible_our[j][k] = "";
                                                    new_dva_value = L"";
                                                    break;
                                                }
                                            }
                                        } // for
                                        if (new_dva_value != L"")
                                            possible_dva.push_back(ToUtf8(new_dva_value));
                                    }
                                } else {
                                    ofs << "ERROR: DVA Bounds/Equal[" << i << "] was a null object for " << ToUtf8(elem.dva_link) << std::endl;
                                }// if
                            } // for

                            std::string head = "==PossibleValue DVA: " + ToUtf8(elem.dva_link) + " vs Arlington: " + elem.link + "/" + vec[TSV_KEYNAME] + "\n";
                            std::string our = "";
                            for (size_t j = 0; j < possible_our.size(); j++) { // split by ';'
                                for (size_t k = 0; k < possible_our[j].size(); k++) { // split by ','
                                    if (possible_our[j][k] != "") {
                                        if (our == "")
                                            our = possible_our[j][k];
                                        else
                                            our += ", " + possible_our[j][k];
                                    }
                                }
                            }

                            if (our != "") {
                                ofs << head << "\tArlington: " << our << std::endl;
                                head = "";
                            }

                            our = "";
                            for (auto& tpe : possible_dva)
                                if (tpe != "") {
                                    if (our == "")
                                        our = tpe;
                                    else
                                        our += ", " + tpe;
                                }

                            if (our != "") {
                                if (head != "")
                                    ofs << head;
                                ofs << "\tDVA: " << our << std::endl;
                            }
                        }
                        else {
                            ofs << "ERROR: DVA Bounds/Equal was not an array for " << ToUtf8(elem.dva_link) << std::endl;
                        }
                    }
                }
            }

            ArlPDFObject* link_obj = inner_obj->get_value(L"VerifyAtFormalRep"); // 0-dict, 1-stream, 2-array

            ArlPDFObject* recursion_obj = inner_obj->get_value(L"RecursionParams");
            if ((recursion_obj != nullptr) && (recursion_obj->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary)) {
                // ofs << "TODO: RecursionParams - special validation required: " << elem.link << "/" << vec[TSV_KEYNAME] << std::endl;
                link_obj = ((ArlPDFDictionary*)recursion_obj)->get_value(L"VerifyAtFormalRep"); // 0-dict, 1-stream, 2-array
            }

            if (link_obj != nullptr) {
                // Should be array or name
                switch (link_obj->get_object_type()) {
                    case PDFObjectType::ArlPDFObjTypeName:
                        {
                            ArlPDFName* nm = (ArlPDFName*)link_obj;
                            std::wstring  dva_link_str_value = nm->get_value();
                            if (vec[TSV_LINK] == "") {
                                ofs << "No link in Arlington: " << elem.link << "/" << vec[TSV_KEYNAME] << " (" << vec[TSV_TYPE] << ")" << std::endl;
                            }
                            else {
                                std::vector<std::string> lnk = split(vec[TSV_LINK], ';');
                                for (std::string s : lnk) {
                                    if (s.size() > 3) {                 // Link is not just "[]"
                                        s = s.substr(1, s.size() - 2);  // strip off [ and ] to make an Arlington TSV filename
                                        to_process_checks.emplace(dva_link_str_value, s);
                                    }
                                }
                            }
                        }
                        break;
                    case PDFObjectType::ArlPDFObjTypeArray:
                        {
                            std::wstring  link_dict_value = L"";
                            std::wstring  link_stream_value = L"";
                            std::wstring  link_array_value = L"";
                            ArlPDFArray* arr = (ArlPDFArray*)link_obj;
                            ArlPDFObject* obj;
                            ArlPDFString* str;

                            // dictionary
                            obj = arr->get_value(0);
                            if ((obj != nullptr) && (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeString)) {
                                str = (ArlPDFString*)obj;
                                link_dict_value = str->get_value();
                            }

                            // stream
                            obj = arr->get_value(1);
                            if ((obj != nullptr) && (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeString)) {
                                str = (ArlPDFString*)obj;
                                link_stream_value = str->get_value();
                            }

                            // array
                            obj = arr->get_value(2);
                            if ((obj != nullptr) && (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeString)) {
                                str = (ArlPDFString*)obj;
                                link_array_value = str->get_value();
                            }

                            std::string lnk_dict   = get_link_for_type("dictionary", vec[TSV_TYPE], vec[TSV_LINK]);
                            std::string lnk_stream = get_link_for_type("stream",     vec[TSV_TYPE], vec[TSV_LINK]);
                            std::string lnk_array  = get_link_for_type("array",      vec[TSV_TYPE], vec[TSV_LINK]);

                            if (link_dict_value != L"" && lnk_dict != "[]")
                                to_process_checks.emplace(link_dict_value, lnk_dict.substr(1, lnk_dict.size() - 2));
                            else if (!(link_dict_value == L"" && lnk_dict == "[]")) {
                                //rt ofs << "TODO: Check link dictionary: " << ToUtf8(elem.dva_link) << "/" << vec[TSV_KEYNAME] << std::endl;
                            }

                            if (link_stream_value != L"" && lnk_stream != "[]")
                                to_process_checks.emplace(link_stream_value, lnk_stream.substr(1, lnk_stream.size() - 2));
                            else if (!(link_stream_value == L"" && lnk_stream == "[]")) {
                                //rt ofs << "TODO: Check link stream: " << ToUtf8(elem.dva_link) << "/" << vec[TSV_KEYNAME] << std::endl;
                            }

                            if (link_array_value != L"" && lnk_array != "[]")
                                to_process_checks.emplace(link_array_value, lnk_array.substr(1, lnk_array.size() - 2));
                            else if (!(link_array_value == L"" && lnk_array == "[]")) {
                                //rt ofs << "TODO: Check link array: " << ToUtf8(elem.dva_link) << "/" << vec[TSV_KEYNAME] << std::endl;
                            }
                        }
                        break;
                default:
                    ofs << "ERROR: Unexpected DVA type for VerifyAtFormalRep!" << std::endl;
                    break;
                } // switch
            } // if
        } // for


        // @brief Checks if a key name exists in Arlington
        // @param[in] key key name (string)
        // @returns true if key exists in Arlington
        auto exists_in_our = [=](auto key) {
            for (auto& vec : *data_list)
                if (vec[TSV_KEYNAME] == ToUtf8(key))
                    return true;
            return false;
        }; // auto


        // @brief Iterates through all keys in a DVA PDF dictionary to see if they are in Arlington
        // @param[in] a_dict   the DVA PDF dictionary
        // @param[in] in_ofs   report stream
        auto check_dict = [=](ArlPDFDictionary* a_dict, std::ostream& in_ofs) {
            for (int i = 0; i < (a_dict->get_num_keys()); i++) {
                std::wstring key = a_dict->get_key_name_by_index(i);
                if (!exists_in_our(key) && (key != L"FormalRepOf") && (key != L"Array)")
                    && (key != L"ArrayStyle") && (key != L"FormalRepOfArray") && (key != L"OR")
                    && (key != L"GenericKey") && (key != L"ConcatWithFormalReps")) {
                    in_ofs << "Missing key in Arlington: " << elem.link << "/" << ToUtf8(key) << std::endl;
                }
            }
        }; // auto

        check_dict(dict, ofs);

        tmp_obj = dict->get_value(L"ConcatWithFormalReps");
        if ((tmp_obj != nullptr) && (tmp_obj->get_object_type() == PDFObjectType::ArlPDFObjTypeArray)) {
            ArlPDFArray* inner_array = (ArlPDFArray*)tmp_obj;
            ArlPDFObject *obj = inner_array->get_value(0);
            if (obj != nullptr) {
                switch (obj->get_object_type()) {
                    case PDFObjectType::ArlPDFObjTypeString:
                        {
                            ArlPDFString *concat = (ArlPDFString*)obj;
                            std::wstring  new_dva_value = concat->get_value();
                            tmp_obj = map_dict->get_value(new_dva_value);
                            if ((tmp_obj != nullptr) && (tmp_obj->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary)) {
                                check_dict((ArlPDFDictionary*)tmp_obj, ofs);
                            }
                            else {
                                ofs << "ERROR: DVA "<< ToUtf8(elem.dva_link) << " ConcatWithFormalReps[0]/(" << ToUtf8(concat->get_value()) << ") string was not a dictionary" << std::endl;
                            }
                        }
                        break;
                    case PDFObjectType::ArlPDFObjTypeName:
                        {
                            ArlPDFName* concat = (ArlPDFName*)obj;
                            std::wstring  new_dva_value = concat->get_value();
                            tmp_obj = map_dict->get_value(new_dva_value);
                            if ((tmp_obj != nullptr) && (tmp_obj->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary)) {
                                check_dict((ArlPDFDictionary*)tmp_obj, ofs);
                            }
                            else {
                                ofs << "ERROR: DVA " << ToUtf8(elem.dva_link) << " ConcatWithFormalReps[0]/" << ToUtf8(concat->get_value()) << " name was not a dictionary" << std::endl;
                            }
                        }
                        break;
                    default:
                        {
                            ofs << "ERROR: DVA " << ToUtf8(elem.dva_link) << " ConcatWithFormalReps[0] was an unexpected type" << std::endl;
                        }
                        break;
                } // switch
            }
            else {
                ofs << "ERROR: DVA " << ToUtf8(elem.dva_link) << " ConcatWithFormalReps[0] did not exist" << std::endl;
            }
        }
    }
}


/// @brief  Compares Arlington TSV file set against Adobe DVA formal representation PDF
/// 
/// @param[in] pdfsdk          already instantiated PDF SDK Arlington shim object
/// @param[in] dva_file        the Adobe DVA PDF file with the FormalRep tree
/// @param[in] grammar_folder  the Arlington PDF model folder with TSV file set
/// @param[in] ofs             report stream
void CheckDVA(ArlingtonPDFShim::ArlingtonPDFSDK &pdfsdk, const std::filesystem::path& dva_file, const fs::path& grammar_folder, std::ostream& ofs) {
    try {
        ofs << "BEGIN - Arlington vs Adobe DVA Report - TestGrammar " << TestGrammar_VERSION << " " << pdfsdk.get_version_string() << std::endl;
        ofs << "Arlington TSV data: " << fs::absolute(grammar_folder).lexically_normal() << std::endl;
        ofs << "Adobe DVA FormalRep file: " << fs::absolute(dva_file).lexically_normal() << std::endl;

        ArlPDFTrailer* trailer = pdfsdk.get_trailer(dva_file.wstring());
        if (trailer != nullptr) {
            ArlPDFObject* root = trailer->get_value(L"Root");
            if ((root != nullptr) && (root->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary)) {
                ArlPDFObject* formal_rep = ((ArlPDFDictionary *)root)->get_value(L"FormalRepTree");
                if ((formal_rep != nullptr) && (formal_rep->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary)) {
                     process_dict(grammar_folder, ofs, (ArlPDFDictionary*)formal_rep);
                }
                else {
                    ofs << "Error: failed to acquire Trailer/Root/FormalRepTree" << std::endl;
                }
            }
            else {
                ofs << "Error: failed to acquire Trailer/Root" << std::endl;
            }
        }
        else {
            ofs << "Error: failed to acquire Trailer" << std::endl;
        }
    }
    catch (std::exception& ex) {
        ofs << "Error: EXCEPTION: " << ex.what() << std::endl;
    }

    // Finally...
    ofs << "END" << std::endl;
}
