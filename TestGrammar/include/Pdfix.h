////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2020 PDFix (http://pdfix.net). All Rights Reserved.
// This file was generated automatically
////////////////////////////////////////////////////////////////////////////////
#ifndef _Pdfix_h
#define _Pdfix_h

#include <stdint.h>
#include <vector>
#include <stdexcept>
#ifdef _WIN32
#include <Windows.h>
#endif

// Unwanted definition of GetObject macro on Windows
#ifdef GetObject
#undef GetObject
#endif
#ifndef _NOEXCEPT
#define _NOEXCEPT throw()
#endif

#define PDFIX_VERSION_MAJOR 5
#define PDFIX_VERSION_MINOR 3
#define PDFIX_VERSION_PATCH 5
#define MAX_INT 2147483647
#define MIN_INT -2147483647
#define _in_
#define _out_
#define _callback_

namespace PDFixSDK {

struct PdsObject;
struct PdsBoolean;
struct PdsNumber;
struct PdsString;
struct PdsName;
struct PdsArray;
struct PdsDictionary;
struct PdsStream;
struct PdsNull;
struct PdsPageObject;
struct PdsText;
struct PdsForm;
struct PdsPath;
struct PdsImage;
struct PdsShading;
struct PdsContentMark;
struct PdeElement;
struct PdeContainer;
struct PdeList;
struct PdeToc;
struct PdeAnnot;
struct PdeFormField;
struct PdeImage;
struct PdeLine;
struct PdeRect;
struct PdeHeader;
struct PdeFooter;
struct PdeCell;
struct PdeTable;
struct PdeWord;
struct PdeTextLine;
struct PdeText;
struct PdfAction;
struct PdfAnnot;
struct PdfLinkAnnot;
struct PdfMarkupAnnot;
struct PdfTextAnnot;
struct PdfTextMarkupAnnot;
struct PdfWidgetAnnot;
struct PdfViewDestination;
struct PdfBaseDigSig;
struct PdfDigSig;
struct PdfCertDigSig;
struct PdfCustomDigSig;
struct PdfDoc;
struct PdfDocTemplate;
struct PdfAlternate;
struct PdfHtmlAlternate;
struct PdfFont;
struct PdfFormField;
struct PsImage;
struct PdfPage;
struct PdePageMap;
struct PdfPageView;
struct PdfBookmark;
struct PdfNameTree;
struct PsRegex;
struct PsStream;
struct PsFileStream;
struct PsMemoryStream;
struct PsCustomStream;
struct PdsStructElement;
struct PdsClassMap;
struct PdsRoleMap;
struct PdsStructTree;
struct PsMetadata;
struct PsEvent;
struct PsAuthorization;
struct PsAccountAuthorization;
struct PsStandardAuthorization;
struct Pdfix;
struct PdfixPlugin;

typedef int PdfErrorType;
typedef int PdfAnnotFlags;
typedef int PdfRemoveAnnotFlags;
typedef int PdfTextStateFlag;
typedef int PdfFieldFlags;
typedef int PdfRenderFlags;
typedef int PdfFontFlags;
typedef int PdfPageContentFlags;
typedef int PdfTableType;
typedef int PdfWordFlags;
typedef int PdfTextLineFlags;
typedef int PdfTextRegexFlags;
typedef int PdfElementFlags;
typedef int PdfPageInsertFlags;

typedef enum {
  kAuthPlatformWin = 0,
  kAuthPlatformMac = 1,
  kAuthPlatformLinux = 2,
  kAuthPlatformAndroid = 3,
  kAuthPlatformiOS = 4,
  kAuthPlatformServer = 5,
} PdfAuthPlatform;

typedef enum {
  kAuthOptionBasic = 0,
  kAuthOptionProfessional = 1,
  kAuthOptionEnterprise = 2,
} PdfAuthOption;

enum {
  kNoError = 0,
  kErrorUnknown = 1,
  kErrorOutOfMemory = 2,
  kErrorMalformedInput = 3,
  kErrorMethodNotImplemented = 4,
  kErrorPathNotFound = 5,
  kErrorOperationCancelled = 6,
  kErrorReadingDataFile = 7,
  kErrorInit = 8,
  kErrorIndexOutOfRange = 9,
  kErrorIncompatiblePluginVersion = 10,
  kErrorPluginInitialization = 11,
  kErrorWritingDataFile = 12,
  kErrorPdfDocInvalid = 30,
  kErrorPdfDocOpen = 31,
  kErrorPdfDocCreate = 32,
  kErrorPdfDocSave = 33,
  kErrorPdfDocXFA = 34,
  kErrorPdfDocClose = 35,
  kErrorDocTemplateInvalid = 60,
  kErrorDocTemplateInvalidValue = 61,
  kErrorPdfDigSigOpenPfxFile = 90,
  kErrorPdfDigSigSaveFile = 91,
  kErrorPdfDigSigReadFile = 92,
  kErrorPdfDigSigCertOpenSystemStore = 93,
  kErrorPdfDigSigPFXImportCertStore = 94,
  kErrorPdfDigSigCertFindInStore = 95,
  kErrorPdfDigSigPFXImportOpenSSL = 96,
  kErrorPdfDigSigPFXParseOpenSSL = 97,
  kErrorPdfDigSigByteRange = 98,
  kErrorPdfDigSigCryptMemAlloc = 99,
  kErrorPdfDigSigCryptSignMessage = 100,
  kErrorPdfDigSigTimeStampMessage = 101,
  kErrorPdfDigSigTimeStampRequest = 102,
  kErrorPdfDigSigCryptHash = 103,
  kErrorPdfDigSigVerifyDetachedMessage = 104,
  kErrorPdfDigSigUnknownType = 105,
  kErrorPdfDigSigCallback = 106,
  kErrorPdsObjectInvalid = 120,
  kErrorPdfPageInvalidObj = 150,
  kErrorPdfPageInvalidColorSpace = 151,
  kErrorPdfPageMapAddElement = 180,
  kErrorPdfPageMapInvalidTextObj = 181,
  kErrorPdfPageMapAddTags = 182,
  kErrorPdfPageMapTagAttributes = 183,
  kErrorPdfPageMapTagParentTree = 184,
  kErrorPdfPageMapRecognition = 185,
  kErrorPdfPageMapAcquire = 186,
  kErrorPdeElementMalformed = 210,
  kErrorPdeTextRunMalformed = 211,
  kErrorPdeWordMalformed = 212,
  kErrorPdeLineMalformed = 213,
  kErrorPdeListMalformed = 214,
  kErrorPdeTextMalformed = 215,
  kErrorPdeTextSelectRange = 216,
  kErrorPdeTableMalformed = 217,
  kErrorPdeElementCreate = 218,
  kErrorPdfFontSubstFontMissing = 240,
  kErrorPdfFontNotEmbedded = 241,
  kErrorPdfFontSave = 242,
  kErrorPsImageOpenFile = 270,
  kErrorPsImageUnsupportedFormat = 271,
  kErrorPsImageWriteBMP = 272,
  kErrorPsImageWritePNG = 273,
  kErrorPsImageWriteJPG = 274,
  kErrorPsImageInvalidBitmap = 275,
  kErrorPsImageFormat = 276,
  kErrorPdfAnnotMalformed = 300,
  kErrorPdfAnnotInvalidType = 301,
  kErrorPsRegexDestroy = 330,
  kErrorPsRegexSearchFail = 331,
  kErrorPsEventMalformed = 360,
  kErrorPsEventExists = 361,
  kErrorPsNoEvent = 362,
  kErrorPdfBookmarkMalformed = 390,
  kErrorPdfBookmarkRoot = 391,
  kErrorPsAuthorizationFailed = 420,
  kErrorPsAuthorizationNeeded = 421,
  kErrorPsAuthorizationEmail = 422,
  kErrorPsAuthorizationPlatform = 423,
  kErrorPsAuthorizationDate = 424,
  kErrorPsAuthorizationVersion = 425,
  kErrorPsAuthorizationNumber = 426,
  kErrorPsAuthorizationOsCheck = 427,
  kErrorPsAuthorizationMaximumConsumptionReached = 428,
  kErrorPsAuthorizationOption = 429,
  kErrorPsStreamReadProcMissing = 450,
  kErrorPsStreamWriteProcMissing = 451,
  kErrorPsStreamGetSizeProcMissing = 452,
  kErrorPdfAlternateNotFound = 480,
  kErrorPdfAlternateInvalid = 481,
  kErrorPdfAlternateResourceNotFound = 482,
  kErrorPdsStructTreeInvalid = 510,
  kErrorPdsStructElementNotFound = 511,
  kErrorPdsStructTreeMissing = 512,
  kErrorPdfActionInvalid = 540,
  kErrorDataFormatInvalid = 570,
} ;

typedef enum {
  kEventUnknown = 0,
  kEventDocWillSave = 1,
  kEventDocWillClose = 2,
  kEventDocDidOpen = 3,
  kEventDocDidSave = 4,
  kEventDocWillChangePages = 5,
  kEventDocDidChangePages = 6,
  kEventDocWillDeletePages = 7,
  kEventDocDidDeletePages = 8,
  kEventDocWillInsertPages = 9,
  kEventDocDidInsertPages = 10,
  kEventDocWillMovePages = 11,
  kEventDocDidMovePages = 12,
  kEventDocWillReplacePages = 13,
  kEventDocDidReplacePages = 14,
  kEventAnnotWillChange = 15,
  kEventAnnotDidChange = 16,
  kEventPageWillAddAnnot = 17,
  kEventPageWillRemoveAnnot = 18,
  kEventPageDidAddAnnot = 19,
  kEventPageDidRemoveAnnot = 20,
  kEventPageContentsDidChange = 21,
} PdfEventType;

typedef enum {
  kSaveIncremental = 0,
  kSaveFull = 1,
} PdfSaveFlags;

typedef enum {
  kDigSigBlank = 0,
  kDigSigUnknown = 1,
  kDigSigInvalid = 2,
  kDigSigValid = 3,
  kDigSigDoubleChecked = 4,
  kDigSigValidStateEnumSize = 5,
} PdfDigSigValidState;

typedef enum {
  kAlignmentNone = 0,
  kAlignmentLeft = 1,
  kAlignmentRight = 2,
  kAlignmentJustify = 3,
  kAlignmentTop = 4,
  kAlignmentBottom = 5,
  kAlignmentCenter = 6,
} PdfAlignment;

typedef enum {
  kRotate0 = 0,
  kRotate90 = 90,
  kRotate180 = 180,
  kRotate270 = 270,
} PdfRotate;

typedef enum {
  kPdsUnknown = 0,
  kPdsBoolean = 1,
  kPdsNumber = 2,
  kPdsString = 3,
  kPdsName = 4,
  kPdsArray = 5,
  kPdsDictionary = 6,
  kPdsStream = 7,
  kPdsNull = 8,
  kPdsReference = 9,
} PdfObjectType;

typedef enum {
  kPdsPageUnknown = 0,
  kPdsPageText = 1,
  kPdsPagePath = 2,
  kPdsPageImage = 3,
  kPdsPageShading = 4,
  kPdsPageForm = 5,
} PdfPageObjectType;

typedef enum {
  kPdeUnknown = 0,
  kPdeText = 1,
  kPdeTextLine = 2,
  kPdeWord = 3,
  kPdeTextRun = 4,
  kPdeImage = 5,
  kPdeContainer = 6,
  kPdeList = 7,
  kPdeLine = 8,
  kPdeRect = 9,
  kPdeTable = 10,
  kPdeCell = 11,
  kPdeToc = 12,
  kPdeFormField = 13,
  kPdeHeader = 14,
  kPdeFooter = 15,
  kPdeAnnot = 16,
} PdfElementType;

typedef enum {
  kPdeContainerUnknown = 0,
  kPdeContainerPage = 1,
  kPdeContainerArt = 2,
} PdfContainerType;

typedef enum {
  kTagUnknown = 0,
  kTagSect = 1,
  kTagArt = 2,
} PdfTagType;

typedef enum {
  kPdfLineCapButt = 0,
  kPdfLineCapRound = 1,
  kPdfLineCapSquare = 2,
} PdfLineCap;

typedef enum {
  kPdfLineJoinMiter = 0,
  kPdfLineJoinRound = 1,
  kPdfLineJoinBevel = 2,
} PdfLineJoin;

typedef enum {
  kFillTypeNone = 0,
  kFillTypeSolid = 1,
  kFillTypePattern = 2,
} PdfFillType;

typedef enum {
  kTextAlignmentNone = 0,
  kTextAlignmentLeft = 1,
  kTextAlignmentRight = 2,
  kTextAlignmentCenter = 3,
  kTextAlignmentJustify = 4,
} PdfTextAlignment;

typedef enum {
  kAnnotUnknown = 0,
  kAnnotText = 1,
  kAnnotLink = 2,
  kAnnotFreeText = 3,
  kAnnotLine = 4,
  kAnnotSquare = 5,
  kAnnotCircle = 6,
  kAnnotPolygon = 7,
  kAnnotPolyLine = 8,
  kAnnotHighlight = 9,
  kAnnotUnderline = 10,
  kAnnotSquiggly = 11,
  kAnnotStrikeOut = 12,
  kAnnotStamp = 13,
  kAnnotCaret = 14,
  kAnnotInk = 15,
  kAnnotPopup = 16,
  kAnnotFileAttachment = 17,
  kAnnotSound = 18,
  kAnnotMovie = 19,
  kAnnotWidget = 20,
  kAnnotScreen = 21,
  kAnnotPrinterMark = 22,
  kAnnotTrapNet = 23,
  kAnnotWatermark = 24,
  kAnnot3D = 25,
  kAnnotRedact = 26,
} PdfAnnotSubtype;

enum {
  kAnnotFlagNone = 0x0000,
  kAnnotFlagInvisible = 0x0001,
  kAnnotFlagHidden = 0x0002,
  kAnnotFlagPrint = 0x0004,
  kAnnotFlagNoZoom = 0x0008,
  kAnnotFlagNoRotate = 0x0010,
  kAnnotFlagNoView = 0x0020,
  kAnnotFlagReadOnly = 0x0040,
  kAnnotFlagLocked = 0x0080,
  kAnnotFlagToggleNoView = 0x0100,
  kAnnotFlagLockedContents = 0x0200,
} ;

enum {
  kRemoveAnnotSingle = 0x0000,
  kRemoveAnnotPopup = 0x0001,
  kRemoveAnnotReply = 0x0002,
} ;

typedef enum {
  kBorderSolid = 0,
  kBorderDashed = 1,
  kBorderBeveled = 2,
  kBorderInset = 3,
  kBorderUnderline = 4,
} PdfBorderStyle;

enum {
  kTextFlagNone = 0x000,
  kTextFlagUnderline = 0x001,
  kTextFlagStrikeout = 0x002,
  kTextFlagHighlight = 0x004,
  kTextFlagSubscript = 0x008,
  kTextFlagSuperscript = 0x010,
  kTextFlagNoUnicode = 0x020,
  kTextFlagPatternFill = 0x040,
  kTextFlagPatternStroke = 0x080,
  kTextFlagWhiteSpace = 0x100,
  kTextFlagUnicode = 0x200,
} ;

enum {
  kFieldFlagNone = 0x00000000,
  kFieldFlagReadOnly = 0x00000001,
  kFieldFlagRequired = 0x00000002,
  kFieldFlagNoExport = 0x00000004,
  kFieldFlagMultiline = 0x00001000,
  kFieldFlagPassword = 0x00002000,
  kFieldFlagNoToggleToOff = 0x00004000,
  kFieldFlagRadio = 0x00008000,
  kFieldFlagPushButton = 0x00010000,
  kFieldFlagCombo = 0x00200000,
  kFieldFlagEdit = 0x00400000,
  kFieldFlagSort = 0x00800000,
  kFieldFlagMultiSelect = 0x00200000,
  kFieldFlagDoNotSpellCheck = 0x00400000,
  kFieldFlagDCommitOnSelChange = 0x04000000,
  kFieldFlagFileSelect = 0x00100000,
  kFieldFlagDoNotScroll = 0x00800000,
  kFieldFlagComb = 0x01000000,
  kFieldFlagRichText = 0x02000000,
  kFieldFlagRadiosInUnison = 0x02000000,
} ;

typedef enum {
  kFieldUnknown = 0,
  kFieldButton = 1,
  kFieldRadio = 2,
  kFieldCheck = 3,
  kFieldText = 4,
  kFieldCombo = 5,
  kFieldList = 6,
  kFieldSignature = 7,
} PdfFieldType;

typedef enum {
  kActionEventAnnotEnter = 0,
  kActionEventAnnotExit = 1,
  kActionEventAnnotMouseDown = 2,
  kActionEventAnnotMouseUp = 3,
  kActionEventAnnotFocus = 4,
  kActionEventAnnotBlur = 5,
  kActionEventAnnotPageOpen = 6,
  kActionEventAnnotPageClose = 7,
  kActionEventAnnotPageVisible = 8,
  kActionEventAnnotPageInvisible = 9,
  kActionEventPageOpen = 10,
  kActionEventPageClose = 11,
  kActionEventFieldKeystroke = 12,
  kActionEventFieldFormat = 13,
  kActionEventFieldValidate = 14,
  kActionEventFieldCalculate = 15,
  kActionEventDocWillClose = 16,
  kActionEventDocWillSave = 17,
  kActionEventDocDidSave = 18,
  kActionEventDocWillPrint = 19,
  kActionEventDocDidPrint = 20,
} PdfActionEventType;

typedef enum {
  kActionUnknown = 0,
  kActionGoTo = 1,
  kActionGoToR = 2,
  kActionGoToE = 3,
  kActionLaunch = 4,
  kActionThread = 5,
  kActionURI = 6,
  kActionSound = 7,
  kActionMovie = 8,
  kActionHide = 9,
  kActionNamed = 10,
  kActionSubmitForm = 11,
  kActionResetForm = 12,
  kActionImportData = 13,
  kActionJavaScript = 14,
  kActionSetOCGState = 15,
  kActionRendition = 16,
  kActionTrans = 17,
  kActionGoTo3DView = 18,
} PdfActionType;

enum {
  kRenderAnnot = 0x001,
  kRenderLCDText = 0x002,
  kRenderNoNativeText = 0x004,
  kRenderGrayscale = 0x008,
  kRenderLimitedCache = 0x010,
  kRenderForceHalftone = 0x020,
  kRenderPrinting = 0x040,
  kRenderNoText = 0x080,
  kRenderNoBackground = 0x100,
} ;

typedef enum {
  kImageFormatPng = 0,
  kImageFormatJpg = 1,
  kImageFormatBmp = 2,
  kImageFormatEmf = 3,
} PdfImageFormat;

enum {
  kFontFixedPitch = 0x00001,
  kFontSerif = 0x00002,
  kFontSymbolic = 0x00004,
  kFontScript = 0x00008,
  kFontNotSymbolic = 0x00020,
  kFontItalic = 0x00040,
  kFontAllCap = 0x10000,
  kFontSmallCap = 0x20000,
  kFontForceBold = 0x40000,
} ;

enum {
  kContentImage = 0x00001,
  kContentText = 0x00002,
  kContentPath = 0x00004,
  kContentForm = 0x00008,
  kContentShading = 0x00020,
  kContentTextTransparent = 0x00040,
  kContentTextFill = 0x00080,
  kContentTextStroke = 0x00100,
} ;

typedef enum {
  kFontAnsiCharset = 0,
  kFontDefaultCharset = 1,
  kFontSymbolCharset = 2,
  kFontUnknownCharset = 3,
  kFontMacintoshCharset = 77,
  kFontShiftJISCharset = 128,
  kFontHangeulCharset = 129,
  kFontKoreanCharset = 130,
  kFontGB2312Charset = 134,
  kFontCHineseBig5Charset = 136,
  kFontGreekCharset = 161,
  kFontTurkishCharset = 162,
  kFontVietnameseCharset = 163,
  kFontHebrewCharset = 177,
  kFontArabicCharset = 178,
  kFontArabicTCharset = 179,
  kFontArabicUCharset = 180,
  kFontHebrewUCharset = 181,
  kFontBalticCharset = 186,
  kFontRussianCharset = 204,
  kFontThaiCharset = 222,
  kFontEastEuropeCharset = 238,
} PdfFontCharset;

typedef enum {
  kAllPages = 0,
  kEvenPagesOnly = 1,
  kOddPagesOnly = 2,
} PdfPageRangeType;

typedef enum {
  kFontUnknownType = 0,
  kFontType1 = 1,
  kFontTrueType = 2,
  kFontType3 = 3,
  kFontCIDFont = 4,
} PdfFontType;

typedef enum {
  kFontFormatTtf = 0,
  kFontFormatWoff = 1,
} PdfFontFormat;

typedef enum {
  kPdfZoomXYZ = 1,
  kPdfZoomFitPage = 2,
  kPdfZoomFitHorz = 3,
  kPdfZoomFitVert = 4,
  kPdfZoomFitRect = 5,
  kPdfZoomFitBbox = 6,
  kPdfZoomFitBHorz = 7,
  kPdfZoomFitBVert = 8,
} PdfDestZoomType;

typedef enum {
  kDigSigOpenSSL = 0,
  kDigSigCert = 1,
  kDigSigCustom = 2,
} PdfDigSigType;

typedef enum {
  kImageFigure = 0,
  kImageImage = 1,
  kImagePath = 2,
  kImageRect = 3,
  kImageShading = 4,
  kImageForm = 5,
} PdfImageType;

enum {
  kTableUnknown = 0x00,
  kTableGraphic = 0x01,
  kTableIsolated = 0x02,
  kTableIsolatedCol = 0x04,
  kTableIsolatedRow = 0x08,
  kTableForm = 0x10,
  kTableElement = 0x20,
} ;

typedef enum {
  kListNone = 0,
  kListUnordered = 1,
  kListOrdered = 2,
  kListDecimal = 3,
  kListRomanUpper = 4,
  kListRomanLower = 5,
  kListLetterUpper = 6,
  kListLetterLower = 7,
} PdfListType;

enum {
  kWordHyphen = 0x0001,
  kWordBullet = 0x0002,
  kWordFilling = 0x0008,
  kWordNumber = 0x0010,
  kWordImage = 0x0020,
  kWordNoUnicode = 0x0040,
} ;

enum {
  kTextLineNewLine = 0x0001,
  kTextLineBullet = 0x0002,
  kTextLineHyphen = 0x0004,
  kTextLineIndent = 0x0008,
  kTextLineDropCap = 0x0010,
} ;

typedef enum {
  kTextNormal = 0,
  kTextH1 = 1,
  kTextH2 = 2,
  kTextH3 = 3,
  kTextH4 = 4,
  kTextH5 = 5,
  kTextH6 = 6,
  kTextH7 = 7,
  kTextH8 = 8,
  kTextNote = 9,
  kTextTitle = 10,
} PdfTextStyle;

enum {
  kTextFlagTableCaption = 0x0001,
  kTextFlagImageCaption = 0x0002,
  kTextFlagChartCaption = 0x0004,
  kTextFlagFilling = 0x008,
} ;

enum {
  kElemNoJoin = 0x01,
  kElemNoSplit = 0x02,
  kElemArtifact = 0x04,
  kElemHeader = 0x08,
  kElemFooter = 0x10,
} ;

typedef enum {
  kPsWrite = 0,
  kPsReadOnly = 1,
  kPsTruncate = 2,
} PsFileMode;

typedef enum {
  kAlternatePdf = 0,
  kAlternateHtml = 1,
} PdfAlternateType;

typedef enum {
  kCSSMediaTypeAll = 0,
  kCSSMediaTypePrint = 1,
  kCSSMediaTypeScreen = 2,
  kCSSMediaTypeSpeech = 3,
} PdfMediaType;

typedef enum {
  kImageDIBFormatArgb = 0x220,
} PsImageDIBFormat;

typedef enum {
  kDataFormatJson = 0,
  kDataFormatXml = 1,
} PsDataFormat;

typedef enum {
  kFileStream = 0,
  kMemoryStream = 1,
  kProcStream = 2,
} PdfStreamType;

typedef enum {
  kPdsStructKidInvalid = 0,
  kPdsStructKidElement = 1,
  kPdsStructKidPageContent = 2,
  kPdsStructKidStreamContent = 3,
  kPdsStructKidObject = 4,
} PdfStructElementType;

enum {
  kPageInsertNone = 0x0000,
  kPageInsertBookmarks = 0x001,
  kPageInsertAll = 0x0002,
} ;

typedef enum {
  kAuthorizationStandard = 0,
  kAuthorizationAccount = 1,
} PdfAuthorizationType;

typedef enum {
  kDestFitUnknown = 0,
  kDestFitXYZ = 1,
  kDestFit = 2,
  kDestFitH = 3,
  kDestFitV = 4,
  kDestFitR = 5,
  kDestFitB = 6,
  kDestFitBH = 7,
  kDestFitBV = 8,
} PdfDestFitType;


typedef struct _PdfPageRangeParams {
  int start_page;
  int end_page;
  PdfPageRangeType page_range_spec;
  _PdfPageRangeParams() {
    start_page = 0;
    end_page = -1;
    page_range_spec = kAllPages;
  }
} PdfPageRangeParams;

typedef struct _PdfWatermarkParams {
  PdfPageRangeParams page_range;
  int order_top;
  PdfAlignment h_align;
  PdfAlignment v_align;
  int percentage_vals;
  double h_value;
  double v_value;
  double scale;
  double rotation;
  double opacity;
  _PdfWatermarkParams() {
    order_top = 1;
    percentage_vals = 0;
    h_align = kAlignmentLeft;
    v_align = kAlignmentTop;
    h_value = 0;
    v_value = 0;
    scale = 1;
    rotation = 0;
    opacity = 1;
  }
} PdfWatermarkParams;

typedef struct _PdfPoint {
  double x;
  double y;
} PdfPoint;

typedef struct _PdfDevPoint {
  int x;
  int y;
} PdfDevPoint;

typedef struct _PdfRect {
  double left;
  double top;
  double right;
  double bottom;
  _PdfRect() {
    left = 0.;
    top = 0.;
    right = 0.;
    bottom = 0.;
  }
} PdfRect;

typedef struct _PdfDevRect {
  int left;
  int top;
  int right;
  int bottom;
  _PdfDevRect() {
    left = 0;
    top = 0;
    right = 0;
    bottom = 0;
  }
} PdfDevRect;

typedef struct _PdfQuad {
  PdfPoint tl;
  PdfPoint tr;
  PdfPoint bl;
  PdfPoint br;
} PdfQuad;

typedef struct _PdfDevQuad {
  PdfDevPoint tl;
  PdfDevPoint tr;
  PdfDevPoint bl;
  PdfDevPoint br;
} PdfDevQuad;

typedef struct _PdfMatrix {
  double a;
  double b;
  double c;
  double d;
  double e;
  double f;
  _PdfMatrix() {
    a = 1;
    b = 0;
    c = 0;
    d = 1;
    e = 0;
    f = 0;
  }
} PdfMatrix;

typedef struct _PdfRGB {
  int r;
  int g;
  int b;
  _PdfRGB() {
    r = 0;
    g = 0;
    b = 0;
  }
} PdfRGB;

typedef struct _PdfColorState {
  PdfFillType fill_type;
  PdfFillType stroke_type;
  PdfRGB fill_color;
  PdfRGB stroke_color;
  int fill_opacity;
  int stroke_opacity;
  _PdfColorState() {
    fill_type = kFillTypeNone;
    stroke_type = kFillTypeNone;
    fill_opacity = 255;
    stroke_opacity = 255;
  }
} PdfColorState;

typedef struct _PdfTextState {
  PdfColorState color_state;
  PdfFont* font;
  double font_size;
  double char_spacing;
  double word_spacing;
  PdfTextStateFlag flags;
  _PdfTextState() {
    font = nullptr;
    font_size = 0;
    char_spacing = 0;
    word_spacing = 0;
    flags = 0;
  }
} PdfTextState;

typedef struct _PdfGraphicState {
  PdfColorState color_state;
  double line_width;
  double miter_limit;
  PdfLineCap line_cap;
  PdfLineJoin line_join;
  _PdfGraphicState() {
    line_width = 1;
    miter_limit = 0;
    line_cap = kPdfLineCapButt;
    line_join = kPdfLineJoinMiter;
  }
} PdfGraphicState;

typedef struct _PdfFontState {
  PdfFontType type;
  PdfFontFlags flags;
  PdfRect bbox;
  int ascent;
  int descent;
  int italic;
  int bold;
  int fixed_width;
  int vertical;
  int embedded;
  int height;
  _PdfFontState() {
    type = kFontUnknownType;
    flags = 0;
    ascent = 0;
    descent = 0;
    italic = 0;
    bold = 0;
    fixed_width = 0;
    vertical = 0;
    embedded = 0;
    height = 0;
  }
} PdfFontState;

typedef struct _PdfPageRenderParams {
  void* device;
  PsImage* image;
  PdfMatrix matrix;
  PdfRect clip_box;
  PdfRenderFlags render_flags;
  _PdfPageRenderParams() {
    device = 0;
    image = nullptr;
    clip_box.left = 0;
    clip_box.right = 0;
    clip_box.top = 0;
    clip_box.bottom = 0;
    render_flags = kRenderAnnot;
  }
} PdfPageRenderParams;

typedef struct _PdfAnnotAppearance {
  PdfRGB fill_color;
  PdfFillType fill_type;
  PdfRGB border_color;
  double border_width;
  PdfBorderStyle border;
  double opacity;
  double font_size;
  PdfTextAlignment text_align;
  _PdfAnnotAppearance() {
    fill_type = kFillTypeNone;
    border_width = 1;
    border = kBorderSolid;
    opacity = 1;
    font_size = 0;
    text_align = kTextAlignmentLeft;
  }
} PdfAnnotAppearance;

typedef struct _PdfBookmarkAppearance {
  PdfRGB color;
  int italic;
  int bold;
  _PdfBookmarkAppearance() {
    italic = 0;
    bold = 0;
  }
} PdfBookmarkAppearance;

typedef struct _PdfWhitespaceParams {
  double width;
  double height;
  _PdfWhitespaceParams() {
    width = 0;
    height = 0;
  }
} PdfWhitespaceParams;

typedef struct _PdfFlattenAnnotsParams {
  PdfPageRangeParams page_range;
  PdfAnnotSubtype subtype;
  _PdfFlattenAnnotsParams() {
    subtype = kAnnotUnknown;
  }
} PdfFlattenAnnotsParams;

typedef struct _PdfMediaQueryParams {
  PdfMediaType type;
  int min_width;
  _PdfMediaQueryParams() {
    type = kCSSMediaTypeAll;
    min_width = 1200;
  }
} PdfMediaQueryParams;

typedef struct _PdfImageParams {
  PdfImageFormat format;
  int quality;
  _PdfImageParams() {
    format = kImageFormatPng;
    quality = 100;
  }
} PdfImageParams;

typedef struct _PdfAccessibleParams {
  int accept_tags;
  int embed_fonts;
  int subset_fonts;
  _PdfAccessibleParams() {
    accept_tags = 0;
    embed_fonts = 0;
    subset_fonts = 0;
  }
} PdfAccessibleParams;

typedef int (*PdfCancelProc) (void* data);
typedef void (*PdfEventProc) (void* data);
typedef unsigned long (*PdfDigestDataProc) (int buffer_count, const unsigned char* buffer_to_sign[], unsigned long buffer_size[], unsigned char* sign_buff, unsigned long sign_buff_size, void* data);
typedef int (*PsStreamReadProc)(uint8_t* buffer, int offset, int size, void* data);
typedef int (*PsStreamWriteProc)(const uint8_t* buffer, int offset, int size, void* data);
typedef void (*PsStreamDestroyProc)(void* data);
typedef int (*PsStreamGetSizeProc)(void* data);

struct PdsObject {
  virtual PdfObjectType GetObjectType() = 0;
  virtual int GetId() = 0;
};

struct PdsBoolean : PdsObject {
  virtual bool GetValue() = 0;
};

struct PdsNumber : PdsObject {
  virtual bool IsIntegerValue() = 0;
  virtual int GetIntegerValue() = 0;
  virtual double GetValue() = 0;
};

struct PdsString : PdsObject {
  virtual int GetValue(_out_ char* buffer, int len) = 0;
  virtual int GetText(_out_ wchar_t* buffer, int len) = 0;
  std::wstring GetText() {
    std::wstring buffer;
    buffer.resize(GetText(nullptr, 0));
    GetText((wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
};

struct PdsName : PdsObject {
  virtual int GetValue(_out_ char* buffer, int len) = 0;
  virtual int GetText(_out_ wchar_t* buffer, int len) = 0;
  std::wstring GetText() {
    std::wstring buffer;
    buffer.resize(GetText(nullptr, 0));
    GetText((wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
};

struct PdsArray : PdsObject {
  virtual int GetNumObjects() = 0;
  virtual PdsObject* Get(int index) = 0;
  virtual bool Put(int index, PdsObject* value) = 0;
  virtual bool Insert(int index, PdsObject* value) = 0;
  virtual PdsDictionary* GetDictionary(int index) = 0;
  virtual PdsArray* GetArray(int index) = 0;
  virtual PdsStream* GetStream(int index) = 0;
  virtual int GetString(int index, _out_ char* buffer, int len) = 0;
  virtual int GetText(int index, _out_ wchar_t* buffer, int len) = 0;
  virtual double GetNumber(int index) = 0;
  virtual int GetInteger(int index) = 0;
  std::wstring GetText(int index) {
    std::wstring buffer;
    buffer.resize(GetText(index, nullptr, 0));
    GetText(index, (wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
};

struct PdsDictionary : PdsObject {
  virtual bool Known(const wchar_t* key) = 0;
  virtual int GetNumKeys() = 0;
  virtual int GetKey(int index, _out_ wchar_t* buffer, int len) = 0;
  virtual PdsObject* Get(const wchar_t* key) = 0;
  virtual bool Put(const wchar_t* key, PdsObject* value) = 0;
  virtual PdsDictionary* GetDictionary(const wchar_t* key) = 0;
  virtual PdsArray* GetArray(const wchar_t* key) = 0;
  virtual PdsStream* GetStream(const wchar_t* key) = 0;
  virtual int GetString(const wchar_t* key, _out_ char* buffer, int len) = 0;
  virtual int GetText(const wchar_t* key, _out_ wchar_t* buffer, int len) = 0;
  virtual double GetNumber(const wchar_t* key) = 0;
  virtual int GetInteger(const wchar_t* key, int default_value) = 0;
  virtual bool GetBoolean(const wchar_t* key, bool default_value) = 0;
  std::wstring GetKey(int index) {
    std::wstring buffer;
    buffer.resize(GetKey(index, nullptr, 0));
    GetKey(index, (wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
  std::wstring GetText(const wchar_t* key) {
    std::wstring buffer;
    buffer.resize(GetText(key, nullptr, 0));
    GetText(key, (wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
};

struct PdsStream : PdsObject {
  virtual PdsDictionary* GetStreamDict() = 0;
  virtual int GetRawDataSize() = 0;
  virtual bool IsEof() = 0;
  virtual int GetSize() = 0;
  virtual bool Read(int offset, _out_ uint8_t* buffer, int size) = 0;
  virtual int GetPos() = 0;
};

struct PdsNull : PdsObject {
};

struct PdsPageObject {
  virtual PdfPageObjectType GetObjectType() = 0;
  virtual void GetBBox(_out_ PdfRect* bbox) = 0;
  virtual int GetId() = 0;
  virtual void SetRender(bool render) = 0;
  virtual PdsObject* GetStructObject(bool struct_parent) = 0;
  virtual PdsContentMark* GetContentMark() = 0;
  virtual PdfPage* GetPage() = 0;
  PdfRect GetBBox() {
    PdfRect bbox;
    GetBBox(&bbox);
    return bbox;
  }
};

struct PdsText : PdsPageObject {
  virtual int GetText(_out_ wchar_t* buffer, int len) = 0;
  virtual bool GetTextState(PdfDoc* doc, _out_ PdfTextState* text_state) = 0;
  std::wstring GetText() {
    std::wstring buffer;
    buffer.resize(GetText(nullptr, 0));
    GetText((wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
  PdfTextState GetTextState(PdfDoc* doc) {
    PdfTextState text_state;
    GetTextState(doc, &text_state);
    return text_state;
  }
};

struct PdsForm : PdsPageObject {
  virtual int GetNumPageObjects() = 0;
  virtual PdsPageObject* GetPageObject (int index) = 0;
};

struct PdsPath : PdsPageObject {
};

struct PdsImage : PdsPageObject {
};

struct PdsShading : PdsPageObject {
};

struct PdsContentMark {
  virtual int GetNumTags() = 0;
  virtual int GetTagName(int index, _out_ wchar_t* buffer, int len) = 0;
  virtual PdsDictionary* GetTagObject(int index) = 0;
  virtual int GetTagMcid() = 0;
  virtual bool GetTagArtifact() = 0;
  virtual bool AddTag(const char* name, PdsDictionary* object, bool indirect) = 0;
  virtual bool RemoveTag(int index) = 0;
  std::wstring GetTagName(int index) {
    std::wstring buffer;
    buffer.resize(GetTagName(index, nullptr, 0));
    GetTagName(index, (wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
};

struct PdeElement {
  virtual PdfElementType GetType() = 0;
  virtual void GetBBox(_out_ PdfRect* bbox) = 0;
  virtual bool SetBBox(const PdfRect* bbox) = 0;
  virtual int GetId() = 0;
  virtual void GetGraphicState(_out_ PdfGraphicState* g_state) = 0;
  virtual int GetNumChildren() = 0;
  virtual PdeElement* GetChild(int index) = 0;
  virtual PdfAlignment GetAlignment() = 0;
  virtual double GetAngle() = 0;
  virtual void SetRender(bool render) = 0;
  virtual void SetData(void* data) = 0;
  virtual void* GetData() = 0;
  virtual bool SetAlt(const wchar_t* alt) = 0;
  virtual bool SetActualText(const wchar_t* text) = 0;
  virtual int GetFlags() = 0;
  virtual bool SetFlags(int flags) = 0;
  PdfRect GetBBox() {
    PdfRect bbox;
    GetBBox(&bbox);
    return bbox;
  }
  PdfGraphicState GetGraphicState() {
    PdfGraphicState g_state;
    GetGraphicState(&g_state);
    return g_state;
  }
};

struct PdeContainer : PdeElement {
};

struct PdeList : PdeElement {
};

struct PdeToc : PdeContainer {
};

struct PdeAnnot : PdeElement {
  virtual PdfAnnot* GetAnnot() = 0;
};

struct PdeFormField : PdeAnnot {
};

struct PdeImage : PdeContainer {
  virtual PdfImageType GetImageType() = 0;
  virtual PdeElement* GetCaption() = 0;
};

struct PdeLine : PdeElement {
};

struct PdeRect : PdeContainer {
};

struct PdeHeader : PdeContainer {
};

struct PdeFooter : PdeContainer {
};

struct PdeCell : PdeContainer {
  virtual int GetRowSpan() = 0;
  virtual int GetColSpan() = 0;
  virtual bool HasBorderGraphicState(int index) = 0;
  virtual PdeCell* GetSpanCell() = 0;
};

struct PdeTable : PdeContainer {
  virtual int GetNumRows() = 0;
  virtual int GetNumCols() = 0;
  virtual PdeCell* GetCell(int row, int col) = 0;
  virtual PdfAlignment GetRowAlignment(int row) = 0;
  virtual PdfAlignment GetColAlignment(int col) = 0;
  virtual PdeElement* GetCaption() = 0;
  virtual PdfTableType GetTableType() = 0;
};

struct PdeWord : PdeElement {
  virtual int GetText(_out_ wchar_t* buffer, int len) = 0;
  virtual bool HasTextState() = 0;
  virtual void GetTextState(_out_ PdfTextState* text_state) = 0;
  virtual int GetNumChars() = 0;
  virtual int GetCharCode(int index) = 0;
  virtual int GetCharText(int index, _out_ wchar_t* buffer, int len) = 0;
  virtual void GetCharTextState(int index, _out_ PdfTextState* text_state) = 0;
  virtual void GetCharBBox(int index, _out_ PdfRect* bbox) = 0;
  virtual int GetWordFlags() = 0;
  virtual PdeElement* GetBackground() = 0;
  virtual void GetOrigin(_out_ PdfPoint* point) = 0;
  std::wstring GetText() {
    std::wstring buffer;
    buffer.resize(GetText(nullptr, 0));
    GetText((wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
  PdfTextState GetTextState() {
    PdfTextState text_state;
    GetTextState(&text_state);
    return text_state;
  }
  std::wstring GetCharText(int index) {
    std::wstring buffer;
    buffer.resize(GetCharText(index, nullptr, 0));
    GetCharText(index, (wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
  PdfTextState GetCharTextState(int index) {
    PdfTextState text_state;
    GetCharTextState(index, &text_state);
    return text_state;
  }
  PdfRect GetCharBBox(int index) {
    PdfRect bbox;
    GetCharBBox(index, &bbox);
    return bbox;
  }
  PdfPoint GetOrigin() {
    PdfPoint point;
    GetOrigin(&point);
    return point;
  }
};

struct PdeTextLine : PdeElement {
  virtual int GetText(_out_ wchar_t* buffer, int len) = 0;
  virtual bool HasTextState() = 0;
  virtual void GetTextState(_out_ PdfTextState* text_state) = 0;
  virtual int GetNumWords() = 0;
  virtual PdeWord* GetWord(int index) = 0;
  virtual int GetTextLineFlags() = 0;
  std::wstring GetText() {
    std::wstring buffer;
    buffer.resize(GetText(nullptr, 0));
    GetText((wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
  PdfTextState GetTextState() {
    PdfTextState text_state;
    GetTextState(&text_state);
    return text_state;
  }
};

struct PdeText : PdeElement {
  virtual int GetText(_out_ wchar_t* buffer, int len) = 0;
  virtual bool HasTextState() = 0;
  virtual void GetTextState(_out_ PdfTextState* text_state) = 0;
  virtual int GetNumTextLines() = 0;
  virtual PdeTextLine* GetTextLine(int index) = 0;
  virtual int GetNumWords() = 0;
  virtual PdeWord* GetWord(int index) = 0;
  virtual double GetLineSpacing() = 0;
  virtual double GetIndent() = 0;
  virtual PdfTextStyle GetTextStyle() = 0;
  virtual PdfTextRegexFlags GetTextFlags() = 0;
  virtual int GetLabelLevel() = 0;
  virtual bool SetLabelLevel(int level) = 0;
  std::wstring GetText() {
    std::wstring buffer;
    buffer.resize(GetText(nullptr, 0));
    GetText((wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
  PdfTextState GetTextState() {
    PdfTextState text_state;
    GetTextState(&text_state);
    return text_state;
  }
};

struct PdfAction {
  virtual PdfActionType GetSubtype() = 0;
  virtual int GetJavaScript(_out_ wchar_t* buffer, int len) = 0;
  virtual int GetDestFile(_out_ wchar_t* buffer, int len) = 0;
  virtual PdfViewDestination* GetViewDestination() = 0;
  std::wstring GetJavaScript() {
    std::wstring buffer;
    buffer.resize(GetJavaScript(nullptr, 0));
    GetJavaScript((wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
  std::wstring GetDestFile() {
    std::wstring buffer;
    buffer.resize(GetDestFile(nullptr, 0));
    GetDestFile((wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
};

struct PdfAnnot {
  virtual PdfAnnotSubtype GetSubtype() = 0;
  virtual PdfAnnotFlags GetFlags() = 0;
  virtual void GetAppearance(_out_ PdfAnnotAppearance* appearance) = 0;
  virtual void GetBBox(_out_ PdfRect* bbox) = 0;
  virtual bool PointInAnnot(PdfPoint* point) = 0;
  virtual bool RectInAnnot(PdfRect* rect) = 0;
  virtual PdsObject* GetStructObject(bool struct_parent) = 0;
  virtual PdsDictionary* GetObject() = 0;
  PdfAnnotAppearance GetAppearance() {
    PdfAnnotAppearance appearance;
    GetAppearance(&appearance);
    return appearance;
  }
  PdfRect GetBBox() {
    PdfRect bbox;
    GetBBox(&bbox);
    return bbox;
  }
};

struct PdfLinkAnnot : PdfAnnot {
  virtual int GetNumQuads() = 0;
  virtual void GetQuad(int index, _out_ PdfQuad* quad) = 0;
  virtual bool AddQuad(PdfQuad* quad) = 0;
  virtual bool RemoveQuad(int index) = 0;
  virtual PdfAction* GetAction() = 0;
  PdfQuad GetQuad(int index) {
    PdfQuad quad;
    GetQuad(index, &quad);
    return quad;
  }
};

struct PdfMarkupAnnot : PdfAnnot {
  virtual int GetContents(_out_ wchar_t* buffer, int len) = 0;
  virtual bool SetContents(const wchar_t* contents) = 0;
  virtual int GetAuthor(_out_ wchar_t* buffer, int len) = 0;
  virtual bool SetAuthor(const wchar_t* author) = 0;
  virtual int GetNumReplies() = 0;
  virtual PdfAnnot* GetReply(int index) = 0;
  virtual PdfAnnot* AddReply(const wchar_t* author, const wchar_t* text) = 0;
  std::wstring GetContents() {
    std::wstring buffer;
    buffer.resize(GetContents(nullptr, 0));
    GetContents((wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
  std::wstring GetAuthor() {
    std::wstring buffer;
    buffer.resize(GetAuthor(nullptr, 0));
    GetAuthor((wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
};

struct PdfTextAnnot : PdfMarkupAnnot {
};

struct PdfTextMarkupAnnot : PdfMarkupAnnot {
  virtual int GetNumQuads() = 0;
  virtual void GetQuad(int index, _out_ PdfQuad* quad) = 0;
  virtual bool AddQuad(PdfQuad* quad) = 0;
  virtual bool RemoveQuad(int index) = 0;
  PdfQuad GetQuad(int index) {
    PdfQuad quad;
    GetQuad(index, &quad);
    return quad;
  }
};

struct PdfWidgetAnnot : PdfAnnot {
  virtual int GetCaption(_out_ wchar_t* buffer, int len) = 0;
  virtual int GetFontName(_out_ wchar_t* buffer, int len) = 0;
  virtual PdfAction* GetAction() = 0;
  virtual PdfAction* GetAAction(PdfActionEventType event) = 0;
  virtual PdfFormField* GetFormField() = 0;
  std::wstring GetCaption() {
    std::wstring buffer;
    buffer.resize(GetCaption(nullptr, 0));
    GetCaption((wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
  std::wstring GetFontName() {
    std::wstring buffer;
    buffer.resize(GetFontName(nullptr, 0));
    GetFontName((wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
};

struct PdfViewDestination {
  virtual int GetPageNum(PdfDoc* doc) = 0;
  virtual PdfDestFitType GetFitType() = 0;
  virtual void GetBBox(_out_ PdfRect* bbox) = 0;
  virtual double GetZoom() = 0;
  virtual PdfViewDestination* Resolve(PdfDoc* doc) = 0;
  virtual bool IsNamed() = 0;
  virtual int GetName(_out_ wchar_t* buffer, int len) = 0;
  virtual PdsObject* GetObject() = 0;
  PdfRect GetBBox() {
    PdfRect bbox;
    GetBBox(&bbox);
    return bbox;
  }
  std::wstring GetName() {
    std::wstring buffer;
    buffer.resize(GetName(nullptr, 0));
    GetName((wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
};

struct PdfBaseDigSig {
  virtual void Destroy() = 0;
  virtual bool SetReason(const wchar_t* reason) = 0;
  virtual bool SetLocation(const wchar_t* location) = 0;
  virtual bool SetContactInfo(const wchar_t* contact) = 0;
  virtual bool SetName(const wchar_t* name) = 0;
  virtual bool SetTimeStampServer(const wchar_t* url, const wchar_t* user_name, const wchar_t* password) = 0;
  virtual bool SignDoc(PdfDoc* doc, const wchar_t* path) = 0;
};

struct PdfDigSig : PdfBaseDigSig {
  virtual bool SetPfxFile(const wchar_t* pfx_file, const wchar_t* pfx_password) = 0;
};

#if defined _WIN32 && defined _MSC_VER 
struct PdfCertDigSig : PdfBaseDigSig {
  virtual bool SetPfxFile(const wchar_t* pfx_file, const wchar_t* pfx_password) = 0;
  virtual bool SetCertContext(void* cert_context) = 0;
};
#endif

struct PdfCustomDigSig : PdfBaseDigSig {
  virtual bool RegisterDigestDataProc(_callback_ PdfDigestDataProc proc, void* data) = 0;
};

struct PdfDoc {
  virtual bool Save(const wchar_t* path, PdfSaveFlags flags) = 0;
  virtual bool SaveToStream(PsStream* stream, PdfSaveFlags flags) = 0;
  virtual bool Close() = 0;
  virtual bool AddWatermarkFromImage(PdfWatermarkParams* params, const wchar_t* path) = 0;
  virtual int GetNumPages() = 0;
  virtual PdfPage* AcquirePage(int page_num) = 0;
  virtual PdfPage* CreatePage(int index, const PdfRect* media_box) = 0;
  virtual bool DeletePages(int index_from, int index_to, _callback_ PdfCancelProc cancel_proc, void* cancel_data) = 0;
  virtual bool InsertPages(int index, const PdfDoc* doc, int index_from, int index_to, PdfPageInsertFlags insert_flags, _callback_ PdfCancelProc cancel_proc, void* cancel_data) = 0;
  virtual bool MovePage(int index_to, int index_from) = 0;
  virtual int GetNumDocumentJavaScripts() = 0;
  virtual int GetDocumentJavaScript(int index, _out_ wchar_t* buffer, int len) = 0;
  virtual int GetDocumentJavaScriptName(int index, _out_ wchar_t* buffer, int len) = 0;
  virtual int GetNumCalculatedFormFields() = 0;
  virtual PdfFormField* GetCalculatedFormField(int index) = 0;
  virtual int GetNumFormFields() = 0;
  virtual PdfFormField* GetFormField(int index) = 0;
  virtual PdfFormField* GetFormFieldByName(const wchar_t* buffer) = 0;
  virtual int GetInfo(const wchar_t* key, _out_ wchar_t* buffer, int len) = 0;
  virtual bool SetInfo(const wchar_t* key, const wchar_t* info) = 0;
  virtual PdfBookmark* GetBookmarkRoot() = 0;
  virtual bool FlattenAnnots(PdfFlattenAnnotsParams* params) = 0;
  virtual int GetNumAlternates() = 0;
  virtual PdfAlternate* AcquireAlternate(int index) = 0;
  virtual PdsObject* CreatePdsObject(PdfObjectType type, bool indirect) = 0;
  virtual bool AddTags(_callback_ PdfCancelProc cancel_proc, void* cancel_data) = 0;
  virtual bool RemoveTags(_callback_ PdfCancelProc cancel_proc, void* cancel_data) = 0;
  virtual PdfDocTemplate* GetDocTemplate() = 0;
  virtual PsMetadata* GetMetadata() = 0;
  virtual int GetLang(_out_ wchar_t* buffer, int len) = 0;
  virtual bool SetLang(const wchar_t* lang) = 0;
  virtual bool EmbedFonts(bool subset, _callback_ PdfCancelProc cancel_proc, void* cancel_data) = 0;
  virtual bool MakeAccessible(PdfAccessibleParams* params, _callback_ PdfCancelProc cancel_proc, void* cancel_data) = 0;
  virtual PdsDictionary* GetRootObject() = 0;
  virtual PdsDictionary* GetInfoObject() = 0;
  virtual PdsDictionary* CreateDictObject(bool indirect) = 0;
  virtual PdsArray* CreateArrayObject(bool indirect) = 0;
  virtual PdsName* CreateNameObject(bool indirect, const wchar_t* value) = 0;
  virtual PdsString* CreateStringObject(bool indirect, const wchar_t* value, bool hex) = 0;
  virtual PdsNumber* CreateIntObject(bool indirect, int value) = 0;
  virtual PdsNumber* CreateNumberObject(bool indirect, double value) = 0;
  virtual PdsStream* CreateStreamObject(bool indirect, PdsDictionary* dict, _in_ const uint8_t* buffer, int size) = 0;
  virtual PdsObject* GetObjectById(int obj_id) = 0;
  virtual PdsStructTree* CreateStructTree() = 0;
  virtual PdsStructTree* GetStructTree() = 0;
  virtual void RemoveStructTree() = 0;
  virtual void RemoveBookmarks() = 0;
  virtual bool CreateBookmarks(_callback_ PdfCancelProc cancel_proc, void* cancel_data) = 0;
  virtual bool AddFontMissingUnicode(_callback_ PdfCancelProc cancel_proc, void* cancel_data) = 0;
  virtual PdfNameTree* GetNameTree(const wchar_t* name, bool create) = 0;
  virtual void RemoveNameTree(const wchar_t* name) = 0;
  std::wstring GetDocumentJavaScript(int index) {
    std::wstring buffer;
    buffer.resize(GetDocumentJavaScript(index, nullptr, 0));
    GetDocumentJavaScript(index, (wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
  std::wstring GetDocumentJavaScriptName(int index) {
    std::wstring buffer;
    buffer.resize(GetDocumentJavaScriptName(index, nullptr, 0));
    GetDocumentJavaScriptName(index, (wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
  std::wstring GetInfo(const wchar_t* key) {
    std::wstring buffer;
    buffer.resize(GetInfo(key, nullptr, 0));
    GetInfo(key, (wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
  std::wstring GetLang() {
    std::wstring buffer;
    buffer.resize(GetLang(nullptr, 0));
    GetLang((wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
};

struct PdfDocTemplate {
  virtual bool PreflightDoc(_callback_ PdfCancelProc cancel_proc, void* cancel_data) = 0;
  virtual bool LoadFromStream(PsStream* stream, PsDataFormat format) = 0;
  virtual bool SaveToStream(PsStream* stream, PsDataFormat format) = 0;
  virtual bool SetDefaults() = 0;
  virtual double GetProperty(const wchar_t* name) = 0;
  virtual bool SetProperty(const wchar_t* name, double value) = 0;
  virtual int GetRegex(const wchar_t* name, _out_ wchar_t* buffer, int len) = 0;
  virtual bool SetRegex(const wchar_t* name, const wchar_t* pattern) = 0;
  std::wstring GetRegex(const wchar_t* name) {
    std::wstring buffer;
    buffer.resize(GetRegex(name, nullptr, 0));
    GetRegex(name, (wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
};

struct PdfAlternate {
  virtual PdfAlternateType GetSubtype() = 0;
  virtual int GetName(_out_ wchar_t* buffer, int len) = 0;
  virtual int GetDescription(_out_ wchar_t* buffer, int len) = 0;
  virtual int GetFileName(_out_ wchar_t* buffer, int len) = 0;
  virtual bool SaveContent(const wchar_t* path) = 0;
  virtual void Release() = 0;
  std::wstring GetName() {
    std::wstring buffer;
    buffer.resize(GetName(nullptr, 0));
    GetName((wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
  std::wstring GetDescription() {
    std::wstring buffer;
    buffer.resize(GetDescription(nullptr, 0));
    GetDescription((wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
  std::wstring GetFileName() {
    std::wstring buffer;
    buffer.resize(GetFileName(nullptr, 0));
    GetFileName((wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
};

struct PdfHtmlAlternate : PdfAlternate {
  virtual bool SaveResource(const wchar_t* resource_name, const wchar_t* path) = 0;
};

struct PdfFont {
  virtual int GetFontName(_out_ wchar_t* buffer, int len) = 0;
  virtual int GetFaceName(_out_ wchar_t* buffer, int len) = 0;
  virtual void GetFontState(_out_ PdfFontState* font_state) = 0;
  virtual int GetSystemFontName(_out_ wchar_t* buffer, int len) = 0;
  virtual PdfFontCharset GetSystemFontCharset() = 0;
  virtual bool GetSystemFontBold() = 0;
  virtual bool GetSystemFontItalic() = 0;
  virtual bool SaveToStream(PsStream* stream, PdfFontFormat format) = 0;
  std::wstring GetFontName() {
    std::wstring buffer;
    buffer.resize(GetFontName(nullptr, 0));
    GetFontName((wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
  std::wstring GetFaceName() {
    std::wstring buffer;
    buffer.resize(GetFaceName(nullptr, 0));
    GetFaceName((wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
  PdfFontState GetFontState() {
    PdfFontState font_state;
    GetFontState(&font_state);
    return font_state;
  }
  std::wstring GetSystemFontName() {
    std::wstring buffer;
    buffer.resize(GetSystemFontName(nullptr, 0));
    GetSystemFontName((wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
};

struct PdfFormField {
  virtual PdfFieldType GetType() = 0;
  virtual PdfFieldFlags GetFlags() = 0;
  virtual bool SetFlags(PdfFieldFlags flags) = 0;
  virtual int GetValue(_out_ wchar_t* buffer, int len) = 0;
  virtual bool SetValue(const wchar_t* value) = 0;
  virtual int GetDefaultValue(_out_ wchar_t* buffer, int len) = 0;
  virtual int GetFullName(_out_ wchar_t* buffer, int len) = 0;
  virtual int GetTooltip(_out_ wchar_t* buffer, int len) = 0;
  virtual int GetOptionCount() = 0;
  virtual int GetOptionValue(int index, _out_ wchar_t* buffer, int len) = 0;
  virtual int GetOptionCaption(int index, _out_ wchar_t* buffer, int len) = 0;
  virtual PdfAction* GetAction() = 0;
  virtual PdfAction* GetAAction(PdfActionEventType event) = 0;
  virtual int GetMaxLength() = 0;
  virtual int GetWidgetExportValue(PdfAnnot* annot, _out_ wchar_t* buffer, int len) = 0;
  virtual PdsDictionary* GetObject() = 0;
  virtual int GetNumExportValues() = 0;
  virtual int GetExportValue(int index, _out_ wchar_t* buffer, int len) = 0;
  std::wstring GetValue() {
    std::wstring buffer;
    buffer.resize(GetValue(nullptr, 0));
    GetValue((wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
  std::wstring GetDefaultValue() {
    std::wstring buffer;
    buffer.resize(GetDefaultValue(nullptr, 0));
    GetDefaultValue((wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
  std::wstring GetFullName() {
    std::wstring buffer;
    buffer.resize(GetFullName(nullptr, 0));
    GetFullName((wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
  std::wstring GetTooltip() {
    std::wstring buffer;
    buffer.resize(GetTooltip(nullptr, 0));
    GetTooltip((wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
  std::wstring GetOptionValue(int index) {
    std::wstring buffer;
    buffer.resize(GetOptionValue(index, nullptr, 0));
    GetOptionValue(index, (wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
  std::wstring GetOptionCaption(int index) {
    std::wstring buffer;
    buffer.resize(GetOptionCaption(index, nullptr, 0));
    GetOptionCaption(index, (wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
  std::wstring GetWidgetExportValue(PdfAnnot* annot) {
    std::wstring buffer;
    buffer.resize(GetWidgetExportValue(annot, nullptr, 0));
    GetWidgetExportValue(annot, (wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
  std::wstring GetExportValue(int index) {
    std::wstring buffer;
    buffer.resize(GetExportValue(index, nullptr, 0));
    GetExportValue(index, (wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
};

struct PsImage {
  virtual void Destroy() = 0;
  virtual bool Save(const wchar_t* path, PdfImageParams* params) = 0;
  virtual bool SaveRect(const wchar_t* path, PdfImageParams* params, PdfDevRect* dev_rect) = 0;
  virtual bool SaveToStream(PsStream* stream, PdfImageParams* params) = 0;
  virtual bool SaveRectToStream(PsStream* stream, PdfImageParams* params, PdfDevRect* dev_rect) = 0;
  virtual void GetPointColor(PdfDevPoint* point, _out_ PdfRGB* color) = 0;
  virtual bool SaveDataToStream(PsStream* stream) = 0;
  PdfRGB GetPointColor(PdfDevPoint* point) {
    PdfRGB color;
    GetPointColor(point, &color);
    return color;
  }
};

struct PdfPage {
  virtual void Release() = 0;
  virtual void GetCropBox(_out_ PdfRect* crop_box) = 0;
  virtual void GetMediaBox(_out_ PdfRect* media_box) = 0;
  virtual PdfRotate GetRotate() = 0;
  virtual PdfRotate GetLogicalRotate() = 0;
  virtual void GetDefaultMatrix(_out_ PdfMatrix* matrix) = 0;
  virtual int GetNumber() = 0;
  virtual PdePageMap* CreatePageMap() = 0;
  virtual PdePageMap* AcquirePageMap(_callback_ PdfCancelProc cancel_proc, void* cancel_data) = 0;
  virtual PdfPageView* AcquirePageView(double zoom, PdfRotate rotate) = 0;
  virtual int GetNumAnnots() = 0;
  virtual PdfAnnot* GetAnnot(int index) = 0;
  virtual bool RemoveAnnot(int index, PdfRemoveAnnotFlags flags) = 0;
  virtual PdfTextAnnot* AddTextAnnot(int index, PdfRect* rect) = 0;
  virtual PdfLinkAnnot* AddLinkAnnot(int index, PdfRect* rect) = 0;
  virtual PdfTextMarkupAnnot* AddTextMarkupAnnot(int index, PdfRect* rect, PdfAnnotSubtype subtype) = 0;
  virtual PdfAnnot* AddAnnot(int index, PdfRect* rect, PdfAnnotSubtype subtype) = 0;
  virtual int GetNumAnnotsAtPoint(PdfPoint* point) = 0;
  virtual PdfAnnot* GetAnnotAtPoint(PdfPoint* point, int index) = 0;
  virtual int GetNumAnnotsAtRect(PdfRect* rect) = 0;
  virtual PdfAnnot* GetAnnotAtRect(PdfRect* rect, int index) = 0;
  virtual bool DrawContent(PdfPageRenderParams* params, _callback_ PdfCancelProc cancel_proc, void* cancel_data) = 0;
  virtual int GetNumPageObjects() = 0;
  virtual PdsPageObject* GetPageObject (int index) = 0;
  virtual PdsDictionary* GetResources(const wchar_t* res_type, bool create) = 0;
  virtual PdsDictionary* GetObject() = 0;
  virtual bool FlattenFormXObjects() = 0;
  virtual PdfPageContentFlags GetContentFlags() = 0;
  virtual bool SetContent() = 0;
  virtual PdfDoc* GetDoc() = 0;
  PdfRect GetCropBox() {
    PdfRect crop_box;
    GetCropBox(&crop_box);
    return crop_box;
  }
  PdfRect GetMediaBox() {
    PdfRect media_box;
    GetMediaBox(&media_box);
    return media_box;
  }
  PdfMatrix GetDefaultMatrix() {
    PdfMatrix matrix;
    GetDefaultMatrix(&matrix);
    return matrix;
  }
};

struct PdePageMap {
  virtual void Release() = 0;
  virtual PdeElement* GetElement() = 0;
  virtual bool GetWhitespace(PdfWhitespaceParams* params, int index, _out_ PdfRect* bbox) = 0;
  virtual void GetBBox(PdfRect* bbox) = 0;
  virtual bool AcquireElements(_callback_ PdfCancelProc cancel_proc, void* cancel_data) = 0;
  virtual PdeElement* CreateElement(PdfElementType type, PdeElement* parent) = 0;
  virtual bool AddTags(PdsStructElement* element, _callback_ PdfCancelProc cancel_proc, void* cancel_data) = 0;
  PdfRect GetWhitespace(PdfWhitespaceParams* params, int index) {
    PdfRect bbox;
    GetWhitespace(params, index, &bbox);
    return bbox;
  }
};

struct PdfPageView {
  virtual void Release() = 0;
  virtual int GetDeviceWidth() = 0;
  virtual int GetDeviceHeight() = 0;
  virtual void GetDeviceMatrix(_out_ PdfMatrix* matrix) = 0;
  virtual void RectToDevice(PdfRect* rect, _out_ PdfDevRect* dev_rect) = 0;
  virtual void PointToDevice(PdfPoint* point, _out_ PdfDevPoint* dev_point) = 0;
  virtual void RectToPage(PdfDevRect* dev_rect, _out_ PdfRect* rect) = 0;
  virtual void PointToPage(PdfDevPoint* dev_point, _out_ PdfPoint* point) = 0;
  PdfMatrix GetDeviceMatrix() {
    PdfMatrix matrix;
    GetDeviceMatrix(&matrix);
    return matrix;
  }
  PdfDevRect RectToDevice(PdfRect* rect) {
    PdfDevRect dev_rect;
    RectToDevice(rect, &dev_rect);
    return dev_rect;
  }
  PdfDevPoint PointToDevice(PdfPoint* point) {
    PdfDevPoint dev_point;
    PointToDevice(point, &dev_point);
    return dev_point;
  }
  PdfRect RectToPage(PdfDevRect* dev_rect) {
    PdfRect rect;
    RectToPage(dev_rect, &rect);
    return rect;
  }
  PdfPoint PointToPage(PdfDevPoint* dev_point) {
    PdfPoint point;
    PointToPage(dev_point, &point);
    return point;
  }
};

struct PdfBookmark {
  virtual int GetTitle(_out_ wchar_t* buffer, int len) = 0;
  virtual void GetAppearance(PdfBookmarkAppearance* appearance) = 0;
  virtual PdfAction* GetAction() = 0;
  virtual int GetNumChildren() = 0;
  virtual PdfBookmark* GetChild(int index) = 0;
  virtual PdfBookmark* GetParent() = 0;
  std::wstring GetTitle() {
    std::wstring buffer;
    buffer.resize(GetTitle(nullptr, 0));
    GetTitle((wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
};

struct PdfNameTree {
  virtual PdsObject* GetObject() = 0;
  virtual PdsObject* Lookup(const wchar_t* name) = 0;
};

struct PsRegex {
  virtual void Destroy() = 0;
  virtual bool SetPattern(const wchar_t* pattern) = 0;
  virtual bool Search(const wchar_t* text, int position) = 0;
  virtual int GetText(_out_ wchar_t* buffer, int len) = 0;
  virtual int GetPosition() = 0;
  virtual int GetLength() = 0;
  virtual int GetNumMatches() = 0;
  virtual int GetMatchText(int index, _out_ wchar_t* buffer, int len) = 0;
  std::wstring GetText() {
    std::wstring buffer;
    buffer.resize(GetText(nullptr, 0));
    GetText((wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
  std::wstring GetMatchText(int index) {
    std::wstring buffer;
    buffer.resize(GetMatchText(index, nullptr, 0));
    GetMatchText(index, (wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
};

struct PsStream {
  virtual void Destroy() = 0;
  virtual bool IsEof() = 0;
  virtual int GetSize() = 0;
  virtual bool Read(int offset, _out_ uint8_t* buffer, int size) = 0;
  virtual bool Write(int offset, _in_ const uint8_t* buffer, int size) = 0;
  virtual int GetPos() = 0;
  virtual bool Flush() = 0;
  virtual void* GetStream() = 0;
  virtual PdfStreamType GetType() = 0;
};

struct PsFileStream : PsStream {
};

struct PsMemoryStream : PsStream {
  virtual bool Resize(int size) = 0;
};

struct PsCustomStream : PsStream {
  virtual void SetReadProc(_callback_ PsStreamReadProc proc) = 0;
  virtual void SetWriteProc(_callback_ PsStreamWriteProc proc) = 0;
  virtual void SetDestroyProc(_callback_ PsStreamDestroyProc proc) = 0;
  virtual void SetGetSizeProc(_callback_ PsStreamGetSizeProc proc) = 0;
};

struct PdsStructElement {
  virtual void Release() = 0;
  virtual int GetType(bool mapped, _out_ wchar_t* buffer, int len) = 0;
  virtual int GetActualText(_out_ wchar_t* buffer, int len) = 0;
  virtual int GetAlt(_out_ wchar_t* buffer, int len) = 0;
  virtual bool SetAlt(const wchar_t* alt) = 0;
  virtual int GetAbbreviation(_out_ wchar_t* buffer, int len) = 0;
  virtual int GetPageNumber() = 0;
  virtual PdsObject* GetAttrObject(int index) = 0;
  virtual bool AddAttrObj(PdsObject* object) = 0;
  virtual void RemoveAttrObj() = 0;
  virtual PdsObject* GetElementObject() = 0;
  virtual PdsObject* GetKidObject(int index) = 0;
  virtual PdfStructElementType GetKidType(int index) = 0;
  virtual int GetKidPageNumber(int index) = 0;
  virtual int GetKidMcid(int index) = 0;
  virtual int GetNumAttrObjects() = 0;
  virtual int GetNumKids() = 0;
  virtual PdsObject* GetParentObject() = 0;
  virtual int GetTitle(_out_ wchar_t* buffer, int len) = 0;
  virtual int GetId(_out_ wchar_t* buffer, int len) = 0;
  virtual bool RemoveKid(int index) = 0;
  virtual PdsStructElement* CreateStructElement(const wchar_t* type, PdsStructElement* parent, int index) = 0;
  virtual bool SetParent(PdsStructElement* parent, int index) = 0;
  virtual void AddPageObject(PdsPageObject* object, int index) = 0;
  virtual void AddAnnot(PdfAnnot* annot, int index) = 0;
  virtual PdsStructTree* GetStructTree() = 0;
  virtual bool SetType(const wchar_t* type) = 0;
  std::wstring GetType(bool mapped) {
    std::wstring buffer;
    buffer.resize(GetType(mapped, nullptr, 0));
    GetType(mapped, (wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
  std::wstring GetActualText() {
    std::wstring buffer;
    buffer.resize(GetActualText(nullptr, 0));
    GetActualText((wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
  std::wstring GetAlt() {
    std::wstring buffer;
    buffer.resize(GetAlt(nullptr, 0));
    GetAlt((wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
  std::wstring GetAbbreviation() {
    std::wstring buffer;
    buffer.resize(GetAbbreviation(nullptr, 0));
    GetAbbreviation((wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
  std::wstring GetTitle() {
    std::wstring buffer;
    buffer.resize(GetTitle(nullptr, 0));
    GetTitle((wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
  std::wstring GetId() {
    std::wstring buffer;
    buffer.resize(GetId(nullptr, 0));
    GetId((wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
};

struct PdsClassMap {
  virtual PdsObject* GetAttrObject(const wchar_t* class_name, int index) = 0;
  virtual int GetNumAttrObjects(const wchar_t* class_name) = 0;
};

struct PdsRoleMap {
  virtual bool DoesMap(const wchar_t* src, const wchar_t* dst) = 0;
  virtual int GetDirectMap(const wchar_t* type, _out_ wchar_t* buffer, int len) = 0;
  std::wstring GetDirectMap(const wchar_t* type) {
    std::wstring buffer;
    buffer.resize(GetDirectMap(type, nullptr, 0));
    GetDirectMap(type, (wchar_t*)buffer.c_str(), (int)buffer.size());
    return buffer;
  }
};

struct PdsStructTree {
  virtual PdsDictionary* GetObject() = 0;
  virtual PdsClassMap* GetClassMap() = 0;
  virtual PdsObject* GetKidObject(int index) = 0;
  virtual int GetNumKids() = 0;
  virtual PdsRoleMap* GetRoleMap() = 0;
  virtual PdsStructElement* AcquireStructElement(PdsObject* object) = 0;
  virtual bool RemoveKid(int index) = 0;
  virtual PdsStructElement* CreateStructElement(const wchar_t* type, int index) = 0;
  virtual PdfDoc* GetDoc() = 0;
  virtual bool UpdateParentTree() = 0;
};

struct PsMetadata {
  virtual bool SaveToStream(PsStream* stream) = 0;
  virtual bool LoadFromStream(PsStream* stream) = 0;
};

struct PsEvent {
  virtual PdfEventType GetType() = 0;
  virtual PdfDoc* GetDoc() = 0;
  virtual PdfPage* GetPage() = 0;
  virtual PdfAnnot* GetAnnot() = 0;
};

struct PsAuthorization {
  virtual bool SaveToStream(PsStream* stream, PsDataFormat format) = 0;
  virtual bool IsAuthorized() = 0;
  virtual bool IsAuthorizedPlatform(PdfAuthPlatform platform)  = 0;
  virtual bool IsAuthorizedOption(PdfAuthOption option) = 0;
  virtual PdfAuthorizationType GetType() = 0;
};

struct PsAccountAuthorization : PsAuthorization {
  virtual bool Authorize(const wchar_t* email, const wchar_t* serial_number) = 0;
};

struct PsStandardAuthorization : PsAuthorization {
  virtual bool Activate(const wchar_t* serial_number) = 0;
  virtual bool Deactivate() = 0;
  virtual bool Update() = 0;
};

struct Pdfix {
  virtual void Destroy() = 0;
  virtual PsAuthorization* GetAuthorization() = 0;
  virtual PsStandardAuthorization* GetStandardAuthorization() = 0;
  virtual PsAccountAuthorization* GetAccountAuthorization() = 0;
  virtual int GetErrorType() = 0;
  virtual const char* GetError() = 0;
  virtual void SetError(int type, const char* error) = 0;
  virtual int GetVersionMajor() = 0;
  virtual int GetVersionMinor() = 0;
  virtual int GetVersionPatch() = 0;
  virtual PdfDoc* CreateDoc() = 0;
  virtual PdfDoc* OpenDoc(const wchar_t* path, const wchar_t* password) = 0;
  virtual PdfDoc* OpenDocFromStream(PsStream* stream, const wchar_t* password) = 0;
  virtual PdfDigSig* CreateDigSig() = 0;
#if defined _WIN32 && defined _MSC_VER
  virtual PdfCertDigSig* CreateCertDigSig() = 0;
#endif
  virtual PdfCustomDigSig* CreateCustomDigSig() = 0;
  virtual PsRegex* CreateRegex() = 0;
  virtual PsFileStream* CreateFileStream(const wchar_t* path, PsFileMode mode) = 0;
  virtual PsMemoryStream* CreateMemStream() = 0;
  virtual PsCustomStream* CreateCustomStream(_callback_ PsStreamReadProc read_proc, void* client_data) = 0;
  virtual bool RegisterEvent(PdfEventType type, _callback_ PdfEventProc proc, void* data) = 0;
  virtual bool UnregisterEvent(PdfEventType type, PdfEventProc proc, void* data) = 0;
  virtual PsImage* CreateImage(int width, int height, PsImageDIBFormat format) = 0;
  virtual void RegisterPlugin(PdfixPlugin* plugin, const wchar_t* name) = 0;
  virtual PdfixPlugin* GetPluginByName(const wchar_t* name) = 0;
  virtual PsEvent* GetEvent() = 0;
};

struct PdfixPlugin {
  virtual void Destroy() = 0;
  virtual bool Initialize(Pdfix* pdfix) = 0;
  virtual int GetVersionMajor() = 0;
  virtual int GetVersionMinor() = 0;
  virtual int GetVersionPatch() = 0;
  virtual int GetPdfixVersionMajor() = 0;
  virtual int GetPdfixVersionMinor() = 0;
  virtual int GetPdfixVersionPatch() = 0;
  virtual Pdfix* GetPdfix() = 0;
};

///////////////////////////////////////////////////////////////////////////////
// Pdfix initialization

#ifdef PDFIX_STATIC_LIB
Pdfix* GetPdfix();
#else

#ifdef _WIN32
#define DLL_HANDLE HMODULE
#define PdfixLoadLibrary LoadLibraryA
#define PdfixFreeLibrary FreeLibrary
#define PdfixGetProcAddress GetProcAddress
#else //UNIX
#include <dlfcn.h>
#define DLL_HANDLE void*
#define PdfixLoadLibrary(name) dlopen(name, RTLD_NOW)
#define PdfixFreeLibrary dlclose
#define PdfixGetProcAddress dlsym
#endif //  _WIN32

// method prototypes
typedef Pdfix* (*GetPdfixProcType)( );

// initialization
extern DLL_HANDLE* Pdfix_init(const char* path);
extern void Pdfix_destroy();
// static declarations
extern DLL_HANDLE g_Pdfix_handle;
extern GetPdfixProcType GetPdfix;

class PdfixException : public std::exception {
public:
  PdfixException(){}
  const char* what() const _NOEXCEPT override {
    return (GetPdfix ? GetPdfix()->GetError() : "unknown error");
  }
  int code() const {
    return (GetPdfix ? GetPdfix()->GetErrorType() : 1);
  }
};

// static definitions Pdfix (use in the source file)
#define Pdfix_statics \
namespace PDFixSDK { \
GetPdfixProcType GetPdfix = nullptr;\
DLL_HANDLE g_Pdfix_handle = nullptr;\
void Pdfix_destroy() {\
  if (g_Pdfix_handle) PdfixFreeLibrary(g_Pdfix_handle);\
  g_Pdfix_handle = nullptr;\
  GetPdfix = nullptr;\
}\
DLL_HANDLE* Pdfix_init(const char* path) {\
  if (g_Pdfix_handle == nullptr) g_Pdfix_handle = PdfixLoadLibrary(path);   if (!g_Pdfix_handle) return nullptr;\
  GetPdfix = (GetPdfixProcType)PdfixGetProcAddress(g_Pdfix_handle, "GetPdfix");\
  if (GetPdfix == nullptr) { Pdfix_destroy(); return nullptr; } return &g_Pdfix_handle; } \
} // namespace PDFixSDK

#ifndef Pdfix_MODULE_NAME
#if defined _WIN32
#define Pdfix_MODULE_NAME "pdfix.dll"
#elif defined __linux__
#define Pdfix_MODULE_NAME "./libpdfix.so"
#elif defined __APPLE__
#define Pdfix_MODULE_NAME "./libpdfix.dylib"
#else
#error unknown platform
#endif
#endif //Pdfix_MODULE_NAME

#endif // PDFIX_STATIC_LIB
} // namespace PDFixSDK
#endif //_Pdfix_h
