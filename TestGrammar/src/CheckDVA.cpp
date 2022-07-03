///////////////////////////////////////////////////////////////////////////////
/// @file 
/// @brief Compares an Arlington PDF model to the Adobe DVA FormalRep as defined in a PDF file
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
/// @author Frantisek Forgac, Normex
/// @author Peter Wyatt, PDF Association
/// 
///////////////////////////////////////////////////////////////////////////////

#include "ArlingtonPDFShim.h"
#include "CheckGrammar.h"
#include "ArlingtonTSVGrammarFile.h"
#include "TestGrammarVers.h"
#include "utils.h"

#include <algorithm>
#include <exception>
#include <queue>
#include <map>
#include <cassert>

using namespace ArlingtonPDFShim;
namespace fs = std::filesystem;

/// @brief define DVA_TRACING for verbose output about which DVA objects are getting read
/// 
/// PDF1_7FormalRep.pdf DocCat::FormalRepTree dictionary has 564 entries total, but need 
/// to ignore operators, operands, etc. Export to a text file as "PDF1_7FormalRep.cos" then
/// manually delete anything with "Operand" or "Operator"
/// 
/// grep "Reading DVA object" dva.txt | sort | uniq | wc -l ===> 266
///  vs
/// grep --color=never -Po "(?<=^\t\/)[^\t]*" PDF1_7FormalRep.cos | wc -l ===> 436
///  vs
/// ls --color=never -1 arlington-pdf-model/tsv/latest *.tsv | wc -l ==> 520
#undef DVA_TRACING


/// @brief Class for storing Adobe DVA object and Arlington TSV pairs
class to_process_elem {
public:
    std::wstring  dva_link;  // Adobe DVA object (key name so wstring)
    std::string   link;      // Arlington TSV filename

    to_process_elem()
        { /* default (empty) constructor */ };

    to_process_elem(const std::wstring dva_lnk, const std::string our_lnk)
        : dva_link(dva_lnk), link(our_lnk)
        { /* constructor */ };

    bool operator == (const to_process_elem& a)
        { /* comparison */ return ((link == a.link) && (dva_link == a.dva_link)); };
};


/// @brief  Process a single Adobe DVA dictionary definition
/// 
/// @param[in]     tsv_dir Arlington TSV directory
/// @param[in,out] ofs     already open output stream for reporting messages
/// @param[in]     map     the Adobe DVA root dictionary for comparison
/// @param[in]     terse   whether output should be brief/terse
/// 
void process_dict(const fs::path &tsv_dir, std::ostream& ofs, ArlPDFDictionary * map_dict, bool terse) {
    int count = 0;

    // A vector of already completed comparisons made up of an Arlington / Adobe DVA key pairs
    std::vector<to_process_elem>                mapped;

    // A vector of to-be-done comparisons made up of an Arlington / Adobe DVA key pairs
    std::queue<to_process_elem>                 to_process_checks;

    // Pre-populate some DVA vs Arlington tuples - mostly direct name matches
    // Group into roughly logical order so that output is also grouped
    to_process_checks.emplace(L"Trailer",           "FileTrailer");
    to_process_checks.emplace(L"Catalog",           "Catalog");
    to_process_checks.emplace(L"XRef",              "XRefStream");
    // 3D
    to_process_checks.emplace(L"3DCrossSection",    "3DCrossSection");
    to_process_checks.emplace(L"3DLightingScheme",  "3DCrossSection");
    to_process_checks.emplace(L"3DNode",            "3DNode");
    to_process_checks.emplace(L"3DRenderMode",      "3DRenderMode");
    // Page tree
    to_process_checks.emplace(L"PagesOrPage",       "PageTreeNodeRoot");
    to_process_checks.emplace(L"PagesOrPage",       "PageTreeNode");
    to_process_checks.emplace(L"Pages",             "PageTreeNodeRoot");
    to_process_checks.emplace(L"Pages",             "PageTreeNode");
    to_process_checks.emplace(L"Page",              "PageObject");
    // Bead
    to_process_checks.emplace(L"Bead_First",        "BeadFirst");
    to_process_checks.emplace(L"Bead",              "Bead");
    to_process_checks.emplace(L"Thread",            "Thread");
    // Outlines
    to_process_checks.emplace(L"Outline",           "OutlineItem");
    to_process_checks.emplace(L"Outlines",          "Outline");
    // Patterns
    to_process_checks.emplace(L"Pattern",           "PatternType1");
    to_process_checks.emplace(L"Pattern",           "PatternType2");
    to_process_checks.emplace(L"PatternType1",      "PatternType1");
    to_process_checks.emplace(L"PatternType2",      "PatternType2");
    // Font
    to_process_checks.emplace(L"FontType1",         "FontType1");
    to_process_checks.emplace(L"FontTrueType",      "FontTrueType");
    to_process_checks.emplace(L"FontMMType1",       "FontMultipleMaster");
    to_process_checks.emplace(L"FontType3",         "FontType3");
    to_process_checks.emplace(L"FontType0",         "FontType0");
    to_process_checks.emplace(L"FontCIDFontType0or2", "FontCIDType0");
    to_process_checks.emplace(L"FontCIDFontType0or2", "FontCIDType2");
    // Font Descriptors
    to_process_checks.emplace(L"FontDescriptor",    "FontDescriptorCIDType0");
    to_process_checks.emplace(L"FontDescriptor",    "FontDescriptorCIDType2");
    to_process_checks.emplace(L"FontDescriptor",    "FontDescriptorTrueType");
    to_process_checks.emplace(L"FontDescriptor",    "FontDescriptorType1");
    to_process_checks.emplace(L"FontDescriptor",    "FontDescriptorType3");
    // Font files
    to_process_checks.emplace(L"FontFile",          "FontFile");
    to_process_checks.emplace(L"FontFile",          "FontFile2");
    to_process_checks.emplace(L"FontFile",          "FontFile3CIDType0");
    to_process_checks.emplace(L"FontFile",          "FontFile3Type1");
    to_process_checks.emplace(L"Type1FontFile3",    "FontFile3Type1");
    to_process_checks.emplace(L"FontFile",          "FontFileType1");
    to_process_checks.emplace(L"Type1FontFile",     "FontFileType1");
    // Functions
    to_process_checks.emplace(L"FunctionType0",     "FunctionType0");
    to_process_checks.emplace(L"FunctionType2",     "FunctionType2");
    to_process_checks.emplace(L"FunctionType3",     "FunctionType3");
    to_process_checks.emplace(L"FunctionType4",     "FunctionType4");
    to_process_checks.emplace(L"Function",          "FunctionType0");
    to_process_checks.emplace(L"Function",          "FunctionType2");
    to_process_checks.emplace(L"Function",          "FunctionType3");
    to_process_checks.emplace(L"Function",          "FunctionType4");
    // Halftones
    to_process_checks.emplace(L"Halftone",          "HalftoneType1");
    to_process_checks.emplace(L"Halftone",          "HalftoneType5");
    to_process_checks.emplace(L"Halftone",          "HalftoneType6");
    to_process_checks.emplace(L"Halftone",          "HalftoneType10");
    to_process_checks.emplace(L"Halftone",          "HalftoneType16");
    to_process_checks.emplace(L"HalftoneType1",     "HalftoneType1");
    to_process_checks.emplace(L"HalftoneType5",     "HalftoneType5");
    to_process_checks.emplace(L"HalftoneType6",     "HalftoneType6");
    to_process_checks.emplace(L"HalftoneType10",    "HalftoneType10");
    to_process_checks.emplace(L"HalftoneType16",    "HalftoneType16");
    // XObjects
    to_process_checks.emplace(L"XObjectForm",       "XObjectFormType1");
    to_process_checks.emplace(L"XObjectImageBase",  "XObjectImage");
    to_process_checks.emplace(L"XObjectPS",         "XObjectFormPS");
    to_process_checks.emplace(L"XObjectPS",         "XObjectFormPSpassthrough");
    to_process_checks.emplace(L"XObjectTrapNet",    "XObjectFormTrapNet");
    to_process_checks.emplace(L"XObjectImageMask",  "XObjectImageMask");
    to_process_checks.emplace(L"XObjectImageSMask", "XObjectImageSoftMask");
    // Rendition
    to_process_checks.emplace(L"Rendition",         "RenditionMedia");
    to_process_checks.emplace(L"Rendition",         "RenditionSelector");
    to_process_checks.emplace(L"MediaRendition",    "RenditionMedia");
    to_process_checks.emplace(L"SelectorRendition", "RenditionSelector");
    // SigRef
    to_process_checks.emplace(L"SigRef",            "SignatureReferenceDocMDP");
    to_process_checks.emplace(L"SigRef",            "SignatureReferenceFieldMDP");
    to_process_checks.emplace(L"SigRef",            "SignatureReferenceIdentity");
    to_process_checks.emplace(L"SigRef",             "SignatureReferenceUR");
    to_process_checks.emplace(L"SigRefDocMDP",      "SignatureReferenceDocMDP");
    to_process_checks.emplace(L"SigRefFieldMDP",    "SignatureReferenceFieldMDP");
    to_process_checks.emplace(L"SigRefIdentity",    "SignatureReferenceIdentity");
    to_process_checks.emplace(L"SigRefUR",          "SignatureReferenceUR");
    // Actions
    to_process_checks.emplace(L"ActionGoTo",        "ActionGoTo");
    to_process_checks.emplace(L"ActionGoTo3DView",  "ActionGoTo3DView");
    // ActionGoToDp = new in PDF 2.0
    to_process_checks.emplace(L"ActionGoToE",       "ActionGoToE");
    to_process_checks.emplace(L"ActionGoToR",       "ActionGoToR");
    to_process_checks.emplace(L"ActionHide",        "ActionHide");
    to_process_checks.emplace(L"ActionImportData",  "ActionImportData");
    to_process_checks.emplace(L"ActionJavaScript",  "ActionECMAScript");
    to_process_checks.emplace(L"ActionLaunch",      "ActionLaunch");
    to_process_checks.emplace(L"ActionMovie",       "ActionMovie");
    to_process_checks.emplace(L"ActionNamed",       "ActionNamed");
    to_process_checks.emplace(L"ActionRendition",   "ActionRendition");
    to_process_checks.emplace(L"ActionResetForm",   "ActionResetForm");
    // ActionRichMediaExecute = new in PDF 2.0
    to_process_checks.emplace(L"ActionSetOCGState", "ActionSetOCGState");
    to_process_checks.emplace(L"ActionSound",       "ActionSound");
    to_process_checks.emplace(L"ActionSubmitForm",  "ActionSubmitForm");
    to_process_checks.emplace(L"ActionThread",      "ActionThread");
    to_process_checks.emplace(L"ActionTrans",       "ActionTransition");
    to_process_checks.emplace(L"ActionURI",         "ActionURI");
    to_process_checks.emplace(L"ArrayOfActions",    "ArrayOfActions");
    //
    to_process_checks.emplace(L"AlternateImageDict", "AlternateImage");
    // Annotations
    to_process_checks.emplace(L"Annot3D",           "Annot3D");
    to_process_checks.emplace(L"AnnotCaret",        "AnnotCaret");
    to_process_checks.emplace(L"AnnotCircle",       "AnnotCircle");
    to_process_checks.emplace(L"AnnotFileAttachment", "AnnotFileAttachment");
    to_process_checks.emplace(L"AnnotFreeText",     "AnnotFreeText");
    to_process_checks.emplace(L"AnnotHighlight",    "AnnotHighlight");
    to_process_checks.emplace(L"AnnotInk",          "AnnotInk");
    to_process_checks.emplace(L"AnnotLine",         "AnnotLine");
    to_process_checks.emplace(L"AnnotLink",         "AnnotLink");
    to_process_checks.emplace(L"AnnotMovie",        "AnnotMovie");
    to_process_checks.emplace(L"AnnotPolyLine",     "AnnotPolyLine");
    to_process_checks.emplace(L"AnnotPolygon",      "AnnotPolygon");
    to_process_checks.emplace(L"AnnotPopup",        "AnnotPopup");
    to_process_checks.emplace(L"AnnotPrinterMark",  "AnnotPrinterMark");
    // AnnotProjection = new in PDF 2.0
    to_process_checks.emplace(L"Annot",             "AnnotRedact");
    // AnnotRichMedia = new in PDF 2.0
    to_process_checks.emplace(L"AnnotScreen",       "AnnotScreen");
    to_process_checks.emplace(L"AnnotSound",        "AnnotSound");
    to_process_checks.emplace(L"AnnotSquare",       "AnnotSquare");
    to_process_checks.emplace(L"AnnotSquiggly",     "AnnotSquiggly");
    to_process_checks.emplace(L"AnnotStamp",        "AnnotStamp");
    to_process_checks.emplace(L"AnnotStrikeOut",    "AnnotStrikeOut");
    to_process_checks.emplace(L"AnnotText",         "AnnotText");
    to_process_checks.emplace(L"AnnotTrapNet",      "AnnotTrapNetwork");
    to_process_checks.emplace(L"AnnotUnderline",    "AnnotUnderline");
    to_process_checks.emplace(L"AnnotWatermark",    "AnnotWatermark");
    to_process_checks.emplace(L"AnnotWidget",       "AnnotWidget");
    // Colorspaces
    to_process_checks.emplace(L"CalGrayColorSpace", "CalGrayColorSpace");
    to_process_checks.emplace(L"CalGrayDict",       "CalGrayDict");
    to_process_checks.emplace(L"CalRGBColorSpace",  "CalRGBColorSpace");
    to_process_checks.emplace(L"CalRGBDict",        "CalRGBDict");
    to_process_checks.emplace(L"CalGrayDict",       "CalGrayDict");
    to_process_checks.emplace(L"ICCBasedColorSpace","ICCBasedColorSpace");
    to_process_checks.emplace(L"ICCBasedDict",      "ICCProfileStream");
    to_process_checks.emplace(L"IndexedColorSpace", "IndexedColorSpace");
    to_process_checks.emplace(L"LabColorSpace",     "LabColorSpace");
    to_process_checks.emplace(L"LabDict",           "LabDict");
    to_process_checks.emplace(L"DeviceNColorSpace", "DeviceNColorSpace");
    to_process_checks.emplace(L"DeviceNDict",       "DeviceNDict");
    to_process_checks.emplace(L"DeviceNMixingHints","DeviceNMixingHints");
    to_process_checks.emplace(L"DeviceNProcess",    "DeviceNProcess");
    to_process_checks.emplace(L"SeparationColorSpace", "SeparationColorSpace");
    to_process_checks.emplace(L"PatternColorSpace", "PatternColorSpace");
    // Appearances
    to_process_checks.emplace(L"Appearance",        "Appearance");
    to_process_checks.emplace(L"AppearanceCharacteristics", "AppearanceCharacteristics");
    to_process_checks.emplace(L"AppearanceSubDict", "AppearanceSubDict");
    to_process_checks.emplace(L"AppearanceTrapNet", "AppearanceTrapNet");
    to_process_checks.emplace(L"AppearanceTrapNetDict", "AppearanceTrapNetSubDict");
    to_process_checks.emplace(L"AppearanceTrapNet", "AppearanceTrapNet");
    to_process_checks.emplace(L"AppearanceCharacteristics", "AppearanceCharacteristics");
    // Misc
    to_process_checks.emplace(L"BorderEffect",      "BorderEffect");
    to_process_checks.emplace(L"BorderStyle",       "BorderStyle");
    to_process_checks.emplace(L"BoxColorInfo",      "BoxColorInfo");
    to_process_checks.emplace(L"CIDSystemInfo",     "CIDSystemInfo");
    to_process_checks.emplace(L"CMap",              "CMap");
    to_process_checks.emplace(L"ClassMap",          "ClassMap");
    // Portable Collections
    to_process_checks.emplace(L"Collection",        "Collection");
    to_process_checks.emplace(L"CollectionField",   "CollectionField");
    to_process_checks.emplace(L"CollectionItem",    "CollectionItem");
    to_process_checks.emplace(L"CollectionSchema",  "CollectionSchema");
    to_process_checks.emplace(L"CollectionSort",    "CollectionSort");
    to_process_checks.emplace(L"CollectionSubitem", "CollectionSubitem");
    to_process_checks.emplace(L"CryptFilter",       "CryptFilter");
    to_process_checks.emplace(L"DestXYZ",           "DestXYZ");
    to_process_checks.emplace(L"DocInfo",           "DocInfo");
    to_process_checks.emplace(L"EmbeddedFileParams","EmbeddedFileParameter");
    to_process_checks.emplace(L"EmbeddedFileStream","EmbeddedFileStream");
    to_process_checks.emplace(L"Encoding",          "Encoding");
    to_process_checks.emplace(L"3DExData",          "ExData3DMarkup");
    to_process_checks.emplace(L"ExData",            "ExData3DMarkup");
    to_process_checks.emplace(L"ExData",            "ExDataMarkupGeo");
    // ExDataProjection = new in PDF 2.0
    to_process_checks.emplace(L"FDDict",            "FDDict");
    // Fields
    to_process_checks.emplace(L"Field",             "Field");
    to_process_checks.emplace(L"FieldBtn",          "FieldBtn");
    to_process_checks.emplace(L"FieldCh",           "FieldCh");
    to_process_checks.emplace(L"FieldSig",          "FieldSig");
    to_process_checks.emplace(L"FieldTx",           "FieldTx");
    to_process_checks.emplace(L"Filespec",          "Filespecification");
    // Filter params
    to_process_checks.emplace(L"DCTDecodeParms",    "FilterDCTDecode");
    to_process_checks.emplace(L"CCITTFaxDecodeParms", "FilterCCITTFaxDecode");
    to_process_checks.emplace(L"JBIG2DecodeParms",  "FilterJBIG2Decode");
    to_process_checks.emplace(L"LZWDecodeParms",    "FilterLZWDecode");
    to_process_checks.emplace(L"CryptFilterDecodeParms", "FilterCrypt");
    //
    to_process_checks.emplace(L"FixedPrint",        "FixedPrint");
    to_process_checks.emplace(L"MarkInfo",          "MarkInfo");
    to_process_checks.emplace(L"MacSpecificFileInfo", "Mac");
    // Measurement
    to_process_checks.emplace(L"Measure",           "MeasureRL");
    to_process_checks.emplace(L"Measure",           "MeasureGEO");
    to_process_checks.emplace(L"MeasureRL",         "MeasureRL");
    // Media clips
    to_process_checks.emplace(L"MediaClip",         "MediaClipData");
    to_process_checks.emplace(L"MediaClip",         "MediaClipDataMHBE");
    to_process_checks.emplace(L"MediaClip",         "MediaClipSection");
    to_process_checks.emplace(L"MediaClip",         "MediaClipSectionMHBE");
    to_process_checks.emplace(L"MediaCriteria",     "MediaCriteria");
    to_process_checks.emplace(L"MediaDuration",     "MediaDuration");
    to_process_checks.emplace(L"MediaOffsetFrame",  "MediaOffsetFrame");
    to_process_checks.emplace(L"MediaOffsetMarker", "MediaOffsetMarker");
    to_process_checks.emplace(L"MediaOffsetTime",   "MediaOffsetTime");
    to_process_checks.emplace(L"MediaPermissions",  "MediaPermissions");
    to_process_checks.emplace(L"MediaPlayParams",   "MediaScreenParameters");
    to_process_checks.emplace(L"MediaPlayParams",   "MediaScreenParametersMHBE");
    to_process_checks.emplace(L"MediaPlayerInfo",   "MediaPlayerInfo");
    to_process_checks.emplace(L"MediaOffset",       "MediaOffsetFrame");
    to_process_checks.emplace(L"MediaOffset",       "MediaOffsetMarker");
    to_process_checks.emplace(L"MediaOffset",       "MediaOffsetTime");
    //
    to_process_checks.emplace(L"Movie",             "Movie");
    to_process_checks.emplace(L"MovieActivation",   "MovieActivation");
    to_process_checks.emplace(L"MinBitDepth",       "MinimumBitDepth");
    to_process_checks.emplace(L"MinScreenSize",     "MinimumScreenSize");
    to_process_checks.emplace(L"Metadata",          "Metadata");
    to_process_checks.emplace(L"NavNode",           "NavNode");
    to_process_checks.emplace(L"NumberFormat",      "NumberFormat");
    // Optional content
    to_process_checks.emplace(L"OCGorOCMD",         "OptContentGroup");
    to_process_checks.emplace(L"OCGorOCMD",         "OptContentMembership");
    to_process_checks.emplace(L"OCG",               "OptContentGroup");
    to_process_checks.emplace(L"OCMD",              "OptContentMembership");
    to_process_checks.emplace(L"OCConfig",          "OptContentConfig");
    to_process_checks.emplace(L"OCCreatorInfo",     "OptContentCreatorInfo");
    to_process_checks.emplace(L"OCExport",          "OptContentExport");
    to_process_checks.emplace(L"OCLanguage",        "OptContentLanguage");
    to_process_checks.emplace(L"OCPageElement",     "OptContentPageElement");
    to_process_checks.emplace(L"OCPrint",           "OptContentPrint");
    to_process_checks.emplace(L"OCProperties",      "OptContentProperties");
    to_process_checks.emplace(L"OCUsage",           "OptContentUsage");
    to_process_checks.emplace(L"OCUsageApplication","OptContentUsageApplication");
    to_process_checks.emplace(L"OCUser",            "OptContentUser");
    to_process_checks.emplace(L"OCView",            "OptContentView");
    to_process_checks.emplace(L"OCZoom",            "OptContentZoom");
    // OPI
    to_process_checks.emplace(L"OPI1.3 ",           "OPIVersion13");
    to_process_checks.emplace(L"OPI1.3",            "OPIVersion13Dict");
    to_process_checks.emplace(L"OPI2.0",            "OPIVersion20");
    to_process_checks.emplace(L"OPI2.0",            "OPIVersion20Dict");
    //
    to_process_checks.emplace(L"OutputIntents",     "OutputIntents");
    to_process_checks.emplace(L"PageLabel",         "PageLabel");
    to_process_checks.emplace(L"PagePieceDict",     "PagePiece");
    to_process_checks.emplace(L"Perms",             "Permissions");
    // Logical structure
    to_process_checks.emplace(L"StructTreeRoot",    "StructTreeRoot");
    to_process_checks.emplace(L"StructElem",        "StructElem");
    to_process_checks.emplace(L"RoleMap",           "RoleMap");
    to_process_checks.emplace(L"ObjStm",            "ObjectStream");
    to_process_checks.emplace(L"LayoutAttributes",  "StandardLayoutAttributesBLSE");
    to_process_checks.emplace(L"LayoutAttributes",  "StandardLayoutAttributesILSE");
    to_process_checks.emplace(L"LayoutAttributes",  "StandardLayoutAttributesColumn");
    to_process_checks.emplace(L"ListAttributes",    "StandardListAttributes");
    to_process_checks.emplace(L"TableAttributes",   "StandardTableAttributes");
    //
    to_process_checks.emplace(L"Stream",            "Stream");
    to_process_checks.emplace(L"SlideShow",         "SlideShow");
    to_process_checks.emplace(L"SoftMaskLuminosity","SoftMaskLuminosity");
    to_process_checks.emplace(L"SoftwareIdentifier","SoftwareIdentifier");
    to_process_checks.emplace(L"Sound",             "SoundObject");
    to_process_checks.emplace(L"SourceInfo",        "SourceInformation");
    // Shadings
    to_process_checks.emplace(L"ShadingType1",      "ShadingType1");
    to_process_checks.emplace(L"ShadingType2",      "ShadingType2");
    to_process_checks.emplace(L"ShadingType3",      "ShadingType3");
    to_process_checks.emplace(L"ShadingType4",      "ShadingType4");
    to_process_checks.emplace(L"ShadingType5",      "ShadingType5");
    to_process_checks.emplace(L"ShadingType6",      "ShadingType6");
    to_process_checks.emplace(L"ShadingType7",      "ShadingType7");
    // Sig Ref.
    to_process_checks.emplace(L"SigRefDocMDP",      "SignatureReferenceDocMDP");
    to_process_checks.emplace(L"SigRefFieldMDP",    "SignatureReferenceFieldMDP");
    to_process_checks.emplace(L"SigRefIdentity",    "SignatureReferenceIdentity");
    to_process_checks.emplace(L"SigRefUR",          "SignatureReferenceUR");
    //
    to_process_checks.emplace(L"SubjectDN",         "SubjectDN");
    to_process_checks.emplace(L"Target",            "Target");
    to_process_checks.emplace(L"Thumbnail",         "Thumbnail");
    to_process_checks.emplace(L"Timespan",          "Timespan");
    to_process_checks.emplace(L"TimeStamp",         "TimeStampDict");
    to_process_checks.emplace(L"UserProperty",      "UserProperty");
    // UR
    to_process_checks.emplace(L"URParamAnnotsArray","URTransformParamAnnotsArray");
    to_process_checks.emplace(L"URParamDocArray",   "URTransformParamDocumentArray");
    to_process_checks.emplace(L"URParamEFArray",    "URTransformParamEFArray");
    to_process_checks.emplace(L"URParamFormArray",  "URTransformParamFormArray");
    to_process_checks.emplace(L"URParamSigArray",   "URTransformParamSignatureArray");
    //
    to_process_checks.emplace(L"ViewPort",          "Viewport");
    to_process_checks.emplace(L"ViewerPreferences", "ViewerPreferences");
    to_process_checks.emplace(L"WebCaptureCommand", "WebCaptureCommand");
    to_process_checks.emplace(L"WebCaptureCommandSettings", "WebCaptureCommandSettings");

    to_process_elem elem;

    while (!to_process_checks.empty()) {
        elem = to_process_checks.front();
        if (ArlingtonPDFShim::debugging && !terse) {
            ofs << "Processing DVA " << ToUtf8(elem.dva_link) << " vs Arlington '" << elem.link << "'" << std::endl;
        }
        to_process_checks.pop();
        if ((elem.link == "") || (elem.dva_link.empty()))
            continue;

        // Separate multiple mappings to Arlington
        elem.link = remove_link_predicates(elem.link);
        std::vector<std::string> links = split(elem.link, ',');
        if (links.size() > 1) {
            for (auto lnk : links) {
                to_process_checks.emplace(elem.dva_link, lnk);
            }
            continue;  // cannot fallthrough as elem.link is a list...
        }

        auto found = std::find_if(mapped.begin(), mapped.end(), 
            [&elem](const to_process_elem& a) { return (a.link == elem.link) && (a.dva_link == elem.dva_link); });

        if (found != mapped.end()) {
            // ofs << "Arlington " << elem.link << " with DVA " << ToUtf8(elem.dva_link) << " already processed" << std::endl;
            continue;
        }

        // Add the current Arlingon / DVA pair to the "already done" vector
        mapped.push_back(to_process_elem(elem.dva_link, elem.link));

        // load Arlington definition (TSV file)
        std::unique_ptr<CArlingtonTSVGrammarFile> reader(new CArlingtonTSVGrammarFile(tsv_dir / (elem.link + ".tsv")));
        if (!reader->load()) {
            ofs << COLOR_ERROR << "loading Arlington TSV file " << (tsv_dir / (elem.link + ".tsv")) << COLOR_RESET;
            continue;
        }

        // locate dict in DVA
#ifdef DVA_TRACING
        ofs << "Reading DVA object " << ToUtf8(elem.dva_link) << std::endl;
#endif
        ArlPDFDictionary* dict = (ArlPDFDictionary*)map_dict->get_value(elem.dva_link);
        if (dict == nullptr) {
            ofs << COLOR_ERROR << "Adobe DVA problem (dictionary not found): " << ToUtf8(elem.dva_link) << COLOR_RESET;
            continue;
        }
        assert(dict->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary);

        const ArlTSVmatrix* data_list = &reader->get_data();
        ofs << std::endl << COLOR_INFO << count++ << ": Comparing Arlington:" << elem.link << " vs DVA:" << ToUtf8(elem.dva_link) << COLOR_RESET;

        // what Arlington has but Adobe DVA doesn't
        for (auto& vec : *data_list) {
            ArlPDFDictionary* inner_obj = nullptr;

            // Arlington wildcard key name or array elements
            /// @todo support repeating array index sets in Arlington
            if (vec[TSV_KEYNAME] == "*") {
                ArlPDFObject* tmp_obj = dict->get_value(L"GenericKey");
                if (tmp_obj != nullptr) {
                    switch (tmp_obj->get_object_type()) {
                        case PDFObjectType::ArlPDFObjTypeDictionary:
                            {
                                inner_obj = (ArlPDFDictionary*)tmp_obj;
                                if (inner_obj == nullptr) {
                                    ArlPDFArray* inner_array = (ArlPDFArray*)dict->get_value(L"Array");
                                    if ((inner_array == nullptr) || (inner_array->get_num_elements() != 1)) {
                                        ofs << COLOR_ERROR << "Arlington wildcard key vs DVA Array - either not linked or multiple links: " << ToUtf8(elem.dva_link) << "/" << vec[TSV_KEYNAME] << COLOR_RESET;
                                    }
                                    else {
                                        tmp_obj = inner_array->get_value(0);
                                        if ((tmp_obj != nullptr) && (tmp_obj->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary)) {
                                            inner_obj = (ArlPDFDictionary*)tmp_obj;
                                        }
                                        else {
                                            ofs << COLOR_ERROR << "Adobe DVA " << ToUtf8(elem.dva_link) <<"/GenericKey/Array[0] entry was not a dictionary" << COLOR_RESET;
                                        }
                                    }
                                    delete inner_array;
                                }
                            }
                            break;
                        default: 
                            ofs << COLOR_ERROR << "Adobe DVA GenericKey dictionary expected but different object type found" << COLOR_RESET;
                            break;
                    } // switch
                }
                else if (!dict->has_key(L"Array")) {
                    ofs << "Arlington wildcard in " << elem.link << " did not have matching GenericKey or Array entry in DVA: " << ToUtf8(elem.dva_link) << std::endl;
                }
            } else {
                ArlPDFObject* tmp_obj = dict->get_value(ToWString(vec[TSV_KEYNAME]));
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
#ifdef DVA_TRACING
                        ofs << "Reading DVA object " << ToUtf8(new_dva_value) << std::endl;
#endif                        
                        ArlPDFDictionary *d = (ArlPDFDictionary*)map_dict->get_value(new_dva_value);
                        if (d != nullptr) {
                            inner_obj = (ArlPDFDictionary*)d->get_value(ToWString(vec[TSV_KEYNAME]));
                        }
                        else {
                            ofs << COLOR_ERROR << "DVA ConcatWithFormalReps target missing for " << ToUtf8(new_dva_value) << " - " << ToUtf8(elem.dva_link) << COLOR_RESET;
                        }
                        delete d;
                    }
                    delete o;
                }
            }

            if (inner_obj == nullptr) {
                // Avoid reporting all the PDF 2.0 new stuff, wildcards and DVA arrays...
                if ((vec[TSV_SINCEVERSION] != "2.0") && (vec[TSV_KEYNAME] != "*") &&
                    (ToUtf8(elem.dva_link).find("Array") == std::string::npos)) {
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
                        ofs << COLOR_ERROR << "DVA MustBeIndirect is not a Boolean " << ToUtf8(elem.dva_link) << COLOR_RESET;
                    }
                    delete indr;
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
                        ArlPDFBoolean* req_b = (ArlPDFBoolean*)req;
                        if (req_b->get_value())
                            required = "TRUE";
                        if (vec[TSV_REQUIRED] != required) {
                            // Suppress if terse and there are predicates
                            if (!terse || (vec[TSV_REQUIRED].find("fn:") == std::string::npos)) {
                                ofs << "Required is different DVA: " << ToUtf8(elem.dva_link) << "/" << vec[TSV_KEYNAME] << "==" << required;
                                ofs << " vs Arlington: " << elem.link << "/" << vec[TSV_KEYNAME] << "==" << vec[TSV_REQUIRED] << std::endl;
                            }
                        }
                    }
                    else {
                        ofs << COLOR_ERROR << "DVA Required is not a Boolean " << ToUtf8(elem.dva_link) << COLOR_RESET;
                    }
                    delete req;
                }
                else {
                    ofs << COLOR_ERROR << "DVA does not specify Required for " << ToUtf8(elem.dva_link) << COLOR_RESET;
                }

                // Arlington SinceVersion (1.0, 1.1, ..., 2.0)
                // Linux CLI: cut -f 3 *.tsv | sort | uniq
                if (inner_obj->has_key(L"PDFMajorVersion") && inner_obj->has_key(L"PDFMinorVersion")) {
                    ArlPDFObject* major = inner_obj->get_value(L"PDFMajorVersion");
                    ArlPDFObject* minor = inner_obj->get_value(L"PDFMinorVersion");
                    if ((major != nullptr) && (minor != nullptr) &&
                        (major->get_object_type() == PDFObjectType::ArlPDFObjTypeNumber) &&
                        (minor->get_object_type() == PDFObjectType::ArlPDFObjTypeNumber)) {
                        int   pdf_major = ((ArlPDFNumber*)major)->get_integer_value();
                        int   pdf_minor = ((ArlPDFNumber*)minor)->get_integer_value();
                        std::string ver = std::to_string(pdf_major) + "." + std::to_string(pdf_minor);
                        if (ver != vec[TSV_SINCEVERSION]) {
                            ofs << "SinceVersion is different in DVA: " << ToUtf8(elem.dva_link) << " (" << ver << ")";
                            ofs << " vs Arlington (" << vec[TSV_SINCEVERSION] << ") for " << elem.link << "/" << vec[TSV_KEYNAME] << std::endl;
                        }
                    }
                    else {
                        ofs << COLOR_ERROR << "DVA PDFMajorVersion/PDFMinorVersion is invalid for " << ToUtf8(elem.dva_link) << COLOR_RESET;
                    }
                    delete major, minor;
                }

                // Check allowed Types
                {
                    ArlPDFObject *vt = inner_obj->get_value(L"ValueType");
                    if (vt == nullptr) {
                        ofs << COLOR_ERROR << "No ValueType defined for DVA: " << ToUtf8(elem.dva_link) << "/" << vec[TSV_KEYNAME] << COLOR_RESET;
                    }
                    else if (vt->get_object_type() != PDFObjectType::ArlPDFObjTypeArray) {
                        ofs << COLOR_ERROR << "ValueType is not an array for DVA: " << ToUtf8(elem.dva_link) << "/" << vec[TSV_KEYNAME] << COLOR_RESET;
                    }
                    else {
                        ArlPDFArray* types_array = (ArlPDFArray*)vt;
                        std::vector<std::string>    types_our = split(vec[TSV_TYPE], ';');

                        // Map broader set of Arlington types (always lowercase) to Adobe DVA types ("CosXxxx")
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

                        // Adobe DVA types are always stored as names
                        std::vector<std::string>    types_dva;
                        for (int i = 0; i < types_array->get_num_elements(); i++) {
                            std::wstring    new_dva_value;
                            ArlPDFObject* obj = types_array->get_value(i);
                            if ((obj != nullptr) && (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeName)) {
                                ArlPDFName* nm = (ArlPDFName*)obj;
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
                                ofs << COLOR_ERROR << "DVA ValueType array element is not a name object!" << COLOR_RESET;
                            }
                            delete obj;
                        } // for

                        std::string head = "==Key type difference - DVA: " + ToUtf8(elem.dva_link) + " vs Arlington: " + elem.link + "/" + vec[TSV_KEYNAME] + "\n";
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
                    delete vt;
                }

// If this is enabled, then many additional DVA/Arlington pairings get compared which is very noisy!
#if 0
                /// @todo Process DVA SpecialCases sub-dictionary:
                //  - /DictToSwitchTo : array of other DVA objects (names), based on /SwitchOnValue array of names
                //  - /SwitchOnValue : array of names needed by /DictToSwitchTo
                //  - /StreamFilters dict : ???
                //  - /RequiredIfOtherKeysAbsent : array of names
                //  - /OtherKeysMustBePresent : array of names
                //  - /SpecialProcessing : ???
                //  - /MayBeInheritedFromDictAtKey : ??? name
                //  - /MayBeInheritedFromDictAtPaths : ??? array of arrays
                if (inner_obj != nullptr) {
                    ArlPDFDictionary* special_cases = (ArlPDFDictionary*)inner_obj->get_value(L"SpecialCases");
                    if (special_cases != nullptr) {
                        assert(special_cases->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary);
                        ArlPDFArray* switch_to = (ArlPDFArray*)special_cases->get_value(L"DictToSwitchTo");
                        ArlPDFArray* switch_on = (ArlPDFArray*)special_cases->get_value(L"SwitchOnValue");
                        if ((switch_to != nullptr) && (switch_on != nullptr)) {
                            assert(switch_to->get_object_type() == PDFObjectType::ArlPDFObjTypeArray);
                            assert(switch_on->get_object_type() == PDFObjectType::ArlPDFObjTypeArray);
                            assert(switch_on->get_num_elements() == switch_to->get_num_elements());

                            // Loop for each DictToSwitchTo reference and add
                            for (int i = 0; i < switch_to->get_num_elements(); i++) {
                                ArlPDFName*   dva_link = (ArlPDFName*)switch_to->get_value(i);
                                assert(dva_link->get_object_type() == PDFObjectType::ArlPDFObjTypeName);
                                std::wstring dva_link_s = dva_link->get_value();
                                // ofs << "DVA special-cased to DVA " << ToUtf8(dva_link_s) << " for " << vec[TSV_KEYNAME] << " to Arlington base " << elem.link << std::endl;
                                if (vec[TSV_LINK] != "") {
                                    std::vector<std::string> all_links;
                                    std::vector<std::string> typed_links = split(remove_link_predicates(vec[TSV_LINK]), ';');
                                    // Loop for each Arlington link and combine into one big list
                                    for (std::string lnks : typed_links) {
                                        if (lnks != "[]") {
                                            lnks = lnks.substr(1, lnks.size() - 2);  // strip off [ and ] 
                                            std::vector<std::string> lnk = split(lnks, ',');
                                            for (std::string& s : lnk) {
                                                all_links.push_back(s);
                                            }
                                        }
                                    } // for

                                    auto exact = std::find(all_links.begin(), all_links.end(), ToUtf8(dva_link_s));
                                    if (exact != all_links.end()) {
                                        // DVA and Arlington names match precisely 
                                        // ofs << "DVA special-cased to DVA " << ToUtf8(dva_link_s) << " for " << vec[TSV_KEYNAME] << " to Arlington " << *exact << std::endl;
                                        to_process_checks.emplace(dva_link_s, *exact);
                                    }
                                    else {
                                        // No exact match so propogate new DVA to all Arlington links...
                                        to_process_checks.emplace(dva_link_s, elem.link);
                                        for (std::string& s : all_links) {
                                            // ofs << "DVA special-cased to DVA " << ToUtf8(dva_link_s) << " for " << vec[TSV_KEYNAME] << " to Arlington " << s << std::endl;
                                            to_process_checks.emplace(dva_link_s, s);
                                        }
                                    }
                                }
                                delete dva_link;
                            } // for
                        }
                        delete switch_on, switch_to;
                    }
                    delete special_cases;
                }
#endif

                // Check Arlington PossibleValue field vs DVA Bounds
                {
                    ArlPDFDictionary* bounds_dict = (ArlPDFDictionary*)inner_obj->get_value(L"Bounds");
                    if ((bounds_dict != nullptr) && (bounds_dict->get_object_type() != PDFObjectType::ArlPDFObjTypeDictionary)) {
                        ofs << COLOR_ERROR << "Bounds is not a dictionary for DVA: " << ToUtf8(elem.dva_link) << "/" << vec[TSV_KEYNAME] << COLOR_RESET;
                        delete bounds_dict;
                    }
                    else if ((vec[TSV_POSSIBLEVALUES] != "") && (!terse)) {
                        std::string possible = "";

                        if (bounds_dict == nullptr) {
                            ofs << "Bounds not defined in DVA for " << ToUtf8(elem.dva_link) << ", but PossibleValue is defined in Arlington: ";
                            ofs << elem.link << "/" << vec[TSV_KEYNAME] << "==" << vec[TSV_POSSIBLEVALUES] << std::endl;
                        }
                        else {
                            ArlPDFArray* possible_array = (ArlPDFArray*)bounds_dict->get_value(L"Equals");
                            if ((possible_array != nullptr) && (possible_array->get_object_type() == PDFObjectType::ArlPDFObjTypeArray)) {
                                std::vector<std::string>    possible_dva;

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

                                ArlPDFObject* obj;
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
                                            ofs << COLOR_ERROR << "DVA Bounds/Equals[" << i << "] was an unexpected type for " << ToUtf8(elem.dva_link) << COLOR_RESET;
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
                                    }
                                    else {
                                        ofs << COLOR_ERROR << "DVA Bounds/Equals[" << i << "] was a null object for " << ToUtf8(elem.dva_link) << COLOR_RESET;
                                    }// if
                                    delete obj;
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
                            else if (possible_array != nullptr) {
                                ofs << COLOR_ERROR << "DVA Bounds/Equals was not an array for " << ToUtf8(elem.dva_link) << COLOR_RESET;
                                delete possible_array;
                            }
                        }
                    }
                    delete bounds_dict;
                }
            }

            {
                ArlPDFObject* link_obj = inner_obj->get_value(L"VerifyAtFormalRep"); // Array: 0-dict, 1-stream, 2-array

                ArlPDFObject* recursion_obj = inner_obj->get_value(L"RecursionParams");
                if ((recursion_obj != nullptr) && (recursion_obj->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary)) {
                    delete link_obj;
                    link_obj = ((ArlPDFDictionary*)recursion_obj)->get_value(L"VerifyAtFormalRep"); // 0-dict, 1-stream, 2-array
                }
                delete recursion_obj;
                recursion_obj = nullptr;

                if (link_obj != nullptr) {
                    switch (link_obj->get_object_type()) {
                        case PDFObjectType::ArlPDFObjTypeName:
                        {
                            std::wstring  dva_link_value = ((ArlPDFName*)link_obj)->get_value();
                            if ((vec[TSV_LINK] == "") && !((vec[TSV_TYPE] == "rectangle") || (vec[TSV_TYPE] == "matrix"))) {
                                ofs << "No link in Arlington for " << elem.link << "/" << vec[TSV_KEYNAME] << " (" << vec[TSV_TYPE] << ")" << std::endl;
                            }
                            else if (vec[TSV_LINK] != "") {
                                std::vector<std::string> typed_links = split(vec[TSV_LINK], ';');
                                for (std::string lnks : typed_links) {
                                    if (lnks != "[]") {
                                        lnks = lnks.substr(1, lnks.size() - 2);  // strip off [ and ] 
                                        std::vector<std::string> lnk = split(remove_link_predicates(lnks), ',');
                                        for (std::string s : lnk) {
                                            to_process_checks.emplace(dva_link_value, s);
                                        }
                                    }
                                }
                            }
                        }
                        break;
                    case PDFObjectType::ArlPDFObjTypeArray:
                    {
                        std::wstring  dva_dict_value = L"";
                        std::wstring  dva_stream_value = L"";
                        std::wstring  dva_array_value = L"";
                        ArlPDFArray* arr = (ArlPDFArray*)link_obj;
                        ArlPDFObject* obj;
                        ArlPDFName* nm;

                        // dictionary
                        obj = arr->get_value(0);
                        if ((obj != nullptr) && (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeName)) {
                            nm = (ArlPDFName*)obj;
                            dva_dict_value = nm->get_value();
                        }
                        delete obj;

                        // stream
                        obj = arr->get_value(1);
                        if ((obj != nullptr) && (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeName)) {
                            nm = (ArlPDFName*)obj;
                            dva_stream_value = nm->get_value();
                        }
                        delete obj;

                        // array
                        obj = arr->get_value(2);
                        if ((obj != nullptr) && (obj->get_object_type() == PDFObjectType::ArlPDFObjTypeName)) {
                            nm = (ArlPDFName*)obj;
                            dva_array_value = nm->get_value();
                        }
                        delete obj;

                        // get_link_for_type() KEEPS predicates and does NOT split for multiple link values!
                        std::string lnk_dict   = get_link_for_type("dictionary", vec[TSV_TYPE], vec[TSV_LINK]);
                        std::string lnk_stream = get_link_for_type("stream", vec[TSV_TYPE], vec[TSV_LINK]);
                        std::string lnk_array  = get_link_for_type("array", vec[TSV_TYPE], vec[TSV_LINK]);

                        if ((dva_dict_value != L"") && (lnk_dict != "[]") && (lnk_dict != "")) {
                            // Both DVA and Arlington have links for dictionaries
                            to_process_checks.emplace(dva_dict_value, lnk_dict.substr(1, lnk_dict.size() - 2));
                        }
                        else if (dva_dict_value != L"") {
                            ofs << COLOR_WARNING << "DVA had dictionary links for " << ToUtf8(elem.dva_link) << "/" << vec[TSV_KEYNAME] << " but Arlington did not for " << elem.link << "/" << vec[TSV_KEYNAME] << COLOR_RESET;
                        }
                        else {
                            ofs << "Arlington had dictionary links for " << elem.link << "/" << vec[TSV_KEYNAME] << " but DVA did not for " << ToUtf8(elem.dva_link) << "/" << vec[TSV_KEYNAME] << COLOR_RESET;
                        }

                        if ((dva_stream_value != L"") && (lnk_stream != "[]")) {
                            // Both DVA and Arlington have links for streams
                            to_process_checks.emplace(dva_stream_value, lnk_stream.substr(1, lnk_stream.size() - 2));
                        }
                        else if (dva_stream_value != L"") {
                            ofs << COLOR_WARNING << "DVA had stream links for " << ToUtf8(elem.dva_link) << "/" << vec[TSV_KEYNAME] << " but Arlington did not for " << elem.link << "/" << vec[TSV_KEYNAME] << COLOR_RESET;
                        }
                        else {
                            ofs << "Arlington had stream links for " << elem.link << "/" << vec[TSV_KEYNAME] << " but DVA did not for " << ToUtf8(elem.dva_link) << "/" << vec[TSV_KEYNAME] << COLOR_RESET;
                        }

                        if ((dva_array_value != L"") && (lnk_array != "[]")) {
                            // Both DVA and Arlington have links for arrays
                            to_process_checks.emplace(dva_array_value, lnk_array.substr(1, lnk_array.size() - 2));
                        }
                        else if (dva_array_value != L"") {
                            ofs << COLOR_WARNING << "DVA had array links for " << ToUtf8(elem.dva_link) << "/" << vec[TSV_KEYNAME] << " but Arlington did not for " << elem.link << "/" << vec[TSV_KEYNAME] << COLOR_RESET;
                        }
                        else {
                            ofs << "Arlington had array links for " << elem.link << "/" << vec[TSV_KEYNAME] << " but DVA did not for " << ToUtf8(elem.dva_link) << "/" << vec[TSV_KEYNAME] << COLOR_RESET;
                        }
                    }
                    break;
                    default:
                        ofs << COLOR_ERROR << "Unexpected DVA type for VerifyAtFormalRep!" << COLOR_RESET;
                        break;
                    } // switch
                } // if
                delete link_obj;
            }
            delete inner_obj;
        } // for arlington


        // @brief Checks if a key name exists in Arlington
        // @param[in] key key name (string)
        // @returns true if key exists in Arlington
        auto exists_in_our = [=](auto key) {
            for (auto& vec : *data_list)
                if (vec[TSV_KEYNAME] == ToUtf8(key))
                    return true;
                else if (vec[TSV_KEYNAME] == "*") // wildcard
                    return true;
            return false;
        }; // auto


        // @brief Iterates through all keys in a DVA PDF dictionary to see if they are in Arlington
        // @param[in] a_dict   the DVA PDF dictionary
        // @param[in] in_ofs   report stream
        auto check_dict = [=](ArlPDFDictionary* a_dict, std::ostream& in_ofs) {
            for (int i = 0; i < (a_dict->get_num_keys()); i++) {
                std::wstring key = a_dict->get_key_name_by_index(i);
                if (!exists_in_our(key) && (key != L"FormalRepOf") && (key != L"Array")
                    && (key != L"ArrayStyle") && (key != L"FormalRepOfArray") && (key != L"OR")
                    && (key != L"GenericKey") && (key != L"ConcatWithFormalReps")
                    && (key != L"Metadata") && (key != L"AF")) // keys in PDF 2.0 allowed anywhere
                {
                    in_ofs << "Missing key in Arlington: " << elem.link << "/" << ToUtf8(key) << std::endl;
                }
            }
        }; // auto

        check_dict(dict, ofs);

        {
            ArlPDFObject* tmp_obj = dict->get_value(L"ConcatWithFormalReps");
            if ((tmp_obj != nullptr) && (tmp_obj->get_object_type() == PDFObjectType::ArlPDFObjTypeArray)) {
                ArlPDFArray* inner_array = (ArlPDFArray*)tmp_obj;
                ArlPDFObject* obj = inner_array->get_value(0);
                if (obj != nullptr) {
                    switch (obj->get_object_type()) {
                        case PDFObjectType::ArlPDFObjTypeString:
                            {
                                ArlPDFString* concat = (ArlPDFString*)obj;
                                std::wstring  new_dva_value = concat->get_value();
#ifdef DVA_TRACING
                                ofs << "Reading DVA object " << ToUtf8(new_dva_value) << std::endl;
#endif                             
                                ArlPDFObject* new_dva = map_dict->get_value(new_dva_value);
                                if ((new_dva != nullptr) && (new_dva->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary)) {
                                    check_dict((ArlPDFDictionary*)new_dva, ofs);
                                }
                                else {
                                    ofs << COLOR_ERROR << "DVA " << ToUtf8(elem.dva_link) << " ConcatWithFormalReps[0]/(" << ToUtf8(concat->get_value()) << ") string was not a dictionary" << COLOR_RESET;
                                }
                                delete new_dva;
                            }
                        break;
                        case PDFObjectType::ArlPDFObjTypeName:
                            {
                                ArlPDFName* concat = (ArlPDFName*)obj;
                                std::wstring  new_dva_value = concat->get_value();
#ifdef DVA_TRACING
                                ofs << "Reading DVA object " << ToUtf8(new_dva_value) << std::endl;
#endif             
                                ArlPDFObject* new_dva = map_dict->get_value(new_dva_value);
                                if ((new_dva != nullptr) && (new_dva->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary)) {
                                    check_dict((ArlPDFDictionary*)new_dva, ofs);
                                }
                                else {
                                    ofs << COLOR_ERROR << "DVA " << ToUtf8(elem.dva_link) << " ConcatWithFormalReps[0]/" << ToUtf8(concat->get_value()) << " name was not a dictionary" << COLOR_RESET;
                                }
                                delete new_dva;
                            }
                            break;
                        default:
                            ofs << COLOR_ERROR << "DVA " << ToUtf8(elem.dva_link) << " ConcatWithFormalReps[0] was an unexpected type" << COLOR_RESET;
                            break;
                    } // switch
                }
                else {
                    ofs << COLOR_ERROR << "DVA " << ToUtf8(elem.dva_link) << " ConcatWithFormalReps[0] did not exist" << COLOR_RESET;
                }
                delete obj;
            }
            delete tmp_obj;
        }

        delete dict;
    } // while

    // Report all Adobe DVA key names that were NOT compared
    if (!terse)
    {
        ofs << std::endl;
        const int dva_num_keys = map_dict->get_num_keys();

        for (int i = 0; i < dva_num_keys; i++) {
            std::wstring key = map_dict->get_key_name_by_index(i);

            auto result = std::find_if(mapped.begin(), mapped.end(), 
                            [&key](to_process_elem a) { return (a.dva_link == key); });

            if (result == mapped.end()) {
                // Candidate DVA key not found - now exclude operators, operands, etc.
                // Look for key "FormalRepOf" in the dictionary
                ArlPDFObject* obj = map_dict->get_value(key);
                assert(obj != nullptr);
                assert(obj->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary);
                if (((ArlPDFDictionary*)obj)->has_key(L"FormalRepOf")) {
                    ofs << COLOR_WARNING << "Adobe DVA comaparison did not check DVA key " << ToUtf8(key) << COLOR_RESET;
                }
                delete obj;
            }
        }
    }

    // Report all Arlington TSV files that were NOT compared
    // This will include all PDF 2.0 stuff.
    // Iterate across all physical files in the TSV folder to list anything that exists but was not referenced
    if (!terse)
    {
        ofs << std::endl;
        for (const auto& entry : fs::directory_iterator(tsv_dir)) {
            if (entry.is_regular_file() && (entry.path().extension().string() == ".tsv")) {
                const auto tsv = entry.path().stem().string();

                auto result = std::find_if(mapped.begin(), mapped.end(), 
                    [&tsv](const to_process_elem& a) { return (a.link == tsv); });

                if (result == mapped.end()) {
                    ofs << COLOR_WARNING << "Adobe DVA comaparison did not check Arlington " << tsv << COLOR_RESET;
                }
            }
        } // for
    }
}


/// @brief  Compares Arlington TSV file set against Adobe DVA formal representation PDF
/// 
/// @param[in] pdfsdk          already instantiated PDF SDK Arlington shim object
/// @param[in] dva_file        the Adobe DVA PDF file with the FormalRep tree
/// @param[in] grammar_folder  the Arlington PDF model folder with TSV file set
/// @param[in] ofs             report stream
void CheckDVA(ArlingtonPDFShim::ArlingtonPDFSDK &pdfsdk, const std::filesystem::path& dva_file, const fs::path& grammar_folder, std::ostream& ofs, bool terse) {
    try {
        ofs << "BEGIN - Arlington vs Adobe DVA Report - TestGrammar " << TestGrammar_VERSION << " " << pdfsdk.get_version_string() << std::endl;
        ofs << "Arlington TSV data: " << fs::absolute(grammar_folder).lexically_normal() << std::endl;
        ofs << "Adobe DVA FormalRep file: " << fs::absolute(dva_file).lexically_normal() << std::endl;

        ArlPDFTrailer* trailer = pdfsdk.get_trailer(dva_file.wstring());
        if (trailer != nullptr) {
            ArlPDFObject* root = trailer->get_value(L"Root");
            if ((root != nullptr) && (root->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary)) {
                // Adobe DVA COS object tree starts at DocCat::FormalRepTree
                ArlPDFObject* formal_rep = ((ArlPDFDictionary *)root)->get_value(L"FormalRepTree");
                if ((formal_rep != nullptr) && (formal_rep->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary)) {
                    ArlPDFDictionary* formal_rep_dict = (ArlPDFDictionary*)formal_rep;
                    process_dict(grammar_folder, ofs, formal_rep_dict, terse);
                    delete formal_rep;
                }
                else {
                    ofs << COLOR_ERROR << "failed to acquire Trailer/Root/FormalRepTree" << COLOR_RESET;
                }
            }
            else {
                ofs << COLOR_ERROR << "failed to acquire Trailer/Root" << COLOR_RESET;
            }
            delete root;
        }
        else {
            ofs << COLOR_ERROR << "failed to acquire Trailer" << COLOR_RESET;
        }
        delete trailer;
    }
    catch (std::exception& ex) {
        ofs << COLOR_ERROR << "EXCEPTION: " << ex.what() << COLOR_RESET;
    }

    // Finally...
    ofs << "END" << std::endl;
}
