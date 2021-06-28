///////////////////////////////////////////////////////////////////////////////
// Main.cpp
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
// Contributors: Roman Toda, Frantisek Forgac, Normex
///////////////////////////////////////////////////////////////////////////////

#define _CRT_SECURE_NO_WARNINGS

#include <exception>
#include <iostream>

#if defined __linux__
#include <cstring>
#endif

#include "ArlingtonPDFShim.h"
#include "ParseObjects.h"
#include "CheckGrammar.h"
#include "TestGrammarVers.h"
#include "Sarge/src/sarge.h"

using namespace ArlingtonPDFShim;
namespace fs = std::filesystem;

/// @brief Processes a single PDF file against the Arlington PDF model
/// @param pdf_file_name[in]  PDF filename for processing
/// @param tsv_folder[in] the folder with the Arlington TSV model files
/// @param ofs[in]   open stream to write output to
void process_single_pdf(const fs::path& pdf_file_name, const fs::path& tsv_folder, ArlingtonPDFSDK& pdfsdk, std::ostream& ofs)
{
    try
    {
        ofs << "BEGIN - TestGrammar " << TestGrammar_VERSION << " " << pdfsdk.get_version_string() << std::endl;
        ofs << "Arlington TSV data: " << fs::absolute(tsv_folder) << std::endl;
        ofs << "PDF: " << fs::absolute(pdf_file_name) << std::endl;

        ArlPDFTrailer* trailer = pdfsdk.get_trailer(pdf_file_name.wstring());
        if (trailer != nullptr) {
            CParsePDF parser(tsv_folder, ofs);
            if (trailer->get_xrefstm()) {
                ofs << "XRefStream detected" << std::endl;
                parser.add_parse_object(trailer, "XRefStream", "Trailer");
            } else {
                ofs << "Trailer detected" << std::endl;
                parser.add_parse_object(trailer, "FileTrailer", "Trailer");
            }
            parser.parse_object();
        }
        else
        {
            ofs << "Error: failed to acquire Trailer in: '" << pdf_file_name.string() << "'" << std::endl;
        }
    }
    catch (std::exception& ex) {
        ofs << "Error: EXCEPTION: " << ex.what() << std::endl;
    }

    // Finally...
    ofs << "END" << std::endl;
};



#if defined(_WIN32) || defined(WIN32)
int wmain(int argc, wchar_t* argv[]) {
    // Convert wchar_t* to char* for command line processing
    mbstate_t   state;
    char        **mbcsargv = new char*[argc];

    for (int i = 0; i < argc; i++) {
        memset(&state, 0, sizeof state);
        size_t len = 1 + wcsrtombs(NULL, (const wchar_t**)&argv[i], 0, &state);
        char *mbstr = new char[len];
        wcsrtombs(mbstr, (const wchar_t**)&argv[i], len, &state);
#if 0
        printf("Arg %d: Multibyte string: '%s'\n", i, mbstr);
        printf("Arg %d: Length, including '\\0': %zu\n", i, len);
#endif
        mbcsargv[i] = mbstr;
    }

    // Simplistic attempt at support for folder and filenames from non-std code pages...
    setlocale(LC_CTYPE, "en_US.UTF-8");
#else
int main(int argc, char* argv[]) {
#endif
    ArlingtonPDFSDK pdf_io;             // PDF SDK
    Sarge           sarge;              // Command line option processing

    sarge.setDescription("Arlington PDF Model C++ P.o.C. version " TestGrammar_VERSION
                         "\nChoose one of: --pdf, --checkdva or --validate.");
    sarge.setUsage("TestGrammar --tsvdir <dir> [--out <fname|dir>] [--debug] [--validate | --checkdva <formalrep> | --pdf <fname|dir> ]");
    sarge.setArgument("h", "help",     "This usage message.", false);
    sarge.setArgument("t", "tsvdir",   "[required] folder containing Arlington PDF model TSV file set.", true);
    sarge.setArgument("o", "out",      "output file or folder. Default is stdout. Existing files will be overwritten.", true);
    sarge.setArgument("p", "pdf",      "input PDF file or folder.", true);
    sarge.setArgument("c", "checkdva", "Adobe DVA formal-rep PDF file to compare against Arlington PDF model.", true);
    sarge.setArgument("v", "validate", "validate the Arlington PDF model.", false);
    sarge.setArgument("d", "debug",    "output additional debugging information (verbose!)", false);

#if defined(_WIN32) || defined(WIN32)
    if (!sarge.parseArguments(argc, mbcsargv)) {
#else
    if (!sarge.parseArguments(argc, argv)) {
#endif
        std::cerr << "ERROR: error parsing command line arguments" << std::endl;
        sarge.printHelp();
        pdf_io.shutdown();
        return -1;
    }

    // Start up the PDF SDK - this may throw exceptions!
    pdf_io.initialize(sarge.exists("debug"));

    if (sarge.exists("help") || (argc == 1)) {
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
    bool            debug_mode = sarge.exists("debug");

    // --tsvdir is required option
    if (!sarge.getFlag("tsvdir", s)) {
        std::cerr << "ERROR: required -t/--tsvdir was not specified!" << std::endl;
        sarge.printHelp();
        pdf_io.shutdown();
        return -1;
    }
    if (!is_folder(s)) {
        std::cerr << "ERROR: -t/--tsvdir \"" << s << "\" is not a valid folder!" << std::endl;
        sarge.printHelp();
        pdf_io.shutdown();
        return -1;
    }
    grammar_folder = fs::absolute(s);

    // --pdf can be a folder or a file
    s.clear();
    (void)sarge.getFlag("pdf", s);
    input_file = fs::absolute(s);

    // --out can be a folder or a file
    s.clear();
    (void)sarge.getFlag("out", s);
    save_path = fs::absolute(s);

    if (debug_mode) {
        std::cout << "Arlington TSV folder: " << grammar_folder << std::endl;
        std::cout << "Output file/folder:   " << save_path << std::endl;
        std::cout << "PDF file/folder       " << input_file << std::endl;
        if (sarge.exists("validate")) {
            std::cout << "Validating Arlington PDF Model grammar." << std::endl;
        }
        if (sarge.exists("checkdva")) {
            (void)sarge.getFlag("checkdva", s);
            std::cout << "Adobe DVA FormalRep:  " << fs::absolute(s) << std::endl;
        }
    }

    // Validate the Arlington PDF grammar itself?
    if (sarge.exists("validate")) {
        if (!save_path.empty()) {
            if (!is_folder(save_path)) {
                ofs.open(save_path, std::ofstream::out | std::ofstream::trunc);
            }
            else {
                save_path /= "arl-validate.txt";
                ofs.open(save_path, std::ofstream::out | std::ofstream::trunc);
            }
        }
        if (!ofs.is_open())
            std::cout << std::endl;
        ValidateGrammarFolder(grammar_folder, (save_path.empty() ? std::cout : ofs));
        ofs.close();
        pdf_io.shutdown();
        return 0;
    }

    // Compare Adobe DVA vs Arlington PDF Model
    if (sarge.getFlag("checkdva", s))
    {
        input_file = s;
        if (is_file(input_file) && fs::exists(input_file)) {
            if (!save_path.empty()) {
                if (!is_folder(save_path)) {
                    ofs.open(save_path, std::ofstream::out | std::ofstream::trunc);
                }
                else {
                    save_path /= "dva-arl-check.txt";
                    ofs.open(save_path, std::ofstream::out | std::ofstream::trunc);
                }
            }
            if (!ofs.is_open())
                std::cout << std::endl;
            CheckDVA(input_file, grammar_folder, (save_path.empty() ? std::cout : ofs));
            ofs.close();
            pdf_io.shutdown();
            return 0;
        }
        else {
            std::cerr << "ERROR: --checkdva argument was not a valid PDF file!" << std::endl;
            pdf_io.shutdown();
            return -1;
        }
    }

    if (input_file.empty()) {
        std::cerr << "ERROR: no PDF file or folder was specified!" << std::endl;
        pdf_io.shutdown();
        return -1;
    }

    // single PDF file or folder of files?
    if (is_folder(input_file))
    {
        fs::path   rptfile;

        for (const auto& entry : fs::recursive_directory_iterator(input_file))
        {
            if (entry.is_regular_file() && entry.path().extension().wstring() == L".pdf")
            {
                rptfile = save_path / entry.path().stem();
                rptfile.replace_extension(".txt"); // change .pdf to .txt
                // if rptfile already exists then try a different filename by continuously appending underscores...
                while (fs::exists(rptfile))
                {
                    rptfile.replace_filename(rptfile.stem().string() + "_");
                    rptfile.replace_extension(".txt");
                }
                ofs.open(rptfile, std::ofstream::out | std::ofstream::trunc);
                std::cout << "Processing " << entry.path() << " to " << rptfile << std::endl;
                process_single_pdf(entry.path(), grammar_folder, pdf_io, ofs);
                ofs.close();
            }
        }
    }
    else {
        // Just a single PDF file to evaluate...
        if (fs::exists(input_file)) {
            if (!save_path.empty()) {
                if (!is_folder(save_path)) {
                    ofs.open(save_path, std::ofstream::out | std::ofstream::trunc);
                }
                else {
                    save_path /= "report.txt";
                    ofs.open(save_path, std::ofstream::out | std::ofstream::trunc);
                }
            }
            process_single_pdf(input_file, grammar_folder, pdf_io, (save_path.empty() ? std::cout : ofs));
            ofs.close();
        }
        else {
            std::cerr << "ERROR: --pdf argument was not a valid file!" << std::endl;
            retval = -1;
        }
    }

    if (ofs.is_open())
        ofs.close();
    pdf_io.shutdown();
    return retval;
}
