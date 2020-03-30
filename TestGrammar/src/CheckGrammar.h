////////////////////////////////////////////////////////////////////////////////////////////////////
// CheckGrammar.h
// Copyright (c) 2020 Normex, Pdfix. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>
#include "Pdfix.h"
#include "utils.h"

#ifdef GetObject
#undef GetObject
#endif

void CheckGrammar(std::string& grammar_folder, std::ofstream& ofs);
void CompareWithAdobe(std::string& adobe_file, std::string& grammar_folder, std::ofstream& ofs);
