#ifndef Z3_SOLVER_WRAPPER_H
#define Z3_SOLVER_WRAPPER_H

#include "z3++.h"

#include "support/debug.h"
#include "support/dshared_ptr.h"

#include "expr/solver_interface.h"
#include "expr/counter_example.h"
#include "expr/expr_to_z3_expr.h"

namespace eqspace {

using duration_us_t = unsigned long long;
class expr_query_cache
{
public:
  struct entry
  {
    expr_ref m_expr;
    solver_res m_result;
    list<counter_example_t> m_counter_examples;
    double m_num_seconds_to_solve;
    unsigned m_quantifiers;
    unsigned m_hits;
    query_comment m_comment;
  };

  bool find_result(expr_ref e, solver_res& res)
  {
    ++m_num_lookups;
    auto id = e->get_id();
    if (m_entries.count(id) > 0)
    {
      res = m_entries.at(id).m_result;
      ++m_entries.at(id).m_hits;
      return true;
    }
    else return false;
  }

  list<counter_example_t> const &find_counter_examples(expr_ref e)
  {
    ASSERT(m_entries.count(e->get_id()));
    return m_entries.at(e->get_id()).m_counter_examples;
  }

  void set_result_and_counter_example(expr_ref e, solver_res res, list<counter_example_t> const &counter_examples, double num_seconds_to_solve, unsigned quantifiers, const query_comment& qc)
  {
    autostop_timer func_timer(__func__);
    auto id = e->get_id();
    struct entry en = {
        .m_expr = e,
        .m_result = res,
        .m_counter_examples = counter_examples,
        .m_num_seconds_to_solve = num_seconds_to_solve,
        .m_quantifiers = quantifiers,
        .m_hits = 0,
        .m_comment = qc
    };
    m_entries.insert(make_pair(id, en));
  }

  /*void print_stats(void) const {
    vector<pair<unsigned, double> > runtimes;
    for (map<unsigned, entry>::const_iterator iter = m_entries.begin();
         iter != m_entries.end();
         iter++) {
      runtimes.push_back(make_pair((*iter).first, (*iter).second.m_num_seconds_to_solve));
    }
    sort(runtimes.begin(), runtimes.end(), num_seconds_comparer);
    printf("Solve time : Num Hits : Result\n");
    int i = 0;
    for (vector<pair<unsigned, double> >::const_iterator iter = runtimes.begin();
         iter != runtimes.end();
         iter++, i++) {
      unsigned id = (*iter).first;
      printf("%d. %.2f : %u : %s\n", i, m_entries.at(id).m_num_seconds_to_solve,
           m_entries.at(id).m_hits,
           solver_res_to_string(m_entries.at(id).m_result).c_str());
    }
    printf("Printing ten slowest queries:\n");
    i = 0;
    for (vector<pair<unsigned, double> >::const_iterator iter = runtimes.begin();
         iter != runtimes.end() && i < 10;
         iter++, i++) {
      unsigned id = (*iter).first;
      printf("Slowest query %d took %.2f seconds\n%s\n", i,
          m_entries.at(id).m_num_seconds_to_solve,
          expr_string(m_entries.at(id).m_expr).c_str());
    }
  }

  string stats() const
  {
    stringstream ss;
    ss << "expr_query_cache entry count: " << m_entries.size() << endl;
    for(map<unsigned, entry>::const_iterator iter = m_entries.begin(); iter != m_entries.end(); ++iter)
    {
      ss << iter->second.m_comment.to_string();
      ss << "result: " << iter->second.m_result << endl;
      ss << "cache hits: " << iter->second.m_hits << endl;
      ss << "----" << endl;
    }
    return ss.str();
  }*/
  void remove_timeout_entries()
  {
    for(map<unsigned, entry>::iterator iter = m_entries.begin(); iter != m_entries.end();)
    {
      if(iter->second.m_result == solver_res_timeout)
      {
        map<unsigned, entry>::iterator del_iter = iter;
        iter++;
        m_entries.erase(del_iter);
      }
      else
        ++iter;
    }
  }

  bool contains_timeout_res()
  {
    for(map<unsigned, entry>::iterator iter = m_entries.begin(); iter != m_entries.end(); ++iter)
    {
      if(iter->second.m_result == solver_res_timeout)
        return true;
    }
    return false;
  }

  string stats() const;

  void clear()
  {
    m_entries.clear();
  }

private:
  unsigned m_num_lookups;
  map<expr_id_t,entry> m_entries;
};

class z3_solver : public solver
{
public:
  z3_solver(context *ctx);
  ~z3_solver() override;

  proof_result_t solver_satisfiable(expr_ref e, const query_comment& qc, relevant_memlabels_t const& relevant_memlabels/*, list<counter_example_t> &counter_examples*/) override;
  proof_result_t solver_provable(expr_ref e, const query_comment& qc, relevant_memlabels_t const& relevant_memlabels/*, list<counter_example_t> &counter_examples*/) override;
  expr_ref solver_simplify(expr_ref const&) override;
  bool solver_prove_internal(expr_ref const&) override;

  //void evaluate(expr_ref);

  string stats() const override
  {
    stringstream ss;
    ss << m_expr_query_cache.stats();
    return ss.str();
  }

  void set_timeout(unsigned timeout_secs);
  unsigned get_timeout() const;
  bool timeout_occured_after_last_set_timeout() override;
  void clear_cache() override;

  dshared_ptr<expr_to_z3_expr> get_expr_to_z3_expr(expr_ref e, relevant_memlabels_t const& relevant_memlabels) const;

private:

  void put_smtlib_string_in_file(z3::expr const& e, string const& filename, dshared_ptr<expr_to_z3_expr> const& z3_converter, string const& solver_name = ""); //unspecified solver-name indicates the default solver configuration

  z3::expr convert_to_z3(expr_ref e, relevant_memlabels_t const& relevant_memlabels) const;
  expr_ref convert_from_z3(z3::expr e);

  pair<solver_res,duration_us_t> spawn_smt_query(string filename, unsigned timeout_secs, list<counter_example_t> &counter_examples);

  void get_counter_example(int recv_channel, expr_ref e_expr);

  expr_ref simplify_internal(expr_ref const&);
  proof_status_t prove_internal(expr_ref const&);

  pair<expr_ref,map<expr_id_t,expr_pair>> solver_get_substituted_expr_and_submap(expr_ref const& e, bool substitute_ineq=true);

  z3::solver m_solver;
  expr_query_cache m_expr_query_cache;
  context *m_context;
};

void solver_init(string const &record_filename = "", string const &replay_filename = "", string const& stats_filename = "");
void solver_kill(void);

//extern map<string,expr_ref> z3_counter_example;
//void print_counter_example();
}

#endif
