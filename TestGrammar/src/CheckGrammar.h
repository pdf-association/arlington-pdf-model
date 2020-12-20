////////////////////////////////////////////////////////////////////////////////////////////////////
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
// Contributors: Roman Toda, Frantisek Forgac, Normex
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>
#include "Pdfix.h"
#include "utils.h"

void CheckGrammarFolder(std::string& grammar_folder, std::ofstream& ofs);
void CheckDVA(std::wstring& dva_file, std::string& grammar_folder, std::ofstream& ofs);
