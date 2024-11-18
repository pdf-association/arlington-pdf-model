///////////////////////////////////////////////////////////////////////////////
/// @file 
/// @brief Arlington predicate processor
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

#ifndef PredicateProcessor_h
#define PredicateProcessor_h
#pragma once

#include "ArlingtonTSVGrammarFile.h"
#include "ArlingtonPDFShim.h"
#include "ArlVersion.h"
#include "ASTNode.h"
#include "PDFFile.h"

#include <iostream>
#include <string>
#include <vector>

using namespace ArlingtonPDFShim;

/// @brief "IndirectReference" column has 3 possible outcomes
enum class ReferenceType { MustBeDirect, MustBeIndirect, DontCare };

class PredicateProcessor {
protected:
    /// @brief the PDF file class object
    CPDFFile*               pdfc;

    /// @brief Data from an Arlington TSV grammar file
    const ArlTSVmatrix&     tsv;

    /// @brief A vector of vector of predicate ASTs, as Arlington fields may be of form: 
    /// [fn:A(...),fn:B(...),fn:C(...)];[];[fn:X(...),fn:Y(...),fn:Z(...)]
    /// - outer vector: supports each Arlington type (e.g. [A,B,C];[];['X','Y','Z'])
    /// - inner vector: supports predicates around each COMMA-separated values 
    ///   for each type (e.g. the A,B,C; nullptr and 'X','Y','Z' above)
    /// Note that PDF arrays also use '[' and ']'. PDF strings use '\''
    /// This is a class data mainly for debugging purposes.
    ASTNodeMatrix           predicate_ast;

    /// @brief returns true if object contains a valid value in pvalues w.r.t. to the TSV data indexed by key_idx
    bool IsValidValue(ArlPDFObject* object, const int key_idx, const std::string& pvalues);

    /// @brief Recursively delete the AST and clear the predicate AST 
    void EmptyPredicateAST();

public:
    PredicateProcessor(CPDFFile* pdfo, const ArlTSVmatrix &tsv_data) :
        pdfc(pdfo), tsv(tsv_data)
        { /* constructor */ };

    ~PredicateProcessor() { EmptyPredicateAST(); };

    bool WasFullyImplemented()    { return pdfc->PredicateWasFullyProcessed(); };
    bool SomethingWasDeprecated() { return pdfc->PredicateWasDeprecated(); };

    bool ValidateKeySyntax(const int key_idx);
    
    bool ValidateTypeSyntax(const int key_idx);
    static std::string ReduceTypeElement(const std::string &t);
    
    bool ValidateSinceVersionSyntax(const int key_idx);
    bool IsValidForPDFVersion(ArlPDFObject* container, ArlPDFObject* obj, const int key_idx);
    
    bool ValidateDeprecatedInSyntax(const int key_idx);
    bool IsDeprecated(const int key_idx);

    bool ValidateRequiredSyntax(const int key_idx);
    bool IsRequired(ArlPDFObject* container, ArlPDFObject* obj, const int key_idx, const int type_idx);

    bool ValidateIndirectRefSyntax(const int key_idx);
    ReferenceType ReduceIndirectRefRow(ArlPDFObject* container, ArlPDFObject* object, const int key_idx, const int type_index);

    bool ValidateInheritableSyntax(const int key_idx);
    bool IsInheritable(const int key_idx);

    bool ValidateDefaultValueSyntax(const int key_idx);
    ASTNode* GetDefaultValue(const int key_idx, const int type_idx);

    bool ValidatePossibleValuesSyntax(const int key_idx);
    bool ReducePVRow(ArlPDFObject* container, ArlPDFObject* object, const int key_idx, const int type_idx);

    bool ValidateSpecialCaseSyntax(const int key_idx);
    bool ReduceSCRow(ArlPDFObject* container, ArlPDFObject* object, const int key_idx, const int type_idx);

    bool ValidateLinksSyntax(const int key_idx);
};

#endif // PredicateProcessor_h
