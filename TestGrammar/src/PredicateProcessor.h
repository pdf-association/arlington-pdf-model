///////////////////////////////////////////////////////////////////////////////
// PredicateProcessor.cpp
// Copyright 2021 PDF Association, Inc. https://www.pdfa.org
//
// This material is based upon work supported by the Defense Advanced
// Research Projects Agency (DARPA) under Contract No. HR001119C0079.
// Any opinions, findings and conclusions or recommendations expressed
// in this material are those of the author(s) and do not necessarily
// reflect the views of the Defense Advanced Research Projects Agency
// (DARPA). Approved for public release.
//
// SPDX-License-Identifier: Apache-2.0
// Contributors: Peter Wyatt, PDF Association
// 
///////////////////////////////////////////////////////////////////////////////

#pragma once
#include "ArlingtonTSVGrammarFile.h"
#include "ArlingtonPDFShim.h"

using namespace ArlingtonPDFShim;

class PredicateProcessor {
protected:
    std::string     tsv_field;
public:
    PredicateProcessor(std::string s) : 
        tsv_field(s)
        { /* constructor */ };
    virtual bool ValidateSyntax() = 0;
};


/// @brief Implements predicate support for the Arlington "Type" field (column 2) 
/// fn:SinceVersion(x.y,type)
/// fn:Deprecated(x.y,type)
/// fn:BeforeVersion(x.y,type)
/// fn:IsPDFVersion(x.y,type)
class TypePredicateProcessor : public PredicateProcessor {
public:
    TypePredicateProcessor(std::string s) : 
        PredicateProcessor(s)
        { /* constructor */ };
    virtual bool ValidateSyntax();
    std::string Process(const std::string pdf_version);
};


/// @brief Implements predicate support for the Arlington "Required" field (column 5) 
/// - never [];[];[]
/// - fn:IsRequired(...) is always the outer predicate.
/// Inner predicates include the following (incl. && and || multiple expressions):
/// - fn:NotPresent(key), fn:IsPresent(key)
/// - fn:SinceVersion(x.y)
/// - @key==value, @key!=value 
class RequiredPredicateProcessor : public PredicateProcessor {

};


/// @brief Implements predicate support for the Arlington "IndirectReference" field (column 6) 
/// - [];[];[]
/// - fn:MustBeDirect()
/// - fn:MustBeDirect(fn:IsPresent(Encrypt))
class IndirectRefPredicateProcessor : public PredicateProcessor {
public:
    IndirectRefPredicateProcessor(std::string s) :
        PredicateProcessor(s)
        { /* constructor */ };
    virtual bool ValidateSyntax();
    bool Process(ArlPDFObject* obj);
};
