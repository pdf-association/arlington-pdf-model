////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 PDFix (http://pdfix.net). All Rights Reserved.
// This file was generated automatically
////////////////////////////////////////////////////////////////////////////////
#ifndef _PdfToHtml_h
#define _PdfToHtml_h

#include <stdint.h>
#include <vector>

#define _in_
#define _out_
#define _callback_

struct PdfToHtml;
struct PdfHtmlDoc;

typedef int PdfHtmlFlags;
typedef PdfToHtml* PdfToHtmlP;
typedef PdfHtmlDoc* PdfHtmlDocP;

enum {
  kErrorHtmlPdfDocInvalid = 1000,
  kErrorHtmlPageOutOfRange = 1001,
} ;

enum {
  kHtmlNone = 0x00,
  kHtmlExportJavaScripts = 0x0001,
  kHtmlExportFonts = 0x0002,
  kHtmlRetainFontSize = 0x0004,
  kHtmlRetainTextColor = 0x0008,
  kHtml41Support = 0x0010,
  kHtmlNoExternalCSS = 0x0020,
  kHtmlNoExternalJS = 0x0040,
  kHtmlNoExternalIMG = 0x0080,
  kHtmlNoExternalFONT = 0x0100,
  kHtmlGrayBackground = 0x0200,
} ;

typedef enum {
  kPdfHtmlFixed = 0,
  kPdfHtmlResponsive = 1,
} PdfHtmlType;


typedef struct _PdfHtmlParams {
  PdfHtmlFlags flags;
  int width;
  PdfHtmlType type;
  PdfImageParams image_params;
  _PdfHtmlParams() {
    flags = 0;
    width = 1200;
    type = kPdfHtmlFixed;
  }
} PdfHtmlParams;


struct PdfToHtml : PdfixPlugin {
  virtual PdfHtmlDoc* OpenHtmlDoc(PdfDoc* doc) = 0;
  virtual bool SaveCSS(PsStream* stream) = 0;
  virtual bool SaveJavaScript(PsStream* stream) = 0;
};

struct PdfHtmlDoc {
  virtual bool Close() = 0;
  virtual bool Save(const wchar_t* path, PdfHtmlParams* params, _callback_ PdfCancelProc cancel_proc, void* cancel_data) = 0;
  virtual bool SaveDocHtml(PsStream* stream, PdfHtmlParams* params, _callback_ PdfCancelProc cancel_proc, void* cancel_data) = 0;
  virtual bool SavePageHtml(PsStream* stream, PdfHtmlParams* params, int page_num, _callback_ PdfCancelProc cancel_proc, void* cancel_data) = 0;
};

///////////////////////////////////////////////////////////////////////////////
// PdfToHtml initialization

#ifdef PDFIX_STATIC_LIB
PdfToHtml* GetPdfToHtml();
#else

#ifdef _WIN32
#include <Windows.h>
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
typedef PdfToHtml* (*GetPdfToHtmlProcType)( );

// initialization
extern DLL_HANDLE* PdfToHtml_init(const char* path);
extern void PdfToHtml_destroy();
// static declarations
extern DLL_HANDLE g_PdfToHtml_handle;
extern GetPdfToHtmlProcType GetPdfToHtml;

// static definitions PdfToHtml (use in the source file)
#define PdfToHtml_statics \
GetPdfToHtmlProcType GetPdfToHtml = nullptr;\
DLL_HANDLE g_PdfToHtml_handle = 0;\
void PdfToHtml_destroy() {\
  if (g_PdfToHtml_handle) PdfixFreeLibrary(g_PdfToHtml_handle);\
  g_PdfToHtml_handle = nullptr;\
  GetPdfToHtml = 0;\
}\
DLL_HANDLE* PdfToHtml_init(const char* path) {\
  if (g_PdfToHtml_handle == nullptr) g_PdfToHtml_handle = PdfixLoadLibrary(path);   if (!g_PdfToHtml_handle) return nullptr;\
  GetPdfToHtml = (GetPdfToHtmlProcType)PdfixGetProcAddress(g_PdfToHtml_handle, "GetPdfToHtml");\
  if (GetPdfToHtml == nullptr) { PdfToHtml_destroy(); return nullptr; } return &g_PdfToHtml_handle; }

#ifndef PdfToHtml_MODULE_NAME
#if defined _WIN32
#if defined _WIN64
#define PdfToHtml_MODULE_NAME "pdf_to_html64.dll"
#else
#define PdfToHtml_MODULE_NAME "pdf_to_html.dll"
#endif
#elif defined __linux__
#if defined __x86_64__
#define PdfToHtml_MODULE_NAME "./libpdf_to_html64.so"
#else
#define PdfToHtml_MODULE_NAME "./libpdf_to_html.so"
#endif
#elif defined __APPLE__
#if defined __x86_64__
#define PdfToHtml_MODULE_NAME "./libpdf_to_html64.dylib"
#else
#define PdfToHtml_MODULE_NAME "./libpdf_to_html.dylib"
#endif
#else
#error unknown platform
#endif
#endif //PdfToHtml_MODULE_NAME

#endif // PDFIX_STATIC_LIB
#endif //_PdfToHtml_h
