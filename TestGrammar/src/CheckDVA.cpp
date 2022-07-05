///////////////////////////////////////////////////////////////////////////////
/// @file 
/// @brief Compares an Arlington PDF model to the Adobe DVA FormalRep 
/// 
/// - Output always has DVA first then Arlington
/// - Output uses "+" when combining multiple Adobe DVA objects
/// - Output uses "/" as the Arlington/DVA dictionary name and key separator
/// - Adobe DVA logic is hard coded with mostly COLOR_ERROR messages but some asserts()  
///   in case of unexpected errors.
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
#include <vector>
#include <cassert>

using namespace ArlingtonPDFShim;
namespace fs = std::filesystem;


/// @brief define DVA_TRACING for verbose output about which DVA objects are getting read
#undef DVA_TRACING


/// @brief Class for storing Adobe DVA object and Arlington TSV tuples.
/// Note that Adobe DVA often uses 2 PDF objects (base object and specialized object)
/// to represent what is in a single Arlington TSV file.
/// dva[0] should always be valid 
class CDVAArlingtonTuple {
public:
    std::vector<std::wstring>   dva;    // potentially multiple Adobe DVA objects (PDF key names so wstring)
    std::string                 link;   // Arlington TSV filename

    CDVAArlingtonTuple()
        { /* default (empty) constructor */ };

    CDVAArlingtonTuple(const std::vector<std::wstring>& dva_vec, const std::string our_lnk)
        : link(our_lnk)
        { /* vector-based constructor */ 
            for (auto d : dva_vec) {
                dva.push_back(d);
            }
        };

    CDVAArlingtonTuple(const std::wstring dva_lnk, const std::string our_lnk)
        : link(our_lnk)
        { /* 2-arg constructor */ dva.push_back(dva_lnk); };

    CDVAArlingtonTuple(const std::wstring dva_lnk1, const std::wstring dva_lnk2, const std::string our_lnk)
        : link(our_lnk)
        { /* 3-arg constructor */ dva.push_back(dva_lnk1); dva.push_back(dva_lnk2); };

    CDVAArlingtonTuple(const std::wstring dva_lnk1, const std::wstring dva_lnk2, const std::wstring dva_lnk3, const std::string our_lnk)
        : link(our_lnk)
        { /* 4-arg constructor */ dva.push_back(dva_lnk1); dva.push_back(dva_lnk2); dva.push_back(dva_lnk3); };

    CDVAArlingtonTuple(const std::wstring dva_lnk1, const std::wstring dva_lnk2, const std::wstring dva_lnk3, const std::wstring dva_lnk4, const std::string our_lnk)
        : link(our_lnk)
        { /* 5-arg constructor */ dva.push_back(dva_lnk1); dva.push_back(dva_lnk2); dva.push_back(dva_lnk3); dva.push_back(dva_lnk4); };

    CDVAArlingtonTuple(const std::wstring dva_lnk1, const std::wstring dva_lnk2, const std::wstring dva_lnk3, const std::wstring dva_lnk4, const std::wstring dva_lnk5, const std::string our_lnk)
        : link(our_lnk)
        { /* 6-arg constructor */ dva.push_back(dva_lnk1); dva.push_back(dva_lnk2); dva.push_back(dva_lnk3); dva.push_back(dva_lnk4); dva.push_back(dva_lnk5); };

    /// @brief returns true if key is in the vector of DVA keys
    bool contains_DVA_key(const std::wstring& key) {
        assert(!dva.empty());
        for (size_t i = 0; i < dva.size(); i++) {
            if (dva[i] == key)
                return true;
        }
        return false;
    };

    /// @brief Concatenates all the DVA keys using '+' separators. 
    /// @param[in] max_key  limit the number of keys report (0..n-1)
    std::string all_DVA_keys(const size_t max_key = 999) {
        assert(!dva.empty());
        std::wstring ws = dva[0];
        for (size_t i = 1; i < dva.size(); i++) {
            if (i > max_key)
                break;
            ws = ws + L" + " + dva[i];
        }
        return ToUtf8(ws);
    };

    bool operator == (const CDVAArlingtonTuple& a) {
        /* comparison */ 
        assert(!dva.empty());
        assert(!a.dva.empty());
        if ((link == a.link) && (dva.size() == a.dva.size())) {
            for (size_t i = 0; i < dva.size(); i++) {
                if (dva[i] != a.dva[i]) 
                    return false;
            }
            return true;
        }
        return false; 
    };
};


/// @brief  Process a single Adobe DVA dictionary definition (FormalRepTree dictionary)
/// 
/// @param[in]     tsv_dir Arlington TSV directory
/// @param[in,out] ofs     already open output stream for reporting messages
/// @param[in]     map     the Adobe DVA root dictionary for comparison
/// @param[in]     terse   whether output should be brief/terse
/// 
void process_dva_formal_rep_tree(const fs::path &tsv_dir, std::ostream& ofs, ArlPDFDictionary * map_dict, bool terse) {
    int count = 0;

    // A vector of already completed comparisons made up of an Arlington / Adobe DVA key tuples
    std::vector<CDVAArlingtonTuple>                mapped;

    // A vector of to-be-done comparisons made up of an Arlington / Adobe DVA key tuples
    std::queue<CDVAArlingtonTuple>                 to_process_checks;

    // Pre-populate some DVA vs Arlington tuples - mostly direct name matches
    // Group into roughly logical order so that output is also grouped
    //
    // THIS IS A MANUALLY CURATED LIST FROM GROKING THE ADOBE DVA BY HAND!!!
    // 
    // Thus ConcatWithFormalRep processing or /VerifyAtFormalRep lookups are no longer required
    //
    to_process_checks.emplace(L"Trailer",                       "FileTrailer");
    to_process_checks.emplace(L"XRef", L"StreamDict",           "XRefStream");
    to_process_checks.emplace(L"Linearized",                    "LinearizationParameterDict");
    // Document Catalog
    to_process_checks.emplace(L"Catalog",                       "Catalog");
    to_process_checks.emplace(L"DocInfo",                       "DocInfo");
    to_process_checks.emplace(L"MarkInfo",                      "MarkInfo");
    to_process_checks.emplace(L"Legal",                         "LegalAttestation");
    to_process_checks.emplace(L"CatalogDests",                  "DestsMap");
    to_process_checks.emplace(L"CatalogDestsDict",              "DestDict");
    to_process_checks.emplace(L"CatalogThreads",                "ArrayOfThreads");
    to_process_checks.emplace(L"CatalogURI",                    "URI");
    to_process_checks.emplace(L"AcroForm",                      "InteractiveForm");
    to_process_checks.emplace(L"AcroFormFields",                "ArrayOfFields");
    // Additional Actions
    to_process_checks.emplace(L"CatalogAdditionalActions",      "AddActionCatalog");
    to_process_checks.emplace(L"ScreenAnnotAdditionalActions",  "AddActionScreenAnnotation");
    to_process_checks.emplace(L"AnnotWidgetAdditionalActions",  "AddActionWidgetAnnotation");
    to_process_checks.emplace(L"PageAdditionalActions",         "AddActionPageObject");
    to_process_checks.emplace(L"FieldAdditionalActions",        "AddActionFormField");
    // 3D
    to_process_checks.emplace(L"3DCrossSection",                "3DCrossSection");
    to_process_checks.emplace(L"3DLightingScheme",              "3DLightingScheme");
    to_process_checks.emplace(L"3DNode",                        "3DNode");
    to_process_checks.emplace(L"3DRenderMode",                  "3DRenderMode");
    to_process_checks.emplace(L"3DVDict",                       "3DView");
    to_process_checks.emplace(L"3DADict",                       "3DActivation");
    to_process_checks.emplace(L"3DDStream", L"StreamDict",      "3DStream");
    to_process_checks.emplace(L"3DVBGDict",                     "3DBackground");
    to_process_checks.emplace(L"3DSectionArray",                "ArrayOf3DCrossSection");
    to_process_checks.emplace(L"3DDDict",                       "3DReference");
    to_process_checks.emplace(L"3DVPDict",                      "Projection");
    to_process_checks.emplace(L"3DANDict",                      "3DAnimationStyle");
    // to_process_checks.emplace(L"3DANDict???", "RichMediaAnimation"); // RichMedia is PDF 2.0
    // Page tree
    to_process_checks.emplace(L"Pages", L"PagesOrPage",          "PageTreeNodeRoot");
    to_process_checks.emplace(L"Pages", L"PagesOrPage",          "PageTreeNode");
    to_process_checks.emplace(L"Page",  L"PagesOrPage",          "PageObject");
    to_process_checks.emplace(L"PageTemplate",                   "PageObject");
    //
    to_process_checks.emplace(L"ExtGState",                     "GraphicsStateParameter");
    // Bead
    to_process_checks.emplace(L"Bead",                          "Bead");
    to_process_checks.emplace(L"Bead", L"Bead_First",           "BeadFirst");
    to_process_checks.emplace(L"Thread",                        "Thread");
    to_process_checks.emplace(L"ThreadInfo",                    "DocInfo");
    // Outlines
    to_process_checks.emplace(L"Outline",                       "OutlineItem");
    to_process_checks.emplace(L"Outlines",                      "Outline");
    // Patterns
    to_process_checks.emplace(L"PatternType1", L"Pattern", L"StreamDict",   "PatternType1");
    to_process_checks.emplace(L"PatternType2", L"Pattern",                  "PatternType2");
    // Font
    to_process_checks.emplace(L"FontType1",         L"Font",    "FontType1");
    to_process_checks.emplace(L"FontTrueType",      L"Font",    "FontTrueType");
    to_process_checks.emplace(L"FontMMType1",       L"Font",    "FontMultipleMaster");
    to_process_checks.emplace(L"FontType3",         L"Font",    "FontType3");
    to_process_checks.emplace(L"FontType0",         L"Font",    "FontType0");
    to_process_checks.emplace(L"FontCIDFontType0",  L"Font",    "FontCIDType0");
    to_process_checks.emplace(L"FontCIDFontType2",  L"Font",    "FontCIDType2");
    to_process_checks.emplace(L"CIDFontDescriptorFDDict",       "FDDict");
    to_process_checks.emplace(L"CIDFontDescriptorStyle",        "StyleDict");
    // Fonts
    to_process_checks.emplace(L"FontDescriptor",                                "FontDescriptorType3");
    to_process_checks.emplace(L"CIDType0FontDescriptor", L"FontDescriptor",     "FontDescriptorCIDType0");
    to_process_checks.emplace(L"CIDType2FontDescriptor", L"FontDescriptor",     "FontDescriptorCIDType2");
    to_process_checks.emplace(L"TrueTypeFontDescriptor", L"FontDescriptor",     "FontDescriptorTrueType");
    to_process_checks.emplace(L"Type1FontDescriptor",    L"FontDescriptor",     "FontDescriptorType1");
    to_process_checks.emplace(L"CIDSystemInfo",                                 "CIDSystemInfo");
    to_process_checks.emplace(L"CMap",                                          "CMap");
    to_process_checks.emplace(L"CharProc", L"StreamDict",                       "Stream");
    to_process_checks.emplace(L"FontFile", L"StreamDict",                       "FontFile");
    to_process_checks.emplace(L"TrueTypeFontFile2", L"FontFile", L"StreamDict", "FontFile2");
    to_process_checks.emplace(L"CIDType0FontFile3", L"FontFile", L"StreamDict", "FontFile3CIDType0");
    to_process_checks.emplace(L"Type1FontFile3",    L"FontFile", L"StreamDict", "FontFile3Type1");
    to_process_checks.emplace(L"Type1FontFile",     L"FontFile", L"StreamDict", "FontFileType1");
    // Functions
    to_process_checks.emplace(L"FunctionType0", L"Function", L"StreamDict",     "FunctionType0");
    to_process_checks.emplace(L"FunctionType2", L"Function",                    "FunctionType2");
    to_process_checks.emplace(L"FunctionType3", L"Function",                    "FunctionType3");
    to_process_checks.emplace(L"FunctionType4", L"Function", L"StreamDict",     "FunctionType4");
    // Halftones
    to_process_checks.emplace(L"HalftoneType1",  L"Halftone",                   "HalftoneType1");
    to_process_checks.emplace(L"HalftoneType5",  L"Halftone",                   "HalftoneType5");
    to_process_checks.emplace(L"HalftoneType6",  L"Halftone", L"StreamDict",    "HalftoneType6");
    to_process_checks.emplace(L"HalftoneType10", L"Halftone", L"StreamDict",    "HalftoneType10");
    to_process_checks.emplace(L"HalftoneType16", L"Halftone",                   "HalftoneType16");
    // XObjects
    to_process_checks.emplace(L"XObjectForm",                           L"XObjectForm",   L"XObject", L"StreamDict", "XObjectFormType1");
    to_process_checks.emplace(L"XObjectTrapNet",                        L"XObjectForm",   L"XObject", L"StreamDict", "XObjectFormTrapNet");
    to_process_checks.emplace(L"XObjectPS",                                               L"XObject", L"StreamDict", "XObjectFormPS");
    to_process_checks.emplace(L"XObjectPS",                                               L"XObject", L"StreamDict", "XObjectFormPSpassthrough");
    to_process_checks.emplace(L"XObjectImage",                       L"XObjectImageBase", L"XObject", L"StreamDict", "XObjectImage");
    to_process_checks.emplace(L"XObjectImageSMask", L"XObjectImage", L"XObjectImageBase", L"XObject", L"StreamDict", "XObjectImageSoftMask");
    to_process_checks.emplace(L"XObjectImageMask",  L"XObjectImage", L"XObjectImageBase", L"XObject", L"StreamDict", "XObjectImageMask");
    to_process_checks.emplace(L"Group",                             "GroupAttributes");
    to_process_checks.emplace(L"GroupTransparency",                 "GroupAttributes");
    // Resources
    to_process_checks.emplace(L"Resources",                         "Resource");
    to_process_checks.emplace(L"ExtGStateResources",                "GraphicsStateParameterMap");
    to_process_checks.emplace(L"ColorSpaceResources",               "ColorSpaceMap");
    to_process_checks.emplace(L"FontResources",                     "FontMap");
    to_process_checks.emplace(L"PatternResources",                  "PatternMap");
    to_process_checks.emplace(L"ShadingResources",                  "ShadingMap");
    to_process_checks.emplace(L"XObjectResources",                  "XObjectMap");
    // Rendition
    to_process_checks.emplace(L"Rendition", L"MediaRendition",      "RenditionMedia");
    to_process_checks.emplace(L"Rendition", L"SelectorRendition", L"MustHonorRendition", L"BestEffortRendition", "RenditionSelector");
    // Digital Signatures
    to_process_checks.emplace(L"SigDict",                       "Signature");
    to_process_checks.emplace(L"SVCert",                        "CertSeedValue");
    to_process_checks.emplace(L"MDP",                           "MDPDict");
    to_process_checks.emplace(L"SigRef", L"SigRefDocMDP",       "SignatureReferenceDocMDP");
    to_process_checks.emplace(L"SigRef", L"SigRefFieldMDP",     "SignatureReferenceFieldMDP");
    to_process_checks.emplace(L"SigRef", L"SigRefIdentity",     "SignatureReferenceIdentity");
    to_process_checks.emplace(L"SigRef", L"SigRefUR",           "SignatureReferenceUR");
    to_process_checks.emplace(L"SigRefDocMDPParams",            "DocMDPTransformParameters");
    to_process_checks.emplace(L"SigRefFieldMDPParams",          "FieldMDPTransformParameters");
    to_process_checks.emplace(L"SigRefURParams",                "URTransformParameters");
    // Actions
    // ActionGoToDp = new in PDF 2.0
    // ActionRichMediaExecute = new in PDF 2.0
    to_process_checks.emplace(L"Action", L"ActionGoTo",         "ActionGoTo");
    to_process_checks.emplace(L"Action", L"ActionGoTo3DView",   "ActionGoTo3DView");
    to_process_checks.emplace(L"Action", L"ActionGoToE",        "ActionGoToE");
    to_process_checks.emplace(L"Action", L"ActionGoToR",        "ActionGoToR");
    to_process_checks.emplace(L"Action", L"ActionHide",         "ActionHide");
    to_process_checks.emplace(L"Action", L"ActionImportData",   "ActionImportData");
    to_process_checks.emplace(L"Action", L"ActionJavaScript",   "ActionECMAScript");
    to_process_checks.emplace(L"Action", L"ActionLaunch",       "ActionLaunch");
    to_process_checks.emplace(L"Action", L"ActionMovie",        "ActionMovie");
    to_process_checks.emplace(L"Action", L"ActionNamed",        "ActionNamed");
    to_process_checks.emplace(L"Action", L"ActionRendition",    "ActionRendition");
    to_process_checks.emplace(L"Action", L"ActionResetForm",    "ActionResetForm");
    to_process_checks.emplace(L"Action", L"ActionSetOCGState",  "ActionSetOCGState");
    to_process_checks.emplace(L"Action", L"ActionSound",        "ActionSound");
    to_process_checks.emplace(L"Action", L"ActionSubmitForm",   "ActionSubmitForm");
    to_process_checks.emplace(L"Action", L"ActionThread",       "ActionThread");
    to_process_checks.emplace(L"Action", L"ActionTrans",        "ActionTransition");
    to_process_checks.emplace(L"Action", L"ActionURI",          "ActionURI");
    to_process_checks.emplace(L"Action", L"ActionNOP",          "ActionNOP");       // PDF 1.2 only
    to_process_checks.emplace(L"Action", L"ActionSetState",     "ActionSetState");  // PDF 1.2 only
    to_process_checks.emplace(L"ArrayOfActions",                "ArrayOfActions");
    to_process_checks.emplace(L"ActionLaunchWin",               "MicrosoftWindowsLaunchParam");
    //
    to_process_checks.emplace(L"AlternateImageArray",           "ArrayOfImageAlternates");
    to_process_checks.emplace(L"AlternateImageDict",            "AlternateImage");
    // Annotations
    // AnnotProjection = new in PDF 2.0
    // AnnotRichMedia = new in PDF 2.0
    // Redaction annotation is missing in DVA!
    to_process_checks.emplace(L"Annot3D",            L"WidgetOrField", L"Annot", "Annot3D");
    to_process_checks.emplace(L"AnnotCaret", L"WidgetOrField", L"Annot",  "AnnotCaret");
    to_process_checks.emplace(L"AnnotCircle", L"WidgetOrField", L"Annot",  "AnnotCircle");
    to_process_checks.emplace(L"AnnotFileAttachment", L"WidgetOrField", L"Annot",  "AnnotFileAttachment");
    to_process_checks.emplace(L"AnnotFreeText", L"WidgetOrField", L"Annot",  "AnnotFreeText");
    to_process_checks.emplace(L"AnnotHighlight", L"WidgetOrField", L"Annot",  "AnnotHighlight");
    to_process_checks.emplace(L"AnnotInk", L"WidgetOrField", L"Annot",  "AnnotInk");
    to_process_checks.emplace(L"AnnotLine", L"WidgetOrField", L"Annot",  "AnnotLine");
    to_process_checks.emplace(L"AnnotLink", L"WidgetOrField", L"Annot",  "AnnotLink");
    to_process_checks.emplace(L"AnnotMovie", L"WidgetOrField", L"Annot",  "AnnotMovie");
    to_process_checks.emplace(L"AnnotPolyLine", L"WidgetOrField", L"Annot",  "AnnotPolyLine");
    to_process_checks.emplace(L"AnnotPolygon", L"WidgetOrField", L"Annot",  "AnnotPolygon");
    to_process_checks.emplace(L"AnnotPopup", L"WidgetOrField", L"Annot",  "AnnotPopup");
    to_process_checks.emplace(L"AnnotPrinterMark", L"WidgetOrField", L"Annot",  "AnnotPrinterMark");
    to_process_checks.emplace(L"AnnotScreen", L"WidgetOrField", L"Annot",  "AnnotScreen");
    to_process_checks.emplace(L"AnnotSound", L"WidgetOrField", L"Annot",  "AnnotSound");
    to_process_checks.emplace(L"AnnotSquare", L"WidgetOrField", L"Annot",  "AnnotSquare");
    to_process_checks.emplace(L"AnnotSquiggly", L"WidgetOrField", L"Annot",  "AnnotSquiggly");
    to_process_checks.emplace(L"AnnotStamp", L"WidgetOrField", L"Annot",  "AnnotStamp");
    to_process_checks.emplace(L"AnnotStrikeOut", L"WidgetOrField", L"Annot",  "AnnotStrikeOut");
    to_process_checks.emplace(L"AnnotText", L"WidgetOrField", L"Annot",  "AnnotText");
    to_process_checks.emplace(L"AnnotTrapNet", L"WidgetOrField", L"Annot",  "AnnotTrapNetwork");
    to_process_checks.emplace(L"AnnotUnderline", L"WidgetOrField", L"Annot",  "AnnotUnderline");
    to_process_checks.emplace(L"AnnotWatermark", L"WidgetOrField", L"Annot",  "AnnotWatermark");
    to_process_checks.emplace(L"AnnotWidget", L"WidgetOrField", L"Annot", L"Field",  "AnnotWidget");
    // Colorspaces
    to_process_checks.emplace(L"CalGrayColorSpace",             "CalGrayColorSpace");
    to_process_checks.emplace(L"CalGrayDict",                   "CalGrayDict");
    to_process_checks.emplace(L"CalRGBColorSpace",              "CalRGBColorSpace");
    to_process_checks.emplace(L"CalRGBDict",                    "CalRGBDict");
    to_process_checks.emplace(L"CalGrayDict",                   "CalGrayDict");
    to_process_checks.emplace(L"ICCBasedColorSpace",            "ICCBasedColorSpace");
    to_process_checks.emplace(L"ICCBasedDict", L"StreamDict",   "ICCProfileStream");
    to_process_checks.emplace(L"IndexedColorSpace",             "IndexedColorSpace");
    to_process_checks.emplace(L"LabColorSpace",                 "LabColorSpace");
    to_process_checks.emplace(L"LabDict",                       "LabDict");
    to_process_checks.emplace(L"PatternColorSpace",             "PatternColorSpace");
    to_process_checks.emplace(L"DeviceNColorSpace",             "DeviceNColorSpace");
    to_process_checks.emplace(L"DeviceNDict",                   "DeviceNDict");
    to_process_checks.emplace(L"DeviceNMixingHints",            "DeviceNMixingHints");
    to_process_checks.emplace(L"DeviceNProcess",                "DeviceNProcess");
    to_process_checks.emplace(L"DeviceNColorants",              "ColorantsDict");
    to_process_checks.emplace(L"DeviceNDotGain",                "DictionaryOfFunctions");
    to_process_checks.emplace(L"DeviceNSolidities",             "Solidities");
    to_process_checks.emplace(L"SeparationColorSpace",          "SeparationColorSpace");
    to_process_checks.emplace(L"SeparationInfo",                "Separation");
    // Appearances
    to_process_checks.emplace(L"Appearance",                    "Appearance");
    to_process_checks.emplace(L"AppearanceCharacteristics",     "AppearanceCharacteristics");
    to_process_checks.emplace(L"AppearanceSubDict",             "AppearanceSubDict");
    to_process_checks.emplace(L"AppearanceTrapNet",             "AppearanceTrapNet");
    to_process_checks.emplace(L"AppearanceTrapNetDict",         "AppearanceTrapNetSubDict");
    to_process_checks.emplace(L"AppearanceTrapNet",             "AppearanceTrapNet");
    // Misc
    to_process_checks.emplace(L"ApplicationDataDict",           "Data");
    to_process_checks.emplace(L"Trans",                         "Transition");
    to_process_checks.emplace(L"BorderEffect",                  "BorderEffect");
    to_process_checks.emplace(L"BorderStyle",                   "BorderStyle");
    to_process_checks.emplace(L"BoxColorInfo",                  "BoxColorInfo");
    to_process_checks.emplace(L"BoxStyleDict",                  "BoxStyle");
    to_process_checks.emplace(L"ClassMap",                      "ClassMap");
    to_process_checks.emplace(L"Names",                         "Name");
    to_process_checks.emplace(L"IconFitDict",                   "IconFit");
    // Portable Collections
    to_process_checks.emplace(L"Collection",                    "Collection");
    to_process_checks.emplace(L"CollectionField",               "CollectionField");
    to_process_checks.emplace(L"CollectionItem",                "CollectionItem");
    to_process_checks.emplace(L"CollectionSchema",              "CollectionSchema");
    to_process_checks.emplace(L"CollectionSort",                "CollectionSort");
    to_process_checks.emplace(L"CollectionSubitem",             "CollectionSubitem");
    //
    to_process_checks.emplace(L"DestXYZ",                       "DestXYZ");
    to_process_checks.emplace(L"EmbeddedFileParams",            "EmbeddedFileParameter");
    to_process_checks.emplace(L"Stream", L"EmbeddedFileStream", "EmbeddedFileStream");
    to_process_checks.emplace(L"EmbeddedFile",                  "FileSpecEF");
    to_process_checks.emplace(L"EmbeddedFileOrFilespec",        "ArrayOfURLs");

    to_process_checks.emplace(L"Encoding",                      "Encoding");
    to_process_checks.emplace(L"3DExData", L"ExData",           "ExData3DMarkup");
    to_process_checks.emplace(L"ExData",                        "ExDataMarkupGeo");
    // ExDataProjection = new in PDF 2.0
    to_process_checks.emplace(L"FDDict",                        "FDDict");
    // Fields
    to_process_checks.emplace(              L"Field",           "Field");
    to_process_checks.emplace(L"FieldBtn",  L"Field",           "FieldBtn");
    to_process_checks.emplace(L"FieldCh",   L"Field",           "FieldCh");
    to_process_checks.emplace(L"FieldSig",  L"Field",           "FieldSig");
    to_process_checks.emplace(L"FieldTx",   L"Field",           "FieldTx");
    to_process_checks.emplace(L"FieldSigLock",                  "SigFieldLock");
    to_process_checks.emplace(L"FieldSigSV",                    "SigFieldSeedValue");
    to_process_checks.emplace(L"Filespec",                      "Filespecification");
    // Filter params
    to_process_checks.emplace(L"CryptFilter",                   "CryptFilter");
    to_process_checks.emplace(L"DCTDecodeParms",                "FilterDCTDecode");
    to_process_checks.emplace(L"FlateDecodeParms",              "FilterFlateDecode");
    to_process_checks.emplace(L"CCITTFaxDecodeParms",           "FilterCCITTFaxDecode");
    to_process_checks.emplace(L"JBIG2DecodeParms",              "FilterJBIG2Decode");
    to_process_checks.emplace(L"LZWDecodeParms",                "FilterLZWDecode");
    to_process_checks.emplace(L"CryptFilterDecodeParms",        "FilterCrypt");
    to_process_checks.emplace(L"StandardSecHandler", L"Encrypt","EncryptionStandard");
    to_process_checks.emplace(L"PublicKeyHandler", L"Encrypt",  "EncryptionPublicKey");
    //
    to_process_checks.emplace(L"FixedPrint",                    "FixedPrint");
    to_process_checks.emplace(L"MacSpecificFileInfo",           "Mac");
    // Measurement
    to_process_checks.emplace(L"MeasureRL", L"Measure",         "MeasureRL");
    to_process_checks.emplace(L"Measure",                       "MeasureGEO");
    // Media clips
    to_process_checks.emplace(L"MediaClip", L"MustHonorMCD", L"BestEffortMCD", L"MCD", "MediaClipData");
    to_process_checks.emplace(L"MediaClip", L"MustHonorMCD", L"BestEffortMCD", L"MCD", "MediaClipDataMHBE");
    to_process_checks.emplace(L"MediaClip", L"MustHonorMCS", L"BestEffortMCS", L"MCS", "MediaClipSection");
    to_process_checks.emplace(L"MediaClip", L"MustHonorMCS", L"BestEffortMCS", L"MCS", "MediaClipSectionMHBE");
    to_process_checks.emplace(L"MediaCriteria",                 "MediaCriteria");
    to_process_checks.emplace(L"MediaDuration",                 "MediaDuration");
    to_process_checks.emplace(L"MediaOffset", L"MediaOffsetFrame",  "MediaOffsetFrame");
    to_process_checks.emplace(L"MediaOffset", L"MediaOffsetMarker", "MediaOffsetMarker");
    to_process_checks.emplace(L"MediaOffset", L"MediaOffsetTime",   "MediaOffsetTime");
    to_process_checks.emplace(L"MediaPermissions",              "MediaPermissions");
    to_process_checks.emplace(L"MediaPlayParams", L"MustHonorMediaPlayParams", L"BestEffortMediaPlayParams", "MediaPlayParameters");
    to_process_checks.emplace(L"MediaPlayers",                  "MediaPlayers");
    to_process_checks.emplace(L"MediaScreenParams", L"MustHonorMediaScreenParams", "MediaScreenParameters");
    to_process_checks.emplace(L"MediaPlayerInfo",               "MediaPlayerInfo");
    to_process_checks.emplace(L"BestEffortMediaScreenParams",   "MediaScreenParametersMHBE");
    to_process_checks.emplace(L"MCR",                           "MarkedContentReference");
    to_process_checks.emplace(L"FWParams",                      "FloatingWindowParameters");
    //
    to_process_checks.emplace(L"Movie",                         "Movie");
    to_process_checks.emplace(L"MovieActivation",               "MovieActivation");
    to_process_checks.emplace(L"MinBitDepth",                   "MinimumBitDepth");
    to_process_checks.emplace(L"MinScreenSize",                 "MinimumScreenSize");
    to_process_checks.emplace(L"Metadata", L"StreamDict",       "Metadata");
    to_process_checks.emplace(L"NavNode",                       "NavNode");
    to_process_checks.emplace(L"NumberFormat",                  "NumberFormat");
    to_process_checks.emplace(L"ReferencedPDF",                 "Reference");
    // Optional content
    to_process_checks.emplace(L"OCGorOCMD", L"OCG",             "OptContentGroup");
    to_process_checks.emplace(L"OCGorOCMD", L"OCMD",            "OptContentMembership");
    to_process_checks.emplace(L"OCConfig",                      "OptContentConfig");
    to_process_checks.emplace(L"OCCreatorInfo",                 "OptContentCreatorInfo");
    to_process_checks.emplace(L"OCExport",                      "OptContentExport");
    to_process_checks.emplace(L"OCLanguage",                    "OptContentLanguage");
    to_process_checks.emplace(L"OCPageElement",                 "OptContentPageElement");
    to_process_checks.emplace(L"OCPrint",                       "OptContentPrint");
    to_process_checks.emplace(L"OCProperties",                  "OptContentProperties");
    to_process_checks.emplace(L"OCUsage",                       "OptContentUsage");
    to_process_checks.emplace(L"OCUsageApplication",            "OptContentUsageApplication");
    to_process_checks.emplace(L"OCUser",                        "OptContentUser");
    to_process_checks.emplace(L"OCView",                        "OptContentView");
    to_process_checks.emplace(L"OCZoom",                        "OptContentZoom");
    // OPI
    to_process_checks.emplace(L"OPIDict",                       "OPIVersion13");    // just the /1.3 key
    to_process_checks.emplace(L"OPI1.3",                        "OPIVersion13Dict");
    to_process_checks.emplace(L"OPIDict",                       "OPIVersion20");    // just the /2.0 key
    to_process_checks.emplace(L"OPI2.0",                        "OPIVersion20Dict");
    //
    to_process_checks.emplace(L"OutputIntents",                 "OutputIntents");
    to_process_checks.emplace(L"PageLabel",                     "PageLabel");
    to_process_checks.emplace(L"PagePieceDict",                 "PagePiece");
    to_process_checks.emplace(L"Perms",                         "Permissions");
    // Document requirements - nothing specific was specified prior to ISO 32000-2
    to_process_checks.emplace(L"RequirementHandler",            "RequirementsHandler");
    to_process_checks.emplace(L"Requirements",                  "RequirementsEnableJavaScripts");
    // Logical structure
    to_process_checks.emplace(L"StructTreeRoot",                "StructTreeRoot");
    to_process_checks.emplace(L"StructElem", L"StructElemAttribute", "StructElem");
    to_process_checks.emplace(L"RoleMap",                       "RoleMap");
    to_process_checks.emplace(L"ObjStm", L"StreamDict",         "ObjectStream");
    to_process_checks.emplace(L"LayoutAttributes",              "StandardLayoutAttributesBLSE");
    to_process_checks.emplace(L"LayoutAttributes",              "StandardLayoutAttributesILSE");
    to_process_checks.emplace(L"LayoutAttributes",              "StandardLayoutAttributesColumn");
    to_process_checks.emplace(L"ListAttributes",                "StandardListAttributes");
    to_process_checks.emplace(L"TableAttributes",               "StandardTableAttributes");
    to_process_checks.emplace(L"OBJR",                          "ObjectReference");
    //
    to_process_checks.emplace(L"StreamDict",                            "Stream");
    to_process_checks.emplace(L"SlideShow", L"AlternatePresentations",  "SlideShow");
    to_process_checks.emplace(L"SoftMask",                              "SoftMaskAlpha");
    to_process_checks.emplace(L"SoftMask", L"SoftMaskLuminosity",       "SoftMaskLuminosity");
    to_process_checks.emplace(L"SoftwareIdentifier",                    "SoftwareIdentifier");
    to_process_checks.emplace(L"Sound", L"StreamDict",                  "SoundObject");
    to_process_checks.emplace(L"SourceInfo",                            "SourceInformation");
    to_process_checks.emplace(L"AliasedURL",                            "URLAlias");
    // Shadings
    to_process_checks.emplace(L"Shading", L"ShadingType1",      "ShadingType1");
    to_process_checks.emplace(L"Shading", L"ShadingType2",      "ShadingType2");
    to_process_checks.emplace(L"Shading", L"ShadingType3",      "ShadingType3");
    to_process_checks.emplace(L"Shading", L"ShadingType4", L"StreamDict", "ShadingType4");
    to_process_checks.emplace(L"Shading", L"ShadingType5", L"StreamDict", "ShadingType5");
    to_process_checks.emplace(L"Shading", L"ShadingType6", L"StreamDict", "ShadingType6");
    to_process_checks.emplace(L"Shading", L"ShadingType7", L"StreamDict", "ShadingType7");
    // Sig Ref.
    to_process_checks.emplace(L"SigRef", L"SigRefDocMDP",       "SignatureReferenceDocMDP");
    to_process_checks.emplace(L"SigRef", L"SigRefFieldMDP",     "SignatureReferenceFieldMDP");
    to_process_checks.emplace(L"SigRef", L"SigRefIdentity",     "SignatureReferenceIdentity");
    to_process_checks.emplace(L"SigRef", L"SigRefUR",           "SignatureReferenceUR");
    //
    to_process_checks.emplace(L"SubjectDN",                     "SubjectDN");
    to_process_checks.emplace(L"Target",                        "Target");
    to_process_checks.emplace(L"Thumbnail", L"StreamDict",      "Thumbnail");
    to_process_checks.emplace(L"Timespan",                      "Timespan");
    to_process_checks.emplace(L"TimeStamp",                     "TimeStampDict");
    to_process_checks.emplace(L"UserProperty",                  "UserProperty");
    // UR
    to_process_checks.emplace(L"URParamAnnotsArray",            "URTransformParamAnnotsArray");
    to_process_checks.emplace(L"URParamDocArray",               "URTransformParamDocumentArray");
    to_process_checks.emplace(L"URParamEFArray",                "URTransformParamEFArray");
    to_process_checks.emplace(L"URParamFormArray",              "URTransformParamFormArray");
    to_process_checks.emplace(L"URParamSigArray",               "URTransformParamSignatureArray");
    //
    to_process_checks.emplace(L"ViewPort",                      "Viewport");
    to_process_checks.emplace(L"ViewerPreferences",             "ViewerPreferences");
    // SpiderInfo / Web capture
    to_process_checks.emplace(L"SpiderInfo",                    "WebCaptureInfo");
    to_process_checks.emplace(L"WebCaptureCommand",             "WebCaptureCommand");
    to_process_checks.emplace(L"WebCaptureCommandSettings",     "WebCaptureCommandSettings");
    to_process_checks.emplace(L"SpiderContentSet", L"SpiderContentSetSIS", "WebCaptureImageSet");
    to_process_checks.emplace(L"SpiderContentSet", L"SpiderContentSetSPS", "WebCapturePageSet");
    to_process_checks.emplace(L"GenericDict",                   "_UniversalDictionary");


    while (!to_process_checks.empty()) {
        CDVAArlingtonTuple elem = to_process_checks.front();

        assert((elem.link != "") && !elem.dva.empty());

        ofs << std::endl;
        to_process_checks.pop();

        // Ensure no predicates, comma lists or complex lists
        assert(elem.link.find(':') == std::string::npos);
        assert(elem.link.find(',') == std::string::npos);
        assert(elem.link.find(';') == std::string::npos);

        auto found = std::find_if(mapped.begin(), mapped.end(), 
            [&elem](CDVAArlingtonTuple& a) { return (a == elem); });

        if (found != mapped.end()) {
            if (ArlingtonPDFShim::debugging && !terse) {
                ofs << "Already processed DVA " << elem.all_DVA_keys() << " vs Arlington '" << elem.link << "'" << std::endl;
            }
            continue;
        }

        // Add the current Arlingon / DVA pair to the "already done" vector
        mapped.push_back(elem);

        // load Arlington definition (TSV file)
        std::unique_ptr<CArlingtonTSVGrammarFile> reader(new CArlingtonTSVGrammarFile(tsv_dir / (elem.link + ".tsv")));
        if (!reader->load()) {
            ofs << COLOR_ERROR << "loading Arlington TSV file " << (tsv_dir / (elem.link + ".tsv")) << COLOR_RESET;
            continue;
        }

        // load the full set of DVA dictionaries that map to a single Arlington definition
        std::vector<ArlPDFDictionary *> dva_dicts;
        for (size_t i = 0; i < elem.dva.size(); i++) {
#ifdef DVA_TRACING
            ofs << "Reading DVA object " << ToUtf8(elem.dva[i]) << std::endl;
#endif
            ArlPDFDictionary * d = (ArlPDFDictionary*)map_dict->get_value(elem.dva[i]);
            if (d == nullptr) {
                ofs << COLOR_ERROR << "Adobe DVA key not found: " << ToUtf8(elem.dva[i]) << COLOR_RESET;
            }
            else if (d->get_object_type() != PDFObjectType::ArlPDFObjTypeDictionary) {
                ofs << COLOR_ERROR << "Adobe DVA key was not a dictionary: " << ToUtf8(elem.dva[i]) << COLOR_RESET;
            }
            else {
                dva_dicts.push_back(d);
            }
        }
        ofs << COLOR_INFO << count++ << ": Comparing DVA: " << elem.all_DVA_keys() << " vs Arlington: " << elem.link << COLOR_RESET;

        const ArlTSVmatrix* data_list = &reader->get_data();

        // what Arlington has but Adobe DVA doesn't
        for (auto& vec : *data_list) {
            ArlPDFDictionary* inner_obj = nullptr; // pointer to the DVA dictionary with the current key
            int array_idx = -1;                    // if >= 0 then Arlington array index 

            // If the 1st and only letter of the Arlington key is a digit convert to array index (integer)
            if ((vec[TSV_KEYNAME].size() == 1) && (std::string("0123456789").find(vec[TSV_KEYNAME][0]) != std::string::npos))  {
                try {
                    array_idx = std::stoi(vec[TSV_KEYNAME]);
                }
                catch (...) {
                    array_idx = -1;
                }
            }

            if (vec[TSV_KEYNAME] == "*") {
                // Arlington wildcard - could be key name or array elements
                // Need to cycle through all DVA dicts looking for either GenericKey or Array entries (assume just one of each)
                ArlPDFDictionary* generic_key = nullptr;
                ArlPDFArray*      inner_array = nullptr;

                for (auto& d : dva_dicts) {
                    if (generic_key == nullptr)
                        generic_key = (ArlPDFDictionary*)d->get_value(L"GenericKey");
                    if (inner_array == nullptr)
                        inner_array = (ArlPDFArray*)d->get_value(L"Array");
                }

                if ((generic_key == nullptr) && (inner_array == nullptr)) {
                    ofs << COLOR_WARNING << "Arlington wildcard did not have any match in Adobe DVA for " << elem.all_DVA_keys() << COLOR_RESET;
                }
                else if ((generic_key != nullptr) && (generic_key->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary)) {
                    // wildcard matches dictionary keys! 
                    ofs << "Arlington wildcard matched Adobe DVA GenericKey (as dictionary)" << std::endl;
                }
                else if ((inner_array != nullptr) && (inner_array->get_object_type() == PDFObjectType::ArlPDFObjTypeArray) || (inner_array->get_num_elements() >= 1)) {
                    ArlPDFObject* elem0 = inner_array->get_value(0);
                    assert(elem0->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary);
                    // wildcard matches array elements!
                    ofs << "Arlington wildcard matched Adobe DVA Array (as array)" << std::endl;
                    delete elem0;
                }
                delete generic_key;
                delete inner_array;
                continue;
            }
            else if (array_idx >= 0) {
                /// DVA Array object definitions:
                ///  - /ArrayStyle = /Direct (meaing 1-for-1) or /Repeat (one array entry = wildcard) or /Switch
                ///  - /Array is an array of dictionaries  
                ///  e.g. /RectangleArray <<
                /// 	    /Array[ << /ValueType [ /CosInteger /CosFixed ] >>
                ///                 << /ValueType [ /CosInteger /CosFixed ] >>
                ///                 << /ValueType [ /CosInteger /CosFixed ] >>
                ///                 << /ValueType [ /CosInteger /CosFixed ] >> ]
                /// 	    /ArrayStyle / Direct
                /// 	    /FormalRepOfArray /RectangleArray
                /// 	 >>
                ///  e.g. /RHArray <<
                ///         /Array [ << /ValueType [ /CosDict ] /VerifyAtFormalRep /RequirementHandler >> ]
                ///         /ArrayStyle /Repeat
                ///         /FormalRepOfArray /RHArray
                // >>
                ArlPDFArray* inner_array = nullptr;
                ArlPDFName*  array_style = nullptr;

                for (auto& d : dva_dicts) {
                    if (inner_array == nullptr) {
                        inner_array = (ArlPDFArray*)d->get_value(L"Array");
                        array_style = (ArlPDFName*)d->get_value(L"ArrayStyle");
                    }
                }

                if (inner_array == nullptr) {
                    ofs << COLOR_WARNING << "Arlington array did not have any matching Adobe DVA array in " << elem.all_DVA_keys() << COLOR_RESET;
                    continue;
                }

                if (inner_array->get_num_elements() <= array_idx) {
                    ofs << "Arlington array index " << array_idx << "is larger than the Adobe DVA array in " << elem.all_DVA_keys() << std::endl;
                    delete array_style;
                    delete inner_array;
                    continue;
                }
                assert(array_style != nullptr);
                inner_obj = (ArlPDFDictionary *)inner_array->get_value(array_idx);
                assert(inner_obj->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary);
                assert(inner_obj->has_key(L"ValueType"));
                delete array_style;
                delete inner_array;
            }
            else {
                // Normal dictionary (named) key. Cycle through all DVA dicts looking for the precise key
                ArlPDFObject* key = nullptr;
                for (auto& d : dva_dicts) {
                    key = d->get_value(ToWString(vec[TSV_KEYNAME]));
                    if (key != nullptr)
                        break;
                }

                if ((key != nullptr) && (key->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary)) {
                    assert(inner_obj == nullptr);
                    inner_obj = (ArlPDFDictionary*)key;
                }
                else {
                    delete key;
                }
            }

            if (inner_obj == nullptr) {
                // Avoid reporting all the PDF 2.0 new stuff, wildcards and DVA arrays...
                if ((vec[TSV_SINCEVERSION] != "2.0") && (vec[TSV_KEYNAME] != "*") && (array_idx < 0)) {
                    ofs << "Missing key " << vec[TSV_KEYNAME] << " in DVA (Arlington version == " << vec[TSV_SINCEVERSION] << ")" << std::endl;
                }
                continue;
            }

            // Have a DVA dictionary for specific Arlington key  
            assert(inner_obj != nullptr);

            // Arlington IndirectReference can have predicates "fn:MustBeDirect(...)", "fn:MustBeDirect(...)" or be complex ([];[];[];...)
            // Linux CLI:  cut -f 6 *.tsv | sort | uniq
            // Arlington field is UPPERCASE
            if (inner_obj->has_key(L"MustBeIndirect")) {
                std::string indirect = "FALSE";
                ArlPDFObject* indr = inner_obj->get_value(L"MustBeIndirect");
                if (indr != nullptr) {
                    assert(indr->get_object_type() == PDFObjectType::ArlPDFObjTypeBoolean);
                    ArlPDFBoolean* indr_b = (ArlPDFBoolean*)indr;
                    if (indr_b->get_value())
                        indirect = "TRUE";
                    if (vec[TSV_INDIRECTREF] != indirect) {
                        ofs << "Indirect is different for key" << vec[TSV_KEYNAME] << ": DVA" << elem.all_DVA_keys()  << "==" << indirect;
                        ofs << " vs Arlington" << elem.link << "==" << vec[TSV_INDIRECTREF] << std::endl;
                    }
                }
                delete indr;
            }

            // Arlington Required field can also have predicates "fn:IsRequired(...)" or be complex ([];[];[];...)
            // Linux CLI:  cut -f 5 *.tsv | sort | uniq
            // Arlington field is UPPERCASE
            if (inner_obj->has_key(L"Required")) {
                ArlPDFBoolean* req_b = (ArlPDFBoolean*)inner_obj->get_value(L"Required");
                if (req_b != nullptr) {
                    assert(req_b->get_object_type() == PDFObjectType::ArlPDFObjTypeBoolean);
                    std::string required = "FALSE";
                    if (req_b->get_value())
                        required = "TRUE";
                    if (vec[TSV_REQUIRED] != required) {
                        // Suppress if terse and there are predicates
                        if (!terse || (vec[TSV_REQUIRED].find("fn:") == std::string::npos)) {
                            ofs << "Required is different for key " << vec[TSV_KEYNAME] << ": DVA " << required;
                            ofs << " vs Arlington: " << elem.link << "==" << vec[TSV_REQUIRED] << std::endl;
                        }
                    }
                }
                delete req_b;
            }
            else if (array_idx < 0)  { // DVA does not define Required for arrays
                ofs << "DVA did not specify Required for " << elem.all_DVA_keys() << "/" << vec[TSV_KEYNAME] << std::endl;
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
                    std::string dva_ver = std::to_string(pdf_major) + "." + std::to_string(pdf_minor);
                    if (dva_ver != vec[TSV_SINCEVERSION]) {
                        ofs << "SinceVersion different for key " << vec[TSV_KEYNAME] << ": DVA (" << dva_ver << ") vs Arlington (" << vec[TSV_SINCEVERSION] << ")" << std::endl;
                    }
                }
                else {
                    ofs << COLOR_ERROR << "DVA PDFMajorVersion/PDFMinorVersion was invalid for " << elem.all_DVA_keys() << "/" << vec[TSV_KEYNAME] << COLOR_RESET;
                }
                delete major; 
                delete minor;
            }

            // Check allowed Types
            {
                ArlPDFObject *vt = inner_obj->get_value(L"ValueType");
                if (vt == nullptr) {
                    ofs << COLOR_ERROR << "No ValueType defined for DVA for " << elem.all_DVA_keys() << "/" << vec[TSV_KEYNAME] << COLOR_RESET;
                }
                else if (vt->get_object_type() != PDFObjectType::ArlPDFObjTypeArray) {
                    ofs << COLOR_ERROR << "ValueType is not an array in DVA for " << elem.all_DVA_keys() << "/" << vec[TSV_KEYNAME] << COLOR_RESET;
                }
                else {
                    ArlPDFArray* types_array = (ArlPDFArray*)vt;
                    std::vector<std::string>    types_our = split(remove_type_link_predicates(vec[TSV_TYPE]), ';');

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
                    } 

                    // Adobe DVA types are always stored as names
                    std::vector<std::string>    types_dva;
                    for (auto i = 0; i < types_array->get_num_elements(); i++) {
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
                            ofs << COLOR_ERROR << "DVA ValueType array element is not a name object for key " << vec[TSV_KEYNAME] << COLOR_RESET;
                        }
                        delete obj;
                    } // for

                    std::string head = "Type differences for key " + vec[TSV_KEYNAME] + "\n";
                    std::string our("");
                    for (auto& tpe : types_our)
                        if (tpe != "") {
                            if (our == "")
                                our = tpe;
                            else
                                our += ", " + tpe;
                        }

                    if (our != "") {
                        ofs << head << "\tArlington has: " << our << std::endl;
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
                        ofs << "\tDVA has: " << our << std::endl;
                    }
                }
                delete vt;
            }

            // Check Arlington PossibleValue field vs DVA Bounds
            {
                ArlPDFDictionary* bounds_dict = (ArlPDFDictionary*)inner_obj->get_value(L"Bounds");
                if ((bounds_dict != nullptr) && (bounds_dict->get_object_type() != PDFObjectType::ArlPDFObjTypeDictionary)) {
                    ofs << COLOR_ERROR << "Bounds is not a dictionary in DVA for " << elem.all_DVA_keys() << "/" << vec[TSV_KEYNAME] << COLOR_RESET;
                }
                else if ((vec[TSV_POSSIBLEVALUES] != "") && (!terse)) {
                    std::string possible = "";

                    if (bounds_dict == nullptr) {
                        ofs << "Bounds not defined for key " << vec[TSV_KEYNAME] << ": Arlington " << elem.link << " has PossibleValues==" << vec[TSV_POSSIBLEVALUES] << std::endl;
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
                                    s = remove_type_link_predicates(pv_typed[i]); 
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
                                        ofs << COLOR_ERROR << "DVA Bounds/Equals[" << i << "] was an unexpected type for " << elem.all_DVA_keys() << "/" << vec[TSV_KEYNAME] << COLOR_RESET;
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
                                    ofs << COLOR_ERROR << "DVA Bounds/Equals[" << i << "] was a null object for " << elem.all_DVA_keys() << "/" << vec[TSV_KEYNAME] << COLOR_RESET;
                                }// if
                                delete obj;
                            } // for

                            std::string head = "PossibleValue differences for key " + vec[TSV_KEYNAME] + "\n";
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
                                ofs << head << "\tArlington has: " << our << std::endl;
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
                                ofs << "\tDVA has: " << our << std::endl;
                            }
                        }
                        else if (possible_array != nullptr) {
                            ofs << COLOR_ERROR << "DVA Bounds/Equals was not an array for " << elem.all_DVA_keys() << "/" << vec[TSV_KEYNAME] << COLOR_RESET;
                        }
                        delete possible_array;
                    }
                }
                delete bounds_dict;
            }
            delete inner_obj;
        } // for-each key in arlington


        /// @brief Checks if a key name exists in Arlington
        /// 
        /// @param[in] key key name (string)
        /// 
        /// @returns true if key exists in Arlington or has a wildcard
        auto exists_in_our = [data_list](auto& key) {
            for (auto& vec : *data_list)
                if ((vec[TSV_KEYNAME] == ToUtf8(key)) || (vec[TSV_KEYNAME].find('*') != std::string::npos))
                    return true;
            return false;
        }; // auto


        /// @brief Iterates through all keys in a DVA PDF dictionary to see if they are in Arlington
        /// 
        /// @param[in] a_dict   the DVA PDF dictionary
        /// @param[in] in_ofs   report stream
        auto check_dict = [=](ArlPDFDictionary* dva_dict, std::ostream& in_ofs) {
            for (int i = 0; i < (dva_dict->get_num_keys()); i++) {
                std::wstring key = dva_dict->get_key_name_by_index(i);
                if (!exists_in_our(key) && (key != L"FormalRepOf") && (key != L"Array")
                    && (key != L"ArrayStyle") && (key != L"FormalRepOfArray") && (key != L"OR")
                    && (key != L"GenericKey") && (key != L"ConcatWithFormalReps")
                    && (key != L"Metadata") && (key != L"AF")) // keys in PDF 2.0 allowed anywhere
                {
                    in_ofs << "Missing key from DVA in Arlington: " << elem.link << "/" << ToUtf8(key) << std::endl;
                }
            }
        }; // auto

        for (auto& d : dva_dicts) {
            check_dict(d, ofs);
            delete d;
        }
    } // while

    // Report all Adobe DVA key names that were NOT compared
    if (!terse)
    {
        ofs << std::endl;
        int missed = 0;
        const int dva_num_keys = map_dict->get_num_keys();

        for (int i = 0; i < dva_num_keys; i++) {
            std::wstring key = map_dict->get_key_name_by_index(i);

            auto result = std::find_if(mapped.begin(), mapped.end(), 
                            [&key](CDVAArlingtonTuple a) { return a.contains_DVA_key(key); });

            if (result == mapped.end()) {
                // Candidate DVA key not found - exclude operators, operands, etc.
                // Look for key "FormalRepOf" in the dictionary
                ArlPDFObject* obj = map_dict->get_value(key);
                assert(obj != nullptr);
                assert(obj->get_object_type() == PDFObjectType::ArlPDFObjTypeDictionary);
                if (((ArlPDFDictionary*)obj)->has_key(L"FormalRepOf")) {
                    ofs << COLOR_WARNING << "Adobe DVA comaparison did not check DVA key: " << ToUtf8(key) << COLOR_RESET;
                    missed++;
                }
                delete obj;
            }
        } // for
        if (missed > 0) {
            ofs << COLOR_INFO << "Adobe DVA comparison did not check " << missed << " Adobe DVA keys" << COLOR_RESET;
        }
    }

    // Report all Arlington TSV files that were NOT compared
    // This will include all PDF 2.0 stuff.
    // Iterate across all physical files in the TSV folder to list anything that exists but was never referenced
    if (!terse)
    {
        ofs << std::endl;
        int missed = 0;
        for (const auto& entry : fs::directory_iterator(tsv_dir)) {
            if (entry.is_regular_file() && (entry.path().extension().string() == ".tsv")) {
                const auto tsv = entry.path().stem().string();

                auto result = std::find_if(mapped.begin(), mapped.end(), 
                    [&tsv](const CDVAArlingtonTuple& a) { return (a.link == tsv); });

                if (result == mapped.end()) {
                    ofs << COLOR_WARNING << "Adobe DVA comaparison did not check Arlington: " << tsv << COLOR_RESET;
                    missed++;
                }
            }
        } // for
        if (missed > 0) {
            ofs << COLOR_INFO << "Adobe DVA comparison did not check " << missed << " Arlington definitions" << COLOR_RESET;
        }
    }
}


/// @brief  Compares Arlington TSV file set against Adobe DVA formal representation of PDF 1.7
/// 
/// @param[in] pdfsdk          already instantiated PDF SDK Arlington shim object
/// @param[in] dva_file        the Adobe DVA PDF file with the FormalRep tree
/// @param[in] grammar_folder  the Arlington PDF model folder with TSV file set
/// @param[in] ofs             report stream
/// @param[in] terse           whether output should be terse (brief)
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
                    process_dva_formal_rep_tree(grammar_folder, ofs, formal_rep_dict, terse);
                }
                else {
                    ofs << COLOR_ERROR << "failed to acquire Trailer/Root/FormalRepTree" << COLOR_RESET;
                }
                delete formal_rep;
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
