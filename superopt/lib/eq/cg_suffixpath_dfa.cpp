
#include "eq/corr_graph.h"
#include "eq/cg_suffixpath_dfa.h"

namespace eqspace {

bool
cg_suffixpath_computation::xfer_and_meet(shared_ptr<corr_graph_edge const> const &cge, tfg_suffixpath_val_t const& in, tfg_suffixpath_val_t &out)
{
  //cout << __func__ << " " << __LINE__ << ": cge = " << cge->to_string() << endl;
  auto src_ec = cge->get_src_edge();
  auto suffixpath_edge = mk_suffixpath_series(in.get_path(), src_ec);

  return out.meet(suffixpath_edge);
}

//map<pcpair, tfg_suffixpath_t> const&
void
corr_graph::update_src_suffixpaths_cg(shared_ptr<corr_graph_edge const> e)
{
  autostop_timer func_timer(__func__);
  map<pcpair, tfg_suffixpath_val_t> vals;

  for (const auto &s : m_src_suffixpath) {
    vals.insert(make_pair(s.first, tfg_suffixpath_val_t(s.second)));
  }

  cg_suffixpath_computation cg_sfp_analysis(this, vals);
  if (e == nullptr) {
    cg_sfp_analysis.initialize(mk_epsilon_ec<pc,tfg_edge>());
    cg_sfp_analysis.solve();
  } else {
    cg_sfp_analysis.incremental_solve(e);
  }
  for (const auto &v : vals) {
    if (v.second.get_path() != m_src_suffixpath.at(v.first)) {
      m_src_suffixpath.at(v.first) = v.second.get_path();
    }
  }
}

}
