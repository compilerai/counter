#include <stdlib.h>
#include <stdio.h>

#include "support/src-defs.h"
#include "support/debug.h"
#include "support/utils.h"
#include "support/distinct_sets.h"
#include "support/relations.h"

#include "valtag/nextpc.h"
#include "valtag/valtag.h"

#include "graph/graph.h"
#include "graph/dfa.h"

#include "i386/insn.h"
#include "x64/insn.h"
#include "ppc/insn.h"

#include "insn/dst-insn.h"

#include "rewrite/dst_insn_schedule.h"
#include "rewrite/dst_insn_folding_opts.h"

using namespace eqspace;
using namespace std;

static char as1[409600];

class dst_insn_ptr_t
{
  dst_insn_t *m_ptr, *m_start;
  bool m_is_start;
  bool m_is_exit;
  long m_exitnum;
public:
  dst_insn_ptr_t() : m_ptr(NULL), m_start(NULL), m_is_start(false), m_is_exit(false), m_exitnum(-1) {}
  dst_insn_ptr_t(dst_insn_t *ptr, dst_insn_t *start) : m_ptr(ptr), m_start(start), m_is_start(false), m_is_exit(false), m_exitnum(-1) {}
  dst_insn_ptr_t(dst_insn_t *ptr, dst_insn_t *start, bool is_start) : m_ptr(ptr), m_start(start), m_is_start(is_start), m_is_exit(false), m_exitnum(-1) {}
  dst_insn_ptr_t(long exitnum) : m_ptr(NULL), m_start(NULL), m_is_start(false), m_is_exit(true), m_exitnum(exitnum) {}
  string to_string() const { stringstream ss; ss << (m_ptr - m_start); return ss.str(); }
  dst_insn_t *get_ptr() const { return m_ptr; }
  long get_inum() const { return m_ptr - m_start; }
  bool operator<(dst_insn_ptr_t const &other) const { return this->m_ptr < other.m_ptr; }
  bool is_start() const { return m_is_start; }
  bool is_exit() const { return m_is_exit; }
  bool operator==(dst_insn_ptr_t const &other) const
  {
    if (this->m_is_exit != other.m_is_exit) {
      return false;
    }
    if (this->m_is_exit) {
      return this->m_exitnum == other.m_exitnum;
    } else {
      return this->m_ptr == other.m_ptr;
    }
    NOT_REACHED();
  }
};

namespace std
{
using namespace eqspace;
template<>
struct hash<dst_insn_ptr_t>
{
  std::size_t operator()(dst_insn_ptr_t const &p) const
  {
    return (size_t)p.get_inum();
  }
};
};





static dshared_ptr<graph_with_regions_t<dst_insn_ptr_t, node<dst_insn_ptr_t>, edge<dst_insn_ptr_t>>>
dst_iseq_get_cfg(dst_insn_t *dst_iseq, long dst_iseq_len)
{
  auto g = make_dshared<graph_with_regions_t<dst_insn_ptr_t, node<dst_insn_ptr_t>, edge<dst_insn_ptr_t>>>();
  for (size_t i = 0; i < dst_iseq_len; i++) {
    dshared_ptr<node<dst_insn_ptr_t>> n = make_dshared<node<dst_insn_ptr_t>>(dst_insn_ptr_t(&dst_iseq[i], dst_iseq, i == 0));
    g->add_node(n);
  }
  for (size_t i = 0; i < dst_iseq_len; i++) {
    vector<valtag_t> vts = dst_insn_get_pcrel_values(&dst_iseq[i]);
    ASSERT(vts.size() <= 1);
    if (vts.size() == 0) {
      //cout << __func__ << " " << __LINE__ << ": dst_iseq[" << i << "] = " << dst_insn_to_string(&dst_iseq[i], as1, sizeof as1) << endl;
      //ASSERT(i < dst_iseq_len - 1 || dst_insn_is_function_return(&dst_iseq[i]));
      if (!dst_insn_is_indirect_branch(&dst_iseq[i])) {
        dshared_ptr<edge<dst_insn_ptr_t> const> e = make_dshared<edge<dst_insn_ptr_t>>(dst_insn_ptr_t(&dst_iseq[i], dst_iseq), dst_insn_ptr_t(&dst_iseq[i + 1], dst_iseq));
        CPP_DBG_EXEC(DST_ISEQ_GET_CFG, cout << __func__ << " " << __LINE__ << ": adding edge " << e->to_string() << endl);
        g->add_edge(e);
      } else {
        if (!g->find_node(dst_insn_ptr_t(0L))) {
          dshared_ptr<node<dst_insn_ptr_t>> n = make_dshared<node<dst_insn_ptr_t>>(dst_insn_ptr_t(0L));
          g->add_node(n);
        }
        dshared_ptr<edge<dst_insn_ptr_t> const> e = make_dshared<edge<dst_insn_ptr_t>>(dst_insn_ptr_t(&dst_iseq[i], dst_iseq), 0L);
        CPP_DBG_EXEC(DST_ISEQ_GET_CFG, cout << __func__ << " " << __LINE__ << ": adding edge " << e->to_string() << endl);
        g->add_edge(e);
      }
    } else {
      ASSERT(vts.size() == 1);
      if (dst_insn_is_unconditional_branch_not_fcall(&dst_iseq[i])) {
        /*if (vts.at(0).tag == tag_const) {
          long dst_insn_index = i + 1 + vts.at(0).val;
          ASSERT(dst_insn_index >= 0 && dst_insn_index < dst_iseq_len);
          shared_ptr<edge<dst_insn_ptr_t>> e = make_shared<edge<dst_insn_ptr_t>>(dst_insn_ptr_t(&dst_iseq[i], dst_iseq), dst_insn_ptr_t(&dst_iseq[dst_insn_index], dst_iseq));
          g.add_edge(e);
        } else */if (vts.at(0).tag == tag_var) {
          if (!g->find_node(dst_insn_ptr_t(vts.at(0).val))) {
            dshared_ptr<node<dst_insn_ptr_t>> n = make_dshared<node<dst_insn_ptr_t>>(dst_insn_ptr_t(vts.at(0).val));
            g->add_node(n);
          }
          dshared_ptr<edge<dst_insn_ptr_t> const> e = make_dshared<edge<dst_insn_ptr_t>>(dst_insn_ptr_t(&dst_iseq[i], dst_iseq), vts.at(0).val);
          CPP_DBG_EXEC(DST_ISEQ_GET_CFG, cout << __func__ << " " << __LINE__ << ": adding edge " << e->to_string() << endl);
          g->add_edge(e);
        } else if (vts.at(0).tag == tag_abs_inum) {
          long dst_insn_index = vts.at(0).val;
          ASSERT(dst_insn_index >= 0 && dst_insn_index < dst_iseq_len);
          dshared_ptr<edge<dst_insn_ptr_t> const> e = make_dshared<edge<dst_insn_ptr_t>>(dst_insn_ptr_t(&dst_iseq[i], dst_iseq), dst_insn_ptr_t(&dst_iseq[dst_insn_index], dst_iseq));
          CPP_DBG_EXEC(DST_ISEQ_GET_CFG, cout << __func__ << " " << __LINE__ << ": adding edge " << e->to_string() << endl);
          g->add_edge(e);
        } else if (   vts.at(0).tag == tag_reloc_string
                   || vts.at(0).tag == tag_src_insn_pc) { //e.g., due to tail call optimization
          if (!g->find_node(dst_insn_ptr_t(0L))) {
            dshared_ptr<node<dst_insn_ptr_t>> n = make_dshared<node<dst_insn_ptr_t>>(dst_insn_ptr_t(0L));
            g->add_node(n);
          }
          dshared_ptr<edge<dst_insn_ptr_t> const> e = make_dshared<edge<dst_insn_ptr_t>>(dst_insn_ptr_t(&dst_iseq[i], dst_iseq), 0L);
          CPP_DBG_EXEC(DST_ISEQ_GET_CFG, cout << __func__ << " " << __LINE__ << ": adding edge " << e->to_string() << endl);
          g->add_edge(e);
        } else {
          pcrel2str(vts.at(0).val, vts.at(0).tag, NULL, 0, as1, sizeof as1);
          CPP_DBG_EXEC(DST_ISEQ_GET_CFG, cout << __func__ << " " << __LINE__ << ": vts.at(0) = " << as1 << endl);
          NOT_REACHED();
        }
      } else {
        ASSERT(   i < dst_iseq_len - 1
               || dst_insn_is_function_call(&dst_iseq[i])); //this could be a call to an exit function
        dshared_ptr<edge<dst_insn_ptr_t> const> e = make_dshared<edge<dst_insn_ptr_t>>(dst_insn_ptr_t(&dst_iseq[i], dst_iseq), dst_insn_ptr_t(&dst_iseq[i + 1], dst_iseq));
        CPP_DBG_EXEC(DST_ISEQ_GET_CFG, cout << __func__ << " " << __LINE__ << ": adding edge " << e->to_string() << endl);
        g->add_edge(e);
        /*if (vts.at(0).tag == tag_const) {
          long dst_insn_index = i + 1 + vts.at(0).val;
          ASSERT(dst_insn_index >= 0 && dst_insn_index < dst_iseq_len);
          shared_ptr<edge<dst_insn_ptr_t>> e = make_shared<edge<dst_insn_ptr_t>>(dst_insn_ptr_t(&dst_iseq[i], dst_iseq), dst_insn_ptr_t(&dst_iseq[dst_insn_index], dst_iseq));
          g->add_edge(e);
        } else */if (vts.at(0).tag == tag_var && !dst_insn_is_function_call(&dst_iseq[i])) {
          if (!g->find_node(dst_insn_ptr_t(vts.at(0).val))) {
            dshared_ptr<node<dst_insn_ptr_t>> n = make_dshared<node<dst_insn_ptr_t>>(dst_insn_ptr_t(vts.at(0).val));
            g->add_node(n);
          }
          dshared_ptr<edge<dst_insn_ptr_t> const> e = make_dshared<edge<dst_insn_ptr_t>>(dst_insn_ptr_t(&dst_iseq[i], dst_iseq), vts.at(0).val);
          CPP_DBG_EXEC(DST_ISEQ_GET_CFG, cout << __func__ << " " << __LINE__ << ": adding edge " << e->to_string() << endl);
          g->add_edge(e);
        } else if (   vts.at(0).tag == tag_reloc_string
                   || vts.at(0).tag == tag_src_insn_pc
                   || (vts.at(0).tag == tag_var && dst_insn_is_function_call(&dst_iseq[i]))) {
          //do nothing
        } else if (vts.at(0).tag == tag_abs_inum) {
          long dst_insn_index = vts.at(0).val;
          ASSERT(dst_insn_index >= 0 && dst_insn_index < dst_iseq_len);
          dshared_ptr<edge<dst_insn_ptr_t> const> e = make_dshared<edge<dst_insn_ptr_t>>(dst_insn_ptr_t(&dst_iseq[i], dst_iseq), dst_insn_ptr_t(&dst_iseq[dst_insn_index], dst_iseq));
          CPP_DBG_EXEC(DST_ISEQ_GET_CFG, cout << __func__ << " " << __LINE__ << ": adding edge " << e->to_string() << endl);
          g->add_edge(e);
        } else {
          CPP_DBG_EXEC(DST_ISEQ_GET_CFG, cout << __func__ << " " << __LINE__ << ": dst_insn = " << dst_insn_to_string(&dst_iseq[i], as1, sizeof as1) << endl);
          //cout << __func__ << " " << __LINE__ << ": vts.at(0).tag = " << tag_to_string(vts.at(0).tag) << endl;
          NOT_REACHED();
        }
      }
    }
  }
  return g;
}

class regset_live_t {
private:
  regset_t m_regs;
public:
  regset_live_t() : m_regs() {}
  regset_live_t(regset_t const &r) : m_regs(r) {}

  static regset_live_t top()
  {
    return regset_live_t(*regset_empty());
  }

  bool regset_live_meet(regset_live_t const &other)
  {
    regset_t old_regs = m_regs;
    regset_union(&m_regs, &other.m_regs);
    return old_regs != m_regs;
  }
  regset_t const &get_regs() const { return m_regs; }
};

class cfg_liveness_dfa_t : public data_flow_analysis<dst_insn_ptr_t, node<dst_insn_ptr_t>, edge<dst_insn_ptr_t>, regset_live_t, dfa_dir_t::backward>
{
public:
  cfg_liveness_dfa_t(dshared_ptr<graph_with_regions_t<dst_insn_ptr_t, node<dst_insn_ptr_t>, edge<dst_insn_ptr_t>> const> cfg, map<dst_insn_ptr_t, regset_live_t> &init_vals) :
                   data_flow_analysis<dst_insn_ptr_t, node<dst_insn_ptr_t>, edge<dst_insn_ptr_t>, regset_live_t, dfa_dir_t::backward>(cfg, init_vals/*, *regset_empty()*/)
  {
  }
  //regset_t meet(regset_t const &a, regset_t const &b)
  //{
  //  regset_t ret = a;
  //  regset_union(&ret, &b);
  //  return ret;
  //}
  //regset_t xfer(shared_ptr<edge<dst_insn_ptr_t>> e, regset_t const &out)
  //{
  //  dst_insn_ptr_t const &from_pc = e->get_from_pc();
  //  dst_insn_t const *from_insn = from_pc.get_ptr();

  //  regset_t use, def;
  //  dst_insn_get_usedef_regs(from_insn, &use, &def);
  //  regset_t ret = out;
  //  regset_diff(&ret, &def);
  //  regset_union(&ret, &use);


  //  if (dst_insn_is_unconditional_branch_not_fcall(from_insn)) {
  //    vector<valtag_t> vts = dst_insn_get_pcrel_values(from_insn);
  //    if (vts.size() == 1) {
  //      if (   vts.at(0).tag == tag_pcrel_reloc_string
  //          || vts.at(0).tag == tag_src_insn_pc) { //e.g., due to tail call optimization
  //        dst_insn_t ret_insn;
  //        size_t n = dst_insn_put_function_return(fixed_reg_mapping_t::dst_fixed_reg_mapping_reserved_identity(), &ret_insn, 1);
  //        ASSERT(n == 1);
  //        dst_insn_get_usedef_regs(&ret_insn, &use, &def);
  //        regset_diff(&ret, &def);
  //        regset_union(&ret, &use);
  //      }
  //    }
  //  }
  //  return ret;
  //}
  dfa_xfer_retval_t xfer_and_meet(dshared_ptr<edge<dst_insn_ptr_t> const> const &e, regset_live_t const &in, regset_live_t &out)
  {
    dst_insn_ptr_t const &from_pc = e->get_from_pc();
    dst_insn_t const *from_insn = from_pc.get_ptr();

    regset_t use, def;
    dst_insn_get_usedef_regs(from_insn, &use, &def);
    regset_t ret = in.get_regs();
    regset_diff(&ret, &def);
    regset_union(&ret, &use);


    if (dst_insn_is_unconditional_branch_not_fcall(from_insn)) {
      vector<valtag_t> vts = dst_insn_get_pcrel_values(from_insn);
      if (vts.size() == 1) {
        if (   vts.at(0).tag == tag_reloc_string
            || vts.at(0).tag == tag_src_insn_pc) { //e.g., due to tail call optimization
          dst_insn_t ret_insn;
          size_t n = dst_insn_put_function_return(fixed_reg_mapping_t::dst_fixed_reg_mapping_reserved_identity(), &ret_insn, 1);
          ASSERT(n == 1);
          dst_insn_get_usedef_regs(&ret_insn, &use, &def);
          regset_diff(&ret, &def);
          regset_union(&ret, &use);
        }
      }
    }
    return out.regset_live_meet(ret) ? DFA_XFER_RETVAL_CHANGED : DFA_XFER_RETVAL_UNCHANGED;
  }
};

static map<dst_insn_ptr_t, regset_t>
cfg_compute_liveness(dshared_ptr<graph_with_regions_t<dst_insn_ptr_t, node<dst_insn_ptr_t>, edge<dst_insn_ptr_t>> const> cfg)
{
  map<dst_insn_ptr_t, regset_live_t> vals;
  cfg_liveness_dfa_t dfa(cfg, vals);
  dfa.initialize(*regset_empty());
  dfa.solve();

  map<dst_insn_ptr_t, regset_t> ret;
  for (const auto &val : vals) {
    ret.insert(make_pair(val.first, val.second.get_regs()));
  }
  //return dfa.get_values();
  return ret;
}

//class reg_used_for_move_only_t {
//public:
//  regset_t live;
//  typedef pair<reg_identifier_t, dst_insn_ptr_t> MOVE_T; //pair of dst reg, pc
//  map<exreg_group_id_t, map<reg_identifier_t, MOVE_T>> moves; // map from (group, src_reg) --> move_t if the src reg is used only in a move instruction subsequently
//  bool operator!=(reg_used_for_move_only_t const &other) const
//  {
//    return !regset_equal(&this->live, &other.live) || !moves_equal(this->moves, other.moves);
//  }
//
//  bool mapping_exists(exreg_group_id_t g, reg_identifier_t const &r) const
//  {
//    return moves.count(g) && moves.at(g).count(r);
//  }
//  MOVE_T const &get_mapping(exreg_group_id_t g, reg_identifier_t const &r) const
//  {
//    ASSERT(moves.count(g));
//    ASSERT(moves.at(g).count(r));
//    return moves.at(g).at(r);
//  }
//  void add_mapping(exreg_group_id_t g, reg_identifier_t const &src, reg_identifier_t const &dst, dst_insn_ptr_t const &pc)
//  {
//    if (moves[g].count(src)) {
//      moves[g].erase(src);
//    }
//    moves[g].insert(make_pair(src, make_pair(dst, pc)));
//  }
//  void remove_mapping(exreg_group_id_t g, reg_identifier_t const &r)
//  {
//    if (!moves.count(g)) {
//      return;
//    }
//    moves.at(g).erase(r);
//  }
//  void remove_all_mappings_for_move_destination_reg(exreg_group_id_t g, reg_identifier_t const &r)
//  {
//    if (!moves.count(g)) {
//      return;
//    }
//    set<reg_identifier_t> to_erase;
//    for (const auto &m : moves.at(g)) {
//      if (m.second.first == r) {
//        to_erase.insert(m.first);
//      }
//    }
//    for (const auto &src_r : to_erase) {
//      moves.at(g).erase(src_r);
//    }
//  }
//  string to_string() const
//  {
//    std::stringstream ss;
//    ss << "(live:" << regset_to_string(&live, as1, sizeof as1) << ", moves: [";
//    for (const auto &g : moves) {
//      for (const auto &m : g.second) {
//        ss << " " << m.first.reg_identifier_to_string() << "->" << m.second.first.reg_identifier_to_string() << endl;
//      }
//    }
//    ss << "])";
//    return ss.str();
//  }
//
//private:
//  static bool moves_equal(map<exreg_group_id_t, map<reg_identifier_t, MOVE_T>>  const &a, map<exreg_group_id_t, map<reg_identifier_t, MOVE_T>> const &b)
//  {
//    for (const auto &g : a) {
//      for (const auto &r : g.second) {
//        if (!b.count(g.first) || !b.at(g.first).count(r.first)) {
//          return false;
//        }
//        if (!move_equal(r.second, b.at(g.first).at(r.first))) {
//          return false;
//        }
//      }
//    }
//    return true;
//  }
//  static bool move_equal(MOVE_T const &a, MOVE_T const &b)
//  {
//    return a.first == b.first && a.second == b.second;
//  }
//};

class must_reach_def_val_t
{
private:
  bool m_is_top;
public:
  map<exreg_group_id_t, map<reg_identifier_t, set<dst_insn_ptr_t>>> m;
  must_reach_def_val_t()
  {
    m_is_top = false;
    //initialize the m with empty sets, to keep it in canonical form always
    for (exreg_group_id_t g = 0; g < DST_NUM_EXREG_GROUPS; g++) {
      for (exreg_id_t r = 0; r < DST_NUM_EXREGS(g); r++) {
        m[g][r] = set<dst_insn_ptr_t>();
      }
    }
  }
  static must_reach_def_val_t top()
  {
    must_reach_def_val_t ret;
    ret.m_is_top = true;
    return ret;
  }
  bool contains(exreg_group_id_t g, reg_identifier_t const &r, dst_insn_ptr_t const &insn) const
  {
    ASSERT(!m_is_top);
    return set_belongs(m.at(g).at(r), insn);
  }
  bool operator!=(must_reach_def_val_t const &other) const
  {
    ASSERT(!m_is_top);
    return !(m == other.m);
  }

  bool meet(must_reach_def_val_t const &other)
  {
    ASSERT(!other.m_is_top);
    if (this->m_is_top) {
      *this = other;
      return true;
    }
    //must_reach_def_val_t ret = *this;
    bool changed = false;
    for (auto &g : this->m) {
      for (auto &r : g.second) {
        auto old_val = r.second;
        set_intersect(r.second, other.m.at(g.first).at(r.first));
        if (r.second != old_val) {
          changed = true;
        }
      }
    }
    return changed;
  }


  string to_string() const
  {
    if (m_is_top) {
      return "TOP";
    }
    stringstream ss;
    ss << "[";
    for (const auto &g : m) {
      for (const auto &r : g.second) {
        ss << ";" << r.first.reg_identifier_to_string() << ":";
        for (const auto &p : r.second) {
          ss << "," << p.to_string();
        }
      }
    }
    ss << "]";
    return ss.str();
  }
};

class must_reach_definitions_dfa_t : public data_flow_analysis<dst_insn_ptr_t, node<dst_insn_ptr_t>, edge<dst_insn_ptr_t>, must_reach_def_val_t, dfa_dir_t::forward>
{
public:
  must_reach_definitions_dfa_t(dshared_ptr<graph_with_regions_t<dst_insn_ptr_t, node<dst_insn_ptr_t>, edge<dst_insn_ptr_t>> const> cfg, map<dst_insn_ptr_t, must_reach_def_val_t> &init_vals) :
                   data_flow_analysis<dst_insn_ptr_t, node<dst_insn_ptr_t>, edge<dst_insn_ptr_t>, must_reach_def_val_t, dfa_dir_t::forward>(cfg, init_vals)
  {
  }
  //must_reach_def_val_t meet(must_reach_def_val_t const &out1, must_reach_def_val_t const &out2)
  //{
  //  must_reach_def_val_t ret = out1;
  //  for (auto &g : ret.m) {
  //    for (auto &r : g.second) {
  //      set_intersect(r.second, out2.m.at(g.first).at(r.first));
  //    }
  //  }
  //  return ret;
  //}
  //must_reach_def_val_t xfer(shared_ptr<edge<dst_insn_ptr_t>> e, must_reach_def_val_t const &out)
  //{
  //  dst_insn_ptr_t const &pc = e->get_from_pc();
  //  dst_insn_t *insn = pc.get_ptr();
  //  regset_t use, def;
  //  dst_insn_get_usedef_regs(insn, &use, &def);
  //  must_reach_def_val_t ret = out;
  //  for (const auto &g : def.excregs) {
  //    for (const auto &r : g.second) {
  //      ASSERT(r.second != 0);
  //      set<dst_insn_ptr_t> s;
  //      s.insert(pc);
  //      ret.m[g.first][r.first] = s;
  //    }
  //  }
  //  return ret;
  //}

  dfa_xfer_retval_t xfer_and_meet(dshared_ptr<edge<dst_insn_ptr_t> const> const &e, must_reach_def_val_t const &in, must_reach_def_val_t &out)
  {
    dst_insn_ptr_t const &pc = e->get_from_pc();
    dst_insn_t *insn = pc.get_ptr();
    regset_t use, def;
    dst_insn_get_usedef_regs(insn, &use, &def);
    must_reach_def_val_t ret = in;
    for (const auto &g : def.excregs) {
      for (const auto &r : g.second) {
        ASSERT(r.second != 0);
        set<dst_insn_ptr_t> s;
        s.insert(pc);
        ret.m[g.first][r.first] = s;
      }
    }
    return out.meet(ret) ? DFA_XFER_RETVAL_CHANGED : DFA_XFER_RETVAL_UNCHANGED;
  }
};

static bool
dst_pcs_belong_to_same_basic_block(dshared_ptr<graph<dst_insn_ptr_t, node<dst_insn_ptr_t>, edge<dst_insn_ptr_t>> const> cfg, dst_insn_ptr_t const &pc1, dst_insn_ptr_t const &pc2)
{
  dst_insn_ptr_t cur_pc = pc1;
  while (!(cur_pc == pc2)) {
    list<dshared_ptr<edge<dst_insn_ptr_t> const>> outgoing, incoming;
    cfg->get_edges_outgoing(cur_pc, outgoing);
    if (outgoing.size() != 1) {
      return false;
    }
    dst_insn_t const *cur_ptr = cur_pc.get_ptr();
    if (dst_insn_is_unconditional_branch_not_fcall(cur_ptr)) {
      return false;
    }
    if (!(cur_pc == pc1)) {
      cfg->get_edges_incoming(cur_pc, incoming);
      if (incoming.size() != 1) {
        return false;
      }
    }
    cur_pc = (*outgoing.begin())->get_to_pc();
  }
  return true;
}

static bool
upward_movement_is_feasible(dshared_ptr<graph<dst_insn_ptr_t, node<dst_insn_ptr_t>, edge<dst_insn_ptr_t>> const> cfg, list<dst_insn_ptr_t> const &need_upward_movement, dst_insn_ptr_t const &pc_to)
{
  for (const auto &i : need_upward_movement) {
    if (!dst_pcs_belong_to_same_basic_block(cfg, pc_to, i)) {
      return false;
    }
  }
  return true;
}

class replacement_option_t
{
private:
  static bool regset_intersection_is_empty(regset_t const &a, regset_t const &b)
  {
    regset_t intersect = a;
    regset_intersect(&intersect, &b);
    return regset_is_empty(&intersect);
  }
public:
  set<dst_insn_ptr_t> insns_that_need_replacement;
  regset_t conflicting_regs_read, conflicting_regs_written;
  bool mem_conflict;
  list<dst_insn_ptr_t> insns_that_conflict; //and need to be moved up for this replacement to be possible

  replacement_option_t() : mem_conflict(false) { }

  //bool operator!=(replacement_option_t const &other) const
  //{
  //  return    insns_that_need_replacement != other.insns_that_need_replacement
  //         || !regset_equal(&conflicting_regs_read, &other.conflicting_regs_read)
  //         || !regset_equal(&conflicting_regs_written, &other.conflicting_regs_written)
  //         || mem_conflict != other.mem_conflict
  //         || insns_that_conflict != other.insns_that_conflict
  //  ;
  //}

  void replacement_option_update_conflict_set(dst_insn_ptr_t const &pc, exreg_group_id_t group, exreg_id_t replacement_reg, regset_t const &use, regset_t const &def, memset_t const &memtouch)
  {
    if (insns_that_need_replacement.size() == 0) {
      ASSERT(regset_is_empty(&conflicting_regs_written));
      //ASSERT(regset_is_single_reg(&conflicting_regs_read));
      return;
    }
    //capture all RAW, WAR and WAW dependencies; also the replacement reg should not belong to use or def
    if (   regset_intersection_is_empty(conflicting_regs_read, def)
        && regset_intersection_is_empty(conflicting_regs_written, use)
        && regset_intersection_is_empty(conflicting_regs_written, def)
        && !regset_belongs_ex(&use, group, replacement_reg)
        && !regset_belongs_ex(&def, group, replacement_reg)
        && (memtouch.memset_is_empty() || !mem_conflict)) {
      return;
    }
    regset_union(&conflicting_regs_read, &use);
    regset_union(&conflicting_regs_written, &def);
    if (!memtouch.memset_is_empty()) {
      mem_conflict = true;
    }
    insns_that_conflict.push_front(pc);
  }

  static replacement_option_t empty()
  {
    replacement_option_t empty_ro;
    empty_ro.insns_that_need_replacement = set<dst_insn_ptr_t>();
    regset_clear(&empty_ro.conflicting_regs_read);
    regset_clear(&empty_ro.conflicting_regs_written);
    empty_ro.mem_conflict = false;
    empty_ro.insns_that_conflict = list<dst_insn_ptr_t>();
    return empty_ro;
  }

  static replacement_option_t empty_with_dst_reg_conflict(exreg_group_id_t group, exreg_id_t dst_reg)
  {
    replacement_option_t empty_ro = replacement_option_t::empty();
    empty_ro.insns_that_need_replacement = set<dst_insn_ptr_t>();
    regset_clear(&empty_ro.conflicting_regs_read);
    regset_clear(&empty_ro.conflicting_regs_written);
    empty_ro.mem_conflict = false;
    regset_mark_ex_mask(&empty_ro.conflicting_regs_read, group, dst_reg, MAKE_MASK(DST_EXREG_LEN(group)));
    empty_ro.insns_that_conflict = list<dst_insn_ptr_t>();
    return empty_ro;
  }


  bool equals(replacement_option_t const &other) const
  {
    if (insns_that_need_replacement != other.insns_that_need_replacement) {
      //cout << "replacement_option_t::" << __func__ << " " << __LINE__ << ": returning false\n";
      return false;
    }
    if (!regset_equal(&conflicting_regs_read, &other.conflicting_regs_read)) {
      //cout << "replacement_option_t::" << __func__ << " " << __LINE__ << ": returning false\n";
      return false;
    }
    if (!regset_equal(&conflicting_regs_written, &other.conflicting_regs_written)) {
      //cout << "replacement_option_t::" << __func__ << " " << __LINE__ << ": returning false\n";
      return false;
    }
    if (mem_conflict != other.mem_conflict) {
      //cout << "replacement_option_t::" << __func__ << " " << __LINE__ << ": returning false\n";
      return false;
    }
    if (insns_that_conflict != other.insns_that_conflict) {
      //cout << "replacement_option_t::" << __func__ << " " << __LINE__ << ": returning false\n";
      return false;
    }
    return true;
  }

  string replacement_option_to_string() const
  {
    stringstream ss;
    ss << "conflicting regs: (read " << regset_to_string(&conflicting_regs_read, as1, sizeof as1);
    ss << ", written " << regset_to_string(&conflicting_regs_written, as1, sizeof as1) << ")";
    if (mem_conflict) {
      ss << ", mem_conflict";
    }
    ss << "; conflicting insns: {";
    for (const auto &p : insns_that_conflict) {
      ss << " " << p.to_string();
    }
    ss << "}; need replacement insns: {";
    for (const auto &p : insns_that_need_replacement) {
      ss << " " << p.to_string();
    }
    ss << "}";
    return ss.str();
  }
};

class replacement_options_t
{
private:
  map<exreg_id_t, pair<bool, replacement_option_t>> m_map; //bool: whether replacement is possible; set: set of dst insns that need to be renamed if replacement is performed
public:

  //bool operator!=(replacement_options_t const &other) const
  //{
  //  if (m_map.size() != other.m_map.size()) {
  //    cout << "replacement_options_t::" << __func__ << " " << __LINE__ << ": returning true because sizes mismatch\n";
  //    return true;
  //  }
  //  for (const auto &er : m_map) {
  //    if (other.m_map.count(er.first) == 0) {
  //      cout << "replacement_options_t::" << __func__ << " " << __LINE__ << ": returning true because of reg " << er.first << "\n";
  //      return true;
  //    }
  //    if (other.m_map.at(er.first).first != er.second.first) {
  //      cout << "replacement_options_t::" << __func__ << " " << __LINE__ << ": returning true because of reg " << er.first << "\n";
  //      return true;
  //    }
  //    if (other.m_map.at(er.first).second != er.second.second) {
  //      cout << "replacement_options_t::" << __func__ << " " << __LINE__ << ": returning true because of reg " << er.first << "\n";
  //      return true;
  //    }
  //  }
  //  return false;
  //}

  map<exreg_id_t, pair<bool, replacement_option_t>> const &get_map()
  {
    return m_map;
  }
  void update_conflict_set(dst_insn_ptr_t const &pc, exreg_group_id_t group, regset_t const &use, regset_t const &def, memset_t const &memtouch)
  {
    for (auto &r : m_map) {
      if (!r.second.first) {
        continue;
      }
      r.second.second.replacement_option_update_conflict_set(pc, group, r.first, use, def, memtouch);
    }
  }
  //void add_pc_that_needs_replacement(exreg_id_t r, dst_insn_ptr_t const &pc)
  //{
  //  if (m_map.count(r) && m_map.at(r).first) {
  //    m_map.at(r).second.insns_that_need_replacement.insert(pc);
  //  } else {
  //    m_map.erase(r); //erase if exists
  //    set<dst_insn_ptr_t> s;
  //    s.insert(pc);
  //    m_map.insert(make_pair(r, make_pair(true, s)));
  //  }
  //}

  static cost_t compute_benefit(set<dst_insn_ptr_t> const &renamed_insns, flow_t const *estimated_flow, exreg_group_id_t group, exreg_id_t from_reg, exreg_id_t to_reg)
  {
    cost_t benefit = 0;
    for (const auto &ri : renamed_insns) {
      exreg_group_id_t g;
      exreg_id_t r1, r2;
      bool is_move = dst_insn_is_move_insn(ri.get_ptr(), g, r1, r2);
      if (   is_move
          && g == group
          && r1 == from_reg
          && r2 == to_reg) {
        long inum = ri.get_inum();
        benefit += estimated_flow[inum];
      }
    }
    return benefit;
  }

  static bool pc_belongs_to_must_reach_set_for_all_renamed_insns(dst_insn_ptr_t const &pc, exreg_group_id_t group, exreg_id_t reg, set<dst_insn_ptr_t> const &renamed_insns, map<dst_insn_ptr_t, must_reach_def_val_t> const &mrdefs)
  {
    for (const auto &ri : renamed_insns) {
      if (ri == pc) {
        continue;
      }
      if (mrdefs.count(ri) == 0) {
        //cout << __func__ << " " << __LINE__ << ": ri = " << ri.to_string() << ", pc = " << pc.to_string() << endl;
        return false;
      }
      must_reach_def_val_t const &mr = mrdefs.at(ri);
      if (!mr.contains(group, reg, pc)) {
        return false;
      }
    }
    return true;
  }
  cost_t pick_best_replacement_option(dshared_ptr<graph<dst_insn_ptr_t, node<dst_insn_ptr_t>, edge<dst_insn_ptr_t>> const> cfg, dst_insn_ptr_t const &pc, regset_t const &use, exreg_group_id_t group, exreg_id_t reg, map<dst_insn_ptr_t, must_reach_def_val_t> const &mrdefs, flow_t const *estimated_flow, exreg_id_t &replacement_reg, replacement_option_t &replacement_option) const
  {
    replacement_reg = reg; //default is to not replace
    dst_insn_t *ptr = pc.get_ptr();
    if (!dst_insn_reg_def_is_replaceable(ptr, group, reg)) {
      CPP_DBG_EXEC(DST_INSN_FOLDING, cout << __func__ << " " << __LINE__ << ": returning 0 because !dst_insn_reg_def_is_replaceable()\n");
      return 0;
    }
    //regset_t use, def, touch;
    //dst_insn_get_usedef_regs(ptr, &use, &def);
    //touch = use;
    //regset_union(&touch, &def);
    cost_t max_benefit = 0;
    for (const auto &r : m_map) {
      if (!r.second.first) {
        continue;
      }
      CPP_DBG_EXEC(DST_INSN_FOLDING, cout << __func__ << " " << __LINE__ << ": replacement candidate dst reg " << r.first << "\n");
      regset_t intersect = use;
      regset_intersect(&intersect, &r.second.second.conflicting_regs_written);
      if (!regset_is_empty(&intersect)) {
        CPP_DBG_EXEC(DST_INSN_FOLDING, cout << __func__ << " " << __LINE__ << ": replacement candidate dst reg " << r.first << ": returning false\n");
        continue;
      }
      if (!pc_belongs_to_must_reach_set_for_all_renamed_insns(pc, group, reg, r.second.second.insns_that_need_replacement, mrdefs)) {
        CPP_DBG_EXEC(DST_INSN_FOLDING, cout << __func__ << " " << __LINE__ << ": replacement candidate dst reg " << r.first << ": returning false\n");
        continue;
      }
      if (!upward_movement_is_feasible(cfg, r.second.second.insns_that_conflict, pc)) {
        CPP_DBG_EXEC(DST_INSN_FOLDING, cout << __func__ << " " << __LINE__ << ": replacement candidate dst reg " << r.first << ": returning false\n");
        continue;
      }
      cost_t cur_benefit = compute_benefit(r.second.second.insns_that_need_replacement, estimated_flow, group, reg, r.first);
      if (max_benefit >= cur_benefit) {
        CPP_DBG_EXEC(DST_INSN_FOLDING, cout << __func__ << " " << __LINE__ << ": replacement candidate dst reg " << r.first << ": returning false, cur_benefit = " << cur_benefit << ", max_benefit = " << max_benefit << "\n");
        continue;
      }
      max_benefit = cur_benefit;
      replacement_reg = r.first;
      replacement_option = r.second.second;
    }
    return max_benefit;
  }
  string replacement_options_to_string() const
  {
    stringstream ss;
    ss << "(";
    for (const auto &r : m_map) {
      ss << " " << r.first << ":";
      if (r.second.first) {
        ss << r.second.second.replacement_option_to_string();
      } else {
        ss << "CANNOT_REPLACE";
      }
    }
    ss << ")";
    return ss.str();
  }
  bool equals(replacement_options_t const &other) const
  {
    if (m_map.size() != other.m_map.size()) {
      //cout << "replacement_options_t::" << __func__ << " " << __LINE__ << ": returning false\n";
      return false;
    }
    for (const auto &r : m_map) {
      if (other.m_map.count(r.first) == 0) {
        //cout << "replacement_options_t::" << __func__ << " " << __LINE__ << ": returning false\n";
        return false;
      }
      if (m_map.at(r.first).first != other.m_map.at(r.first).first) {
        //cout << "replacement_options_t::" << __func__ << " " << __LINE__ << ": returning false\n";
        return false;
      }
      if (!m_map.at(r.first).second.equals(other.m_map.at(r.first).second)) {
        //cout << "replacement_options_t::" << __func__ << " " << __LINE__ << ": returning false\n";
        return false;
      }
      //for (const auto &d : m_map.at(r.first).second) {
      //  if (!other.m_map.at(r.first).second.count(d)) {
      //    return false;
      //  }
      //}
    }
    return true;
  }
  replacement_options_t meet(replacement_options_t const &other) const
  {
    replacement_options_t ret;
    ASSERT(m_map.size() == other.m_map.size());
    for (const auto &r : m_map) {
      ASSERT(other.m_map.count(r.first));
      if (!r.second.first || !other.m_map.at(r.first).first) {
        ret.m_map[r.first] = make_pair(false, replacement_option_t());
      } else if (r.second.second.insns_that_conflict != other.m_map.at(r.first).second.insns_that_conflict) {
        ret.m_map[r.first] = make_pair(false, replacement_option_t());
      } else {
        ASSERT(regset_equal(&r.second.second.conflicting_regs_read, &other.m_map.at(r.first).second.conflicting_regs_read));
        ASSERT(regset_equal(&r.second.second.conflicting_regs_written, &other.m_map.at(r.first).second.conflicting_regs_written));
        set<dst_insn_ptr_t> s = r.second.second.insns_that_need_replacement;
        set_union(s, other.m_map.at(r.first).second.insns_that_need_replacement);
        replacement_option_t ro;
        ro.insns_that_need_replacement = s;
        ro.conflicting_regs_read = r.second.second.conflicting_regs_read;
        ro.conflicting_regs_written = r.second.second.conflicting_regs_written;
        ro.insns_that_conflict = r.second.second.insns_that_conflict;
        ret.m_map[r.first] = make_pair(true, ro);
      }
    }
    return ret;
  }
  static replacement_options_t replaceable_options_initial_value_at_exit(exreg_group_id_t g, exreg_id_t r)
  {
    replacement_options_t ret;
    for (exreg_id_t nr = 0; nr < DST_NUM_EXREGS(g); nr++) {
      if (nr != r) {
        ret.m_map[nr] = make_pair(true, replacement_option_t::empty_with_dst_reg_conflict(g, nr));
      }
    }
    return ret;
  }
  void set_to_empty_for_all_dst_regs(exreg_group_id_t group)
  {
    for (auto &m : m_map) {
      m.second = make_pair(true, replacement_option_t::empty_with_dst_reg_conflict(group, m.first));
    }
  }

  void set_to_false_for_all_dst_regs()
  {
    for (auto &m : m_map) {
      m.second = make_pair(false, replacement_option_t::empty());
    }
  }
  //void set_to_false_for_dst_reg_if_nonempty(exreg_id_t dst_reg)
  //{
  //  if (   m_map.count(dst_reg)
  //      && m_map.at(dst_reg).first
  //      && m_map.at(dst_reg).second.size() > 0) {
  //    m_map.at(dst_reg) = make_pair(false, set<dst_insn_ptr_t>());
  //  }
  //}
  void add_to_dst_insn_set_if_dst_reg_is_dead(dst_insn_ptr_t const &pc, regset_t const &dead_regs, exreg_group_id_t group)
  {
    for (auto &p : m_map) {
      if (   p.second.first
          && regset_belongs_ex(&dead_regs, group, p.first)
          && !list_contains(p.second.second.insns_that_conflict, pc)) {
        p.second.second.insns_that_need_replacement.insert(pc);
      } else {
        p.second.first = false; //if dst_reg is live, this replacement is not feasible
      }
    }
  }
};

class replaceable_pairs_t
{
private:
  map<exreg_group_id_t, map<exreg_id_t, replacement_options_t>> m_replacement_options;
  bool m_is_top;
public:
  regset_t m_dead_regs;

  replaceable_pairs_t() : m_is_top(false) {}

  static replaceable_pairs_t top()
  {
    replaceable_pairs_t ret;
    ret.m_is_top = true;
    return ret;
  }

  map<exreg_group_id_t, map<exreg_id_t, replacement_options_t>> const &get_replacement_options() const
  {
    ASSERT(!m_is_top);
    return m_replacement_options;
  }
  string to_string() const
  {
    if (m_is_top) {
      return "TOP";
    }
    stringstream ss;
    for (const auto &g : m_replacement_options) {
      for (const auto &r : g.second) {
        ss << "(" << g.first << "," << r.first << "): " << r.second.replacement_options_to_string() << endl;
      }
    }
    ss << "dead regs: " << regset_to_string(&m_dead_regs, as1, sizeof as1) << endl;
    return ss.str();
  }
  bool operator!=(replaceable_pairs_t const &other) const
  {
    ASSERT(!m_is_top);
    ASSERT(!other.m_is_top);
    if (m_replacement_options.size() != other.m_replacement_options.size()) {
      //cout << __func__ << " " << __LINE__ << ": operator!= returning true\n";
      return true;
    }
    for (const auto &g : m_replacement_options) {
      if (other.m_replacement_options.count(g.first) == 0) {
        //cout << __func__ << " " << __LINE__ << ": operator!= returning true\n";
        return true;
      }
      if (g.second.size() != other.m_replacement_options.at(g.first).size()) {
        //cout << __func__ << " " << __LINE__ << ": operator!= returning true\n";
        return true;
      }
      for (const auto &r : g.second) {
        if (other.m_replacement_options.at(g.first).count(r.first) == 0) {
          //cout << __func__ << " " << __LINE__ << ": operator!= returning true\n";
          return true;
        }
        replacement_options_t const &opts = r.second;
        replacement_options_t const &other_opts = other.m_replacement_options.at(g.first).at(r.first);
        if (!opts.equals(other_opts)) {
          //cout << __func__ << " " << __LINE__ << ": operator!= returning true for group " << g.first << ", reg " << r.first << "\n";
          return true;
        }
      }
    }
    if (!regset_equal(&m_dead_regs, &other.m_dead_regs)) {
      //cout << __func__ << " " << __LINE__ << ": operator!= returning true\n";
      return true;
    }
    //cout << __func__ << " " << __LINE__ << ": operator!= returning false\n";
    return false;
  }
  bool meet(replaceable_pairs_t const &other)
  {
    ASSERT(!other.m_is_top);
    if (this->m_is_top) {
      *this = other;
      return true;
    }
    bool changed = false;
    //replaceable_pairs_t ret;
    ASSERT(m_replacement_options.size() == other.m_replacement_options.size());
    //auto orig_val = *this;
    for (const auto &g : m_replacement_options) {
      ASSERT(other.m_replacement_options.count(g.first));
      ASSERT(other.m_replacement_options.at(g.first).size() == g.second.size());
      for (const auto &r : g.second) {
        ASSERT(other.m_replacement_options.at(g.first).count(r.first));
        //ret.m_replacement_options[g.first][r.first] = m_replacement_options.at(g.first).at(r.first).meet(other.m_replacement_options.at(g.first).at(r.first));
        m_replacement_options.at(g.first).at(r.first) = m_replacement_options.at(g.first).at(r.first).meet(other.m_replacement_options.at(g.first).at(r.first));
        auto orig_val = m_replacement_options.at(g.first).at(r.first);
        if (!orig_val.equals(m_replacement_options.at(g.first).at(r.first))) {
          //cout << __func__ << " " << __LINE__ << ": setting changed = true due to group " << g.first << ", reg " << r.first << "\n"; 
          //cout << __func__ << " " << __LINE__ << ": orig value = " << orig_val.replacement_options_to_string() << endl;
          //cout << __func__ << " " << __LINE__ << ": new value = " << m_replacement_options.at(g.first).at(r.first).replacement_options_to_string() << endl;
          changed = true;
        }
      }
    }
    //ret.m_dead_regs = m_dead_regs;
    //regset_intersect(&ret.m_dead_regs, &other.m_dead_regs);
    auto orig_val = m_dead_regs;
    regset_intersect(&m_dead_regs, &other.m_dead_regs);
    if (orig_val != m_dead_regs) {
      changed = true;
      //cout << __func__ << " " << __LINE__ << ": setting changed = true due to dead regs\n";
      //cout << __func__ << " " << __LINE__ << ": orig value = " << regset_to_string(&orig_val, as1, sizeof as1) << endl;
      //cout << __func__ << " " << __LINE__ << ": new value = " << regset_to_string(&m_dead_regs, as1, sizeof as1) << endl;
    }
    //return *this != orig_val;
    return changed;
  }
  static replaceable_pairs_t replaceable_pairs_initial_value_at_exit()
  {
    replaceable_pairs_t ret;
    for (exreg_group_id_t g = 0; g < DST_NUM_EXREG_GROUPS; g++) {
      for (exreg_id_t r = 0; r < DST_NUM_EXREGS(g); r++) {
        ret.m_replacement_options[g][r] = replacement_options_t::replaceable_options_initial_value_at_exit(g, r);
        regset_mark_ex_mask(&ret.m_dead_regs, g, r, MAKE_MASK(DST_EXREG_LEN(g)));
      }
    }
    return ret;
  }
  void update_dead_regs(regset_t const &use, regset_t const &def)
  {
    regset_union(&m_dead_regs, &def);
    regset_diff(&m_dead_regs, &use);
  }
  bool reg_is_dead(exreg_group_id_t group, exreg_id_t r) const
  {
    return regset_belongs_ex(&m_dead_regs, group, r);
  }
  void set_replacement_options_for_src_reg_to_empty_set_for_all_dead_dst_regs(exreg_group_id_t group, exreg_id_t reg)
  {
    m_replacement_options[group][reg].set_to_empty_for_all_dst_regs(group);
  }
  void set_replacement_options_for_src_reg_to_false(exreg_group_id_t group, exreg_id_t reg)
  {
    m_replacement_options[group][reg].set_to_false_for_all_dst_regs();
  }

  void update_conflict_set_for_all_pairs(dst_insn_ptr_t const &pc, regset_t const &use, regset_t const &def, memset_t const &memtouch)
  {
    for (auto &src_g : m_replacement_options) {
      for (auto &src_r : src_g.second) {
        src_r.second.update_conflict_set(pc, src_g.first, use, def, memtouch);
      }
    }
  }
  void add_to_dst_insn_set_for_src_reg(exreg_group_id_t group, exreg_id_t reg, dst_insn_ptr_t const &pc)
  {
    m_replacement_options[group][reg].add_to_dst_insn_set_if_dst_reg_is_dead(pc, m_dead_regs, group);
  }
};

class regname_replaceable_pairs_dfa_t : public data_flow_analysis<dst_insn_ptr_t, node<dst_insn_ptr_t>, edge<dst_insn_ptr_t>, replaceable_pairs_t, dfa_dir_t::backward>
{
public:
  regname_replaceable_pairs_dfa_t(dshared_ptr<graph_with_regions_t<dst_insn_ptr_t, node<dst_insn_ptr_t>, edge<dst_insn_ptr_t>> const> cfg, map<dst_insn_ptr_t, replaceable_pairs_t> &init_vals) :
                   data_flow_analysis<dst_insn_ptr_t, node<dst_insn_ptr_t>, edge<dst_insn_ptr_t>, replaceable_pairs_t, dfa_dir_t::backward>(cfg, init_vals/*replaceable_pairs_t::replaceable_pairs_initial_value_at_exit()*/)
  {
  }
  //replaceable_pairs_t meet(replaceable_pairs_t const &out1, replaceable_pairs_t const &out2)
  //{
  //  return out1.meet(out2);
  //}
  dfa_xfer_retval_t xfer_and_meet(dshared_ptr<edge<dst_insn_ptr_t> const> const &e, replaceable_pairs_t const &in, replaceable_pairs_t &out)
  {
    dst_insn_ptr_t const &from_pc = e->get_from_pc();
    dst_insn_ptr_t const &to_pc = e->get_to_pc();
    //cout << __func__ << " " << __LINE__ << ": from_pc = " << from_pc.to_string() << " -> to_pc " << to_pc.to_string() << endl;
    dst_insn_t const *from_insn = from_pc.get_ptr();
    regset_t use, def/*, touch*/;
    memset_t memuse, memdef;
    dst_insn_get_usedef(from_insn, &use, &def, &memuse, &memdef);
    memset_union(&memuse, &memdef);
    //touch = def;
    //regset_union(&touch, &use);

    //cout << __func__ << " " << __LINE__ << ": from_pc " << from_pc.to_string() << ", in = " << in.to_string() << endl;
    replaceable_pairs_t ret = in;
    //cout << __func__ << " " << __LINE__ << ": from_pc " << from_pc.to_string() << ", ret = " << ret.to_string() << endl;
    //cout << __func__ << " " << __LINE__ << ": from_pc " << from_pc.to_string() << ", ret = " << ret.to_string() << endl;

    ret.update_dead_regs(use, def);
    //cout << __func__ << " " << __LINE__ << ": from_pc " << from_pc.to_string() << ", ret = " << ret.to_string() << endl;
    //cout << __func__ << " " << __LINE__ << ": from_pc " << from_pc.to_string() << ", use = " << regset_to_string(&use, as1, sizeof as1) << endl;
    //cout << __func__ << " " << __LINE__ << ": from_pc " << from_pc.to_string() << ", def = " << regset_to_string(&def, as1, sizeof as1) << endl;
    //cout << __func__ << " " << __LINE__ << ": from_pc " << from_pc.to_string() << ", ret.m_dead_regs  = " << regset_to_string(&ret.m_dead_regs, as1, sizeof as1) << endl;
    //if dst_reg is touched, set the corresponding replacement option to <FALSE, emptyset> for all src_regs
    ret.update_conflict_set_for_all_pairs(from_pc, use, def, memuse);
    //cout << __func__ << " " << __LINE__ << ": from_pc " << from_pc.to_string() << ", ret = " << ret.to_string() << endl;
    //cout << __func__ << " " << __LINE__ << ": from_pc " << from_pc.to_string() << ", use = " << regset_to_string(&use, as1, sizeof as1) << endl;
    //cout << __func__ << " " << __LINE__ << ": from_pc " << from_pc.to_string() << ", def = " << regset_to_string(&def, as1, sizeof as1) << endl;
    //cout << __func__ << " " << __LINE__ << ": from_pc " << from_pc.to_string() << ", touch = " << regset_to_string(&touch, as1, sizeof as1) << endl;
    //cout << __func__ << " " << __LINE__ << ": from_pc " << from_pc.to_string() << ", ret = " << ret.to_string() << endl;

    //if src_reg is touched (and maybe defed); for all dst_regs where the replacement option is <TRUE, set>, insert the current instruction in set
    for (const auto &g : use.excregs) {
      for (const auto &r : g.second) {
        if (dst_insn_reg_is_replaceable(from_insn, g.first, r.first.reg_identifier_get_id())) {
          ret.add_to_dst_insn_set_for_src_reg(g.first, r.first.reg_identifier_get_id(), e->get_from_pc());
        } else {
          //cout << __func__ << " " << __LINE__ << ": setting to false for group " << g.first << " reg " << r.first.reg_identifier_to_string() << endl;
          ret.set_replacement_options_for_src_reg_to_false(g.first, r.first.reg_identifier_get_id());
        }
      }
    }
    //cout << __func__ << " " << __LINE__ << ": from_pc " << from_pc.to_string() << ", ret = " << ret.to_string() << endl;

    for (const auto &g : def.excregs) {
      for (const auto &r : g.second) {
        if (ret.reg_is_dead(g.first, r.first.reg_identifier_get_id())) {
          //if both src/dst are dead at this point, we can replace either with the other one
          ret.set_replacement_options_for_src_reg_to_empty_set_for_all_dead_dst_regs(g.first, r.first.reg_identifier_get_id());
        }
      }
    }


    ////if dst_reg is defed but not used; change its replacement options from <FALSE, emptyset> to <TRUE, emptyset> for dead src registers; this can happen if the reg appeared in a later instruction where it could not be replaced; but now it can be replaced because it is starting a new life (just got def'ed/killed)
    //for (const auto &g : def.excregs) {
    //  for (const auto &r : g.second) {
    //    if (r.second == MAKE_MASK(DST_EXREG_LEN(g.first)) && !regset_belongs_ex(&use, g.first, r.first)) {
    //      //if both src/dst are dead at this point, we can replace either with the other one
    //      ret.set_replacement_options_for_dst_reg_to_empty_set_for_all_dead_dst_regs(g.first, r.first.reg_identifier_get_id());
    //      regset_mark_ex_mask(&ret.m_dead_regs, g.first, r.first, MAKE_MASK(DST_EXREG_LEN(g.first)));
    //    }
    //  }
    //}
    //for (const auto &g : use.excregs) {
    //  for (const auto &r : g.second) {
    //    regset_unmark_ex(&ret.m_dead_regs, g.first, r.first);
    //  }
    //}
    //cout << __func__ << " " << __LINE__ << ": from_pc " << from_pc.to_string() << ", out = " << out.to_string() << endl;
    //cout << __func__ << " " << __LINE__ << ": from_pc " << from_pc.to_string() << ", ret = " << ret.to_string() << endl;

    bool changed = out.meet(ret);
    //cout << __func__ << " " << __LINE__ << ": from_pc " << from_pc.to_string() << ", after meet, out = " << out.to_string() << endl;
    //cout << __func__ << " " << __LINE__ << ": from_pc " << from_pc.to_string() << ": changed = " << changed << endl;
    return changed ? DFA_XFER_RETVAL_CHANGED : DFA_XFER_RETVAL_UNCHANGED;
  }
};

//static string
//dst_iseq_cfg_to_string(graph<dst_insn_ptr_t, node<dst_insn_ptr_t>, edge<dst_insn_ptr_t>> const &g)
//{
//  
//}

static void
dst_insns_move_upwards(dshared_ptr<graph<dst_insn_ptr_t, node<dst_insn_ptr_t>, edge<dst_insn_ptr_t>> const> cfg, list<dst_insn_ptr_t> const &need_upward_movement, dst_insn_ptr_t const &pc_to, dst_insn_t const *dst_iseq, size_t dst_iseq_len)
{
  if (need_upward_movement.size() == 0) {
    return;
  }
  dst_insn_ptr_t cur_pc = pc_to;
  dst_insn_ptr_t const &last_pc = *need_upward_movement.rbegin();
  list<dst_insn_ptr_t> new_order = need_upward_movement;
  while (!(cur_pc == last_pc)) {
    if (!list_contains(new_order, cur_pc)) {
      new_order.push_back(cur_pc);
    }
    list<dshared_ptr<edge<dst_insn_ptr_t> const>> outgoing;
    cfg->get_edges_outgoing(cur_pc, outgoing);
    ASSERT(outgoing.size() == 1);
    cur_pc = (*outgoing.begin())->get_to_pc();
  }
  CPP_DBG_EXEC(DST_INSN_FOLDING,
    cout << __func__ << " " << __LINE__ << ": printing new order:";
    for (auto no : new_order) {
      cout << " " << no.to_string() << endl;
    }
  );
  dst_insn_t *ptr = pc_to.get_ptr();
  ASSERT((ptr - dst_iseq) < dst_iseq_len);
  ASSERT((ptr - dst_iseq) + new_order.size() <= dst_iseq_len);
  dst_insn_t dst_iseq_cp[new_order.size()];
  size_t i = 0;
  for (auto iter = new_order.begin(); iter != new_order.end(); iter++) {
    dst_insn_copy(&dst_iseq_cp[i], iter->get_ptr());
    i++;
  }
  dst_iseq_copy(ptr, dst_iseq_cp, new_order.size());
}

static void
dst_iseq_execute_replacement(dshared_ptr<graph<dst_insn_ptr_t, node<dst_insn_ptr_t>, edge<dst_insn_ptr_t>> const> cfg, dst_insn_t *dst_iseq, size_t dst_iseq_len, pair<dst_insn_ptr_t, pair<exreg_group_id_t, exreg_id_t>> const &replacement_candidate, exreg_id_t replacement_reg, replacement_option_t const &replacement)
{
  dst_insn_ptr_t const &pc = replacement_candidate.first;
  dst_insn_t *dst_iseq_ptr = pc.get_ptr();
  exreg_group_id_t group = replacement_candidate.second.first;
  exreg_id_t from_reg = replacement_candidate.second.second;
  exreg_id_t to_reg = replacement_reg;
  set<dst_insn_ptr_t> const &need_renaming = replacement.insns_that_need_replacement;

  //do renaming before upward movement; as upward movement can invalidate pointers
  regmap_t regmap;
  regmap.add_mapping(group, from_reg, to_reg);
  CPP_DBG_EXEC(DST_INSN_FOLDING, cout << __func__ << " " << __LINE__ << ": pc " << replacement_candidate.first.to_string() << " from_reg " << from_reg << " to_reg " << to_reg << " regmap:\n" << regmap_to_string(&regmap, as1, sizeof as1) << endl);
  //cout << __func__ << " " << __LINE__ << ": dst_iseq_ptr = " << dst_insn_to_string(dst_iseq_ptr, as1, sizeof as1) << endl;
  dst_insn_rename_dst_reg(dst_iseq_ptr, group, from_reg, to_reg); //rename only dst reg; not all regs
  for (auto &d : need_renaming) {
    dst_insn_t *dptr = d.get_ptr();
    dst_insn_rename_regs(dptr, &regmap); //rename all regs
  }

  list<dst_insn_ptr_t> const &need_upward_movement = replacement.insns_that_conflict;
  CPP_DBG_EXEC(DST_INSN_FOLDING, cout << __func__ << " " << __LINE__ << ": before upward movement: dst_iseq =\n" << dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1) << endl);
  dst_insns_move_upwards(cfg, need_upward_movement, pc, dst_iseq, dst_iseq_len);
  CPP_DBG_EXEC(DST_INSN_FOLDING, cout << __func__ << " " << __LINE__ << ": after upward movement: dst_iseq =\n" << dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1) << endl);

}

static size_t
dst_insns_eliminate_redundant_moves(dst_insn_t *dst_iseq, size_t dst_iseq_len, size_t dst_iseq_size, flow_t const *estimated_flow)
{
  cost_t max_replacement_benefit;
  do {
    CPP_DBG_EXEC(DST_INSN_FOLDING, cout << __func__ << " " << __LINE__ << ": dst_iseq =\n" << dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1) << endl);
    auto cfg = dst_iseq_get_cfg(dst_iseq, dst_iseq_len);
    //cout << __func__ << " " << __LINE__ << ": dst_iseq_cfg =\n" << dst_iseq_cfg_to_string(cfg) << endl;
    map<dst_insn_ptr_t, replaceable_pairs_t> vals;
    regname_replaceable_pairs_dfa_t replaceable_dfa(cfg, vals);
    CPP_DBG_EXEC(DST_INSN_FOLDING, cout << "Solving replaceable_dfa.\n");
    replaceable_dfa.initialize(replaceable_pairs_t::replaceable_pairs_initial_value_at_exit());
    replaceable_dfa.solve();

    map<dst_insn_ptr_t, must_reach_def_val_t> mvals;
    must_reach_definitions_dfa_t mrdefs_dfa(cfg, mvals);
    //cout << "Solving must reach definitions dfa.\n";
    mrdefs_dfa.initialize(must_reach_def_val_t());
    mrdefs_dfa.solve();
    //cout << "done Solving must reach definitions dfa.\n";
    map<dst_insn_ptr_t, replaceable_pairs_t> const &soln = vals; //replaceable_dfa.get_values();
    CPP_DBG_EXEC(DST_INSN_FOLDING,
      for (const auto &pr : soln) {
        cout << __func__ << " " << __LINE__ << ": printing replaceable pairs at " << pr.first.to_string() << endl << pr.second.to_string() << endl;
      }
    );
    map<dst_insn_ptr_t, must_reach_def_val_t> const &mrdefs = mvals; //mrdefs_dfa.get_values();
    max_replacement_benefit = 0;
    pair<dst_insn_ptr_t, pair<exreg_group_id_t, exreg_id_t>> best_replacement_candidate;
    replacement_option_t best_replacement;
    exreg_id_t best_replacement_reg;
    for (const auto &s : soln) {
      dst_insn_ptr_t const &pc = s.first;
      if (pc.is_exit()) {
        continue;
      }
      //dst_insn_t *ptr = pc.get_ptr();
      //ASSERT(ptr);
      list<dshared_ptr<edge<dst_insn_ptr_t> const>> incoming;
      cfg->get_edges_incoming(pc, incoming);
      if (incoming.size() != 1) {
        continue;
      }
      dst_insn_ptr_t const &pred_pc = (*incoming.begin())->get_from_pc();
      dst_insn_t *pred_ptr = pred_pc.get_ptr();
      ASSERT(pred_ptr);
      regset_t use, def, touch;
      dst_insn_get_usedef_regs(pred_ptr, &use, &def);
      touch = use;
      regset_union(&touch, &def);
      map<exreg_group_id_t, map<exreg_id_t, replacement_options_t>> const &moves = s.second.get_replacement_options();
      for (const auto &g : moves) {
        for (const auto &r : g.second) {
          if (!regset_belongs_ex(&def, g.first, r.first)) {
            continue;
          }
          cost_t replacement_benefit;
          exreg_id_t new_r;
          replacement_option_t pc_best_opt;
          CPP_DBG_EXEC(DST_INSN_FOLDING, cout << __func__ << " " << __LINE__ << ": calling pick best replacement option for pred_pc " << pred_pc.to_string() << ", pc " << pc.to_string() << endl);
          replacement_benefit = moves.at(g.first).at(r.first).pick_best_replacement_option(cfg, pred_pc, use, g.first, r.first, mrdefs, estimated_flow, new_r, pc_best_opt);
          CPP_DBG_EXEC(DST_INSN_FOLDING, cout << __func__ << " " << __LINE__ << ": best replacement option for pred_pc " << pred_pc.to_string() << ", pc " << pc.to_string() << " has cost " << replacement_benefit << endl);
          //cost_t replacement_benefit = pc_best_opt.get_benefit(pc, g.first, r.first);
          if (replacement_benefit > max_replacement_benefit) {
            best_replacement_candidate = make_pair(pred_pc, make_pair(g.first, r.first));
            best_replacement = pc_best_opt;
            best_replacement_reg = new_r;
            max_replacement_benefit = replacement_benefit;
          }
        }
      }
    }
    if (max_replacement_benefit > 0) {
      CPP_DBG_EXEC(DST_INSN_FOLDING, cout << __func__ << " " << __LINE__ << ": best replacement candidate = pc " << best_replacement_candidate.first.to_string() << ": group " << best_replacement_candidate.second.first << " reg " << best_replacement_candidate.second.second << ": replacement reg: " << best_replacement_reg << ": replacement " << best_replacement.replacement_option_to_string() << endl);
      dst_iseq_execute_replacement(cfg, dst_iseq, dst_iseq_len, best_replacement_candidate, best_replacement_reg, best_replacement);
    }
  } while (max_replacement_benefit > 0);
        //reg_identifier_t const &new_r = r.second.first;
        //dst_insn_ptr_t const &move_pc = r.second.second;
        //cout << __func__ << " " << __LINE__ << ": reg_used_for_move_only for dst inum " << (ptr - dst_iseq) << ": group " << g.first << ": r " << r.first.reg_identifier_to_string() << ": new_r " << new_r.reg_identifier_to_string() << ": move dst inum " << (move_pc.get_ptr() - dst_iseq) << endl;
        //if (!mrdefs.at(move_pc).contains(g.first, r.first, ptr)) { //if this pc is not the only definition that reaches the move instruction, ignore
        //  continue;
        //}
        //dst_insn_t *move_ptr = move_pc.get_ptr();
        //if (dst_insn_rename_defed_reg(ptr, g.first, r.first, new_r)) {
        //  //if rename succeeded
        //  long ret = dst_insn_put_nop(move_ptr, 1);
        //  ASSERT(ret == 1);
        //}
  return dst_iseq_len;
}

static vector<valtag_t>
dst_insn_get_successors(dst_insn_t const *insn, long idx, size_t iseq_len)
{
  ASSERT(idx >= 0 && idx < iseq_len);
  vector<valtag_t> pcrels = dst_insn_get_pcrel_values(insn);
  vector<valtag_t> ret;
  valtag_t fallthrough_vt;
  fallthrough_vt.val = idx + 1;
  fallthrough_vt.tag = tag_abs_inum;
  if (dst_insn_is_function_return(insn)) {
    return ret;
  }
  if (pcrels.size() == 0 || dst_insn_is_function_call(insn)) {
    if (fallthrough_vt.val < iseq_len) {
      //cout << __func__ << " " << __LINE__ << ": idx = " << idx << ", fallthrough_vt.val = " << fallthrough_vt.val << ", iseq_len = " << iseq_len << ", insn = " << dst_insn_to_string(insn, as1, sizeof as1) << endl;
      ret.push_back(fallthrough_vt);
    }
    return ret;
  }
  ASSERT(pcrels.size() == 1);
  if (dst_insn_is_conditional_direct_branch(insn)) {
    ret.push_back(pcrels.at(0));
    ASSERT(fallthrough_vt.val < iseq_len);
    ret.push_back(fallthrough_vt);
    return ret;
  } else if (dst_insn_is_unconditional_branch_not_fcall(insn)) {
    ret.push_back(pcrels.at(0));
    return ret;
  } else NOT_REACHED();
  NOT_REACHED();
}

static void
dst_iseq_collect_unknowns_and_constraints_starting_at_index(dst_insn_t const *dst_iseq, long dst_iseq_len, flow_t const *estimated_flow, long idx, set<long> &unknowns, set<pair<long, long>> &eq_constraints, map<long, flow_t> &constant_constraints)
{
  if (estimated_flow[idx] != (unsigned long long)-1) {
    constant_constraints.insert(make_pair(idx, estimated_flow[idx]));
  }
  vector<valtag_t> succs = dst_insn_get_successors(&dst_iseq[idx], idx, dst_iseq_len);
  if (succs.size() == 1 && succs.begin()->tag == tag_abs_inum) {
    valtag_t const &succ = *succs.begin();
    ASSERT(succ.val >= 0 && succ.val < dst_iseq_len);
    if (estimated_flow[succ.val] == (unsigned long long)-1) {
      eq_constraints.insert(make_pair(idx, succ.val));
    } else {
      constant_constraints.insert(make_pair(idx, estimated_flow[succ.val]));
    }
  }
  for (const auto &vt : succs) {
    if (vt.tag != tag_abs_inum) {
      continue;
    }
    ASSERT(vt.val >= 0 && vt.val < dst_iseq_len);
    if (estimated_flow[vt.val] != (flow_t)-1) {
      continue;
    }
    if (unknowns.count(vt.val)) {
      continue;
    }
    unknowns.insert(vt.val);
    dst_iseq_collect_unknowns_and_constraints_starting_at_index(dst_iseq, dst_iseq_len, estimated_flow, vt.val, unknowns, eq_constraints, constant_constraints);
  }
}

static map<long, flow_t>
solve_flow_constraints(set<long> const &unknowns, set<pair<long, long>> const &eq_constraints, map<long, flow_t> const &constant_constraints)
{
  CPP_DBG_EXEC2(POPULATE_ESTIMATED_FLOW,
    cout << __func__ << " " << __LINE__ << ": unknowns =";
    for (const auto &u : unknowns) {
      cout << " " << u;
    }
    cout << endl;
    cout << "eq-constraints =";
    for (const auto &e : eq_constraints) {
      cout << " (" << e.first << ", " << e.second << ")";
    }
    cout << endl;
    cout << "constant-constraints =";
    for (const auto &e : constant_constraints) {
      cout << " (" << e.first << ", " << e.second << ")";
    }
    cout << endl;
  );
  distinct_set_t<long> eq_classes;
  for (const auto &u : unknowns) {
    eq_classes.distinct_set_make(u);
  }
  for (const auto &e : eq_constraints) {
    eq_classes.distinct_set_union(e.first, e.second);
  }
  map<long, flow_t> ret;
  for (const auto &c : constant_constraints) {
    long cu = c.first;
    long cf = c.second;
    for (const auto &u : unknowns) {
      if (eq_classes.distinct_set_find(cu) == eq_classes.distinct_set_find(u)) {
        ret.insert(make_pair(u, cf));
      }
    }
  }
  return ret;
}

static void
dst_iseq_label_estimated_flow_starting_at_index(dst_insn_t const *dst_iseq, long dst_iseq_len, flow_t *estimated_flow, long idx)
{
  //for now, we assume that all flows can be computed uniquely through a simple union-find data structure; a more general solution is to set this up as a system of linear equations and solve them by computing pseudo-inverse of coefficient matrix
  ASSERT(idx >= 0 && idx < dst_iseq_len);
  set<long> unknowns;
  set<pair<long,long>> eq_constraints;
  map<long, flow_t> constant_constraints;
  dst_iseq_collect_unknowns_and_constraints_starting_at_index(dst_iseq, dst_iseq_len, estimated_flow, idx, unknowns, eq_constraints, constant_constraints);
  map<long, flow_t> soln = solve_flow_constraints(unknowns, eq_constraints, constant_constraints);
  for (const auto &s : soln) {
    ASSERT(s.first >= 0 && s.first < dst_iseq_len);
    estimated_flow[s.first] = s.second;
  }
}

static void
populate_estimated_flow_from_computed_flow(flow_t *estimated_flow, dst_insn_t const *dst_iseq, long dst_iseq_len, map<long, flow_t> const &computed_flow)
{
  CPP_DBG_EXEC(POPULATE_ESTIMATED_FLOW,
    cout << __func__ << " " << __LINE__ << ": dst_iseq =\n" << dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1) << endl;
    cout << __func__ << " " << __LINE__ << ": computed_flow =\n";
    for (const auto &f : computed_flow) {
      cout << "computed_flow[" << f.first << "] = " << f.second << endl;
    }
  );
  if (dst_iseq_len == 0) {
    return;
  }
  for (size_t i = 0; i < dst_iseq_len; i++) {
    estimated_flow[i] = (unsigned long long)-1;
  }
  for (const auto &cf : computed_flow) {
    if (!(cf.first >= 0 && cf.first < dst_iseq_len)) {
      //cout << __func__ << " " << __LINE__ << ": cf.first = " << cf.first << ", dst_iseq_len = " << dst_iseq_len << endl;
      continue;
    }
    estimated_flow[cf.first] = cf.second;
  }
  for (size_t i = 0; i < dst_iseq_len; i++) {
    dst_iseq_label_estimated_flow_starting_at_index(dst_iseq, dst_iseq_len, estimated_flow, i);
  }
}

//static size_t
//dst_insns_identify_tail_call_optimizations(dst_insn_t *dst_iseq, size_t dst_iseq_len, size_t dst_iseq_size)
//{
//  for (size_t i = 0; i < dst_iseq_len - 2; i++) {
//    if (   dst_insn_is_function_call(&dst_iseq[i])
//        && dst_insn_deallocates_stack(&dst_iseq[i + 1])
//        && dst_insn_is_function_return(&dst_iseq[i + 2])) {
//      dst_insn_t tmp;
//      dst_insn_copy(&tmp, &dst_iseq[i]);
//      dst_insn_copy(&dst_iseq[i], &dst_iseq[i + 1]);
//      dst_insn_copy(&dst_iseq[i + 1], &tmp);
//      dst_insn_convert_function_call_to_branch(&dst_iseq[i + 1]);
//      dst_insn_put_nop(&dst_iseq[i + 2]);
//    }
//  }
//  return dst_iseq_len;
//}

class dst_insn_ptr_set_t
{
private:
  std::set<dst_insn_ptr_t> m_set;
  bool m_is_top;
public:
  dst_insn_ptr_set_t() : m_is_top(false) { }
  static dst_insn_ptr_set_t top() 
  {
    dst_insn_ptr_set_t ret;
    ret.m_is_top = true;
    return ret;
  }
  bool meet(dst_insn_ptr_set_t const &other)
  {
    ASSERT(!other.m_is_top);
    if (this->m_is_top) {
      *this = other;
      return true;
    }
    auto old_val = m_set;
    set_intersect(m_set, other.m_set);
    return old_val != m_set;
  }

  void intersect(dst_insn_ptr_set_t const &other)
  {
    set_intersect(this->m_set, other.m_set);
  }
  void add(dst_insn_ptr_t const &d)
  {
    this->m_set.insert(d);
  }
  std::set<dst_insn_ptr_t> const &get_set() const { return m_set; }
  string to_string() const
  {
    stringstream ss;
    if (m_is_top) {
      ss << "top";
    } else {
      ss << "{";
      for (const auto &d : m_set) {
        ss << d.get_inum() << ",";
      }
      ss << "}";
    }
    return ss.str();
  }
  bool operator!=(dst_insn_ptr_set_t const &other) const
  {
    return this->m_set != other.m_set;
  }
};

class dominators_dfa_t : public data_flow_analysis<dst_insn_ptr_t, node<dst_insn_ptr_t>, edge<dst_insn_ptr_t>, dst_insn_ptr_set_t, dfa_dir_t::forward>
{
public:
  dominators_dfa_t(dshared_ptr<graph_with_regions_t<dst_insn_ptr_t, node<dst_insn_ptr_t>, edge<dst_insn_ptr_t>> const> cfg, map<dst_insn_ptr_t, dst_insn_ptr_set_t> &init_vals) :
                   data_flow_analysis<dst_insn_ptr_t, node<dst_insn_ptr_t>, edge<dst_insn_ptr_t>, dst_insn_ptr_set_t, dfa_dir_t::forward>(cfg, init_vals)
  {
    //dst_insn_ptr_set_t()
  }
  //dst_insn_ptr_set_t meet(dst_insn_ptr_set_t const &out1, dst_insn_ptr_set_t const &out2)
  //{
  //  dst_insn_ptr_set_t ret = out1;
  //  ret.intersect(out2);
  //  return ret;
  //}
  //dst_insn_ptr_set_t xfer(shared_ptr<edge<dst_insn_ptr_t>> e, dst_insn_ptr_set_t const &out)
  //{
  //  dst_insn_ptr_t const &pc = e->get_from_pc();
  //  dst_insn_ptr_set_t ret = out;
  //  ret.add(pc);
  //  return ret;
  //}
  dfa_xfer_retval_t xfer_and_meet(dshared_ptr<edge<dst_insn_ptr_t> const> const &e, dst_insn_ptr_set_t const &in, dst_insn_ptr_set_t &out) override
  {
    //NOT_TESTED(true);
    dst_insn_ptr_t const &pc = e->get_to_pc(); //XXX: changed from_pc to to_pc; needs testing
    dst_insn_ptr_set_t ret = in;
    ret.add(pc);
    return out.meet(ret) ? DFA_XFER_RETVAL_CHANGED : DFA_XFER_RETVAL_UNCHANGED;
  }

};

class postdominators_dfa_t : public data_flow_analysis<dst_insn_ptr_t, node<dst_insn_ptr_t>, edge<dst_insn_ptr_t>, dst_insn_ptr_set_t, dfa_dir_t::backward>
{
public:
  postdominators_dfa_t(dshared_ptr<graph_with_regions_t<dst_insn_ptr_t, node<dst_insn_ptr_t>, edge<dst_insn_ptr_t>> const> cfg, map<dst_insn_ptr_t, dst_insn_ptr_set_t> &init_vals) :
                   data_flow_analysis<dst_insn_ptr_t, node<dst_insn_ptr_t>, edge<dst_insn_ptr_t>, dst_insn_ptr_set_t, dfa_dir_t::backward>(cfg, init_vals)
  {
    //dst_insn_ptr_set_t()
  }
  //dst_insn_ptr_set_t meet(dst_insn_ptr_set_t const &out1, dst_insn_ptr_set_t const &out2)
  //{
  //  dst_insn_ptr_set_t ret = out1;
  //  ret.intersect(out2);
  //  return ret;
  //}
  //dst_insn_ptr_set_t xfer(shared_ptr<edge<dst_insn_ptr_t>> e, dst_insn_ptr_set_t const &out)
  //{
  //  dst_insn_ptr_t const &pc = e->get_from_pc();
  //  dst_insn_ptr_set_t ret = out;
  //  ret.add(pc);
  //  return ret;
  //}
  dfa_xfer_retval_t xfer_and_meet(dshared_ptr<edge<dst_insn_ptr_t> const> const &e, dst_insn_ptr_set_t const &in, dst_insn_ptr_set_t &out) override
  {
    //cout << __func__ << " " << __LINE__ << ": e = " << e->to_string() << endl;
    //cout << __func__ << " " << __LINE__ << ": in = " << in.to_string() << endl;
    dst_insn_ptr_t const &pc = e->get_from_pc();
    dst_insn_ptr_set_t ret = in;
    ret.add(pc);
    return out.meet(ret) ? DFA_XFER_RETVAL_CHANGED : DFA_XFER_RETVAL_UNCHANGED;
  }
};

/*static map<dst_insn_ptr_t, dst_insn_ptr_t>
compute_immediate_relation(map<dst_insn_ptr_t, dst_insn_ptr_set_t> const &rel)
{
  map<dst_insn_ptr_t, dst_insn_ptr_t> ret;
  for (const auto &p : rel) {
    dst_insn_ptr_t const &dp = p.first;
    //dst_insn_ptr_set_t const &dps = p.second;
    set<dst_insn_ptr_t> const &dps = p.second.get_set();
    dst_insn_ptr_t const *imm_d = NULL;
    for (const auto &d : dps) {
      if (d == dp) {
        continue;
      }
      bool found_e = false;
      for (const auto &e : dps) {
        if (e == d || e == dp) {
          continue;
        }
        if (   set_belongs(rel.at(dp).get_set(), e)
            && set_belongs(rel.at(e).get_set(), d)) {
          found_e = true;
          break;
        }
      }
      if (!found_e) {
        imm_d = &d;
        break;
      }
    }
    if (imm_d) {
      ret.insert(make_pair(dp, *imm_d));
    } else {
      ret.insert(make_pair(dp, dp));
    }
  }
  return ret;
}*/

static map<dst_insn_ptr_t, dst_insn_ptr_t>
compute_immediate_dominators(dshared_ptr<graph_with_regions_t<dst_insn_ptr_t, node<dst_insn_ptr_t>, edge<dst_insn_ptr_t>> const> cfg)
{
  map<dst_insn_ptr_t, dst_insn_ptr_set_t> doms;
  dominators_dfa_t dominators_dfa(cfg, doms);
  dominators_dfa.initialize(dst_insn_ptr_set_t());
  dominators_dfa.solve();
  //map<dst_insn_ptr_t, dst_insn_ptr_set_t> doms = dominators_dfa.get_values();
  return compute_immediate_relation<dst_insn_ptr_t,dst_insn_ptr_set_t>(doms);
}

static map<dst_insn_ptr_t, dst_insn_ptr_t>
compute_immediate_postdominators(dshared_ptr<graph_with_regions_t<dst_insn_ptr_t, node<dst_insn_ptr_t>, edge<dst_insn_ptr_t>> const> cfg)
{
  map<dst_insn_ptr_t, dst_insn_ptr_set_t> postdoms;
  postdominators_dfa_t postdominators_dfa(cfg, postdoms);
  postdominators_dfa.initialize(dst_insn_ptr_set_t());
  postdominators_dfa.solve();
  //map<dst_insn_ptr_t, dst_insn_ptr_set_t> postdoms = postdominators_dfa.get_values();
  return compute_immediate_relation<dst_insn_ptr_t,dst_insn_ptr_set_t>(postdoms);
}

class pc_has_majority_flow_split_in_future_val_t
{
private:
  bool m_is_top;
public:
  list<dst_insn_ptr_t> path;
  dst_insn_ptr_t last_pc;

  pc_has_majority_flow_split_in_future_val_t() : m_is_top(false) { }

  static pc_has_majority_flow_split_in_future_val_t
  top()
  {
    pc_has_majority_flow_split_in_future_val_t ret;
    ret.m_is_top = true;
    return ret;
  }

  bool
  meet(/*pc_has_majority_flow_split_in_future_val_t const &out1, */pc_has_majority_flow_split_in_future_val_t const &out2, flow_t const *estimated_flow, size_t estimated_flow_len, flow_t MAJORITY_THRESHOLD)
  {
    //NOT_TESTED(true);
    ASSERT(!out2.m_is_top);
    if (this->m_is_top) {
      *this = out2;
      return true;
    }
    ASSERT(this->last_pc == out2.last_pc);
    dst_insn_t const *dst_insn = this->last_pc.get_ptr();
    long inum = this->last_pc.get_inum();
    //cout << __func__ << " " << __LINE__ << ": called meet on inum " << inum << endl;
    vector<valtag_t> vts = dst_insn_get_pcrel_values(dst_insn);
    ASSERT(vts.size() <= 1);
    if (vts.size() == 0 || vts.at(0).tag != tag_abs_inum) {
      if (this->path.size() == 0) {
        if (*this != out2) {
          *this = out2;
          return true;
        } else {
          return false;
        }
      }
      if (out2.path.size() == 0) {
        return false;
        //return out1;
      }
      if (this->path.size() < out2.path.size()) {
        return false;
        //return out1;
      }
      if (out2.path.size() < this->path.size()) {
        *this = out2;
        return true;
        //return out2;
      }
      if (*this != out2) {
        cout << __func__ << " " << __LINE__ << ": this = " << this->to_string() << endl;
        cout << __func__ << " " << __LINE__ << ": out2 = " << out2.to_string() << endl;
      }
      ASSERT(*this == out2);
      //return out1;
      return false;
    }
    //if (vts.size() != 1 || vts.at(0).tag != tag_abs_inum) {
    //  cout << __func__ << " " << __LINE__ << ": dst_insn =\n" << dst_insn_to_string(dst_insn, as1, sizeof as1) << endl;
    //  list<shared_ptr<edge<dst_insn_ptr_t>>> outgoing;
    //  this->m_graph->get_edges_outgoing(this->last_pc, outgoing);
    //  cout << __func__ << " " << __LINE__ << ": outgoing.size() = " << outgoing.size() << endl;
    //  for (const auto &out_e : outgoing) {
    //    cout << out_e->to_string() << endl;
    //  }
    //  cout << endl;
    //}
    ASSERT(vts.size() == 1);
    ASSERT(vts.at(0).tag == tag_abs_inum);
    long fallthrough_inum = inum + 1;
    long target_inum = vts.at(0).val;
    pc_has_majority_flow_split_in_future_val_t ret;
    if (   (fallthrough_inum >= 0 && fallthrough_inum < estimated_flow_len)
        && (target_inum >= 0 && target_inum < estimated_flow_len)
        && (estimated_flow[target_inum] >= MAJORITY_THRESHOLD * estimated_flow[fallthrough_inum])) {
      ret.path.push_front(this->last_pc);
    }
    this->path = ret.path;
    //ret.last_pc = out1.last_pc;
    //return ret;
    return true;
  }


  bool operator!=(pc_has_majority_flow_split_in_future_val_t const &other) const
  {
    ASSERT(!m_is_top);
    ASSERT(!other.m_is_top);
    return    path != other.path
           || !(last_pc == other.last_pc);
  }
  bool operator==(pc_has_majority_flow_split_in_future_val_t const &other) const
  {
    ASSERT(!m_is_top);
    ASSERT(!other.m_is_top);
    return    path == other.path
           && last_pc == other.last_pc;
  }
  string to_string() const
  {
    if (m_is_top) {
      return "TOP";
    }
    stringstream ss;
    ss << "[";
    for (const auto &d : path) {
      ss << d.get_inum() << ",";
    }
    ss << "]";
    return ss.str();
  }
};

class pc_has_majority_flow_split_in_future_dfa_t : public data_flow_analysis<dst_insn_ptr_t, node<dst_insn_ptr_t>, edge<dst_insn_ptr_t>, pc_has_majority_flow_split_in_future_val_t, dfa_dir_t::backward>
{
private:
  flow_t const *m_estimated_flow;
  size_t m_estimated_flow_len;
  flow_t const MAJORITY_THRESHOLD = 2;
public:
  pc_has_majority_flow_split_in_future_dfa_t(dshared_ptr<graph_with_regions_t<dst_insn_ptr_t, node<dst_insn_ptr_t>, edge<dst_insn_ptr_t>> const> cfg, flow_t const *estimated_flow, size_t estimated_flow_len, map<dst_insn_ptr_t, pc_has_majority_flow_split_in_future_val_t> &in_vals) :
                   data_flow_analysis<dst_insn_ptr_t, node<dst_insn_ptr_t>, edge<dst_insn_ptr_t>, pc_has_majority_flow_split_in_future_val_t, dfa_dir_t::backward>(cfg, in_vals),
                   m_estimated_flow(estimated_flow),
                   m_estimated_flow_len(estimated_flow_len)
  {
    //, pc_has_majority_flow_split_in_future_val_t()
  }
  //pc_has_majority_flow_split_in_future_val_t
  //pc_has_majority_flow_split_in_future_val_t xfer(shared_ptr<edge<dst_insn_ptr_t>> e, pc_has_majority_flow_split_in_future_val_t const &out)
  //{
  //  pc_has_majority_flow_split_in_future_val_t ret = out;
  //  if (ret.path.size()) {
  //    ret.path.push_front(e->get_from_pc());
  //  }
  //  ret.last_pc = e->get_from_pc();
  //  return ret;
  //}

  dfa_xfer_retval_t xfer_and_meet(dshared_ptr<edge<dst_insn_ptr_t> const> const &e, pc_has_majority_flow_split_in_future_val_t const &in, pc_has_majority_flow_split_in_future_val_t &out)
  {
    pc_has_majority_flow_split_in_future_val_t ret = in;
    if (ret.path.size()) {
      ret.path.push_front(e->get_from_pc());
    }
    ret.last_pc = e->get_from_pc();
    return out.meet(ret, m_estimated_flow, m_estimated_flow_len, MAJORITY_THRESHOLD) ? DFA_XFER_RETVAL_CHANGED : DFA_XFER_RETVAL_UNCHANGED;
  }
};

static map<dst_insn_ptr_t, list<dst_insn_ptr_t>>
compute_pc_has_majority_flow_split_in_future(dshared_ptr<graph_with_regions_t<dst_insn_ptr_t, node<dst_insn_ptr_t>, edge<dst_insn_ptr_t>> const> cfg, flow_t const *estimated_flow, size_t estimated_flow_len)
{
  map<dst_insn_ptr_t, pc_has_majority_flow_split_in_future_val_t> v;
  pc_has_majority_flow_split_in_future_dfa_t mdfa(cfg, estimated_flow, estimated_flow_len, v);
  mdfa.initialize(pc_has_majority_flow_split_in_future_val_t());
  mdfa.solve();
  //map<dst_insn_ptr_t, pc_has_majority_flow_split_in_future_val_t> v = mdfa.get_values();
  map<dst_insn_ptr_t, list<dst_insn_ptr_t>> ret;
  for (const auto &d : v) {
    ret.insert(make_pair(d.first, d.second.path));
  }
  return ret;
}

static size_t
dst_iseq_eliminate_unconditional_branch_to_single_pred_target(dst_insn_t *dst_iseq, size_t dst_iseq_len, size_t dst_iseq_size, size_t ubranch_idx)
{
  CPP_DBG_EXEC(OPEN_DIAMOND,
    cout << __func__ << " " << __LINE__ << ": dst_iseq =\n" << dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1) << endl;
  );

  dst_insn_t *new_dst_iseq = new dst_insn_t[dst_iseq_size];
  ASSERT(new_dst_iseq);
  ssize_t *insn_map = new ssize_t[dst_iseq_len];
  ASSERT(insn_map);
  for (size_t i = 0; i < dst_iseq_len; i++) {
    insn_map[i] = -1;
  }
  size_t j = 0;
  for (size_t i = 0; i < dst_iseq_len; i++) {
    if (insn_map[i] != -1) {
      continue;
    }
    dst_insn_copy(&new_dst_iseq[j], &dst_iseq[i]);
    insn_map[i] = j;
    j++;
    ASSERT(j < dst_iseq_size);
    if (i == ubranch_idx) {
      ASSERT(dst_insn_is_unconditional_branch_not_fcall(&dst_iseq[i]));
      vector<valtag_t> vts = dst_insn_get_pcrel_values(&dst_iseq[i]);
      ASSERT(vts.size() == 1);
      ASSERT(vts.at(0).tag == tag_abs_inum);
      size_t t = vts.at(0).val;
      ASSERT(t >= 0 && t < dst_iseq_len);
      do {
        ASSERT(t < dst_iseq_len);
        ASSERT(j < dst_iseq_size);
        dst_insn_copy(&new_dst_iseq[j], &dst_iseq[t]);
        insn_map[t] = j;
        t++;
        j++;
      } while (!dst_insn_is_unconditional_branch_not_fcall(&dst_iseq[t - 1]));
      //cout << __func__ << " " << __LINE__ << ": new_dst_iseq =\n" << dst_iseq_to_string(new_dst_iseq, j, as1, sizeof as1) << endl;
    }
  }
  dst_iseq_rename_abs_inums_using_map(new_dst_iseq, j, insn_map, dst_iseq_len);
  dst_iseq_copy(dst_iseq, new_dst_iseq, j);
  delete [] new_dst_iseq;
  delete [] insn_map;
  CPP_DBG_EXEC(OPEN_DIAMOND,
    cout << __func__ << " " << __LINE__ << ": returning dst_iseq =\n" << dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1) << endl;
  );
  return j;
}

static size_t
open_diamond_structure_for_pred(dst_insn_t *dst_iseq, size_t dst_iseq_len, size_t dst_iseq_size, dshared_ptr<graph_with_regions_t<dst_insn_ptr_t, node<dst_insn_ptr_t>, edge<dst_insn_ptr_t>> const> cfg, dst_insn_ptr_t const &head, dst_insn_ptr_t const &joinpoint, list<dst_insn_ptr_t> const &tailpath, dst_insn_ptr_t const &pred)
{
  //for all predecessors of joinpoint that are direct unconditional branches:
  //  - add a copy of the instructions from joinpoint to tail to the end of dst_iseq, and
  //  - make the predecessor branch to the copy instead.
  CPP_DBG_EXEC(OPEN_DIAMOND,
    cout << __func__ << " " << __LINE__ << ": opening diamond structure, dst_iseq =\n" << dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1) << endl;
    cout << "head = " << head.get_inum() << ", joinpoint = " << joinpoint.get_inum() << endl;
    cout << "tailpath =";
    for (const auto &t : tailpath) {
      cout << " " << t.get_inum();
    }
    cout << endl;
    cout << "pred = " << pred.get_inum() << endl;
  );
  dst_insn_t *pred_insn = pred.get_ptr();
  vector<valtag_t> vts = dst_insn_get_pcrel_values(pred_insn);
  ASSERT(vts.size() == 1);
  ASSERT(vts.at(0).tag == tag_abs_inum);
  vts.at(0).val = dst_iseq_len;
  dst_insn_set_pcrel_values(pred_insn, vts);
  long new_dst_iseq_len = dst_iseq_len;
  for (const auto &t : tailpath) {
    ASSERT(new_dst_iseq_len < dst_iseq_size);
    dst_insn_t *insn = t.get_ptr();
    dst_insn_copy(&dst_iseq[new_dst_iseq_len], insn);
    new_dst_iseq_len++;
  }
  ASSERT(new_dst_iseq_len < dst_iseq_size);
  const auto &last_t = *tailpath.rbegin();
  dst_insn_put_jump_to_abs_inum(last_t.get_inum() + 1, &dst_iseq[new_dst_iseq_len], 1);
  new_dst_iseq_len++;
  dst_iseq_len = new_dst_iseq_len;
  dst_iseq_len = dst_iseq_eliminate_unconditional_branch_to_single_pred_target(dst_iseq, dst_iseq_len, dst_iseq_size, pred.get_inum());

  CPP_DBG_EXEC2(OPEN_DIAMOND,
    cout << __func__ << " " << __LINE__ << ": returning dst_iseq =\n" << dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1) << endl;
  );
  return dst_iseq_len;
}

size_t
dst_insns_open_small_diamond_structures_in_cfg(dst_insn_t *dst_iseq, size_t dst_iseq_len, size_t dst_iseq_size, flow_t const *estimated_flow)
{
  size_t estimated_flow_len = dst_iseq_len;
  bool changed = true;
  CPP_DBG_EXEC2(OPEN_DIAMOND, cout << __func__ << " " << __LINE__ << ": dst_iseq =\n" << dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1) << endl);
  CPP_DBG_EXEC2(OPEN_DIAMOND,
    cout << __func__ << " " << __LINE__ << ": estimated_flow =\n" << endl;
    for (size_t i = 0; i < dst_iseq_len; i++) {
      cout << "estimated_flow[" << i << "] = " << estimated_flow[i] << endl;
    }
  );
  while (changed) {
    auto cfg = dst_iseq_get_cfg(dst_iseq, dst_iseq_len);
    map<dst_insn_ptr_t, dst_insn_ptr_t> imm_dominators = compute_immediate_dominators(cfg);
    map<dst_insn_ptr_t, dst_insn_ptr_t> imm_postdominators = compute_immediate_postdominators(cfg);
    map<dst_insn_ptr_t, list<dst_insn_ptr_t>> pc_has_majority_flow_split_in_future = compute_pc_has_majority_flow_split_in_future(cfg, estimated_flow, estimated_flow_len);
    changed = false;
    for (const auto &p : cfg->get_all_pcs()) {
      CPP_DBG_EXEC3(OPEN_DIAMOND, cout << __func__ << " " << __LINE__ << ": p = " << p.to_string() << endl);
      if (!imm_postdominators.count(p)) {
        continue;
      }
      CPP_DBG_EXEC3(OPEN_DIAMOND, cout << __func__ << " " << __LINE__ << ": p = " << p.to_string() << endl);
      dst_insn_ptr_t const &pdom = imm_postdominators.at(p);
      CPP_DBG_EXEC3(OPEN_DIAMOND, cout << __func__ << " " << __LINE__ << ": p = " << p.to_string() << ", pdom = " << pdom.to_string() << endl);
      if (imm_dominators.count(pdom) == 0) {
        continue;
      }
      dst_insn_ptr_t const &dom = imm_dominators.at(pdom);
      CPP_DBG_EXEC3(OPEN_DIAMOND, cout << __func__ << " " << __LINE__ << ": p = " << p.to_string() << ", pdom = " << pdom.to_string() << ", dom = " << dom.to_string() << endl);
      if (!(dom == p)) {
        continue;
      }
      if (pdom.get_inum() == p.get_inum() + 1) {
        continue; //discard adjacent instructions; we are interested in diamond structures
      }
      const auto &majority_flow_split = pc_has_majority_flow_split_in_future.at(pdom);
      CPP_DBG_EXEC3(OPEN_DIAMOND, cout << __func__ << " " << __LINE__ << ": p = " << p.to_string() << ", pdom = " << pdom.to_string() << ", dom = " << dom.to_string() << ", majority_flow_split.size() = " << majority_flow_split.size() << endl);
      if (   majority_flow_split.size() == 0
          || majority_flow_split.size() > MAX_TAIL_PATH_LENGTH_FOR_OPENING_DIAMOND_CFG_STRUCTURES) {
        continue;
      }
      list<dshared_ptr<edge<dst_insn_ptr_t> const>> incoming;
      cfg->get_edges_incoming(pdom, incoming);
      for (const auto &in_e : incoming) {
        dst_insn_ptr_t const &pred = in_e->get_from_pc();
        CPP_DBG_EXEC3(OPEN_DIAMOND, cout << __func__ << " " << __LINE__ << ": p = " << p.to_string() << ", pdom = " << pdom.to_string() << ", dom = " << dom.to_string() << ", pred = " << pred.to_string() << endl);
        if (pred.get_inum() + 1 == pdom.get_inum()) {
          continue;
        }
        dst_insn_t *pred_insn = pred.get_ptr();
        CPP_DBG_EXEC3(OPEN_DIAMOND, cout << __func__ << " " << __LINE__ << ": p = " << p.to_string() << ", pdom = " << pdom.to_string() << ", dom = " << dom.to_string() << ", pred = " << pred.to_string() << ", pred_insn = " << dst_insn_to_string(pred_insn, as1, sizeof as1) << endl);
        if (!dst_insn_is_unconditional_branch_not_fcall(pred_insn)) {
          continue;
        }
        CPP_DBG_EXEC3(OPEN_DIAMOND, cout << __func__ << " " << __LINE__ << ": p = " << p.to_string() << ", pdom = " << pdom.to_string() << ", dom = " << dom.to_string() << ", pred = " << pred.to_string() << ", pred_insn = " << dst_insn_to_string(pred_insn, as1, sizeof as1) << endl);
        changed = true;
        dst_iseq_len = open_diamond_structure_for_pred(dst_iseq, dst_iseq_len, dst_iseq_size, cfg, p, pdom, majority_flow_split, pred);
        break;
      }
      if (changed) {
        break;
      }
    }
  }
  CPP_DBG_EXEC2(OPEN_DIAMOND, cout << __func__ << " " << __LINE__ << ": returning dst_iseq =\n" << dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1) << endl);
  return dst_iseq_len;
}

static void
dst_insns_convert_dead_insns_to_nop(dst_insn_t *dst_iseq, size_t dst_iseq_len)
{
  CPP_DBG_EXEC(DST_INSN_DEAD_CODE_ELIM, cout << __func__ << " " << __LINE__ << ": dst_iseq =\n" << dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1) << endl);
  auto cfg = dst_iseq_get_cfg(dst_iseq, dst_iseq_len);
  map<dst_insn_ptr_t, regset_t> live = cfg_compute_liveness(cfg);
  for (size_t i = 0; i < dst_iseq_len - 1; i++) {
    vector<valtag_t> vts = dst_insn_get_pcrel_values(&dst_iseq[i]);
    if (vts.size() > 0) {
      continue;
    }
    if (dst_insn_is_indirect_branch(&dst_iseq[i])) {
      continue;
    }
    memset_t memuse, memdef;
    regset_t use, def;
    dst_insn_get_usedef(&dst_iseq[i], &use, &def, &memuse, &memdef);
    if (!memdef.memset_is_empty()) {
      continue;
    }
    dst_insn_ptr_t dptr(&dst_iseq[i + 1], dst_iseq);
    if (!live.count(dptr)) {
      continue;
    }
    regset_t const &live_out = live.at(dptr);
    CPP_DBG_EXEC(DST_INSN_DEAD_CODE_ELIM, cout << __func__ << " " << __LINE__ << ": instruction " << i << ": " << dst_insn_to_string(&dst_iseq[i], as1, sizeof as1) << ":\n");
    CPP_DBG_EXEC(DST_INSN_DEAD_CODE_ELIM, cout << "live " << regset_to_string(&live_out, as1, sizeof as1) << endl);
    CPP_DBG_EXEC(DST_INSN_DEAD_CODE_ELIM, cout << "def " << regset_to_string(&def, as1, sizeof as1) << endl);
    regset_intersect(&def, &live_out);
    if (regset_is_empty(&def)) {
      CPP_DBG_EXEC(DST_INSN_DEAD_CODE_ELIM, cout << __func__ << " " << __LINE__ << ": replacing instruction " << i << ": " << dst_insn_to_string(&dst_iseq[i], as1, sizeof as1) << " with nop\n");
      dst_insn_put_nop(&dst_iseq[i], 1);
    }
  }
}

size_t
dst_insns_folding_optimizations(dst_insn_t *dst_iseq, size_t dst_iseq_len, size_t dst_iseq_size, map<long, flow_t> const &computed_flow/*, size_t stack_size*/)
{
  CPP_DBG_EXEC(DST_INSN_FOLDING,
    dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1);
    string pid = int_to_string(getpid());
    stringstream ss;
    ss << g_query_dir << "/" << DST_INSN_FOLDING_FILENAME_PREFIX << ".hash" << md5_checksum(as1);
    string filename = ss.str();
    ofstream fo(filename.data());
    fo << as1 << endl;
    fo << "==\n";
    for (auto const& cf : computed_flow) {
      fo << cf.first << " : " << cf.second << endl;
    }
    fo.close();
    cout << "dst_insns_folding_optimizations filename " << filename << '\n';
  );
  flow_t *estimated_flow = new flow_t[dst_iseq_len];
  ASSERT(estimated_flow);
  dst_iseq_convert_pcrel_inums_to_abs_inums(dst_iseq, dst_iseq_len);
  populate_estimated_flow_from_computed_flow(estimated_flow, dst_iseq, dst_iseq_len, computed_flow);
  dst_iseq_len = dst_insns_open_small_diamond_structures_in_cfg(dst_iseq, dst_iseq_len, dst_iseq_size, estimated_flow);
  //dst_iseq_len = dst_insns_eliminate_redundant_moves(dst_iseq, dst_iseq_len, dst_iseq_size, estimated_flow);
  dst_insns_convert_dead_insns_to_nop(dst_iseq, dst_iseq_len);
  dst_iseq_convert_abs_inums_to_pcrel_inums(dst_iseq, dst_iseq_len);
  dst_iseq_len = dst_iseq_eliminate_nops(dst_iseq, nullptr, dst_iseq_len);
  dst_iseq_len = dst_iseq_optimize_branching_code(dst_iseq, dst_iseq_len, dst_iseq_size);
  delete [] estimated_flow;
  return dst_iseq_len;
}
