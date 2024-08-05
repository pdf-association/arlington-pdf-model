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

/// @brief Global flag to ignore wildcards `*` when processing PossibleValues field.
bool explicit_values_only = false;


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
/// @param[in] pwd         password
/// 
/// @returns true on success. false on a fatal error
bool process_single_pdf(
    const fs::path& pdf_file_name, 
    const fs::path& tsv_folder, 
    ArlingtonPDFSDK& pdfsdk, 
    std::ostream& ofs, 
    const bool terse, 
    const bool debug_mode, 
    const std::string& forced_ver, 
    std::vector<std::string>& extns,
    std::wstring& pwd)
{
    bool retval = true;
    try
    {
        ofs << "BEGIN - TestGrammar " << TestGrammar_VERSION << " " << pdfsdk.get_version_string() << std::endl;
        ofs << "Arlington TSV data: " << fs::absolute(tsv_folder).lexically_normal() << std::endl;
        ofs << "PDF: " << fs::absolute(pdf_file_name).lexically_normal() << std::endl;

        if (pdfsdk.open_pdf(pdf_file_name, pwd)) {
            int pre = preamble_offset_to_start_pdf(pdf_file_name);
            if (pre > 0) {
                ofs << COLOR_WARNING << pre << " preamble junk bytes detected before '%PDF-x.y' header" << COLOR_RESET;
            }
            else if (pre < 0) {
                ofs << COLOR_ERROR << "error detecting '%PDF-x.y' header - not present?" << COLOR_RESET;
            }

            int post = postamble_offset_to_end_pdf(pdf_file_name);
            if (post > 0) {
                ofs << COLOR_WARNING << post << " postamble junk bytes (non-whitespace) detected after '%%EOF'" << COLOR_RESET;
            }
            else if (post < 0) {
                ofs << COLOR_ERROR << "error detecting '%%EOF' - not present?" << COLOR_RESET;
            }

            CParsePDF parser(tsv_folder, ofs, terse, debug_mode, forced_ver);
            CPDFFile  pdf(pdf_file_name, pdfsdk, forced_ver, extns);
            std::string s;
            ArlPDFTrailer* t = pdfsdk.get_trailer();
            if (t != nullptr) {
                if (t->is_xrefstm()) {
                    ofs << COLOR_INFO << "XRefStream detected." << COLOR_RESET;
                    s = "Trailer (as XRefStream)";
                    parser.add_root_parse_object(t, "XRefStream", s);
                }
                else {
                    ofs << COLOR_INFO << "Traditional trailer dictionary detected." << COLOR_RESET;
                    s = "Trailer";
                    parser.add_root_parse_object(t, "FileTrailer", s);
                }

                /// @todo - add Linearization via add_root_parse_object() as another root object to parse from

                if (t->is_encrypted()) {
                    if (t->is_unsupported_encryption()) {
                        ofs << COLOR_INFO << "Unsupported encryption" << COLOR_RESET;
                    }
                    else {
                        ofs << COLOR_INFO << "Encrypted PDF" << COLOR_RESET;
                    }
                }

                retval = parser.parse_object(pdf);
                if (retval) {
                    ofs << COLOR_INFO << "Latest Arlington object was" << pdf.get_latest_feature_version_info() << " compared using" << (pdf.is_forced_version() ? " forced" : "") << " PDF " << pdf.pdf_version;
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
            pdfsdk.close_pdf();
        }
        else {
            ofs << COLOR_ERROR << "failed to open PDF" << COLOR_RESET;
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

#ifdef DO_DOXYGEN
/// @def CRT_MEMORY_LEAK_CHECK
/// @brief enables C RTL memory leak checking (slow!) under Microsoft Windows
#define CRT_MEMORY_LEAK_CHECK
#else
#define CRT_MEMORY_LEAK_CHECK
#endif

int wmain(int argc, wchar_t* argv[]) {
#if defined(_DEBUG) && defined(CRT_MEMORY_LEAK_CHECK)
    int tmp = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
    tmp = tmp | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF; // | _CRTDBG_CHECK_ALWAYS_DF; // _CRTDBG_CHECK_ALWAYS_DF is VERY slow!!
    _CrtSetDbgFlag(tmp);
    //_CrtSetBreakAlloc(6518);
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
    sarge.setUsage("TestGrammar --tsvdir <dir> [--force <ver>|exact] [--out <fname|dir>] [--no-color] [--clobber] [--debug] [--brief] [--extensions <extn1[,extn2]>] [--password <pwd>] [--exclude string | @textfile.txt] [--dryrun] [--allfiles] [--validate | --checkdva <formalrep> | --pdf <fname|dir|@file.txt> ]");
    sarge.setArgument("h", "help", "This usage message.", false);
    sarge.setArgument("b", "brief", "terse output when checking PDFs. The full PDF DOM tree is NOT output.", false);
    sarge.setArgument("c", "checkdva", "Adobe DVA formal-rep PDF file to compare against Arlington PDF model.", true);
    sarge.setArgument("d", "debug", "output additional debugging information.", false);
    sarge.setArgument("",  "clobber", "always overwrite PDF output report files (--pdf) rather than append underscores.", false);
    sarge.setArgument("",  "no-color", "disable colorized text output (useful when redirecting or piping output)", false);
    sarge.setArgument("m", "batchmode", "stop popup error dialog windows and redirect everything to console (Windows only, includes memory leak reports).", false);
    sarge.setArgument("o", "out", "output file or folder. Default is stdout. See --clobber for overwriting behavior.", true);
    sarge.setArgument("p", "pdf", "input PDF file, folder, or text file of PDF files/folders.", true);
    sarge.setArgument("f", "force", "force the PDF version to the specified value (1,0, 1.1, ..., 2.0 or 'exact'). Only applicable to --pdf.", true);
    sarge.setArgument("t", "tsvdir", "[required] folder containing Arlington PDF model TSV file set.", true);
    sarge.setArgument("v", "validate", "validate the Arlington PDF model.", false);
    sarge.setArgument("e", "extensions", "a comma-separated list of extensions, or '*' for all extensions.", true);
    sarge.setArgument("",  "password", "password. Only applicable to --pdf.", true);
    sarge.setArgument("",  "exclude", "PDF exclusion string or filelist (# is a comment). Only applicable to --pdf.", true);
    sarge.setArgument("",  "dryrun", "Dry run - don't do any actual processing.", false);
    sarge.setArgument("a", "allfiles", "Process all files regardless of file extension.", false);
    sarge.setArgument("",  "explicit-values-only", "Ignore wildcards in PossibleValues.", false);

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
        std::cout << COLOR_RESET;
        sarge.printHelp();
        std::cout << "\nBuilt using " << pdf_io.get_version_string() << std::endl;
        pdf_io.shutdown();
        return 0;
    }

    int             retval = 0;         // final return code to O/S
    std::string     s;                  // temp variable
    fs::path        grammar_folder;     // folder with TSV files (required and must exist)
    fs::path        save_path;          // output file or folder. Optional. Default is "." or to stdout
    bool            save_file_is_folder = false; // true iff save_path is an existing folder
    bool            save_file_is_file   = false; // true iff save_path is a file in an existing folder (will not mkdir!)
    fs::path        input_filename;     // --pdf @filename.txt
    bool            input_is_a_file = false; // --pdf
    std::vector<fs::path> input_list;   // --pdf files and folder list
    std::ofstream   ofs;                // output filestream
    std::string     force_version;      // Optional forced PDF version
    std::wstring    pdf_password;       // Optional password
    bool            clobber = sarge.exists("clobber");
    bool            debug_mode = sarge.exists("debug");
    bool            terse = sarge.exists("brief");
    bool            dryrun = sarge.exists("dryrun");
    bool            all_files = sarge.exists("allfiles");
    std::vector<std::string> supported_extns;       // --extensions
    bool            exclude_as_string = false;      // --exclude
    fs::path        exclusion_filename;             // --exclude
    std::vector<std::string> exclusions;            // --exclude
    unsigned int    count = 0;                      // number of files processed


    // Set globals (yuck, but very convenient)
    no_color = sarge.exists("no-color");
    explicit_values_only = sarge.exists("explicit-values-only");

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
    grammar_folder = fs::absolute(s).lexically_normal();

    // --out can be a folder or a file
    s.clear();
    (void)sarge.getFlag("out", s);
    if (s.size() > 0) {
        save_path = fs::absolute(s).lexically_normal();
        save_file_is_folder = is_folder(save_path);
        save_file_is_file   = is_file(save_path);
        if (!save_file_is_folder && !save_file_is_file) {
            auto p = save_path.parent_path();
            if (!is_folder(p)) {
                std::cerr << COLOR_ERROR << "-o/--out \"" << p << "\" is not a valid folder!" << COLOR_RESET;
                sarge.printHelp();
                pdf_io.shutdown();
                return -1;
            }
            auto e = save_path.extension();
            save_file_is_file = true;  // new file in existing folder
        }
    }

    // --pdf can be a folder, or a single PDF file, or "@file.txt"
    s.clear();
    (void)sarge.getFlag("pdf", s);
    if (s.size() > 0) {
        if (s[0] != '@') {
            // either file or folder.
            input_list.push_back(fs::absolute(s));
            try {
                input_is_a_file = fs::is_regular_file(input_list.at(0));
            }
            catch (...) {
                input_is_a_file = false; // assume a folder
            }
        }
        else {
            // File list. Treat arg as a filename (remove leading '@')
            input_filename = s.substr(1, s.size() - 1);
            input_filename = fs::absolute(input_filename).lexically_normal();

            std::ifstream input_filelist(input_filename);
            while (std::getline(input_filelist, s)) {
                s = trim(s);
                if ((s.size() > 0) && (s[0] != '#'))
                    input_list.push_back(fs::absolute(s));
            }
            input_filelist.close();

            if (input_list.size() == 0)
                std::cerr << COLOR_ERROR << "--pdf '" << input_filename << "' was invalid or empty!" << COLOR_RESET;
        }
    }

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

    // Optional --password <pwd>
    if (sarge.getFlag("password", s)) {
        pdf_password = ToWString(s);
    }

    //Optional --exclude <string> | @filelist.txt
    if (sarge.getFlag("exclude", s)) 
        if (s.size() > 0)
        {
            if (s[0] == '@') {
                // treat as a filename (remove leading '@')
                exclusion_filename = s.substr(1, s.size() - 1);
                exclusion_filename = fs::absolute(exclusion_filename).lexically_normal();
                exclude_as_string = false;

                std::ifstream exclusion_file(exclusion_filename);
                while (std::getline(exclusion_file, s)) {
                    s = trim(s);
                    if ((s.size() > 0) && (s[0] != '#'))
                        exclusions.push_back(s);
                }
                exclusion_file.close();

                if (exclusions.size() == 0)
                    std::cerr << COLOR_ERROR << "--exclude '" << exclusion_filename << "' was invalid or empty!" << COLOR_RESET;
            }
            else {
                // treat as a string
                exclusions.push_back(s);
                exclude_as_string = true;
            }
        }

    // Dump all the processed command line options to screen (stdout)
    if (debug_mode) {
        std::cout << COLOR_RESET_NO_EOL;
        std::cout << "TestGrammar version:  " << TestGrammar_VERSION << std::endl;
        std::cout << "PDF SDK:              " << pdf_io.get_version_string() << std::endl;
        std::cout << "Arlington TSV folder: " << grammar_folder << std::endl;
        if (save_path.empty())
            std::cout << "Output:               stdout" << std::endl;
        else
            std::cout << "Output " << (save_file_is_file ? "file:          " : "folder:        ") << save_path << std::endl;
        if (input_list.size() == 0)
            std::cout << "PDF file/folder:      <none>"<< std::endl;
        else if (input_list.size() == 1) {
            if (input_is_a_file)
                std::cout << "PDF file:  ";
            else
                std::cout << "PDF folder:";
            std::cout << "           " << input_list.at(0).lexically_normal() << std::endl;
        }
        else 
            std::cout << "PDF file list:        " << input_filename << " (" << input_list.size() << " lines)" << std::endl;
        std::cout << "Colorized output:     " << (no_color ? "off" : "on") << std::endl;
        std::cout << "Explicit values only: " << (explicit_values_only ? "yes" : "no") << std::endl;
        std::cout << "Clobber mode:         " << (clobber ? "on" : "off") << std::endl;
        std::cout << "Dry run:              " << (dryrun ? "on" : "off") << std::endl;
        std::cout << "All files:            " << (all_files ? "on (*.* wildcard)" : "off  (*.pdf only)") << std::endl;
        std::cout << "Brief mode:           " << (terse ? "on" : "off") << std::endl;
        if (pdf_password.size() == 0)
            std::cout << "Password:             <none>" << std::endl;
        else
            std::cout << "Password:             '" << ToUtf8(pdf_password) << "'" << std::endl;
        if (exclusions.size() > 0) {
            if (exclude_as_string)
                std::cout << "Exclusion string:     '" << exclusions.at(0) << "'" << std::endl;
            else
                std::cout << "Exclusion filename:   " << exclusion_filename << " (" << exclusions.size() << " lines)" << std::endl;
        }
        else
            std::cout << "Exclusions enabled:   <none>" << std::endl;

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

        std::cout << std::endl;
    }

    // Validate the Arlington PDF grammar itself?
    if (sarge.exists("validate")) {
        if (!save_path.empty()) {
            if (save_file_is_file) {
                ofs.open(save_path, std::ofstream::out | std::ofstream::trunc);
            }
            else {
                save_path /= (no_color ? "arl-validate.txt" : "arl-validate.ansi");
                ofs.open(save_path, std::ofstream::out | std::ofstream::trunc);
            }
        }
        count++;
        if (!dryrun)
            ValidateGrammarFolder(grammar_folder, debug_mode, (save_path.empty() ? std::cout : ofs));
        ofs.close();
        pdf_io.shutdown();
        return 0;
    }

    // Compare Adobe DVA FormalRep vs Arlington PDF Model
    if (sarge.getFlag("checkdva", s)) {
        input_list.push_back(fs::absolute(s));
        if (is_file(input_list.at(0)) && fs::exists(input_list.at(0))) {
            if (!save_path.empty()) {
                if (save_file_is_file) {
                    ofs.open(save_path, std::ofstream::out | std::ofstream::trunc);
                }
                else {
                    save_path /= (no_color ? "dva.txt" : "dva.ansi");
                    ofs.open(save_path, std::ofstream::out | std::ofstream::trunc);
                }
            }
            if (!ofs.is_open())
                std::cout << std::endl;
            count++;
            if (!dryrun)
                CheckDVA(pdf_io, input_list.at(0).lexically_normal(), grammar_folder, (save_path.empty() ? std::cout : ofs), terse);
            ofs.close();
            pdf_io.shutdown();
            return 0;
        }
        else {
            std::cerr << COLOR_ERROR << "--checkdva argument '" << s << "' was not a valid PDF file!" << COLOR_RESET;
            pdf_io.shutdown();
            return -1;
        }
    }

    if (input_list.size() == 0) {
        std::cerr << COLOR_ERROR << "no PDF file, folder, or file list was specified via --pdf! Or missing --validate or --checkdva." << COLOR_RESET;
        pdf_io.shutdown();
        return -1;
    }

    try {
        for (auto& input_file : input_list) {
            fs::recursive_directory_iterator dir_iter;
            fs::directory_entry              entry;
            bool is_folder = false;
            if (fs::is_directory(input_file)) {
                try {
                    // Attempt first as a folder - will throw a C++ exception if invalid
                    dir_iter = fs::recursive_directory_iterator(input_file, fs::directory_options::skip_permission_denied);
                    entry = *dir_iter++;
                    is_folder = true;
                }
                catch (...) {
                    // had an error as a folder name, try anyway as a normal file
                    entry = fs::directory_entry(input_file);
                    is_folder = false;
                }
            }
            else {
                entry = fs::directory_entry(input_file);
                is_folder = false;
            }

            try {
                do {
                    if (entry.is_regular_file() && (all_files || iequals(entry.path().extension().string(), ".pdf"))) {
                        fs::path  rptfile;
                        if (!save_path.empty()) {
                            if (save_file_is_folder) {
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
                                rptfile = fs::absolute(rptfile).lexically_normal();
                            }
                            else { // save_file_is_file
                                rptfile = fs::absolute(save_path).lexically_normal();
                            }
                        }

                        bool exclude_for_processing = false;
                        for (auto& excl : exclusions) {
                            s = entry.path().lexically_normal().string();
                            if (s.find(excl) != std::string::npos) {
                                exclude_for_processing = true;
                            }
#if defined(_WIN32) || defined(WIN32)
                            // Microsoft Windows path separator is a BACKSLASH which is very annoying in C++!
                            s = std::regex_replace(s, std::regex("\\\\"), "/");
                            if (s.find(excl) != std::string::npos) {
                                exclude_for_processing = true;
                            }
#endif // _WIN32/WIN32
                            else {
                                try {
                                    if (std::regex_match(s, std::regex(excl))) {
                                        exclude_for_processing = true;
                                    }
                                }
                                catch (...) { // const std::regex_error& e 
                                    // ignore all exceptions from regex creation and process PDF file
                                }
                            }
                        } // for

                        if (!exclude_for_processing) {
                            std::cout << "Processing " << entry.path().lexically_normal() << " to ";
                            if (rptfile.empty())
                                std::cout << "stdout ";
                            else {
                                std::cout << rptfile << ((is_folder && !clobber) ? " (appended) " : " ");
                                ofs.open(rptfile, std::ofstream::out | ((is_folder && !clobber) ? std::ofstream::app : std::ofstream::trunc));
                            }
                            count++;
                            if (!dryrun)
                                if (!process_single_pdf(entry.path().lexically_normal(), grammar_folder, pdf_io, (rptfile.empty() ? std::cout : ofs) , terse, debug_mode, force_version, supported_extns, pdf_password)) {
                                    std::cout << COLOR_ERROR << "- FATAL ERROR!" << COLOR_RESET_NO_EOL;
                                    retval = -1;
                                }
                            if (!rptfile.empty())
                                ofs.close();
                        }
                        else {
                            std::cout << COLOR_INFO << "Excluded " << entry.path().lexically_normal() << COLOR_RESET_NO_EOL;
                        }
                        std::cout << std::endl;
                    }
                    else if (!entry.exists()) {
                        std::cout << COLOR_ERROR << "Invalid PDF file/folder " << entry.path().lexically_normal() << COLOR_RESET;
                    }

                    if (is_folder) {
                        // Recursively processing a folder so get next file
                        entry = *dir_iter++;
                    }
                } while (dir_iter != fs::end(dir_iter));
            }
            catch (const std::exception& e) {
                retval = -1;
                std::cerr << std::endl << COLOR_ERROR << "EXCEPTION " << e.what() << COLOR_RESET;
            }
        }
        std::cout << "DONE - " << count << " files processed" << std::endl;
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
