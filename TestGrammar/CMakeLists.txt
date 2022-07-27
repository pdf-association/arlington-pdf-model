## Arlington PDF Model: TestGrammar C++ PoC
##
## In the ./TestGrammar folder:
##
## If using PDFix:
## $ export LD_LIBRARY_PATH=$PWD/bin/linux:$LD_LIBRARY_PATH
##
## Using Unix Makefile build system:
## $ cmake -B cmake-linux/debug -DPDFSDK_PDFIUM=ON -DCMAKE_BUILD_TYPE=Debug .
## $ cmake --build cmake-linux/debug --config Debug
## $ cmake --build cmake-linux/debug --target clean
##
## Can use CMAKE_BUILD_TYPE Release or RelWithDebInfo:
## $ cmake -B cmake-linux/release -DPDFSDK_PDFIX=ON -DCMAKE_BUILD_TYPE=Release .
## $ cmake --build cmake-linux/release --config Release
## $ cmake --build cmake-linux/release --target clean
##
## Using Ninja build system:
## $ cmake -G Ninja -B cmake-linux/debug -DPDFSDK_PDFIUM=ON -DCMAKE_BUILD_TYPE=Debug .
## $ ninja -C cmake-linux/debug
##
## Using Microsoft x64 Native Tools (DOS) build system:
## % cmake -G "" -B cmake-dos\debug -DPDFSDK_PDFIX=ON -DCMAKE_BUILD_TYPE=Debug .
## % cd cmake-dos\debug
## % nmake


cmake_minimum_required (VERSION 3.12)

PROJECT(TestGrammar)

## Pick which PDF SDK to use:
option(PDFSDK_PDFIX  "Use PDFix SDK"  OFF)
option(PDFSDK_PDFIUM "Use pdfium SDK" OFF)
option(PDFSDK_QPDF   "Use QPDF SDK"   OFF)

if((NOT PDFSDK_PDFIX) AND
   (NOT PDFSDK_PDFIUM) AND
   (NOT PDFSDK_QPDF))
        message(FATAL_ERROR "Must select which PDF SDK to use! Use -Dxx=ON with PDFSDK_PDFIX, PDFSDK_PDFIUM or PDFSDK_QPDF")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

#=========== PDFix ============

if(PDFSDK_PDFIX)
    message(STATUS "Building with PDFix")
    add_compile_definitions(ARL_PDFSDK_PDFIX)
    set(SRC_PDFSDK
        src/ArlingtonPDFShimPDFix.cpp
    )
endif()

#=========== QPDF =============

if(PDFSDK_QPDF)
    message(STATUS "Building with QPDF")
    add_compile_definitions(ARL_PDFSDK_QPDF)
    set(SRC_PDFSDK
        src/ArlingtonPDFShimQPDF.cpp
    )
endif()

#=========== pdfium ===========

if(PDFSDK_PDFIUM)
    message(STATUS "Building with pdfium")

    if(MSVC)
        #add_compile_options(/W3 /WX-)
        add_compile_definitions(BUILD_WINDOWS PNG_PREFIX PNG_USE_READ_MACROS OPJ_STATIC PNG_INTEL_SSE_OPT=1 NOMINMAX)
    else()
        add_compile_definitions(register=) # mac doesn't compile lcms with C++17 due to using register keyword
    endif()

    add_compile_definitions(ARL_PDFSDK_PDFIUM PNG_PREFIX PNG_USE_READ_MACROS OPJ_STATIC PNG_INTEL_SSE_OPT=1 NOMINMAX _CRT_NONSTDC_NO_DEPRECATE)

    set(SRC_PDFIUM
        src/ArlingtonPDFShimPDFium.cpp
        pdfium/core/src/fdrm/crypto/fx_crypt.cpp
        pdfium/core/src/fdrm/crypto/fx_crypt_aes.cpp
        pdfium/core/src/fdrm/crypto/fx_crypt_sha.cpp
        pdfium/core/src/fpdfapi/fpdf_basic_module.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/CNS1/Adobe-CNS1-UCS2_5.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/CNS1/B5pc-H_0.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/CNS1/B5pc-V_0.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/CNS1/cmaps_cns1.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/CNS1/CNS-EUC-H_0.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/CNS1/CNS-EUC-V_0.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/CNS1/ETen-B5-H_0.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/CNS1/ETen-B5-V_0.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/CNS1/ETenms-B5-H_0.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/CNS1/ETenms-B5-V_0.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/CNS1/HKscs-B5-H_5.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/CNS1/HKscs-B5-V_5.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/CNS1/UniCNS-UCS2-H_3.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/CNS1/UniCNS-UCS2-V_3.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/CNS1/UniCNS-UTF16-H_0.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/fpdf_cmaps.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/GB1/Adobe-GB1-UCS2_5.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/GB1/cmaps_gb1.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/GB1/GB-EUC-H_0.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/GB1/GB-EUC-V_0.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/GB1/GBK-EUC-H_2.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/GB1/GBK-EUC-V_2.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/GB1/GBK2K-H_5.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/GB1/GBK2K-V_5.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/GB1/GBKp-EUC-H_2.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/GB1/GBKp-EUC-V_2.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/GB1/GBpc-EUC-H_0.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/GB1/GBpc-EUC-V_0.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/GB1/UniGB-UCS2-H_4.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/GB1/UniGB-UCS2-V_4.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/Japan1/83pv-RKSJ-H_1.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/Japan1/90ms-RKSJ-H_2.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/Japan1/90ms-RKSJ-V_2.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/Japan1/90msp-RKSJ-H_2.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/Japan1/90msp-RKSJ-V_2.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/Japan1/90pv-RKSJ-H_1.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/Japan1/Add-RKSJ-H_1.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/Japan1/Add-RKSJ-V_1.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/Japan1/Adobe-Japan1-UCS2_4.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/Japan1/cmaps_japan1.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/Japan1/EUC-H_1.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/Japan1/EUC-V_1.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/Japan1/Ext-RKSJ-H_2.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/Japan1/Ext-RKSJ-V_2.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/Japan1/H_1.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/Japan1/UniJIS-UCS2-HW-H_4.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/Japan1/UniJIS-UCS2-HW-V_4.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/Japan1/UniJIS-UCS2-H_4.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/Japan1/UniJIS-UCS2-V_4.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/Japan1/UniJIS-UTF16-H_5.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/Japan1/UniJIS-UTF16-V_5.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/Japan1/V_1.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/Korea1/Adobe-Korea1-UCS2_2.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/Korea1/cmaps_korea1.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/Korea1/KSC-EUC-H_0.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/Korea1/KSC-EUC-V_0.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/Korea1/KSCms-UHC-HW-H_1.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/Korea1/KSCms-UHC-HW-V_1.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/Korea1/KSCms-UHC-H_1.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/Korea1/KSCms-UHC-V_1.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/Korea1/KSCpc-EUC-H_0.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/Korea1/UniKS-UCS2-H_1.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/Korea1/UniKS-UCS2-V_1.cpp
        pdfium/core/src/fpdfapi/fpdf_cmaps/Korea1/UniKS-UTF16-H_0.cpp
        pdfium/core/src/fpdfapi/fpdf_edit/fpdf_edit_content.cpp
        pdfium/core/src/fpdfapi/fpdf_edit/fpdf_edit_create.cpp
        pdfium/core/src/fpdfapi/fpdf_edit/fpdf_edit_doc.cpp
        pdfium/core/src/fpdfapi/fpdf_edit/fpdf_edit_image.cpp
        pdfium/core/src/fpdfapi/fpdf_font/fpdf_font.cpp
        pdfium/core/src/fpdfapi/fpdf_font/fpdf_font_charset.cpp
        pdfium/core/src/fpdfapi/fpdf_font/fpdf_font_cid.cpp
        pdfium/core/src/fpdfapi/fpdf_font/fpdf_font_utility.cpp
        pdfium/core/src/fpdfapi/fpdf_font/ttgsubtable.cpp
        pdfium/core/src/fpdfapi/fpdf_page/fpdf_page.cpp
        pdfium/core/src/fpdfapi/fpdf_page/fpdf_page_colors.cpp
        pdfium/core/src/fpdfapi/fpdf_page/fpdf_page_doc.cpp
        pdfium/core/src/fpdfapi/fpdf_page/fpdf_page_func.cpp
        pdfium/core/src/fpdfapi/fpdf_page/fpdf_page_graph_state.cpp
        pdfium/core/src/fpdfapi/fpdf_page/fpdf_page_image.cpp
        pdfium/core/src/fpdfapi/fpdf_page/fpdf_page_parser.cpp
        pdfium/core/src/fpdfapi/fpdf_page/fpdf_page_parser_new.cpp
        pdfium/core/src/fpdfapi/fpdf_page/fpdf_page_parser_old.cpp
        pdfium/core/src/fpdfapi/fpdf_page/fpdf_page_path.cpp
        pdfium/core/src/fpdfapi/fpdf_page/fpdf_page_pattern.cpp
        pdfium/core/src/fpdfapi/fpdf_parser/fpdf_parser_decode.cpp
        pdfium/core/src/fpdfapi/fpdf_parser/fpdf_parser_document.cpp
        pdfium/core/src/fpdfapi/fpdf_parser/fpdf_parser_encrypt.cpp
        pdfium/core/src/fpdfapi/fpdf_parser/fpdf_parser_fdf.cpp
        pdfium/core/src/fpdfapi/fpdf_parser/fpdf_parser_filters.cpp
        pdfium/core/src/fpdfapi/fpdf_parser/fpdf_parser_objects.cpp
        pdfium/core/src/fpdfapi/fpdf_parser/fpdf_parser_parser.cpp
        pdfium/core/src/fpdfapi/fpdf_parser/fpdf_parser_utility.cpp
        pdfium/core/src/fpdfapi/fpdf_render/fpdf_render.cpp
        pdfium/core/src/fpdfapi/fpdf_render/fpdf_render_cache.cpp
        pdfium/core/src/fpdfapi/fpdf_render/fpdf_render_image.cpp
        pdfium/core/src/fpdfapi/fpdf_render/fpdf_render_loadimage.cpp
        pdfium/core/src/fpdfapi/fpdf_render/fpdf_render_pattern.cpp
        pdfium/core/src/fpdfapi/fpdf_render/fpdf_render_text.cpp
        pdfium/core/src/fpdfdoc/doc_action.cpp
        pdfium/core/src/fpdfdoc/doc_annot.cpp
        pdfium/core/src/fpdfdoc/doc_ap.cpp
        pdfium/core/src/fpdfdoc/doc_basic.cpp
        pdfium/core/src/fpdfdoc/doc_bookmark.cpp
        pdfium/core/src/fpdfdoc/doc_form.cpp
        pdfium/core/src/fpdfdoc/doc_formcontrol.cpp
        pdfium/core/src/fpdfdoc/doc_formfield.cpp
        pdfium/core/src/fpdfdoc/doc_link.cpp
        pdfium/core/src/fpdfdoc/doc_metadata.cpp
        pdfium/core/src/fpdfdoc/doc_ocg.cpp
        pdfium/core/src/fpdfdoc/doc_tagged.cpp
        pdfium/core/src/fpdfdoc/doc_utils.cpp
        pdfium/core/src/fpdfdoc/doc_viewerPreferences.cpp
        pdfium/core/src/fpdfdoc/doc_vt.cpp
        pdfium/core/src/fpdfdoc/doc_vtmodule.cpp
        pdfium/core/src/fpdftext/fpdf_text.cpp
        pdfium/core/src/fpdftext/fpdf_text_int.cpp
        pdfium/core/src/fpdftext/fpdf_text_search.cpp
        pdfium/core/src/fpdftext/unicodenormalization.cpp
        pdfium/core/src/fpdftext/unicodenormalizationdata.cpp
        pdfium/core/src/fxcodec/codec/fx_codec.cpp
        pdfium/core/src/fxcodec/codec/fx_codec_fax.cpp
        pdfium/core/src/fxcodec/codec/fx_codec_flate.cpp
        pdfium/core/src/fxcodec/codec/fx_codec_icc.cpp
        pdfium/core/src/fxcodec/codec/fx_codec_jbig.cpp
        pdfium/core/src/fxcodec/codec/fx_codec_jbig_enc.cpp
        pdfium/core/src/fxcodec/codec/fx_codec_jpeg.cpp
        pdfium/core/src/fxcodec/codec/fx_codec_jpx_opj.cpp
        pdfium/core/src/fxcodec/fx_libopenjpeg/src/fx_bio.c
        pdfium/core/src/fxcodec/fx_libopenjpeg/src/fx_cio.c
        pdfium/core/src/fxcodec/fx_libopenjpeg/src/fx_dwt.c
        pdfium/core/src/fxcodec/fx_libopenjpeg/src/fx_event.c
        pdfium/core/src/fxcodec/fx_libopenjpeg/src/fx_function_list.c
        pdfium/core/src/fxcodec/fx_libopenjpeg/src/fx_image.c
        pdfium/core/src/fxcodec/fx_libopenjpeg/src/fx_invert.c
        pdfium/core/src/fxcodec/fx_libopenjpeg/src/fx_j2k.c
        pdfium/core/src/fxcodec/fx_libopenjpeg/src/fx_j2k_lib.c
        pdfium/core/src/fxcodec/fx_libopenjpeg/src/fx_jpt.c
        pdfium/core/src/fxcodec/fx_libopenjpeg/src/fx_mct.c
        pdfium/core/src/fxcodec/fx_libopenjpeg/src/fx_mqc.c
        pdfium/core/src/fxcodec/fx_libopenjpeg/src/fx_openjpeg.c
        pdfium/core/src/fxcodec/fx_libopenjpeg/src/fx_openjpeg_jp2.c
        pdfium/core/src/fxcodec/fx_libopenjpeg/src/fx_opj_clock.c
        pdfium/core/src/fxcodec/fx_libopenjpeg/src/fx_pi.c
        pdfium/core/src/fxcodec/fx_libopenjpeg/src/fx_raw.c
        pdfium/core/src/fxcodec/fx_libopenjpeg/src/fx_t1.c
        pdfium/core/src/fxcodec/fx_libopenjpeg/src/fx_t2.c
        pdfium/core/src/fxcodec/fx_libopenjpeg/src/fx_tcd.c
        pdfium/core/src/fxcodec/fx_libopenjpeg/src/fx_tgt.c
        pdfium/core/src/fxcodec/fx_zlib/src/fx_zlib_adler32.c
        pdfium/core/src/fxcodec/fx_zlib/src/fx_zlib_compress.c
        pdfium/core/src/fxcodec/fx_zlib/src/fx_zlib_crc32.c
        pdfium/core/src/fxcodec/fx_zlib/src/fx_zlib_deflate.c
        pdfium/core/src/fxcodec/fx_zlib/src/fx_zlib_gzclose.c
        pdfium/core/src/fxcodec/fx_zlib/src/fx_zlib_gzlib.c
        pdfium/core/src/fxcodec/fx_zlib/src/fx_zlib_gzread.c
        pdfium/core/src/fxcodec/fx_zlib/src/fx_zlib_gzwrite.c
        pdfium/core/src/fxcodec/fx_zlib/src/fx_zlib_infback.c
        pdfium/core/src/fxcodec/fx_zlib/src/fx_zlib_inffast.c
        pdfium/core/src/fxcodec/fx_zlib/src/fx_zlib_inflate.c
        pdfium/core/src/fxcodec/fx_zlib/src/fx_zlib_inftrees.c
        pdfium/core/src/fxcodec/fx_zlib/src/fx_zlib_trees.c
        pdfium/core/src/fxcodec/fx_zlib/src/fx_zlib_uncompr.c
        pdfium/core/src/fxcodec/fx_zlib/src/fx_zlib_zutil.c
        pdfium/core/src/fxcodec/jbig2/JBig2_ArithIntDecoder.cpp
        pdfium/core/src/fxcodec/jbig2/JBig2_Context.cpp
        pdfium/core/src/fxcodec/jbig2/JBig2_GeneralDecoder.cpp
        pdfium/core/src/fxcodec/jbig2/JBig2_HuffmanDecoder.cpp
        pdfium/core/src/fxcodec/jbig2/JBig2_HuffmanTable.cpp
        pdfium/core/src/fxcodec/jbig2/JBig2_Image.cpp
        pdfium/core/src/fxcodec/jbig2/JBig2_Object.cpp
        pdfium/core/src/fxcodec/jbig2/JBig2_PatternDict.cpp
        pdfium/core/src/fxcodec/jbig2/JBig2_Segment.cpp
        pdfium/core/src/fxcodec/jbig2/JBig2_SymbolDict.cpp
        pdfium/core/src/fxcodec/lcms2/src/fx_cmscam02.c
        pdfium/core/src/fxcodec/lcms2/src/fx_cmscgats.c
        pdfium/core/src/fxcodec/lcms2/src/fx_cmscnvrt.c
        pdfium/core/src/fxcodec/lcms2/src/fx_cmserr.c
        pdfium/core/src/fxcodec/lcms2/src/fx_cmsgamma.c
        pdfium/core/src/fxcodec/lcms2/src/fx_cmsgmt.c
        pdfium/core/src/fxcodec/lcms2/src/fx_cmshalf.c
        pdfium/core/src/fxcodec/lcms2/src/fx_cmsintrp.c
        pdfium/core/src/fxcodec/lcms2/src/fx_cmsio0.c
        pdfium/core/src/fxcodec/lcms2/src/fx_cmsio1.c
        pdfium/core/src/fxcodec/lcms2/src/fx_cmslut.c
        pdfium/core/src/fxcodec/lcms2/src/fx_cmsmd5.c
        pdfium/core/src/fxcodec/lcms2/src/fx_cmsmtrx.c
        pdfium/core/src/fxcodec/lcms2/src/fx_cmsnamed.c
        pdfium/core/src/fxcodec/lcms2/src/fx_cmsopt.c
        pdfium/core/src/fxcodec/lcms2/src/fx_cmspack.c
        pdfium/core/src/fxcodec/lcms2/src/fx_cmspcs.c
        pdfium/core/src/fxcodec/lcms2/src/fx_cmsplugin.c
        pdfium/core/src/fxcodec/lcms2/src/fx_cmsps2.c
        pdfium/core/src/fxcodec/lcms2/src/fx_cmssamp.c
        pdfium/core/src/fxcodec/lcms2/src/fx_cmssm.c
        pdfium/core/src/fxcodec/lcms2/src/fx_cmstypes.c
        pdfium/core/src/fxcodec/lcms2/src/fx_cmsvirt.c
        pdfium/core/src/fxcodec/lcms2/src/fx_cmswtpnt.c
        pdfium/core/src/fxcodec/lcms2/src/fx_cmsxform.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jcapimin.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jcapistd.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jccoefct.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jccolor.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jcdctmgr.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jchuff.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jcinit.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jcmainct.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jcmarker.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jcmaster.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jcomapi.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jcparam.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jcphuff.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jcprepct.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jcsample.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jctrans.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jdapimin.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jdapistd.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jdcoefct.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jdcolor.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jddctmgr.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jdhuff.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jdinput.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jdmainct.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jdmarker.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jdmaster.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jdmerge.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jdphuff.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jdpostct.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jdsample.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jdtrans.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jerror.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jfdctfst.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jfdctint.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jidctfst.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jidctint.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jidctred.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jmemmgr.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jmemnobs.c
        pdfium/core/src/fxcodec/libjpeg/fpdfapi_jutils.c
        pdfium/core/src/fxcrt/fxcrt_platforms.cpp
        pdfium/core/src/fxcrt/fxcrt_posix.cpp
        pdfium/core/src/fxcrt/fxcrt_windows.cpp
        pdfium/core/src/fxcrt/fx_arabic.cpp
        pdfium/core/src/fxcrt/fx_basic_array.cpp
        pdfium/core/src/fxcrt/fx_basic_bstring.cpp
        pdfium/core/src/fxcrt/fx_basic_buffer.cpp
        pdfium/core/src/fxcrt/fx_basic_coords.cpp
        pdfium/core/src/fxcrt/fx_basic_gcc.cpp
        pdfium/core/src/fxcrt/fx_basic_list.cpp
        pdfium/core/src/fxcrt/fx_basic_maps.cpp
        pdfium/core/src/fxcrt/fx_basic_memmgr.cpp
        pdfium/core/src/fxcrt/fx_basic_plex.cpp
        pdfium/core/src/fxcrt/fx_basic_utf.cpp
        pdfium/core/src/fxcrt/fx_basic_util.cpp
        pdfium/core/src/fxcrt/fx_basic_wstring.cpp
        pdfium/core/src/fxcrt/fx_extension.cpp
        pdfium/core/src/fxcrt/fx_ucddata.cpp
        pdfium/core/src/fxcrt/fx_unicode.cpp
        pdfium/core/src/fxcrt/fx_xml_composer.cpp
        pdfium/core/src/fxcrt/fx_xml_parser.cpp
        pdfium/core/src/fxge/agg/src/fxfx_agg_curves.cpp
        pdfium/core/src/fxge/agg/src/fxfx_agg_driver.cpp
        pdfium/core/src/fxge/agg/src/fxfx_agg_path_storage.cpp
        pdfium/core/src/fxge/agg/src/fxfx_agg_rasterizer_scanline_aa.cpp
        pdfium/core/src/fxge/agg/src/fxfx_agg_vcgen_dash.cpp
        pdfium/core/src/fxge/agg/src/fxfx_agg_vcgen_stroke.cpp
        pdfium/core/src/fxge/android/fpf_skiafont.cpp
        pdfium/core/src/fxge/android/fpf_skiafontmgr.cpp
        pdfium/core/src/fxge/android/fpf_skiamodule.cpp
        pdfium/core/src/fxge/dib/fx_dib_composite.cpp
        pdfium/core/src/fxge/dib/fx_dib_convert.cpp
        pdfium/core/src/fxge/dib/fx_dib_engine.cpp
        pdfium/core/src/fxge/dib/fx_dib_main.cpp
        pdfium/core/src/fxge/dib/fx_dib_transform.cpp
        pdfium/core/src/fxge/fontdata/chromefontdata/FoxitDingbats.c
        pdfium/core/src/fxge/fontdata/chromefontdata/FoxitFixed.c
        pdfium/core/src/fxge/fontdata/chromefontdata/FoxitFixedBold.c
        pdfium/core/src/fxge/fontdata/chromefontdata/FoxitFixedBoldItalic.c
        pdfium/core/src/fxge/fontdata/chromefontdata/FoxitFixedItalic.c
        pdfium/core/src/fxge/fontdata/chromefontdata/FoxitSans.c
        pdfium/core/src/fxge/fontdata/chromefontdata/FoxitSansBold.c
        pdfium/core/src/fxge/fontdata/chromefontdata/FoxitSansBoldItalic.c
        pdfium/core/src/fxge/fontdata/chromefontdata/FoxitSansItalic.c
        pdfium/core/src/fxge/fontdata/chromefontdata/FoxitSansMM.c
        pdfium/core/src/fxge/fontdata/chromefontdata/FoxitSerif.c
        pdfium/core/src/fxge/fontdata/chromefontdata/FoxitSerifBold.c
        pdfium/core/src/fxge/fontdata/chromefontdata/FoxitSerifBoldItalic.c
        pdfium/core/src/fxge/fontdata/chromefontdata/FoxitSerifItalic.c
        pdfium/core/src/fxge/fontdata/chromefontdata/FoxitSerifMM.c
        pdfium/core/src/fxge/fontdata/chromefontdata/FoxitSymbol.c
        pdfium/core/src/fxge/fx_freetype/src/fxft_cff.c
        pdfium/core/src/fxge/fx_freetype/src/fxft_ftbase.c
        pdfium/core/src/fxge/fx_freetype/src/fxft_ftbitmap.c
        pdfium/core/src/fxge/fx_freetype/src/fxft_ftglyph.c
        pdfium/core/src/fxge/fx_freetype/src/fxft_ftinit.c
        pdfium/core/src/fxge/fx_freetype/src/fxft_ftlcdfil.c
        pdfium/core/src/fxge/fx_freetype/src/fxft_ftmm.c
        pdfium/core/src/fxge/fx_freetype/src/fxft_ftsystem.c
        pdfium/core/src/fxge/fx_freetype/src/fxft_psaux.c
        pdfium/core/src/fxge/fx_freetype/src/fxft_pshinter.c
        pdfium/core/src/fxge/fx_freetype/src/fxft_psmodule.c
        pdfium/core/src/fxge/fx_freetype/src/fxft_raster.c
        pdfium/core/src/fxge/fx_freetype/src/fxft_sfnt.c
        pdfium/core/src/fxge/fx_freetype/src/fxft_smooth.c
        pdfium/core/src/fxge/fx_freetype/src/fxft_truetype.c
        pdfium/core/src/fxge/fx_freetype/src/fxft_type1.c
        pdfium/core/src/fxge/fx_freetype/src/fxft_type1cid.c
        pdfium/core/src/fxge/ge/fx_ge.cpp
        pdfium/core/src/fxge/ge/fx_ge_device.cpp
        pdfium/core/src/fxge/ge/fx_ge_font.cpp
        pdfium/core/src/fxge/ge/fx_ge_fontmap.cpp
        pdfium/core/src/fxge/ge/fx_ge_path.cpp
        pdfium/core/src/fxge/ge/fx_ge_ps.cpp
        pdfium/core/src/fxge/ge/fx_ge_text.cpp
        pdfium/core/src/fxge/skia/fx_skia_blitter_new.cpp
        pdfium/core/src/fxge/skia/fx_skia_device.cpp
        pdfium/core/src/reflow/autoreflow.cpp
        pdfium/core/src/reflow/layoutprocessor_reflow.cpp
        pdfium/core/src/reflow/layoutprovider_taggedpdf.cpp
        pdfium/core/src/reflow/reflowedpage.cpp
        pdfium/core/src/reflow/reflowedtextpage.cpp
        pdfium/core/src/reflow/reflowengine.cpp
    )

    if(WIN32)
        set(SRC_PDFIUM_PLATFORM
            pdfium/core/src/fxge/win32/fx_win32_device.cpp
            pdfium/core/src/fxge/win32/fx_win32_dib.cpp
            pdfium/core/src/fxge/win32/fx_win32_dwrite.cpp
            pdfium/core/src/fxge/win32/fx_win32_gdipext.cpp
            pdfium/core/src/fxge/win32/fx_win32_print.cpp
        )
    elseif(APPLE)
        set(SRC_PDFIUM_PLATFORM
            pdfium/core/src/fxge/apple/fx_apple_platform.cpp
            pdfium/core/src/fxge/apple/fx_mac_imp.cpp
            pdfium/core/src/fxge/apple/fx_quartz_device.cpp
        )
    elseif(UNIX)
        set(SRC_PDFIUM_PLATFORM
            pdfium/core/src/fxge/ge/fx_ge_linux.cpp
        )
    endif()

    set(SRC_PDFSDK ${SRC_PDFIUM} ${SRC_PDFIUM_PLATFORM})

endif()

# =========== common ===========

SET(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

SET(CMAKE_REQUIRED_LIBRARIES stdc++fs)
SET(CMAKE_DEBUG_POSTFIX _d)

set(SOURCES
    src/ArlingtonTSVGrammarFile.cpp
    src/CheckDVA.cpp
    src/CheckGrammar.cpp
    src/ParseObjects.cpp
    src/PredicateProcessor.cpp
    src/LRParsePredicate.cpp
    src/ArlVersion.cpp
    src/PDFFile.cpp
    src/Utils.cpp
    sarge/sarge.cpp
    )

if(WIN32)
    add_compile_options(/EHsc)
    if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/x86)
    else()
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/x64)
    endif()
elseif (APPLE)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/darwin)
elseif (UNIX)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/linux)
endif()

add_executable(TestGrammar src/Main.cpp ${SOURCES} ${SRC_PDFSDK})
set_target_properties(TestGrammar PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
add_compile_definitions(TestGrammar $<$<CONFIG:DEBUG>:DEBUG>)

target_include_directories(TestGrammar
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${CMAKE_CURRENT_SOURCE_DIR}/sarge"
        "${CMAKE_CURRENT_SOURCE_DIR}/pdfium"
        "${CMAKE_CURRENT_SOURCE_DIR}/pdfix"
        "${CMAKE_CURRENT_SOURCE_DIR}/qpdf/include"
    )

if(APPLE)
    target_link_libraries(TestGrammar dl
        "-framework CoreFoundation"
        "-framework CoreGraphics"
        "-framework CoreText"
    )
elseif (UNIX)
    target_link_libraries(TestGrammar dl stdc++fs)
endif()
