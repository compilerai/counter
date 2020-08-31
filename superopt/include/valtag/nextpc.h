#pragma once
#include <iostream>
#include "support/types.h"
#include "support/src_tag.h"

class nextpc_t
{
private:
  src_tag_t m_tag;
  src_ulong m_val;
public:
  nextpc_t() : m_tag(tag_const), m_val((src_ulong)-1) {}
  nextpc_t(src_ulong v) : m_tag(tag_const), m_val(v) {}
  nextpc_t(src_tag_t t, src_ulong v) : m_tag(t), m_val(v) { ASSERT(m_tag == tag_const || m_tag == tag_input_exec_reloc_index || m_tag == tag_src_insn_pc || m_tag == tag_invalid); }
  src_tag_t get_tag() const { return m_tag; }
  src_ulong get_val() const { if (m_tag != tag_const) { std::cerr << "tag = " << tag2str(m_tag) << "\n"; } ASSERT(m_tag == tag_const); return m_val; }
  int get_reloc_idx() const { ASSERT(m_tag == tag_input_exec_reloc_index); return m_val; }
  int get_src_insn_pc() const { ASSERT(m_tag == tag_src_insn_pc); return m_val; }
  bool equals(nextpc_t const& other) const { return m_tag == other.m_tag && m_val == other.m_val; }
  string nextpc_to_string() const;

  static long nextpc_array_search(nextpc_t const* nextpcs_haystack, long num_nextpcs, src_ulong needle);
  static long nextpc_array_search(nextpc_t const* nextpcs_haystack, long num_nextpcs, nextpc_t const& needle);
  static long nextpc_array_min(nextpc_t const* nextpcs, long num_nextpcs);
  static void nextpc_array_copy(nextpc_t* dst, nextpc_t const* src, size_t n);
  static nextpc_t *nextpcs_realloc(nextpc_t* orig, size_t orig_sz, size_t new_size);
  static void nextpcs_get_pcs(src_ulong* dst, nextpc_t const* src, size_t sz);
};


int pcrel2str(int64_t val, int tag, char const *reloc_strings, size_t reloc_strings_size,
    char *buf, int buflen);
