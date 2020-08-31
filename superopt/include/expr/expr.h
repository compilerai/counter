#pragma once

#include <stdlib.h>
//#include "eqcheck/common.h"
//#include "expr/comment.h"
#include "expr/langtype.h"
#include "expr/memaccess_size.h"
//#include "support/ref.h"
#include "support/types.h"
#include "support/utils.h"
#include "support/mybitset.h"
#include "support/manager.h"
#include <vector>
#include <list>
#include <stack>
#include <string>
#include <map>
#include <set>
#include <functional>
#include <unordered_set>
//#include "expr/counter_example.h"
//#include "expr/counter_example_set.h"
#include "expr/memlabel.h"
//#include "memlabel_map.h"
#include "support/mybitset.h"
#include "expr/constant_value.h"
//#include "expr/graph_arg_regs.h"
//#include "tfg/graph_symbol.h"

#include "support/mytimer.h"
namespace eqspace {

class graph_symbol_map_t;
class graph_arg_regs_t;


using namespace std;


#define DWORD_LEN 32
#define BYTE_LEN 8
#define WORD_LEN 16

//class state;
//class eqcheck;

void set_context(context*);
//void solver_init(void);


expr_ref expr_true(context* ctx);
expr_ref expr_false(context* ctx);
expr_ref expr_bvadd(expr_ref, expr_ref);
expr_ref expr_bvsub(expr_ref, expr_ref);
expr_ref expr_bvmul(expr_ref, expr_ref);

expr_ref expr_not(expr_ref);
expr_ref expr_and(expr_ref, expr_ref);
expr_ref expr_list_and(const expr_list& es, expr_ref in);
//expr_ref expr_pair_list_and(list<pair<expr_ref, expr_ref>> const &ls, expr_ref in);
expr_ref expr_or(expr_ref, expr_ref);
expr_ref expr_list_or(const expr_list& es, expr_ref in);
expr_ref expr_implies(expr_ref, expr_ref);
expr_ref expr_eq(expr_ref, expr_ref);
expr_ref expr_ite(expr_ref cond, expr_ref e1, expr_ref e2);
expr_ref expr_bvneg(expr_ref);

string expr_string(expr_ref);
string get_unique_name(string prefix);

//expr_ref expr_substitute(expr_ref e, map<unsigned, pair<expr_ref, expr_ref> > const &submap);
expr_ref expr_substitute(expr_ref e, const expr_vector& from, const expr_vector& to);
expr_ref expr_substitute_using_state(expr_ref e/*, state const &from*/, state const &to);
expr_ref expr_substitute(expr_ref e, expr_ref from, expr_ref to);
expr_ref expr_substitute(expr_ref e, vector<pair<expr_ref, expr_ref> > const &from_to);
pair<expr_ref, map<expr_id_t,pair<expr_ref,expr_ref>>> expr_substitute_and_get_pruned_submap_using_states(expr_ref const& e, state const &from, state const &to);

//expr_ref expr_substitute_till_fixpoint(expr_ref e, map<unsigned, pair<expr_ref, expr_ref> > const &submap);
//expr_ref expr_substitute_till_fixpoint(expr_ref e, vector<pair<expr_ref, expr_ref> > const &from_to);

bool is_expr_boolean(expr_ref e);
bool is_expr_bitvector32(expr_ref e);
bool is_expr_array(expr_ref e);

expr_ref expr_get_minus(expr_ref e);

string sort_to_string(expr_ref e);

//bool is_expr_equal(expr_ref e1, expr_ref e2, const query_comment& qc, bool timeout_res, counter_example_t &counter_example, consts_struct_t const &cs);
bool is_expr_equal_syntactic(expr_ref const &e1, expr_ref const &e2);
bool is_expr_not_equal_syntactic(expr_ref const &e1, expr_ref const &e2);
bool is_expr_bv_greatereq_syntactic(expr_ref const &e1, expr_ref const &e2);
bool is_expr_bv_lesseq_syntactic(expr_ref const &e1, expr_ref const &e2);
//bool is_expr_equal_syntactic_mod_memlabels(expr_ref e1, expr_ref e2);
bool is_sort_equal(expr_ref const &e1, expr_ref const &e2);
//bool is_expr_numeral(expr_ref const &e);

//bool is_expr_equal_using_theorems_syntactic(expr_ref e1, expr_ref e2, vector<pair<expr_ref, expr_ref> > const &theorems, query_comment const &qc);

//string get_counter_example_string();

bool is_expr_bitvector(expr_ref e);

double expr_compute_dof(expr_ref e);

expr_ref expr_bvconcat(expr_ref a, expr_ref b);
expr_ref expr_bvconcat(list<expr_ref> const &a);
expr_ref expr_bvextract(unsigned high, unsigned low, expr_ref e);
expr_ref expr_bvgt(expr_ref a, expr_ref b);
expr_ref expr_bvge(expr_ref a, expr_ref b);
expr_ref expr_bvle(expr_ref a, expr_ref b);
bool expr_constants_agree(expr_ref a, expr_ref b);

//class ref_object
//{
//public:
//  ref_object() : m_ref_count(0) {}
//  void inc_ref_count() { ++m_ref_count; }
//  void dec_ref_count() { --m_ref_count; }
//  unsigned get_ref_count()  { return m_ref_count; }
//
//private:
//  unsigned m_ref_count;
//};

class expr_sort// : public ref_object
{
public:
  enum kind
  {
    BV_SORT,
    BOOL_SORT,
    ARRAY_SORT,
    //IO_SORT,
    FUNCTION_SORT,
    INT_SORT,
    MEMLABEL_SORT,
    COMMENT_SORT,
    MEMACCESS_LANGTYPE_SORT,
    MEMACCESS_SIZE_SORT,
    MEMACCESS_TYPE_SORT,
    REGID_SORT,
    UNDEFINED_SORT,
  };
  expr_sort(kind, context*);
  expr_sort(kind, unsigned, context*);
  expr_sort(kind, vector<sort_ref> domain, sort_ref range, context*);
  ~expr_sort();

  sort_ref dup_sort(context* nctx) const;

  bool operator==(expr_sort const &) const;
  string to_string() const;
  unsigned get_size() const;
  bool is_undefined_kind() const;
  bool is_bool_kind() const;
  bool is_bv_kind() const;
  bool is_int_kind() const;
  bool is_array_kind() const;
  //bool is_io_kind() const;
  bool is_function_kind() const;
  bool is_memlabel_kind() const;
  //bool is_comment_kind() const;
  bool is_langtype_kind() const;
  bool is_memaccess_size_kind() const;
  bool is_memaccess_type_kind() const;
  bool is_regid_kind() const;
  vector<sort_ref> get_domain_sort() const;
  sort_ref get_range_sort() const;
  void dec_ref_count();
  unsigned get_id() const;
  kind get_kind() const;
  size_t get_mybitset_size() const;
  //void clear_id_to_zero() { m_id = 0; }
  void set_id_to_free_id(expr_id_t suggested_id);
  bool id_is_zero() const { return m_id == 0; }
  context *get_context() const { return m_context; }

  context* get_context() { return m_context; }

private:
  kind m_kind;
  unsigned m_size;
  vector<sort_ref> m_domain;
  sort_ref m_range;
  unsigned m_id;
  context* m_context;  // reference to parent
};

class expr// : public ref_object
{
public:
  enum operation_kind
  {
    OP_VAR = 0,
    OP_CONST,
    OP_EQ,
    OP_IFF,
    OP_OR,
    OP_AND,
    OP_NOT,
    OP_ANDNOT1,
    OP_ANDNOT2,
    OP_XOR,
    OP_IMPLIES,
    OP_ITE,
    OP_BVNOT,
    OP_BVAND,
    OP_BVOR,
    OP_BVNAND,
    OP_BVNOR,
    OP_BVXOR,
    OP_BVXNOR,
    OP_BVNEG,
    OP_BVSUB,
    OP_BVADD,
    OP_BVMUL,
    OP_BVUDIV, //returns the floor of the result if it is positive, returns ceiling of the result if it is negative
    OP_BVSDIV, //returns the floor of the result if it is positive, returns ceiling of the result if it is negative
    OP_BVUREM,
    OP_BVSREM,
    OP_BVCONCAT,
    OP_BVEXTRACT,
    OP_SELECT,
    OP_STORE,
    OP_BVSIGN_EXT,
    OP_BVZERO_EXT,
    OP_BVEXSHL,
    OP_BVEXLSHR,
    OP_BVEXASHR,
    OP_BVROTATE_LEFT,
    OP_BVROTATE_RIGHT,
    OP_BVEXTROTATE_LEFT,
    OP_BVEXTROTATE_RIGHT,
    OP_BVULT,
    OP_BVULE,
    OP_BVSLE,
    OP_BVSLT,
    OP_BVUGT,
    OP_BVUGE,
    OP_BVSGE,
    OP_BVSGT,
    OP_FUNCTION_CALL,
    OP_BVSIGN,
    OP_DONOTSIMPLIFY_USING_SOLVER_SUBSIGN,
    OP_DONOTSIMPLIFY_USING_SOLVER_SUBOVERFLOW,
    OP_DONOTSIMPLIFY_USING_SOLVER_SUBZERO,
    //OP_FUNCTION_ARGUMENT_PTR,
    //OP_RETURN_VALUE_PTR,
    OP_MEMMASK,
    OP_ALLOCA,
    OP_IS_MEMACCESS_LANGTYPE,
    OP_ISLANGALIGNED,
    OP_ISSHIFTCOUNT,
    OP_ISGEPOFFSET,
    OP_ISINDEXFORSIZE,
    OP_ISMEMLABEL,
    OP_BOOL_TO_BV,
    //OP_MEMSPLICE,
    OP_UNDEFINED,
    //OP_MEMJOIN,
    //OP_MEMSPLICE_ARG,
    //OP_SWITCHMEMLABEL,
    OP_MEMMASKS_ARE_EQUAL
  };

  expr(operation_kind kind, const vector<expr_ref>& args, sort_ref const &s, context* ctx);
  expr(operation_kind kind, string_ref const &name, sort_ref const &s, context* ctx);
  expr(operation_kind kind, int val, sort_ref const &s, context* ctx);
  expr(operation_kind kind, uint64_t val, sort_ref const &s, context* ctx);
  expr(operation_kind kind, int64_t val, sort_ref const &s, context* ctx);
  expr(operation_kind kind, mybitset const &val, sort_ref const &s, context* ctx);
  expr(operation_kind kind, sort_ref const &s, context* ctx, string_ref const &val);
  expr(operation_kind kind, sort_ref const &s, context* ctx);
  expr(operation_kind kind, array_constant_ref const &a, sort_ref const &s, context* ctx);
  expr(operation_kind kind, memlabel_t const &ml, sort_ref const &s, context* ctx);
  expr(context* ctx);
  //expr(expr const &other);
  ~expr();
  expr_ref dup_expr(context* nctx) const;

  operation_kind get_operation_kind() const;
  static bool expr_op_is_donotsimplify_using_solver(operation_kind op);
  static bool expr_op_can_be_replaced_by_first_arg(operation_kind op);
  static bool expr_op_is_bvcmp(operation_kind op);
  const vector<expr_ref>& get_args() const;
  unsigned get_id() const;
  bool operator==(expr const &) const;
  string_ref const &get_name() const;
  //void set_name(string const &);
  sort_ref get_sort() const;
  //expr_ref update_sp_memlabels_using_lhs_set(set<pair<expr_ref, pair<expr_ref, expr_ref>>> const &lhs_set, map<symbol_id_t, pair<string, size_t>> const &symbol_map, graph_locals_map_t const &locals_map, set<cs_addr_ref_id_t> const &relevant_addr_refs, consts_struct_t const &cs);
  //expr_ref simplify_for_solver(consts_struct_t const *cs);
  bool is_undefined_sort() const;
  bool is_bool_sort() const;
  bool is_bv_sort() const;
  bool is_array_sort() const;
  //bool is_io_sort() const;
  bool is_int_sort() const;
  bool is_function_sort() const;
  bool is_memlabel_sort() const;
  //bool is_comment_sort() const;
  bool is_langtype_sort() const;
  bool is_memaccess_size_sort() const;
  bool is_memaccess_type_sort() const;
  bool is_regid_sort() const;
  bool is_const() const;
  bool is_const_bool_true() const;
  bool is_const_bool_false() const;
  bool is_highorder_mask_const() const;
  bool is_loworder_mask_const() const;
  bool is_var() const;
  bool is_undefined() const;
  bool is_uninit() const;
  bool is_local_var();
  bool is_common_mem() const;
  memlabel_t get_memlabel_value() const;
  //comment_t get_comment_value() const;
  langtype_ref get_langtype_value() const;
  int get_regid_value() const;
  int get_int_value() const;
  int64_t get_int64_value() const;
  uint64_t get_uint64_value() const;
  array_constant_ref const &get_array_constant() const;
  mybitset const &get_mybitset_value() const;
  //mybitset get_unsigned_mybitset_value() const;
  //mybitset get_signed_mybitset_value() const;
  string get_str_value() const;
  bool get_bool_value() const;
  void dec_ref_count();
  bool is_input_mem(void) const;
  //expr_ref substitute(const expr_vector& from, const expr_vector& to);
  //expr_ref substitute_locals_with_input_stack_pointer_const(consts_struct_t const &cs);
  context* get_context() const;
  //void get_mem_slots(list<tuple<expr_ref, memlabel_t, expr_ref, unsigned, bool>>& addrs);

  bool is_nextpc_const() const;
  nextpc_id_t get_nextpc_num() const;

  bool is_input_reg_id(exreg_group_id_t &group, exreg_id_t &regnum) const;

  //constant_value* get_constant_value();
  bool is_true() const;
  string to_string_node_using_map(map<unsigned, unsigned> const &map) const;
  string to_string_node(bool formatted_const = false) const;

  //void clear_id_to_zero() { m_id = 0; }
  void set_id_to_free_id(expr_id_t suggested_id);
  bool id_is_zero() const { return m_id == 0; }
  static bool operation_is_commutative_and_associative(operation_kind op);
  constant_value const *get_const_value_ptr() const { return m_const_value2.get(); }
private:
  string get_formatted_constant() const;

private:

  vector<expr_ref> m_args;
  operation_kind m_kind;
  sort_ref m_sort;
  //string m_name;
  shared_ptr<constant_value const> m_const_value2;     // OP_CONST
  unsigned m_id;
  context* m_context;  // reference to parent
};

struct expr_ref_cmp_t {
  bool operator()(expr_ref const &a, expr_ref const &b) const {
    return a->get_id() < b->get_id();
  }
};

struct expr_deterministic_cmp_t {
  bool operator()(expr_ref const &a, expr_ref const &b) const {
    //return a->to_string_table() < b->to_string_table(); //takes too much time for eqgen on crafty/Evaluate
    return a->get_id() < b->get_id(); //XXX
  }
};



struct expr_compare {
    bool operator() (const expr_ref& lhs, const expr_ref& rhs) const{
        return lhs->get_id() < rhs->get_id();
    }
};

/*
struct expr_triple_compare {
    bool operator() (pair<expr_ref, pair<expr_ref, expr_ref>> const & lhs, pair<expr_ref, pair<expr_ref, expr_ref>> const &rhs) const{
        if (lhs.first->get_id() < rhs.first->get_id()) {
          return true;
        }
        if (rhs.first->get_id() < lhs.first->get_id()) {
          return false;
        }
        if (lhs.second.first->get_id() < rhs.second.first->get_id()) {
          return true;
        }
        if (rhs.second.first->get_id() < lhs.second.first->get_id()) {
          return false;
        }
        return lhs.second.second->get_id() < rhs.second.second->get_id();
    }
};
*/

//void expr_get_nextpc_num_args_map(expr_ref const &e, map<unsigned, pair<size_t, set<expr_ref, expr_ref_cmp_t>>> &out);
//expr_ref expr_truncate_function_arguments_using_nextpc_num_args_map(expr_ref const &e, map<unsigned, pair<size_t, set<expr_ref, expr_ref_cmp_t>>> const &out);


bool is_logical_bit_operation(expr::operation_kind opkind);
bool is_logical_bv_operation(expr::operation_kind opkind);
bool is_logical_operation(expr::operation_kind opkind);
bool is_bv_arith_operation(expr::operation_kind opkind);
bool is_bv_shiftrotate_operation(expr::operation_kind opkind);

bool is_bv_arithcmp_operation(expr::operation_kind opkind);

expr_ref expr_bv1_to_bool(expr_ref const &);
expr_ref expr_bool_to_bv1(expr_ref const &);

static bool num_seconds_comparer(pair<unsigned, double> const &a, pair<unsigned, double> const &b) {
  return a.second > b.second;
}

expr_ref expr_convert_constant_function_to_array_sort(expr_ref const &e, sort_ref const &s);
//expr_ref expr_convert_constant_bv_to_io_sort(expr_ref e);
//expr_ref expr_convert_function_to_apply_on_io_and_memlabel_sorts(expr_ref e, sort_ref s);
expr_ref expr_convert_function_to_apply_on_memlabel_sort(expr_ref const &e, sort_ref const &s);
//expr_ref expr_erase_unnecessary_elements(expr_ref const &e);

bool expr_get_size_for_regname(expr_ref const &e, string const &regname, size_t &sz);
}

/*namespace std{
using namespace eqspace;
template <> struct equal_to<expr>
{
  bool operator() (expr const &x, expr const &y) const
  {
    return x.equal(y);
  }
};
}*/

namespace std
{
using namespace eqspace;
template<>
struct hash<expr_sort>
{
  std::size_t operator()(expr_sort const &s) const
  {
    std::size_t seed = 0;
    myhash_combine<size_t>(seed, (size_t)s.get_kind());
    if (s.is_bv_kind()) {
      myhash_combine(seed, s.get_size());
    }
    if (s.is_array_kind() || s.is_function_kind()) {
      vector<sort_ref> dom = s.get_domain_sort();
      for (unsigned i = 0; i < dom.size(); ++i) {
        myhash_combine(seed, dom[i]->get_id());
      }
      myhash_combine(seed, s.get_range_sort()->get_id());
    }
    //cout << "hash<expr_sort>::" << __func__ << " " << __LINE__ << ": returning " << seed << " for " << s.to_string() << endl;
    return seed;
  }
};

template<>
struct hash<expr>
{
  std::size_t operator()(expr const &e) const
  {
    std::size_t seed = 0;
    /* TODO: why not just use the id? */
    myhash_combine<size_t>(seed, (size_t)e.get_operation_kind());
    myhash_combine(seed, e.get_sort()->get_id());
    myhash_combine(seed, e.get_const_value_ptr());
    //myhash_combine(seed, e.get_str_value());
    //myhash_combine(seed, e.get_name());
    //cout << __func__ << " " << __LINE__ << ": sort " << e->get_sort()->to_string() << endl;
    //cout << __func__ << " " << __LINE__ << ": e->get_args().size() = " << e->get_args().size() << endl;
    for (unsigned i = 0; i < e.get_args().size(); i++) {
      myhash_combine(seed, e.get_args()[i]->get_id());
    }
    return seed;
  }
};

template<>
struct hash<expr_ref>
{
  std::size_t operator()(expr_ref const e) const
  {
    std::size_t seed = 0;
    myhash_combine<int>(seed, e->get_id());
    return seed;
  }
};

template<>
struct hash<constant_value>
{
  std::size_t operator()(constant_value const &c) const
  {
    std::size_t seed = 0;
    myhash_combine(seed, c.get_string_ref().get());
    if (c.is_array_const())
      myhash_combine(seed, c.get_array());
    else
      myhash_combine(seed, c.get_mybitset());
    return seed;
  }
};

template<>
struct hash<array_constant>
{
  std::size_t operator()(array_constant const &c) const
  {
    std::size_t seed = 0;
    myhash_combine(seed, c.get_default_value());
    for (auto const& mp : c.get_mapping()) {
      for (auto const& mpe : mp.first) {
        myhash_combine(seed, mpe);
      }
      myhash_combine(seed, mp.second);
    }
    for (auto const& mp : c.get_range_mapping()) {
      myhash_combine(seed, mp.first.first);
      myhash_combine(seed, mp.first.second);
      myhash_combine(seed, mp.second);
    }
    return seed;
  }
};

template<>
struct hash<array_constant_default_value_t>
{
  std::size_t operator()(array_constant_default_value_t const &c) const
  {
    std::size_t seed = 0;
    myhash_combine(seed, c.is_RAC());
    if(c.is_RAC()) {
      myhash_combine(seed, c.get_adder_value());
      myhash_combine(seed, c.get_multiplier_value());
    }
    else
      myhash_combine(seed, c.get_default_value());
    return seed;
  }
};

}

/*namespace std
{
using namespace eqspace;
template <> struct equal_to<expr_sort>
{
  bool operator() (expr_sort const &x, expr_sort const &y) const
  {
    return x.equal(y);
  }
};
}*/

namespace eqspace{
class expr_visitor
{
public:
  void visit_recursive(expr_ref const &e);
  virtual void previsit(expr_ref const &e, int interesting_args[], int &num_interesting_args);
  virtual void visit(expr_ref const &e) = 0;

  //bool m_visit_each_expr_only_once;
  virtual ~expr_visitor() = default;

private:
  set<expr_id_t> m_visited;
};

/*class expr_printer_dot_file : public expr_visitor
{
public:
  expr_printer_dot_file(expr_ref e) { m_expr = e; m_visit_each_expr_only_once = true; }
  void get_string();

private:
  virtual void visit(expr_ref e);
  expr_ref m_expr;
  stringstream m_ss;
};*/


/*class expr_printer_dot : public expr_visitor
{
public:
  expr_printer_dot(expr_ref e, consts_struct_t const &cs) : m_cs(cs) { m_visit_each_expr_only_once = true; m_expr = e;  }
  string get_string();

private:
  void visit(expr_ref e);
  expr_ref m_expr;
  consts_struct_t const &m_cs;
  //map<string, expr_ref> m_addr_refs;
  map<unsigned, pair<string, pair<string, unsigned> > > m_expr_string;
  map<string, map<unsigned, pair<string, string> > > m_references;
};*/

/*
class expr_specialize
{
public:
  expr_specialize(expr_ref pred) { m_pred = pred; }
  expr_ref get_specialized(const expr_ref e, const query_comment& qc);

private:
  expr_ref m_pred;
  map<unsigned, expr_ref> m_visited;
};

class expr_specialize_cond
{
public:
  expr_specialize_cond(expr_ref pred) { m_pred = pred; }
  expr_ref get_specialized(const expr_ref e, const query_comment& qc);

private:
  expr_ref m_pred;
  map<unsigned, expr_ref> m_visited;
};
*/

class free_id_list
{
public:
  free_id_list() { m_free_ids.insert(make_pair(1, UINT_MAX)); }
  expr_id_t get_free_id(expr_id_t suggested_id = 0);
  void add_free_id(expr_id_t id);

private:
  set<pair<expr_id_t, expr_id_t>> m_free_ids; //set of id ranges
  //set<expr_id_t> m_used_ids;
  //set<expr_id_t> m_free_ids;
  //expr_id_t m_max_limit_used;
};

class expr_map_cache
{
public:
  struct entry
  {
    expr_ref m_expr;
    expr_ref m_expr_simpl;
    double m_substitute_num_seconds;
    unsigned m_count;
  };

  bool find_result(expr_ref const &e, expr_ref& e_simpl)
  {
    unsigned id = e->get_id();
    if(m_entries.count(id) > 0)
    {
      e_simpl = m_entries.at(id).m_expr_simpl;
      ++m_entries[id].m_count;
      return true;
    }
    else return false;
  }

  void set_result(expr_ref const &e, expr_ref const &e_simpl, double num_seconds)
  {
    unsigned id = e->get_id();
    m_entries[id].m_expr = e;
    m_entries[id].m_count = 0;
    m_entries[id].m_expr_simpl = e_simpl;
    m_entries[id].m_substitute_num_seconds = num_seconds;
  }

  void clear(void) { m_entries.clear(); }

  void print_stats(string name) {
    cout << "Printing expr_map_stats for " << name << endl;
    vector<pair<unsigned, double> > runtimes;
    for (map<unsigned, entry>::const_iterator iter = m_entries.begin();
         iter != m_entries.end();
         iter++) {
      runtimes.push_back(make_pair((*iter).first, (*iter).second.m_substitute_num_seconds));
    }
    std::sort(runtimes.begin(), runtimes.end(), num_seconds_comparer);
    printf("Substitute time : Num Hits\n");
    int i = 0;
    for (vector<pair<unsigned, double> >::const_iterator iter = runtimes.begin();
         iter != runtimes.end();
         iter++, i++) {
      unsigned id = (*iter).first;
      printf("%d. %.2f : %u\n", i, m_entries.at(id).m_substitute_num_seconds,
           m_entries.at(id).m_count);/*, expr_string(m_entries.at(id).m_expr).c_str(),
           expr_string(m_entries.at(id).m_expr_simpl).c_str()*/
    }
    printf("Printing ten slowest substitutions:\n");
    i = 0;
    for (vector<pair<unsigned, double> >::const_iterator iter = runtimes.begin();
         iter != runtimes.end() && i < 10;
         iter++, i++) {
      unsigned id = (*iter).first;
      printf("Slowest substitution %d key\n%s\nvalue\n%s\n", i, expr_string(m_entries.at(id).m_expr).c_str(), expr_string(m_entries.at(id).m_expr_simpl).c_str());
    }

  }

  string stats() const
  {
    stringstream ss;
    ss << "expr_simplify_cache entry count: " << m_entries.size() << endl;
    return ss.str();
  }


private:
  map<unsigned, entry> m_entries;
};

/*class context : public context
{
public:
  context(const config& cfg) : context(cfg)
  {
  }

  ~context()
  {
    //m_theo_from.clear();
    //m_theo_to.clear();
    //m_theorems.clear();
    //m_theo_substitute_cache.clear();
    //m_theo_submap.clear();
  }
private:
};*/

/*class expr_table_visitor
{
public:
  expr_table_visitor(context* ctx) { m_context = ctx; }
  virtual void visit(expr_ref e) = 0;
  //void start_visiting();

  context* m_context;
};*/

string op_kind_to_string(expr::operation_kind op);
expr::operation_kind op_string_to_kind(string str);

bool to_value_depends_on_from_value(expr_ref const &to_value, expr_ref const &from_value);

//int expr_vector_search(expr_vector const &haystack, expr_ref needle, bool timeout_res);

#define MAX_G_PROBES 3
extern expr_ref g_probe[MAX_G_PROBES];

//extern expr_vector g_theo_from;
//extern expr_vector g_theo_to;
//extern expr_list g_theorems;
//extern expr_map_cache g_theo_substitute_cache;
//extern map<unsigned, expr_ref> g_theo_submap;

vector<expr_ref> make_args(expr_ref const &a);
vector<expr_ref> make_args(expr_ref const &a, expr_ref const &b);
vector<expr_ref> make_args(expr_ref const &a, expr_ref const &b, expr_ref const &c);
vector<expr_ref> make_args(expr_ref const &a, expr_ref const &b, expr_ref const &c, expr_ref const &d);
vector<expr_ref> make_args(expr_ref const &a, expr_ref const &b, expr_ref const &c, expr_ref const &d, expr_ref const &e);
vector<expr_ref> make_args(expr_ref const &a, expr_ref const &b, expr_ref const &c, expr_ref const &d, expr_ref const &e, expr_ref const &f);
vector<expr_ref> make_args(expr_ref const &a, expr_ref const &b, expr_ref const &c, expr_ref const &d, expr_ref const &e, expr_ref const &f, expr_ref const &g);
//void expr_pair_vector_get_second(expr_vector &out, vector<pair<expr_ref, expr_ref> > const &in);
//void expr_pair_vector_get_first(expr_vector &out, vector<pair<expr_ref, expr_ref> > const &in);

//char const *operation_kind_to_string(expr::operation_kind kind);
//void expr_set_memlabels_to_top_or_constant_values(expr_ref const &e, graph_symbol_map_t const &symbol_map, graph_locals_map_t const &locals_map/*, set<cs_addr_ref_id_t> const &relevant_memlabels*/, consts_struct_t const &cs, bool update_callee_memlabels, sprel_map_t const *sprel_map, map<graph_loc_id_t, expr_ref> const &locid2expr_map, string const &graph_name, long &mlvarnum, graph_memlabel_map_t &memlabel_map);
//bool expr_replace_needle_with_keyword(expr_ref const &e, expr_ref const &needle, string const &dummy_keyword, expr_ref &new_e);
//extern map<unsigned, pair<expr_ref, list<pair<bool, set<expr_ref, expr_compare> > > > > g_expr_list_lhs_prove_rhs_cache;
//bool expr_list_lhs_prove_rhs(expr_list lhs, expr_ref rhs, query_comment const &comment, bool timeout_res);
bool expr_func_has_bv_output(expr_ref const &e);
bool expr_func_has_array_output(expr_ref const &e);
//expr_ref expr_substitute_using_theorems(expr_ref e, map<unsigned, pair<expr_ref, expr_ref> > const &theorems_submap);
//int expr_var_is_pred_dependency(expr_ref const &e);
//expr_ref expr_simplify_const(expr_ref e);
string read_expr(istream& db, expr_ref& e, context* ctx);
string read_expr_and_expr_map(istream& db, expr_ref &e, map<expr_id_t, expr_ref> &emap, context* ctx);
bool str_to_reg_type_index(const string& line, reg_type& rt, int& i, int& j);
//expr_ref expr_transform_function_arguments_to_avoid_stack_pointers(expr_ref in, const void *aux);
//expr_ref expr_convert_function_argument_local_memlabels(expr_ref const &in, consts_struct_t const *cs);
//expr_ref expr_eliminate_io_arguments_from_function_calls(expr_ref in, consts_struct_t const &cs);
//expr_ref expr_rename_locals_to_stack_pointer(expr_ref in, const void *aux);
//bool expr_contains_memlabel_top(expr_ref const &e);
//bool expr_contains_memlabel_bottom(expr_ref const &e);
//void expr_set_comments_to_zero(expr_ref &e);
//void expr_append_insn_id_to_comments(expr_ref &e, int insn_id);
size_t expr_size(expr_ref e);
size_t expr_height(expr_ref e);
expr_ref expr_expand_calls_to_gcc_standard_function(expr_ref e, unsigned nextpc_num, char const *function_name, expr_ref stack_pointer/*, graph_memlabel_map_t &memlabel_map*/, string const &graph_name, long &max_memlabel_varnum);

struct memsplice_args_sort_less_than_t {
  inline bool operator()(pair<expr_ref, expr_ref> const &a, pair<expr_ref, expr_ref> const &b) {
    //cout << __func__ << " " << __LINE__ << ": a = " << expr_string(a) << endl;
    //assert(a->get_operation_kind() == expr::OP_MEMSPLICE_ARG);
    //assert(a->get_operation_kind() == expr::OP_MEMMASK);
    memlabel_t const &ml_a = a.first->get_memlabel_value();
    memlabel_t const &ml_b = b.first->get_memlabel_value();
    return memlabel_t::memlabel_less(&ml_a, &ml_b);
  }
};

//set<pair<expr_ref, pair<expr_ref, expr_ref>>/*, expr_triple_compare*/> expr_triple_list_to_expr_triple_set(list<pair<expr_ref, pair<expr_ref, expr_ref>>> const &ls);
bool expr_set_contains(set<expr_ref, expr_compare> const &haystack, set<expr_ref, expr_compare> const &needle);
//expr_ref expr_simplify_solver(expr_ref e, consts_struct_t const &cs);
//void get_atomic_memlabels(memlabel_t const &in, vector<memlabel_t> &out);
bool expr_is_esp_version(expr_ref const& e);
//void expr_vector_pair_remove_duplicates(vector<pair<expr_ref, expr_ref>> &fromto);
bool mem_accesses_cmp(pair<string, pair<expr_ref, pair<unsigned, bool>>> const &a,
                      pair<string, pair<expr_ref, pair<unsigned, bool>>> const &b);
bool mem_accesses_eq(pair<string, pair<expr_ref, pair<unsigned, bool>>> const &a,
                     pair<string, pair<expr_ref, pair<unsigned, bool>>> const &b);
expr_list expr_compute_implicit_exprs(expr_ref const &e, set<memlabel_t> const &relevant_memlabels, consts_struct_t const &cs);
//pair<expr_ref, set<graph_loc_id_t>> expr_find_dependencies(expr_ref e, map<expr_id_t, graph_loc_id_t> const &locid2expr_map);
bool expr_contains_array_constant(expr_ref e);
bool expr_contains_only_symbols_args(expr_ref e);
//bool expr_contains_empty_memlabel_in_select_or_store_op(expr_ref e);

//pair<bool,expr_ref> expr_try_evaluate_on_counter_example(expr_ref e, counter_example_t &counter_example, bool check_out_of_bounds = false);
//expr_ref expr_evaluate_on_counter_example(expr_ref e, counter_example_t &counter_example);
map<expr_id_t, expr_ref> evaluate_counter_example_on_expr_map(map<expr_id_t, expr_ref> const &expr_map, set<memlabel_ref> const& relevant_memlabels, counter_example_t const& counter_example);

//extern bool SIMPLIFY_DISABLE_CACHES;

void expr_identify_local_variables(expr_ref e, set<local_id_t> &locals, consts_struct_t const &cs);
void expr_identify_address_taken_local_variables(expr_ref e, set<local_id_t> &locals, consts_struct_t const &cs);
void expr_identify_stack_pointer_relative_expressions(expr_ref e, set<expr_ref> &exprs, consts_struct_t const &cs);
map<nextpc_id_t, set<unsigned>> expr_gen_nextpc_farg_pos_map(expr_ref haystack, expr_ref needle);
bool nextpc_farg_pos_maps_are_compatible(map<nextpc_id_t, set<unsigned>> const &a, map<nextpc_id_t, set<unsigned>> const &b);
//bool memlabel_is_local_or_symbol(memlabel_t const &ml, tuple<expr_ref, size_t, memlabel_t> &t, consts_struct_t const &cs);
//memlabel_t memlabel_remove_rodata_symbols(memlabel_t const &ml, eqspace::graph_symbol_map_t const &symbol_map);
bool expr_reads_heap_or_symbols(expr_ref e);
bool expr_writes_heap_or_symbols(expr_ref e);
string to_string_node_node(string op, vector<string> args);
bool string_is_register_id(string const &s, exreg_group_id_t &group, exreg_id_t &regnum);
expr_ref expr_introduce_src_dst_prefix(expr_ref const &e, char const *prefix);
list<expr_ref> expr_get_arg_list(expr_ref const &e);
//set<graph_loc_id_t> expr_get_pred_loc_ids(expr_ref e);
bool is_overlapping_syntactic(context* ctx, expr_ref const &a1, int a1nbytes, expr_ref const &b1, int b1nbytes);
bool expr_contains_only_constants_or_arguments_or_esp_versions(expr_ref e, consts_struct_t const &cs, graph_arg_regs_t const &argument_regs);

}

extern class context *g_ctx;
pair<expr_ref, map<expr_id_t, expr_ref>> parse_expr(context* ctx, const char* str);

#define check_expr_size(e, ret) do { \
  /*if (expr_size(ret) > expr_size(e)) { \
    cout << __func__ << " " << __LINE__ << ": simplifying expression of size " << expr_size(e) << ", height " << expr_height(e) << endl; \
    cout << __func__ << " " << __LINE__ << ": returned expression of size " << expr_size(ret) << ", height " << expr_height(ret) << endl << ret->to_string_table() << endl << "orig:" << endl << e->to_string_table() << endl;; \
  } */\
} while(0)

void g_ctx_init();
typedef expr_ref memvar_t;
