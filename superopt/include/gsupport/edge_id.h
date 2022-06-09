#ifndef EDGE_ID_H
#define EDGE_ID_H

#include <string>
#include <memory>

#include "support/dyn_debug.h"

#include "expr/state.h"
#include "expr/pc.h"

#include "gsupport/te_comment.h"

template<typename T> class dshared_ptr;
template<class T, class... Args>
dshared_ptr<T> make_dshared(Args&&... args);

namespace eqspace {
using namespace std;

template<typename T_PC>
class edge_id_t {
public:
  edge_id_t() {}
  edge_id_t(T_PC const &from, T_PC const &to) : m_from(from), m_to(to), m_is_empty(false) {}
  edge_id_t(T_PC const &from, T_PC const &to, bool is_empty) : m_from(from), m_to(to), m_is_empty(is_empty) {}
  edge_id_t(istream& is) {
    string line;
    bool end;
    end = !getline(is, line);
    ASSERT(!end);
    trim(line);
    size_t sep = line.find("=>");
    if (sep == string::npos) {
      cout << _FNLN_ << ": line =\n" << line << endl;
    }
    ASSERT(sep != string::npos);
    m_from = T_PC::create_from_string(line.substr(0, sep));
    m_to = T_PC::create_from_string(line.substr(sep + 2));
    m_is_empty = false;
  }
  static dshared_ptr<edge_id_t const> empty() { return make_dshared<edge_id_t const>(T_PC::start(), T_PC::start(), true); }
  static shared_ptr<edge_id_t const> empty(T_PC const&) { NOT_REACHED(); }
  bool is_empty() const { return (m_is_empty == true); }
  bool operator<(edge_id_t const &other) const
  {
    return    m_from < other.m_from
           || (m_from == other.m_from && m_to < other.m_to);
  }
  bool operator==(edge_id_t const &other) const
  {
    return m_from == other.m_from && m_to == other.m_to && m_is_empty == other.m_is_empty;
  }
  bool operator!=(edge_id_t const &other) const { return !(*this == other); }
  T_PC const &get_from_pc() const { return m_from; }
  T_PC const &get_to_pc() const { return m_to; }
  string edge_id_to_string() const { return m_from.to_string() + "=>" + m_to.to_string(); }
  string to_string_for_eq(string const&) const { NOT_REACHED(); }
  string to_string_concise() const { return this->edge_id_to_string(); }

  expr_ref get_cond() const { NOT_REACHED(); }
  //expr_ref get_simplified_cond() const { NOT_REACHED(); }
  state const& get_to_state() const { NOT_REACHED(); }
  //state const& get_simplified_to_state() const { NOT_REACHED(); }
  unordered_set<expr_ref> const& get_assumes() const { NOT_REACHED(); }
  te_comment_t const& get_te_comment() const { NOT_REACHED(); }
  void visit_exprs_const(function<void (T_PC const&, string const&, expr_ref)> f) const { NOT_REACHED(); }

  //template<typename T_VAL, xfer_dir_t T_DIR>
  //T_VAL xfer(T_VAL const& in, function<T_VAL (T_VAL const&, expr_ref const&, state const&, unordered_set<expr_ref> const&)> atom_f, function<T_VAL (T_VAL const&, T_VAL const&)> meet_f, bool simplified) const
  //{ NOT_REACHED(); }
private:
  T_PC m_from, m_to;
  bool m_is_empty;
};

}

namespace std
{
using namespace eqspace;
template<typename T_PC>
struct hash<edge_id_t<T_PC>>
{
  std::size_t operator()(edge_id_t<T_PC> const &e) const
  {
    std::hash<T_PC> pc_hasher;
    std::size_t seed = 0;
    myhash_combine<size_t>(seed, pc_hasher(e.get_from_pc()));
    myhash_combine<size_t>(seed, pc_hasher(e.get_to_pc()));
    return seed;
  }
};

}

#endif
