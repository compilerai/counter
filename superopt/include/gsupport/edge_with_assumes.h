#pragma once

#include "support/utils.h"
#include "support/log.h"
#include "support/timers.h"

#include "expr/context.h"
#include "expr/expr_utils.h"
#include "expr/state.h"
#include "expr/pc.h"

#include "gsupport/edge_id.h"
#include "gsupport/edge.h"

namespace eqspace {

using namespace std;

template <typename T_PC>
class edge_with_assumes : public edge<T_PC>
{
public:
  edge_with_assumes(const T_PC& pc_from, const T_PC& pc_to, unordered_set<expr_ref> const& assumes) : edge<T_PC>(pc_from, pc_to), m_assumes(assumes)
  { }

  bool operator==(edge_with_assumes const& other) const
  {
    return    this->edge<T_PC>::operator==(other)
           && this->m_assumes == other.m_assumes
    ;
  }
  unordered_set<expr_ref> const& get_assumes() const { return m_assumes; }
private:
  unordered_set<expr_ref> const m_assumes;
};

}

namespace std
{
using namespace eqspace;
template<typename T_PC>
struct hash<edge_with_assumes<T_PC>>
{
  std::size_t operator()(edge_with_assumes<T_PC> const& te) const
  {
    size_t seed = 0;
    myhash_combine<size_t>(seed, hash<edge<T_PC>>()(te));
    myhash_combine<size_t>(seed, te.get_assumes().size());
    return seed;
  }
};

}
