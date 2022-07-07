#pragma once
#include "support/debug.h"
#include "support/dyn_debug.h"
#include "support/consts.h"
#include "support/utils.h"
#include "support/dshared_ptr.h"

using namespace std;

template <typename T_ATOM>
class serpar_composition_node_t
{
public:
  enum type
  {
    atom,
    series,
    parallel,
  };

  serpar_composition_node_t(type t, shared_ptr<serpar_composition_node_t const> const &a, shared_ptr<serpar_composition_node_t const> const &b) : m_type(t), m_a(a), m_b(b)
  {
    assert(a);
    assert(b);
  }

  enum type get_type() const { return m_type; }

  serpar_composition_node_t(T_ATOM const &e) : m_type(atom), m_atom(make_dshared<T_ATOM const>(e))
  { }

  serpar_composition_node_t(shared_ptr<T_ATOM const> const &e) : m_type(atom), m_atom(e)
  { }

  serpar_composition_node_t(dshared_ptr<T_ATOM const> const &e) : m_type(atom), m_atom(e)
  { }

  string to_string_concise() const
  {
    std::function<string (T_ATOM const &)> f = [](T_ATOM const &a)
    {
      if (a.is_empty()) {
        return string(EPSILON);
      }
      return a.get_from_pc().to_string() + "=>" + a.get_to_pc().to_string();
    };
    return this->to_string(f);
  }


  string to_string(std::function<string (T_ATOM const &)> f) const
  {
    switch (m_type) {
      case atom:
        return "(" + f(*m_atom)+ ")";
        break;
      case parallel:
        return "(" + m_a->to_string(f) + parallel_sep + m_b->to_string(f) + ")";
        break;
      case series:
        return "(" + m_a->to_string(f) + serial_sep + m_b->to_string(f) + ")";
        break;
    }
    NOT_REACHED();
  }

  //static shared_ptr<serpar_composition_node_t const>
  //serpar_composition_node_create_from_string(string const &str, std::function<T_ATOM (string const&)> f_atom)
  //{
  //  string a, b;
  //  type typ = find_pivot(str, a, b);
  //  switch(typ) {
  //    case atom:
  //    {
  //      //T_ATOM a = T_ATOM::read_from_string(str, aux);
  //      //cout << __func__ << " " << __LINE__ << ": str = " << str << ".\n";
  //      T_ATOM a = f_atom(str);
  //      return make_shared<serpar_composition_node_t const>(a);
  //    }
  //    case parallel:
  //    case series:
  //    {
  //      shared_ptr<serpar_composition_node_t const> e_a = serpar_composition_node_create_from_string(a, f_atom);
  //      shared_ptr<serpar_composition_node_t const> e_b = serpar_composition_node_create_from_string(b, f_atom);
  //      ASSERT(e_a && e_b);
  //      return make_shared<serpar_composition_node_t const>(typ, e_a, e_b);
  //    }
  //  }
  //  NOT_REACHED();
  //}

  list<dshared_ptr<T_ATOM const>> get_constituent_atoms() const
  {
    list<dshared_ptr<T_ATOM const>> ret;
    if (m_type == serpar_composition_node_t::atom) {
      ret.push_back(m_atom);
      return ret;
    }
    list<dshared_ptr<T_ATOM const>> ret_a = m_a->get_constituent_atoms();
    list<dshared_ptr<T_ATOM const>> ret_b = m_b->get_constituent_atoms();
    for (auto b : ret_b) {
      ret_a.push_back(b);
    }
    return ret_a;
  }

  //static shared_ptr<serpar_composition_node_t<T_ATOM> const>
  //deep_clone(shared_ptr<serpar_composition_node_t<T_ATOM> const> const& ec)
  //{
  //  if (ec->m_type == serpar_composition_node_t::atom) {
  //    return make_shared<serpar_composition_node_t const>(*ec->m_atom);
  //  } else {
  //    return make_shared<serpar_composition_node_t const>(ec->m_type, ec->m_a, ec->m_b);
  //  }
  //}

  static list<list<T_ATOM>> serpar_composition_get_paths(shared_ptr<serpar_composition_node_t<T_ATOM> const> const &ec)
  {
    list<list<T_ATOM>> ret;
    if (ec->m_type == serpar_composition_node_t::atom) {
      list<T_ATOM> path;
      path.push_back(*ec->m_atom);
      ret.push_back(path);
      return ret;
    } else if (ec->m_type == serpar_composition_node_t::parallel) {
      list<list<T_ATOM>> ret_a = serpar_composition_get_paths(ec->m_a);
      list<list<T_ATOM>> ret_b = serpar_composition_get_paths(ec->m_b);
      //union
      list_append(ret_a, ret_b);
      /*for (auto path : ret_a) {
        ret_b.push_back(path);
      }*/
      return ret_a;
    } else if (ec->m_type == serpar_composition_node_t::series) {
      list<list<T_ATOM>> ret_a = serpar_composition_get_paths(ec->m_a);
      list<list<T_ATOM>> ret_b = serpar_composition_get_paths(ec->m_b);
      //cartesian product
      list<list<T_ATOM>> ret;
      for (auto path_a : ret_a) {
        for (auto path_b : ret_b) {
          list<T_ATOM> combined_path = path_a;
          list_append(combined_path, path_b);
          ret.push_back(combined_path);
        }
      }
      return ret;
    } else NOT_REACHED();
  }

  bool is_atom() const { return m_type == atom; }
  bool is_series_node() const { return m_type == series; }
  bool is_parallel_node() const { return m_type == parallel; }
  //T_ATOM get_atom() const { ASSERT(m_atom); return *m_atom; }
  shared_ptr<T_ATOM const> get_atom() const { ASSERT(m_atom); return m_atom.get_shared_ptr(); }
  shared_ptr<serpar_composition_node_t const> get_a() const { return m_a; }
  shared_ptr<serpar_composition_node_t const> get_b() const { return m_b; }
  template<typename T_RET>
  T_RET visit_nodes(std::function<T_RET (T_ATOM const &)> f, std::function<T_RET (type, T_RET const &, T_RET const &)> fold_f) const
  {
    if (m_type == atom) {
      return f(*m_atom);
    } else {
      T_RET ret_a = m_a->visit_nodes(f, fold_f);
      T_RET ret_b = m_b->visit_nodes(f, fold_f);
      return fold_f(m_type, ret_a, ret_b);
    }
  }
  template<typename T_RET>
  T_RET visit_nodes(std::function<T_RET (dshared_ptr<T_ATOM const> const &)> f, std::function<T_RET (type, T_RET const &, T_RET const &)> fold_f) const
  {
    if (m_type == atom) {
      return f(m_atom);
    } else {
      T_RET ret_a = m_a->visit_nodes(f, fold_f);
      T_RET ret_b = m_b->visit_nodes(f, fold_f);
      return fold_f(m_type, ret_a, ret_b);
    }
  }
  void visit_nodes_const(std::function<void (dshared_ptr<T_ATOM const> const &)> f) const
  {
    if (m_type == atom) {
      f(m_atom);
    } else {
      m_a->visit_nodes_const(f);
      m_b->visit_nodes_const(f);
    }
  }
  void visit_nodes_const(std::function<void (T_ATOM const &)> f) const
  {
    if (m_type == atom) {
      f(*m_atom);
    } else {
      m_a->visit_nodes_const(f);
      m_b->visit_nodes_const(f);
    }
  }
  template<typename T_RET>
  T_RET visit_nodes_postorder_series(std::function<T_RET (dshared_ptr<T_ATOM const> const &, T_RET const &)> fold_atom_f,
                                     std::function<T_RET (T_RET const &, T_RET const &)> fold_parallel_f,
                                     T_RET const &postorder_val,
                                     std::function<T_RET (T_RET const &, T_RET const &)> fold_series_f = ignore_second_arg_and_return_first<T_RET>) const
  {
    if (m_type == atom) {
      return fold_atom_f(m_atom.get_shared_ptr(), postorder_val);
    } else if (m_type == series) {
      T_RET ret_b = m_b->visit_nodes_postorder_series(fold_atom_f, fold_parallel_f, postorder_val, fold_series_f);
      T_RET ret_a = m_a->visit_nodes_postorder_series(fold_atom_f, fold_parallel_f, ret_b, fold_series_f);
      return fold_series_f(ret_a, ret_b);
    } else if (m_type == parallel) {
      T_RET ret_a = m_a->visit_nodes_postorder_series(fold_atom_f, fold_parallel_f, postorder_val, fold_series_f);
      T_RET ret_b = m_b->visit_nodes_postorder_series(fold_atom_f, fold_parallel_f, postorder_val, fold_series_f);
      return fold_parallel_f(ret_a, ret_b);
    } else NOT_REACHED();
  }
  template<typename T_RET>
  T_RET visit_nodes_preorder_series(std::function<T_RET (dshared_ptr<T_ATOM const> const &, T_RET const &)> fold_atom_f,
                                     std::function<T_RET (T_RET const &, T_RET const &)> fold_parallel_f,
                                     T_RET const &preorder_val,
                                     std::function<T_RET (T_RET const &, T_RET const &)> fold_series_f = ignore_first_arg_and_return_second<T_RET>) const
  {
    if (m_type == atom) {
      return fold_atom_f(m_atom.get_shared_ptr(), preorder_val);
    } else if (m_type == series) {
      T_RET ret_a = m_a->visit_nodes_preorder_series(fold_atom_f, fold_parallel_f, preorder_val, fold_series_f);
      T_RET ret_b = m_b->visit_nodes_preorder_series(fold_atom_f, fold_parallel_f, ret_a, fold_series_f);
      return fold_series_f(ret_a, ret_b);
    } else if (m_type == parallel) {
      T_RET ret_a = m_a->visit_nodes_preorder_series(fold_atom_f, fold_parallel_f, preorder_val, fold_series_f);
      T_RET ret_b = m_b->visit_nodes_preorder_series(fold_atom_f, fold_parallel_f, preorder_val, fold_series_f);
      return fold_parallel_f(ret_a, ret_b);
    } else NOT_REACHED();
  }
  template<typename T_RET>
  T_RET visit_nodes_preorder_series_exclusive(std::function<T_RET (dshared_ptr<T_ATOM const> const &, T_RET const &)> fold_atom_f,
                                     //std::function<T_RET (T_RET const &, T_RET const &)> fold_parallel_f,
                                     T_RET const &preorder_val) const
  {
    if (m_type == atom) {
      return fold_atom_f(m_atom, preorder_val);
    } else if (m_type == series) {
      T_RET ret_a = m_a->visit_nodes_preorder_series_exclusive(fold_atom_f, /*fold_parallel_f, */preorder_val);
      if (!ret_a.is_true()) {
        return ret_a;
      }
      return m_b->visit_nodes_preorder_series_exclusive(fold_atom_f, /*fold_parallel_f, */ret_a);
    } else if (m_type == parallel) {
      T_RET ret_a = m_a->visit_nodes_preorder_series_exclusive(fold_atom_f, /*fold_parallel_f, */preorder_val);
      if (ret_a.is_true()) {
        return ret_a;
      }
      return m_b->visit_nodes_preorder_series_exclusive(fold_atom_f, /*fold_parallel_f, */preorder_val);
      //return fold_parallel_f(ret_a, ret_b);
    } else NOT_REACHED();
  }


  T_ATOM get_leftmost_atom() const
  {
    if (m_type == atom) {
      return *m_atom;
    } else {
      return m_a->get_leftmost_atom();
    }
  }
  T_ATOM get_rightmost_atom() const
  {
    if (m_type == atom) {
      return *m_atom;
    } else {
      return m_b->get_rightmost_atom();
    }
  }

  static list<shared_ptr<serpar_composition_node_t const>> serpar_composition_get_op_terms(shared_ptr<serpar_composition_node_t const> const &ec, type op)
  {
    list<shared_ptr<serpar_composition_node_t const>> ret, b;
    if (!ec) {
      return ret;
    }
    ASSERT(op == series || op == parallel);
    if (ec->get_type() == atom) {
      ret.push_back(ec);
      return ret;
    } else if (ec->get_type() == op) {
      ret = serpar_composition_get_op_terms(ec->m_a, op);
      b = serpar_composition_get_op_terms(ec->m_b, op);
      list_append(ret, b);
      return ret;
    } else {
      ret.push_back(ec);
      return ret;
    }
  }

  static list<shared_ptr<serpar_composition_node_t const>> serpar_composition_get_serial_terms(shared_ptr<serpar_composition_node_t const> const &ec)
  {
    return serpar_composition_get_op_terms(ec, series);
  }

  static list<shared_ptr<serpar_composition_node_t const>> serpar_composition_get_parallel_terms(shared_ptr<serpar_composition_node_t const> const &ec)
  {
    return serpar_composition_get_op_terms(ec, parallel);
  }

  //static void serpar_composition_list_sort(list<shared_ptr<serpar_composition_node_t const>> &ls, std::function<bool (T_ATOM const &, T_ATOM const &)> is_less)
  //{
  //  std::function<bool (shared_ptr<serpar_composition_node_t const> const &, shared_ptr<serpar_composition_node_t const> const &)> f =
  //  [&is_less](shared_ptr<serpar_composition_node_t const> const &a, shared_ptr<serpar_composition_node_t const> const &b)
  //  {
  //    return a->serpar_composition_is_less(*b, is_less);
  //  };
  //  ls.sort(f);
  //}

  //bool serpar_composition_is_less(serpar_composition_node_t<T_ATOM> const &other, std::function<bool (T_ATOM const &, T_ATOM const &)> is_less) const
  //{
  //  if (m_type == atom && other.m_type == atom) {
  //    return is_less(this->get_atom(), other.get_atom());
  //  }
  //  if ((m_type == atom || m_type == parallel) && (other.m_type == atom || other.m_type == parallel)) {
  //    list<shared_ptr<serpar_composition_node_t const>> terms = serpar_composition_get_parallel_terms(make_shared<serpar_composition_node_t<T_ATOM>>(*this));
  //    list<shared_ptr<serpar_composition_node_t const>> other_terms = serpar_composition_get_parallel_terms(make_shared<serpar_composition_node_t<T_ATOM>>(other));
  //    ASSERT(terms.size() > 1 || other_terms.size() > 1);
  //    serpar_composition_list_sort(terms, is_less);
  //    serpar_composition_list_sort(other_terms, is_less);
  //    auto cur_term = terms.begin();
  //    auto cur_other_term = other_terms.begin();
  //    for (size_t i = 0; i < min(terms.size(), other_terms.size()); i++) {
  //      ASSERT(cur_term != terms.end());
  //      ASSERT(cur_other_term != other_terms.end());
  //      ASSERT((*cur_term)->m_type != parallel);
  //      ASSERT((*cur_other_term)->m_type != parallel);
  //      /*if ((*cur_term)->m_type == atom && (*cur_other_term)->m_type == atom) {
  //        if (is_less((*cur_term)->get_atom(), (*cur_other_term)->get_atom())) {
  //          return true;
  //        }
  //        if (is_less((*cur_other_term)->get_atom(), (*cur_term)->get_atom())) {
  //          return false;
  //        }
  //        continue;
  //      }*/
  //      if ((*cur_term)->serpar_composition_is_less(*(*cur_other_term), is_less)) {
  //        return true;
  //      }
  //      if ((*cur_other_term)->serpar_composition_is_less(*(*cur_term), is_less)) {
  //        return false;
  //      }
  //      cur_term++;
  //      cur_other_term++;
  //    }
  //    if (terms.size() < other_terms.size()) {
  //      return true;
  //    }
  //    if (other_terms.size() < terms.size()) {
  //      return false;
  //    }
  //    return false;
  //  }

  //  if ((m_type == atom || m_type == parallel) && other.m_type == series) {
  //    serpar_composition_node_t<T_ATOM> const &other_a = *other.get_a();
  //    if (this->serpar_composition_is_less(other_a, is_less)) {
  //      return true;
  //    }
  //    if (other_a.serpar_composition_is_less(*this, is_less)) {
  //      return false;
  //    }
  //    return true;
  //  }
  //  if (m_type == series && (other.m_type == atom || other.m_type == parallel)) {
  //    serpar_composition_node_t<T_ATOM> const &a = *this->get_a();
  //    if (other.serpar_composition_is_less(a, is_less)) {
  //      return false;
  //    }
  //    if (a.serpar_composition_is_less(other, is_less)) {
  //      return true;
  //    }
  //    return false;
  //  }
  //  if (m_type == series && other.m_type == series) {
  //    serpar_composition_node_t<T_ATOM> const &a = *this->get_a();
  //    serpar_composition_node_t<T_ATOM> const &other_a = *other.get_a();
  //    serpar_composition_node_t<T_ATOM> const &b = *this->get_b();
  //    serpar_composition_node_t<T_ATOM> const &other_b = *other.get_b();
  //    if (a.serpar_composition_is_less(other_a, is_less)) {
  //      return true;
  //    }
  //    if (other_a.serpar_composition_is_less(a, is_less)) {
  //      return false;
  //    }
  //    if (b.serpar_composition_is_less(other_b, is_less)) {
  //      return true;
  //    }
  //    if (other_b.serpar_composition_is_less(b, is_less)) {
  //      return false;
  //    }
  //    return false;
  //  }
  //  NOT_REACHED();
  //}

  bool operator==(serpar_composition_node_t<T_ATOM> const &other) const
  {
    if (this->m_type != other.m_type) {
      return false;
    }
    if (this->m_type == atom) {
      return *this->m_atom == *other.m_atom;
    }
    return    this->m_a == other.m_a
           && this->m_b == other.m_b;
  }

  /*static bool graph_edge_composition_list_is_subset(list<graph_edge_composition_t<T_PC, T_N, T_E> const *> const &needle, list<graph_edge_composition_t<T_PC, T_N, T_E> const *> const &haystack)
  {
    // can be made linear time by using a manager for tfg_edge_composition_t!
    for (auto n : needle) {
      bool found = false;
      for (auto h : haystack) {
        if (   n == h
            || n->graph_edge_composition_equal(h)) {
          found = true;
          break;
        }
      }
      if (!found) {
        return false;
      }
    }
    return true;
  }*/
  virtual ~serpar_composition_node_t() = default;

protected:
  static type find_pivot(string const &str, string &a, string &b)
  {
    int num_braces = 0;
    unsigned i;
    for (i = 0; i < str.size(); ++i) {
      char c = str[i];
      if (c == '(') {
        ++num_braces;
      } else if (c == ')') {
        --num_braces;
      }
      if ((c == serial_sep || c == parallel_sep) && num_braces == 1) {
        break;
      }
    }
  
    if (i == str.size()) {
      a = str.substr(1, i-2);
      return atom;
    }
    a = str.substr(1, i-1);
    b = str.substr(i+1, str.length()-i-2);
  
    if (str[i] == serial_sep) {
      return series;
    } else if (str[i] == parallel_sep) {
      return parallel;
    } else {
      NOT_REACHED();
    }
  }

private:
  type const m_type;
  shared_ptr<serpar_composition_node_t const> const m_a;
  shared_ptr<serpar_composition_node_t const> const m_b;
  dshared_ptr<T_ATOM const> const m_atom;

  static char const parallel_sep = '+';
  static char const serial_sep = '*';
};

namespace std
{
//using namespace eqspace;
template<typename T_ATOM>
struct hash<serpar_composition_node_t<T_ATOM>>
{
  std::size_t operator()(serpar_composition_node_t<T_ATOM> const &s) const
  {
    if (s.get_type() == serpar_composition_node_t<T_ATOM>::atom) {
      return hash<T_ATOM>()(*s.get_atom());
    } else {
      std::size_t seed = 0;
      myhash_combine<size_t>(seed, s.get_type());
      myhash_combine<size_t>(seed, hash<const void *>()(s.get_a().get()));
      myhash_combine<size_t>(seed, hash<const void *>()(s.get_b().get()));
      return seed;
    }
  }
};

}

