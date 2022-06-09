#pragma once

#include "support/types.h"
#include "support/utils.h"
#include <map>
#include <string>
#include <list>
#include <set>

using namespace std;

using asm_insn_id_t = size_t;

class assembly_label_t {
private:
  string m_name;
public:
  assembly_label_t(string const& name) : m_name(name)
  { }
  string const& get_name() const { return m_name; }
  bool operator<(assembly_label_t const& other) const
  {
    return m_name < other.m_name;
  }
};

class asm_subline_context_t {
private:
  asm_insn_id_t m_insn_id;
  set<assembly_label_t> m_cur_labels;
public:
  asm_subline_context_t() : m_insn_id(0)
  { }
  asm_insn_id_t get_insn_id() const
  {
    return m_insn_id;
  }
  void increment_insn_id()
  {
    m_insn_id++;
    m_cur_labels.clear();
  }
  void clear_cur_labels()
  {
    m_cur_labels.clear();
  }
  void add_label(assembly_label_t const& label)
  {
    m_cur_labels.insert(label);
  }
  set<assembly_label_t> const& get_cur_labels() const
  {
    return m_cur_labels;
  }
};

class assembly_file_t {
private:
  using function_name_t = string;
  using asm_line_number_t = size_t;
  using asm_line_semicolon_number_t = size_t;

  list<string> m_lines;

  map<asm_insn_id_t, set<assembly_label_t>> m_insn_labels;   //the set of labels that alias with this instruction

  map<asm_insn_id_t, tuple<list<string>::iterator, asm_line_semicolon_number_t, string>> m_insn_locations; //map from insn_id to <pointer to line, semicolon number in that line, string of that subline>
public:
  assembly_file_t(string const& filename) : m_lines(file_get_lines(filename))
  {
    populate_assembly_labels_and_locations();
  }
  //void convert_semicolons_to_newlines();
  //void annotate_using_etfg_harvest_and_proof_files(string const& etfg_filename, string const& harvest_filename, string const& proof_filename);
  void assembly_file_to_stream(ostream& os) const;
  void asm_file_remove_marker_calls(set<asm_insn_id_t> const& marker_call_instructions);
  void asm_file_gen_annotated_assembly(ostream& os, map<asm_insn_id_t, string> const& pc_annotations, map<string, map<string, string>> const& exit_annotations) const; //exit annotations: function-name -> exitname -> annotation
  static void gen_unified_assembly_from_assembly_files(string const& asmresult, vector<asmfile_t> const& asmfiles);
  asm_insn_id_t get_start_asm_insn_id_for_function(string const& function_name);
  map<asm_insn_id_t, string> get_insn_id_to_linename_map() const;
private:
  void populate_assembly_labels_and_locations();
  static vector<string> get_sublines_for_line(string const& line);
  void process_subline(string const& subline_in, list<string>::iterator line_iterator, int subline_num, asm_subline_context_t& subline_context);
  string reconstruct_line_from_modified_insn_locations(list<string>::iterator iter) const;
  static size_t get_next_token(char const* buf, string& token);
};
