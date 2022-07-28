///////////////////////////////////////////////////////////////////////////////
/// @file
/// @brief ArlVersion class declaration
///
/// @copyright
/// Copyright 2022 PDF Association, Inc. https://www.pdfa.org
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

#ifndef ArlVersion_h
#define ArlVersion_h
#pragma once

#include "ArlingtonPDFShim.h"

#include <string>
#include <vector>


using namespace ArlingtonPDFShim;


/// @brief
/// Handling PDF and Arlington versioning is complicated!
///
/// Arlington complex fields ([];[];[]) are always the same length regardless
/// But when processing a PDF file, some complex sets may reduce in size because a Type is
/// version-dependent - and thus the same integer index may no longer "align" to other complex
/// fields.
///
/// +--------------------------------+-------+-------+-------+------------------------+
/// |                   PDF version: |  < X  |   X   |  > X  | Enum                   |
/// +--------------------------------+-------+-------+-------+------------------------+
/// | fn:BeforeVersion(X) - removed  |  OK   | Error | Error | After_fnBeforeVersion  |
/// | fn:SinceVersion(X)             | Error |  OK   |  OK   | Before_fnSinceVersion  |
/// | fn:IsPDFVersion(X)             | Error |  OK   | Error | Not_fnIsPDFVersion     |
/// | fn:Deprecated(X) - not removed |  OK   | Warn  | Warn  | Is_fnDeprecated        |
/// +--------------------------------+-------+-------+-------+------------------------+
///
/// (Removed / Not Removed refers to what happens with the definition in the PDF spec.)
///
/// Things can also have MORE than 1 version:
/// fn:Deprecated(2.0, fn:SinceVersion(1.5, xxx)) --> only valid in PDF 1.5, 1.6 and 1.7, then deprecated from 2.0
///  - if PDF 1.4 --> Before_fnSinceVersion --> error message
///  - if PDF 1.6 --> OK (no message)
///  - if PDF 2.0 --> Is_fnDeprecated --> warning message
///
/// Error, warning and info messages can be selective if an enum is associated with the type of a PDF object
/// (when processed against a specific PDF version).
enum class ArlVersionReason { Unknown = 0, OK, After_fnBeforeVersion, Before_fnSinceVersion, Not_fnIsPDFVersion, Is_fnDeprecated };


/// @brief Class to support versioning of Arlington with a given PDF object and PDF version for a file
struct ArlVersion {
private:
    /// @brief the raw Arlington TSV data row
    std::vector<std::string>    tsv;

    /// @brief PDF version of file being analyzed (multiplied by 10 to make an integer)
    int                 pdf_version;

    /// @brief PDF version of any Arlington 'Type' field version predicate related to arl_type.
    /// 0 means no predicate was defined in Arlington.
    int                 arl_version;

    /// @brief the index into the Arlington 'Type' field for arl_type
    /// This can then be used to index into other Arlington TSV fields.
    int                 arl_type_index;

    /// @brief how the PDF object type directly maps across (e.g. integer)
    std::string         arl_type_of_pdf_object;

    /// @brief more refined Arlington type from Arlington (e.g. bitmask).
    /// Always compatible with arl_type_of_pdf_object
    std::string         arl_type;

    /// @brief any versioning from Arlington TSV data
    ArlVersionReason    version_reason;

public:
    /// @brief Constructor
    ArlVersion(ArlPDFObject* obj, std::vector<std::string> vec, const int pdf_ver);

    bool             object_matched_arlington_type() { return (arl_type.size() > 0); };
    std::string      get_object_arlington_type() { return arl_type_of_pdf_object; };
    std::string      get_matched_arlington_type() { return arl_type; };
    std::vector<std::string> get_appropriate_linkset(std::string arl_links);
    std::vector<std::string> get_full_linkset(std::string arl_links);
    int              get_arlington_type_index() { return arl_type_index; };

    ArlVersionReason get_version_reason() { return version_reason; };
    int get_reason_version() { return arl_version; }
};

#endif // ArlVersion_h
