#pragma once
#include <map>
#include <string>

#include "expr/expr.h"
#include "support/types.h"
#include "gsupport/suffixpath.h"

using namespace std;
namespace eqspace {

// define the type
using avail_exprs_t = map<graph_loc_id_t, expr_ref>;
using pred_avail_exprs_t = map<tfg_suffixpath_t, avail_exprs_t>;

string pred_avail_exprs_to_string(pred_avail_exprs_t const &src_pred_avail_exprs);
bool pred_avail_exprs_contains(pred_avail_exprs_t const &haystack, pred_avail_exprs_t const &needle);

}

namespace std {
template<>
struct hash<pred_avail_exprs_t>
{
  std::size_t operator()(pred_avail_exprs_t const &t) const
  {
    size_t seed = 0;
    for (auto const& p : t) {
      myhash_combine<size_t>(seed, hash<tfg_suffixpath_t>()(p.first));
      for (auto const& pp : p.second) {
        myhash_combine<size_t>(seed, hash<graph_loc_id_t>()(pp.first));
        myhash_combine<size_t>(seed, hash<expr_ref>()(pp.second));
      }
    }
    return seed;
  }
};
}
