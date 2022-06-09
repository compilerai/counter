#include "support/debug.h"
#include "support/cl.h"
#include "support/globals.h"
#include "support/read_source_files.h"
#include "expr/consts_struct.h"
#include "gsupport/tfg_edge.h"
#include "tfg/tfg_llvm.h"
#include "ptfg/llptfg.h"

#include <fstream>
#include <iostream>
#include <string>

using namespace std;

class expr_compares_a_malloced_var_against_null_visitor : public expr_visitor {
public:
  expr_compares_a_malloced_var_against_null_visitor(expr_ref const &e, set<string_ref> const& unchecked_malloced_vars) : m_expr(e), m_unchecked_malloced_vars(unchecked_malloced_vars), m_ctx(e->get_context())
  {
    visit_recursive(e);
  }

  pair<string_ref, bool> get_result() const
  {
    if (!m_result_map.count(m_expr->get_id())) {
      return make_pair(nullptr, false);
    }
    return m_result_map.at(m_expr->get_id());
  }

private:
  virtual void visit(expr_ref const &e);

  expr_ref m_expr;
  set<string_ref> const& m_unchecked_malloced_vars;
  context* m_ctx;
  map<expr_id_t, pair<string_ref, bool>> m_result_map;
};

void
expr_compares_a_malloced_var_against_null_visitor::visit(expr_ref const& e)
{
  if (e->get_operation_kind() == expr::OP_EQ) {
    if (   e->get_args().at(0)->is_var()
        && m_unchecked_malloced_vars.count(e->get_args().at(0)->get_name())
        && e->get_args().at(1)->is_const()
        && e->get_args().at(1) == m_ctx->mk_zerobv(e->get_args().at(1)->get_sort()->get_size())) {
      string name = e->get_args().at(0)->get_name()->get_str();
      ASSERT(string_has_prefix(name, G_INPUT_KEYWORD "."));
      name = name.substr(strlen(G_INPUT_KEYWORD "."));
      m_result_map.insert(make_pair(e->get_id(), make_pair(mk_string_ref(name), false)));
    }
  } else if (e->get_operation_kind() == expr::OP_NOT) {
    if (m_result_map.count(e->get_args().at(0)->get_id())) {
      pair<string_ref, bool> const& child_result = m_result_map.at(e->get_args().at(0)->get_id());
      m_result_map.insert(make_pair(e->get_id(), make_pair(child_result.first, !child_result.second)));
    }
  }
}

class typecheck_val_t
{
private:
  set<string_ref> m_unchecked_malloced_variables;
  map<string_ref,pair<string_ref,bool>> m_condition_variables_checking_malloced_variables_against_null;
  bool m_has_type_error = false;
  bool m_is_top = false;
public:
  static typecheck_val_t top() { typecheck_val_t ret; ret.m_is_top = true; return ret; }
  set<string_ref> const& get_unchecked_malloced_variables() const { return m_unchecked_malloced_variables; }
  map<string_ref,pair<string_ref,bool>> const& get_condition_variables_checking_malloced_variables_against_null() const { return m_condition_variables_checking_malloced_variables_against_null; }

  string to_string(string const& prefix) const
  {
    stringstream ss;
    ss << prefix << "Unchecked malloc variables\n";
    for (auto const& var : m_unchecked_malloced_variables) {
      ss << prefix << "  " << var->get_str() << endl;
    }
    ss << prefix << "Condition variables checking malloced variables against null\n";
    for (auto const& cond : m_condition_variables_checking_malloced_variables_against_null) {
      ss << prefix << "  " << cond.first->get_str() << " : " << cond.second.first->get_str() << ", " << cond.second.second << endl;
    }
    return ss.str();
  }
  bool meet(typecheck_val_t const& other)
  {
    ASSERT(!other.m_is_top);
    if (this->m_is_top) {
      *this = other;
      return true;
    }
    auto old_unchecked_malloced_variables = m_unchecked_malloced_variables;
    //set_intersect(m_unchecked_malloced_variables, other.m_unchecked_malloced_variables);
    set_union(m_unchecked_malloced_variables, other.m_unchecked_malloced_variables);
    m_has_type_error = m_has_type_error || other.m_has_type_error;
    return old_unchecked_malloced_variables != m_unchecked_malloced_variables;
  }
  bool has_type_error() const { return m_has_type_error; }
  void add_unchecked_malloc_varname(string_ref const& varname)
  {
    m_unchecked_malloced_variables.insert(varname);
  }
  void check_deref(string_ref const& varname)
  {
    //cout << _FNLN_ << ": " << this->to_string("  ");
    //cout << _FNLN_ << ": varname = " << varname->get_str() << ", m_unchecked_malloced_variables.count(varname) = " << m_unchecked_malloced_variables.count(varname) << endl;
    if (m_unchecked_malloced_variables.count(varname)) {
      m_has_type_error = true;
    }
  }
  void mark_checked(string_ref const& varname)
  {
    m_unchecked_malloced_variables.erase(varname);
  }
  void capture_dataflow(state const& to_state)
  {
    //if a value X flows into a variable Y, and X is an unchecked malloc varname, then add Y to the set of unchecked malloc varnames
    map<string_ref, expr_ref> const& m = to_state.get_value_expr_map_ref();
    for (auto const& mapping : m) {
      if (mapping.second->is_bool_sort()) {
        string_ref malloced_var;
        bool negated;
        if (malloced_var = expr_compares_a_malloced_var_against_null(mapping.second, m_unchecked_malloced_variables, negated)) {
          m_condition_variables_checking_malloced_variables_against_null.insert(make_pair(mapping.first, make_pair(malloced_var, negated)));
        }
      }
    }
  }
private:
  string_ref expr_compares_a_malloced_var_against_null(expr_ref const& e, set<string_ref> const& unchecked_malloced_vars, bool& negated)
  {
    expr_compares_a_malloced_var_against_null_visitor visitor(e, unchecked_malloced_vars);
    string_ref ret;
    tie(ret, negated) = visitor.get_result();
    return ret;
  }
};

class typecheck_dfa_t : public data_flow_analysis<pc, tfg_node, tfg_edge, typecheck_val_t, dfa_dir_t::forward>
{
public:
  typecheck_dfa_t(dshared_ptr<tfg_llvm_t const> t, map<pc, typecheck_val_t>& in_vals)
    : data_flow_analysis<pc, tfg_node, tfg_edge, typecheck_val_t, dfa_dir_t::forward>(t, in_vals)
  { }

  dfa_xfer_retval_t xfer_and_meet(dshared_ptr<tfg_edge const> const &te, typecheck_val_t const& in, typecheck_val_t &out) override
  {
    string_ref varname;
    typecheck_val_t in_xferred = in;

    dshared_ptr<tfg const> t =  dynamic_pointer_cast<tfg const>(this->m_graph);
    ASSERT(t);
    map<nextpc_id_t, string> const& nextpc_map = t->get_nextpc_map();
    if (varname = is_malloc(te, nextpc_map)) {
      in_xferred.add_unchecked_malloc_varname(varname);
    } else if (varname = is_memaccess(te)) {
      //cout << _FNLN_ << ": " << te->to_string_concise() << ": calling check_deref on " << varname->get_str() << endl;
      in_xferred.check_deref(varname);
    } else if (varname = is_comparison_against_null(te, in.get_condition_variables_checking_malloced_variables_against_null())) {
      in_xferred.mark_checked(varname);
    } else {
      in_xferred.capture_dataflow(te->get_to_state());
    }
    return out.meet(in_xferred) ? DFA_XFER_RETVAL_CHANGED : DFA_XFER_RETVAL_UNCHANGED;
  }
private:
  static string_ref is_malloc(dshared_ptr<tfg_edge const> const &te, map<nextpc_id_t, string> const& nextpc_map)
  {
    //check if the edge contains a call to malloc()
    //cout << _FNLN_ << ": " << te->to_string_concise() << ": contains_function_call = " << te->get_to_state().contains_function_call() << endl;
    if (te->get_to_state().contains_function_call()) {
      expr_ref fcall_target;
      te->get_to_state().get_function_call_nextpc(fcall_target);
      //cout << _FNLN_ << ": " << te->to_string_concise() << ": fcall_target = " << expr_string(fcall_target) << endl;
      if (fcall_target->is_nextpc_const()) {
        nextpc_id_t nextpc_id = fcall_target->get_nextpc_num();
        //cout << _FNLN_ << ": " << te->to_string_concise() << ": nextpc_id = " << nextpc_id << ", nextpc_map.count(nextpc_id) = " << nextpc_map.count(nextpc_id) << endl;
        if (nextpc_map.count(nextpc_id) && nextpc_map.at(nextpc_id) == "malloc") {
          return te->get_to_state().get_key_with_bv_retval_for_function_call();
        }
      }
    }
    return nullptr;
  }
  static string_ref is_memaccess(dshared_ptr<tfg_edge const> const &te)
  {
    //check if the edge contains a dereference (load/store)
    set<state::mem_access> mas = te->get_all_mem_accesses("ctypecheck");
    for (auto const& ma : mas) {
      if (ma.is_select() || ma.is_store()) {
        expr_ref const& addr = ma.get_address();
        //cout << _FNLN_ << ": " << te->to_string_concise() << ": addr = " << expr_string(addr) << endl;
        if (addr->is_var()) {
          string name = addr->get_name()->get_str();
          ASSERT(string_has_prefix(name, G_INPUT_KEYWORD "."));
          return mk_string_ref(name.substr(strlen(G_INPUT_KEYWORD ".")));
        }
      }
    }
    return nullptr;
  }
  static string_ref
  is_comparison_against_null(dshared_ptr<tfg_edge const> const &te, map<string_ref, pair<string_ref,bool>> const& condition_vars_checking_malloced_vars)
  {
    //check if the edge contains a comparison against null. return the variable name being compared
    expr_ref econd = te->get_cond();
    if (econd->is_var()) {
      return check_cond_name(econd->get_name(), false, condition_vars_checking_malloced_vars);
    } else if (econd->get_operation_kind() == expr::OP_NOT && econd->get_args().at(0)->is_var()) {
      return check_cond_name(econd->get_args().at(0)->get_name(), true, condition_vars_checking_malloced_vars);
    } else {
      return nullptr;
    }
  }
  static string_ref
  check_cond_name(string_ref cond_name, bool negated, map<string_ref, pair<string_ref,bool>> const& condition_vars_checking_malloced_vars)
  {
    if (!condition_vars_checking_malloced_vars.count(cond_name)) {
      return nullptr;
    }
    if (condition_vars_checking_malloced_vars.at(cond_name).second == negated) {
      return condition_vars_checking_malloced_vars.at(cond_name).first;
    }
    return nullptr;
  }
};

int main(int argc, char **argv)
{
  // command line args processing
  cl::arg<string> cfile(cl::implicit_prefix, "", "path to C source code file");
  cl::arg<string> debug(cl::explicit_prefix, "debug", "", "Enable dynamic debugging for debug-class(es).  Expects comma-separated list of debug-classes with optional level e.g. --debug=correlate=2,smt_query=1,dumptfg,decide_hoare_triple,update_invariant_state_over_edge");
  cl::cl cmd("ctypecheck");

  cmd.add_arg(&cfile);
  cmd.add_arg(&debug);

  cl::arg<string> xml_output_format_str(cl::explicit_prefix, "xml-output-format", "text-color", "xml output format to use during xml printing [html|text-color|text-nocolor]");
  cmd.add_arg(&xml_output_format_str);

  cl::arg<string> xml_output_file(cl::explicit_prefix, "xml-output", "", "Dump XML output to this file");
  cmd.add_arg(&xml_output_file);

  cl::arg<string> tmpdir_path(cl::explicit_prefix, "tmpdir-path", "", "Include path for C source compilation");
  cmd.add_arg(&tmpdir_path);

  cmd.parse(argc, argv);

  init_dyn_debug_from_string(debug.get_value());

  context::xml_output_format_t xml_output_format = context::xml_output_format_from_string(xml_output_format_str.get_value());

  if (xml_output_file.get_value() != "") {
    g_xml_output_stream.open(xml_output_file.get_value());
    ASSERT(g_xml_output_stream.is_open());
    g_xml_output_stream << "<typechecker>";
  }


  context::config cfg(3600, 3600);
  context ctx(cfg);
  //consts_struct_t cs;
  //cs.parse_consts_db(&ctx);
  ctx.parse_consts_db(CONSTS_DB_FILENAME);
  consts_struct_t &cs = ctx.get_consts_struct();

  string filename_llvm = cfile.get_value();

  string etfg_filename = convert_source_file_to_etfg(cfile.get_value(), xml_output_file.get_value(), xml_output_format_str.get_value(), "ALL", false /*model_llvm_semantics*/, 0 /* call-context-depth */, "", tmpdir_path.get_value());
  ifstream in_llvm(etfg_filename);
  if (!in_llvm.is_open()) {
    cout << __func__ << " " << __LINE__ << ": Could not open tfg_filename '" << filename_llvm << "'\n";
  }
  ASSERT(in_llvm.is_open());
  //llptfg_t llptfg = llptfg_t::read_from_stream(in_llvm, &ctx);
  llptfg_t llptfg(in_llvm, &ctx);
  in_llvm.close();

  auto const& function_tfg_map = llptfg.get_function_tfg_map();

  set<string> failing_functions;
  for (auto const& fname_tfg : function_tfg_map) {
    string const& fname = fname_tfg.first->call_context_get_cur_fname()->get_str();
    dshared_ptr<tfg> const& t = fname_tfg.second;
    dshared_ptr<tfg_llvm_t> t_llvm = dynamic_pointer_cast<tfg_llvm_t>(t);
    ASSERT(t_llvm);

    map<pc, typecheck_val_t> pc_vals_map;
    pc_vals_map.insert(make_pair(pc::start(), typecheck_val_t()));
    typecheck_dfa_t typecheck_dfa(t_llvm, pc_vals_map);
    typecheck_dfa.solve();

    //for (auto const& p : t_llvm->get_all_pcs()) {
    //  cout << _FNLN_ << ": " << fname << ": " << p.to_string() << ":\n" << pc_vals_map.at(p).to_string("  ") << endl;
    //}

    for (auto const& exit_pc : t_llvm->get_exit_pcs()) {
      if (pc_vals_map.at(exit_pc).has_type_error()) {
        stringstream ss;
        ss << "Found type error in function '" << fname << "'.";
        cout << ss.str();
        MSG(ss.str().c_str());
        failing_functions.insert(fname);
        break;
      } else {
        stringstream ss;
        ss << "Type check succeeded for function '" << fname << "'.";
        cout << ss.str() << endl;
        MSG(ss.str().c_str());
      }
    }
  }

  if (g_xml_output_stream.is_open()) {
    g_xml_output_stream << "</typechecker>";
    g_xml_output_stream.close();
  }
  return 0;
}
