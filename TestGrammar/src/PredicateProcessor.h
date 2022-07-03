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
#include "ASTNode.h"
#include "PDFFile.h"

#include <iostream>
#include <string>
#include <vector>

using namespace ArlingtonPDFShim;

/// @brief Left-to-right recursive descent parser, based on regex pattern matching
std::string LRParsePredicate(std::string s, ASTNode *root);

class PredicateProcessor {
protected:
    /// @brief the PDF file class object
    CPDFFile*               pdfc;

    /// @brief Single field from a row in an Arlington TSV grammar file
    std::string             tsv_field;

    /// @brief A vector of vector of predicate ASTs, as Arlington fields may be of form: 
    /// [fn:A(...),fn:B(...),fn:C(...)];[];[fn:X(...),fn:Y(...),fn:Z(...)]
    /// - outer vector: supports each Arlington type (e.g. [A,B,C];[];[X,Y,Z])
    /// - inner vector: supports predicates around each COMMA-separated values 
    ///   for each type (e.g. the A,B,C; nullptr and X,Y,Z above)
    ASTNodeMatrix           predicate_ast;

public:
    PredicateProcessor(CPDFFile* pdfo, std::string &s) :
        pdfc(pdfo), tsv_field(s)
        { /* constructor */ };

    ~PredicateProcessor();

    /// @brief ValidateRowSyntax() returns true if the Arlington content is valid
    virtual bool ValidateRowSyntax() = 0;

    /// @brief ReduceRow(...)
};


/// @brief Implements predicate support for the Arlington "Key" field (column 1)
/// - No COMMAs or SEMI-COLONs
/// - any alphanumeric or "." or "-" or "_"
/// - any integer (assumed to be an array index)
/// - wildcard "*" - must always be last row
/// - all rows are integer + "*" then indicates a repeating sets of N array elements
class KeyPredicateProcessor : public PredicateProcessor {
public:
    KeyPredicateProcessor(CPDFFile* pdfo, std::string s) :
        PredicateProcessor(pdfo, s)
        { /* constructor */ };
    virtual bool ValidateRowSyntax();
    // ReduceRow(...) is not required 
};


/// @brief Implements predicate support for the Arlington "Type" field (column 2)
/// - SEMI-COLON separated, alphabetically sorted, but no [ ] brackets
/// - fn:SinceVersion(x.y,type)
/// - fn:Deprecated(x.y,type)
/// - fn:BeforeVersion(x.y,type)
/// - fn:IsPDFVersion(x.y,type)
class TypePredicateProcessor : public PredicateProcessor {
public:
    TypePredicateProcessor(CPDFFile* pdfo, std::string s) :
        PredicateProcessor(pdfo, s)
        { /* constructor */ };
    virtual bool ValidateRowSyntax();
    std::string ReduceRow();
};


/// @brief Implements predicate support for the Arlington "SinceVersion" field (column 3)
/// - only "1.0" or "1.1" or ... or "1.7 or "2.0"
class SinceVersionPredicateProcessor : public PredicateProcessor {
public:
    SinceVersionPredicateProcessor(CPDFFile* pdfo, std::string s) :
        PredicateProcessor(pdfo, s)
        { /* constructor */ };
    virtual bool ValidateRowSyntax();
    bool ReduceRow();
};


/// @brief Implements predicate support for the Arlington "DeprecatedIn" field (column 4)
/// - only blank ("") or "1.0" or "1.1" or ... or "1.7 or "2.0"
class DeprecatedInPredicateProcessor : public PredicateProcessor {
public:
    DeprecatedInPredicateProcessor(CPDFFile* pdfo, std::string s) :
        PredicateProcessor(pdfo, s)
        { /* constructor */ };
    virtual bool ValidateRowSyntax();
    bool ReduceRow();
};


/// @brief Implements predicate support for the Arlington "Required" field (column 5)
/// - either TRUE, FALSE or fn:IsRequired(...)
/// - argument of fn:IsRequired(...) can be very flexible, including logical " && " and " || " expressions:
///   . fn:BeforeVersion(x.y), fn:IsPDFVersion(x.y)
///   . fn:IsPresent(key) or fn:NotPresent(key)
///   . \@key==... or \@key!=...
///   . use of Arlington-PDF-Path "::", "parent::"
///   . various highly specialized predicates: fn:IsEncryptedWrapper(), fn:NotStandard14Font(), ...
class RequiredPredicateProcessor : public PredicateProcessor {
public:
    RequiredPredicateProcessor(CPDFFile* pdfo, std::string s) :
        PredicateProcessor(pdfo, s)
        { /* constructor */ };
    virtual bool ValidateRowSyntax();
    bool ReduceRow(ArlPDFObject* obj);
};


/// @brief "IndirectReference" column has 3 possible outcomes
enum class ReferenceType { MustBeDirect, MustBeIndirect, DontCare };

/// @brief Implements predicate support for the Arlington "IndirectReference" field (column 6)
/// - [];[];[]
/// - fn:MustBeDirect()
/// - fn:MustBeDirect(fn:IsPresent(key))
class IndirectRefPredicateProcessor : public PredicateProcessor {
public:
    IndirectRefPredicateProcessor(CPDFFile* pdfo, std::string s) :
        PredicateProcessor(pdfo, s)
        { /* constructor */ };
    virtual bool ValidateRowSyntax();
    ReferenceType ReduceRow(const int type_index);
};


/// @brief Implements predicate support for the Arlington "Inheritable" field (column 7)
/// -- TRUE or FALSE only
class InheritablePredicateProcessor : public PredicateProcessor {
public:
    InheritablePredicateProcessor(CPDFFile* pdfo, std::string s) :
        PredicateProcessor(pdfo, s)
        { /* constructor */ };
    virtual bool ValidateRowSyntax();
    bool ReduceRow();
};


/// @brief Implements predicate support for the Arlington "DefaultValue" field (column 8)
/// - can be [];[];[]
/// - constants
/// - '[' and ']' also used for PDF arrays
class DefaultValuePredicateProcessor : public PredicateProcessor {
public:
    DefaultValuePredicateProcessor(CPDFFile* pdfo, std::string s) :
        PredicateProcessor(pdfo, s)
        { /* constructor */ };
    virtual bool ValidateRowSyntax();
};


/// @brief Implements predicate support for the Arlington "PossibleValues" field (column 9)
/// - can be [];[];[]
/// - complex expressions
/// - '[' and ']' also used for sets and PDF arrays
class PossibleValuesPredicateProcessor : public PredicateProcessor {
public:
    PossibleValuesPredicateProcessor(CPDFFile* pdfo, std::string s) :
        PredicateProcessor(pdfo, s)
        { /* constructor */ };
    virtual bool ValidateRowSyntax();
    bool ReduceRow(ArlPDFObject* object, const int key_idx, const ArlTSVmatrix& tsv_data, const int idx, bool* fully_processed);
    bool IsValidValue(ArlPDFObject* object, const std::string& pvalues);
};



/// @brief Implements predicate support for the Arlington "SpecialCase" field (column 10)
/// - inconsistent [] or no [] at all
/// - NO SEMI-COLONS
/// - complex expressions
class SpecialCasePredicateProcessor : public PredicateProcessor {
public:
    SpecialCasePredicateProcessor(CPDFFile* pdfo, std::string s) :
        PredicateProcessor(pdfo, s)
        { /* constructor */ };
    virtual bool ValidateRowSyntax();
};


/// @brief Implements predicate support for the Arlington "Links" field (column 11)
/// - [];[];[]
/// - fn:SinceVersion(x.y,type)
/// - fn:Deprecated(x.y,type)
/// - fn:BeforeVersion(x.y,type)
/// - fn:IsPDFVersion(x.y,type)
class LinkPredicateProcessor : public PredicateProcessor {
public:
    LinkPredicateProcessor(CPDFFile* pdfo, std::string s) :
        PredicateProcessor(pdfo, s)
        { /* constructor */ };
    virtual bool ValidateRowSyntax();
    std::string ReduceRow();
};

/// Arlington "Notes" field (column 12)
/// - free form text so no support required

#endif // PredicateProcessor_h
