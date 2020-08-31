#include "tfg/parse_input_eq_file.h"
#include "support/log.h"
#include "support/utils.h"
#include "gsupport/rodata_map.h"
#include "gsupport/sprel_map.h"
#include "expr/sprel_map_pair.h"
#include "tfg/tfg_llvm.h"
#include "tfg/tfg_asm.h"
#include "graph/te_comment.h"
#include "gsupport/parse_edge_composition.h"
#include "rewrite/dst-insn.h"

#include <fstream>

#define DEBUG_TYPE "parse-eq"
namespace eqspace
{

using namespace std;

bool is_next_line(istream& in, const string& keyword)
{
  string line;
  getline(in, line);
  return is_line(line, keyword);
}

/*string read_in_out_llvm(ifstream& in, tfg& t, context* ctx)
{
  vector<expr_ref> args;
  map<pc, vector<expr_ref>, pc_comp> rets;
  string line;
  line = read_in_out_values(in, t, ctx, args, rets);
  //cout << __func__ << " " << __LINE__ << ": rets.size() = " << rets.size() << endl;
  t.set_argument_regs(args);
  t.set_return_regs(rets);
  return line;
}*/

//string
//read_pred_and_insert_into_set(istream &in, tfg const *t, context *ctx, unordered_set<predicate> &theos)
//{
//  string line, comment;
//  predicate::proof_status status;
//  bool end;
//  consts_struct_t const &cs = ctx->get_consts_struct();
//
//  //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
//  end = !!getline(in, line);
//  ASSERT(end);
//  //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
//  ASSERT(is_line(line, "Comment"));
//  end = !!getline(in, line);
//  ASSERT(end);
//  comment = line;
//  //cout << __func__ << " " << __LINE__ << ": comment = " << comment << endl;
//  end = !!getline(in, line);
//  ASSERT(end);
//  ASSERT(is_line(line, "Status"));
//  end = !!getline(in, line);
//  ASSERT(end);
//  status = predicate::status_from_string(line);
//  //cout << __func__ << " " << __LINE__ << ": status = " << predicate::status_to_string(status) << endl;
//  end = !!getline(in, line);
//  ASSERT(end);
//  ASSERT(is_line(line, "LocalSprelAssumptions"));
//  local_sprel_expr_guesses_t lsprel_guesses(cs);
//  line = read_local_sprel_expr_assumptions(in, ctx, lsprel_guesses);
//  ASSERT(is_line(line, "Guard"));
//  //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
//  edge_guard_t guard_e;
//  line = edge_guard_t::read_edge_guard(in, guard_e, t);
//  ASSERT(is_line(line, "LhsExpr"));
//  //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
//  expr_ref lhs_e;
//  line = read_expr(in, lhs_e, ctx);
//  //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
//  ASSERT(is_line(line, "RhsExpr"));
//  //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
//  expr_ref rhs_e;
//  line = read_expr(in, rhs_e, ctx);
//
//  precond_t precond(guard_e, lsprel_guesses);
//  predicate p(precond, lhs_e, rhs_e, comment, status);
//  //p.set_local_sprel_expr_assumes(lsprel_guesses);
//  theos.insert(p);
//  return line;
//}


/*static string
read_alias_map(ifstream& in, context* ctx, string line, map<expr_ref, memlabel_t> &alias_map)
{
  while (is_line(line, "=alias_map")) {
    getline(in, line);
    ASSERT(is_line(line, "=expr"));
    expr_ref e;
    line = read_expr(in, e, ctx);
    ASSERT(is_line(line, "=memlabel"));
    getline(in, line);
    line = line.substr(0, line.size() - 1);
    memlabel_t ml;
    memlabel_from_string(&ml, line.c_str());
    alias_map[e] = ml;
    getline(in, line);
  }
  return line;
}*/

//string
//read_edge_with_cond_using_start_state(istream &in, string const& first_line, context *ctx, string const &prefix, state const &start_state, pc& p1, pc& p2, expr_ref& cond, state& state_to)
//{
//  string line/*, comment*/;
//  //pc p1, p2;
//  string comment;
//
//  //cout << __func__ << " " << __LINE__ << ": first_line = " << first_line << ".\n";
//  edge_read_pcs_and_comment(first_line.substr(prefix.size() + 1 /* for colon*/), p1, p2, comment);
//  ASSERT(comment == "");
//
//  getline(in, line);
//  if (!is_line(line, prefix + ".EdgeCond:")) {
//    cout << __func__ << " " << __LINE__ << ": line = " << line << ".\n";
//    cout << __func__ << " " << __LINE__ << ": prefix = " << prefix << ".\n";
//  }
//  assert(is_line(line, prefix + ".EdgeCond:"));
//  //expr_ref cond;
//  line = read_expr(in, cond, ctx);
//
//  assert(is_line(line, prefix + ".StateTo:"));
//  //state state_to;
//  //cout << __func__ << " " << __LINE__ << ": reading state_to, line = " << line << endl;
//  map<string_ref, expr_ref> state_to_value_expr_map = state_to.get_value_expr_map_ref();
//  line = read_state(in, state_to_value_expr_map, ctx, &start_state);
//  state_to.set_value_expr_map(state_to_value_expr_map);
//  state_to.populate_reg_counts(&start_state);
//  return line;
//}

//string
//read_tfg_edge_using_start_state(istream &in, string const& first_line, context *ctx, string const &prefix, state const &start_state, shared_ptr<tfg_edge const>& te)
//{
//  pc from_pc, to_pc;
//  //string comment;
//  state state_to;
//  expr_ref cond;
//  //cout << __func__ << " " << __LINE__ << ": first_line = " << first_line << ".\n";
//  string line = read_edge_with_cond_using_start_state(in, first_line, ctx, prefix, start_state, from_pc, to_pc, cond, state_to);
//  bool end;
//  //cout << __func__ << " " << __LINE__ << ": line = " << line << ".\n";
//  ASSERT(is_line(line, prefix + ".te_comment"));
//  end = !getline(in, line);
//  ASSERT(!end);
//  vector<string> te_comment_fields = splitOnChar(line, ':');
//  ASSERT(te_comment_fields.size() == 5);
//  ASSERT(stringIsInteger(te_comment_fields.at(0)));
//  bbl_order_descriptor_t bo(string_to_int(te_comment_fields.at(0)), te_comment_fields.at(1));
//  ASSERT(stringIsInteger(te_comment_fields.at(2)));
//  bool is_phi = string_to_int(te_comment_fields.at(2));
//  ASSERT(stringIsInteger(te_comment_fields.at(3)));
//  int insn_idx = string_to_int(te_comment_fields.at(3));
//  string comment_str = te_comment_fields.at(4);
//  //cout << __func__ << " " << __LINE__ << ": line = " << line << ".\n";
//  end = !getline(in, line);
//  ASSERT(!end);
//  while (line != "" && line.at(0) != '=') {
//    comment_str += "\n" + line;
//    end = !getline(in, line);
//    ASSERT(!end);
//  }
//  te_comment_t te_comment(bo, is_phi, insn_idx, comment_str);
//  //cout << __func__ << " " << __LINE__ << ": parsed te_comment\n" << te_comment.get_string() << endl;
//  //ASSERT(comment == "");
//  //ASSERT(is_line(line, "=constituent_itfg_edge"));
//  //list<itfg_edge_ref> itfg_edges;
//  //while (is_line(line, "=constituent_itfg_edge")) {
//  //  pc ifrom_pc, ito_pc;
//  //  string icomment;
//  //  state istate_to;
//  //  expr_ref icond;
//
//  //  bool end = !getline(in, line);
//  //  ASSERT(!end);
//  //  line = read_edge_with_cond_using_start_state(in, line, ctx, "=itfg_edge", start_state, ifrom_pc, ito_pc, icond, istate_to);
//  //  ASSERT(is_line(line, "=itfg_edge.Comment"));
//  //  end = !getline(in, line);
//  //  ASSERT(!end);
//  //  ASSERT(line.size() >= 1);
//  //  if (line.at(0) != '=') {
//  //    icomment = line;
//  //    end = !getline(in, line);
//  //    ASSERT(!end);
//  //  }
//  //  itfg_edges.push_back(mk_itfg_edge(ifrom_pc, ito_pc, istate_to, icond, start_state, icomment));
//  //}
//  //if (!is_line(line, "=itfg_ec")) {
//  //  cout << __func__ << " " << __LINE__ << ": line = " << line << ".\n";
//  //}
//  //ASSERT(is_line(line, "=itfg_ec"));
//  //end = !getline(in, line);
//  //ASSERT(!end);
//
//  //std::function<itfg_edge_ref (string const&)> f =
//  //  [&itfg_edges](string const& str)
//  //{
//  //  pc p1, p2;
//  //  string comment;
//  //  ASSERT(str.at(0) == '(');
//  //  ASSERT(str.at(str.size() - 1) == ')');
//  //  string s = str.substr(1, str.size() - 2);
//  //  edge_read_pcs_and_comment(s, p1, p2, comment);
//  //  ASSERT(comment == "");
//  //  for (auto const& ie : itfg_edges) {
//  //    if (   ie->get_from_pc() == p1
//  //        && ie->get_to_pc() == p2) {
//  //      //cout << __func__ << " " << __LINE__ << ": ie = " << ie.to_string_for_eq() << ".\n";
//  //      return ie;
//  //    }
//  //  }
//  //  NOT_REACHED();
//  //};
//  //cout << __func__ << " " << __LINE__ << ": line = " << line << ".\n";
//  //ec = serpar_composition_node_t<itfg_edge>::serpar_composition_node_create_from_string(line, f);
//  //itfg_ec_ref ec = itfg_ec_node_t::itfg_ec_node_create_from_string(line, f);
//  te = make_shared<tfg_edge>(from_pc, to_pc, cond, state_to, te_comment);
//  //end = !getline(in, line);
//  //ASSERT(!end);
//  while (line == "") {
//    bool end = !getline(in, line);
//    ASSERT(!end);
//  }
//  //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
//  return line;
//}
//static string
//read_tfg_edge(istream& in, string const& first_line, tfg& t, string const& prefix)
//{
//  context *ctx = t.get_context();
//  //expr_ref cond/*, indir_tgt*/;
//  //bool is_indir_exit;
//  //state state_to;
//
//  shared_ptr<tfg_edge const> te;
//  string line = read_tfg_edge_using_start_state(in, first_line, ctx, prefix, t.get_start_state(), te);
//  //shared_ptr<tfg_edge> e = make_shared<tfg_edge>(itfg_edge(from_pc, pc_to, state_to, cond, t.get_start_state(), comment));
//  ASSERT(te);
//  t.add_edge(te);
//
//  return line;
//}



//static string
//read_tfg_symbol_map_nodes_and_edges(istream& in, string line, tfg **t, string const &tfg_name, context *ctx, bool is_ssa)
//{
//  consts_struct_t const &cs = ctx->get_consts_struct();
//  if (!is_line(line, "=Symbol-map:")) {
//    cout << __func__ << " " << __LINE__ << ": line =\n" << line << endl;
//  }
//  ASSERT(is_line(line, "=Symbol-map:"));
//  graph_symbol_map_t symbol_map;
//  graph_locals_map_t local_map;
//  getline(in, line);
//  while (string_has_prefix(line, "C_SYMBOL")) {
//    //ASSERT(line.substr(0, strlen("C_SYMBOL")) == "C_SYMBOL");
//    size_t remaining;
//    int symbol_id = stoi(line.substr(strlen("C_SYMBOL")), &remaining);
//    ASSERT(symbol_id >= 0);
//    size_t colon = line.find(':');
//    //ASSERT(remaining  + strlen("C_SYMBOL") == colon);
//    ASSERT(colon != string::npos);
//    size_t colon2 = line.find(':', colon + 1);
//    ASSERT(colon2 != string::npos);
//    string symbol_name = line.substr(colon + 1, colon2 - (colon + 1));
//    trim(symbol_name);
//    size_t colon3 = line.find(':', colon2 + 1);
//    ASSERT(colon3 != string::npos);
//    string symbol_size = line.substr(colon2 + 1, colon3 - (colon2 + 1));
//    trim(symbol_size);
//    size_t size = stoi(symbol_size);
//    size_t colon4 = line.find(':', colon3 + 1);
//    ASSERT(colon4 != string::npos);
//    string symbol_align = line.substr(colon3 + 1, colon4 - (colon3 + 1));
//    trim(symbol_align);
//    align_t align = stoi(symbol_align);
//    string symbol_is_const = line.substr(colon4 + 1);
//    trim(symbol_is_const);
//    bool is_const = (stoi(symbol_is_const) != 0);
//    ASSERT(!symbol_map.count(symbol_id));
//    symbol_map.insert(make_pair(symbol_id, graph_symbol_t(mk_string_ref(symbol_name), size, align, is_const)));
//    getline(in, line);
//  }
//
//  /*if (is_line(line, "=Nextpc-map:")) */{
//    getline(in, line);
//    while (/*!is_line(line, "=Node:")  && */!is_line(line, "=StartState:")) {
//      size_t colon = line.find(':');
//      ASSERT(colon != string::npos);
//      size_t colon2 = line.find(':', colon + 1);
//      ASSERT(colon2 != string::npos);
//      string local_name = line.substr(colon + 1, colon2 - (colon + 1));
//      trim(local_name);
//      string local_size = line.substr(colon2 + 1);
//      size_t size = stoi(local_size);
//      local_map.push_back(make_pair(mk_string_ref(local_name), size));
//      getline(in, line);
//    }
//  }
//
//  //cout << "symbol_map = " << symbol_map << endl;
//  assert(is_line(line, "=StartState:"));
//  //cout << __func__ << " " << __LINE__ << ": reading start_state, line = " << line << endl;
//  map<string_ref, expr_ref> start_state_value_expr_map;
//  line = read_state(in, start_state_value_expr_map, ctx, NULL);
//  //cout << __func__ << " " << __LINE__ << ": st =\n" << st.to_string() << endl;
//  state st;
//  st.set_value_expr_map(start_state_value_expr_map);
//  st.populate_reg_counts(NULL);
//
//  if (is_ssa) {
//    *t = new tfg_ssa_t(mk_string_ref(tfg_name), ctx, st);
//  } else {
//    *t = new tfg(mk_string_ref(tfg_name), ctx, st);
//  }
//
//  (*t)->set_symbol_map(symbol_map);
//  (*t)->set_locals_map(local_map);
// 
//  unordered_set<predicate> alignment_assumes;
//
//  while(is_line(line, "=Node:")) {
//    //cout << __func__ << " " << __LINE__ << endl;
//    DEBUG("reading node: " <<  line << endl);
//    pc p = pc::create_from_string(line.substr(6));
//    //assert(is_next_line(in, "=State:"));
//    //state st;
//    //line = read_state(in, st, ctx);
//    getline(in, line);
//    unordered_set<predicate> theos;
//    map<expr_ref, memlabel_t> alias_map;
//    //cout << __func__ << " " << __LINE__ << endl;
//    //line = read_alias_map(in, ctx, line, alias_map);
//    //cout << __func__ << " " << __LINE__ << endl;
//    line = read_assumes(in, **t, ctx, cs, line, theos);
//    //cout << __func__ << " " << __LINE__ << endl;
//    list<pair<string, expr_ref>> outgoing_values;
//    //cout << __func__ << " " << __LINE__ << endl;
//    line = read_outgoing_values(in, ctx, line, outgoing_values);
//    //cout << __func__ << " " << __LINE__ << endl;
//    //cout << __func__ << " " << __LINE__ << ": adding node with pc " << p.to_string() << endl;
//    shared_ptr<tfg_node> new_node = make_shared<tfg_node>(p/*, st*/);
//    //cout << __func__ << " " << __LINE__ << ": new_node = " << new_node << endl;
//    (*t)->add_node(new_node);
//    //cout << __func__ << " " << __LINE__ << endl;
//    //(*t)->add_alias_map(p, alias_map);
//    //cout << __func__ << " " << __LINE__ << endl;
//    if(alignment_assumes.size() > 0)
//      (*t)->add_assume_preds_at_pc(p, alignment_assumes);
//    (*t)->add_assume_preds_at_pc(p, theos);
//    if(p.is_start())
//      (*t)->extract_align_assumes(theos, alignment_assumes);
//    //cout << __func__ << " " << __LINE__ << endl;
//    //(*t)->add_outgoing_values(p, outgoing_values);
//    //cout << __func__ << " " << __LINE__ << endl;
//  }
//
//  //cout << __func__ << " " << __LINE__ << endl;
//
//  while (line == "") {
//    //cout << __func__ << " " << __LINE__ << endl;
//    getline(in, line);
//  }
//  //cout << __func__ << " " << __LINE__ << endl;
//
//  graph_memlabel_map_t memlabel_map;
//  line = read_memlabel_map(in, ctx, line, memlabel_map);
//  (*t)->set_memlabel_map(memlabel_map);
//  (*t)->compute_max_memlabel_varnum();
//
//  while (line == "") {
//    //cout << __func__ << " " << __LINE__ << endl;
//    getline(in, line);
//  }
//
//  string const tfg_edge_prefix = "=Edge";
//  while (is_line(line, tfg_edge_prefix + ":")) {
//    //cout << __func__ << " " << __LINE__ << ": reading edge: " << line << endl;
//    line = read_tfg_edge(in, line, **t, tfg_edge_prefix);
//    //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
//  }
//  //cout << "done reading edges: line = " << line << endl;
//  return line;
//}

#if 0
string read_tfg_llvm(ifstream& in, tfg& t/*, tfg const &tfg_src*/, context* ctx)
{
  //string line = read_in_out_llvm(in, t, ctx);
  string line = read_in_out(in, t, ctx);
  line = read_tfg_symbol_map_nodes_and_edges(in, line, t, ctx);
  DEBUG(__func__ << " " << __LINE__ << ": line = " << line << endl);
  t.rename_llvm_symbols_to_keywords(ctx/*, tfg_src*/);
  return line;
}
#endif

//static string
//read_locs(istream& in, string line, tfg &t, context *ctx)
//{
//  ASSERT(is_line(line, "=Locs"));
//  //string pc_string = line.substr(6);
//  //pc p = pc::create_from_string(pc_string);
//  getline(in, line);
//  //cout << "line = " << line << ", pc = " << p.to_string() << endl;
//  map<graph_loc_id_t, graph_cp_location> locs;
//  while (is_line(line, "=Loc ")) {
//    graph_loc_id_t loc_id = stoi(line.substr(5));
//    graph_cp_location loc;
//    line = read_loc(in, loc, ctx);
//    locs[loc_id] = loc;
//  }
//  ASSERT(line == "");
//  getline(in, line);
//  //t.graph_set_pc_locs(/*p, */locs);
//  t.set_locs(/*p, */locs);
//  return line;
//}

//static set<graph_loc_id_t>
//read_locset_from_string(string const &s)
//{
//  set<graph_loc_id_t> ret;
//  istringstream iss(s);
//  do {
//    string loc_str;
//    iss >> loc_str;
//    if (loc_str.length() > 0) {
//      graph_loc_id_t loc_id = stoi(loc_str);
//      ret.insert(loc_id);
//    }
//  } while(iss);
//  return ret;
//}

//pair<bool, string>
//read_tfg(istream& in, tfg **t, string const &tfg_name, context* ctx, bool is_ssa)
//{
//  DEBUG("reading tfg:" << endl);
//  //ASSERT(is_line(line, "=Dependencies"));
//  //(*t)->populate_default_pred_min_dependencies_for_loc();
//  //string dependencies_for_edge_prefix = "=Dependencies for edge: ";
//  //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
//  //bool end = !getline(in, line);
//  //cout << __func__ << " " << __LINE__ << ": end = " << end << ", line = " << line << endl;
//  //cout << __func__ << " " << __LINE__ << "\n";
//
//  //while (!end && is_line(line, dependencies_for_edge_prefix)) {
//  //  string comment;
//  //  pc p1, p2;
//  //  edge_read_pcs_and_comment(line.substr(dependencies_for_edge_prefix.length()), p1, p2, comment);
//  //  shared_ptr<tfg_edge> e = (*t)->find_edge(p1, p2);
//  //  ASSERT(e);
//  //  //cout << __func__ << " " << __LINE__ << ": e = " << e->to_string() << endl;
//  //  getline(in, line);
//  //  string dependencies_expr_for_loc_prefix = "=Dependencies expr for loc ";
//  //  while (is_line(line, dependencies_expr_for_loc_prefix)) {
//  //    graph_loc_id_t loc_id = stoi(line.substr(dependencies_expr_for_loc_prefix.length()));
//  //    expr_ref dep_expr;
//  //    line = read_expr(in, dep_expr, ctx);
//  //    string dependencies_for_loc_prefix = "=Dependencies for loc ";
//  //    graph_loc_id_t loc_id2 = stoi(line.substr(dependencies_for_loc_prefix.length()));
//  //    ASSERT(loc_id == loc_id2);
//  //    //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
//  //    getline(in, line);
//  //    //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
//  //    set<graph_loc_id_t> locset = read_locset_from_string(line);
//  //    end = !getline(in, line);
//  //    //cout << __func__ << " " << __LINE__ << ": loc_id = " << loc_id << ", dep_expr = " << expr_string(dep_expr) << endl;
//  //    (*t)->add_pred_min_dependency(e, loc_id, dep_expr, locset);
//  //  }
//  //}
//}

void
tfgs_get_relevant_memlabels(vector<memlabel_ref> &relevant_memlabels, tfg const &tfg_llvm, tfg const &tfg_dst)
{
  //vector<memlabel_ref> relevant_memlabels_ref;
  tfg_llvm.graph_get_relevant_memlabels(relevant_memlabels);
  tfg_dst.graph_get_relevant_memlabels(relevant_memlabels);
  //for (auto const& mlr : relevant_memlabels_ref) {
  //  relevant_memlabels.push_back(mlr->get_ml());
  //}
}

//string
//read_rodata_map(istream &in, context *ctx, rodata_map_t &rodata_map)
//{
//  string line;
//  getline(in, line);
//  while (line.at(0) != '=') {
//    //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
//    ASSERT(line.substr(0, strlen("C_SYMBOL")) == "C_SYMBOL");
//    size_t remaining;
//    int symbol_id = stoi(line.substr(strlen("C_SYMBOL")), &remaining);
//    ASSERT(symbol_id >= 0);
//    size_t colon = line.find(':');
//    ASSERT(colon != string::npos);
//    size_t cur = colon + 1;
//    size_t comma;
//    set<src_ulong> symaddrs;
//    do {
//      comma = line.find(',', cur);
//      string symaddr_str;
//      if (comma != string::npos) {
//        symaddr_str = line.substr(cur, comma - cur);
//      } else {
//        symaddr_str = line.substr(cur);
//      }
//      src_ulong symaddr = stoi(symaddr_str);
//      symaddrs.insert(symaddr);
//      cur = comma + 1;
//    } while (comma != string::npos);
//    rodata_map.add_mapping(symbol_id, symaddrs);
//    getline(in, line);
//  }
//  return line;
//}

string
read_aliasing_constraints(istream &in, tfg const* t, context *ctx, aliasing_constraints_t &alias_cons)
{
  consts_struct_t const &cs = ctx->get_consts_struct();
  //map<pair<expr_ref,unsigned>, map<precond_t,set<bound_t>>> alias_cons_map;
  string line;
  bool end;

  end = !!getline(in, line);
  if (!end) {
    return string("");
  }
  //cout << __func__ << " " << __LINE__ << ": line = " << line << ".\n";
  while (is_line(line, "=guard")) {
    //cout << __func__ << " " << __LINE__ << ": line = " << line << ".\n";
    expr_ref guard;
    line = read_expr(in, guard, ctx);
    //cout << __func__ << " " << __LINE__ << ": addr_begin = " << expr_string(addr_begin) << ".\n";
    ASSERT (is_line(line, "=addr"));
    expr_ref addr;
    line = read_expr(in, addr, ctx);
    unsigned count;
    ASSERT (is_line(line, "=count"));
    getline(in, line);
    istringstream (line) >> count;
    ASSERT (count > 0);
    getline(in, line);
    ASSERT (is_line(line, "=memlabel"));
    getline(in, line);
    memlabel_t ml;
    memlabel_t::memlabel_from_string(&ml, line.c_str());
    alias_cons.add_constraint(aliasing_constraint_t(guard, addr, count, mk_memlabel_ref(ml)));

    if (!getline(in, line)) {
      line = "";
      break;
    }
  }
  return line;
}

static bool
read_llvm_function(istream &in, string const &function_name, shared_ptr<tfg> *t, char const *name, context *ctx)
{
  consts_struct_t const &cs = ctx->get_consts_struct();
  if(!is_next_line(in, "=TFG")) {
    cout << __func__ << " " << __LINE__ << ": parsing failed" << endl;
    return false;
  }
  //read_tfg(in, t, name, ctx, true);
  //*t = new tfg_ssa_t(in, name, ctx);
  *t = make_shared<tfg_llvm_t>(in, name, ctx);
  //(*t)->graph_from_stream(in);
  return true;
}

bool
parse_input_eq_file(string const &function_name, string const &filename_llvm, istream &in/*string const &filename_dst*/, shared_ptr<tfg> *tfg_dst, shared_ptr<tfg> *tfg_llvm, rodata_map_t &rodata_map, vector<dst_insn_t>& dst_iseq_vec, vector<dst_ulong>& dst_insn_pcs_vec, context* ctx, bool llvm2ir)
{
  consts_struct_t const &cs = ctx->get_consts_struct();

  ifstream in_llvm(filename_llvm);
  if (!in_llvm.is_open()) {
    cout << __func__ << " " << __LINE__ << ": parsing failed" << endl;
    return false;
  }

  string line;
  getline(in_llvm, line);
  string match_line(string(FUNCTION_NAME_FIELD_IDENTIFIER) + " " + function_name);
  while (!is_line(line, match_line)) {
    bool end = !getline(in_llvm, line);
    if (end) {
      cout << __func__ << " " << __LINE__ << ": function " << function_name << " not found in llvm file " << filename_llvm << endl;
      exit(1);
    }
  }
  if (!read_llvm_function(in_llvm, function_name, tfg_llvm, "llvm", ctx)) {
    in_llvm.close();
    return false;
  }
  //getline(in_llvm, line);
  //string match_line(string(FUNCTION_NAME_FIELD_IDENTIFIER) + " " + function_name);
  //while (!is_line(line, match_line)) {
  //  bool end = !getline(in_llvm, line);
  //  if (end) {
  //    cout << __func__ << " " << __LINE__ << ": function " << function_name << " not found in llvm file " << filename_llvm << endl;
  //    exit(1);
  //  }
  //}
  //if(!is_next_line(in_llvm, "=TFG")) {
  //  cout << __func__ << " " << __LINE__ << ": parsing failed" << endl;
  //  return false;
  //}
  //read_tfg(in_llvm, tfg_llvm, "llvm", ctx, cs);
  (*tfg_llvm)->rename_llvm_symbols_to_keywords(ctx); //XXX: this seems unncessary! Check and remove.

  //(*tfg_llvm)->replace_llvm_memoutput_with_memmasks(**tfg_dst);
  //(*tfg_dst)->reconcile_outputs_with_llvm_tfg(**tfg_llvm);

  //cout << __func__ << " " << __LINE__ << ": tfg_llvm:\n" << (*tfg_llvm)->graph_to_string() << endl;
  //cout << __func__ << " " << __LINE__ << ": tfg_dst:\n" << (*tfg_dst)->graph_to_string() << endl;
  in_llvm.close();

  //ifstream in(filename_dst);
  //ASSERT(in.is_open());
  //if(!in.is_open()) {
  //  cout << __func__ << " " << __LINE__ << ": parsing failed: could not open file " << filename_dst << endl;
  //  return false;
  //}
  //string line;

  if (!llvm2ir) {
    if (!is_next_line(in, "=Rodata-map")) {
      cout << __func__ << " " << __LINE__ << ": parsing failed" << endl;
      NOT_REACHED();
      return false;
    }
    //line = read_rodata_map(in, ctx, rodata_map);
    rodata_map = rodata_map_t(in);
    bool end = !getline(in, line);
    ASSERT(!end);

    ASSERT(line == "=dst_iseq");
    dst_iseq_vec = dst_iseq_from_stream(in);

    //cout << __func__ << " " << __LINE__ << ": dst_iseq_vec.size() = " << dst_iseq_vec.size() << endl;

    end = !getline(in, line);
    ASSERT(!end);
    ASSERT(line == "=dst_insn_pcs");
    dst_insn_pcs_vec = dst_insn_pcs_from_stream(in);

    end = !getline(in, line);
    ASSERT(!end);

    (*tfg_dst) = make_shared<tfg_asm_t>(in, "dst", ctx);
    //(*tfg_dst)->graph_from_stream(in);
  } else {
    if (!read_llvm_function(in, function_name, tfg_dst, "ir", ctx)) {
      //in.close();
      return false;
    }
    (*tfg_dst)->tfg_rename_srcdst_identifier(G_SRC_KEYWORD, G_DST_KEYWORD);
    (*tfg_dst)->tfg_update_symbol_map_and_rename_expressions_at_zero_offset((*tfg_llvm)->get_symbol_map());
    CPP_DBG_EXEC(EQCHECK, cout << __func__ << " " << __LINE__ << ": tfg_dst:\n" << (*tfg_dst)->graph_to_string() << endl);
  }

  return true;
}

predicate_set_t const
read_assumes_from_file(string const &assume_file, context* ctx)
{
  predicate_set_t ret;
  ifstream in(assume_file);
  if(!in.is_open()) {
    cout << __func__ << " " << __LINE__ << ": parsing of assume clauses failed. could not open file " << assume_file << "." << endl;
    assert(false);
  }

  string line;
  getline(in, line);
  while (is_line(line, "=assume")) {
    predicate_ref pred;
    line = predicate::predicate_from_stream(in, ctx, pred);
    ret.insert(pred);
  }
  return ret;
}

string
read_sprel_map_pair(istream &in, context *ctx, sprel_map_pair_t &sprel_map_pair)
{
  map<expr_id_t, pair<expr_ref, expr_ref>> submap;
  string line;
  bool end;

  end = !!getline(in, line);
  if (!end) {
    return string("");
  }
  while (is_line(line, "=loc_expr")) {
    expr_ref loc_expr, sprel_expr;
    line = read_expr(in, loc_expr, ctx);
    ASSERT(is_line(line, "=sprel_expr"));
    line = read_expr(in, sprel_expr, ctx);
    submap[loc_expr->get_id()] = make_pair(loc_expr, sprel_expr);
  }
  sprel_map_pair.init_from_submap(submap);
  return line;
}

string
read_local_sprel_expr_assumptions(istream &in, context *ctx, local_sprel_expr_guesses_t &lsprel_guesses)
{
  string line;
  bool end;

  end = !!getline(in, line);
  if (!end) {
    return string("");
  }
  while (is_line(line, "LocalSprelAssumption")) {
    end = !!getline(in, line);
    ASSERT(end);
    ASSERT(is_line(line, "=local."));
    int local_id = stoi(line.substr(strlen("=local.")));
    expr_ref sprel_expr;
    line = read_expr(in, sprel_expr, ctx);
    lsprel_guesses.add_local_sprel_expr_guess(local_id, sprel_expr);
  }
  return line;
}

string
read_src_suffixpath(istream &in/*, state const& start_state*/, context* ctx, tfg_suffixpath_t &src_suffixpath)
{
  string line;
  getline(in, line);
  if (line == "") {
    getline(in, line);
    src_suffixpath = nullptr;
    return line;
  }
  //src_suffixpath = parse_edge_composition<pc,tfg_edge>(line.c_str());
  //getline(in, line);
  //return line;
  line = graph_edge_composition_t<pc,tfg_edge>::graph_edge_composition_from_stream(in, line, "=src_suffixpath"/*, start_state*/, ctx, src_suffixpath);
  return line;
}

string
read_src_pred_avail_exprs(istream &in/*, state const& start_state*/, context *ctx, pred_avail_exprs_t &src_pred_avail_exprs)
{
  string line;
  bool end;

  end = !!getline(in, line);
  if (!end) {
    return string("");
  }
  while (is_line(line, "=suffixpath")) {
    tfg_suffixpath_t sf;
    // read suffixpath
    line = read_src_suffixpath(in/*, start_state*/, ctx, sf);
    ASSERT(sf != nullptr);
    // read (locid, expr) pairs
    if (!src_pred_avail_exprs.count(sf)) { src_pred_avail_exprs[sf] = {}; }
    while (is_line(line, "=loc")) {
      // read locid
      graph_loc_id_t locid = stoi(line.substr(strlen("=loc")));
      getline(in, line);
      // read (suffixpath, expr) pairs
      // read expr
      ASSERT(is_line(line, "=expr"));
      expr_ref expr;
      line = read_expr(in, expr, ctx);
      // insert the new pair
      src_pred_avail_exprs[sf][locid] = expr;
    }
  }
  return line;
}

map<string, shared_ptr<tfg>>
get_function_tfg_map_from_etfg_file(string const &etfg_filename, context* ctx)
{
  consts_struct_t const &cs = ctx->get_consts_struct();
  map<string, shared_ptr<tfg>> ret;
  ifstream in(etfg_filename);

  //cout << __func__ << " " << __LINE__ << ": etfg_filename = " << etfg_filename << ".\n";
  if (!in.is_open()) {
    cout << __func__ << " " << __LINE__ << ": parsing failed: could not open file " << etfg_filename << endl;
    NOT_REACHED();
  }
  string line;
  bool end = !getline(in, line);
  //cout << __func__ << " " << __LINE__ << ": line = " << line << ".\n";
  while (!end) {
    while (line == "") {
      if (!getline(in, line)) {
        return ret;
      }
    }
    //cout << __func__ << " " << __LINE__ << ": line = " << line << ".\n";
    while (!is_line(line, FUNCTION_NAME_FIELD_IDENTIFIER)) {
      //cout << __func__ << " " << __LINE__ << ": parsing failed" << endl;
      //cout << __func__ << " " << __LINE__ << ": line = " << line << "." << endl;
      //NOT_REACHED();
      end = !getline(in, line);
      if (end) {
        break;
      }
      continue;
    }
    if (end) {
      break;
    }
    string function_name = line.substr(strlen(FUNCTION_NAME_FIELD_IDENTIFIER));
    trim(function_name);
    if(!is_next_line(in, "=TFG")) {
      cout << __func__ << " " << __LINE__ << ": parsing failed" << endl;
      NOT_REACHED();
    }
    //tfg *tfg_llvm = NULL;
    //auto pr = read_tfg(in, &tfg_llvm, "llvm", ctx, true);
    shared_ptr<tfg_llvm_t> tfg_llvm = make_shared<tfg_llvm_t>(in, "llvm", ctx);
    ASSERT(tfg_llvm);
    //line = tfg_llvm->graph_from_stream(in);
    //cout << __func__ << " " << __LINE__ << ": after parsing " << etfg_filename << ": TFG=\n" << tfg_llvm->graph_to_string() << endl;
    //line = pr.second;
    //end = pr.first;
    ret[function_name] = dynamic_pointer_cast<tfg>(tfg_llvm);
    //ASSERT(line == FUNCTION_FINISH_IDENTIFIER);
    //ASSERT(!end);
    end = false;
    end = !getline(in, line);
  }
  //cout << __func__ << " " << __LINE__ << ": ret.size() = " << ret.size() << endl;
  return ret;
}

string
read_memlabel_map(istream& in, context* ctx, string line, graph_memlabel_map_t &memlabel_map)
{
  while (is_line(line, "=memlabel_map")) {
    //cout << __func__ << " " << __LINE__ << ": line = " << line << ".\n";
    getline(in, line);
    //cout << __func__ << " " << __LINE__ << ": line = " << line << ".\n";
    size_t colon = line.find(':');
    string mlvarname_str = line.substr(0, colon);
    trim(mlvarname_str);
    string ml_str = line.substr(colon + 1);
    memlabel_t ml;
    memlabel_t::memlabel_from_string(&ml, ml_str.c_str());
    memlabel_map[mk_string_ref(mlvarname_str)] = mk_memlabel_ref(ml);
    getline(in, line);
  }
  return line;
}

void
read_lhs_set_guard_lsprels_and_src_dst_from_file(istream& in, context* ctx, predicate_set_t& lhs_set, precond_t& precond, local_sprel_expr_guesses_t& lsprel_guesses, sprel_map_pair_t& sprel_map_pair, shared_ptr<tfg_edge_composition_t>& src_suffixpath, pred_avail_exprs_t& src_pred_avail_exprs, aliasing_constraints_t& alias_cons, graph_memlabel_map_t& memlabel_map, set<local_sprel_expr_guesses_t>& all_guesses, expr_ref& src, expr_ref& dst, map<expr_id_t, expr_ref>& src_map, map<expr_id_t, expr_ref>& dst_map, shared_ptr<memlabel_assertions_t>& mlasserts, shared_ptr<tfg>& src_tfg)
{
  consts_struct_t const &cs = ctx->get_consts_struct();
  string line;
  bool end;

  getline(in, line);
  ASSERT(is_line(line, "=TFG"));
  src_tfg = make_shared<tfg_llvm_t>(in, "llvm", ctx);
  mlasserts = make_shared<memlabel_assertions_t>(in, ctx);
  do {
    end = !getline(in, line);
    ASSERT(!end);
  } while (line == "");

  while (is_line(line, "=lhs-pred")) {
    predicate_ref pred;
    line = predicate::predicate_from_stream(in/*, src_tfg->get_start_state()*/, src_tfg->get_context(), pred);
    lhs_set.insert(pred);
  }
  while (line == "") {
    getline(in, line);
  }
  if (!is_line(line, "=precond")) {
    cout << __func__ << " " << __LINE__ << ": line = " << line << "." << endl;
  }
  ASSERT(is_line(line, "=precond"));
  line = precond_t::read_precond(in/*, src_tfg->get_start_state()*/, ctx, precond);
  ASSERTCHECK(is_line(line, "=local_sprel_expr_assumes_required"), cerr << "line = " << line << endl);
  line = read_local_sprel_expr_assumptions(in, ctx, lsprel_guesses);
  while (line == "") {
    getline(in, line);
  }
  ASSERT(is_line(line, "=sprel_map_pair"));
  line = read_sprel_map_pair(in, ctx, sprel_map_pair);
  ASSERT(is_line(line, "=src_suffixpath"));
  line = read_src_suffixpath(in/*, src_tfg->get_start_state()*/, ctx, src_suffixpath);
  ASSERT(src_suffixpath != nullptr);
  ASSERT(is_line(line, "=src_pred_avail_exprs"));
  line = read_src_pred_avail_exprs(in/*, src_tfg->get_start_state()*/, ctx, src_pred_avail_exprs);
  ASSERT(is_line(line, "=aliasing_constraints"));
  line = read_aliasing_constraints(in, src_tfg.get(), ctx, alias_cons);

  while (line == "") {
    getline(in, line);
  }
  if (!is_line(line, "=memlabel_map")) {
    cout << __func__ << " " << __LINE__ << ": line =\n" << line << ".\n";
  }
  ASSERT(is_line(line, "=memlabel_map"));
  getline(in, line);
  line = tfg::read_memlabel_map(in, line, memlabel_map);
  while (line == "") {
    getline(in, line);
  }
  if (!is_line(line, "=all_guesses.num")) {
    cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
  }
  ASSERT(is_line(line, "=all_guesses.num"));
  getline(in, line);
  while (is_line(line, "=guess")) {
    local_sprel_expr_guesses_t aguess;
    line = read_local_sprel_expr_assumptions(in, ctx, aguess);
    all_guesses.insert(aguess);
  }
  ASSERT(is_line(line, "=src"));
  line = read_expr_and_expr_map(in, src, src_map, ctx);
  ASSERT(is_line(line, "=dst"));
  line = read_expr_and_expr_map(in, dst, dst_map, ctx);
}

}
