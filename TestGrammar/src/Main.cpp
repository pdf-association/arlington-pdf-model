///////////////////////////////////////////////////////////////////////////////
/// @file 
/// @brief TestGrammar proof-of-concept main program
/// 
/// TestGrammar proof-of-concept main program: command line option processing,
/// initialization of PDF SDK, setting up output streams, etc.
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

#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#endif

#include <exception>
#include <iostream>
#include <string>
#include <vector>

#if defined __linux__
#include <cstring>
#endif

#include "ArlingtonPDFShim.h"
#include "ArlPredicates.h"
#include "ParseObjects.h"
#include "CheckGrammar.h"
#include "TestGrammarVers.h"
#include "PDFFile.h"
#include "sarge.h"
#include "utils.h"

using namespace ArlingtonPDFShim;
namespace fs = std::filesystem;


/// @brief /dev/null equivalent streams for chars - see https://stackoverflow.com/questions/6240950/platform-independent-dev-null-in-c#6240980
std::ostream  cnull(0);

/// @brief /dev/null equivalent stream for wide chars - see https://stackoverflow.com/questions/6240950/platform-independent-dev-null-in-c#6240980
std::wostream wcnull(0);

/// @brief Global control over colorized output
bool no_color = false;

/// @brief Validates a single PDF file against the Arlington PDF model
///
/// @param[in] pdf_file_name  PDF filename for processing
/// @param[in] tsv_folder  the folder with the Arlington TSV model files
/// @param[in] pdfsdk      the already initiated PDF SDK library to use
/// @param[in] ofs         already open file stream for output
/// @param[in] terse       terse style (brief) output (will sort | uniq better under Linux CLI)
/// @param[in] debug_mode  verbose style output (PDF-file specific information e.g. object numbers)
/// @param[in] forced_ver  forced PDF version or empty string to use PDF
/// @param[in] extns       list of extension names to support
/// 
/// @returns true on success. false on a fatal error
bool process_single_pdf(const fs::path& pdf_file_name, const fs::path& tsv_folder, ArlingtonPDFSDK& pdfsdk, std::ostream& ofs, const bool terse, const bool debug_mode, const std::string& forced_ver, std::vector<std::string>& extns)
{
    bool retval = true;
    try
    {
        ofs << "BEGIN - TestGrammar " << TestGrammar_VERSION << " " << pdfsdk.get_version_string() << std::endl;
        ofs << "Arlington TSV data: " << fs::absolute(tsv_folder).lexically_normal() << std::endl;
        ofs << "PDF: " << fs::absolute(pdf_file_name).lexically_normal() << std::endl;

        CParsePDF parser(tsv_folder, ofs, terse, debug_mode);
        CPDFFile  pdf(pdf_file_name, pdfsdk, forced_ver, extns);

        ArlPDFTrailer* t = pdf.get_trailer();
        if (t != nullptr) {
            if (pdf.uses_xref_stream()) {
                ofs << COLOR_INFO << "XRefStream detected." << COLOR_RESET;
                parser.add_root_parse_object(t, "XRefStream", "Trailer (as XRefStream)");
            }
            else {
                ofs << COLOR_INFO << "Traditional trailer dictionary detected." << COLOR_RESET;
                parser.add_root_parse_object(t, "FileTrailer", "Trailer");
            }

            retval = parser.parse_object(pdf);
            if (retval) {
                ofs << COLOR_INFO << "Latest Arlington feature was" << pdf.get_latest_feature_version_info() << " compared using" << (pdf.is_forced_version() ? " forced" : "") << " PDF " << pdf.pdf_version;
                if (extns.size() > 0) {
                    ofs << " with extensions ";
                    for (size_t i = 0; i < extns.size(); i++)
                        ofs << extns[i] << ((i < (extns.size() - 1)) ? ", " : "");
                }
                ofs << COLOR_RESET;
            }
        }
        else {
            ofs << COLOR_ERROR << "failed to acquire Trailer" << COLOR_RESET;
        }
    }
    catch (std::exception& ex) {
        ofs << COLOR_ERROR << "EXCEPTION: " << ex.what() << COLOR_RESET;
        retval = false;
    }

    ofs << "END" << std::endl;
    return retval;
};



#if defined(_WIN32) || defined(WIN32)
#include <crtdbg.h>

/// @brief #define CRT_MEMORY_LEAK_CHECK to enable C RTL memory leak checking (slow!)
#undef CRT_MEMORY_LEAK_CHECK

int wmain(int argc, wchar_t* argv[]) {
#if defined(_DEBUG) && defined(CRT_MEMORY_LEAK_CHECK)
    int tmp = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    tmp = tmp | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF; // | _CRTDBG_CHECK_ALWAYS_DF; // _CRTDBG_CHECK_ALWAYS_DF is VERY slow!!
    _CrtSetDbgFlag(tmp);
    //_CrtSetBreakAlloc(2987);
#endif // _DEBUG && CRT_MEMORY_LEAK_CHECK

    // Convert wchar_t* to char* for command line processing
    mbstate_t   state;
    char** mbcsargv = new char* [argc];

    for (int i = 0; i < argc; i++) {
        memset(&state, 0, sizeof state);
        size_t len = 1 + wcsrtombs(NULL, (const wchar_t**)&argv[i], 0, &state);
        char* mbstr = new char[len];
        wcsrtombs(mbstr, (const wchar_t**)&argv[i], len, &state);
        mbcsargv[i] = mbstr;
    }

    // Simplistic attempt at support for folder and filenames from non-std code pages...
    setlocale(LC_CTYPE, "en_US.UTF-8");
#else
int main(int argc, char* argv[]) {
#endif // _WIN32 || WIN32

    ArlingtonPDFSDK pdf_io;             // PDF SDK
    Sarge           sarge;              // Command line option processing

    sarge.setDescription("Arlington PDF Model C++ P.o.C. version " TestGrammar_VERSION
        "\nChoose one of: --pdf, --checkdva or --validate.");
    sarge.setUsage("TestGrammar --tsvdir <dir> [--force <ver>|exact] [--out <fname|dir>] [--no-color] [--clobber] [--debug] [--brief] [--extensions <extn1[,extn2]>] [--validate | --checkdva <formalrep> | --pdf <fname|dir> ]");
    sarge.setArgument("h", "help", "This usage message.", false);
    sarge.setArgument("b", "brief", "terse output when checking PDFs. The full PDF DOM tree is NOT output.", false);
    sarge.setArgument("c", "checkdva", "Adobe DVA formal-rep PDF file to compare against Arlington PDF model.", true);
    sarge.setArgument("d", "debug", "output additional debugging information (verbose!)", false);
    sarge.setArgument("",  "clobber", "always overwrite PDF output report files (--pdf) rather than append underscores", false);
    sarge.setArgument("",  "no-color", "disable colorized text output (useful when redirecting or piping output)", false);
    sarge.setArgument("m", "batchmode", "stop popup error dialog windows and redirect everything to console (Windows only, includes memory leak reports)", false);
    sarge.setArgument("o", "out", "output file or folder. Default is stdout. See --clobber for overwriting behavior", true);
    sarge.setArgument("p", "pdf", "input PDF file or folder.", true);
    sarge.setArgument("f", "force", "force the PDF version to the specified value (1,0, 1.1, ..., 2.0 or 'exact'). Requires --pdf", true);
    sarge.setArgument("t", "tsvdir", "[required] folder containing Arlington PDF model TSV file set.", true);
    sarge.setArgument("v", "validate", "validate the Arlington PDF model.", false);
    sarge.setArgument("e", "extensions", "a comma-separated list of extensions, or '*' for all extensions.", true);

#if defined(_WIN32) || defined(WIN32)
    if (!sarge.parseArguments(argc, mbcsargv)) {
#else
    if (!sarge.parseArguments(argc, argv)) {
#endif
        std::cerr << COLOR_ERROR << "error parsing command line arguments" << COLOR_RESET;
#if defined(_WIN32) || defined(WIN32)
        // Delete the temp stuff for command line processing
        for (int i = 0; i < argc; i++)
            delete[] mbcsargv[i];
        delete[] mbcsargv;
#endif
        sarge.printHelp();
        pdf_io.shutdown();
        return -1;
    }
    
    pdf_io.initialize();    // Start up the PDF SDK - this may throw exceptions depending on PDF SDK!

    if (sarge.exists("help") || (argc == 1)) {
#if defined(_WIN32) || defined(WIN32)
        // Delete the temp stuff for command line processing
        for (int i = 0; i < argc; i++)
            delete[] mbcsargv[i];
        delete[] mbcsargv;
#endif
        sarge.printHelp();
        std::cout << "\nBuilt using " << pdf_io.get_version_string() << std::endl;
        pdf_io.shutdown();
        return 0;
    }

    int             retval = 0;         // return value to O/S
    std::string     s;                  // temp
    fs::path        grammar_folder;     // folder with TSV files (must exist)
    fs::path        save_path;          // output file or folder. Optional.
    fs::path        input_file;         // input PDF or Adobe DVA
    std::ofstream   ofs;                // output filestream
    std::string     force_version;      // Optional forced PDF version
    bool            clobber = sarge.exists("clobber");
    bool            debug_mode = sarge.exists("debug");
    bool            terse = sarge.exists("brief");
    std::vector<std::string> supported_extns;

    no_color = sarge.exists("no-color");

#if defined(_WIN32) || defined(WIN32)
    // Delete the temp stuff for command line processing
    for (int i = 0; i < argc; i++)
        delete[] mbcsargv[i];
    delete[] mbcsargv;

    if (sarge.exists("batchmode")) {
        // Suppress popup dialogs for assertions, errors and mem leaks - send to stderr and debugger instead during batch CLI processing
        _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
        _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
        _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
        _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
        _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
        _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
        // Suppress color as batchmode also implies post-processing grep, etc.
        no_color = true;
    }
#endif // _WIN32/WIN32

    // --tsvdir is required option
    if (!sarge.getFlag("tsvdir", s)) {
        std::cerr << COLOR_ERROR << "required -t/--tsvdir was not specified!" << COLOR_RESET;
        sarge.printHelp();
        pdf_io.shutdown();
        return -1;
    }
    if (!is_folder(s)) {
        std::cerr << COLOR_ERROR << "-t/--tsvdir \"" << s << "\" is not a valid folder!" << COLOR_RESET;
        sarge.printHelp();
        pdf_io.shutdown();
        return -1;
    }
    grammar_folder = fs::absolute(s);

    // --pdf can be a folder or a file
    s.clear();
    (void)sarge.getFlag("pdf", s);
    if (s.size() > 0)
        input_file = fs::absolute(s);

    // --out can be a folder or a file
    s.clear();
    (void)sarge.getFlag("out", s);
    if (s.size() > 0)
        save_path = fs::absolute(s);

    // Optional -f/--force <version>
    if (sarge.getFlag("force", s)) {
        if (!FindInVector(v_ArlPDFVersions, s) && (s != "exact")) {
            std::cerr << COLOR_ERROR << "-f/--force PDF version '" << s << "' is not valid! Needs to be '1.0', '1.1', ..., '1.7', '2.0' or 'exact'." << COLOR_RESET;
            sarge.printHelp();
            pdf_io.shutdown();
            return -1;
        }
        force_version = s;
    }

    // Optional -e/--extensions <extn1[,extn2]>
    if (sarge.getFlag("extensions", s)) {
        supported_extns = split(s, ',');
    }

    if (debug_mode) {
        std::cout << "Arlington TSV folder: " << grammar_folder.lexically_normal() << std::endl;
        std::cout << "Output file/folder:   " << save_path.lexically_normal() << std::endl;
        std::cout << "PDF file/folder:      " << input_file.lexically_normal() << std::endl;
        std::cout << "Colorized output      " << (no_color ? "off" : "on") << std::endl;
        std::cout << "Clobber mode:         " << (clobber ? "on" : "off") << std::endl;
        std::cout << "Brief mode:           " << (terse ? "on" : "off") << std::endl;

        if (supported_extns.size() > 0) {
            std::cout << supported_extns.size() << " Extensions enabled: ";
            for (size_t i = 0; i < supported_extns.size(); i++)
                std::cout << supported_extns[i] << ((i < (supported_extns.size() - 1)) ? ", " : "");
        }
        else
            std::cout << "Extensions enabled:   <none>";
        std::cout << std::endl;

        if (force_version.size() > 0) {
            std::cout << "Forced PDF version:   " << force_version << std::endl;
        }
        if (sarge.exists("validate")) {
            std::cout << "Validating Arlington PDF Model grammar." << std::endl;
        }
        if (sarge.exists("checkdva")) {
            (void)sarge.getFlag("checkdva", s);
            std::cout << "Adobe DVA FormalRep:  " << fs::absolute(s).lexically_normal() << std::endl;
        }
    }

    // Validate the Arlington PDF grammar itself?
    if (sarge.exists("validate")) {
        if (!save_path.empty()) {
            if (!is_folder(save_path)) {
                ofs.open(save_path, std::ofstream::out | std::ofstream::trunc);
            }
            else {
                save_path /= (no_color ? "arl-validate.txt" : "arl-validate.ansi");
                ofs.open(save_path, std::ofstream::out | std::ofstream::trunc);
            }
        }
        ValidateGrammarFolder(grammar_folder, debug_mode, (save_path.empty() ? std::cout : ofs));
        ofs.close();
        pdf_io.shutdown();
        return 0;
    }

    // Compare Adobe DVA vs Arlington PDF Model
    if (sarge.getFlag("checkdva", s)) {
        input_file = s;
        if (is_file(input_file) && fs::exists(input_file)) {
            if (!save_path.empty()) {
                if (!is_folder(save_path)) {
                    ofs.open(save_path, std::ofstream::out | std::ofstream::trunc);
                }
                else {
                    save_path /= (no_color ? "dva.txt" : "dva.ansi");
                    ofs.open(save_path, std::ofstream::out | std::ofstream::trunc);
                }
            }
            if (!ofs.is_open())
                std::cout << std::endl;
            CheckDVA(pdf_io, input_file.lexically_normal(), grammar_folder, (save_path.empty() ? std::cout : ofs), terse);
            ofs.close();
            pdf_io.shutdown();
            return 0;
        }
        else {
            std::cerr << COLOR_ERROR << "--checkdva argument '" << input_file << "' was not a valid PDF file!" << COLOR_RESET;
            pdf_io.shutdown();
            return -1;
        }
    }

    try {
        if (input_file.empty()) {
            std::cerr << COLOR_ERROR << "no PDF file or folder was specified via --pdf! Or missing --validate or --checkdva." << COLOR_RESET;
            pdf_io.shutdown();
            return -1;
        }

        // single PDF file or folder of files?
        if (is_folder(input_file)) {
            fs::path        rptfile;
            std::error_code ec;

            for (const auto& entry : fs::recursive_directory_iterator(input_file)) {
                try {
                    // To avoid file permission access errors, check filename extension first to skip over system files
                    if (iequals(entry.path().extension().string(), ".pdf") && entry.is_regular_file()) {
                        rptfile = save_path / entry.path().stem();
                        if (no_color)
                            rptfile.replace_extension(".txt");  // change .pdf to .txt for uncolorized output
                        else
                            rptfile.replace_extension(".ansi"); // change .pdf to .ansi if colorized output
                        if (!clobber) {
                            // if rptfile already exists then try a different filename by continuously appending underscores...
                            while (fs::exists(rptfile)) {
                                rptfile.replace_filename(rptfile.stem().string() + "_");
                                if (no_color)
                                    rptfile.replace_extension(".txt");  // change .pdf to .txt for uncolorized output
                                else
                                    rptfile.replace_extension(".ansi"); // change .pdf to .ansi if colorized output
                            }
                        }
                        std::cout << "Processing " << entry.path().lexically_normal() << " to " << rptfile.lexically_normal() << " ";
                        ofs.open(rptfile, std::ofstream::out | std::ofstream::trunc);
                        if (!process_single_pdf(entry.path().lexically_normal(), grammar_folder, pdf_io, ofs, terse, debug_mode, force_version, supported_extns)) {
                            std::cout << COLOR_ERROR_ANSI << "- FATAL ERROR!" << COLOR_RESET_ANSI;
                            retval = -1;
                        }
                        std::cout << std::endl;
                        ofs.close();
                    }
                }
                catch (const std::exception& e) {
                    retval = -1;
                    std::cerr << COLOR_ERROR << "EXCEPTION " << e.what() << COLOR_RESET;
                }
            } // for
            std::cout << "DONE" << std::endl;
        }
        else {
            // Just a single PDF file (doesn't have to be a regular file!) to try and process ...
            if (fs::exists(input_file)) {
                if (!save_path.empty()) {
                    if (is_folder(save_path)) {
                        save_path /= input_file.stem();
                        if (no_color)
                            save_path.replace_extension(".txt");  // change .pdf to .txt for uncolorized output
                        else
                            save_path.replace_extension(".ansi"); // change .pdf to .ansi if colorized output
                    }
                    if (!clobber) {
                        // if output file already exists then try a different filename by continuously appending underscores...
                        while (fs::exists(save_path)) {
                            save_path.replace_filename(save_path.stem().string() + "_");
                            if (no_color)
                                save_path.replace_extension(".txt");  // change .pdf to .txt for uncolorized output
                            else
                                save_path.replace_extension(".ansi"); // change .pdf to .ansi if colorized output
                        }
                    }
                    std::cout << "Processing " << input_file.lexically_normal() << " to " << save_path.lexically_normal() << std::endl;
                }
                ofs.open(save_path, std::ofstream::out | std::ofstream::trunc);
                input_file = input_file.lexically_normal();
                if (!process_single_pdf(input_file, grammar_folder, pdf_io, (save_path.empty() ? std::cout : ofs), terse, debug_mode, force_version, supported_extns))
                    retval = -1;
                ofs.close();
                std::cout << ((retval != 0) ? COLOR_ERROR_ANSI : "") << "DONE" << ((retval != 0) ? COLOR_RESET_ANSI : "") << std::endl;
            }
            else {
                retval = -1;
                std::cerr << COLOR_ERROR << "--pdf argument '" << input_file << "' was not a valid file!" << COLOR_RESET;
            }
        }
    }
    catch (const std::exception& e) {
        retval = -1;
        std::cerr << COLOR_ERROR << "EXCEPTION " << e.what() << COLOR_RESET;
    }

    if (ofs.is_open())
        ofs.close();
    pdf_io.shutdown();

    return retval;
}
