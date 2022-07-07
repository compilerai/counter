#pragma once

#include "support/dyn_debug.h"

#include "expr/expr.h"

#include "gsupport/rodata_map.h"

namespace eqspace {
using namespace std;

template<typename T_PC, typename T_N, typename T_E>
class graphce_t;

template<typename T_PC, typename T_N, typename T_E>
using graphce_ref = dshared_ptr<graphce_t<T_PC, T_N, T_E>>;

template<typename T_PC, typename T_N, typename T_E>
graphce_ref<T_PC, T_N, T_E>
mk_graphce(counter_example_t const &ce, T_PC const &p, string const& name);

template<typename T_PC, typename T_N, typename T_E>
class graphce_t //: public memstats_object_t
{
private:
  counter_example_t m_ce;
  T_PC m_pc;
  string_ref m_name;
  static size_t next_name_id;

  //shared_ptr<graph_edge_composition_t<T_PC,T_N,T_E>> m_path_traversed;
  //bool m_is_managed;

public:
  /*static manager<graphce_t<T_PC, T_N, T_E>> *get_graphce_manager()
  {
    static manager<graphce_t<T_PC, T_N, T_E>> *ret = NULL;
    if (!ret) {
      ret = new manager<graphce_t<T_PC, T_N, T_E>>;
    }
    return ret;
  }*/

public:
  //virtual string classname() const override { return "graphce_t"; }
  //counter_example_t &get_counter_example() { return m_ce; }
  counter_example_t const &get_counter_example() const { return m_ce; }
  void graphce_counter_example_union(counter_example_t const &other) { m_ce.counter_example_union(other); }
  T_PC const &get_pc() const { return m_pc; }
  //shared_ptr<graph_edge_composition_t<T_PC,T_N,T_E>> const &get_path_traversed() const { return m_path_traversed; }
  string_ref const& graphce_get_name() const { return m_name; }

  static string get_next_graphce_name();

  bool operator==(graphce_t const &other) const
  {
    return    m_ce.counter_example_equals(other.m_ce)
           && m_pc == other.m_pc;
  }

  /*void set_id_to_free_id(unsigned suggested_id)
  {
    ASSERT(!m_is_managed);
    m_is_managed = true;
  }

  bool id_is_zero() const { return !m_is_managed; }*/

  ~graphce_t()
  {
    stats_num_graphce_destructions++;
    /*if (m_is_managed) {
      this->get_graphce_manager()->deregister_object(*this);
    }*/
  }

  //friend graphce_ref<T_PC, T_N, T_E> mk_graphce(counter_example_t const &ce, T_PC const &p, shared_ptr<graph_edge_composition_t<T_PC,T_N,T_E>> path_traversed); //TODO: make constructor private
  graphce_t(counter_example_t const &ce, T_PC const &p, string const& name) : /*memstats_object_t(), */m_ce(ce), m_pc(p), m_name(mk_string_ref(name))
  {
    stats_num_graphce_constructions++;
  }
  graphce_t(graphce_t const& other) : m_ce(other.m_ce), m_pc(other.m_pc)
  {
    stats_num_graphce_constructions++;
  }

  static string graphce_from_stream(istream& in, context* ctx, graphce_ref<T_PC, T_N, T_E>& gce)
  {
    string line;
    bool end;
    end = !getline(in, line);
    ASSERT(!end);
    string const prefix = "=graphce counterexample at pc ";
    if (!is_line(line, prefix)) {
      return line;
    }
    size_t comma = line.find(',');
    ASSERT(comma != string::npos);
    T_PC p = T_PC::create_from_string(line.substr(prefix.length(), comma - prefix.length()));
    string name = line.substr(comma + 1);
    trim(name);
    counter_example_t ce = counter_example_t::counter_example_from_stream(in, ctx);
    gce = mk_graphce<T_PC, T_N, T_E>(ce, p, name);
    end = !getline(in, line);
    ASSERT(!end);
    return line;
  }

  unsigned long graphce_get_hash_code() const;

  void graphce_to_stream(ostream& os) const
  {
    os << "=graphce counterexample at pc " << m_pc.to_string() << ", " << m_name->get_str() << "\n";
    m_ce.counter_example_to_stream(os);
  }

private:
};

}

/*namespace std
{
using namespace eqspace;
template<typename T_PC, typename T_N, typename T_E>
struct hash<graphce_t<T_PC, T_N, T_E>>
{
  std::size_t operator()(graphce_t<T_PC, T_N, T_E> const &gce) const
  {
    size_t ret = 0;
    myhash_combine(ret, hash<counter_example_t>(gce.get_counter_example()));
    myhash_combine(ret, hash<T_PC>(gce.get_counter_example()));
    return ret;
  }
};

}*/




namespace eqspace {
template<typename T_PC, typename T_N, typename T_E>
graphce_ref<T_PC, T_N, T_E> mk_graphce(counter_example_t const &ce, T_PC const &p, string const& name)
{
  /*graphce_t<T_PC, T_N, T_E> gce(ce, p, path_traversed);
  return graphce_ref<T_PC, T_N, T_E>::get_graphce_manager()->register_object(gce);*/
  //using a manager is not possible because counter-examples mutate during their lifetimes (more entries are added over time)
  DYN_DEBUG(ce_add, cout << _FNLN_ << ": Added new graphce " << name << " at " << p.to_string() << endl);
  return make_dshared<graphce_t<T_PC, T_N, T_E>>(ce, p, name);
}

}

