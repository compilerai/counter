#include "rewrite/assembly_handling.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <iterator>
#include <map>
#include <fstream>
#include "support/debug.h"
#include "rewrite/translation_instance.h"
#include "rewrite/static_translate.h"

void
assembly_file_t::populate_assembly_labels_and_locations()
{
  asm_subline_context_t subline_context;
  for (auto iter = m_lines.begin(); iter != m_lines.end(); iter++) {
    string const& line = *iter;
    CPP_DBG_EXEC2(READ_ASM_FILE, cout << __func__ << " " << __LINE__ << ": line =\n" << line << endl);
    vector<string> sublines = get_sublines_for_line(line);
    for (size_t i = 0; i < sublines.size(); i++) {
      process_subline(sublines.at(i), iter, i, subline_context);
    }
  }
  CPP_DBG_EXEC(READ_ASM_FILE, cout << __func__ << " " << __LINE__ << ": finally:\n"; this->assembly_file_to_stream(cout); cout << endl);
}

vector<string>
assembly_file_t::get_sublines_for_line(string const& line)
{
  return splitOnChar(line, ';');
}

void
assembly_file_t::process_subline(string const& subline_in, list<string>::iterator line_iterator, int subline_num, asm_subline_context_t& subline_context)
{
  string subline = subline_in;
  trim(subline);
  size_t hash = subline.find('#');
  if (hash != string::npos) {
    subline = subline.substr(0, hash);
  }
  char const* slchars = subline.c_str();
  char const* ptr = slchars;

  CPP_DBG_EXEC2(READ_ASM_FILE, cout << __func__ << " " << __LINE__ << ": subline = '" << subline << "'\n");
  while (1) {
    string token1, token2;
    size_t token_size = get_next_token(ptr, token1);
    CPP_DBG_EXEC2(READ_ASM_FILE, cout << __func__ << " " << __LINE__ << ": token1 = '" << token1 << "'\n");
    if (token_size == 0) {
      return;
    }
    ptr += token_size;
    if (token1.length() == 0) {
      return;
    }
    if (token1 == ":") { //if colon, this looks like an assembly error; return anyway
      return;
    }
    ptr += get_next_token(ptr, token2);
    CPP_DBG_EXEC2(READ_ASM_FILE, cout << __func__ << " " << __LINE__ << ": token2 = '" << token2 << "'\n");
    if (token2 == ":") {
      subline_context.add_label(token1);
      CPP_DBG_EXEC2(READ_ASM_FILE, cout << __func__ << " " << __LINE__ << ": continuing loop to read more tokens starting at " << ptr << endl);
    } else if (   string_has_prefix(token1, ".file")
               || string_has_prefix(token1, ".loc")
               || string_has_prefix(token1, ".cfi_")) {
      CPP_DBG_EXEC2(READ_ASM_FILE, cout << __func__ << " " << __LINE__ << ": ignoring directive (without clearing labels) for subline: " << subline << endl);
      return;
    } else if (token1.at(0) == '.') {
      CPP_DBG_EXEC2(READ_ASM_FILE, cout << __func__ << " " << __LINE__ << ": ignoring directive and clearing labels for subline: " << subline << endl);
      subline_context.clear_cur_labels();
      return;
    } else {
      CPP_DBG_EXEC2(READ_ASM_FILE, cout << __func__ << " " << __LINE__ << ": adding instruction at " << subline_in << endl);
      m_insn_labels.insert(make_pair(subline_context.get_insn_id(), subline_context.get_cur_labels()));
      m_insn_locations.insert(make_pair(subline_context.get_insn_id(), make_tuple(line_iterator, subline_num, subline_in)));
      subline_context.increment_insn_id();
      return;
    }
  }
}

size_t
assembly_file_t::get_next_token(char const* buf, string& token)
{
  token.clear();
  char const* ptr = buf;
  while (*ptr && isspace(*ptr)) {
    ptr++;
  }
  if (!*ptr) {
    return ptr - buf;
  }
  if (*ptr == ':') {
    token.push_back(*ptr);
    return ptr + 1 - buf;
  }
  while (*ptr && !isspace(*ptr) && *ptr != ':') {
    token.push_back(*ptr);
    ptr++;
  }
  return ptr - buf;
}

void
assembly_file_t::assembly_file_to_stream(ostream& os) const
{
  for (auto const& il : m_insn_locations) {
    os << "insn_id " << il.first << ":\n";
    os << get<2>(il.second) << endl;
    if (m_insn_labels.count(il.first) && m_insn_labels.at(il.first).size()) {
      os << "labels:";
      for (auto const& l : m_insn_labels.at(il.first)) {
        os << " " << l.get_name();
      }
      os << endl;
    }
  }
}

void
assembly_file_t::asm_file_remove_marker_calls(set<asm_insn_id_t> const& marker_call_instructions)
{
  vector<list<string>::iterator> lines_changed;
  for (auto const& marker_insn_id : marker_call_instructions) {
    get<2>(m_insn_locations.at(marker_insn_id)) = "";
    list<string>::iterator line_iter = get<0>(m_insn_locations.at(marker_insn_id));
    lines_changed.push_back(line_iter);
  }
  //TODO: dedup lines_changed; not consequential though
  for (auto line_iter : lines_changed) {
    *line_iter = reconstruct_line_from_modified_insn_locations(line_iter);
  }
}

string
assembly_file_t::reconstruct_line_from_modified_insn_locations(list<string>::iterator iter) const
{
  string ret;
  for (auto const& insn_loc : m_insn_locations) {
    if (get<0>(insn_loc.second) == iter) {
      ret += get<2>(insn_loc.second) + ";";
    }
  }
  return ret;
}

void
assembly_file_t::asm_file_gen_annotated_assembly(ostream& os, map<asm_insn_id_t, string> const& insn_annotations, map<string, map<string, string>> const& exit_annotations) const
{
  list<string>::const_iterator cur_line = m_lines.begin();
  for (auto const& insn_annot : insn_annotations) {
    asm_insn_id_t next_asm_insn_id = insn_annot.first;
    list<string>::const_iterator next_line = get<0>(m_insn_locations.at(next_asm_insn_id));
    while (cur_line != next_line) {
      os << *cur_line << endl;
      cur_line++;
    }
    asm_line_semicolon_number_t subline_id = get<1>(m_insn_locations.at(next_asm_insn_id));
    string const& annot = insn_annot.second;
    if (subline_id == 0) {
      os << annot;
      os << *cur_line << endl;
    } else {
      auto iter = m_insn_locations.find(next_asm_insn_id);
      ASSERT(iter != m_insn_locations.end());
      list<string>::const_iterator cur_line = get<0>(iter->second);
      bool added_annotation = false;
      while (get<0>(iter->second) == cur_line) {
        if (get<1>(iter->second) == subline_id) {
          os << annot;
          added_annotation = true;
        }
        os << get<2>(iter->second) << endl;
        iter++;
      }
      ASSERT(added_annotation);
    }
    cur_line++;
  }
  while (cur_line != m_lines.end()) {
    os << *cur_line << endl;
    cur_line++;
  }
}

void
assembly_file_t::gen_unified_assembly_from_assembly_files(string const& asmresult, vector<asmfile_t> const& asmfiles)
{
  if (asmfiles.size() == 1) {
    file_copy(asmfiles.at(0), asmresult);
  } else {
    NOT_IMPLEMENTED();
  }
}

asm_insn_id_t
assembly_file_t::get_start_asm_insn_id_for_function(string const& function_name)
{
  for (auto const& insn_label : m_insn_labels) {
    if (set_belongs(insn_label.second, assembly_label_t(function_name))) {
      return insn_label.first;
    }
  }
  cout << __func__ << " " << __LINE__ << ": could not find asm_insn_id for function '" << function_name << "'\n";
  cout << _FNLN_ << ": m_insn_labels (size " << m_insn_labels.size() << ") =\n";
  for (auto const& il : m_insn_labels) {
    cout << il.first << " --> ";
    for (auto const& al : il.second) {
      cout << " " << al.get_name();
    }
    cout << endl;
  }
  cout.flush();
  NOT_REACHED();
}

map<asm_insn_id_t, string>
assembly_file_t::get_insn_id_to_linename_map() const
{
  map<asm_insn_id_t, string> ret;
  for (auto const& iloc : m_insn_locations) {
    size_t linenum = std::distance(m_lines.begin(), (list<string>::const_iterator)get<0>(iloc.second));
    stringstream ss;
    ss << LINENAME_PREFIX << linenum;
    ret.insert(make_pair(iloc.first, ss.str()));
    //cout << __func__ << " " << __LINE__ << ": adding insn id " << iloc.first << " --> " << ss.str() << endl;
  }
  return ret;
}
