///////////////////////////////////////////////////////////////////////////////
// CheckGrammar.h
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
// Contributors: Roman Toda, Frantisek Forgac, Normex. Peter Wyatt, PDF Association
//
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include "ArlingtonPDFShim.h"

#include <iostream>
#include <string>
#include <filesystem>

// Validate the Arlington PDF model grammar
void ValidateGrammarFolder(const std::filesystem::path& grammar_folder, std::ostream& ofs);

// Check Adobe DVA vs Arlington PDF model
void CheckDVA(ArlingtonPDFShim::ArlingtonPDFSDK& pdfsdk, const std::filesystem::path& dva_file, const std::filesystem::path& grammar_folder, std::ostream& ofs);
