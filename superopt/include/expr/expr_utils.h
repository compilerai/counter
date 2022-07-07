#pragma once

#include "support/str_utils.h"

#include "expr/expr.h"
#include "gsupport/rodata_map.h"
#include "gsupport/graph_symbol.h"

//extern expr_ref g_esp;

namespace eqspace {

//string address_to_mem_pool_syntactic(expr_ref addr, consts_struct_t const &cs/*const map<string, expr_ref>& address_references*/);
//string address_to_mem_pool(expr_ref addr/*const map<string, expr_ref>& address_references*/, const map<unsigned, pair<expr_ref, string> >& add_sym_map, consts_struct_t const &cs);
//memlabel_t address_to_memlabel(expr_ref addr, map<unsigned, pair<expr_ref, map<int, lr_status_t>>> const &add_sym_map, consts_struct_t const &cs);
//bool is_address_relative(expr_ref addr, expr_ref reference);
//expr_list expr_top_level_bvs(expr_ref e);
//expr_list under_approx_conds2(expr_ref src_cond, expr_ref dst_cond);
//expr_list under_approx_cond_equalities(expr_ref src_cond, expr_ref dst_cond);
//void computer_expr_fixed_point(expr_vector& from, expr_vector& to);
//void computer_expr_fixed_point(vector<pair<expr_ref, expr_ref> > &fromto);
//bool expr_check_simple_containment(expr_ref e, expr_ref contain);

/*class expr_stack_mem_split : public expr_table_visitor
{
public:
  expr_stack_mem_split(context* ctx, const map<string, expr_ref>& address_references, const map<unsigned, pair<expr_ref, string> >& add_sym_map, const map<string, map<unsigned, pair<expr_ref, expr_ref> > >& subst_map);
  expr_ref get_split_mem(string i, expr_ref old_mem);
  void get_split_mems(expr_ref old_mem, map<string, expr_ref>& mem_pools);
  //void get_substituition_map(expr_vector& from, expr_vector& to);
  void get_substituition_map(map<unsigned, expr_ref> &submap);
  void theorems_substitute(expr_vector &th_from, expr_vector &th_to);

private:
  void visit(expr_ref e);
  bool is_expr_in_subst_map(expr_ref e);
  bool is_expr_in_select_subst_map(expr_ref e);

  void apply_select_subs_map_to_add_sym_map();

  map<string, expr_ref> m_address_references;
  map<string, map<unsigned, pair<expr_ref, expr_ref> > > m_subst_map;
  map<unsigned, pair<expr_ref, expr_ref> > m_select_subst_map;
  bool m_change;
  map<unsigned, pair<expr_ref, string> > m_add_sym_map;
};
*/

class expr_find_bv_non_linear_op : public expr_visitor
{
public:
  expr_find_bv_non_linear_op(expr_ref const &e);
  expr_vector get_matched_expr();

  virtual ~expr_find_bv_non_linear_op() = default;

private:
  void visit(expr_ref const &e) override;
  bool is_bv_non_linear_op(expr::operation_kind k);
  expr_vector m_result;
};

class expr_find_op : public expr_visitor
{
public:
  expr_find_op(expr_ref const &e, expr::operation_kind k);
  expr_vector get_matched_expr();

  virtual ~expr_find_op() = default;

private:
  void visit(expr_ref const &e) override;
  expr::operation_kind m_kind;
  expr_vector m_result;
};

class expr_count_ops_visitor : public expr_visitor
{
public:
  expr_count_ops_visitor(expr_ref const &e);
  long get_result();

  virtual ~expr_count_ops_visitor() = default;

private:
  void visit(expr_ref const &e) override;
  long m_result;
};

/*class expr_is_stack_addr : public expr_visitor
{
public:
  expr_is_stack_addr(expr_ref const &e, expr_ref const &esp);
  bool is_stack_addr() const;

private:
  virtual void visit(expr_ref const &e);
  expr_ref const &m_expr;
  expr_ref const &m_esp;
  map<unsigned, bool> m_is_expr_stack_addr;
};*/

class expr_contains_elem : public expr_visitor
{
public:
  expr_contains_elem(expr_ref const &e, bool (*contains_elem)(expr_ref const &, const void *),
      const void *contains_elem_arg, bool (*ignore_elem)(expr_ref const &, const void *),
      const void *ignore_elem_arg);

  bool satisfies() const;

  virtual ~expr_contains_elem() = default;

private:
  void visit(expr_ref const &e) override;
  expr_ref const &m_expr;
  //expr_ref m_esp;
  bool (*m_contains_elem)(expr_ref const &, const void *);
  bool (*m_ignore_elem)(expr_ref const &, const void *);
  const void *m_contains_elem_arg;
  const void *m_ignore_elem_arg;
  map<unsigned, bool> m_satisfies;
};

class expr_remove_any_one_ite_visitor : public expr_visitor
{
public:
  expr_remove_any_one_ite_visitor(expr_ref const &e);
  expr_ref get_cond_ite();
  expr_ref get_out1();
  expr_ref get_out2();

private:
  void visit(expr_ref const &e) override;
  expr_ref const &m_expr;
  expr_ref m_cond_ite, m_out1, m_out2;
};

class expr_print_with_ce_visitor : expr_visitor
{
public:
  expr_print_with_ce_visitor(expr_ref const &e, map<expr_id_t, expr_ref> const &emap, map<expr_id_t, expr_ref> const &ce_map, ostream &os) : m_expr(e), m_emap(emap), m_ce_map(ce_map), m_os(os), m_ctx(e->get_context())
  { }
  void print_result()
  {
    visit_recursive(m_expr);
  }
  void visit(expr_ref const &e) override
  {
    stringstream ss;
    expr_id_t eid = get_eid_using_emap(e);
    ss << eid << " : " << e->to_string_node_using_map(m_id2printid_map) << " : " << e->get_sort()->to_string();
    string estr = pad_string(ss.str(), 50);
    m_os << estr << " : ";
    //if (!m_id2printid_map.count(eid)) {
    //  cout << __func__ << " " << __LINE__ << ": no printid mapping for " << eid << ": " << expr_string(e) << endl;
    //  cout << __func__ << " " << __LINE__ << ": m_id2printid_map.size() = " << m_id2printid_map.size() << endl;
    //}
    //expr_id_t eid_mapped = m_id2printid_map.at(eid);
    if (!m_ce_map.count(eid)) {
      cout << __func__ << " " << __LINE__ << ": fatal: no CE mapping for eid " << eid << ": " << expr_string(e) << endl;
      NOT_REACHED();
    }
    m_os << expr_string(m_ce_map.at(eid)) << endl;
    m_id2printid_map.insert(make_pair(e->get_id(), eid));
  }
private:
  expr_id_t get_eid_using_emap(expr_ref const &e) const
  {
    for (auto const& ee : m_emap) {
      if (ee.second == e) {
        return ee.first;
      }
    }
    NOT_REACHED();
  }
private:
  expr_ref const &m_expr;
  map<expr_id_t, expr_ref> const &m_emap;
  map<expr_id_t, expr_ref> const &m_ce_map;
  ostream &m_os;
  context *m_ctx;
  map<expr_id_t, expr_id_t> m_id2printid_map;
};


//expr_ref expr_remove_selects(expr_ref e);
vector<pair<expr_ref, expr_ref> > remove_if_then_else(expr_ref e);
list<pair<expr_ref, pair<unsigned, bool> > > find_addrs_operating_on_array(expr_ref e, expr_ref iarray, consts_struct_t const &cs);
list<pair<expr_ref, pair<unsigned, bool> > > find_stack_read_addrs(expr_ref esp, expr_ref e, consts_struct_t const &cs);
list<expr_ref> get_stack_slots(list<pair<expr_ref, pair<unsigned, bool> > > & addrs, expr_ref stack_array);

bool expr_is_equal(expr_ref const &a, const void *opaque);
string expr_vector_to_string(expr_vector const &ev);
string expr_submap_to_string(map<expr_id_t, pair<expr_ref,expr_ref>> const& submap);
expr_ref get_base_array(expr_ref arr);
string get_base_array_name(expr_ref arr);
expr_ref expr_remove_selects(expr_ref e);
//vector<pair<expr_ref, expr_ref> > remove_ite(expr_ref e, long max_ite_depth, bool use_queries, consts_struct_t const &cs);
//expr_ref expr_eliminate_function_calls(expr_ref e);
//list<pair<expr_ref, pair<unsigned, bool> > > find_stack_read_addrs(expr_ref esp, expr_ref e);
//list<expr_ref> get_stack_slots(list<pair<expr_ref, pair<unsigned, bool> > > & addrs, expr_ref stack_array);
string print_expr_list(list<expr_ref>& es);
bool expr_list_is_satisfiable(expr_list const &ls, context *ctx);
bool expr_lists_are_satisfiable(list<expr_list> const &lists, context *ctx);
//bool expr_list_is_satisfiable_with_binsearch(expr_list const &ls, struct context *ctx);
//bool expr_zero_out_stack_addresses_in_function_calls(expr_ref &in, consts_struct_t const &cs);
expr_vector expr_get_function_calls(expr_ref e);
size_t expr_num_op_occurrences(expr_ref e, expr::operation_kind op);
size_t expr_count_ops(expr_ref e);
void expr_debug_check(expr_ref e, consts_struct_t const *cs);
//set<memlabel_ref> expr_get_relevant_memlabels(expr_ref const& e);
//void expr_vector_union(vector<expr_ref> &dst, vector<expr_ref> const &src, query_comment const &qc, bool timeout_res, consts_struct_t const &cs);
//
expr_ref expr_replace_fcall_with_const(expr_ref e);
bool expr_contains_input_stack_pointer_const(expr_ref e);

bool is_mem_alloc_sort(expr_ref const& e);
bool is_mem_data_sort(expr_ref const& e);
bool is_dst_expr(expr_ref const& e);
expr_ref expr_rename_symbols_to_dst_symbol_addrs(expr_ref const& e);
unordered_set<expr_ref> unordered_set_visit_exprs(unordered_set<expr_ref> const& in_set, function<expr_ref (string const&, expr_ref)>);
void unordered_set_visit_exprs_const(unordered_set<expr_ref> const& in_set, function<void (string const&, expr_ref)>);
bool expr_has_mem_ops(expr_ref const& e);
bool expr_is_local_size(expr_ref const& e);

bool expr_all_callee_readable_memlabels_are_subset_of_ml(expr_ref const& e, memlabel_t const& haystack, memlabel_t& needle);
expr_ref expr_replace_fcall_input_memvars_with_new_memvar(expr_ref const &e, map<expr_id_t, expr_ref> const &input_memvars, expr_ref const &new_memvar);

map<expr_id_t,expr_ref> expr_get_expr_map(expr_ref const& e);

expr_ref get_dummy_mem_alloc_expr(sort_ref const& s);
//expr_ref get_corresponding_mem_alloc_from_mem_expr(expr_ref const& e);

expr_ref expr_convert_constant_function_to_array_sort(expr_ref const &e);
expr_ref expr_convert_function_to_apply_on_memlabel_sort(expr_ref const &e, sort_ref const &s);
expr_ref expr_convert_alloca_ptr_function_to_apply_on_count_and_memlabel_sorts(expr_ref const& e, sort_ref const& s);

expr_ref expr_convert_memlabel_id_in_solver_to_memlabel_expr_using_map(expr_ref const& constant_expr, map<expr_ref,memlabel_ref> const& id_in_solver_to_memlabel);

expr_ref expr_replace_readonly_memlabels_with_rodata_memlabel(expr_ref const& e);

vector<sort_ref> expr_vector_to_sort_vector(expr_vector const& ev);

bool expr_is_simple_select(expr_ref const& e);
expr_set expr_get_simple_selects(expr_ref const& e);

expr_ref expr_substitute_iteratively_using_submap(expr_ref e, expr_submap_t const& submap);

bool expr_represents_signed_inequality(expr_ref const& e);
bool expr_represents_unsigned_inequality(expr_ref const& e);

}
