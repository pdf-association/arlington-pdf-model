////////////////////////////////////////////////////////////////////////////////////////////////////
// CheckGrammar.h
// 2020 Roman Toda, Normex
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
