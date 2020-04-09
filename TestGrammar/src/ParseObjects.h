////////////////////////////////////////////////////////////////////////////////////////////////////
// ParseObjects.h
// Copyright (c) 2020 Normex, Pdfix. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
  Reading the whole PDF starting from specific object and validating against grammar provided via tsv file
*/

#pragma once

#include <string>
#include <map>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <memory>
#include <algorithm> 
#include <codecvt>

#include "GrammarFile.h"
#include "Pdfix.h"
#include "utils.h"

using namespace PDFixSDK;

#ifdef GetObject
#undef GetObject
#endif

class CParsePDF
{
  std::map<PdsObject*, int> mapped;
  std::string grammar_folder;
  PdfDoc* pdf_doc;
  std::ofstream &output;

public:
  CParsePDF(PdfDoc* doc, std::string tsv_folder, std::ofstream &ofs) :
    pdf_doc(doc), grammar_folder(tsv_folder), output(ofs)
  { }

  void parse_object(PdsObject *object, std::string link, std::string context);
  void parse_name_tree(PdsDictionary* obj, std::string links, std::string context);
  void parse_number_tree(PdsDictionary* obj, std::string links, std::string context);

  std::string select_one(PdsObject* obj, const std::string &links_string, std::string &obj_name);
  std::string get_link_for_type(PdsObject* obj, const std::string &types, const std::string &links);
  int get_type_index(PdsObject *obj, std::string types);
};

