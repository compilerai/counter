#pragma once
#include "expr/context.h"
#include "expr/consts_struct.h"
#include "support/debug.h"
#include "graph/graph_cp_location.h"
#include "expr/graph_arg_regs.h"
#include "graph/callee_summary.h"
#include "expr/state.h"
#include "support/types.h"
#include "expr/memlabel.h"
#include "support/timers.h"

namespace eqspace {
struct consts_struct_t;

typedef enum { LR_STATUS_TOP, LR_STATUS_LINEARLY_RELATED, LR_STATUS_BOTTOM } lr_status_type_t;

class lr_status_t;
using lr_status_ref = shared_ptr<lr_status_t const>;
lr_status_ref mk_lr_status(lr_status_type_t typ, set<cs_addr_ref_id_t> const &cs_addr_ref_ids, set<memlabel_t> const &bottom_set);


class lr_status_t
{
private:
  lr_status_type_t m_lr_status;
  set<cs_addr_ref_id_t> m_cs_addr_ref_ids;
  set<memlabel_t> m_bottom_set;

  bool m_is_managed;
public:
  lr_status_t() = delete;
  ~lr_status_t();

  bool id_is_zero() const { return !m_is_managed; }
  void set_id_to_free_id(unsigned suggested_id)
  {
    ASSERT(!m_is_managed);
    m_is_managed = true;
  }
  static manager<lr_status_t> *get_lr_status_manager();
  friend shared_ptr<lr_status_t const> mk_lr_status(lr_status_type_t typ, set<cs_addr_ref_id_t> const &cs_addr_ref_ids, set<memlabel_t> const &bottom_set);
  friend struct std::hash<lr_status_t>;

  static lr_status_ref top()
  {
    //lr_status_t ret(LR_STATUS_TOP, set<cs_addr_ref_id_t>(), set<memlabel_t>());
    //ret.m_lr_status = LR_STATUS_TOP;
    return mk_lr_status(LR_STATUS_TOP, set<cs_addr_ref_id_t>(), set<memlabel_t>());
  }
  static lr_status_ref bottom(set<cs_addr_ref_id_t> const &cs_addr_ref_ids, set<memlabel_t> const &bottom_set)
  {
    //lr_status_t ret(LR_STATUS_BOTTOM, cs_addr_ref_ids, bottom_set);
    //ret.m_lr_status = LR_STATUS_BOTTOM;
    //ret.m_cs_addr_ref_ids = cs_addr_ref_ids;
    //ret.m_bottom_set = bottom_set;
    return mk_lr_status(LR_STATUS_BOTTOM, cs_addr_ref_ids, bottom_set);
  }
  static lr_status_ref linearly_related(cs_addr_ref_id_t cs_addr_ref_id)
  {
    //lr_status_t ret(LR_STATUS_LINEARLY_RELATED, {cs_addr_ref_id}, {});
    //ret.m_lr_status = LR_STATUS_LINEARLY_RELATED;
    //ret.m_cs_addr_ref_ids.insert(cs_addr_ref_id);
    //return ret;
    return mk_lr_status(LR_STATUS_LINEARLY_RELATED, {cs_addr_ref_id}, {});
  }
  static lr_status_ref linearly_related(set<cs_addr_ref_id_t> const &cs_addr_ref_ids)
  {
    //lr_status_t ret(LR_STATUS_LINEARLY_RELATED, cs_addr_ref_ids, {});
    //ret.m_lr_status = LR_STATUS_LINEARLY_RELATED;
    //ret.m_cs_addr_ref_ids = cs_addr_ref_ids;
    //return ret;
    return mk_lr_status(LR_STATUS_LINEARLY_RELATED, cs_addr_ref_ids, {});
  }
  static set<cs_addr_ref_id_t> subtract_memlocs_from_addr_ref_ids(set<cs_addr_ref_id_t> const &m_relevant_addr_ref_ids, graph_symbol_map_t const &symbol_map, context *ctx);
  static lr_status_ref start_pc_value(expr_ref const &e, set<cs_addr_ref_id_t> const &relevant_addr_refs, context *ctx, graph_arg_regs_t const &argument_regs, graph_symbol_map_t const &symbol_map);

  static lr_status_ref lr_status_union(lr_status_ref const &a, lr_status_ref const &b)
  {
    ASSERT(a->m_lr_status == LR_STATUS_LINEARLY_RELATED || a->m_lr_status == LR_STATUS_BOTTOM);
    ASSERT(a->m_lr_status == b->m_lr_status);
    set<cs_addr_ref_id_t> cs_addr_ref_ids = a->m_cs_addr_ref_ids;
    set_union(cs_addr_ref_ids, b->m_cs_addr_ref_ids);

    set<memlabel_t> bottom_set = a->m_bottom_set;
    set_union(bottom_set, b->m_bottom_set);

    //lr_status_t ret = a;
    //set_union(ret.m_cs_addr_ref_ids, b.m_cs_addr_ref_ids);
    //set_union(ret.m_bottom_set, b.m_bottom_set);
    //lr_status_t ret(a.m_lr_status, cs_addr_ref_ids, bottom_set);
    //return ret;
    return mk_lr_status(a->m_lr_status, cs_addr_ref_ids, bottom_set);
  }

  static lr_status_ref lr_status_meet(lr_status_ref const &a, lr_status_ref const &b)
  {
    autostop_timer func_timer(__func__);
    if (a->is_top()) {
      return b;
    }
    if (b->is_top()) {
      return a;
    }
    //lr_status_t ret = a;
    set<cs_addr_ref_id_t> cs_addr_ref_ids = a->m_cs_addr_ref_ids;
    set_union(cs_addr_ref_ids, b->m_cs_addr_ref_ids);

    set<memlabel_t> bottom_set = a->m_bottom_set;
    set_union(bottom_set, b->m_bottom_set);

    if (   a->is_bottom() 
        || b->is_bottom()) {
      //ret.m_lr_status = LR_STATUS_BOTTOM;
      //set_union(ret.m_bottom_set, b.m_bottom_set);
      //return lr_status_t(LR_STATUS_BOTTOM, cs_addr_ref_ids, bottom_set);
      return mk_lr_status(LR_STATUS_BOTTOM, cs_addr_ref_ids, bottom_set);
    }
    ASSERT(a->m_lr_status == LR_STATUS_LINEARLY_RELATED);
    ASSERT(b->m_lr_status == LR_STATUS_LINEARLY_RELATED);
    //return lr_status_t(LR_STATUS_LINEARLY_RELATED, cs_addr_ref_ids, bottom_set);
    //return lr_status_t(LR_STATUS_LINEARLY_RELATED, cs_addr_ref_ids, bottom_set);
    return mk_lr_status(LR_STATUS_LINEARLY_RELATED, cs_addr_ref_ids, bottom_set);
  }

  set<cs_addr_ref_id_t> const &get_cs_addr_ref_ids() const
  {
    ASSERT(m_lr_status == LR_STATUS_LINEARLY_RELATED || m_lr_status == LR_STATUS_BOTTOM);
    return m_cs_addr_ref_ids;
  }

  set<memlabel_t> const &get_bottom_set() const
  {
    ASSERT(m_lr_status == LR_STATUS_LINEARLY_RELATED || m_lr_status == LR_STATUS_BOTTOM);
    ASSERT(m_lr_status == LR_STATUS_BOTTOM || m_bottom_set.empty());
    return m_bottom_set;
  }

  bool is_top() const
  {
    return m_lr_status == LR_STATUS_TOP;
  }


  bool is_linearly_related_to_non_empty_addr_ref_ids() const
  {
    return m_lr_status == LR_STATUS_LINEARLY_RELATED && m_cs_addr_ref_ids.size() > 0;
  }

  bool is_bottom() const
  {
    return m_lr_status == LR_STATUS_BOTTOM;
  }

  bool equals(lr_status_t const &other) const
  {
    if (m_lr_status != other.m_lr_status) {
      return false;
    }
    if (m_lr_status == LR_STATUS_TOP) {
      return true;
    }
    if (m_lr_status == LR_STATUS_LINEARLY_RELATED) {
      return m_cs_addr_ref_ids == other.m_cs_addr_ref_ids;
    }
    if (m_lr_status == LR_STATUS_BOTTOM) {
      return    m_cs_addr_ref_ids == other.m_cs_addr_ref_ids
             && m_bottom_set == other.m_bottom_set;
    }
    NOT_REACHED();
  }

  bool operator==(lr_status_t const &other) const
  {
    return this->equals(other);
  }

  memlabel_t to_memlabel(consts_struct_t const &cs, graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map) const;

  string lr_status_to_string(consts_struct_t const &cs) const
  {
    string prefix;
    if (m_lr_status == LR_STATUS_TOP) {
      return "lr_status_top";
    } else if (m_lr_status == LR_STATUS_LINEARLY_RELATED) {
      prefix = "lr_status_linearly_related";
    } else if (m_lr_status == LR_STATUS_BOTTOM) {
      prefix = "lr_status_bottom";
    } else NOT_REACHED();
    stringstream ss;
    ss << prefix << "(";
    for (auto cs_addr_ref_id : m_cs_addr_ref_ids) {
      ss << cs.get_addr_ref(cs_addr_ref_id).first << ", ";
    }
    ss << "; ";
    for (const auto &m : m_bottom_set) {
      ss << m.to_string() << ", ";
    }
    ss << ")";
    return ss.str();
  }
private:
  lr_status_t(lr_status_type_t typ, set<cs_addr_ref_id_t> const &cs_addr_ref_ids, set<memlabel_t> const &bottom_set) : m_lr_status(typ), m_cs_addr_ref_ids(cs_addr_ref_ids), m_bottom_set(bottom_set), m_is_managed(false)
  { }
};

}

namespace std
{
using namespace eqspace;
template<>
struct hash<lr_status_t>
{
  std::size_t operator()(lr_status_t const &lrs) const
  {
    std::size_t seed = 0;
    if (lrs.m_lr_status == LR_STATUS_TOP) {
      seed = 1;
    } else if (lrs.m_lr_status == LR_STATUS_LINEARLY_RELATED) {
      seed = 2;
    } else if (lrs.m_lr_status == LR_STATUS_BOTTOM) {
      seed = 3;
    } else NOT_REACHED();
    for (auto const &cs_addr_ref_id : lrs.m_cs_addr_ref_ids) {
      seed += 31 * cs_addr_ref_id;
    }
    hash<memlabel_obj_t> h;
    for (auto const &ml : lrs.m_bottom_set) {
      seed += 73 * ml.get_alias_set_size();
    }
    return seed;
  }
};
}
