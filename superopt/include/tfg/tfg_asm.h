#pragma once

#include "tfg/tfg.h"

namespace eqspace {

class tfg_asm_t : public tfg
{
private:
public:
  tfg_asm_t(string const &name, context *ctx) : tfg(name, ctx)
  { }
  tfg_asm_t(tfg const &other) : tfg(other)
  { }
  tfg_asm_t(istream& is, string const &name, context *ctx) : tfg(is, name, ctx)
  { }
  virtual void tfg_preprocess(bool collapse, list<string> const& sorted_bbl_indices, map<graph_loc_id_t, graph_cp_location> const &llvm_locs = map<graph_loc_id_t, graph_cp_location>()) override;
  virtual shared_ptr<tfg> tfg_copy() const override;
};

}
