////////////////////////////////////////////////////////////////////////////////////////////////////
// Initialization.cpp
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
/*!
\page CPP_Samples C++ Samples
- \subpage Initialization_cpp
*/
/*!
\page Initialization_cpp Initialization Sample
Example how to initialize PDFix SDK in c++.
\snippet /Initialization.hpp Initialization_cpp
*/

#pragma once

//! [Initialization_cpp]
#include <string>
#include <iostream>
#include "Pdfix.h"

void Initialization() {
  // initialize Pdfix
  if (!PDFixSDK::Pdfix_init(Pdfix_MODULE_NAME))
    throw std::runtime_error("Pdfix initialization fail");

  PDFixSDK::Pdfix* pdfix = PDFixSDK::GetPdfix();
  if (!pdfix)
    throw std::runtime_error("GetPdfix fail");

  if (pdfix->GetVersionMajor() != PDFIX_VERSION_MAJOR || 
    pdfix->GetVersionMinor() != PDFIX_VERSION_MINOR ||
    pdfix->GetVersionPatch() != PDFIX_VERSION_PATCH)
    throw std::runtime_error("Incompatible version");

  pdfix->Destroy();
}
//! [Initialization_cpp]
