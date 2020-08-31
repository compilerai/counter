#include "tfg/edge_guard.h"
#include "tfg/tfg.h"
#include "expr/consts_struct.h"
#include "support/timers.h"
//#include "parser/parse_edge_composition.h"

/*
std::function<expr_ref (eg_cnode_t::type t, expr_ref const &a, expr_ref const &b)> edge_guard_t::fold_expr_ops = [](eg_cnode_t::type t, expr_ref const &a, expr_ref const &b)
{
  if (t == eg_cnode_t::series) {
    return expr_and(a, b);
  } else if (t == eg_cnode_t::parallel) {
    return expr_or(a, b);
  } else NOT_REACHED();
};

std::function<shared_ptr<eg_cnode_t> (eg_cnode_t::type t, shared_ptr<eg_cnode_t> const &a, shared_ptr<eg_cnode_t> const &b)> edge_guard_t::fold_edge_guard_ops = [](eg_cnode_t::type t, shared_ptr<eg_cnode_t> const &a, shared_ptr<eg_cnode_t> const &b)
{
  return make_shared<eg_cnode_t>(t, a, b);
};
*/

namespace eqspace {

expr_ref
edge_guard_t::edge_guard_get_expr(tfg const &t, bool simplified) const
{
  return t.tfg_edge_composition_get_edge_cond(m_guard_node, simplified);
}

string
edge_guard_t::read_edge_guard(istream &in, string const& first_line/*, state const& start_state*/, context* ctx, edge_guard_t &guard/*, tfg const *t*/)
{
  string line;
  ASSERT(is_line(first_line, "=Guard"));
  getline(in, line);
  if (line == "" || line == "(" EPSILON ")") {
    getline(in, line);
    guard = edge_guard_t();
    return line;
  }
  shared_ptr<tfg_edge_composition_t> ec;
  line = graph_edge_composition_t<pc,tfg_edge>::graph_edge_composition_from_stream(in, line, "=edge_guard"/*, start_state*/, ctx, ec);
  guard.m_guard_node = ec;
  return line;
}

edge_guard_t::edge_guard_t(shared_ptr<tfg_edge const> const& e, tfg const &t)// : m_tfg(t)
{
  //m_guard_node = make_shared<tfg_edge_composition_t>(e, t);
  if (e) {
    m_guard_node = mk_edge_composition<pc,tfg_edge>(e);
  } else {
    m_guard_node = mk_epsilon_ec<pc,tfg_edge>();
  }
}

edge_guard_t::edge_guard_t(list<shared_ptr<tfg_edge const>> const &path, tfg const &t)// : m_tfg(t)
{
  m_guard_node = t.tfg_edge_composition_create_from_path(path);
}

bool
edge_guard_t::operator==(edge_guard_t const &other) const
{
  return m_guard_node == other.m_guard_node;
}

bool
edge_guard_t::operator<(edge_guard_t const &other) const
{
  // just compare the pointers
  return m_guard_node < other.m_guard_node;
}


/*bool
edge_guard_t::equals(edge_guard_t const &other) const //XXX: make this more efficient!
{
  return m_guard_node->graph_edge_composition_to_string() == other.m_guard_node->graph_edge_composition_to_string();
}*/

string
edge_guard_t::edge_guard_to_string() const
{
  if (!m_guard_node) {
    return "";
  } else {
    return m_guard_node->graph_edge_composition_to_string();
  }
}

void
edge_guard_t::edge_guard_to_stream(ostream& os) const
{
  os << "=Guard\n";
  if (!m_guard_node) {
    os << "(" EPSILON ")" << '\n';
    return;
  }
  os << m_guard_node->graph_edge_composition_to_string_for_eq("=edge_guard");
}

string
edge_guard_t::edge_guard_to_string_for_eq() const
{
  ostringstream oss;
  this->edge_guard_to_stream(oss);
  return oss.str();
}

void
edge_guard_t::conjunct_guard(edge_guard_t const &other, tfg const &t)
{
  if (m_guard_node == NULL) {
    m_guard_node = other.m_guard_node;
    return;
  } else if (other.m_guard_node == NULL) {
    //do nothing
    return;
  }

  this->m_guard_node = t.tfg_edge_composition_conjunct(this->m_guard_node, other.m_guard_node);
}

/*pc
edge_guard_t::edge_guard_get_to_pc() const
{
  if (!m_guard_node) {
    return pc::start();
  }
  return m_guard_node->graph_edge_composition_get_to_pc();
}*/

void
edge_guard_t::add_guard_in_series(edge_guard_t const &other/*, tfg const &g*/)
{
  if (m_guard_node == NULL || m_guard_node->is_epsilon()) {
    m_guard_node = other.m_guard_node;
  } else if (other.m_guard_node == NULL || other.m_guard_node->is_epsilon()) {
    //do nothing
  } else {
    if (m_guard_node->graph_edge_composition_get_to_pc() != other.m_guard_node->graph_edge_composition_get_from_pc()) {
      cout << __func__ << " " << __LINE__ << ": this = " << this->edge_guard_to_string() << endl;
      cout << __func__ << " " << __LINE__ << ": other = " << other.edge_guard_to_string() << endl;
    }
    ASSERT(m_guard_node->graph_edge_composition_get_to_pc() == other.m_guard_node->graph_edge_composition_get_from_pc());
    //auto const &g = m_guard_node->get_graph();
    //m_guard_node = make_shared<tfg_edge_composition_t>(tfg_edge_composition_t::series, m_guard_node, other.m_guard_node, m_guard_node->get_graph());
    m_guard_node = mk_series(m_guard_node, other.m_guard_node);
  }
}

void
edge_guard_t::add_guard_in_parallel(edge_guard_t const &other)
{
  if (m_guard_node == NULL) {
    //do nothing
  } else if (other.m_guard_node == NULL) {
    m_guard_node = other.m_guard_node;
  } else {
    if (!m_guard_node->is_epsilon() && !other.m_guard_node->is_epsilon()) {
      if (m_guard_node->graph_edge_composition_get_from_pc() != other.m_guard_node->graph_edge_composition_get_from_pc()) {
        cout << __func__ << " " << __LINE__ << ": this_ec = " << m_guard_node->graph_edge_composition_to_string() << endl;
        cout << __func__ << " " << __LINE__ << ": other_ec = " << other.m_guard_node->graph_edge_composition_to_string() << endl;
      }
      ASSERT(m_guard_node->graph_edge_composition_get_from_pc() == other.m_guard_node->graph_edge_composition_get_from_pc());
      ASSERT(m_guard_node->graph_edge_composition_get_to_pc() == other.m_guard_node->graph_edge_composition_get_to_pc());
    }
    m_guard_node = mk_parallel(m_guard_node, other.m_guard_node);
  }
}

bool
edge_guard_t::is_true() const
{
  return m_guard_node == NULL;
}

bool
edge_guard_t::edge_guard_implies(edge_guard_t const &other) const
{
  if (other.m_guard_node == nullptr || other.m_guard_node->is_epsilon()) {
    return true;
  }
  if (this->m_guard_node == nullptr || this->m_guard_node->is_epsilon()) {
    ASSERT(other.m_guard_node);
    return false;
  }
  if (this->m_guard_node == other.m_guard_node) {
    return true;
  }

  return tfg::tfg_edge_composition_implies(this->m_guard_node, other.m_guard_node);
}

bool
edge_guard_t::edge_guard_represents_all_possible_paths(tfg const &t) const
{
  return t.tfg_edge_composition_represents_all_possible_paths(m_guard_node);
}

void
edge_guard_t::edge_guard_trim_true_prefix_and_true_suffix(tfg const &t)
{
  list<shared_ptr<serpar_composition_node_t<tfg_edge> const>> terms = serpar_composition_node_t<tfg_edge>::serpar_composition_get_serial_terms(this->m_guard_node);
  list<shared_ptr<serpar_composition_node_t<tfg_edge> const>>::const_iterator iter;
  list<shared_ptr<serpar_composition_node_t<tfg_edge> const>>::const_reverse_iterator riter;
  set<shared_ptr<serpar_composition_node_t<tfg_edge> const>> ignore;
  for (iter = terms.begin(); iter != terms.end(); iter++) {
    shared_ptr<tfg_edge_composition_t const> termg = dynamic_pointer_cast<tfg_edge_composition_t const>(*iter);
    if (t.tfg_edge_composition_represents_all_possible_paths(termg)) {
      ignore.insert(*iter);
    } else {
      break;
    }
  }
  for (riter = terms.rbegin(); riter != terms.rend(); riter++) {
    shared_ptr<tfg_edge_composition_t const> termg = dynamic_pointer_cast<tfg_edge_composition_t const>(*riter);
    if (set_belongs(ignore, *riter)) {
      break;
    }
    if (t.tfg_edge_composition_represents_all_possible_paths(termg)) {
      ignore.insert(*riter);
    } else {
      break;
    }
  }

  list<graph_edge_composition_ref<pc,tfg_edge>> new_terms;
  for (iter = terms.begin(); iter != terms.end(); iter++) {
    if (set_belongs(ignore, *iter)) {
      continue;
    }
    shared_ptr<tfg_edge_composition_t const> termg = dynamic_pointer_cast<tfg_edge_composition_t const>(*iter);
    new_terms.push_back(termg);
  }
  if (new_terms.size() == 0) {
    m_guard_node = nullptr;
  } else {
    m_guard_node = graph_edge_composition_construct_edge_from_serial_edgelist(new_terms);
  }
}

/*bool
edge_guard_t::edge_guard_evaluates_to_true(tfg const &t) const
{
  if (m_guard_node == NULL) {
    return true;
  }
  list<shared_ptr<serpar_composition_node_t<shared_ptr<tfg_edge const>> const>> terms = serpar_composition_node_t<shared_ptr<tfg_edge const>>::serpar_composition_get_serial_terms(this->m_guard_node);
  for (auto term : terms) {
    shared_ptr<tfg_edge_composition_t const> termg = dynamic_pointer_cast<tfg_edge_composition_t const>(term);
    if (!t.tfg_edge_composition_edge_cond_evaluates_to_true(termg)) {
      return false;
    }
  }
  return true;
}*/
void
edge_guard_t::edge_guard_clear()
{
  m_guard_node = nullptr;
}

bool
edge_guard_t::edge_guard_is_trivial() const
{
  return m_guard_node == nullptr;
}

bool
edge_guard_t::edge_guard_may_imply(edge_guard_t const& other) const
{
  if (other.m_guard_node == nullptr || other.m_guard_node->is_epsilon()) {
    return true;
  }
  if (this->m_guard_node == nullptr || this->m_guard_node->is_epsilon()) {
    ASSERT(other.m_guard_node);
    return false;
  }
  if (this->m_guard_node == other.m_guard_node) {
    return true;
  }

  return !graph_edge_composition_are_disjoint(this->m_guard_node, other.m_guard_node);
}

bool
edge_guard_t::edge_guard_compare_ids(edge_guard_t const &a, edge_guard_t const &b)
{
  //compare_ids function needs to have a deterministic output (e.g., cannot depend on pointer values)
  if (a.m_guard_node == b.m_guard_node) {
    return false;
  }
  if (!a.m_guard_node) {
    return true;
  }
  if (!b.m_guard_node) {
    return false;
  }
  if(a.m_guard_node->graph_edge_composition_get_from_pc() != b.m_guard_node->graph_edge_composition_get_from_pc())
    return (a.m_guard_node->graph_edge_composition_get_from_pc() < b.m_guard_node->graph_edge_composition_get_from_pc());
  if(a.m_guard_node->graph_edge_composition_get_to_pc() != b.m_guard_node->graph_edge_composition_get_to_pc())
    return (a.m_guard_node->graph_edge_composition_get_to_pc() < b.m_guard_node->graph_edge_composition_get_to_pc());
  return a.m_guard_node->graph_edge_composition_to_string() < b.m_guard_node->graph_edge_composition_to_string();
}

}
