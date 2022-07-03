///////////////////////////////////////////////////////////////////////////////
/// @file
/// @brief Header for Arlington model validation and checking against Adobe DVA
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

#ifndef CheckGrammar_h
#define CheckGrammar_h
#pragma once

#include "ArlingtonPDFShim.h"

#include <iostream>
#include <string>
#include <filesystem>

/// @brief Validate the Arlington PDF model grammar
void ValidateGrammarFolder(const std::filesystem::path& grammar_folder, bool verbose, std::ostream& ofs);

/// @brief Check Adobe DVA vs Arlington PDF model
void CheckDVA(ArlingtonPDFShim::ArlingtonPDFSDK& pdfsdk, const std::filesystem::path& dva_file, const std::filesystem::path& grammar_folder, std::ostream& ofs, bool terse);

#endif // CheckGrammar_h