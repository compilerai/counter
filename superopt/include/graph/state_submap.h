#pragma once

#include "support/string_ref.h"
#include "support/str_utils.h"
#include "expr/expr.h"

class state_submap_t
{
private:
  map<string_ref, expr_ref> m_name_submap;
  map<expr_id_t, pair<expr_ref, expr_ref>> m_expr_submap;
public:
  //state_submap_t(map<string_ref, expr_ref> const& name_submap, map<expr_id_t, pair<expr_ref, expr_ref>> const& expr_submap) : m_name_submap(name_submap), m_expr_submap(expr_submap)
  //{ }
  state_submap_t(map<expr_id_t, pair<expr_ref, expr_ref>> const& expr_submap) : m_expr_submap(expr_submap)
  {
    for (auto const& sm : expr_submap) {
      ASSERT(sm.second.first->is_var());
      string const& iname = sm.second.first->get_name()->get_str();
      if (!string_has_prefix(iname, G_INPUT_KEYWORD ".")) {
        ASSERT(   string_has_prefix(iname, CANON_CONSTANT_PREFIX)
               || string_has_prefix(iname, MEMVAR_NAME_PREFIX));
        continue;
      }
      string name = iname.substr(strlen(G_INPUT_KEYWORD "."));
      m_name_submap.insert(make_pair(mk_string_ref(name), sm.second.second));
    }
  }
  map<expr_id_t, pair<expr_ref, expr_ref>> const& get_expr_submap() const { return m_expr_submap; }
  map<string_ref, expr_ref> const& get_name_submap() const { return m_name_submap; }
};
