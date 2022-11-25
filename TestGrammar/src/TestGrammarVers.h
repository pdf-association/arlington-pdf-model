///////////////////////////////////////////////////////////////////////////////
/// @file
/// @brief TestGrammar version, platform and build details
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
/// @author Peter Wyatt, PDF Association
///
///////////////////////////////////////////////////////////////////////////////

#ifndef TestGrammarVers_h
#define TestGrammarVers_h
#pragma once

// Compiler
#if defined(_MSC_VER)
#   define TG_COMPILER "MSC"
#elif defined(__llvm__)
#   define TG_COMPILER "llvm"
#elif defined(__GNUC__)
#   define TG_COMPILER "GNU-C"
#else
#   define TG_COMPILER "<other compiler>"
#endif

// Platform
#if defined(_WIN64)
#   define TG_PLATFORM "x64"
#elif defined(_WIN32)
#   define TG_PLATFORM "x86"
#elif defined(__linux__)
#   define TG_PLATFORM "linux"
#elif defined(__APPLE__)
#   define TG_PLATFORM "mac"
#else
#   define TG_PLATFORM "<unknown platform>"
#endif

// Debug vs release
#if defined(DEBUG) || defined(_DEBUG)
#   define TG_CONFIG "debug"
#else
#   define TG_CONFIG "release"
#endif

/// @brief String showing version, build date/time, compiler, platform and configuration
#define TestGrammar_VERSION "v0.81 built " __DATE__ " " __TIME__ " (" TG_COMPILER " " TG_PLATFORM " " TG_CONFIG ")"

#endif // TestGrammarVers_h
