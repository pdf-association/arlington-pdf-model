////////////////////////////////////////////////////////////////////////////////////////////////////
// ParseObjects.h
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
#include <queue>

#include "GrammarFile.h"
#include "Pdfix.h"
#include "utils.h"

using namespace PDFixSDK;

#ifdef GetObject
#undef GetObject
#endif

class CParsePDF
{
  //remembering processed objects (and how they were validated)
  std::map<PdsObject*, std::string> mapped;
  
  // cache of loaded grammar files
  std::map<std::string, std::unique_ptr<CGrammarReader>> grammar_map;

  // simulating recursive processing of the PDObjects
  struct queue_elem {
    PdsObject* object;
    std::string link;
    std::string context;
    queue_elem(PdsObject* o, const std::string &l, std::string &c)
      : object(o), link(l), context(c)
    {}
  };
  std::queue<queue_elem> to_process;

  std::string grammar_folder;
  PdfDoc* pdf_doc;
  std::ofstream &output;
  const std::vector<std::vector<std::string>>* get_grammar(const std::string& link);

public:
  CParsePDF(PdfDoc* doc, std::string tsv_folder, std::ofstream &ofs) :
    pdf_doc(doc), grammar_folder(tsv_folder), output(ofs)
  { }

  void add_parse_object(PdsObject* object, const std::string& link, std::string context);
  void parse_object(); 
  void parse_name_tree(PdsDictionary* obj, const std::string &links, std::string context);
  void parse_number_tree(PdsDictionary* obj, const std::string &links, std::string context);

  std::string select_one(PdsObject* obj, const std::string &links_string, std::string &obj_name);
  std::string get_link_for_type(PdsObject* obj, const std::string &types, const std::string &links);
  int get_type_index(PdsObject *obj, std::string types);
  std::string get_type_string(PdsObject *obj);
  void check_basics(PdsObject *object, const std::vector<std::string> &vec, const std::string &grammar_file);
  bool check_possible_values(PdsObject* object, const std::string& possible_value_str, int index, std::wstring& real_str_value);
};

