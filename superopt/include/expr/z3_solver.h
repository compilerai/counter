#ifndef Z3_SOLVER_WRAPPER_H
#define Z3_SOLVER_WRAPPER_H

#include "z3++.h"
#include "expr/solver_interface.h"
#include "support/debug.h"
#include "expr/counter_example.h"

namespace eqspace {

class expr_query_cache
{
public:
  struct entry
  {
    expr_ref m_expr;
    solver::solver_res m_result;
    list<counter_example_t> m_counter_examples;
    double m_num_seconds_to_solve;
    unsigned m_count;
    query_comment m_comment;
  };

  bool find_result(expr_ref e, solver::solver_res& res)
  {
    /*if(DISABLE_CACHES)
      return false;*/
    //expr_ref e_zero_comment = e->zero_out_comments();
    //ASSERT(e_zero_comment == e); //currently zero_out_comments() does nothing
    //unsigned id = e_zero_comment->get_id();
    unsigned id = e->get_id();
    if(m_entries.count(id) > 0)
    {
      res = m_entries.at(id).m_result;
      ++m_entries.at(id).m_count;
      return true;
    }
    else return false;
  }

  list<counter_example_t> const &find_counter_examples(expr_ref e)
  {
    ASSERT(m_entries.count(e->get_id()));
    return m_entries.at(e->get_id()).m_counter_examples;
  }

  void set_result_and_counter_example(expr_ref e, solver::solver_res res, list<counter_example_t> const &counter_examples, double num_seconds_to_solve, const query_comment& qc)
  {
    //expr_ref e_zero_comment = e->zero_out_comments();
    //ASSERT(e_zero_comment == e); //currently zero_out_comments() does nothing
    unsigned id = e->get_id();
    struct entry en = {
        .m_expr = e,
        .m_result = res,
        .m_counter_examples = counter_examples,
        .m_num_seconds_to_solve = num_seconds_to_solve,
        .m_count = 0,
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
           m_entries.at(id).m_count,
           solver::solver_res_to_string(m_entries.at(id).m_result).c_str());
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
      ss << "cache hits: " << iter->second.m_count << endl;
      ss << "----" << endl;
    }
    return ss.str();
  }*/
  void remove_timeout_entries()
  {
    for(map<unsigned, entry>::iterator iter = m_entries.begin(); iter != m_entries.end();)
    {
      if(iter->second.m_result == solver::solver_res_timeout)
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
      if(iter->second.m_result == solver::solver_res_timeout)
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
  map<unsigned, entry> m_entries;
};


class z3_solver : public solver
{
public:
  //static memlabel_t const *relevant_memlabels;
  //static char * const *relevant_memlabels_str;
  //static size_t num_relevant_memlabels;
  //static size_t relevant_memlabels_bvsize;
  static z3::func_decl_vector z3_memlabel_names;
  static z3::func_decl_vector z3_memlabel_preds;

  z3_solver(context *ctx);
  ~z3_solver() override;
  solver_res solver_satisfiable(expr_ref e, const query_comment& qc, set<memlabel_ref> const& relevant_memlabels, list<counter_example_t> &counter_examples, consts_struct_t const &cs) override;
  solver_res solver_provable(expr_ref e, const query_comment& qc, set<memlabel_ref> const& relevant_memlabels, list<counter_example_t> &counter_examples, consts_struct_t const &cs) override;
  expr_ref solver_simplify(expr_ref, consts_struct_t const &cs) override;

  //void evaluate(expr_ref);

  static z3::sort memlabel_sort(consts_struct_t const &cs);
  //static z3::sort io_sort();

  expr_ref convert_from_z3(z3::expr e, context* ctx);

  string stats() const override
  {
    stringstream ss;
    ss << __func__ << " " << __LINE__ << ":";
    ss << m_expr_query_cache.stats();
    return ss.str();
  }

  void set_timeout(unsigned timeout_secs);
  unsigned get_timeout() const;
  bool timeout_occured_after_last_set_timeout() override;
  void clear_cache() override;
  //void set_relevant_memlabels(vector<memlabel_t> const &relevant_memlabels);
  //void get_relevant_memlabels(vector<memlabel_t> &relevant_memlabels);
  static vector<relevant_memlabel_idx_t> get_relevant_memlabel_indices_from_memlabel(memlabel_t const &ml, consts_struct_t const &cs);

private:
  solver_res spawn_smt_query(string filename, unsigned timeout_secs, list<counter_example_t> &counter_examples);

  std::function<z3::expr (z3::expr, size_t)> get_heap_constraint_z3_expr_from_relevant_memlabels(set<memlabel_ref> const& relevant_memlabels);
  z3::expr gen_z3_heap_constraint_expr(set<memlabel_ref> const& relevant_memlabels, z3::expr e, size_t count);

  void get_counter_example(int recv_channel, expr_ref e_expr);
  expr_ref simplify_internal(expr_ref, consts_struct_t const &);
  z3::solver m_solver;
  expr_query_cache m_expr_query_cache;
  //unsigned m_timeout_secs;
  //int m_helper_pid;
  context *m_context;
};

void solver_init(string const &record_filename = "", string const &replay_filename = "", string const& stats_filename = "");
void solver_kill(void);

//extern map<string,expr_ref> z3_counter_example;
//void print_counter_example();
}

#endif
