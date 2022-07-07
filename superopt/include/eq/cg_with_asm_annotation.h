#pragma once

#include "support/log.h"
#include "support/mymemory.h"
#include "support/backtracker.h"

#include "expr/counter_example.h"

#include "rewrite/assembly_handling.h"

#include "gsupport/corr_graph_edge.h"
#include "gsupport/failcond.h"

#include "graph/eqclasses.h"
#include "graph/graph_with_guessing.h"

#include "tfg/tfg.h"

#include "eq/cg_with_backtracker.h"
#include "eq/unsafe_cond.h"

namespace eqspace {

using namespace std;

class cg_with_asm_annotation : public cg_with_backtracker
{
private:
  string m_asm_filename;
  mutable failcond_t m_failcond = failcond_t::failcond_null();
  mutable map<pc, string> m_src_pc_linename_map;
  mutable map<pc, string> m_asm_pc_linename_map;
public:
  cg_with_asm_annotation(cg_with_backtracker const &cg, string const& asm_filename) : cg_with_backtracker(cg), m_asm_filename(asm_filename)
  { }

  cg_with_asm_annotation(string_ref const &name, string_ref const& fname, dshared_ptr<eqcheck> const& eq, dshared_ptr<backtracker> const& backtracker) : cg_with_backtracker(name, fname, eq, backtracker), m_asm_filename(eq->get_asm_filename())
  { }

  cg_with_asm_annotation(cg_with_asm_annotation const& other) : cg_with_backtracker(other), m_asm_filename(other.m_asm_filename), m_src_pc_linename_map(other.m_src_pc_linename_map), m_asm_pc_linename_map(other.m_asm_pc_linename_map)
  { }

  tuple<set<asm_insn_id_t>, map<asm_insn_id_t, string>, map<string, string>> cg_obtain_marker_call_and_other_annotations(asm_insn_id_t start_asm_insn_id) const;

  virtual void cg_xml_print(ostream& os, string const& prefix = "") const;

  static dshared_ptr<cg_with_asm_annotation> corr_graph_from_stream(istream& is, context* ctx);

  failcond_t const& cg_with_asm_annotation_get_failcond() const { return m_failcond; }

  virtual void graph_to_stream(ostream& os, string const& prefix="") const override;

  virtual bool mark_graph_unstable(pcpair const& pp, failcond_t const& failcond) const override;
  virtual void cg_pcpair_xml_print(ostream& os, pcpair const& pp) const override;
protected:
private:
  pair<map<dst_ulong, string>, map<nextpc_id_t, string>> cg_get_dst_pc_and_nextpc_annotations() const;
  string get_asm_annotation_for_pcpair(pcpair const& pp) const;
  std::map<asm_insn_id_t, pc> dst_tfg_get_insn_id_to_pc_map(asm_insn_id_t start_insn_id) const;
  std::map<pc, asm_insn_id_t> dst_tfg_get_pc_to_insn_id_map(asm_insn_id_t start_insn_id) const;
  std::map<pc, dst_ulong> dst_tfg_get_pc_to_dst_insn_addr_map() const;
  std::map<dst_ulong, pc> dst_tfg_get_dst_insn_addr_to_pc_map() const;
  std::map<asm_insn_id_t, dst_ulong> dst_tfg_get_insn_id_to_dst_insn_addr_map(asm_insn_id_t start_insn_id) const;
  std::map<dst_ulong, asm_insn_id_t> dst_tfg_get_dst_insn_addr_to_insn_id_map(asm_insn_id_t start_insn_id) const;
};

}
