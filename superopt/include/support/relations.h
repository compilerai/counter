#pragma once

#include "support/utils.h"

template<typename T_DOMAIN, typename T_RANGE>
std::map<T_DOMAIN, T_DOMAIN>
compute_immediate_relation(std::map<T_DOMAIN, T_RANGE> const &rel)
{
  std::map<T_DOMAIN, T_DOMAIN> ret;
  for (const auto &p : rel) {
    T_DOMAIN const &dp = p.first;
    //dst_insn_ptr_set_t const &dps = p.second;
    set<T_DOMAIN> const &dps = p.second.get_set();
    T_DOMAIN const *imm_d = NULL;
    for (const auto &d : dps) {
      if (d == dp) {
        continue;
      }
      bool found_e = false;
      for (const auto &e : dps) {
        if (e == d || e == dp) {
          continue;
        }
        if (/*   set_belongs(rel.at(dp).get_set(), e)
            && */set_belongs(rel.at(e).get_set(), d)) {
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
}

template<typename T_DOMAIN, typename T_RANGE>
std::map<T_DOMAIN,T_RANGE>
compute_strict_relation(std::map<T_DOMAIN, T_RANGE> const &rel)
{
  map<T_DOMAIN,T_RANGE> ret;
  for (auto const& pp : rel) {
    auto const& dp = pp.first;
    auto doms = pp.second;
    auto itr = find(doms.begin(), doms.end(), dp);
    ASSERT(itr != doms.end());
    doms.erase(itr);
    ret.insert(make_pair(dp, doms));
  }
  return ret;
}
