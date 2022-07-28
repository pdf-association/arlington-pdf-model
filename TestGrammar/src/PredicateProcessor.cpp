///////////////////////////////////////////////////////////////////////////////
/// @file 
/// @brief A left-to-right, recursive descent regex-based parser for Arlington predicates.
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

#include "PredicateProcessor.h"
#include "ArlPredicates.h"
#include "LRParsePredicate.h"
#include "PDFFile.h"
#include "utils.h"

#include <iterator>
#include <regex>
#include <cassert>
#include <math.h>
#include <algorithm>


/// @brief \#define PP_DEBUG to see the predicate decomposition as they are calculated
#undef PP_DEBUG


/// @brief Recursively empty the predicate AST data structure and then clear it.
void PredicateProcessor::EmptyPredicateAST() {
    // A vector of ASTNodeStack = a vector of vectors of ASTNodes
    if (!predicate_ast.empty()) {
        for (int i = (int)predicate_ast.size()-1; i >= 0; i--)
            if (predicate_ast[i].size() > 0) {
                for (int j = (int)predicate_ast[i].size()-1; j >= 0; j--)
                    delete predicate_ast[i][j];
                predicate_ast[i].clear(); // inner vector
            }
        predicate_ast.clear(); // outer vector of matrix
    }
};


/// @brief Validates an Arlington "Key" field (column 1)
/// - no predicates allowed
/// - No COMMAs or SEMI-COLONs
/// - any alphanumeric including "." or "-" or "_" only
/// - any integer (i.e. an array index)
/// - wildcard "*" by itself - must be last row (cannot be checked here)
/// - integer + "*" for a repeating set of N array elements - all rows (cannot be checked here)
/// 
/// @param[in]   key_idx     the key index into the TSV data
/// 
/// @returns  true if the TSV data is valid. false otherwise. 
bool PredicateProcessor::ValidateKeySyntax(const int key_idx) {
    // no predicates allowed
    assert((key_idx >= 0) && (key_idx < tsv.size()));
    std::string tsv_field = tsv[key_idx][TSV_KEYNAME];

    if (tsv_field.find("fn:") != std::string::npos)
        return false;

    return std::regex_search(tsv_field, r_Keys);
}


/// @brief Validates an Arlington "Type" field (column 2)
/// Arlington types are all lowercase.
///  - fn:SinceVersion(x.y,type)
///  - fn:Deprecated(x.y,type)
///  - fn:BeforeVersion(x.y,type)
///  - fn:IsPDFVersion(x.y,type)
/// 
/// @param[in]   key_idx     the key index into the TSV data
/// 
/// @returns true if the TSV data is valid. false otherwise.
bool PredicateProcessor::ValidateTypeSyntax(const int key_idx) {
    assert((key_idx >= 0) && (key_idx < tsv.size()));
    std::string tsv_field = tsv[key_idx][TSV_TYPE];

    std::vector<std::string> type_list = split(tsv_field, ';');
    bool valid;
    for (auto& t : type_list) {
        std::smatch     m;
        if (t.find("fn:") == std::string::npos) {
            valid = FindInVector(v_ArlAllTypes, t);
            if (!valid)
                return false;
        }
        else if (std::regex_search(t, m, r_Types) && m.ready() && (m.size() == 4)) {
            // m[1] = predicate function name (no "fn:")
            // m[2] = PDF version "x.y"
            // fn:BeforeVersion(1.0,xxx) makes no sense and fn:SinceVersion(1.0,xxx) is pointless overhead!!
            valid = !(((m[1] == "BeforeVersion") || (m[1] == "SinceVersion")) && (m[2] == "1.0"));
            if (!valid)
                return false;
            valid = FindInVector(v_ArlPDFVersions, m[2]);
            if (!valid)
                return false;
            // m[3] = Arlington pre-defined type
            valid = FindInVector(v_ArlAllTypes, m[3]);
            if (!valid)
                return false;
        }
        else
            return false;
    } // for
    return true;
}


/// @brief Validates an Arlington "SinceVersion" field (column 3)
/// - only "1.0" or "1.1" or ... or "1.7 or "2.0"
/// 
/// @param[in]   key_idx     the key index into the TSV data
/// 
/// @returns true if the TSV data is valid, false otherwise
bool PredicateProcessor::ValidateSinceVersionSyntax(const int key_idx) {
    assert((key_idx >= 0) && (key_idx < tsv.size()));
    std::string tsv_field = tsv[key_idx][TSV_SINCEVERSION];

    if (tsv_field.size() == 3)
        return FindInVector(v_ArlPDFVersions, tsv_field);
    return false;
}


/// @brief Determines if the current Arlington row is valid based on the "SinceVersion" field (column 3)
///
/// @param[in]   key_idx     the key index into the TSV data
/// 
/// @returns true if this row is valid for the specified by PDF version. false otherwise
bool PredicateProcessor::IsValidForPDFVersion(const int key_idx) {
    assert((key_idx >= 0) && (key_idx < tsv.size()));
    std::string tsv_field = tsv[key_idx][TSV_SINCEVERSION];
    pdfc->ClearPredicateStatus();

    // PDF version "x.y" --> convert to integer as x*10 + y
    assert(tsv_field.size() == 3);

    int pdf_v = pdfc->pdf_version[0] * 10 + pdfc->pdf_version[2];
    int tsv_v = tsv_field[0]   * 10 + tsv_field[2];
    return (tsv_v <= pdf_v);
};


/// @brief Validates an Arlington "DeprecatedIn" field (column 4)
/// - only "", "1.0" or "1.1" or ... or "1.7 or "2.0"
///
/// @param[in]   key_idx     the key index into the TSV data 
/// 
/// @returns true if the field is valid, false otherwise
bool PredicateProcessor::ValidateDeprecatedInSyntax(const int key_idx) {
    assert((key_idx >= 0) && (key_idx < tsv.size()));
    std::string tsv_field = tsv[key_idx][TSV_DEPRECATEDIN];

    if (tsv_field == "")
        return true;
    else if (tsv_field.size() == 3)
        return FindInVector(v_ArlPDFVersions, tsv_field);
    return false;
}


/// @brief Determines if the current Arlington row states that it is deprecated based "DeprecatedIn" field (column 4)
///
/// @param[in]   key_idx     the key index into the TSV data
/// 
/// @returns true if this row is deprecated. false otherwise
bool PredicateProcessor::IsDeprecated(const int key_idx) {
    assert((key_idx >= 0) && (key_idx < tsv.size()));
    std::string tsv_field = tsv[key_idx][TSV_DEPRECATEDIN];

    pdfc->ClearPredicateStatus();

    if (tsv_field == "")
        return false;

    // PDF version "x.y" --> convert to integer as x*10 + y
    assert(tsv_field.size() == 3);
    assert(pdfc->pdf_version.size() == 3);

    int pdf_v = pdfc->pdf_version[0] * 10 + pdfc->pdf_version[2];
    int tsv_v = tsv_field[0]   * 10 + tsv_field[2];
    return (tsv_v >= pdf_v);
};


/// @brief Validates an Arlington "Required" field (column 5)
/// - either TRUE, FALSE or fn:IsRequired(...)
/// - inner can be very flexible expressions, including logical operators " && " and " || ":
///   . fn:BeforeVersion(x.y), fn:IsPDFVersion(x.y)
///   . fn:IsPresent(key) or fn:Not(fn:IsPresent(key))
///   . \@key==value or \@key!=value
///   . use of Arlington-PDF-Path key syntax "::", "parent::"
///   . various highly specialized predicates: fn:IsEncryptedWrapper(), fn:NotStandard14Font(), ...
/// 
/// @param[in]   key_idx     the key index into the TSV data
/// 
/// @returns true if the TSV data is valid, false otherwise
bool PredicateProcessor::ValidateRequiredSyntax(const int key_idx) {
    assert((key_idx >= 0) && (key_idx < tsv.size()));
    std::string tsv_field = tsv[key_idx][TSV_REQUIRED];

    if ((tsv_field == "TRUE") || (tsv_field == "FALSE"))
        return true;
    else if ((tsv_field.find("fn:IsRequired(") == 0) && (tsv_field[tsv_field.size()-1] == ')')) {
        ASTNode *ast = new ASTNode();
        ASTNodeStack stack;

        std::string whats_left = LRParsePredicate(tsv_field, ast);
        EmptyPredicateAST();
        stack.push_back(ast);
        predicate_ast.push_back(stack);
        return (whats_left.size() == 0);
    }
    return false;
}


/// @brief Reduces an Arlington "Required" field (column 5) for a given PDF version and parent PDF object.
/// - either TRUE, FALSE or fn:IsRequired(...)
/// - NO SEMI-COLONs or [ ]
/// - inner can be very flexible, including logical && and || expressions:
///   . fn:BeforeVersion(x.y), fn:IsPDFVersion(x.y)
///   . fn:IsPresent(key) or fn:Not(fn:IsPresent(key))
///   . \@key==... or \@key!=...
///   . use of Arlington-PDF-Path "::", "parent::"
///   . various highly specialized predicates: fn:IsEncryptedWrapper(), fn:NotStandard14Font(), ...
///
/// @param[in]   parent      the parent object (needed for predicates)
/// @param[in]   obj         the object of the current key
/// @param[in]   key_idx     the key index into the TSV data
/// @param[in]   type_idx    the index into the 'Type' field
/// 
/// @returns true if field is required for the PDF version and PDF object
bool PredicateProcessor::IsRequired(ArlPDFObject* parent, ArlPDFObject* obj, const int key_idx, const int type_idx) {
    assert(parent != nullptr);
    assert(obj != nullptr);
    bool retval = false;

    assert((key_idx >= 0) && (key_idx < tsv.size()));
    std::string tsv_field = tsv[key_idx][TSV_REQUIRED];
    pdfc->ClearPredicateStatus();
    EmptyPredicateAST();

    if (tsv_field == "TRUE")
        retval = true;
    else if ((tsv_field == "FALSE") || (type_idx < 0)) 
        retval = false;
    else {
        ASTNode* ast = new ASTNode();
        ASTNodeStack stack;

        std::string whats_left = LRParsePredicate(tsv_field, ast);
        stack.push_back(ast);
        predicate_ast.push_back(stack);
        assert(whats_left.size() == 0);

        /// Process the AST using the PDF objects - expect reduction to a boolean true/false
        ASTNode* pp = pdfc->ProcessPredicate(parent, obj, predicate_ast[0][0], key_idx, tsv, type_idx);
        assert(pp != nullptr);
        assert(pp->valid());
        assert(pp->type == ASTNodeType::ASTNT_ConstPDFBoolean);
        retval = (pp->node == "true") ? true : false;
        delete pp;
    }
    return retval;
}


/// @brief Validates an Arlington "IndirectReference" field (column 6)
/// - TRUE or FALSE or complex array of TRUE/FALSE [];[];[]
/// - fn:MustBeDirect()
/// - fn:MustBeDirect(...)
/// 
/// @param[in]   key_idx     the key index into the TSV data
/// 
/// @returns true if the TSV data is valid, false otherwise
bool PredicateProcessor::ValidateIndirectRefSyntax(const int key_idx) {
    assert((key_idx >= 0) && (key_idx < tsv.size()));
    std::string tsv_field = tsv[key_idx][TSV_INDIRECTREF];

    if ((tsv_field == "TRUE") || (tsv_field == "FALSE") || (tsv_field == "fn:MustBeDirect()"))
        return true;
    else if (tsv_field.find(";") != std::string::npos) {
        // complex form: [];[];[]
        std::vector<std::string> indirect_list = split(tsv_field, ';');
        for (auto& ir : indirect_list)
            if ((ir != "[TRUE]") && (ir != "[FALSE]"))
                return false;
        return true;
    }
    else {
        ASTNode *ast = new ASTNode();
        ASTNodeStack stack;

        std::string whats_left = LRParsePredicate(tsv_field, ast);
        assert((ast->node == "fn:MustBeDirect(") || (ast->node == "fn:MustBeIndirect("));
        EmptyPredicateAST();
        stack.push_back(ast);
        predicate_ast.push_back(stack);
        return (whats_left.size() == 0);
    }
}



/// @brief Reduces an Arlington "IndirectReference" field (column 6) based on a Type index
/// - TRUE or FALSE or complex array of TRUE/FALSE [];[];[]
/// - fn:MustBeDirect()
/// - fn:MustBeDirect(...)
/// - fn:MustBeIndirect(...)
/// Untested but should also handle complex array with predicates in the future
/// 
/// @param[in]   parent      the parent object (needed for predicates)
/// @param[in]   object      the object of the current key
/// @param[in]   key_idx     the key index into the TSV data
/// @param[in]   type_index  the index into the 'Type' field
/// 
/// @returns the requirement for indirectness: must be direct, must be indirect, or don't care
ReferenceType PredicateProcessor::ReduceIndirectRefRow(ArlPDFObject* parent, ArlPDFObject* object, const int key_idx, const int type_index) {
    assert(type_index >= 0);
    assert((key_idx >= 0) && (key_idx < tsv.size()));
    std::string tsv_field = tsv[key_idx][TSV_INDIRECTREF];
    pdfc->ClearPredicateStatus();

    if (tsv_field == "TRUE") {
        return ReferenceType::MustBeIndirect;
    }
    else if (tsv_field == "FALSE") {
        return ReferenceType::DontCare;
    }
    else if (tsv_field == "fn:MustBeDirect()") { // Quite common so special case
        return ReferenceType::MustBeDirect;
    }
    else { // a complex type [];[];[] and/or predicate expression
        std::vector<std::string> ir_list = split(tsv_field, ';');
        assert(type_index < (int)ir_list.size());
        std::string s = ir_list[type_index];

        if (s[0] == '[')
            s = s.substr(1, s.size() - 2); // strip off any '[' and ']'

        // Handle common trivial complex case
        if (s == "TRUE")
            return ReferenceType::MustBeIndirect;
        else if (s == "FALSE")
            return ReferenceType::DontCare;

        // Must be a predicate
        assert(s.find("fn:") != std::string::npos);
#ifdef PP_DEBUG
        std::cout << std::endl << "IndirectRef::ReduceRow " << s << std::endl;
#endif 
        EmptyPredicateAST();
        ASTNodeStack stack;
        int loop = 0;
        do {
            ASTNode* n = new ASTNode();

            s = LRParsePredicate(s, n);
            stack.push_back(n);
            loop++;
            while ((s.size() > 0) && ((s[0] == ',') || (s[0] == ' '))) {
                s = s.substr(1, s.size() - 1); // skip over COMMAs and SPACEs
            }
        } while ((s.size() > 0) && (loop < 100));
        if (loop >= 100) {
            assert(false && "Arlington complex type IndirectRef field too long and complex!");
            return ReferenceType::DontCare;
        }

        predicate_ast.push_back(stack);

        // Only makes sense for 'IndirectRef' field if there is one expression and
        // this expression has an outer predicate AND results in a boolean!
        // Outer predicate must be either "fn:MustBeDirect(" or "fn:MustBeIndirect("
        assert(predicate_ast.size() == 1); 
        assert(stack[0]->type == ASTNodeType::ASTNT_Predicate);
        assert((stack[0]->node == "fn:MustBeDirect(") || (stack[0]->node == "fn:MustBeIndirect("));
        assert(stack[0]->arg[1] == nullptr); // optional 1st argument only, never 2nd arg

        ReferenceType ret_val = (stack[0]->node == "fn:MustBeDirect(") ? ReferenceType::MustBeDirect : ReferenceType::MustBeIndirect;
        if (stack[0]->arg[0] == nullptr)
            return ret_val;

        // Have a predicate with an internal expression (argument) that needs to be processed...
        ASTNode* pp = pdfc->ProcessPredicate(parent, object, stack[0]->arg[0], key_idx, tsv, type_index);
        assert(pp != nullptr);
        assert(pp->valid());
        assert(pp->type == ASTNodeType::ASTNT_ConstPDFBoolean);

        // Answer should always be a true/false (boolean)
        bool b = (pp->node == "true"); // Cache answer so can delete pp
        delete pp;
        if (pdfc->PredicateWasFullyProcessed())
            if (b) {
                return ret_val;
            } 
            else {
                // was false so invert outer predicate
                if (ret_val == ReferenceType::MustBeDirect) 
                    return ReferenceType::MustBeIndirect; 
                else if (ret_val == ReferenceType::MustBeIndirect) 
                    return ReferenceType::MustBeDirect;   
            }
    }
    return ReferenceType::DontCare; // Default behaviour (incl. not fully processed predicates)
}


/// @brief Validates an Arlington "Inheritable" field (column 7)
/// - only TRUE or FALSE
/// 
/// @param[in]   key_idx   the index into TSV data for the key of interest
/// 
/// @returns true if "TRUE" or "FALSE"
bool PredicateProcessor::ValidateInheritableSyntax(const int key_idx) {
    assert((key_idx >= 0) && (key_idx < tsv.size()));
    std::string tsv_field = tsv[key_idx][TSV_INHERITABLE];

    return ((tsv_field == "TRUE") || (tsv_field == "FALSE"));
}


/// @brief Validates an Arlington "Inheritable" field (column 7)
/// - only TRUE or FALSE
///
/// @returns true if the row is inheritable, false otherwise
bool PredicateProcessor::IsInheritable(const int key_idx) {
    assert((key_idx >= 0) && (key_idx < tsv.size()));
    std::string tsv_field = tsv[key_idx][TSV_INHERITABLE];
    pdfc->ClearPredicateStatus();

    return (tsv_field == "TRUE");
}


/// @brief Validates an Arlington "DefaultValue" field (column 8)
/// Can be pretty much anything but so as long as it parses, assume it is OK.
/// 
/// @param[in]   key_idx   the index into TSV data for the key of interest
///
/// @returns true if syntax is valid. false otherwise
bool PredicateProcessor::ValidateDefaultValueSyntax(const int key_idx) {
    assert((key_idx >= 0) && (key_idx < tsv.size()));
    std::string tsv_field = tsv[key_idx][TSV_DEFAULTVALUE];

    if (tsv_field == "")
        return true;

    std::string s;
    ASTNodeStack stack;
    EmptyPredicateAST();

    if (tsv_field.find(";") != std::string::npos) {
        // complex type [];[];[], so therefore everything has [ and ]
        std::vector<std::string> dv_list = split(tsv_field, ';');

        for (auto& dv : dv_list) {
            stack.clear();
            if (dv.find("fn:") != std::string::npos) {
                int loop = 0;
                s = dv.substr(1, dv.size() - 2); // strip off '[' and ']'
                do {
                    ASTNode* n = new ASTNode();

                    s = LRParsePredicate(s, n);
                    stack.push_back(n);
                    loop++;
                    while ((s.size() > 0) && ((s[0] == ',') || (s[0] == ' '))) {
                        s = s.substr(1, s.size() - 1); // skip over COMMAs and SPACEs
                    }
                } while ((s.size() > 0) && (loop < 100));
                if (loop >= 100) {
                    assert(false && "Arlington complex type DefaultValue field too long and complex!");
                    return false;
                }
            }
            predicate_ast.push_back(stack);
        } // for
    }
    else {
        // non-complex type - [ and ] may be PDF array with SPACE separators so don't strip
        if (tsv_field.find("fn:") != std::string::npos) {
            int loop = 0;
            s = tsv_field;
            do {
                ASTNode* n = new ASTNode();
                s = LRParsePredicate(s, n);
                stack.push_back(n);
                loop++;
                while ((s.size() > 0) && ((s[0] == ',') || (s[0] == ' '))) {
                    s = s.substr(1, s.size() - 1); // skip over COMMAs and SPACEs
                }
            } while ((s.size() > 0) && (loop < 100));
            if (loop >= 100) {
                assert(false && "Arlington simple type DefaultValue field too long and complex!");
                return false;
            }
        } 
        predicate_ast.push_back(stack);
    }
    return true;
}


/// @brief Converts the DefaultValue for the specified Arlington type into an ASTNode tree
/// 
/// @param[in]   key_idx   the index into TSV data for the key of interest
/// 
/// @returns an ASTNode tree or nullptr if nothing or an error
ASTNode* PredicateProcessor::GetDefaultValue(const int key_idx) {
    assert((key_idx >= 0) && (key_idx < tsv.size()));
    std::string tsv_field = tsv[key_idx][TSV_DEFAULTVALUE];
    pdfc->ClearPredicateStatus();

    if (tsv_field == "") 
        return nullptr;

    std::string s;
    ASTNodeStack stack;
    EmptyPredicateAST();

    if (tsv_field.find(";") != std::string::npos) {
        // complex type [];[];[], so therefore everything has [ and ]
        std::vector<std::string> dv_list = split(tsv_field, ';');

        for (auto& dv : dv_list) {
            stack.clear();
            if (dv.find("fn:") != std::string::npos) {
                int loop = 0;
                s = dv.substr(1, dv.size() - 2); // strip off '[' and ']'
                do {
                    ASTNode* n = new ASTNode();

                    s = LRParsePredicate(s, n);
                    stack.push_back(n);
                    loop++;
                    while ((s.size() > 0) && ((s[0] == ',') || (s[0] == ' '))) {
                        s = s.substr(1, s.size() - 1); // skip over COMMAs and SPACEs
                    }
                } while ((s.size() > 0) && (loop < 100));
                if (loop >= 100) {
                    assert(false && "Arlington complex type DefaultValue field too long and complex!");
                    return nullptr;
                }
            }
            predicate_ast.push_back(stack);
        } // for
    }
    else {
        // non-complex type - [ and ] may be PDF array with SPACE separators so don't strip
        if (tsv_field.find("fn:") != std::string::npos) {
            int loop = 0;
            s = tsv_field;
            do {
                ASTNode* n = new ASTNode();
                s = LRParsePredicate(s, n);
                stack.push_back(n);
                loop++;
                while ((s.size() > 0) && ((s[0] == ',') || (s[0] == ' '))) {
                    s = s.substr(1, s.size() - 1); // skip over COMMAs and SPACEs
                }
            } while ((s.size() > 0) && (loop < 100));
            if (loop >= 100) {
                assert(false && "Arlington simple type DefaultValue field too long and complex!");
                return nullptr;
            }
        }
        predicate_ast.push_back(stack);
    }

    // Parsed the DefaultValue, now work out which AST to return based in Type index (idx)
    if (key_idx < (int)predicate_ast.size())
        return predicate_ast[key_idx][0];
    else
        return nullptr;
}



/// @brief  Validates an Arlington "PossibleValues" row (column 9)
/// Can be pretty much anything so as long as it parses, assume it is OK.
/// 
/// @param[in]   key_idx   the index into TSV data for the key of interest 
///
/// @returns true if syntax is valid. false otherwise
bool PredicateProcessor::ValidatePossibleValuesSyntax(const int key_idx) {
    assert((key_idx >= 0) && (key_idx < tsv.size()));
    std::string tsv_field = tsv[key_idx][TSV_POSSIBLEVALUES];

    if (tsv_field == "")
        return true;

    std::vector<std::string> pv_list = split(tsv_field, ';');
    std::string s;
    ASTNodeStack stack;
    EmptyPredicateAST();

    for (auto& pv : pv_list) {
        stack.clear();
        if (pv.find("fn:") != std::string::npos) {
            int loop = 0;
            s = pv.substr(1, pv.size() - 2); // strip off '[' and ']'
            do {
                ASTNode* n = new ASTNode();
                s = LRParsePredicate(s, n);
                stack.push_back(n);
                loop++;
                while ((s.size() > 0) && ((s[0] == ',') || (s[0] == ' '))) {
                    s = s.substr(1, s.size() - 1); // skip over COMMAs and SPACEs
                }
            } while ((s.size() > 0) && (loop < 100));
            if (loop >= 100) {
                assert(false && "Arlington complex type PossibleValues field too long and complex when validating!");
                return false;
            }
        }
        predicate_ast.push_back(stack);
    }
    return true;
}


/// @brief Checks if the PDF object matches a valid value from a COMMA-separated set
/// 
/// @param[in]   object    the PDF object
/// @param[in]   key_idx   the index into TSV data for the key of interest
/// @param[in]   pvalues   possible values list (COMMA-separated). NO PREDICATES!
/// 
/// @returns true if the PDF object matches something in the list and is thus a valid value.
bool PredicateProcessor::IsValidValue(ArlPDFObject* object, const int key_idx, const std::string& pvalues) {
    assert((key_idx >= 0) && (key_idx < tsv.size()));
    std::string tsv_field = tsv[key_idx][TSV_POSSIBLEVALUES];
    pdfc->ClearPredicateStatus();

    assert(pvalues.find("fn:") == std::string::npos);
    PDFObjectType obj_type = object->get_object_type();
    std::vector<std::string> val_list = split(pvalues, ',');
    bool retval = false;

    switch (obj_type) {
        case PDFObjectType::ArlPDFObjTypeNull:
            // null always matches so always OK
            retval = true;
            break;

        case PDFObjectType::ArlPDFObjTypeName:
            {
                // PDF Names are raw with no leading SLASH - can string match
                // PDF SDKs have sorted out #-escapes 
                std::string nm = ToUtf8(((ArlPDFName*)object)->get_value());
                auto it = std::find(val_list.begin(), val_list.end(), nm);
                retval = (it != val_list.end());
            }
            break;

        case PDFObjectType::ArlPDFObjTypeString:
            {
                // PDF Strings are single quoted in Arlington so strip then string match
                // PDF SDKs have sorted out hex strings, escapes, etc.
                std::string s = "'" + ToUtf8(((ArlPDFString*)object)->get_value()) + "'";
                auto it = std::find(val_list.begin(), val_list.end(), s);
                retval = (it != val_list.end());
            }
            break;

        case PDFObjectType::ArlPDFObjTypeNumber:
            if (((ArlPDFNumber*)object)->is_integer_value()) {
                // Integers can be directly matched numerically
                int num_value = ((ArlPDFNumber*)object)->get_integer_value();
                auto it = std::find(val_list.begin(), val_list.end(), std::to_string(num_value));
                retval = (it != val_list.end());
            }
            else {
                // Real number need a tolerance for matching
                double num_value = ((ArlPDFNumber*)object)->get_value();
                for (auto& it : val_list) {
                    try {
                        auto double_val = std::stod(it);
                        // Double-precision comparison often fails because parsed PDF value is not precisely stored
                        // Old Adobe PDF specs used to recommend 5 digits so go +/- half of that
                        if (fabs(num_value - double_val) <= ArlNumberTolerance) {
                            retval = true;
                            break;
                        }
                    }
                    catch (...) {
                        // fallthrough, iterate and do next option in options list
                    }
                } // for
            }
            break;

        case PDFObjectType::ArlPDFObjTypeArray:
            {
                // Arrays can have Possible Values e.g. XObjectImageMask Decode = [[0,1],[1,0]] 
                ArlPDFArray* arr = (ArlPDFArray*)object;
                int arr_len = arr->get_num_elements();
                for (int i = 0; (i < (int)val_list.size()) && !retval; i++) {
                    assert((val_list[i][0] == '[') && (pvalues[val_list[i].size() - 1] == ']'));
                    if ((arr_len == 2) && ((val_list[i] == "[0,1]") || (val_list[i] == "[1,0]"))) {
                        /// @todo - Hard-coded only for Decode arrays!
                        ArlPDFNumber* a0 = (ArlPDFNumber*)arr->get_value(0);
                        ArlPDFNumber* a1 = (ArlPDFNumber*)arr->get_value(1);
                        if ((a0 != nullptr) && (a0->get_object_type() == PDFObjectType::ArlPDFObjTypeNumber) &&
                            (a1 != nullptr) && (a1->get_object_type() == PDFObjectType::ArlPDFObjTypeNumber)) 
                        {
                            if (((a0->get_value() == 0.0) && (a1->get_value() == 1.0)) ||
                                ((a0->get_value() == 1.0) && (a1->get_value() == 0.0)))
                            {
                                retval = true;
                                break;
                            }
                        }
                        delete a0;
                        delete a1;
                    }
                }
            }
            break;

        case PDFObjectType::ArlPDFObjTypeBoolean:    // Booleans don't have Possible Values in Arlington!
            assert(false && "Booleans don't have Possible Values");
            break;
        case PDFObjectType::ArlPDFObjTypeDictionary: // Dictionaries are linked types and don't have Possible Values!
            assert(false && "Dictionaries are linked types and don't have Possible Values");
            break;
        case PDFObjectType::ArlPDFObjTypeStream:     // Streams are linked types and don't have Possible Values!
            assert(false && "Streams are linked types and don't have Possible Values");
            break;
        case PDFObjectType::ArlPDFObjTypeReference:  // Should not happen
            assert(false && "ArlPDFObjTypeReference when matching Possible Values");
            break;
        case PDFObjectType::ArlPDFObjTypeUnknown:    // Should not happen
            assert(false && "ArlPDFObjTypeUnknown when matching Possible Values");
            break;
        default:                                     // Should not happen
            assert(false && "default when matching Possible Values");
            break;
    }
    return retval;
}



/// @brief Validates an Arlington "SpecialCase" field (column 10)
///
/// @param[in] key_idx      the row index into tsv_data matrix for this key
/// 
/// @returns   true if the TSV data is valid. false otherwise.
bool PredicateProcessor::ValidateSpecialCaseSyntax(const int key_idx) {
    assert((key_idx >= 0) && (key_idx < tsv.size()));
    std::string tsv_field = tsv[key_idx][TSV_SPECIALCASE];

    if (tsv_field == "")
        return true;

    std::string s;
    std::vector<std::string> sc_list = split(tsv_field, ';');
    ASTNodeStack stack;
    EmptyPredicateAST();

    for (auto& sc : sc_list) {
        int loop = 0;
        stack.clear();
        s = sc.substr(1, sc.size() - 2); // strip off '[' and ']'
        do {
            ASTNode *n = new ASTNode();
            s = LRParsePredicate(s, n);
            stack.push_back(n);
            loop++;
            while ((s.size() > 0) && ((s[0] == ',') || (s[0] == ' '))) {
                s = s.substr(1, s.size() - 1); // skip over COMMAs and SPACEs
            }
        } while ((s.size() > 0) && (loop < 100));
        if (loop >= 100) {
            assert(false && "Arlington complex type SpecialCase field too long and complex when validating!");
            return false;
        }
        predicate_ast.push_back(stack);
    } // for
    return true;
}



/// @brief Validates an Arlington "Links" field (column 11)
///  - fn:SinceVersion(x.y,link)
///  - fn:Deprecated(x.y,link)
///  - fn:BeforeVersion(x.y,link)
///  - fn:IsPDFVersion(x.y,link)
/// 
/// @param[in] key_idx      the row index into tsv_data matrix for this key
/// 
/// @returns true if the TSV data is valid. false otherwise.
bool PredicateProcessor::ValidateLinksSyntax(const int key_idx) {
    assert((key_idx >= 0) && (key_idx < tsv.size()));
    std::string tsv_field = tsv[key_idx][TSV_LINK];

    // Nothing to do?
    if (tsv_field.find("fn:") == std::string::npos)
        return true;

    std::vector<std::string> link_list = split(tsv_field, ';');
    bool valid;
    for (auto& lnk : link_list) {
        std::smatch     m;
        if (std::regex_search(lnk, m, r_Links) && m.ready() && (m.size() == 4)) {
            // m[1] = predicate function name (no "fn:")
            // m[2] = PDF version "x.y"
            // fn:BeforeVersion(1.0,xxx) makes no sense and fn:SinceVersion(1.0,xxx) is pointless overhead!!
            valid = !(((m[1] == "BeforeVersion") || (m[1] == "SinceVersion")) && (m[2] == "1.0"));
            if (!valid)
                return false;
            valid = FindInVector(v_ArlPDFVersions, m[2]);
            if (!valid)
                return false;
            // m[3] = Arlington link (TSV filename)
        }
        else if (lnk.find("fn:") != std::string::npos)
            return false;
    } // for
    return true;
}



/// @brief Reduces an Arlington "Links" field (column 11) based on a PDF version
///  - fn:SinceVersion(x.y,link)
///  - fn:Deprecated(x.y,link)
///  - fn:BeforeVersion(x.y,link)
///  - fn:IsPDFVersion(x.y,link)
/// 
/// @param[in] key_idx      the row index into tsv_data matrix for this key
///
/// @returns an Arlington Links field with all predicates removed. May be empty string "".
std::string PredicateProcessor::ReduceLinkRow(const int key_idx) {
    assert((key_idx >= 0) && (key_idx < tsv.size()));
    std::string tsv_field = tsv[key_idx][TSV_LINK];
    pdfc->ClearPredicateStatus();

    // Nothing to do?
    if (tsv_field.find("fn:") == std::string::npos)
        return tsv_field;

    std::string to_ret = "";
    std::vector<std::string> link_list = split(tsv_field, ';');
    for (auto& lnk : link_list) {
        std::smatch     m;
        if (std::regex_search(lnk, m, r_Links) && m.ready() && (m.size() == 4)) {
            // m[1] = predicate function name (no "fn:")
            // m[2] = PDF version "x.y" --> convert to integer as x*10 + y
            int pdf_v = pdfc->pdf_version[0] * 10 + pdfc->pdf_version[2];
            assert(m[2].str().size() == 3);
            int arl_v = m[2].str()[0] * 10 + m[2].str()[2];
            if (((m[1] == "SinceVersion")  && (pdf_v >= arl_v)) ||
                ((m[1] == "BeforeVersion") && (pdf_v <  arl_v)) ||
                ((m[1] == "IsPDFVersion")  && (pdf_v == arl_v)) ||
                ((m[1] == "Deprecated")    && (pdf_v <  arl_v))) {
                // m[3] = Arlington link
                if (to_ret == "")
                    to_ret = m[3];
                else
                    to_ret += ";" + m[3].str();
            }
        }
        else {
            if (to_ret == "")
                to_ret = lnk;
            else
                to_ret += ";" + lnk;
        }
    } // for

    assert(to_ret.find("fn:") == std::string::npos);
    return to_ret;
}



/// @brief Reduces an Arlington "PossibleValues" row (column 9)
/// Can be pretty much anything.
///
/// @param[in] parent       PDF object of parent (that contains object) so dict, array or stream
/// @param[in] object       PDF object (in parent)
/// @param[in] key_idx      the row index into tsv_data matrix for this key
/// @param[in] type_idx     the index in the 'Type' field of the Arlington TSV data 
/// 
/// @returns true if syntax is valid. false otherwise
bool PredicateProcessor::ReducePVRow(ArlPDFObject* parent, ArlPDFObject* object, const int key_idx, const int type_idx) {
    assert(parent != nullptr);
    assert(object != nullptr);
    assert((key_idx >= 0) && (key_idx < tsv.size()));

    std::string tsv_field = tsv[key_idx][TSV_POSSIBLEVALUES];
    pdfc->ClearPredicateStatus();

    if ((tsv_field == "") || (tsv_field == "[]"))
        return true;

    /// Split on SEMI-COLON
    std::vector<std::string> pv_list = split(tsv_field, ';');

    // Complex types (arrays, dicts, streams) are just "[]" so this reduces away
    assert((type_idx >= 0) && (type_idx < (int)pv_list.size()));
    if (pv_list[type_idx] == "[]")
        return true;

    EmptyPredicateAST();
    ASTNodeStack stack;

    for (auto& pv : pv_list) {
        stack.clear();
        if (pv.find("fn:") != std::string::npos) {
            int loop = 0;
            std::string s = pv.substr(1, pv.size() - 2); // strip off '[' and ']'
            do {
                ASTNode* n = new ASTNode();
                s = LRParsePredicate(s, n);
                stack.push_back(n);
                loop++;
                while ((s.size() > 0) && ((s[0] == ',') || (s[0] == ' '))) {
                    s = s.substr(1, s.size() - 1); // skip over COMMAs and SPACEs
                }
            } while ((s.size() > 0) && (loop < 100));
            if (loop >= 100) {
                assert(false && "Arlington complex type PossibleValues field too long and complex when reducing!");
                return false;
            }
        }
        predicate_ast.push_back(stack);
    }

    // There should now be a vector of ASTs or nullptr for each type of the TSV field
    assert(predicate_ast.size() == pv_list.size());
    assert(type_idx < (int)predicate_ast.size());
    assert(pv_list[type_idx][0] == '[');

    std::string s = pv_list[type_idx].substr(1, pv_list[type_idx].size() - 2); // strip off '[' and ']'

    if ((predicate_ast[type_idx].size() == 0) || (predicate_ast[type_idx][0] == nullptr)) {
        // No predicates - but could be a set of COMMA-separated constants (e.g. names, integers, etc.)
        return IsValidValue(object, key_idx, s);
    }

    // At least one predicate was in the COMMA list of Possible Values
    // Walk vector of ASTs
#ifdef PP_DEBUG
    std::cout << std::endl << "PossibleValues: " << s << std::endl;
#endif 
    stack = predicate_ast[type_idx];
    for (auto i = 0; i < (int)stack.size(); i++) {
        ASTNode* n = stack[i];

        switch (n->type) {
        case ASTNodeType::ASTNT_ConstPDFBoolean:
        case ASTNodeType::ASTNT_ConstString:
        case ASTNodeType::ASTNT_ConstInt:
        case ASTNodeType::ASTNT_ConstNum:
        case ASTNodeType::ASTNT_Key:
            // Primitive type means this is a NON-predicate value so see if it is a match
            // otherwise loop and keep trying...
            assert((n->arg[0] == nullptr) && (n->arg[1] == nullptr));
            if (IsValidValue(object, key_idx, n->node))
                return true;
            break;

        case ASTNodeType::ASTNT_Predicate:
            {
                ASTNode *pp = pdfc->ProcessPredicate(parent, object, n, key_idx, tsv, type_idx);
                if (pp != nullptr) {
                    // Booleans can either be a valid value OR the result of an fn:Eval(...) calculation
                    ASTNodeType pp_type = pp->type;
                    bool vv = (pp->node == "true");
                    if ((pp->type != ASTNodeType::ASTNT_ConstPDFBoolean) && (object->get_object_type() != PDFObjectType::ArlPDFObjTypeBoolean)) {
                        vv = IsValidValue(object, key_idx, pp->node);
                    }
                    delete pp;
                    switch (pp_type) {
                        case ASTNodeType::ASTNT_ConstPDFBoolean:
                            return vv;
                        case ASTNodeType::ASTNT_ConstString:
                        case ASTNodeType::ASTNT_ConstInt:
                        case ASTNodeType::ASTNT_ConstNum:
                        case ASTNodeType::ASTNT_Key:
                            if (vv) 
                                return true;
                            break;
                        default:
                            assert(false && "unexpected node type from ProcessPredicate!");
                            return false;
                    } // switch
                } // if 
            }
            break;

        case ASTNodeType::ASTNT_MathComp:  // Math comparison operators - cannot be start of an AST!
        case ASTNodeType::ASTNT_MathOp:    // Math operators - cannot be start of an AST!
        case ASTNodeType::ASTNT_LogicalOp: // Logical operators - cannot be start of an AST!
        case ASTNodeType::ASTNT_KeyValue:  // @keyname - cannot be start of an AST! Needs to be wrapped in fn:Eval()
        case ASTNodeType::ASTNT_Unknown:
        case ASTNodeType::ASTNT_Type:
        default:
            // Likely a parsing error or bad Arlington data! Check via "--validate" CLI option
            assert(false && "unexpected AST node when reducing Possible Values!");
            return false;
        } // switch
    } // for
    return false;
}



/// @brief Reduces an Arlington "PossibleValues" row (column 9)
/// Can be pretty much anything.
///
/// @param[in] parent       PDF object of parent (that contains object) so dict, array or stream
/// @param[in] object       PDF object (in parent)
/// @param[in] key_idx      the row index into tsv_data matrix for this key
/// @param[in] type_idx     the index in the 'Type' field of the Arlington TSV data 
/// 
/// @returns true if syntax is valid. false otherwise
bool PredicateProcessor::ReduceSCRow(ArlPDFObject* parent, ArlPDFObject* object, const int key_idx, const int type_idx) {
    assert(parent != nullptr);
    assert(object != nullptr);
    assert((key_idx >= 0) && (key_idx < tsv.size()));
    assert(type_idx >= 0);

    std::string tsv_field = tsv[key_idx][TSV_SPECIALCASE];
    pdfc->ClearPredicateStatus();

    if (tsv_field == "")
        return true;

    /// Split on SEMI-COLON
    std::vector<std::string> sc_list = split(tsv_field, ';');

    // Special cases is either a single [] for all Types, or a complex [];[];[] that matches Type field 
    // Complex types (arrays, dicts, streams) are just "[]" so this reduces away
    if ((sc_list.size() > 1) && (type_idx > 0) && (type_idx < (int)sc_list.size())) {
        if (sc_list[type_idx] == "[]")
            return true;
    }
    else if (sc_list[0] == "[]")
        return true;

    EmptyPredicateAST();
    ASTNodeStack stack;

    for (auto& sc : sc_list) {
        stack.clear();
        if (sc.find("fn:") != std::string::npos) {
            int loop = 0;
            std::string s = sc.substr(1, sc.size() - 2); // strip off '[' and ']'
            do {
                ASTNode* n = new ASTNode();
                s = LRParsePredicate(s, n);
                stack.push_back(n);
                loop++;
                while ((s.size() > 0) && ((s[0] == ',') || (s[0] == ' '))) {
                    s = s.substr(1, s.size() - 1); // skip over COMMAs and SPACEs
                }
            } while ((s.size() > 0) && (loop < 100));
            if (loop >= 100) {
                assert(false && "Arlington complex type SpecialCAse field too long and complex when reducing!");
                return false;
            }
        }
        predicate_ast.push_back(stack);
    }

    // There should now be a vector of ASTs or nullptr for each type of the TSV field,
    assert(predicate_ast.size() == sc_list.size());
    assert(sc_list[type_idx][0] == '[');

    std::string s = sc_list[type_idx].substr(1, sc_list[type_idx].size() - 2); // strip off '[' and ']'

    if ((predicate_ast[type_idx].size() == 0) || (predicate_ast[type_idx][0] == nullptr))
        return true;

#ifdef PP_DEBUG
    std::cout << "SpecialCase: " << s << std::endl;
#endif 
    stack = predicate_ast[type_idx];
    assert(stack.size() == 1);
    
    ASTNode* n = stack[0];
    if (n->type == ASTNodeType::ASTNT_Predicate) {
        bool valid = true;
        ASTNode* pp = pdfc->ProcessPredicate(parent, object, n, key_idx, tsv, type_idx);
        // SpecialCase can return nullptr only when versioning makes everything go away...
        if (pp != nullptr) {
            assert(pp->valid());
            assert(pp->type == ASTNodeType::ASTNT_ConstPDFBoolean);
            valid = (pp->node == "true");
        }
        delete pp;
        return valid;
    }
    else {
        // Likely a parsing error or bad Arlington data! Check via "--validate" CLI option
        assert(false && "unexpected AST node type when reducing Special Case!");
    } 
    return false;
}
