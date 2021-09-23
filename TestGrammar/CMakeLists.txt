cmake_minimum_required (VERSION 3.10)
SET (CMAKE_CXX_STANDARD 17)
SET (CMAKE_CXX_STANDARD_REQUIRED ON)
SET (CMAKE_REQUIRED_LIBRARIES stdc++fs)
SET (CMAKE_DEBUG_POSTFIX _d)

PROJECT(TestGrammar)

set(SOURCES
    src/ArlingtonTSVGrammarFile.cpp
    src/CheckDVA.cpp
    src/CheckGrammar.cpp
    src/ParseObjects.cpp
    src/PredicateProcessor.cpp
    src/Utils.cpp
    sarge/sarge.cpp
    )

#=========== pdfix ===========
set (SRC_PDFIX
    src/ArlingtonPDFShimPDFix.cpp
)

#=========== QPDF ===========
set (SRC_QPDF
    src/ArlingtonPDFShimQPDF.cpp
)

#=========== pdfium ===========
if(MSVC)
    add_compile_options(/W3 /WX- /utf-8)
    add_compile_definitions(X86_WINDOWS WIN32 BUILD_WINDOWS ICU_DATA_DIR_WINDOWS="")
else()
endif()

add_compile_definitions(PNG_NO_CONFIG_H USE_LIBJPEG_TURBO NOMINMAX UNICODE _UNICODE U_COMMON_IMPLEMENTATION FT2_BUILD_LIBRARY _NO_GDIPLUS_ _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE _HAS_EXCEPTIONS=0 LS_STATIC)

#old: add_compile_definitions(PNG_PREFIX PNG_USE_READ_MACROS OPJ_STATIC PNG_INTEL_SSE_OPT=1 NOMINMAX _CRT_NONSTDC_NO_DEPRECATE)
# USE_STD_NAMESPACE
# PDF_ENABLE_XFA
# PDF_ENABLE_XFA_BMP
# PDF_ENABLE_XFA_GIF
# PDF_ENABLE_XFA_TIFF
#LS_STATIC;WIN3;USE_LIBJPEG_TURBO;BOOST_ALL_NO_LIB;PDF_ENABLE_XFA_BMP;PDF_ENABLE_XFA_GIF;PDF_ENABLE_XFA_TIFF;U_COMMON_IMPLEMENTATION;X86_WINDOWS;PDF_ENABLE_XFA;FT2_BUILD_LIBRARY;_FPDFSDK_LIB;_NO_GDIPLUS_;OPJ_STATIC;V8_DEPRECATION_WARNINGS;NOMINMAX;_CRT_SECURE_NO_DEPRECATE;_CRT_NONSTDC_NO_DEPRECATE;_HAS_EXCEPTIONS=0;WIN32;_DEBUG;_CONSOLE;%(PreprocessorDefinitions)
#add_compile_definitions(register=) #mac doesn't compile lcms with C++17

set(SRC_PDFIUM
    src/ArlingtonPDFShimPDFium.cpp
	pdfium/constants/annotation_common.cpp
	pdfium/constants/appearance.cpp
	pdfium/constants/form_fields.cpp
	pdfium/constants/page_object.cpp
	pdfium/constants/stream_dict_common.cpp
	pdfium/constants/transparency.cpp
	#pdfium/core/fpdfapi/edit/cpdf_creator.cpp
    pdfium/core/fpdfapi/font/cfx_cttgsubtable.cpp
    pdfium/core/fpdfapi/font/cfx_stockfontarray.cpp
    pdfium/core/fpdfapi/font/cpdf_cid2unicodemap.cpp
    pdfium/core/fpdfapi/font/cpdf_cidfont.cpp
    pdfium/core/fpdfapi/font/cpdf_cmap.cpp
    pdfium/core/fpdfapi/font/cpdf_cmapmanager.cpp
    pdfium/core/fpdfapi/font/cpdf_cmapparser.cpp
    pdfium/core/fpdfapi/font/cpdf_font.cpp
    pdfium/core/fpdfapi/font/cpdf_fontencoding.cpp
    pdfium/core/fpdfapi/font/cpdf_fontglobals.cpp
    pdfium/core/fpdfapi/font/cpdf_simplefont.cpp
    pdfium/core/fpdfapi/font/cpdf_tounicodemap.cpp
    pdfium/core/fpdfapi/font/cpdf_truetypefont.cpp
    pdfium/core/fpdfapi/font/cpdf_type1font.cpp
    pdfium/core/fpdfapi/font/cpdf_type3char.cpp
    pdfium/core/fpdfapi/font/cpdf_type3font.cpp
    pdfium/core/fpdfapi/page/cpdf_devicecs.cpp
    pdfium/core/fpdfapi/page/cpdf_expintfunc.cpp
    pdfium/core/fpdfapi/page/cpdf_function.cpp
    pdfium/core/fpdfapi/page/cpdf_iccprofile.cpp
    pdfium/core/fpdfapi/page/cpdf_patterncs.cpp
    pdfium/core/fpdfapi/page/cpdf_psengine.cpp
    pdfium/core/fpdfapi/page/cpdf_psfunc.cpp
    pdfium/core/fpdfapi/page/cpdf_sampledfunc.cpp
    pdfium/core/fpdfapi/page/cpdf_stitchfunc.cpp
    pdfium/core/fpdfapi/page/cpdf_basedcs.cpp
    pdfium/core/fpdfapi/page/cpdf_transparency.cpp
    pdfium/core/fpdfapi/parser/cpdf_cross_ref_avail.cpp
    pdfium/core/fpdfapi/parser/cpdf_cross_ref_table.cpp
    pdfium/core/fpdfapi/parser/cpdf_encryptor.cpp
    pdfium/core/fpdfapi/parser/cpdf_flateencoder.cpp
    pdfium/core/fpdfapi/parser/cpdf_object_avail.cpp
    pdfium/core/fpdfapi/parser/cpdf_object_stream.cpp
    pdfium/core/fpdfapi/parser/cpdf_object_walker.cpp
    pdfium/core/fpdfapi/parser/cpdf_page_object_avail.cpp
    pdfium/core/fpdfapi/parser/cpdf_read_validator.cpp
    pdfium/core/fpdfapi/parser/cpdf_syntax_parser.cpp
    #pdfium/core/fxcodec/bmp/cfx_bmpcontext.cpp
    #pdfium/core/fxcodec/bmp/cfx_bmpdecompressor.cpp
    #pdfium/core/fxcodec/bmp/bmp_decoder.cpp
    pdfium/core/fxcodec/cfx_codec_memory.cpp
    pdfium/core/fxcodec/scanlinedecoder.cpp
    # pdfium/core/fxcodec/tiff/tiff_decoder.cpp
    # pdfium/core/fxcodec/gif/cfx_gif.cpp
    # pdfium/core/fxcodec/gif/cfx_gifcontext.cpp
    # pdfium/core/fxcodec/gif/lzw_decompressor.cpp
    # pdfium/core/fxcodec/gif/gif_decoder.cpp
    pdfium/core/fxcrt/bytestring.cpp
    pdfium/core/fxcrt/cfx_binarybuf.cpp
    pdfium/core/fxcrt/cfx_bitstream.cpp
    pdfium/core/fxcrt/cfx_datetime.cpp
    pdfium/core/fxcrt/cfx_memorystream.cpp
    pdfium/core/fxcrt/cfx_readonlymemorystream.cpp
    pdfium/core/fxcrt/cfx_seekablestreamproxy.cpp
    pdfium/core/fxcrt/cfx_utf8decoder.cpp
    pdfium/core/fxcrt/cfx_utf8encoder.cpp
    pdfium/core/fxcrt/cfx_widetextbuf.cpp
    pdfium/core/fxcrt/fx_bidi.cpp
    pdfium/core/fxcrt/fx_codepage.cpp
    pdfium/core/fxcrt/fx_coordinates.cpp
    pdfium/core/fxcrt/fx_extension.cpp
    pdfium/core/fxcrt/fx_number.cpp
    pdfium/core/fxcrt/fx_memory.cpp
    pdfium/core/fxcrt/fx_random.cpp
    pdfium/core/fxcrt/fx_stream.cpp
    pdfium/core/fxcrt/fx_string.cpp
    pdfium/core/fxcrt/fx_system.cpp
    pdfium/core/fxcrt/fx_unicode.cpp
    pdfium/core/fxcrt/observed_ptr.cpp
    pdfium/core/fxcrt/string_data_template.cpp
    pdfium/core/fxcrt/widestring.cpp
    # pdfium/core/fxcrt/xml/cfx_xmlchardata.cpp
    # pdfium/core/fxcrt/xml/cfx_xmldocument.cpp
    # pdfium/core/fxcrt/xml/cfx_xmlelement.cpp
    # pdfium/core/fxcrt/xml/cfx_xmlinstruction.cpp
    # pdfium/core/fxcrt/xml/cfx_xmlnode.cpp
    # pdfium/core/fxcrt/xml/cfx_xmlparser.cpp
    # pdfium/core/fxcrt/xml/cfx_xmltext.cpp
    pdfium/core/fxge/cfx_cliprgn.cpp
    pdfium/core/fxge/cfx_color.cpp
    pdfium/core/fxge/cfx_glyphcache.cpp
    pdfium/core/fxge/cfx_face.cpp
    pdfium/core/fxge/cfx_folderfontinfo.cpp
    pdfium/core/fxge/cfx_font.cpp
    pdfium/core/fxge/cfx_fontcache.cpp
    pdfium/core/fxge/cfx_fontmapper.cpp
    pdfium/core/fxge/cfx_fontmgr.cpp
    pdfium/core/fxge/cfx_gemodule.cpp
    pdfium/core/fxge/cfx_glyphbitmap.cpp
    pdfium/core/fxge/cfx_graphstate.cpp
    pdfium/core/fxge/cfx_graphstatedata.cpp
	# pdfium/core/fxge/cfx_pathdata.cpp
    pdfium/core/fxge/cfx_path.cpp
    pdfium/core/fxge/cfx_renderdevice.cpp
    pdfium/core/fxge/cfx_substfont.cpp
    pdfium/core/fxge/cfx_unicodeencoding.cpp
    pdfium/core/fxge/cfx_unicodeencodingex.cpp
    pdfium/core/fxge/dib/cfx_bitmapcomposer.cpp
    pdfium/core/fxge/dib/cfx_bitmapstorer.cpp
    pdfium/core/fxge/dib/cfx_cmyk_to_srgb.cpp
    pdfium/core/fxge/dib/cfx_dibextractor.cpp
    pdfium/core/fxge/dib/cfx_dibitmap.cpp
    pdfium/core/fxge/dib/cfx_dibbase.cpp
    pdfium/core/fxge/dib/cfx_imagerenderer.cpp
    pdfium/core/fxge/dib/cfx_imagestretcher.cpp
    pdfium/core/fxge/dib/cfx_imagetransformer.cpp
    pdfium/core/fxge/dib/cfx_scanlinecompositor.cpp
    pdfium/core/fxge/dib/cstretchengine.cpp
    pdfium/core/fxge/dib/fx_dib.cpp
    pdfium/core/fxge/fx_font.cpp
    pdfium/core/fxge/renderdevicedriver_iface.cpp
    pdfium/core/fxge/scoped_font_transform.cpp
    pdfium/core/fxge/text_glyph_pos.cpp
    pdfium/third_party/base/allocator/partition_allocator/address_space_randomization.cc
    pdfium/third_party/base/allocator/partition_allocator/oom_callback.cc
    pdfium/third_party/base/allocator/partition_allocator/page_allocator.cc
    pdfium/third_party/base/allocator/partition_allocator/partition_alloc.cc
    pdfium/third_party/base/allocator/partition_allocator/partition_bucket.cc
    pdfium/third_party/base/allocator/partition_allocator/partition_oom.cc
    pdfium/third_party/base/allocator/partition_allocator/partition_page.cc
    pdfium/third_party/base/allocator/partition_allocator/partition_root_base.cc
    pdfium/third_party/base/allocator/partition_allocator/random.cc
    pdfium/third_party/base/allocator/partition_allocator/spin_lock.cc
    pdfium/third_party/base/debug/alias.cc
    pdfium/third_party/base/memory/aligned_memory.cc
    # pdfium/third_party/freetype/src/src/autofit/afangles.c
    pdfium/third_party/freetype/src/src/autofit/afblue.c
    pdfium/third_party/freetype/src/src/autofit/afcjk.c
    pdfium/third_party/freetype/src/src/autofit/afdummy.c
    pdfium/third_party/freetype/src/src/autofit/afglobal.c
    pdfium/third_party/freetype/src/src/autofit/afhints.c
    pdfium/third_party/freetype/src/src/autofit/afindic.c
    pdfium/third_party/freetype/src/src/autofit/aflatin.c
    pdfium/third_party/freetype/src/src/autofit/afloader.c
    pdfium/third_party/freetype/src/src/autofit/afmodule.c
    pdfium/third_party/freetype/src/src/autofit/afranges.c
    pdfium/third_party/freetype/src/src/autofit/afshaper.c
    # pdfium/third_party/freetype/src/src/autofit/afwarp.c
    pdfium/third_party/freetype/src/src/base/ftadvanc.c
    pdfium/third_party/freetype/src/src/base/ftbbox.c
    pdfium/third_party/freetype/src/src/base/ftbdf.c
    pdfium/third_party/freetype/src/src/base/ftbitmap.c
    pdfium/third_party/freetype/src/src/base/ftcalc.c
    pdfium/third_party/freetype/src/src/base/ftcid.c
    pdfium/third_party/freetype/src/src/base/ftcolor.c
    pdfium/third_party/freetype/src/src/base/ftdbgmem.c
    pdfium/third_party/freetype/src/src/base/ftdebug.c
    pdfium/third_party/freetype/src/src/base/ftfntfmt.c
    pdfium/third_party/freetype/src/src/base/ftfstype.c
    pdfium/third_party/freetype/src/src/base/ftgasp.c
    pdfium/third_party/freetype/src/src/base/ftgloadr.c
    pdfium/third_party/freetype/src/src/base/ftglyph.c
    pdfium/third_party/freetype/src/src/base/ftgxval.c
    pdfium/third_party/freetype/src/src/base/fthash.c
    pdfium/third_party/freetype/src/src/base/ftinit.c
    pdfium/third_party/freetype/src/src/base/ftlcdfil.c
    pdfium/third_party/freetype/src/src/base/ftmac.c
    pdfium/third_party/freetype/src/src/base/ftmm.c
    pdfium/third_party/freetype/src/src/base/ftobjs.c
    pdfium/third_party/freetype/src/src/base/ftotval.c
    pdfium/third_party/freetype/src/src/base/ftoutln.c
    pdfium/third_party/freetype/src/src/base/ftpfr.c
    pdfium/third_party/freetype/src/src/base/ftpsprop.c
    pdfium/third_party/freetype/src/src/base/ftrfork.c
    pdfium/third_party/freetype/src/src/base/ftsnames.c
    pdfium/third_party/freetype/src/src/base/ftstream.c
    pdfium/third_party/freetype/src/src/base/ftstroke.c
    pdfium/third_party/freetype/src/src/base/ftsynth.c
    pdfium/third_party/freetype/src/src/base/ftsystem.c
    pdfium/third_party/freetype/src/src/base/fttrigon.c
    pdfium/third_party/freetype/src/src/base/fttype1.c
    pdfium/third_party/freetype/src/src/base/ftutil.c
    pdfium/third_party/freetype/src/src/base/ftwinfnt.c
    pdfium/third_party/freetype/src/src/bdf/bdfdrivr.c
    pdfium/third_party/freetype/src/src/bdf/bdflib.c
    pdfium/third_party/freetype/src/src/cff/cffcmap.c
    pdfium/third_party/freetype/src/src/cff/cffdrivr.c
    pdfium/third_party/freetype/src/src/cff/cffgload.c
    pdfium/third_party/freetype/src/src/cff/cffload.c
    pdfium/third_party/freetype/src/src/cff/cffobjs.c
    pdfium/third_party/freetype/src/src/cff/cffparse.c
    pdfium/third_party/freetype/src/src/cid/cidgload.c
    pdfium/third_party/freetype/src/src/cid/cidload.c
    pdfium/third_party/freetype/src/src/cid/cidobjs.c
    pdfium/third_party/freetype/src/src/cid/cidparse.c
    pdfium/third_party/freetype/src/src/cid/cidriver.c
    pdfium/third_party/freetype/src/src/gzip/ftgzip.c
    pdfium/third_party/freetype/src/src/lzw/ftlzw.c
    pdfium/third_party/freetype/src/src/pcf/pcfdrivr.c
    pdfium/third_party/freetype/src/src/pcf/pcfread.c
    pdfium/third_party/freetype/src/src/pcf/pcfutil.c
    pdfium/third_party/freetype/src/src/pfr/pfrcmap.c
    pdfium/third_party/freetype/src/src/pfr/pfrdrivr.c
    pdfium/third_party/freetype/src/src/pfr/pfrgload.c
    pdfium/third_party/freetype/src/src/pfr/pfrload.c
    pdfium/third_party/freetype/src/src/pfr/pfrobjs.c
    pdfium/third_party/freetype/src/src/pfr/pfrsbit.c
    pdfium/third_party/freetype/src/src/psaux/afmparse.c
    pdfium/third_party/freetype/src/src/psaux/cffdecode.c
    pdfium/third_party/freetype/src/src/psaux/psarrst.c
    pdfium/third_party/freetype/src/src/psaux/psauxmod.c
    pdfium/third_party/freetype/src/src/psaux/psblues.c
    pdfium/third_party/freetype/src/src/psaux/psconv.c
    pdfium/third_party/freetype/src/src/psaux/pserror.c
    pdfium/third_party/freetype/src/src/psaux/psfont.c
    pdfium/third_party/freetype/src/src/psaux/psft.c
    pdfium/third_party/freetype/src/src/psaux/pshints.c
    pdfium/third_party/freetype/src/src/psaux/psintrp.c
    pdfium/third_party/freetype/src/src/psaux/psobjs.c
    pdfium/third_party/freetype/src/src/psaux/psread.c
    pdfium/third_party/freetype/src/src/psaux/psstack.c
    pdfium/third_party/freetype/src/src/psaux/t1cmap.c
    pdfium/third_party/freetype/src/src/psaux/t1decode.c
    pdfium/third_party/freetype/src/src/pshinter/pshalgo.c
    pdfium/third_party/freetype/src/src/pshinter/pshglob.c
    pdfium/third_party/freetype/src/src/pshinter/pshmod.c
    pdfium/third_party/freetype/src/src/pshinter/pshrec.c
    pdfium/third_party/freetype/src/src/psnames/psmodule.c
    pdfium/third_party/freetype/src/src/raster/ftraster.c
    pdfium/third_party/freetype/src/src/raster/ftrend1.c
    pdfium/third_party/freetype/src/src/sdf/sdf.c
    pdfium/third_party/freetype/src/src/sfnt/pngshim.c
    pdfium/third_party/freetype/src/src/sfnt/sfdriver.c
    pdfium/third_party/freetype/src/src/sfnt/sfobjs.c
    pdfium/third_party/freetype/src/src/sfnt/sfwoff.c
    pdfium/third_party/freetype/src/src/sfnt/sfwoff2.c
    pdfium/third_party/freetype/src/src/sfnt/ttbdf.c
    pdfium/third_party/freetype/src/src/sfnt/ttcmap.c
    pdfium/third_party/freetype/src/src/sfnt/ttcolr.c
    pdfium/third_party/freetype/src/src/sfnt/ttcpal.c
    pdfium/third_party/freetype/src/src/sfnt/ttkern.c
    pdfium/third_party/freetype/src/src/sfnt/ttload.c
    pdfium/third_party/freetype/src/src/sfnt/ttmtx.c
    pdfium/third_party/freetype/src/src/sfnt/ttpost.c
    pdfium/third_party/freetype/src/src/sfnt/ttsbit.c
    pdfium/third_party/freetype/src/src/sfnt/woff2tags.c
    pdfium/third_party/freetype/src/src/smooth/ftgrays.c
    pdfium/third_party/freetype/src/src/smooth/ftsmooth.c
    pdfium/third_party/freetype/src/src/truetype/truetype.c
    pdfium/third_party/freetype/src/src/truetype/ttsubpix.c
    pdfium/third_party/freetype/src/src/type1/t1afm.c
    pdfium/third_party/freetype/src/src/type1/t1driver.c
    pdfium/third_party/freetype/src/src/type1/t1gload.c
    pdfium/third_party/freetype/src/src/type1/t1load.c
    pdfium/third_party/freetype/src/src/type1/t1objs.c
    pdfium/third_party/freetype/src/src/type1/t1parse.c
    pdfium/third_party/freetype/src/src/type42/t42drivr.c
    pdfium/third_party/freetype/src/src/type42/t42objs.c
    pdfium/third_party/freetype/src/src/type42/t42parse.c
    pdfium/third_party/freetype/src/src/winfonts/winfnt.c
    # pdfium/third_party/icu/source/common/appendable.cpp
    # pdfium/third_party/icu/source/common/bytesinkutil.cpp
    # pdfium/third_party/icu/source/common/bytestream.cpp
    # pdfium/third_party/icu/source/common/charstr.cpp
    # pdfium/third_party/icu/source/common/cmemory.cpp
    # pdfium/third_party/icu/source/common/cstring.cpp
    # pdfium/third_party/icu/source/common/edits.cpp
    # pdfium/third_party/icu/source/common/errorcode.cpp
    # pdfium/third_party/icu/source/common/locmap.cpp
    # pdfium/third_party/icu/source/common/putil.cpp
    # pdfium/third_party/icu/source/common/stringpiece.cpp
    # pdfium/third_party/icu/source/common/ucase.cpp
    # pdfium/third_party/icu/source/common/uchar.cpp
    # pdfium/third_party/icu/source/common/ucln_cmn.cpp
    # pdfium/third_party/icu/source/common/udataswp.cpp
    # pdfium/third_party/icu/source/common/uinvchar.cpp
	# pdfium/third_party/icu/source/common/uloc.cpp	
    # pdfium/third_party/icu/source/common/umutex.cpp
    # pdfium/third_party/icu/source/common/unistr.cpp
    # pdfium/third_party/icu/source/common/unistr_case.cpp
    # pdfium/third_party/icu/source/common/unistr_cnv.cpp
    # pdfium/third_party/icu/source/common/uobject.cpp
    # pdfium/third_party/icu/source/common/ustrtrns.cpp
    # pdfium/third_party/icu/source/common/ustring.cpp
	# pdfium/third_party/icu/source/common/ustrenum.cpp
    # pdfium/third_party/icu/source/common/utrace.cpp
    # pdfium/third_party/icu/source/common/utrie2.cpp
    # pdfium/third_party/icu/source/common/utf_impl.cpp
    # pdfium/third_party/icu/source/common/utypes.cpp
	pdfium/third_party/icu/source/common/uniset.cpp
	pdfium/third_party/icu/source/common/dictbe.cpp
	pdfium/third_party/icu/source/common/chariter.cpp
	pdfium/third_party/icu/source/common/utf_impl.cpp
	pdfium/third_party/icu/source/common/uhash.cpp
	pdfium/third_party/icu/source/common/ushape.cpp
	pdfium/third_party/icu/source/common/ucnvsel.cpp
	pdfium/third_party/icu/source/common/uarrsort.cpp
	pdfium/third_party/icu/source/common/ucnv_set.cpp
	pdfium/third_party/icu/source/common/uniset_props.cpp
	pdfium/third_party/icu/source/common/caniter.cpp
	pdfium/third_party/icu/source/common/ubidiwrt.cpp
	pdfium/third_party/icu/source/common/ucnvscsu.cpp
	pdfium/third_party/icu/source/common/cstr.cpp
	pdfium/third_party/icu/source/common/wintz.cpp
	pdfium/third_party/icu/source/common/ubiditransform.cpp
	pdfium/third_party/icu/source/common/dictionarydata.cpp
	pdfium/third_party/icu/source/common/loclikelysubtags.cpp
	pdfium/third_party/icu/source/common/stringtriebuilder.cpp
	pdfium/third_party/icu/source/common/util.cpp
	pdfium/third_party/icu/source/common/cstring.cpp
	pdfium/third_party/icu/source/common/localeprioritylist.cpp
	pdfium/third_party/icu/source/common/uinit.cpp
	pdfium/third_party/icu/source/common/ucharstrieiterator.cpp
	pdfium/third_party/icu/source/common/unisetspan.cpp
	pdfium/third_party/icu/source/common/icuplug.cpp
	pdfium/third_party/icu/source/common/ucnv_ext.cpp
	pdfium/third_party/icu/source/common/servnotf.cpp
	pdfium/third_party/icu/source/common/locid.cpp
	pdfium/third_party/icu/source/common/ustr_cnv.cpp
	pdfium/third_party/icu/source/common/unames.cpp
	pdfium/third_party/icu/source/common/filterednormalizer2.cpp
	pdfium/third_party/icu/source/common/loclikely.cpp
	pdfium/third_party/icu/source/common/uscript_props.cpp
	pdfium/third_party/icu/source/common/bytestrieiterator.cpp
	pdfium/third_party/icu/source/common/propsvec.cpp
	pdfium/third_party/icu/source/common/ucnv_u32.cpp
	pdfium/third_party/icu/source/common/punycode.cpp
	pdfium/third_party/icu/source/common/ubidiln.cpp
	pdfium/third_party/icu/source/common/putil.cpp
	pdfium/third_party/icu/source/common/bytestrie.cpp
	pdfium/third_party/icu/source/common/resource.cpp
	pdfium/third_party/icu/source/common/uvector.cpp
	pdfium/third_party/icu/source/common/ucase.cpp
	pdfium/third_party/icu/source/common/rbbi.cpp
	pdfium/third_party/icu/source/common/locutil.cpp
	pdfium/third_party/icu/source/common/servls.cpp
	pdfium/third_party/icu/source/common/ucnvdisp.cpp
	pdfium/third_party/icu/source/common/ucnv_err.cpp
	pdfium/third_party/icu/source/common/unistr_titlecase_brkiter.cpp
	pdfium/third_party/icu/source/common/uloc_tag.cpp
	pdfium/third_party/icu/source/common/icudataver.cpp
	pdfium/third_party/icu/source/common/uenum.cpp
	pdfium/third_party/icu/source/common/rbbisetb.cpp
	pdfium/third_party/icu/source/common/filteredbrk.cpp
	pdfium/third_party/icu/source/common/ubrk.cpp
	pdfium/third_party/icu/source/common/ucnv_bld.cpp
	pdfium/third_party/icu/source/common/brkeng.cpp
	pdfium/third_party/icu/source/common/charstr.cpp
	pdfium/third_party/icu/source/common/uloc_keytype.cpp
	pdfium/third_party/icu/source/common/ucnv_ct.cpp
	pdfium/third_party/icu/source/common/bytestriebuilder.cpp
	pdfium/third_party/icu/source/common/ubidi.cpp
	pdfium/third_party/icu/source/common/ucnv_u8.cpp
	pdfium/third_party/icu/source/common/locdispnames.cpp
	pdfium/third_party/icu/source/common/ucnv_cb.cpp
	pdfium/third_party/icu/source/common/ustr_titlecase_brkiter.cpp
	pdfium/third_party/icu/source/common/locdistance.cpp
	pdfium/third_party/icu/source/common/static_unicode_sets.cpp
	pdfium/third_party/icu/source/common/bmpset.cpp
	pdfium/third_party/icu/source/common/uniset_closure.cpp
	pdfium/third_party/icu/source/common/locdspnm.cpp
	pdfium/third_party/icu/source/common/bytesinkutil.cpp
	pdfium/third_party/icu/source/common/rbbi_cache.cpp
	pdfium/third_party/icu/source/common/ucharstriebuilder.cpp
	pdfium/third_party/icu/source/common/utrie2.cpp
	pdfium/third_party/icu/source/common/unifiedcache.cpp
	pdfium/third_party/icu/source/common/servlkf.cpp
	pdfium/third_party/icu/source/common/schriter.cpp
	pdfium/third_party/icu/source/common/locbased.cpp
	pdfium/third_party/icu/source/common/udatamem.cpp
	pdfium/third_party/icu/source/common/umutablecptrie.cpp
	pdfium/third_party/icu/source/common/patternprops.cpp
	pdfium/third_party/icu/source/common/localebuilder.cpp
	pdfium/third_party/icu/source/common/unistr_props.cpp
	pdfium/third_party/icu/source/common/ucnv.cpp
	pdfium/third_party/icu/source/common/stringpiece.cpp
	pdfium/third_party/icu/source/common/ulist.cpp
	pdfium/third_party/icu/source/common/ustrtrns.cpp
	pdfium/third_party/icu/source/common/uiter.cpp
	pdfium/third_party/icu/source/common/cmemory.cpp
	pdfium/third_party/icu/source/common/resbund_cnv.cpp
	pdfium/third_party/icu/source/common/uvectr64.cpp
	pdfium/third_party/icu/source/common/unistr_case.cpp
	pdfium/third_party/icu/source/common/ustr_wcs.cpp
	pdfium/third_party/icu/source/common/ucnvlat1.cpp
	pdfium/third_party/icu/source/common/simpleformatter.cpp
	pdfium/third_party/icu/source/common/uts46.cpp
	pdfium/third_party/icu/source/common/ucnvmbcs.cpp
	pdfium/third_party/icu/source/common/locresdata.cpp
	pdfium/third_party/icu/source/common/pluralmap.cpp
	pdfium/third_party/icu/source/common/uprops.cpp
	pdfium/third_party/icu/source/common/usetiter.cpp
	pdfium/third_party/icu/source/common/propname.cpp
	pdfium/third_party/icu/source/common/rbbistbl.cpp
	pdfium/third_party/icu/source/common/ucat.cpp
	pdfium/third_party/icu/source/common/unifilt.cpp
	pdfium/third_party/icu/source/common/utrie_swap.cpp
	pdfium/third_party/icu/source/common/uresdata.cpp
	pdfium/third_party/icu/source/common/umapfile.cpp
	pdfium/third_party/icu/source/common/messagepattern.cpp
	pdfium/third_party/icu/source/common/locavailable.cpp
	pdfium/third_party/icu/source/common/ustrcase.cpp
	pdfium/third_party/icu/source/common/cwchar.cpp
	pdfium/third_party/icu/source/common/rbbiscan.cpp
	pdfium/third_party/icu/source/common/umath.cpp
	pdfium/third_party/icu/source/common/uset.cpp
	pdfium/third_party/icu/source/common/ucln_cmn.cpp
	pdfium/third_party/icu/source/common/ures_cnv.cpp
	pdfium/third_party/icu/source/common/normlzr.cpp
	pdfium/third_party/icu/source/common/ucmndata.cpp
	pdfium/third_party/icu/source/common/servlk.cpp
	pdfium/third_party/icu/source/common/edits.cpp
	pdfium/third_party/icu/source/common/ucnvbocu.cpp
	pdfium/third_party/icu/source/common/ustrfmt.cpp
	pdfium/third_party/icu/source/common/parsepos.cpp
	pdfium/third_party/icu/source/common/loadednormalizer2impl.cpp
	pdfium/third_party/icu/source/common/udata.cpp
	pdfium/third_party/icu/source/common/errorcode.cpp
	pdfium/third_party/icu/source/common/uchriter.cpp
	pdfium/third_party/icu/source/common/ucnv_lmb.cpp
	pdfium/third_party/icu/source/common/ucurr.cpp
	pdfium/third_party/icu/source/common/udataswp.cpp
	pdfium/third_party/icu/source/common/restrace.cpp
	pdfium/third_party/icu/source/common/ustrcase_locale.cpp
	pdfium/third_party/icu/source/common/localematcher.cpp
	pdfium/third_party/icu/source/common/unistr.cpp
	pdfium/third_party/icu/source/common/ucnvhz.cpp
	pdfium/third_party/icu/source/common/normalizer2.cpp
	pdfium/third_party/icu/source/common/sharedobject.cpp
	pdfium/third_party/icu/source/common/resbund.cpp
	pdfium/third_party/icu/source/common/uhash_us.cpp
	pdfium/third_party/icu/source/common/serv.cpp
	pdfium/third_party/icu/source/common/utrie.cpp
	pdfium/third_party/icu/source/common/umutex.cpp
	pdfium/third_party/icu/source/common/unorm.cpp
	pdfium/third_party/icu/source/common/uidna.cpp
	pdfium/third_party/icu/source/common/servrbf.cpp
	pdfium/third_party/icu/source/common/ucnv_u7.cpp
	pdfium/third_party/icu/source/common/ruleiter.cpp
	pdfium/third_party/icu/source/common/uset_props.cpp
	pdfium/third_party/icu/source/common/appendable.cpp
	pdfium/third_party/icu/source/common/lsr.cpp
	pdfium/third_party/icu/source/common/utypes.cpp
	pdfium/third_party/icu/source/common/ucasemap.cpp
	pdfium/third_party/icu/source/common/usc_impl.cpp
	pdfium/third_party/icu/source/common/uresbund.cpp
	pdfium/third_party/icu/source/common/rbbidata.cpp
	pdfium/third_party/icu/source/common/unormcmp.cpp
	pdfium/third_party/icu/source/common/ucptrie.cpp
	pdfium/third_party/icu/source/common/ubidi_props.cpp
	pdfium/third_party/icu/source/common/uchar.cpp
	pdfium/third_party/icu/source/common/normalizer2impl.cpp
	pdfium/third_party/icu/source/common/ucnvisci.cpp
	pdfium/third_party/icu/source/common/unistr_cnv.cpp
	pdfium/third_party/icu/source/common/rbbirb.cpp
	pdfium/third_party/icu/source/common/unifunct.cpp
	pdfium/third_party/icu/source/common/characterproperties.cpp
	pdfium/third_party/icu/source/common/ustack.cpp
	pdfium/third_party/icu/source/common/uinvchar.cpp
	pdfium/third_party/icu/source/common/ucasemap_titlecase_brkiter.cpp
	pdfium/third_party/icu/source/common/bytestream.cpp
	pdfium/third_party/icu/source/common/ustring.cpp
	pdfium/third_party/icu/source/common/usprep.cpp
	pdfium/third_party/icu/source/common/servslkf.cpp
	pdfium/third_party/icu/source/common/rbbitblb.cpp
	pdfium/third_party/icu/source/common/ucnv_u16.cpp
	pdfium/third_party/icu/source/common/util_props.cpp
	pdfium/third_party/icu/source/common/uloc.cpp
	pdfium/third_party/icu/source/common/ustrenum.cpp
	pdfium/third_party/icu/source/common/ucnv_io.cpp
	pdfium/third_party/icu/source/common/unistr_case_locale.cpp
	pdfium/third_party/icu/source/common/uobject.cpp
	pdfium/third_party/icu/source/common/ucnv_cnv.cpp
	pdfium/third_party/icu/source/common/ucnv2022.cpp
	pdfium/third_party/icu/source/common/ucol_swp.cpp
	pdfium/third_party/icu/source/common/dtintrv.cpp
	pdfium/third_party/icu/source/common/rbbinode.cpp
	pdfium/third_party/icu/source/common/ucharstrie.cpp
	pdfium/third_party/icu/source/common/uscript.cpp
	pdfium/third_party/icu/source/common/utext.cpp
	pdfium/third_party/icu/source/common/locmap.cpp
	pdfium/third_party/icu/source/common/utrie2_builder.cpp
	pdfium/third_party/icu/source/common/uvectr32.cpp
	pdfium/third_party/icu/source/common/utrace.cpp
	pdfium/third_party/icu/source/common/brkiter.cpp
    pdfium/third_party/lcms/src/cmsalpha.c
    pdfium/third_party/lcms/src/cmscam02.c
    pdfium/third_party/lcms/src/cmscgats.c
    pdfium/third_party/lcms/src/cmscnvrt.c
    pdfium/third_party/lcms/src/cmserr.c
    pdfium/third_party/lcms/src/cmsgamma.c
    pdfium/third_party/lcms/src/cmsgmt.c
    pdfium/third_party/lcms/src/cmshalf.c
    pdfium/third_party/lcms/src/cmsintrp.c
    pdfium/third_party/lcms/src/cmsio0.c
    pdfium/third_party/lcms/src/cmsio1.c
    pdfium/third_party/lcms/src/cmslut.c
    pdfium/third_party/lcms/src/cmsmd5.c
    pdfium/third_party/lcms/src/cmsmtrx.c
    pdfium/third_party/lcms/src/cmsnamed.c
    pdfium/third_party/lcms/src/cmsopt.c
    pdfium/third_party/lcms/src/cmspack.c
    pdfium/third_party/lcms/src/cmspcs.c
    pdfium/third_party/lcms/src/cmsplugin.c
    pdfium/third_party/lcms/src/cmsps2.c
    pdfium/third_party/lcms/src/cmssamp.c
    pdfium/third_party/lcms/src/cmssm.c
    pdfium/third_party/lcms/src/cmstypes.c
    pdfium/third_party/lcms/src/cmsvirt.c
    pdfium/third_party/lcms/src/cmswtpnt.c
    pdfium/third_party/lcms/src/cmsxform.c
    pdfium/third_party/libjpeg_turbo/jsimd_none.c
    pdfium/third_party/libopenjpeg20/bio.c
    pdfium/third_party/libopenjpeg20/cio.c
    pdfium/third_party/libopenjpeg20/dwt.c
    pdfium/third_party/libopenjpeg20/event.c
    pdfium/third_party/libopenjpeg20/function_list.c
    pdfium/third_party/libopenjpeg20/image.c
    pdfium/third_party/libopenjpeg20/invert.c
    pdfium/third_party/libopenjpeg20/j2k.c
    pdfium/third_party/libopenjpeg20/jp2.c
    pdfium/third_party/libopenjpeg20/mct.c
    pdfium/third_party/libopenjpeg20/mqc.c
    pdfium/third_party/libopenjpeg20/openjpeg.c
    pdfium/third_party/libopenjpeg20/opj_clock.c
    pdfium/third_party/libopenjpeg20/opj_malloc.cc
    pdfium/third_party/libopenjpeg20/pi.c
    pdfium/third_party/libopenjpeg20/sparse_array.c
    pdfium/third_party/libopenjpeg20/t1.c
    # pdfium/third_party/libopenjpeg20/t1_generate_luts.c
    pdfium/third_party/libopenjpeg20/t2.c
    pdfium/third_party/libopenjpeg20/tcd.c
    pdfium/third_party/libopenjpeg20/tgt.c
    pdfium/third_party/libopenjpeg20/thread.c
	pdfium/third_party/zlib/adler32.c
    pdfium/third_party/zlib/adler32_simd.c
    pdfium/third_party/zlib/compress.c
    pdfium/third_party/zlib/cpu_features.c
    pdfium/third_party/zlib/crc32.c
    pdfium/third_party/zlib/crc32_simd.c
    pdfium/third_party/zlib/gzlib.c
    pdfium/third_party/zlib/deflate.c
    pdfium/third_party/zlib/inffast.c
    pdfium/third_party/zlib/inflate.c
    pdfium/third_party/zlib/inftrees.c
    pdfium/third_party/zlib/trees.c
    pdfium/third_party/zlib/uncompr.c
    pdfium/third_party/zlib/zutil.c
    # pdfium/xfa/fgas/font/cfgas_gemodule.cpp
    # pdfium/xfa/fgas/font/cfgas_gefont.cpp    
    # pdfium/xfa/fgas/font/cfgas_fontmgr.cpp
    # pdfium/xfa/fgas/font/cfgas_pdffontmgr.cpp
    # pdfium/xfa/fgas/font/fgas_fontutils.cpp
    # pdfium/xfa/fgas/layout/fx_arabic.cpp
    # pdfium/xfa/fgas/layout/cfgas_char.cpp
    # pdfium/xfa/fgas/crt/cfgas_decimal.cpp
    pdfium/core/fdrm/fx_crypt.cpp
    pdfium/core/fdrm/fx_crypt_aes.cpp
    pdfium/core/fdrm/fx_crypt_sha.cpp
    pdfium/core/fpdfapi/cmaps/CNS1/Adobe-CNS1-UCS2_5.cpp
    pdfium/core/fpdfapi/cmaps/CNS1/B5pc-H_0.cpp
    pdfium/core/fpdfapi/cmaps/CNS1/B5pc-V_0.cpp
    pdfium/core/fpdfapi/cmaps/CNS1/CNS-EUC-H_0.cpp
    pdfium/core/fpdfapi/cmaps/CNS1/CNS-EUC-V_0.cpp
    pdfium/core/fpdfapi/cmaps/CNS1/ETen-B5-H_0.cpp
    pdfium/core/fpdfapi/cmaps/CNS1/ETen-B5-V_0.cpp
    pdfium/core/fpdfapi/cmaps/CNS1/ETenms-B5-H_0.cpp
    pdfium/core/fpdfapi/cmaps/CNS1/ETenms-B5-V_0.cpp
    pdfium/core/fpdfapi/cmaps/CNS1/HKscs-B5-H_5.cpp
    pdfium/core/fpdfapi/cmaps/CNS1/HKscs-B5-V_5.cpp
    pdfium/core/fpdfapi/cmaps/CNS1/UniCNS-UCS2-H_3.cpp
    pdfium/core/fpdfapi/cmaps/CNS1/UniCNS-UCS2-V_3.cpp
    pdfium/core/fpdfapi/cmaps/CNS1/UniCNS-UTF16-H_0.cpp
    pdfium/core/fpdfapi/cmaps/CNS1/cmaps_cns1.cpp
    pdfium/core/fpdfapi/cmaps/GB1/Adobe-GB1-UCS2_5.cpp
    pdfium/core/fpdfapi/cmaps/GB1/GB-EUC-H_0.cpp
    pdfium/core/fpdfapi/cmaps/GB1/GB-EUC-V_0.cpp
    pdfium/core/fpdfapi/cmaps/GB1/GBK-EUC-H_2.cpp
    pdfium/core/fpdfapi/cmaps/GB1/GBK-EUC-V_2.cpp
    pdfium/core/fpdfapi/cmaps/GB1/GBK2K-H_5.cpp
    pdfium/core/fpdfapi/cmaps/GB1/GBK2K-V_5.cpp
    pdfium/core/fpdfapi/cmaps/GB1/GBKp-EUC-H_2.cpp
    pdfium/core/fpdfapi/cmaps/GB1/GBKp-EUC-V_2.cpp
    pdfium/core/fpdfapi/cmaps/GB1/GBpc-EUC-H_0.cpp
    pdfium/core/fpdfapi/cmaps/GB1/GBpc-EUC-V_0.cpp
    pdfium/core/fpdfapi/cmaps/GB1/UniGB-UCS2-H_4.cpp
    pdfium/core/fpdfapi/cmaps/GB1/UniGB-UCS2-V_4.cpp
    pdfium/core/fpdfapi/cmaps/GB1/cmaps_gb1.cpp
    pdfium/core/fpdfapi/cmaps/Japan1/83pv-RKSJ-H_1.cpp
    pdfium/core/fpdfapi/cmaps/Japan1/90ms-RKSJ-H_2.cpp
    pdfium/core/fpdfapi/cmaps/Japan1/90ms-RKSJ-V_2.cpp
    pdfium/core/fpdfapi/cmaps/Japan1/90msp-RKSJ-H_2.cpp
    pdfium/core/fpdfapi/cmaps/Japan1/90msp-RKSJ-V_2.cpp
    pdfium/core/fpdfapi/cmaps/Japan1/90pv-RKSJ-H_1.cpp
    pdfium/core/fpdfapi/cmaps/Japan1/Add-RKSJ-H_1.cpp
    pdfium/core/fpdfapi/cmaps/Japan1/Add-RKSJ-V_1.cpp
    pdfium/core/fpdfapi/cmaps/Japan1/Adobe-Japan1-UCS2_4.cpp
    pdfium/core/fpdfapi/cmaps/Japan1/EUC-H_1.cpp
    pdfium/core/fpdfapi/cmaps/Japan1/EUC-V_1.cpp
    pdfium/core/fpdfapi/cmaps/Japan1/Ext-RKSJ-H_2.cpp
    pdfium/core/fpdfapi/cmaps/Japan1/Ext-RKSJ-V_2.cpp
    pdfium/core/fpdfapi/cmaps/Japan1/H_1.cpp
    pdfium/core/fpdfapi/cmaps/Japan1/UniJIS-UCS2-HW-H_4.cpp
    pdfium/core/fpdfapi/cmaps/Japan1/UniJIS-UCS2-HW-V_4.cpp
    pdfium/core/fpdfapi/cmaps/Japan1/UniJIS-UCS2-H_4.cpp
    pdfium/core/fpdfapi/cmaps/Japan1/UniJIS-UCS2-V_4.cpp
    pdfium/core/fpdfapi/cmaps/Japan1/V_1.cpp
    pdfium/core/fpdfapi/cmaps/Japan1/cmaps_japan1.cpp
    pdfium/core/fpdfapi/cmaps/Korea1/Adobe-Korea1-UCS2_2.cpp
    pdfium/core/fpdfapi/cmaps/Korea1/KSC-EUC-H_0.cpp
    pdfium/core/fpdfapi/cmaps/Korea1/KSC-EUC-V_0.cpp
    pdfium/core/fpdfapi/cmaps/Korea1/KSCms-UHC-HW-H_1.cpp
    pdfium/core/fpdfapi/cmaps/Korea1/KSCms-UHC-HW-V_1.cpp
    pdfium/core/fpdfapi/cmaps/Korea1/KSCms-UHC-H_1.cpp
    pdfium/core/fpdfapi/cmaps/Korea1/KSCms-UHC-V_1.cpp
    pdfium/core/fpdfapi/cmaps/Korea1/KSCpc-EUC-H_0.cpp
    pdfium/core/fpdfapi/cmaps/Korea1/UniKS-UCS2-H_1.cpp
    pdfium/core/fpdfapi/cmaps/Korea1/UniKS-UCS2-V_1.cpp
    pdfium/core/fpdfapi/cmaps/Korea1/UniKS-UTF16-H_0.cpp
    pdfium/core/fpdfapi/cmaps/Korea1/cmaps_korea1.cpp
    pdfium/core/fpdfapi/cmaps/fpdf_cmaps.cpp
    pdfium/core/fpdfapi/page/cpdf_allstates.cpp
    pdfium/core/fpdfapi/page/cpdf_clippath.cpp
    pdfium/core/fpdfapi/page/cpdf_color.cpp
    pdfium/core/fpdfapi/page/cpdf_colorspace.cpp
    pdfium/core/fpdfapi/page/cpdf_colorstate.cpp
    pdfium/core/fpdfapi/page/cpdf_contentmarks.cpp
    pdfium/core/fpdfapi/page/cpdf_contentmarkitem.cpp
    pdfium/core/fpdfapi/page/cpdf_contentparser.cpp
    pdfium/core/fpdfapi/page/cpdf_docpagedata.cpp
    pdfium/core/fpdfapi/page/cpdf_form.cpp
    pdfium/core/fpdfapi/page/cpdf_formobject.cpp
    pdfium/core/fpdfapi/page/cpdf_generalstate.cpp
    pdfium/core/fpdfapi/page/cpdf_graphicstates.cpp
    pdfium/core/fpdfapi/page/cpdf_image.cpp
    pdfium/core/fpdfapi/page/cpdf_imageobject.cpp
    pdfium/core/fpdfapi/page/cpdf_meshstream.cpp
    pdfium/core/fpdfapi/page/cpdf_occontext.cpp
    pdfium/core/fpdfapi/page/cpdf_page.cpp
    pdfium/core/fpdfapi/page/cpdf_pagemodule.cpp
    pdfium/core/fpdfapi/page/cpdf_pageobject.cpp
    pdfium/core/fpdfapi/page/cpdf_pageobjectholder.cpp
    pdfium/core/fpdfapi/page/cpdf_path.cpp
    pdfium/core/fpdfapi/page/cpdf_pathobject.cpp
    pdfium/core/fpdfapi/page/cpdf_pattern.cpp
    pdfium/core/fpdfapi/page/cpdf_shadingobject.cpp
    pdfium/core/fpdfapi/page/cpdf_shadingpattern.cpp
    pdfium/core/fpdfapi/page/cpdf_streamcontentparser.cpp
    pdfium/core/fpdfapi/page/cpdf_streamparser.cpp
    pdfium/core/fpdfapi/page/cpdf_textobject.cpp
    pdfium/core/fpdfapi/page/cpdf_textstate.cpp
    pdfium/core/fpdfapi/page/cpdf_tilingpattern.cpp
    pdfium/core/fpdfapi/page/cpdf_transferfunc.cpp
    pdfium/core/fpdfapi/page/cpdf_transferfuncdib.cpp
    pdfium/core/fpdfapi/page/cpdf_dib.cpp
    pdfium/core/fpdfapi/parser/cfdf_document.cpp
    pdfium/core/fpdfapi/parser/cpdf_array.cpp
    pdfium/core/fpdfapi/parser/cpdf_boolean.cpp
    pdfium/core/fpdfapi/parser/cpdf_crypto_handler.cpp
    pdfium/core/fpdfapi/parser/cpdf_data_avail.cpp
    pdfium/core/fpdfapi/parser/cpdf_dictionary.cpp
    pdfium/core/fpdfapi/parser/cpdf_document.cpp
    pdfium/core/fpdfapi/parser/cpdf_hint_tables.cpp
    pdfium/core/fpdfapi/parser/cpdf_indirect_object_holder.cpp
    pdfium/core/fpdfapi/parser/cpdf_linearized_header.cpp
    pdfium/core/fpdfapi/parser/cpdf_name.cpp
    pdfium/core/fpdfapi/parser/cpdf_null.cpp
    pdfium/core/fpdfapi/parser/cpdf_number.cpp
    pdfium/core/fpdfapi/parser/cpdf_object.cpp
    pdfium/core/fpdfapi/parser/cpdf_parser.cpp
    pdfium/core/fpdfapi/parser/cpdf_reference.cpp
    pdfium/core/fpdfapi/parser/cpdf_security_handler.cpp
    pdfium/core/fpdfapi/parser/cpdf_seekablemultistream.cpp
    pdfium/core/fpdfapi/parser/cpdf_simple_parser.cpp
    pdfium/core/fpdfapi/parser/cpdf_stream.cpp
    pdfium/core/fpdfapi/parser/cpdf_stream_acc.cpp
    pdfium/core/fpdfapi/parser/cpdf_string.cpp
    pdfium/core/fpdfapi/parser/fpdf_parser_decode.cpp
    pdfium/core/fpdfapi/parser/fpdf_parser_utility.cpp
    # pdfium/core/fpdfapi/render/charposlist.cpp
    # pdfium/core/fpdfapi/render/cpdf_devicebuffer.cpp
    pdfium/core/fpdfapi/render/cpdf_docrenderdata.cpp
    # pdfium/core/fpdfapi/render/cpdf_imagecacheentry.cpp
    # pdfium/core/fpdfapi/render/cpdf_imageloader.cpp
    # pdfium/core/fpdfapi/render/cpdf_imagerenderer.cpp
    # pdfium/core/fpdfapi/render/cpdf_pagerendercache.cpp
    # pdfium/core/fpdfapi/render/cpdf_pagerendercontext.cpp
    # pdfium/core/fpdfapi/render/cpdf_progressiverenderer.cpp
    # pdfium/core/fpdfapi/render/cpdf_rendercontext.cpp
    # pdfium/core/fpdfapi/render/cpdf_renderoptions.cpp
    # pdfium/core/fpdfapi/render/cpdf_renderstatus.cpp
    # pdfium/core/fpdfapi/render/cpdf_rendershading.cpp
    # pdfium/core/fpdfapi/render/cpdf_rendertiling.cpp
    # pdfium/core/fpdfapi/render/cpdf_scaledrenderbuffer.cpp
    # pdfium/core/fpdfapi/render/cpdf_textrenderer.cpp
    pdfium/core/fpdfapi/render/cpdf_type3cache.cpp
    pdfium/core/fpdfapi/render/cpdf_type3glyphmap.cpp
    # pdfium/core/fpdfdoc/cpdf_aaction.cpp
    # pdfium/core/fpdfdoc/cpdf_action.cpp
    # pdfium/core/fpdfdoc/cpdf_annot.cpp
    # pdfium/core/fpdfdoc/cpdf_annotlist.cpp
    # pdfium/core/fpdfdoc/cpdf_apsettings.cpp
    # pdfium/core/fpdfdoc/cpdf_defaultappearance.cpp
    # pdfium/core/fpdfdoc/cpdf_formcontrol.cpp
    # pdfium/core/fpdfdoc/cpdf_formfield.cpp
    # pdfium/core/fpdfdoc/cpdf_interactiveform.cpp
    # pdfium/core/fpdfdoc/cpdf_link.cpp
    # pdfium/core/fpdfdoc/cpdf_linklist.cpp
    # pdfium/core/fpdfdoc/cpdf_nametree.cpp
    # pdfium/core/fpdfdoc/cpvt_variabletext.cpp
    # pdfium/core/fpdfdoc/cpvt_fontmap.cpp
    # pdfium/core/fpdfdoc/cpdf_generateap.cpp
    # pdfium/core/fpdfdoc/cpvt_section.cpp
    # pdfium/core/fpdfdoc/cpvt_wordinfo.cpp
    # pdfium/core/fpdfdoc/cpdf_bookmark.cpp
    # pdfium/core/fpdfdoc/cpdf_dest.cpp
    # pdfium/core/fpdfdoc/cpdf_filespec.cpp
    # pdfium/core/fpdfdoc/cpdf_icon.cpp
    # pdfium/core/fpdfdoc/cpdf_iconfit.cpp
    # pdfium/core/fpdfdoc/cpdf_metadata.cpp
    # pdfium/core/fpdfdoc/cpdf_numbertree.cpp
    # pdfium/core/fpdfdoc/cpdf_color_utils.cpp
    pdfium/core/fxcodec/fx_codec.cpp
    pdfium/core/fxcodec/basic/basicmodule.cpp
    pdfium/core/fxcodec/fax/faxmodule.cpp
    pdfium/core/fxcodec/flate/flatemodule.cpp
    pdfium/core/fxcodec/icc/icc_transform.cpp
    pdfium/core/fxcodec/jbig2/jbig2_decoder.cpp
    pdfium/core/fxcodec/jbig2/JBig2_ArithDecoder.cpp
    pdfium/core/fxcodec/jbig2/JBig2_ArithIntDecoder.cpp
    pdfium/core/fxcodec/jbig2/JBig2_BitStream.cpp
    pdfium/core/fxcodec/jbig2/JBig2_Context.cpp
    pdfium/core/fxcodec/jbig2/JBig2_DocumentContext.cpp
    pdfium/core/fxcodec/jbig2/JBig2_GrdProc.cpp
    pdfium/core/fxcodec/jbig2/JBig2_GrrdProc.cpp
    pdfium/core/fxcodec/jbig2/JBig2_HtrdProc.cpp
    pdfium/core/fxcodec/jbig2/JBig2_HuffmanDecoder.cpp
    pdfium/core/fxcodec/jbig2/JBig2_HuffmanTable.cpp
    pdfium/core/fxcodec/jbig2/JBig2_Image.cpp
    pdfium/core/fxcodec/jbig2/JBig2_PatternDict.cpp
    pdfium/core/fxcodec/jbig2/JBig2_PddProc.cpp
    pdfium/core/fxcodec/jbig2/JBig2_SddProc.cpp
    pdfium/core/fxcodec/jbig2/JBig2_Segment.cpp
    pdfium/core/fxcodec/jbig2/JBig2_SymbolDict.cpp
    pdfium/core/fxcodec/jbig2/JBig2_TrdProc.cpp
	pdfium/core/fxcodec/jpeg/jpegmodule.cpp
    pdfium/core/fxcodec/jpeg/jpeg_common.cpp
    pdfium/core/fxcodec/jpx/cjpx_decoder.cpp
    pdfium/core/fxcodec/jpx/jpx_decode_utils.cpp
	pdfium/core/fxge/agg/fx_agg_driver.cpp
    # libs/pdfium/core/fxge/apple/fx_apple_platform.cpp
    # libs/pdfium/core/fxge/apple/fx_mac_impl.cpp
    # libs/pdfium/core/fxge/apple/fx_quartz_device.cpp
    pdfium/core/fxge/fontdata/chromefontdata/FoxitDingbats.cpp
    pdfium/core/fxge/fontdata/chromefontdata/FoxitFixed.cpp
    pdfium/core/fxge/fontdata/chromefontdata/FoxitFixedBold.cpp
    pdfium/core/fxge/fontdata/chromefontdata/FoxitFixedBoldItalic.cpp
    pdfium/core/fxge/fontdata/chromefontdata/FoxitFixedItalic.cpp
    pdfium/core/fxge/fontdata/chromefontdata/FoxitSans.cpp
    pdfium/core/fxge/fontdata/chromefontdata/FoxitSansBold.cpp
    pdfium/core/fxge/fontdata/chromefontdata/FoxitSansBoldItalic.cpp
    pdfium/core/fxge/fontdata/chromefontdata/FoxitSansItalic.cpp
    pdfium/core/fxge/fontdata/chromefontdata/FoxitSansMM.cpp
    pdfium/core/fxge/fontdata/chromefontdata/FoxitSerif.cpp
    pdfium/core/fxge/fontdata/chromefontdata/FoxitSerifBold.cpp
    pdfium/core/fxge/fontdata/chromefontdata/FoxitSerifBoldItalic.cpp
    pdfium/core/fxge/fontdata/chromefontdata/FoxitSerifItalic.cpp
    pdfium/core/fxge/fontdata/chromefontdata/FoxitSerifMM.cpp
    pdfium/core/fxge/fontdata/chromefontdata/FoxitSymbol.cpp
    pdfium/core/fxge/freetype/fx_freetype.cpp
    pdfium/core/fxge/text_char_pos.cpp
    pdfium/third_party/agg23/agg_curves.cpp
    pdfium/third_party/agg23/agg_path_storage.cpp
    pdfium/third_party/agg23/agg_rasterizer_scanline_aa.cpp
    pdfium/third_party/agg23/agg_vcgen_dash.cpp
    pdfium/third_party/agg23/agg_vcgen_stroke.cpp
    pdfium/third_party/libjpeg_turbo/jaricom.c
    pdfium/third_party/libjpeg_turbo/jcapimin.c
    pdfium/third_party/libjpeg_turbo/jcapistd.c
    pdfium/third_party/libjpeg_turbo/jcarith.c
    pdfium/third_party/libjpeg_turbo/jccoefct.c
    pdfium/third_party/libjpeg_turbo/jccolor.c
    pdfium/third_party/libjpeg_turbo/jcdctmgr.c
    pdfium/third_party/libjpeg_turbo/jchuff.c
    pdfium/third_party/libjpeg_turbo/jcinit.c
    pdfium/third_party/libjpeg_turbo/jcmainct.c
    pdfium/third_party/libjpeg_turbo/jcmarker.c
    pdfium/third_party/libjpeg_turbo/jcmaster.c
    pdfium/third_party/libjpeg_turbo/jcomapi.c
    pdfium/third_party/libjpeg_turbo/jcparam.c
    pdfium/third_party/libjpeg_turbo/jcphuff.c
    pdfium/third_party/libjpeg_turbo/jcprepct.c
    pdfium/third_party/libjpeg_turbo/jcsample.c
    pdfium/third_party/libjpeg_turbo/jdapimin.c
    pdfium/third_party/libjpeg_turbo/jdapistd.c
    pdfium/third_party/libjpeg_turbo/jdarith.c
    pdfium/third_party/libjpeg_turbo/jdatadst.c
    pdfium/third_party/libjpeg_turbo/jdcoefct.c
    pdfium/third_party/libjpeg_turbo/jdcolor.c
    pdfium/third_party/libjpeg_turbo/jddctmgr.c
    pdfium/third_party/libjpeg_turbo/jdhuff.c
    pdfium/third_party/libjpeg_turbo/jdinput.c
    pdfium/third_party/libjpeg_turbo/jdmainct.c
    pdfium/third_party/libjpeg_turbo/jdmarker.c
    pdfium/third_party/libjpeg_turbo/jdmaster.c
    pdfium/third_party/libjpeg_turbo/jdmerge.c
    pdfium/third_party/libjpeg_turbo/jdphuff.c
    pdfium/third_party/libjpeg_turbo/jdpostct.c
    pdfium/third_party/libjpeg_turbo/jdsample.c
    pdfium/third_party/libjpeg_turbo/jerror.c
    pdfium/third_party/libjpeg_turbo/jfdctflt.c
    pdfium/third_party/libjpeg_turbo/jfdctfst.c
    pdfium/third_party/libjpeg_turbo/jfdctint.c
    pdfium/third_party/libjpeg_turbo/jidctflt.c
    pdfium/third_party/libjpeg_turbo/jidctfst.c
    pdfium/third_party/libjpeg_turbo/jidctint.c
    pdfium/third_party/libjpeg_turbo/jidctred.c
    pdfium/third_party/libjpeg_turbo/jmemmgr.c
    pdfium/third_party/libjpeg_turbo/jmemnobs.c
    pdfium/third_party/libjpeg_turbo/jquant1.c
    pdfium/third_party/libjpeg_turbo/jquant2.c
    pdfium/third_party/libjpeg_turbo/jpeg_nbits_table.c
    pdfium/third_party/libjpeg_turbo/jutils.c
    # pdfium/third_party/libtiff/tif_aux.c
    # pdfium/third_party/libtiff/tif_close.c
    # pdfium/third_party/libtiff/tif_codec.c
    # pdfium/third_party/libtiff/tif_color.c
    # pdfium/third_party/libtiff/tif_compress.c
    # pdfium/third_party/libtiff/tif_dir.c
    # pdfium/third_party/libtiff/tif_dirinfo.c
    # pdfium/third_party/libtiff/tif_dirread.c
    # pdfium/third_party/libtiff/tif_dirwrite.c
    # pdfium/third_party/libtiff/tif_dumpmode.c
    # pdfium/third_party/libtiff/tif_error.c
    # pdfium/third_party/libtiff/tif_extension.c
    # pdfium/third_party/libtiff/tif_fax3.c
    # pdfium/third_party/libtiff/tif_fax3sm.c
    # pdfium/third_party/libtiff/tif_flush.c
    # pdfium/third_party/libtiff/tif_getimage.c
    # pdfium/third_party/libtiff/tif_jpeg.c
    # pdfium/third_party/libtiff/tif_luv.c
    # pdfium/third_party/libtiff/tif_lzw.c
    # pdfium/third_party/libtiff/tif_next.c
    # pdfium/third_party/libtiff/tif_ojpeg.c
    # pdfium/third_party/libtiff/tif_open.c
    # pdfium/third_party/libtiff/tif_packbits.c
    # pdfium/third_party/libtiff/tif_pixarlog.c
    # pdfium/third_party/libtiff/tif_predict.c
    # pdfium/third_party/libtiff/tif_print.c
    # pdfium/third_party/libtiff/tif_read.c
    # pdfium/third_party/libtiff/tif_strip.c
    # pdfium/third_party/libtiff/tif_swab.c
    # pdfium/third_party/libtiff/tif_thunder.c
    # pdfium/third_party/libtiff/tif_tile.c
    # pdfium/third_party/libtiff/tif_version.c
    # pdfium/third_party/libtiff/tif_warning.c
    # pdfium/third_party/libtiff/tif_write.c
    # pdfium/third_party/libtiff/tif_zip.c
)

if (WIN32)
    set(SRC_PDFIUM_PLATFORM
    pdfium/core/fxge/win32/cwin32_platform.cpp
    pdfium/core/fxge/win32/cgdi_plus_ext.cpp
    pdfium/core/fxcrt/fx_folder_windows.cpp
	pdfium/core/fxcrt/cfx_fileaccess_windows.cpp
	pdfium/core/fxcrt/fx_folder_windows.cpp		
	pdfium/third_party/base/win/win_util.cc
	pdfium/core/fxge/win32/cfx_psfonttracker.cpp	
    )
elseif(APPLE)
    set(SRC_PDFIUM_PLATFORM
    pdfium/core/fxge/apple/fx_apple_platform.cpp
    pdfium/core/fxge/apple/fx_apple_impl.cpp
    pdfium/core/fxge/apple/fx_quartz_device.cpp
	pdfium/core/fxcrt/cfx_fileaccess_posix.cpp
	pdfium/core/fxcrt/fx_folder_posix.cpp	
    )
elseif(UNIX)
    set(SRC_PDFIUM_PLATFORM
      pdfium/core/src/fxge/ge/fx_ge_linux.cpp
    )
endif()

# =========== common ===========
if (WIN32)
    if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/x86)
    else()
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/x64)
    endif()
elseif(APPLE)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/darwin)
elseif(UNIX)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/linux)
endif()

add_executable(TestGrammar
    src/Main.cpp
    ${SOURCES}
    #${SRC_PDFIX}
    #${SRC_QPDF}
    ${SRC_PDFIUM}
    ${SRC_PDFIUM_PLATFORM}
    )

set_target_properties(TestGrammar PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})

target_include_directories(TestGrammar
PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/sarge"
    "${CMAKE_CURRENT_SOURCE_DIR}/pdfium"
    "${CMAKE_CURRENT_SOURCE_DIR}/pdfium/third_party/icu/source/common"
	"${CMAKE_CURRENT_SOURCE_DIR}/pdfium/third_party/freetype/src/include"
	"${CMAKE_CURRENT_SOURCE_DIR}/pdfium/third_party/freetype/src/include/freetype"	
	"${CMAKE_CURRENT_SOURCE_DIR}/pdfium/third_party/zlib"		
    "${CMAKE_CURRENT_SOURCE_DIR}/pdfix"
    "${CMAKE_CURRENT_SOURCE_DIR}/qpdf"
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
