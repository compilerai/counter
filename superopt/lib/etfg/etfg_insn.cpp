#include <map>
#include <iostream>

#include "support/debug.h"
#include "support/types.h"
#include "support/src_tag.h"
#include "support/permute.h"
#include "support/c_utils.h"

#include "valtag/imm_vt_map.h"
#include "valtag/regmap.h"
#include "valtag/regcons.h"
#include "valtag/valtag.h"
#include "valtag/fixed_reg_mapping.h"
#include "valtag/ldst_input.h"
#include "valtag/regset.h"
#include "valtag/reg_identifier.h"
#include "valtag/nextpc.h"

#include "expr/expr.h"

#include "gsupport/predicate.h"

#include "tfg/tfg.h"
#include "tfg/tfg_llvm.h"

#include "ptfg/ftmap.h"

#include "etfg/etfg_insn.h"
#include "i386/insn.h"
#include "x64/insn.h"

#include "insn/src-insn.h"
#include "insn/dst-insn.h"

#include "rewrite/bbl.h"
#include "rewrite/translation_instance.h"
#include "rewrite/static_translate.h"
#include "rewrite/src_iseq_match_renamings.h"

#include "eq/parse_input_eq_file.h"

//COMMENT1 : when the instruction is fetched and when etfg_insn_branch_targets() is called, pcrels store the absolute instruction addresses. However after the call to cmn_insn_convert_pcrels_to_inums, the pcrels store relative instruction indices. Ths invariant is followed for all src architectures (etfg, ppc, i386)

using namespace std;
static char as1[40960];
static char as2[40960];

static string
etfg_regname(exreg_group_id_t g, reg_identifier_t const &r)
{
  stringstream ss;
  //ss << "reg.";
  ss << G_REGULAR_REG_NAME;
  ss << g;
  ss << ".";
  ss << r.reg_identifier_to_string();
  return ss.str();
}


state const &
etfg_canonical_start_state(context *ctx)
{
  static state st;
  if (st.get_value_expr_map_ref().size() == 0) {
    map<string_ref, expr_ref> value_expr_map;
    for (exreg_group_id_t i = 0; i < ETFG_NUM_EXREG_GROUPS; i++) {
      for (exreg_id_t j = 0; j < ETFG_NUM_EXREGS(i); j++) {
        string rname = etfg_regname(i, j);
        value_expr_map.insert(make_pair(mk_string_ref(string(G_SRC_KEYWORD ".") + rname), ctx->mk_var(string(G_INPUT_KEYWORD "." G_SRC_KEYWORD) + "." + rname, ctx->mk_bv_sort(ETFG_EXREG_LEN(i)))));
      }
    }
    //st.set_expr(G_SRC_KEYWORD "." LLVM_MEM_SYMBOL, ctx->mk_var(string(G_INPUT_KEYWORD "." G_SRC_KEYWORD) + "." + LLVM_MEM_SYMBOL, ctx->mk_array_sort(ctx->mk_bv_sort(DWORD_LEN), ctx->mk_bv_sort(BYTE_LEN))));
    value_expr_map.insert(make_pair(mk_string_ref(G_SRC_KEYWORD "." LLVM_MEM_SYMBOL), ctx->mk_var(string(G_INPUT_KEYWORD "." G_SRC_KEYWORD) + "." + LLVM_MEM_SYMBOL, ctx->mk_array_sort(ctx->mk_bv_sort(DWORD_LEN), ctx->mk_bv_sort(BYTE_LEN)))));
    value_expr_map.insert(make_pair(mk_string_ref(G_SRC_KEYWORD "." LLVM_MEM_SYMBOL "." G_ALLOC_SYMBOL), ctx->mk_var(string(G_INPUT_KEYWORD "." G_SRC_KEYWORD) + "." + LLVM_MEM_SYMBOL + "." + G_ALLOC_SYMBOL, ctx->mk_array_sort(ctx->mk_bv_sort(DWORD_LEN), ctx->mk_memlabel_sort()))));
    //st.set_expr(G_LLVM_RETURN_REGISTER_NAME, ctx->mk_var(string(G_INPUT_KEYWORD) + "." + G_LLVM_RETURN_REGISTER_NAME, ctx->mk_bv_sort(ETFG_MAX_REG_SIZE)));
    //st.set_expr(G_SRC_KEYWORD "." LLVM_STATE_INDIR_TARGET_KEY_ID, ctx->mk_var(string(G_INPUT_KEYWORD "." G_SRC_KEYWORD) + "." + LLVM_STATE_INDIR_TARGET_KEY_ID, ctx->mk_bv_sort(DWORD_LEN)));
    value_expr_map.insert(make_pair(mk_string_ref(G_SRC_KEYWORD "." LLVM_STATE_INDIR_TARGET_KEY_ID), ctx->mk_var(string(G_INPUT_KEYWORD "." G_SRC_KEYWORD) + "." + LLVM_STATE_INDIR_TARGET_KEY_ID, ctx->mk_bv_sort(DWORD_LEN))));
    st.set_value_expr_map(value_expr_map);
  }
  return st;
}

static input_section_t const *
input_exec_get_text_section(struct input_exec_t const *in)
{
  for (int i = 0; i < in->num_input_sections; i++) {
    if (   !strcmp(in->input_sections[i].name, ".text")
        || !strcmp(in->input_sections[i].name, ".srctext")) {
      return &in->input_sections[i];
    }
  }
  NOT_REACHED();
}

src_ulong
etfg_input_exec_get_start_pc_for_function_name(struct input_exec_t const *in, string const &function_name)
{
  input_section_t const *isec = input_exec_get_text_section(in);
  map<string, tfg const *>  const *function_tfg_map = (map<string, tfg const *>  const *)isec->data;
  if (!function_tfg_map->count(function_name)) {
    return (src_ulong)-1;
  }
  size_t i = 0;
  for (auto ft : *function_tfg_map) {
    if (ft.first == function_name) {
      src_ulong ret = i * ETFG_MAX_INSNS_PER_FUNCTION;
      //cout << __func__ << " " << __LINE__ << ": returning " << hex << ret << dec << " for " << function_name << endl;
      return ret;
    }
    i++;
  }
  return (src_ulong)-1;
}

src_ulong
etfg_input_exec_get_stop_pc_for_function_name(struct input_exec_t const *in, string const &function_name)
{
  input_section_t const *isec = input_exec_get_text_section(in);
  map<string, tfg const *>  const *function_tfg_map = (map<string, tfg const *>  const *)isec->data;
  if (!function_tfg_map->count(function_name)) {
    return (src_ulong)-1;
  }
  size_t i = 0;
  for (auto ft : *function_tfg_map) {
    if (ft.first == function_name) {
      src_ulong ret = (i + 1) * ETFG_MAX_INSNS_PER_FUNCTION;
      //cout << __func__ << " " << __LINE__ << ": returning " << hex << ret << dec << " for " << function_name << endl;
      return ret;
    }
    i++;
  }
  return (src_ulong)-1;
}



/*static shared_ptr<tfg_edge>
tfg_get_edge_from_edgenum(const void *t_in, unsigned edgenum)
{
  tfg const *t = (tfg const *)t_in;
  list<shared_ptr<tfg_edge>> es = t->get_edges_ordered();
  ASSERT(edgenum < es.size());
  auto iter = es.begin();
  for (unsigned i = 0; i < edgenum; i++, iter++);
  return *iter;
}*/

static void
etfg_iseq_copy(struct etfg_insn_t *dst, struct etfg_insn_t const *src, long n)
{
  for (long i = 0; i < n; i++) {
    etfg_insn_copy(&dst[i], &src[i]);
  }
}

static bool
etfg_imm_operand_needs_canonicalization(opc_t opc, int explicit_op_index)
{
  return true;
}

//#define CANONICAL_CONSTANT_PREFIX "C"

/*static string
get_canonical_constant_expr(int cnum)
{
  stringstream ss;
  ss << CANONICAL_CONSTANT_PREFIX << cnum;
  return ss.str();
}*/

static bool
expr_is_canonical_constant(expr_ref const &e, vconstants_t *vconstant)
{
  if (!e->is_var()) {
    return false;
  }
  string e_str = expr_string(e);
  string_replace(e_str, CANON_CONSTANT_PREFIX, "C");
  char e_chrs[e_str.size() + 2];
  e_chrs[0] = ' ';
  strncpy(&e_chrs[1], e_str.c_str(), sizeof e_chrs - 1);
  vconstants_t l_vconstant;
  if (!vconstant) {
    vconstant = &l_vconstant;
  }
  pair<bool, size_t> p = parse_vconstant(e_chrs, vconstant, NULL);
  return p.first;
}



static bool
expr_is_potential_llvm_reg_id(expr_ref const &e_in)
{
  context *ctx = e_in->get_context();
  expr_ref e = e_in;
  consts_struct_t const &cs = ctx->get_consts_struct();
  if (   (   e->is_const()
          && e->is_bv_sort()
          && e->get_sort()->get_size() <= ETFG_MAX_REG_SIZE)
      || (   e->is_const()
          && e->is_bool_sort())
      || cs.is_symbol(e)
      || expr_is_canonical_constant(e, NULL)
      || cs.expr_represents_llvm_undef(e)) {
    /*if (var) {
      DYN_DEBUG(insn_match, cout << __func__ << " " << __LINE__ << ": e = " << expr_string(e) << ": returning false because const but var\n");
      return false;
    } else */{
      DYN_DEBUG(insn_match, cout << __func__ << " " << __LINE__ << ": e = " << expr_string(e) << ": returning true\n");
      return true;
    }
  }

  if (   e->get_operation_kind() == expr::OP_ITE
      && e->is_bv_sort()
      && e->get_args().at(1) == ctx->mk_bv_const(1, 1)
      && e->get_args().at(2) == ctx->mk_bv_const(1, 0)) {
    e = e->get_args().at(0);
  }

  if (!e->is_var()) {
    DYN_DEBUG(insn_match, cout << __func__ << " " << __LINE__ << ": e = " << expr_string(e) << ": returning false because !is_var()\n");
    return false;
  }
  /*if (expr_is_canonical_constant(e, NULL)) {
    return false;
  }*/
  string e_str = expr_string(e);
  if (e_str.substr(0, strlen(G_INPUT_KEYWORD ".")) != G_INPUT_KEYWORD ".") {
    DYN_DEBUG(insn_match, cout << __func__ << " " << __LINE__ << ": e = " << expr_string(e) << ": returning false because is_var() but not prefixed with input keyword\n");
    return false;
  }
  return true;
}

static expr_ref
convert_valtag_to_expr(context *ctx, valtag_t const &vt, sort_ref const &s)
{
  ASSERT(s->is_bv_kind());
  if (vt.tag == tag_const) {
    return ctx->mk_bv_const(s->get_size(), vt.val);
  } else {
    size_t len = imm2str(vt.val, vt.tag, vt.mod.imm.modifier, vt.mod.imm.constant_val, vt.mod.imm.constant_tag, vt.mod.imm.sconstants, vt.mod.imm.exvreg_group, nullptr, 0, etfg_get_i386_as_mode(), as1, sizeof as1);
    string name = as1;
    string_replace(name, "C", CANON_CONSTANT_PREFIX);
    return ctx->mk_var(name, s);
  }
}

static long
expr_strict_canonicalize_imms(expr_ref const &in, expr_ref *out_var_ops, imm_vt_map_t *out_imm_maps,
    long max_num_out_var_ops, imm_vt_map_t const *st_map, context *ctx)
{
  //cout << __func__ << " " << __LINE__ << ": in = " << expr_string(in) << endl;
  consts_struct_t const &cs = ctx->get_consts_struct();
  ASSERT(in->is_bv_sort());
  ASSERT(max_num_out_var_ops >= 1);
  ASSERT(in->is_const() || cs.is_symbol(in) || cs.expr_is_local_id(in, G_SRC_KEYWORD));
  valtag_t vt;
  if (in->is_const()) {
    vt.tag = tag_const;
    vt.val = in->get_int_value();
  } else if (cs.is_symbol(in)) {
    //vt.tag = tag_imm_symbol;
    //vt.val = consts_struct_t::get_symbol_id_from_expr(in);
    //ASSERT(vt.val >= 0 && vt.val < NUM_CANON_SYMBOLS);
    vt.tag = tag_const;
    symbol_id_t symbol_id = consts_struct_t::get_symbol_id_from_expr(in);
    ASSERT(symbol_id >= 0 && symbol_id < NUM_CANON_SYMBOLS);
    vt.val = RANDOM_CONSTANT_FOR_SYMBOL_ID(symbol_id); //use some random constant that ensures that each symbol is assigned a different canonical constant
  /*} else if (cs.expr_represents_llvm_undef(in)) {
    vt.tag = tag_const;
    vt.val = 0;*/
  } else if (cs.expr_is_local_id(in, G_SRC_KEYWORD)) {
    vt.tag = tag_const;
    allocstack_t local_id_stack = cs.expr_get_local_id(in, G_SRC_KEYWORD);
    allocsite_t local_id = local_id_stack.allocstack_get_only_allocsite();
    //ASSERT(local_id >= 0 && local_id < NUM_CANON_SYMBOLS);
    vt.val = RANDOM_CONSTANT_FOR_LOCAL_ID(local_id.allocsite_to_longlong()); //use some random constant that ensures that each symbol is assigned a different canonical constant
  } else NOT_REACHED();
  valtag_t *out_vts = new valtag_t[max_num_out_var_ops];
  //cout << __func__ << " " << __LINE__ << ": canonicalizing vt.val = " << vt.val << ", vt.tag = " << vt.tag << endl;
  //cout << __func__ << " " << __LINE__ << ": before : out_imm_maps[" << 0 << "] = " << imm_vt_map_to_string(&out_imm_maps[0], as1, sizeof as1) << endl;
  long num_out_var_ops = ST_CANON_IMM(-1, -1, out_vts, out_imm_maps, max_num_out_var_ops, vt, DWORD_LEN, st_map, etfg_imm_operand_needs_canonicalization);
  //cout << __func__ << " " << __LINE__ << ": num_out_var_ops = " << num_out_var_ops << endl;
  for (size_t i = 0; i < num_out_var_ops; i++) {
    out_var_ops[i] = convert_valtag_to_expr(ctx, out_vts[i], in->get_sort());
    //cout << __func__ << " " << __LINE__ << ": after: out_var_ops[" << i << "] = " << expr_string(out_var_ops[i]) << endl;
    //cout << __func__ << " " << __LINE__ << ": after: out_imm_maps[" << i << "] = " << imm_vt_map_to_string(&out_imm_maps[i], as1, sizeof as1) << endl;
  }
  delete [] out_vts;
  return num_out_var_ops;
  //out_var_ops[0] = in;
  //imm_vt_map_copy(&out_imm_maps[0], st_map);
  //return 1;
}

class expr_getset_at_operand_index_visitor : public expr_visitor {
public:
  expr_getset_at_operand_index_visitor(context *ctx, expr_ref const &e, long &op_index, expr_ref const &val) : m_ctx(ctx), m_in(e), m_op_index(op_index), m_val(val), m_cs(ctx->get_consts_struct())
  {
    visit_recursive(m_in);
  }

  expr_ref get_expr_at_op_index() const
  {
    if (m_map.count(0) == 0) {
      return nullptr;
    }
    return m_map.at(0);
  }

  long get_num_imm_operands() const
  {
    return m_map.size();
  }

  expr_ref get_substituted_expr() const
  {
    return m_submap.at(m_in->get_id());
  }

private:
  virtual void visit(expr_ref const &);
  struct context *m_ctx;
  expr_ref const &m_in;
  long &m_op_index;
  expr_ref const &m_val;
  consts_struct_t const &m_cs;
  map<long, expr_ref> m_map;
  map<expr_id_t, expr_ref> m_submap;
};

void
expr_getset_at_operand_index_visitor::visit(expr_ref const &e)
{
  bool is_imm_operand = false;
  if (   e->is_const()
      && e->is_bv_sort()
      && e->get_sort()->get_size() >= 5
      && e->get_int_value() != 0
      && e->get_int_value() != 1
      && e->get_int_value() != -1) {
    is_imm_operand = true;
  } else if (   e->is_var()
             && (   m_cs.is_symbol(e)
                 //|| m_cs.expr_represents_llvm_undef(e)
                 || m_cs.expr_is_local_id(e, G_SRC_KEYWORD))) {
    is_imm_operand = true;
  } else if (   e->is_var()
             && expr_is_canonical_constant(e, NULL)) {
    is_imm_operand = true;
  }
  //cout << __func__ << " " << __LINE__ << ": e = " << expr_string(e) << endl;
  //cout << __func__ << " " << __LINE__ << ": is_imm_operand = " << is_imm_operand << endl;

  if (is_imm_operand) {
    m_map[m_op_index] = e;
    if (m_val && m_op_index == 0) {
      m_submap[e->get_id()] = m_val;
    } else {
      m_submap[e->get_id()] = e;
    }
    m_op_index--;
    return;
  }
  if (e->is_var() || e->is_const()) {
    m_submap[e->get_id()] = e;
    return;
  }
  expr_vector new_args;
  for (auto arg : e->get_args()) {
    new_args.push_back(m_submap.at(arg->get_id()));
  }
  m_submap[e->get_id()] = m_ctx->mk_app(e->get_operation_kind(), new_args);
}

static expr_ref
expr_get_expr_at_operand_index(context *ctx, expr_ref const &e, long &op_index)
{
  ASSERT(op_index >= 0);
  //cout << __func__ << " " << __LINE__ << ": entry: op_index = " << op_index << endl;
  //cout << __func__ << " " << __LINE__ << ": entry: e = " << expr_string(e) << endl;
  expr_getset_at_operand_index_visitor visitor(ctx, e, op_index, nullptr);
  return visitor.get_expr_at_op_index();
}

static expr_ref
expr_set_expr_at_operand_index(context *ctx, expr_ref const &e, long &op_index, expr_ref const &val)
{
  if (op_index < 0) {
    return e;
  }
  ASSERT(op_index >= 0);
  expr_getset_at_operand_index_visitor visitor(ctx, e, op_index, val);
  return visitor.get_substituted_expr();
}

static long
expr_get_max_num_imm_operands(context *ctx, expr_ref const &e)
{
  long op_index = LONG_MAX;
  expr_getset_at_operand_index_visitor visitor(ctx, e, op_index, nullptr);
  long ret = visitor.get_num_imm_operands();
  //cout << __func__ << " " << __LINE__ << ": returning " << ret << " for " << expr_string(e) << endl;
  return ret;
}

static void
state_set_expr_at_operand_index(context *ctx, state &s, long &op_index, expr_ref const &val)
{
  std::function<expr_ref (const string&, expr_ref)> f = [ctx, &op_index, &val](const string& key, expr_ref e) -> expr_ref
  {
    //cout << __func__ << " " << __LINE__ << ": e = " << expr_string(e) << endl;
    expr_ref ret = expr_set_expr_at_operand_index(ctx, e, op_index, val);
    //cout << __func__ << " " << __LINE__ << ": ret = " << expr_string(ret) << endl;
    return ret;
  };
  s.state_visit_exprs(f);
}

static expr_ref
state_get_expr_at_operand_index(context *ctx, state const &s, long &op_index)
{
  expr_ref ret = nullptr;
  //cout << __func__ << " " << __LINE__ << ": entry: op_index = " << op_index << endl;
  std::function<void (const string&, expr_ref)> f = [ctx, &ret, &op_index](const string& key, expr_ref e) -> void
  {
    if (!ret) {
      ASSERT(op_index >= 0);
      ret = expr_get_expr_at_operand_index(ctx, e, op_index);
    }
  };
  s.state_visit_exprs(f);
  return ret;
}

static expr_ref
tfg_edge_get_expr_at_operand_index(context *ctx, shared_ptr<tfg_edge const> const &e, long &op_index)
{
  state state_to = e->get_to_state();
  expr_ref econd = e->get_cond();
  //expr_ref indir_tgt = e->get_indir_tgt();
  expr_ref ret = state_get_expr_at_operand_index(ctx, state_to, op_index);
  if (ret) {
    return ret;
  }
  ASSERT(op_index >= 0);
  ret = expr_get_expr_at_operand_index(ctx, econd, op_index);
  if (ret) {
    return ret;
  }
  ASSERT(op_index >= 0);
  /*if (e->get_is_indir_exit()) {
    ASSERT(indir_tgt);
    ret = expr_get_expr_at_operand_index(ctx, indir_tgt, op_index);
    if (ret) {
      return ret;
    }
  }
  ASSERT(op_index >= 0);*/
  return nullptr;
}

static expr_ref
predicate_get_expr_at_operand_index(context *ctx, predicate_ref const &pred, long &op_index)
{
  expr_ref ret = expr_get_expr_at_operand_index(ctx, pred->get_lhs_expr(), op_index);
  if (ret) {
    return ret;
  }
  ASSERT(op_index >= 0);
  ret = expr_get_expr_at_operand_index(ctx, pred->get_rhs_expr(), op_index);
  if (ret) {
    return ret;
  }
  ASSERT(op_index >= 0);
  return nullptr;
}

static expr_ref
etfg_insn_get_expr_at_operand_index(etfg_insn_t const *insn, long op_index)
{
  //ASSERT(insn->m_tfg);
  ASSERT(insn->m_edges.size() > 0);
  //tfg const *t = (tfg const *)insn->m_tfg;
  //context *ctx = t->get_context();
  context *ctx = insn->get_context();
  expr_ref ret = nullptr;
  ASSERT(op_index >= 0);
  for (const auto &pred : insn->m_preconditions) {
    ASSERT(op_index >= 0);
    ret = predicate_get_expr_at_operand_index(ctx, pred, op_index);
    if (ret) {
      return ret;
    }
    ASSERT(op_index >= 0);
  }
  for (const auto &e : insn->m_edges) {
    ASSERT(op_index >= 0);
    ret = tfg_edge_get_expr_at_operand_index(ctx, e.get_tfg_edge(), op_index);
    if (ret) {
      return ret;
    }
    ASSERT(op_index >= 0);
  }
  cout << __func__ << " " << __LINE__ << ": op_index = " << op_index << endl;
  NOT_REACHED();
}

static shared_ptr<tfg_edge const>
tfg_edge_set_expr_at_operand_index(context *ctx, shared_ptr<tfg_edge const> const &e, long &op_index, expr_ref const &val)
{
  //state state_to = e->get_to_state();
  //expr_ref econd = e->get_cond();
  //state_set_expr_at_operand_index(ctx, state_to, op_index, val);
  //econd = expr_set_expr_at_operand_index(ctx, econd, op_index, val);
  std::function<expr_ref (expr_ref const&)> f = [ctx, &op_index, &val](expr_ref const& e)
  {
    return expr_set_expr_at_operand_index(ctx, e, op_index, val);
  };
  return e->visit_exprs(f);
}

void
etfg_insn_edge_t::etfg_insn_edge_set_expr_at_operand_index(context *ctx, long op_index, expr_ref const& val)
{
  m_tfg_edge = tfg_edge_set_expr_at_operand_index(ctx, m_tfg_edge, op_index, val);
}

void
etfg_insn_edge_t::etfg_insn_edge_update_state(std::function<void (pc const&, state&)> f)
{
  m_tfg_edge = m_tfg_edge->tfg_edge_update_state(f);
}

void
etfg_insn_edge_t::etfg_insn_edge_update_edgecond(std::function<expr_ref (pc const&, expr_ref const&)> f)
{
  m_tfg_edge = m_tfg_edge->tfg_edge_update_edgecond(f);
}

predicate_ref
predicate_set_expr_at_operand_index(context *ctx, predicate_ref &pred, long &op_index, expr_ref const &val)
{
  return pred->set_lhs_and_rhs_exprs(expr_set_expr_at_operand_index(ctx, pred->get_lhs_expr(), op_index, val),
                                     expr_set_expr_at_operand_index(ctx, pred->get_rhs_expr(), op_index, val));
}



static void
etfg_insn_set_expr_at_operand_index(etfg_insn_t *insn, long op_index, expr_ref const &val)
{
  //ASSERT(insn->m_tfg);
  ASSERT(insn->m_edges.size() > 0);
  //tfg const *t = (tfg const *)insn->m_tfg;
  //context *ctx = t->get_context();
  context *ctx = insn->get_context();
  for (auto &pred : insn->m_preconditions) {
    if (op_index >= 0) {
      pred = predicate_set_expr_at_operand_index(ctx, pred, op_index, val);
    }
  }
  for (auto &e : insn->m_edges) {
    if (op_index >= 0) {
      e.etfg_insn_edge_set_expr_at_operand_index(ctx, op_index, val);
    }
  }
}

long
etfg_operand_strict_canonicalize_imms(etfg_strict_canonicalization_state_t *scanon_state,
    long insn_index,
    long op_index,
    struct etfg_insn_t *iseq_var[], long iseq_var_len[], struct regmap_t *st_regmap,
    struct imm_vt_map_t *st_imm_map, fixed_reg_mapping_t const &fixed_reg_mapping,
    long num_iseq_var, long max_num_iseq_var)
{
  ASSERT(op_index >= 0);
  if (op_index == 0) {
    return num_iseq_var; //operand 0 is for regs; ignore
  }
  //cout << __func__ << " " << __LINE__ << ": strict canonicalizing operand index " << op_index << endl;
  //cout << __func__ << " " << __LINE__ << ": before iseq_var[0]:\n" << etfg_iseq_to_string(iseq_var[0], iseq_var_len[0], as1, sizeof as1) << endl;
  op_index--; //imm operands start at op_index == 1; decrement op_index by one for the rest of the code logic
  long max_num_out_var_ops = max_num_iseq_var - num_iseq_var + 1;
  long max_op_size, m, n, new_n, num_out_var_ops;
  imm_vt_map_t out_imm_maps[max_num_out_var_ops];
  //valtag_t out_var_insns[max_num_out_var_insns];
  expr_ref out_var_ops[max_num_out_var_ops];
  etfg_insn_t *iseq_var_copy[num_iseq_var];
  imm_vt_map_t st_imm_map_copy[num_iseq_var];
  long iseq_var_copy_len[num_iseq_var];
  imm_vt_map_t *st_map;

  ASSERT(num_iseq_var >= 1);
  ASSERT(max_num_iseq_var >= num_iseq_var);
  for (n = 0; n < num_iseq_var; n++) {
    iseq_var_copy[n] = new etfg_insn_t[iseq_var_len[n]];
    ASSERT(iseq_var_copy[n]);
    etfg_iseq_copy(iseq_var_copy[n], iseq_var[n], iseq_var_len[n]);
    iseq_var_copy_len[n] = iseq_var_len[n];
    imm_vt_map_copy(&st_imm_map_copy[n], &st_imm_map[n]);
  }
  DYN_DBG(PEEP_TAB, "num_iseq_var = %ld\n", num_iseq_var);
  new_n = 0;
  for (n = 0; n < num_iseq_var; n++) {
    etfg_insn_t *insn;

    insn = &iseq_var_copy[n][insn_index];
    //tfg const *t = (tfg const *)insn->m_tfg;
    context *ctx = insn->get_context();
    //cout << __func__ << " " << __LINE__ << ": op_index = " << op_index << endl;
    expr_ref op_expr = etfg_insn_get_expr_at_operand_index(insn, op_index);
    st_map = &st_imm_map_copy[n];
    DBG2(PEEP_TAB, "n = %ld, st_map:\n%s\n", n, imm_vt_map_to_string(st_map, as1, sizeof as1));
    imm_vt_map_copy(&out_imm_maps[0], st_map);
    num_out_var_ops = expr_strict_canonicalize_imms(op_expr, out_var_ops, out_imm_maps, max_num_out_var_ops, st_map, ctx);
    DBG(PEEP_TAB, "n = %ld, st_map:\n%s\n", n, imm_vt_map_to_string(st_map, as1, sizeof as1));
    for (m = 0; m < num_out_var_ops; m++) {
      ASSERT(new_n <= max_num_iseq_var);
      ASSERT(max_num_iseq_var - new_n >= num_iseq_var - n);
      if (m != 0 && max_num_iseq_var - new_n == num_iseq_var - n) {
        break;
      }
      ASSERT(new_n < max_num_iseq_var);
      DBG2(PEEP_TAB, "m = %ld, out_imm_map:\n%s\n", m,
          imm_vt_map_to_string(&out_imm_maps[m], as1, sizeof as1));
      ASSERT(new_n < max_num_iseq_var);
      etfg_iseq_copy(iseq_var[new_n], iseq_var_copy[n], iseq_var_copy_len[n]);
      iseq_var_len[new_n] = iseq_var_copy_len[n];
      //i386_operand_copy(&iseq_var[new_n][insn_index].op[op_index], var_op);
      imm_vt_map_copy(&st_imm_map[new_n], &out_imm_maps[m]);
      etfg_insn_set_expr_at_operand_index(&iseq_var[new_n][insn_index], op_index, out_var_ops[m]);
      new_n++;
      ASSERT(new_n <= max_num_iseq_var);
    }
  }
  for (n = 0; n < num_iseq_var; n++) {
    delete [] iseq_var_copy[n];
  }
  //cout << __func__ << " " << __LINE__ << ": after iseq_var[0]:\n" << etfg_iseq_to_string(iseq_var[0], iseq_var_len[0], as1, sizeof as1) << endl;
  return new_n;
}

static long
state_get_max_num_imm_operands(context *ctx, state const &s)
{
  long ret = 0;
  std::function<void (const string&, expr_ref)> f = [ctx, &ret](const string& key, expr_ref e) -> void
  {
    ret += expr_get_max_num_imm_operands(ctx, e);
  };
  s.state_visit_exprs(f);
  //cout << __func__ << " " << __LINE__ << ": returning " << ret << " for state\n";
  return ret;
}

static long
tfg_edge_get_max_num_imm_operands(context *ctx, shared_ptr<tfg_edge const> const &e)
{
  state state_to = e->get_to_state();
  expr_ref econd = e->get_cond();
  //expr_ref indir_tgt = e->get_indir_tgt();
  long ret = 0;
  ret += state_get_max_num_imm_operands(ctx, state_to);
  ret += expr_get_max_num_imm_operands(ctx, econd);
  /*if (e->get_is_indir_exit()) {
    ASSERT(indir_tgt);
    ret += expr_get_max_num_imm_operands(ctx, indir_tgt);
  }*/
  //cout << __func__ << " " << __LINE__ << ": returning " << ret << " for edge\n";
  return ret;
}

static long
pred_get_max_num_imm_operands(context *ctx, predicate_ref const &pred)
{
  long ret = 0;
  ret += expr_get_max_num_imm_operands(ctx, pred->get_lhs_expr());
  ret += expr_get_max_num_imm_operands(ctx, pred->get_rhs_expr());
  return ret;
}

long
etfg_insn_get_max_num_imm_operands(etfg_insn_t const *insn)
{
  //ASSERT(insn->m_tfg);
  ASSERT(insn->m_edges.size() > 0);
  //tfg const *t = (tfg const *)insn->m_tfg;
  //context *ctx = t->get_context();
  context *ctx = insn->get_context();
  long ret = 0;
  for (const auto &pred : insn->m_preconditions) {
    ret += pred_get_max_num_imm_operands(ctx, pred);
  }
  for (const auto &e : insn->m_edges) {
    ret += tfg_edge_get_max_num_imm_operands(ctx, e.get_tfg_edge());
  }
  return ret + 1;
}

char *
etfg_exregname_suffix(int groupnum, int exregnum, char const *suffix, char *buf, size_t size)
{
  NOT_IMPLEMENTED();
}


char *
etfg_insn_to_string(etfg_insn_t const *insn, char *buf, size_t buf_size)
{
  string llvm_insn = insn->etfg_insn_get_short_llvm_description();

  ASSERT(insn->m_edges.size() > 0);
  stringstream ss;
  bool first = true;
  //tfg const *t = (tfg const *)insn->m_tfg;
  ss << "=etfg_insn " << llvm_insn << " {{{\n";
  size_t precond_index = 0;
  for (const auto &p : insn->m_preconditions) {
    ss << "=precondition." << precond_index << "\n";
    ss << p->to_string_for_eq() << endl;
    precond_index++;
  }
  for (const auto &e : insn->m_edges) {
    //shared_ptr<tfg_edge> e = tfg_get_edge_from_edgenum(insn->m_tfg, edgenum);
    //str += (first ? string("{") : string(", ")) + e.etfg_insn_edge_to_string();
    ss << e.etfg_insn_edge_to_string(insn->get_context());
  }
  ss << "=comment\n";
  ss << insn->m_comment << "\n";
  ss << "}}}\n";;
  //str += string("}");
  strncpy(buf, ss.str().c_str(), buf_size);
  ASSERT(strlen(buf) < buf_size);
  return buf;
}

char *
etfg_iseq_to_string(etfg_insn_t const *iseq, long iseq_len,
    char *buf, size_t buf_size)
{
  char *ptr = buf, *end = buf + buf_size;
  long i;
  ASSERT(buf_size > 0);
  *ptr = '\0';
  for (i = 0; i < iseq_len; i++) {
    ASSERT(ptr < end);
    etfg_insn_to_string(&iseq[i], ptr, end - ptr);
    ptr += strlen(ptr);
    ASSERT(ptr < end);
  }
  return buf;
}

size_t
etfg_insn_to_int_array(etfg_insn_t const *insn, int64_t arr[], size_t len)
{
  NOT_IMPLEMENTED();
}

void
etfg_insn_rename_addresses_to_symbols(struct etfg_insn_t *insn,
    struct input_exec_t const *in, src_ulong pc)
{
  NOT_IMPLEMENTED();
}

void
etfg_insn_copy_symbols(struct etfg_insn_t *dst, struct etfg_insn_t const *src)
{
  NOT_IMPLEMENTED();
}

static bool
tfg_edges_equal(shared_ptr<tfg_edge const> const &e1, shared_ptr<tfg_edge const> const &e2)
{
  if (e1->get_cond() != e2->get_cond()) {
    return false;
  }
  if (e1->get_to_state().state_to_string_for_eq() != e2->get_to_state().state_to_string_for_eq()) {
    return false;
  }
  return true;
}

static bool
etfg_insn_edges_equal(struct etfg_insn_edge_t const &e1, struct etfg_insn_edge_t const &e2)
{
  if (e1.get_pcrels().size() != e2.get_pcrels().size()) {
    return false;
  }
  for (size_t i = 0; i < e1.get_pcrels().size(); i++) {
    if (e1.get_pcrels().at(i).val != e2.get_pcrels().at(i).val) {
      return false;
    }
    if (e1.get_pcrels().at(i).tag != e2.get_pcrels().at(i).tag) {
      return false;
    }
  }
  if (!tfg_edges_equal(e1.get_tfg_edge(), e2.get_tfg_edge())) {
    return false;
  }
  return true;
}

bool
etfg_insns_equal(struct etfg_insn_t const *i1, struct etfg_insn_t const *i2)
{
  if (i1->m_preconditions.size() != i2->m_preconditions.size()) {
    return false;
  }
  auto iter2 = i2->m_preconditions.begin();
  for (auto iter1 = i1->m_preconditions.begin(); iter1 != i1->m_preconditions.end(); iter1++, iter2++) {
    ASSERT(iter2 != i2->m_preconditions.end());
    if (!(*iter1 == *iter2)) {
      return false;
    }
  }
  if (i1->m_edges.size() != i2->m_edges.size()) {
    return false;
  }
  for (size_t i = 0; i < i1->m_edges.size(); i++) {
    if (!etfg_insn_edges_equal(i1->m_edges.at(i), i2->m_edges.at(i))) {
      return false;
    }
  }
  return true;
}

size_t
etfg_insn_get_symbols(struct etfg_insn_t const *insn, long const *start_symbol_ids, long *symbol_ids, size_t max_symbol_ids)
{
  NOT_IMPLEMENTED();
}

static exreg_group_id_t
etfg_exreg_group_id_from_expression_name(string const &expr_name)
{
  ASSERT(expr_name.find(G_INPUT_KEYWORD) == string::npos);
  if (expr_name.find(G_LLVM_HIDDEN_REGISTER_NAME) != string::npos) {
    return ETFG_EXREG_GROUP_HIDDEN_REGS;
  } else if (expr_name.find(G_LLVM_RETURN_REGISTER_NAME) != string::npos) {
    return ETFG_EXREG_GROUP_RETURN_REGS;
  } else if (expr_name.find(LLVM_CALLEE_SAVE_REGNAME) != string::npos) {
    return ETFG_EXREG_GROUP_CALLEE_SAVE_REGS;
  } else {
    return ETFG_EXREG_GROUP_GPRS;
  }
}

static reg_identifier_t
expr_to_reg_id(expr_ref e)
{
  context *ctx = e->get_context();
  consts_struct_t const &cs = ctx->get_consts_struct();
  if (e->get_operation_kind() == expr::OP_ITE) {
    ASSERT(e->get_args().at(1) == ctx->mk_bv_const(1, 1));
    ASSERT(e->get_args().at(2) == ctx->mk_bv_const(1, 0));
    e = e->get_args().at(0);
  }
  string name = expr_string(e);
  if (name.substr(0, strlen(G_INPUT_KEYWORD ".")) == G_INPUT_KEYWORD ".") {
    return get_reg_identifier_for_regname(name.substr(strlen(G_INPUT_KEYWORD ".")));
  } else if (e->is_const() || cs.is_symbol(e) || expr_is_canonical_constant(e, NULL) || cs.expr_represents_llvm_undef(e)) {
    return get_reg_identifier_for_regname(string(REG_ASSIGNED_TO_CONST_PREFIX) + name);
  } else {
    return get_reg_identifier_for_regname(name);
  }
}

class expr_matches_template_visitor {
public:
  expr_matches_template_visitor(context *ctx, expr_ref const &a, expr_ref const &b, vector<src_iseq_match_renamings_t> const &renamings, bool var) : m_a(a), m_b(b), m_renamings(renamings)/*, m_var(var)*/, m_ctx(ctx), m_cs(ctx->get_consts_struct())
  {
    ASSERT(renamings.size() > 0);
  }

  vector<src_iseq_match_renamings_t> get_result() { return visit_recursive(m_a, m_b, m_renamings); }

  static bool match_areg_with_breg_for_renaming(exreg_group_id_t group, exreg_id_t a_regnum, reg_identifier_t const &b_reg_in, bool var, src_iseq_match_renamings_t &renaming)
  {
    reg_identifier_t b_reg = b_reg_in;
    exreg_group_id_t b_group = var ? group : -1;
    exreg_id_t b_regnum;
    DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": group = " << group << ", a_regnum = " << a_regnum << ", b_reg_in = " << b_reg_in.reg_identifier_to_string() << endl);
    if (!var && string_is_register_id(b_reg.reg_identifier_to_string(), b_group, b_regnum)) {
      if (b_group != group) {
        DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": group = " << group << ", b_group = " << b_group << ", b_reg_in = " << b_reg_in.reg_identifier_to_string() << endl);
        return false;
      }
      b_reg = reg_identifier_t(b_regnum);
    }
    regmap_t const &st_regmap = renaming.get_regmap();
    if (   st_regmap.regmap_extable.count(group)
        && st_regmap.regmap_extable.at(group).count(reg_identifier_t(a_regnum))) {
      bool ret = st_regmap.regmap_extable.at(group).at(reg_identifier_t(a_regnum)) == b_reg;
      //ASSERT(m_st_regmap->regmap_extable.at(group).at(reg_identifier_t(a_regnum)).reg_identifier_get_str_id() != "0" || b_reg.reg_identifier_get_str_id() == "0");
      DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": returning " << ret << " for group " << group << ", regnum " << a_regnum << ", st_regmap->regmap_extable.at(group).at(a_regnum) = " << st_regmap.regmap_extable.at(group).at(reg_identifier_t(a_regnum)).reg_identifier_to_string() << ", b_reg " << b_reg.reg_identifier_to_string() << endl);
      return ret;
    } else {
      if (b_group == -1) {
        ASSERT(!var);
        b_group = etfg_exreg_group_id_from_expression_name(b_reg.reg_identifier_to_string());
      }
      if (b_group != group) {
        ASSERT(!var);
        DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": group = " << group << ", b_group = " << b_group << ", b_reg = " << b_reg.reg_identifier_to_string() << ", a_regnum = " << a_regnum << endl);
        return false;
      }
      regmap_t r = st_regmap;
      r.regmap_extable[group].insert(make_pair(reg_identifier_t(a_regnum), b_reg));
      renaming.set_regmap(r);
      DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": returning true for group " << group << ", regnum " << a_regnum << ", b_reg " << b_reg.reg_identifier_to_string() << endl);
      return true;
    }
  }

  static vector<src_iseq_match_renamings_t> match_areg_with_breg(exreg_group_id_t group, exreg_id_t a_regnum, reg_identifier_t const &b_reg_in, bool var, vector<src_iseq_match_renamings_t> const &input_renamings)
  {
    vector<src_iseq_match_renamings_t> output_renamings;
    for (auto input_renaming : input_renamings) {
      if (match_areg_with_breg_for_renaming(group, a_regnum, b_reg_in, var, input_renaming)) {
        output_renamings.push_back(input_renaming);
      }
    }
    return output_renamings;
  }

private:
  vector<src_iseq_match_renamings_t> visit_recursive(expr_ref const &a, expr_ref const &b, vector<src_iseq_match_renamings_t> const &input_renamings);
  expr_ref const &m_a;
  expr_ref const &m_b;
  vector<src_iseq_match_renamings_t> const &m_renamings;
  //bool m_var;
  context *m_ctx;
  consts_struct_t const &m_cs;
  set<pair<expr_id_t, expr_id_t>> m_visited;
};

static bool
etfg_expr_match_imm_vt_field_for_renaming(valtag_t const &bvt, valtag_t const &avt, src_iseq_match_renamings_t &renaming)
{
  imm_vt_map_t imm_vt_map_copy = renaming.get_imm_vt_map();
  MATCH_IMM_VT_FIELD(bvt, avt, (&imm_vt_map_copy));
  renaming.set_imm_vt_map(imm_vt_map_copy);
  DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": returning true\n");
  return true;
}

static vector<src_iseq_match_renamings_t>
etfg_expr_match_imm_vt_field(valtag_t const &bvt, valtag_t const &avt, vector<src_iseq_match_renamings_t> const &input_renamings)
{
  vector<src_iseq_match_renamings_t> output_renamings;
  for (auto input_renaming : input_renamings) {
    if (etfg_expr_match_imm_vt_field_for_renaming(bvt, avt, input_renaming)) {
      output_renamings.push_back(input_renaming);
    }
  }
  return output_renamings;
}

static list<list<expr_ref>>
get_all_arg_reorderings(expr_ref const &e)
{
  expr::operation_kind op = e->get_operation_kind();
  list<expr_ref> arglist = expr_get_arg_list(e);
  if (expr::operation_is_commutative_and_associative(op)) {
    permute_t<expr_ref> permute(arglist);
    list<list<expr_ref>> ret = permute.get_all_permutations();
    if (ret.size() == 0) {
      cout << __func__ << " " << __LINE__ << ": e = " << expr_string(e) << endl;
    }
    ASSERT(ret.size() > 0);
    return ret;
  } else {
    list<list<expr_ref>> ret;
    ret.push_back(arglist);
    return ret;
  }
}

//`a` represents the template, and `b` represents the input
vector<src_iseq_match_renamings_t>
expr_matches_template_visitor::visit_recursive(expr_ref const &a, expr_ref const &b, vector<src_iseq_match_renamings_t> const &input_renamings)
{
  ASSERT(input_renamings.size() > 0);
  if (m_visited.count(make_pair(a->get_id(), b->get_id()))) {
    return input_renamings;
  }
  DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": a = " << expr_string(a) << ", b = " << expr_string(b) << endl);
  if (a->is_memlabel_sort() && b->is_memlabel_sort()) {
    return input_renamings; //ignore memlabels
  }
  if (a->is_nextpc_const() != b->is_nextpc_const()) {
    DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": returning false" << endl);
    return vector<src_iseq_match_renamings_t>();
  }
  if (a->is_nextpc_const()) {
    if (b->is_nextpc_const()) {
      return input_renamings;
    } else {
      DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": returning false" << endl);
      return vector<src_iseq_match_renamings_t>();
    }
  }
  if (a->is_const() && a != b) {
    DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": returning false" << endl);
    return vector<src_iseq_match_renamings_t>();
  }
  bool a_is_reg_id = false;
  exreg_group_id_t a_group, b_group;
  exreg_id_t a_regnum, b_regnum;
  size_t asize;
  if (   a->is_var()
      && a->is_input_reg_id(a_group, a_regnum)) {
    a_is_reg_id = true;
    ASSERT(a->is_bv_sort());
    asize = a->get_sort()->get_size();
  } else if (   a->is_bv_sort()
             && a->get_operation_kind() == expr::OP_BVEXTRACT
             && a->get_args().at(0)->is_var()
             && a->get_args().at(0)->is_input_reg_id(a_group, a_regnum)
             && a->get_args().at(2)->get_int_value() == 0) {
    a_is_reg_id = true;
    asize = a->get_args().at(1)->get_int_value() + 1;
  } else if (a->get_operation_kind() == expr::OP_EQ) {
    int argnum_const1 = -1;
    int other_argnum = -1;
    if (a->get_args().at(1) == m_ctx->mk_bv_const(1, 1)) {
      argnum_const1 = 1;
      other_argnum = 0;
    } else if (a->get_args().at(0) == m_ctx->mk_bv_const(1, 1)) {
      argnum_const1 = 0;
      other_argnum = 1;
    }
    if (argnum_const1 != -1) {
      ASSERT(argnum_const1 == 0 || argnum_const1 == 1);
      ASSERT(argnum_const1 + other_argnum == 1);
      if (   a->get_args().at(other_argnum)->get_operation_kind() == expr::OP_BVEXTRACT
          && a->get_args().at(other_argnum)->get_args().at(0)->is_var()
          && a->get_args().at(other_argnum)->get_args().at(0)->is_input_reg_id(a_group, a_regnum)
          && a->get_args().at(other_argnum)->get_args().at(2)->get_int_value() == 0) {
        a_is_reg_id = true;
        asize = 1;
      }
    }
  }
  DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": a_is_reg_id = " << a_is_reg_id << endl);
  if (a_is_reg_id) {
    if (   expr_is_potential_llvm_reg_id(b)
        && (b->is_bool_sort() || b->get_sort()->get_size() <= asize)) {
      reg_identifier_t b_reg_id = expr_to_reg_id(b);
      DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": calling match_areg_with_breg()" << endl);
      //bool ret = match_areg_with_breg(a_group, a_regnum, b_reg_id, false);
      //DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": returning " << ret << endl);
      //return ret;
      return match_areg_with_breg(a_group, a_regnum, b_reg_id, false, input_renamings);
    } else if (   b->get_operation_kind() == expr::OP_BVEXTRACT
               && b->get_args().at(0)->is_var()
               && b->get_args().at(0)->is_input_reg_id(b_group, b_regnum)
               && a_group == b_group
               && b->get_args().at(2)->get_int_value() == 0
               && b->get_args().at(1)->get_int_value() < asize) {
      DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": calling match_areg_with_breg()" << endl);
      return match_areg_with_breg(a_group, a_regnum, reg_identifier_t(b_regnum), true, input_renamings);
      //bool ret = match_areg_with_breg(a_group, a_regnum, reg_identifier_t(b_regnum), true);
      //DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": returning " << ret << endl);
      //return ret;
    } else if (   b->get_operation_kind() == expr::OP_EQ) {
      int argnum_const1 = -1;
      int other_argnum = -1;
      if (b->get_args().at(1) == m_ctx->mk_bv_const(1, 1)) {
        argnum_const1 = 1;
        other_argnum = 0;
      } else if (b->get_args().at(0) == m_ctx->mk_bv_const(1, 1)) {
        argnum_const1 = 0;
        other_argnum = 1;
      }
      if (argnum_const1 != -1) {
        ASSERT(argnum_const1 == 0 || argnum_const1 == 1);
        ASSERT(argnum_const1 + other_argnum == 1);
        if (   b->get_args().at(other_argnum)->get_operation_kind() == expr::OP_BVEXTRACT
            && b->get_args().at(other_argnum)->get_args().at(0)->is_var()
            && b->get_args().at(other_argnum)->get_args().at(0)->is_input_reg_id(b_group, b_regnum)
            && a_group == b_group
            && b->get_args().at(other_argnum)->get_args().at(2)->get_int_value() == 0
            && b->get_args().at(other_argnum)->get_args().at(1)->get_int_value() < asize) {
          DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": calling match_areg_with_breg()" << endl);
          return match_areg_with_breg(a_group, a_regnum, reg_identifier_t(b_regnum), true, input_renamings);
          //bool ret = match_areg_with_breg(a_group, a_regnum, reg_identifier_t(b_regnum), true);
          //DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": returning " << ret << endl);
          //return ret;
        }
      }
    } else if (b->is_const()) {
      DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": a = " << expr_string(a) << endl);
      DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": b = " << expr_string(b) << endl);
      DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": returning false\n");
      return vector<src_iseq_match_renamings_t>();
    }
  }
  vconstants_t vconstant;
  vconstants_t vconstant_b;
  DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": a = " << expr_string(a) << ", b = " << expr_string(b) << endl);
  if (   a->is_var()
      && expr_is_canonical_constant(a, &vconstant)
      && (b->is_const() || m_cs.is_symbol(b) || expr_is_canonical_constant(b, NULL) || m_cs.expr_represents_llvm_undef(b) || m_cs.expr_is_local_id(b, G_SRC_KEYWORD))
      && b->get_sort() == a->get_sort()) {
    valtag_t avt, bvt;
    DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": a = " << expr_string(a) << ", b = " << expr_string(b) << endl);
    avt = vconstant_to_valtag(vconstant);
    if (b->is_const()) {
      int b_val = b->get_int_value();
      bvt.tag = tag_const;
      bvt.val = b_val;
    } else if (m_cs.is_symbol(b)) {
      symbol_id_t symbol_id = consts_struct_t::get_symbol_id_from_expr(b);
      bvt.tag = tag_imm_symbol;
      bvt.val = symbol_id;
      bvt.mod.imm.modifier = IMM_UNMODIFIED;
    } else if (expr_is_canonical_constant(b, &vconstant_b)) {
      bvt = vconstant_to_valtag(vconstant_b);
    } else if (m_cs.expr_represents_llvm_undef(b)) {
      int b_val = 0;
      bvt.tag = tag_const;
      bvt.val = b_val;
    } else if (m_cs.expr_is_local_id(b, G_SRC_KEYWORD)) {
      allocstack_t local_id_stack = m_cs.expr_get_local_id(b, G_SRC_KEYWORD);
      allocsite_t local_id = local_id_stack.allocstack_get_only_allocsite();
      bvt.tag = tag_imm_local;
      bvt.val = local_id.allocsite_to_longlong();
      bvt.mod.imm.modifier = IMM_UNMODIFIED;
    } else NOT_REACHED();
    //DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": m_st_imm_map = " << imm_vt_map_to_string(&m_src_iseq_match_renamings.get_imm_vt_map(), as1, sizeof as1) << endl);
    return etfg_expr_match_imm_vt_field(bvt, avt, input_renamings);
  }
  if (a->get_operation_kind() != b->get_operation_kind()) {
    DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": returning false: a = " << expr_string(a) << ", b = " << expr_string(b) << endl);
    return vector<src_iseq_match_renamings_t>();
  }
  if (   (a->is_var() || a->is_const())
      && !(a->is_function_sort() && b->is_function_sort())
      && a != b) {
    DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": returning false\n");
    return vector<src_iseq_match_renamings_t>();
    //return false;
  }
  if (   a->is_function_sort()
      && b->is_function_sort()
      && (a->is_var() || a->is_const())) {
    bool ret = (expr_string(a) == expr_string(b));
    DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": returning " << (ret ? "true" : "false") << "\n");
    //return ret;
    return input_renamings;
  }

  if (a->get_args().size() != b->get_args().size()) {
    DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": returning false" << endl);
    return vector<src_iseq_match_renamings_t>();
  }
  vector<src_iseq_match_renamings_t> output_renamings;
  list<list<expr_ref>> a_arg_reorderings = get_all_arg_reorderings(a);
  ASSERT(a_arg_reorderings.size() > 0);
  for (const auto &a_arg_reordering : a_arg_reorderings) {
    ASSERT(a_arg_reordering.size() == b->get_args().size());
    vector<src_iseq_match_renamings_t> out_renamings = input_renamings;
    ASSERT(out_renamings.size() > 0);
    list<expr_ref>::const_iterator iter;
    size_t i;
    for (iter = a_arg_reordering.begin(), i = 0; iter != a_arg_reordering.end() && i < a_arg_reordering.size(); iter++, i++) {
      out_renamings = visit_recursive(*iter, b->get_args().at(i), out_renamings);
      if (out_renamings.size() == 0) {
        break;
      }
    }
    DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": out_renamings.size() = " << out_renamings.size() << endl);
    vector_append(output_renamings, out_renamings);
  }
  m_visited.insert(make_pair(a->get_id(), b->get_id()));
  DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": a = " << expr_string(a) << ", b = " << expr_string(b) << ": output_renamings.size() = " << output_renamings.size() << endl);
  return output_renamings;
}

static vector<src_iseq_match_renamings_t>
expr_matches_template(expr_ref const &te, expr_ref const &e,
    vector<src_iseq_match_renamings_t> const &renamings,
    expr_ref const &te_start, bool var)
{
  ASSERT(renamings.size() > 0);
  context *ctx = e->get_context();
  expr_ref te_lsb = te;
  if (te->get_sort() != te_start->get_sort()) {
    cout << __func__ << " " << __LINE__ << ": te = " << expr_string(te) << endl;
    cout << __func__ << " " << __LINE__ << ": te_start = " << expr_string(te_start) << endl;
  }
  ASSERT(te->get_sort() == te_start->get_sort());
  if (   e->is_bv_sort()
      && te->is_bv_sort()
      && e->get_sort()->get_size() < te->get_sort()->get_size()
      //&& ctx->expr_do_simplify_no_const_specific(ctx->mk_bvextract(te, te->get_sort()->get_size() - 1, e->get_sort()->get_size())) == ctx->expr_do_simplify_no_const_specific(ctx->mk_bvextract(te_start, te->get_sort()->get_size() - 1, e->get_sort()->get_size()))
  ) {
    te_lsb = ctx->expr_do_simplify_no_const_specific(ctx->mk_bvextract(te, e->get_sort()->get_size() - 1, 0));
  }
  if (   e->is_bool_sort()
      && te->is_bv_sort()
      && ctx->expr_do_simplify_no_const_specific(ctx->mk_bvextract(te, te->get_sort()->get_size() - 1, 1)) == ctx->expr_do_simplify_no_const_specific(ctx->mk_bvextract(te_start, te->get_sort()->get_size() - 1, 1))) {
    te_lsb = ctx->expr_do_simplify_no_const_specific(ctx->mk_eq(ctx->mk_bvextract(te, 0, 0), ctx->mk_bv_const(1, 1)));
  }

  expr_ref te_lsb_simpl = ctx->expr_do_simplify_no_const_specific(te_lsb);
  expr_ref e_simpl = ctx->expr_do_simplify_no_const_specific(e);
  DYN_DEBUG(insn_match, cout << __func__ << " " << __LINE__ << ": te = " << ctx->expr_to_string_table(te) << endl);
  DYN_DEBUG(insn_match, cout << __func__ << " " << __LINE__ << ": te = " << expr_string(te) << endl);
  DYN_DEBUG(insn_match, cout << __func__ << " " << __LINE__ << ": te_lsb = " << expr_string(te_lsb) << endl);
  DYN_DEBUG(insn_match, cout << __func__ << " " << __LINE__ << ": te_lsb_simpl = " << expr_string(te_lsb_simpl) << endl);
  DYN_DEBUG(insn_match, cout << __func__ << " " << __LINE__ << ": e = " << expr_string(e) << endl);
  DYN_DEBUG(insn_match, cout << __func__ << " " << __LINE__ << ": e_simpl = " << expr_string(e_simpl) << endl);
  expr_matches_template_visitor visitor(ctx, te_lsb_simpl, e_simpl, renamings, var);
  vector<src_iseq_match_renamings_t> ret = visitor.get_result();
  //DYN_DEBUG(insn_match, cout << __func__ << " " << __LINE__ << ": ret = " << ret << endl);
  //DYN_DEBUG(insn_match, cout << __func__ << " " << __LINE__ << ": st_imm_map = " << imm_vt_map_to_string(&src_iseq_match_renamings.get_imm_vt_map(), as1, sizeof as1) << endl);
  //if (!ret) {
  //  //try with non-simplified version
  //  expr_matches_template_visitor visitor(ctx, te_lsb, e, st_regmap, st_imm_map, nextpc_map, var);
  //  ret = visitor.get_result();
  //}
  return ret;
}

static vector<src_iseq_match_renamings_t>
state_key_registers_match(string const &treg, string const &sreg, vector<src_iseq_match_renamings_t> const&input_renamings, bool var)
{
  exreg_group_id_t tgroup;
  exreg_id_t tregnum;
  if (!string_is_register_id(treg, tgroup, tregnum)) {
    bool ret = (treg == sreg);
    DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": returning " << ret << endl);
    return ret ? input_renamings : vector<src_iseq_match_renamings_t>();
  }
  /*if (treg == sreg) {
    DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": returning true" << endl);
    return true;
  }*/
  vector<src_iseq_match_renamings_t> output_renamings;
  for (auto src_iseq_match_renamings : input_renamings) {
    exreg_group_id_t sgroup;
    exreg_id_t sregnum;
    regmap_t st_regmap = src_iseq_match_renamings.get_regmap();
    if (   var
        && string_is_register_id(sreg, sgroup, sregnum)) {
      if (tgroup != sgroup) {
        DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": returning false" << endl);
        continue;
      }
      if (   st_regmap.regmap_extable.count(tgroup)
          && st_regmap.regmap_extable.at(tgroup).count(tregnum)) {
        bool ret = st_regmap.regmap_extable.at(tgroup).at(tregnum) == reg_identifier_t(sregnum);
        DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": returning " << ret << endl);
        if (!ret) {
          continue;
        }
      } else {
        st_regmap.regmap_extable[tgroup].insert(make_pair(reg_identifier_t(tregnum), reg_identifier_t(sregnum)));
      }
    } else {
      if (   st_regmap.regmap_extable.count(tgroup)
          && st_regmap.regmap_extable.at(tgroup).count(tregnum)) {
        bool ret = st_regmap.regmap_extable.at(tgroup).at(tregnum) == get_reg_identifier_for_regname(sreg);
        DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": returning " << ret << endl);
        if (!ret) {
          continue;
        }
      } else {
        if (treg == sreg) {
          st_regmap.regmap_extable[tgroup].insert(make_pair(reg_identifier_t(tregnum), reg_identifier_t(tregnum)));
          //return true;
        } else {
          exreg_group_id_t sreg_group_id = etfg_exreg_group_id_from_expression_name(sreg);
          if (sreg_group_id != tgroup) {
            DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": returning false. sreg = " << sreg << ", treg = " << treg << ", sreg_group_id = " << sreg_group_id << ", tgroup = " << tgroup << endl);
            continue;
          }
          st_regmap.regmap_extable[tgroup].insert(make_pair(reg_identifier_t(tregnum), get_reg_identifier_for_regname(sreg)));
        }
      }
    }
    DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": returning true for " << treg << " and " << sreg << endl);
    src_iseq_match_renamings.set_regmap(st_regmap);
    output_renamings.push_back(src_iseq_match_renamings);
  }
  return output_renamings;
}

static bool
key_represents_intermediate_value(string const &k)
{
  if (k.find(LLVM_INTERMEDIATE_VALUE_KEYWORD) != string::npos) {
    return true;
  }
  return false;
}

static void
key_value_map_erase_intermediate_keys(map<string_ref, expr_ref> &m)
{
  set<string_ref> intermediate_keys;
  for (const auto &e : m) {
    if (key_represents_intermediate_value(e.first->get_str())) {
      intermediate_keys.insert(e.first);
    }
  }
  for (const auto &k : intermediate_keys) {
    m.erase(k);
  }
}

static vector<src_iseq_match_renamings_t>
state_matches_template(state const &ts, state const &s,
    vector<src_iseq_match_renamings_t> const &src_iseq_match_renamings,
    state const &t_start_state, bool var)
{
  map<string_ref, expr_ref> const &ts_map = ts.get_value_expr_map_ref();
  map<string_ref, expr_ref> s_map = s.get_value_expr_map_ref();
  key_value_map_erase_intermediate_keys(s_map);
  DYN_DEBUG(insn_match,
      cout << __func__ << " " << __LINE__ << ": ts_map =\n";
      for (auto tse : ts_map) {
         cout << tse.first << ": " << expr_string(tse.second) << endl;
      }
      cout << __func__ << " " << __LINE__ << ": s_map =\n";
      for (auto se : s_map) {
         cout << se.first << ": " << expr_string(se.second) << endl;
      }
  );
  if (ts_map.size() != s_map.size()) {
    DYN_DEBUG(insn_match, cout << __func__ << " " << __LINE__ << ": returning false\n");
    return vector<src_iseq_match_renamings_t>();
  }
  set<string> s_map_keys_matched;
  auto renamings = src_iseq_match_renamings;
  for (const auto &tsp : ts_map) {
    string const &tskey = tsp.first->get_str();
    expr_ref const &tsexpr = tsp.second;

    DYN_DEBUG(insn_match, cout << __func__ << " " << __LINE__ << ": checking " << tskey << ": " << expr_string(tsexpr) << "\n");
    //bool found_match = false;
    for (auto &sp : s_map) {
      string const &skey = sp.first->get_str();
      expr_ref const &sexpr = sp.second;
      if (set_belongs(s_map_keys_matched, skey)) {
        continue;
      }
      vector<src_iseq_match_renamings_t> irenamings = renamings;
      irenamings = state_key_registers_match(tskey, skey, irenamings, var);
      if (irenamings.size() == 0) {
        continue;
      }

      irenamings = expr_matches_template(tsexpr, sexpr, irenamings, t_start_state.get_expr(tskey/*, t_start_state*/), var);
      if (irenamings.size() == 0) {
        continue;
      }
      //DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": found match for " << skey << " with " << tskey << ": st_regmap = " << regmap_to_string(&src_iseq_match_renamings.get_regmap(), as1, sizeof as1) << endl);
      s_map_keys_matched.insert(skey);
      renamings = irenamings;
      //delete src_iseq_match_renamings_copy;
      break;
    }
    if (renamings.size() == 0) {
      //DYN_DEBUG(insn_match, cout << __func__ << " " << __LINE__ << ": returning false for " << tskey << ": " << expr_string(tsexpr) << "\n");
      //DYN_DEBUG(insn_match, cout << __func__ << " " << __LINE__ << ": sp =\n");
      //DYN_DEBUG(insn_match,
      //    for (auto &sp : s_map) {
      //      cout << sp.first << ": " << expr_string(sp.second) << endl;
      //    }
      //);
      return renamings;
    }
  }
  for (const auto &sp : s_map) {
    if (s_map_keys_matched.find(sp.first->get_str()) == s_map_keys_matched.end()) {
      return vector<src_iseq_match_renamings_t>();
    }
  }
  return renamings;
}

static bool
assumes_some_assume_matches_template_assume(unordered_set<predicate_ref> const &assumes,
    predicate_ref const &t_assume, struct regmap_t *st_regmap,
    struct imm_vt_map_t *st_imm_map,
    struct nextpc_map_t *nextpc_map,
    state const &t_start_state,
    bool var)
{
  NOT_IMPLEMENTED();
}

static bool
assumes_match_template(unordered_set<predicate_ref> const &t_assumes,
    unordered_set<predicate_ref> const &assumes,
    struct regmap_t *st_regmap,
    struct imm_vt_map_t *st_imm_map,
    struct nextpc_map_t *nextpc_map,
    state const &t_start_state,
    bool var)
{
  for (const auto &t_assume : t_assumes) {
    if (!assumes_some_assume_matches_template_assume(assumes, t_assume, st_regmap, st_imm_map, nextpc_map, t_start_state, var)) {
      return false;
    }
  }
  return true;
}

static vector<src_iseq_match_renamings_t>
tfg_edge_matches_template(shared_ptr<tfg_edge const> const &te, shared_ptr<tfg_edge const> const &e,
    src_iseq_match_renamings_t const &input_renamings,
    state const &t_start_state, context *ctx,
    bool var)
{
  vector<src_iseq_match_renamings_t> renamings;
  renamings.push_back(input_renamings);
  //DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": st_imm_map =\n" << imm_vt_map_to_string(&src_iseq_match_renamings.get_imm_vt_map(), as1, sizeof as1) << endl);
  renamings = expr_matches_template(te->get_cond(), e->get_cond(), renamings, expr_true(ctx), var);
  //DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": st_imm_map =\n" << imm_vt_map_to_string(&src_iseq_match_renamings.get_imm_vt_map(), as1, sizeof as1) << endl);
  renamings = state_matches_template(te->get_to_state(), e->get_to_state(), renamings, t_start_state, var);
  //vector<src_iseq_match_renamings_t> ret;
  //ret.push_back(src_iseq_match_renamings);
  return renamings;
}

static bool
etfg_nextpc_valtags_match(valtag_t const &i_pcrel, valtag_t const &t_pcrel, nextpc_map_t &nextpc_map, bool var)
{
  MATCH_NEXTPC_VALTAG(i_pcrel, t_pcrel, (&nextpc_map), var);
  return true;
}

static vector<src_iseq_match_renamings_t>
etfg_insn_edge_matches_template_for_renaming(etfg_insn_edge_t const &_template, etfg_insn_edge_t const &insn_edge,
    src_iseq_match_renamings_t const &input_renaming,
    state const &start_state, context *ctx, bool var)
{
  autostop_timer func_timer(__func__);
  //cout << __func__ << " " << __LINE__ << ": _template = " << _template.etfg_insn_edge_to_string(ctx) << endl;
  //cout << __func__ << " " << __LINE__ << ": insn_edge = " << insn_edge.etfg_insn_edge_to_string(ctx) << endl;
  //DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": st_imm_vt_map =\n" << imm_vt_map_to_string(&src_iseq_match_renamings.get_imm_vt_map(), as1, sizeof as1) << endl);
  if (_template.get_pcrels().size() != insn_edge.get_pcrels().size()) {
    DYN_DEBUG(insn_match, cout << __func__ << " " << __LINE__ << ": returning false. template.get_pcrels().size() = " << _template.get_pcrels().size() << ", insn_edge.get_pcrels().size() = " << insn_edge.get_pcrels().size() << "\n");
    return vector<src_iseq_match_renamings_t>();
  }
  shared_ptr<tfg_edge const> t_edge = _template.get_tfg_edge();
  shared_ptr<tfg_edge const> i_edge = insn_edge.get_tfg_edge();
  vector<src_iseq_match_renamings_t> output_renamings;
  output_renamings = tfg_edge_matches_template(t_edge, i_edge, input_renaming, start_state, ctx, var);
  //DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": st_imm_vt_map =\n" << imm_vt_map_to_string(&src_iseq_match_renamings.get_imm_vt_map(), as1, sizeof as1) << endl);
  vector<src_iseq_match_renamings_t> ret;
  for (auto src_iseq_match_renamings : output_renamings) {
    nextpc_map_t nextpc_map = src_iseq_match_renamings.get_nextpc_map();
    bool mismatch = false;
    for (size_t i = 0; i < _template.get_pcrels().size(); i++) {
      valtag_t const &t_pcrel = _template.get_pcrels().at(i);
      valtag_t const &i_pcrel = insn_edge.get_pcrels().at(i);
      {
        //pcrel2str(i_pcrel.val, i_pcrel.tag, NULL, 0, as1, sizeof as1);
        //pcrel2str(t_pcrel.val, t_pcrel.tag, NULL, 0, as2, sizeof as2);
        //cout << __func__ << " " << __LINE__ << ": comparing pcrels: " <<  as1 << " <-> " << as2 << endl;
        //cout << __func__ << " " << __LINE__ << ": nextpc_map = " << nextpc_map_to_string(&nextpc_map, as1, sizeof as1) << endl;
      }
      if (!etfg_nextpc_valtags_match(i_pcrel, t_pcrel, nextpc_map, var)) {
        DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": pcrel mismatch: " << pcrel2str(i_pcrel.val, i_pcrel.tag, NULL, 0, as1, sizeof as1) << " <-> " << pcrel2str(t_pcrel.val, t_pcrel.tag, NULL, 0, as2, sizeof as2) << endl);
        mismatch = true;
        break;
      }
    }
    if (!mismatch) {
      if (_template.etfg_insn_edge_is_indirect_branch()) {
        ASSERT(insn_edge.etfg_insn_edge_is_indirect_branch());
        nextpc_map_add_indir_nextpc_if_not_present(&nextpc_map);
      }
      src_iseq_match_renamings.set_nextpc_map(nextpc_map);
      ret.push_back(src_iseq_match_renamings);
    }
    //DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": returning true. st_imm_vt_map =\n" << imm_vt_map_to_string(&src_iseq_match_renamings.get_imm_vt_map(), as1, sizeof as1) << endl);
    //return true;
  }
  return ret;
}


static vector<src_iseq_match_renamings_t>
etfg_insn_edge_matches_template(etfg_insn_edge_t const &_template, etfg_insn_edge_t const &insn_edge,
    vector<src_iseq_match_renamings_t> const &input_renamings,
    //vector<src_iseq_match_renamings_t> &output_renamings,
    state const &start_state, context *ctx,
    bool var)
{
  autostop_timer func_timer(__func__);
  vector<src_iseq_match_renamings_t> output_renamings;
  for (const auto &input_renaming : input_renamings) {
    vector<src_iseq_match_renamings_t> out_renamings;
    out_renamings = etfg_insn_edge_matches_template_for_renaming(_template, insn_edge, input_renaming, start_state, ctx, var);
    vector_append(output_renamings, out_renamings);
  }
  return output_renamings;
}

static vector<src_iseq_match_renamings_t>
predicate_matches(predicate_ref const &_template, predicate_ref const &pred,
    vector<src_iseq_match_renamings_t> const &input_renamings,
    bool var)
{
  ASSERT(input_renamings.size() > 0);
  context *ctx = _template->get_context();
  auto renamings = input_renamings;
  renamings = expr_matches_template(_template->get_lhs_expr(), pred->get_lhs_expr(), renamings, expr_true(ctx), var);
  if (renamings.size() == 0) {
    return renamings;
  }
  renamings = expr_matches_template(_template->get_rhs_expr(), pred->get_rhs_expr(), renamings, expr_true(ctx), var);
  return renamings;
}

static expr_ref
imm_valtag_to_etfg_exreg_len_extended_expr_ref(context *ctx, valtag_t const *imm_vt, size_t etfg_exreg_len)
{
  consts_struct_t const &cs = ctx->get_consts_struct();
  expr_ref e = imm_valtag_to_expr_ref(ctx, imm_vt, &cs);
  ASSERT(e->is_bv_sort());
  if (etfg_exreg_len > e->get_sort()->get_size()) {
    return ctx->mk_bvzero_ext(e, etfg_exreg_len - e->get_sort()->get_size());
  } else if (etfg_exreg_len == e->get_sort()->get_size()) {
    return e;
  } else {
    NOT_REACHED();
  }
}

static map<expr_id_t, pair<expr_ref, expr_ref>>
get_submap_for_regmap_and_imm_map(context *ctx, struct regmap_t const *st_regmap, struct imm_vt_map_t const *st_imm_map)
{
  map<expr_id_t, pair<expr_ref, expr_ref>> ret;
  consts_struct_t const &cs = ctx->get_consts_struct();
  if (st_regmap) {
    for (const auto &g : st_regmap->regmap_extable) {
      for (const auto &r : g.second) {
        expr_ref from_expr = ctx->mk_var(string(G_INPUT_KEYWORD) + "." + G_SRC_KEYWORD + "." + etfg_regname(g.first, r.first), ctx->mk_bv_sort(ETFG_EXREG_LEN(g.first)));
        expr_ref to_expr;
        if (r.second.reg_identifier_denotes_constant()) {
          valtag_t imm_vt = r.second.reg_identifier_get_imm_valtag();
          to_expr = imm_valtag_to_etfg_exreg_len_extended_expr_ref(ctx, &imm_vt, ETFG_EXREG_LEN(g.first));
        } else {
          to_expr = ctx->mk_var(string(G_INPUT_KEYWORD) + "." + G_SRC_KEYWORD + "." + etfg_regname(g.first, r.second), ctx->mk_bv_sort(ETFG_EXREG_LEN(g.first)));
        }
        ret[from_expr->get_id()] = make_pair(from_expr, to_expr);
      }
    }
  }
  for (int i = 0; i < st_imm_map->num_imms_used; i++) {
    valtag_t const &vt = st_imm_map->table[i];
    expr_ref from_expr = cs.get_expr_value(reg_type_const, i);
    expr_ref to_expr = imm_valtag_to_expr_ref(ctx, &vt, &cs);
    ret[from_expr->get_id()] = make_pair(from_expr, to_expr);
  }
  return ret;
}

static bool
expr_evaluates_to_true_on_regmap_and_imm_map(expr_ref const &e, struct regmap_t const *st_regmap, struct imm_vt_map_t const *st_imm_map)
{
  context *ctx = e->get_context();
  map<expr_id_t, pair<expr_ref, expr_ref>> submap = get_submap_for_regmap_and_imm_map(ctx, st_regmap, st_imm_map);
  expr_ref ret = ctx->expr_substitute(e, submap);
  ret = ctx->expr_do_simplify(ret);
  return ret == ctx->mk_bool_true();
}

static bool
predicate_evaluates_to_true_on_regmap_and_imm_map(predicate_ref const &pred, struct regmap_t const *st_regmap, struct imm_vt_map_t const *st_imm_map, string const& insn_match_prefix)
{
  ASSERT(pred->get_precond().precond_is_trivial());
  context *ctx = pred->get_context();
  expr_ref e = ctx->mk_eq(pred->get_lhs_expr(), pred->get_rhs_expr());
  DYN_DEBUGS2(insn_match_prefix.c_str(), cout << __func__ << " " << __LINE__ << ": evaluated pred\n" << pred->to_string_for_eq() << " on regmap\n" << regmap_to_string(st_regmap, as1, sizeof as1) << "\nand on imm_map\n" << imm_vt_map_to_string(st_imm_map, as2, sizeof as2) << ": e = " << expr_ref(e) << endl);
  bool ret = expr_evaluates_to_true_on_regmap_and_imm_map(e, st_regmap, st_imm_map);
  DYN_DEBUGS2(insn_match_prefix.c_str(), cout << __func__ << " " << __LINE__ << ": evaluates_to_true = " << ret << endl);
  return ret;
}

static vector<src_iseq_match_renamings_t>
etfg_precondition_matches_with_any_candidate(predicate_ref const &p, list<predicate_ref> const &candidates, vector<src_iseq_match_renamings_t> const &input_renamings, bool var, string const& insn_match_prefix)
{
  vector<src_iseq_match_renamings_t> output_renamings;
  for (const auto &src_iseq_match_renamings : input_renamings) {
    if (predicate_evaluates_to_true_on_regmap_and_imm_map(p, &src_iseq_match_renamings.get_regmap(), &src_iseq_match_renamings.get_imm_vt_map(), insn_match_prefix)) {
      output_renamings.push_back(src_iseq_match_renamings);
    } else {
      //bool found_match = false;
      list<predicate_ref>::const_iterator iter;
      for (iter = candidates.begin(); iter != candidates.end(); iter++) {
        //src_iseq_match_renamings_t src_iseq_match_renamings_copy = src_iseq_match_renamings;
        vector<src_iseq_match_renamings_t> irenamings;
        irenamings.push_back(src_iseq_match_renamings);
        
        irenamings = predicate_matches(p, *iter, irenamings, var);
        if (irenamings.size() == 0) {
          continue;
        }
        //src_iseq_match_renamings = src_iseq_match_renamings_copy;
        //output_renamings.push_back(src_iseq_match_renamings_copy);
        vector_append(output_renamings, irenamings);
        //found_match = true;
        break;
      }
      //if (!found_match) {
      //  DYN_DEBUG(insn_match, cout << __func__ << " " << __LINE__ << ": returning false because could not find match for following pred:\n" << p.to_string_for_eq() << endl);
      //  return vector<src_iseq_match_renamings_t>();
      //}
    }
  }
  DYN_DEBUGS2(insn_match_prefix.c_str(), cout << __func__ << " " << __LINE__ << ": returning true\n");
  return output_renamings;
}

static vector<src_iseq_match_renamings_t>
etfg_preconditions_match(list<predicate_ref> const &_template, list<predicate_ref> const &preds, vector<src_iseq_match_renamings_t> input_renamings, bool var, string const& insn_match_prefix)
{
  list<predicate_ref>::const_iterator t_iter;
  for (t_iter = _template.begin(); t_iter != _template.end(); t_iter++) {
    input_renamings = etfg_precondition_matches_with_any_candidate(*t_iter, preds, input_renamings, var, insn_match_prefix);
  }
  return input_renamings;
}

static etfg_insn_edge_t const *
find_match_for_template_edge(context *ctx, etfg_insn_edge_t const &_template, vector<etfg_insn_edge_t> const &candidates, vector<src_iseq_match_renamings_t> const &input_renamings, vector<src_iseq_match_renamings_t> &output_renamings, set<etfg_insn_edge_t const *> const &insn_edges_found_match, state const &template_start_state, bool var, string const& insn_match_prefix)
{
  autostop_timer func_timer(__func__);
  DYN_DEBUGS2(insn_match_prefix.c_str(), cout << __func__ << " " << __LINE__ << ": candidates.size() = " << candidates.size() << endl);
  for (size_t j = 0; j < candidates.size(); j++) {
    etfg_insn_edge_t const &candidate = candidates.at(j);
    DYN_DEBUGS2(insn_match_prefix.c_str(), cout << __func__ << " " << __LINE__ << ": matching insn edge " << j << " (of " << candidates.size() << ") with template edge\n");
    if (set_belongs(insn_edges_found_match, &candidate)) {
      DYN_DEBUGS2(insn_match_prefix.c_str(), cout << __func__ << " " << __LINE__ << ": insn edge " << j << " belongs to set of edges for which a match has been already found\n");
      continue;
    }

    vector<src_iseq_match_renamings_t> tmp_renamings;
    tmp_renamings = etfg_insn_edge_matches_template(_template, candidate,
             input_renamings,
             template_start_state, ctx, var);
    if (tmp_renamings.size() > 0) {
      DYN_DEBUGS2(insn_match_prefix.c_str(), cout << __func__ << " " << __LINE__ << ": found match.\n");
      //src_iseq_match_renamings = *src_iseq_match_renamings_copy;
      output_renamings = tmp_renamings;
      return &candidate;
    } else {
      DYN_DEBUGS2(insn_match_prefix.c_str(), cout << __func__ << " " << __LINE__ << ": could not match insn edge " << j << " with template edge\n");
    }
  }
  return NULL;
}

static vector<src_iseq_match_renamings_t>
etfg_insn_matches_template_for_renaming(etfg_insn_t const *_template, etfg_insn_t const *insn,
    src_iseq_match_renamings_t const &input_renaming,
    string insn_match_prefix, bool var)
{
  autostop_timer func_timer(__func__);
  context *ctx = _template->get_context();
  vector<src_iseq_match_renamings_t> ret;
  //src_iseq_match_renamings_t src_iseq_match_renamings = input_renaming;
  vector<src_iseq_match_renamings_t> input_renamings;
  input_renamings.push_back(input_renaming);
  
  //ASSERT(nextpc_map);
  //DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": st_imm_map =\n" << imm_vt_map_to_string(&src_iseq_match_renamings.get_imm_vt_map(), as1, sizeof as1) << endl);
  DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": insn_match_prefix = " << insn_match_prefix << endl);
  //cout << __func__ << " " << __LINE__ << ": prefix = " << prefix << endl;
  //cout << __func__ << " " << __LINE__ << ": insn_match_prefix = " << insn_match_prefix << endl;
  DYN_DEBUGS2(insn_match_prefix.c_str(), cout << __func__ << " " << __LINE__ << ": var = " << var << endl);
  DYN_DEBUGS2(insn_match_prefix.c_str(), cout << __func__ << " " << __LINE__ << ": comparing _template " << _template->m_comment << " with " << insn->m_comment << endl);
  DYN_DEBUGS2(insn_match_prefix.c_str(), cout << __func__ << " " << __LINE__ << ": _template =\n" << etfg_insn_to_string(_template, as1, sizeof as1) << endl);
  DYN_DEBUGS2(insn_match_prefix.c_str(), cout << __func__ << " " << __LINE__ << ": insn =\n" << etfg_insn_to_string(insn, as1, sizeof as1) << endl);
  if (_template->m_edges.size() != insn->m_edges.size()) {
    DYN_DEBUGS(insn_match_prefix.c_str(), cout << __func__ << " " << __LINE__ << ": returning false\n");
    return vector<src_iseq_match_renamings_t>();
  }
  set<etfg_insn_edge_t const *> insn_edges_found_match;
  state const &template_start_state = *_template->etfg_insn_get_start_state();
  //bool found_match = true;
  for (size_t i = 0; i < _template->m_edges.size(); i++) {
    //DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": st_imm_map =\n" << imm_vt_map_to_string(&src_iseq_match_renamings.get_imm_vt_map(), as1, sizeof as1) << endl);
    vector<src_iseq_match_renamings_t> output_renamings;
    //found_match = false;
    DYN_DEBUGS2(insn_match_prefix.c_str(), cout << __func__ << " " << __LINE__ << ": trying to match template edge " << i << "\n");
    etfg_insn_edge_t const *matching_edge = find_match_for_template_edge(ctx, _template->m_edges.at(i), insn->m_edges, input_renamings, output_renamings, insn_edges_found_match, template_start_state, var, insn_match_prefix);
    if (!matching_edge) {
      DYN_DEBUGS2(insn_match_prefix.c_str(), cout << __func__ << " " << __LINE__ << ": returning false\n");
      //found_match = false;
      //break;
      return vector<src_iseq_match_renamings_t>();
    } else {
      insn_edges_found_match.insert(matching_edge);
      input_renamings = output_renamings;
    }
  }
  if (insn_edges_found_match.size() != insn->m_edges.size()) {
    DYN_DEBUGS2(insn_match_prefix.c_str(), cout << __func__ << " " << __LINE__ << ": returning false\n");
    return vector<src_iseq_match_renamings_t>();
  }
  vector<src_iseq_match_renamings_t> output_renamings;
  output_renamings = etfg_preconditions_match(_template->m_preconditions, insn->m_preconditions, input_renamings, var, insn_match_prefix);
  DYN_DEBUGS2(insn_match_prefix.c_str(), cout << __func__ << " " << __LINE__ << ": returning " << ((output_renamings.size() > 0) ? "true\n" : "false\n"));
  //ret.push_back(src_iseq_match_renamings);
  return output_renamings;
}

static bool
etfg_insn_matches_template(etfg_insn_t const *_template, etfg_insn_t const *insn,
    vector<src_iseq_match_renamings_t> &renamings,
    string insn_match_prefix, bool var)
{
  autostop_timer func_timer(__func__);

  vector<src_iseq_match_renamings_t> input_renamings = renamings;
  renamings.clear();
  for (const auto &input_renaming : input_renamings) {
    vector<src_iseq_match_renamings_t> new_renamings;
    new_renamings = etfg_insn_matches_template_for_renaming(_template, insn, input_renaming, insn_match_prefix, var);
    for (const auto &new_renaming : new_renamings) {
      renamings.push_back(new_renaming);
    }
  }
  DYN_DEBUGS2(insn_match_prefix.c_str(), cout << __func__ << " " << __LINE__ << ": returning " << ((renamings.size() > 0) ? "true\n" : "false\n"));
  return renamings.size() > 0;
}

//bool
//etfg_iseq_matches_template_var(etfg_insn_t const *_template, etfg_insn_t const *iseq,
//    long len,
//    src_iseq_match_renamings_t &src_iseq_match_renamings,
//    //struct regmap_t *st_regmap, struct imm_vt_map_t *st_imm_vt_map,
//    //struct nextpc_map_t *nextpc_map,
//    char const *prefix
//    )
//{
//  //ASSERT(nextpc_map);
//  DYN_DEBUG(insn_match, cout << __func__ << " " << __LINE__ << ": _template =\n" << etfg_iseq_to_string(_template, len, as1, sizeof as1) << endl);
//  DYN_DEBUG(insn_match, cout << __func__ << " " << __LINE__ << ": iseq =\n" << etfg_iseq_to_string(iseq, len, as1, sizeof as1) << endl);
//  src_iseq_match_renamings.clear();
//  //regmap_init(st_regmap);
//  //imm_vt_map_init(st_imm_vt_map);
//  //nextpc_map_init(nextpc_map);
//  //DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": st_imm_vt_map =\n" << imm_vt_map_to_string(st_imm_vt_map, as1, sizeof as1) << endl);
//  for (size_t i = 0; i < len; i++) {
//    //DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": i = " << i << ", st_imm_vt_map =\n" << imm_vt_map_to_string(st_imm_vt_map, as1, sizeof as1) << endl);
//    if (!etfg_insn_matches_template(&_template[i], &iseq[i], src_iseq_match_renamings, prefix, true)) {
//      DYN_DEBUG(insn_match, cout << __func__ << " " << __LINE__ << ": i = " << i << ": returning false\n");
//      return false;
//    }
//  }
//  DYN_DEBUG2(insn_match, cout << __func__ << " " << __LINE__ << ": returning true\n");
//  //cout << __func__ << " " << __LINE__ << ": returning true for " << _template->m_comment << " vs. " << iseq->m_comment <</* ": st_regmap =\n" << regmap_to_string(st_regmap, as1, sizeof as1) << */endl;
//  //cout << __func__ << " " << __LINE__ << ": nextpc_map =\n" << nextpc_map_to_string(nextpc_map, as1, sizeof as1) << endl;
//  return true;
//}

void
etfg_insn_copy(struct etfg_insn_t *dst, struct etfg_insn_t const *src)
{
  dst->etfg_insn_copy_from(*src);
  //dst->etfg_insn_set_tfg((tfg const *)src->etfg_insn_get_tfg());
  //dst->m_preconditions = src->m_preconditions;
  //dst->m_edges = src->m_edges;
  //dst->m_comment = src->m_comment;
}

unsigned long
etfg_insn_hash_func(etfg_insn_t const *insn)
{
  //printf("%s() is not implemented; using dummy for now\n", __func__);
  return 0;//NOT_IMPLEMENTED();
}

bool
etfg_insn_merits_optimization(etfg_insn_t const *insn)
{
  NOT_IMPLEMENTED();
}

bool
etfg_insn_is_nop(etfg_insn_t const *insn)
{
  ASSERT(insn->m_edges.size() > 0);
  if (insn->m_edges.size() > 1) {
    return false;
  }
  //src_ulong edgenum = insn->m_edgenums.at(0);
  //shared_ptr<tfg_edge> e = tfg_get_edge_from_edgenum(insn->m_tfg, edgenum);
  etfg_insn_edge_t ie = insn->m_edges.at(0);
  if (ie.get_pcrels().size() > 1) {
    NOT_REACHED();
    return false;
  }
  shared_ptr<tfg_edge const> const &te = ie.get_tfg_edge();
  state const &to_state = te->get_to_state();
  if (ie.etfg_insn_edge_is_indirect_branch()) {
    return false;
  }
  if (to_state.get_value_expr_map_ref().size() != 0) {
    return false;
  }
  return ie.get_pcrels().size() == 0;
}

void
etfg_insn_canonicalize_locals_symbols(struct etfg_insn_t *etfg_insn,
    struct imm_vt_map_t *imm_vt_map, src_tag_t tag)
{
  NOT_IMPLEMENTED();
}

void
etfg_insn_fetch_preprocess(etfg_insn_t *insn, struct input_exec_t const *in,
    src_ulong pc, unsigned long size)
{
}

i386_as_mode_t etfg_get_i386_as_mode()
{
  return (DST_LONG_BITS == DWORD_LEN) ? I386_AS_MODE_32 : (DST_LONG_BITS == QWORD_LEN) ? I386_AS_MODE_64 : ({ NOT_REACHED(); I386_AS_MODE_32; });
}

long
etfg_insn_put_jump_to_pcrel(nextpc_t const& target, etfg_insn_t *insns,
    size_t insns_size)
{
  ASSERT(target.get_tag() == tag_const);
  char const *format =
"=etfg_insn jump_to_pcrel {{{\n"
"=tfg_edge: L0%%1%%0 => E0%%0%%1\n"
"=tfg_edge.EdgeCond:\n"
"1 : 1 : BOOL\n"
"=tfg_edge.StateTo:\n"
"=state_end\n"
"=tfg_edge.Assumes.begin:\n"
"=tfg_edge.Assumes.end\n"
"=tfg_edge.te_comment\n"
"0:-1:jump_to_pcrel\n"
"tfg_edge_comment end\n"
"=pcrel_val\n"
"%d\n"
"=pcrel_tag\n"
"tag_const\n"
"=comment\n"
"branch:1\n"
"}}}\n";
  size_t str_size = strlen(format) + 256;
  char str[str_size];
  snprintf(str, str_size, format, target.get_val());

  long ret = src_iseq_from_string(NULL, insns, insns_size, str, etfg_get_i386_as_mode());
  ASSERT(ret <= insns_size);
  ASSERT(ret == 1);
  return ret;
}

static unsigned
tfg_get_first_insn_num_from_pc(const void *tfg_in, pc const &p)
{
  tfg const *t = (tfg const *)tfg_in;
  list<dshared_ptr<tfg_edge const>> const &es = t->get_edges();
  int insn_num = 0;
  pc prev_from_pc;
  for (auto iter = es.begin(); iter != es.end(); iter++) {
    dshared_ptr<tfg_edge const> const &e = *iter;
    if (e->get_from_pc() == p) {
      return insn_num;
    }
    if (e->get_from_pc() != prev_from_pc) {
      insn_num++;
    }
    prev_from_pc = e->get_from_pc();
  }
  NOT_REACHED();
}

bool
etfg_insn_is_function_call(etfg_insn_t const *insn)
{
  ASSERT(insn->m_edges.size() > 0);
  if (insn->m_edges.size() > 1) {
    return false;
  }
  //src_ulong edgenum = insn->m_edgenums.at(0);
  //shared_ptr<tfg_edge> e = tfg_get_edge_from_edgenum(insn->m_tfg, edgenum);
  shared_ptr<tfg_edge const> e = insn->m_edges.at(0).get_tfg_edge();
  expr_ref nextpc = e->get_function_call_nextpc();
  return nextpc != nullptr;
}

std::vector<std::string>
etfg_insn_get_fcall_argnames(etfg_insn_t const *insn)
{
  ASSERT(etfg_insn_is_function_call(insn));
  ASSERT(insn->m_edges.size() == 1);
  //src_ulong edgenum = insn->m_edgenums.at(0);
  //shared_ptr<tfg_edge> e = tfg_get_edge_from_edgenum(insn->m_tfg, edgenum);
  shared_ptr<tfg_edge const> e = insn->m_edges.at(0).get_tfg_edge();
  state const &to_state = e->get_to_state();
  vector<string> ret;
  bool found_fcall = false;
  std::function<void (const string&, expr_ref)> f = [&ret, &found_fcall](const string& key, expr_ref e) -> void
  {
    if (!found_fcall) {
      expr_find_op find_op(e, expr::OP_FUNCTION_CALL);
      expr_vector found = find_op.get_matched_expr();
      if (found.size() > 0) {
        expr_ref f = found.at(0);
	ASSERT(f->get_operation_kind() == expr::OP_FUNCTION_CALL);
	for (size_t i = OP_FUNCTION_CALL_ARGNUM_FARG0; i < f->get_args().size(); i++) {
          expr_ref arg = f->get_args().at(i);
	  ASSERT(arg->is_var());
	  ret.push_back(expr_string(arg));
	}
	found_fcall = true;
      }
    }
  };
  to_state.state_visit_exprs(f);
  ASSERT(found_fcall);
  return ret;
}

vector<nextpc_t>
etfg_insn_branch_targets(etfg_insn_t const *insn, src_ulong nip)
{
  vector<nextpc_t> ret;
  ASSERT(insn->m_edges.size() > 0);
  //tfg const *t = (tfg const *)insn->etfg_insn_get_tfg();
  for (auto ie : insn->m_edges) {
    vector<valtag_t> const &pcrels = ie.get_pcrels();
    if (pcrels.size()) {
      valtag_t const &vt = ie.etfg_insn_edge_get_last_pcrel();
      if (vt.tag == tag_const) {
        //cout << __func__ << " " << __LINE__ << ": inserting " << hex << vt.val << " + " << nip << " = " << vt.val + nip << dec << endl;
        ret.push_back(vt.val/* + nip*/); //see COMMENT1
      } else if (vt.tag == tag_var) {
        ret.push_back(PC_UNDEFINED);
      } else NOT_REACHED();
    }
  }
  if (!etfg_insn_is_unconditional_branch_not_fcall(insn)) {
    ret.push_back(nip);
  }
  return ret;
/*
  set<src_ulong> ret;
  shared_ptr<tfg_edge> e = tfg_get_edge_from_edgenum(insn->m_tfg, insn->m_edgenum);
  pc from_pc = e->get_from_pc();
  pc to_pc = e->get_to_pc();

  tfg const *t = (tfg const *)insn->m_tfg;

  //iterate over all edges that start at from_pc
  unsigned from_edgenum = tfg_get_first_edge_from_pc(t, from_pc);
  shared_ptr<tfg_edge> e_sibling = tfg_get_edge_from_edgenum(t, from_edgenum);
  while (e_sibling->get_from_pc() == from_pc) {
    //find the first edge that starts at to_pc
    //cout << __func__ << " " << __LINE__ << ": e_sibling = " << e_sibling->to_string() << endl;
    unsigned to_edgenum = tfg_get_first_edge_from_pc(t, e_sibling->get_to_pc());
    src_ulong target = (nip - (nip % ETFG_MAX_INSNS_PER_FUNCTION)) + to_edgenum;
    ret.insert(target);
    from_edgenum++;
    e_sibling = tfg_get_edge_from_edgenum(t, from_edgenum);
  }
  return ret;
*/
}

nextpc_t
etfg_insn_fcall_target(etfg_insn_t const *insn, src_ulong nip, struct input_exec_t const *in)
{
  ASSERT(etfg_insn_is_function_call(insn));
  ASSERT(insn->m_edges.size() == 1);
  //src_ulong edgenum = insn->m_edgenums.at(0);
  //shared_ptr<tfg_edge> e = tfg_get_edge_from_edgenum(insn->m_tfg, edgenum);
  etfg_insn_edge_t const &ie = insn->m_edges.at(0);
  shared_ptr<tfg_edge const> const &e = ie.get_tfg_edge();
  //tfg const *t = (tfg const *)insn->etfg_insn_get_tfg();
  expr_ref nextpc = e->get_function_call_nextpc();
  if (nextpc && nextpc->is_nextpc_const()) {
    valtag_t const &vt = ie.etfg_insn_edge_get_first_pcrel();
    if (vt.tag == tag_const) {
      return vt.val;
    } else {
      return (src_ulong)-1;
    }
    //nextpc_id_t nextpc_id = nextpc->get_nextpc_num();
    //string const &nextpc_name = t->get_nextpc_map().at(nextpc_id);
    //return input_exec_get_pc_for_function_name(in, nextpc_name);
  } else {
    return (src_ulong)-1;
  }
}

static bool
etfg_insn_edge_is_fallthrough(tfg const *t, etfg_insn_edge_t const &ie)
{
  NOT_IMPLEMENTED();
  /*if (e->get_to_pc().is_exit()) {
    return false;
  }
  unsigned from_insn_num = tfg_get_first_insn_num_from_pc(t, e->get_from_pc());
  unsigned to_insn_num = tfg_get_first_insn_num_from_pc(t, e->get_to_pc());
  if (to_insn_num != from_insn_num + 1) {
    return true;
  } else {
    return false;
  }*/
}

static valtag_t
convert_nextpc_const_expr_to_valtag(expr_ref const &nextpc_const)
{
  ASSERT(nextpc_const->is_nextpc_const());
  nextpc_id_t nextpc_id = nextpc_const->get_nextpc_num();
  valtag_t ret;
  ret.tag = tag_var;
  ret.val = nextpc_id;
  return ret;
}

/*void
etfg_insn_edge_t::etfg_insn_edge_identify_fcall_nextpcs_as_pcrels(vector<valtag_t> &pcrels) const
{
  shared_ptr<tfg_edge> e = this->get_tfg_edge();
  state const &to_state = e->get_to_state();
  expr_ref found_nextpc = nullptr;
  std::function<void (string const &, expr_ref)> f = [&found_nextpc](string const &key, expr_ref e) -> void
  {
    if (found_nextpc) {
      return;
    }
    context *ctx = e->get_context();
    consts_struct_t const &cs = ctx->get_consts_struct();
    set<nextpc_id_t> nextpc_ids = ctx->expr_contains_nextpc_const(e);
    ASSERT(nextpc_ids.size() <= 1);
    if (nextpc_ids.size()) {
      found_nextpc = cs.get_expr_value(reg_type_nextpc_const, *nextpc_ids.begin());
    }
  };
  to_state.state_visit_exprs(f, to_state);
  if (found_nextpc) {
    valtag_t vt = convert_nextpc_const_expr_to_valtag(found_nextpc);
    pcrels.push_back(vt);
  }
}

bool
etfg_insn_edge_t::etfg_insn_edge_set_fcall_nextpcs_as_pcrels(context *ctx, valtag_t const &new_pcrel)
{
  if (new_pcrel.tag == tag_const) {
    return false;
  }
  if (new_pcrel.tag != tag_var) {
    //cout << __func__ << " " << __LINE__ << ": new_pcrel.tag = " << new_pcrel.tag << endl;
    return false;
  }
  ASSERT(new_pcrel.tag == tag_var);
  consts_struct_t const &cs = ctx->get_consts_struct();
  expr_ref new_nextpc_const = cs.get_expr_value(reg_type_nextpc_const, new_pcrel.val);
  shared_ptr<tfg_edge> e = this->get_tfg_edge();
  state &to_state = e->get_to_state();
  bool found_nextpc_const = false;
  std::function<expr_ref (string const &, expr_ref)> f = [ctx, &new_nextpc_const, &found_nextpc_const](string const &key, expr_ref e) -> expr_ref
  {
    expr_ref ret = ctx->expr_replace_nextpc_consts_with_new_nextpc_const(e, new_nextpc_const, found_nextpc_const);
    return ret;
  };
  to_state.state_visit_exprs(f, to_state);
  return found_nextpc_const;
}*/

vector<valtag_t>
etfg_insn_get_pcrel_values(struct etfg_insn_t const *insn)
{
  ASSERT(insn->m_edges.size() > 0);
  //tfg const *t = (tfg const *)insn->m_tfg;
  vector<valtag_t> ret;
  for (const auto &e : insn->m_edges) {
    //e.etfg_insn_edge_identify_fcall_nextpcs_as_pcrels(ret);
    for (auto pcrel : e.get_pcrels()) {
      ret.push_back(pcrel);
    }
  }
  return ret;
}

bool etfg_insn_is_function_return(etfg_insn_t const *insn)
{
  //tfg const *t = (tfg const *)insn->m_tfg;
  //context *ctx = t->get_context();
  context *ctx = insn->get_context();
  ASSERT(ctx); //ctx=NULL means that insn has not been filled
  consts_struct_t const &cs = ctx->get_consts_struct();
  if (insn->m_edges.size() != 1) {
    //cout << __func__ << " " << __LINE__ << ":\n";
    return false;
  }
  etfg_insn_edge_t const &ie = insn->m_edges.at(0);
  if (ie.get_pcrels().size() != 0) {
    //cout << __func__ << " " << __LINE__ << ":\n";
    return false;
  }
  shared_ptr<tfg_edge const> const &te = ie.get_tfg_edge();
  state const &to_state = te->get_to_state();
  string k;
  if ((k = to_state.has_expr_suffix(LLVM_STATE_INDIR_TARGET_KEY_ID)) == "") {
    //cout << __func__ << " " << __LINE__ << ":\n";
    return false;
  }
  expr_ref indir_addr = to_state.get_expr(k/*, to_state*/);
  bool ret = cs.expr_is_retaddr_const(indir_addr);
  //cout << __func__ << " " << __LINE__ << ":ret = " << ret << "\n";
  return ret;
}

bool etfg_insn_is_indirect_branch(etfg_insn_t const *insn)
{
  return    etfg_insn_is_indirect_branch_not_fcall(insn)
         || etfg_insn_is_indirect_function_call(insn);
}

bool etfg_insn_is_indirect_branch_not_fcall(etfg_insn_t const *insn)
{
  ASSERT(insn->m_edges.size() > 0);
  //src_ulong edgenum = insn->m_edgenums.at(0);
  //shared_ptr<tfg_edge> e = tfg_get_edge_from_edgenum(insn->m_tfg, edgenum);
  return insn->m_edges.at(0).etfg_insn_edge_is_indirect_branch_not_fcall();
}

bool etfg_insn_is_conditional_indirect_branch(etfg_insn_t const *insn)
{
  ASSERT(insn->m_edges.size() > 0);
  //src_ulong edgenum = insn->m_edgenums.at(0);
  //shared_ptr<tfg_edge> e = tfg_get_edge_from_edgenum(insn->m_tfg, edgenum);
  shared_ptr<tfg_edge const> e = insn->m_edges.at(0).get_tfg_edge();
  return    etfg_insn_is_indirect_branch_not_fcall(insn)
         && !e->get_cond()->is_true();
}

bool etfg_insn_is_direct_function_call(etfg_insn_t const *insn)
{
  ASSERT(insn->m_edges.size() > 0);
  //src_ulong edgenum = insn->m_edgenums.at(0);
  //shared_ptr<tfg_edge> e = tfg_get_edge_from_edgenum(insn->m_tfg, edgenum);
  shared_ptr<tfg_edge const> e = insn->m_edges.at(0).get_tfg_edge();
  expr_ref nextpc = e->get_function_call_nextpc();
  if (nextpc && nextpc->is_nextpc_const()) {
    return true;
  } else {
    return false;
  }
}

bool etfg_insn_is_indirect_function_call(etfg_insn_t const *insn)
{
  ASSERT(insn->m_edges.size() > 0);
  //src_ulong edgenum = insn->m_edgenums.at(0);
  //shared_ptr<tfg_edge> e = tfg_get_edge_from_edgenum(insn->m_tfg, edgenum);
  return insn->m_edges.at(0).etfg_insn_edge_is_indirect_function_call();
}

src_ulong etfg_insn_branch_target_var(etfg_insn_t const *insn)
{
  NOT_IMPLEMENTED();
}

bool etfg_insn_is_branch(struct etfg_insn_t const *insn)
{
  NOT_IMPLEMENTED();
}

bool etfg_insn_is_branch_not_fcall(etfg_insn_t const *insn)
{
  NOT_IMPLEMENTED();
}

bool etfg_insn_is_unconditional_indirect_branch_not_fcall(etfg_insn_t const *insn)
{
  NOT_IMPLEMENTED();
}

bool etfg_insn_is_direct_branch(etfg_insn_t const *insn)
{
  NOT_IMPLEMENTED();
}

bool
etfg_insn_is_conditional_direct_branch(etfg_insn_t const *insn)
{
  ASSERT(insn->m_edges.size() > 0);
  //src_ulong edgenum = insn->m_edgenums.at(0);
  //shared_ptr<tfg_edge> e = tfg_get_edge_from_edgenum(insn->m_tfg, edgenum);
  shared_ptr<tfg_edge const> e = insn->m_edges.at(0).get_tfg_edge();
  pc from_pc = e->get_from_pc();
  pc to_pc = e->get_to_pc();
  if (etfg_insn_is_indirect_branch_not_fcall(insn)) {
    return false;
  }
  ASSERT(to_pc.is_label());
  bool ret = (insn->m_edges.size() > 1);
  //cout << __func__ << " " << __LINE__ << ": returning " << ret << " for " << etfg_insn_to_string(insn, as1, sizeof as1) << endl;
  return ret;
  //tfg const *t = (tfg const *)insn->etfg_insn_get_tfg();
  //list<shared_ptr<tfg_edge>> outgoing;
  //t->get_edges_outgoing(from_pc, outgoing);
  //return outgoing.size() > 1;
}

bool etfg_insn_is_unconditional_direct_branch(etfg_insn_t const *insn)
{
  NOT_IMPLEMENTED();
}

bool
etfg_insn_is_unconditional_branch_not_fcall(etfg_insn_t const *insn)
{
  ASSERT(insn->m_edges.size() > 0);
  if (insn->m_edges.size() > 1) {
    return false;
  }
  //src_ulong edgenum = insn->m_edgenums.at(0);
  //shared_ptr<tfg_edge> e = tfg_get_edge_from_edgenum(insn->m_tfg, edgenum);
  etfg_insn_edge_t ie = insn->m_edges.at(0);
  if (ie.get_pcrels().size() > 1) {
    NOT_REACHED();
    return false;
  }
  shared_ptr<tfg_edge const> const &te = ie.get_tfg_edge();
  state const &to_state = te->get_to_state();
  if (ie.get_tfg_edge()->edge_is_indirect_branch()) {
    return true;
  }
  if (ie.get_pcrels().size() == 0) {
    return false;
  }
  ASSERT(ie.get_pcrels().size() == 1);
  if (etfg_insn_is_function_call(insn)) {
    return false;
  }
  if (to_state.get_value_expr_map_ref().size()) {
    cout << __func__ << " " << __LINE__ << ": insn =\n" << etfg_insn_to_string(insn, as1, sizeof as1) << endl;
  }
  ASSERT(to_state.get_value_expr_map_ref().size() == 0);
  if (   ie.get_pcrels().at(0).val == 0
      && ie.get_pcrels().at(0).tag == tag_const) {
    return false; //it is simply a fallthrough; not an unconditional branch
  }
  return true;
}

bool
etfg_insn_is_unconditional_direct_branch_not_fcall(etfg_insn_t const *insn)
{
  ASSERT(insn->m_edges.size() > 0);
  //shared_ptr<tfg_edge> e = tfg_get_edge_from_edgenum(insn->m_tfg, insn->m_edgenums.at(0));
  shared_ptr<tfg_edge const> e = insn->m_edges.at(0).get_tfg_edge();
  return    etfg_insn_is_unconditional_branch_not_fcall(insn)
         && !etfg_insn_is_indirect_branch_not_fcall(insn);
}

bool etfg_insn_is_unconditional_branch(etfg_insn_t const *insn)
{
  return etfg_insn_is_unconditional_branch_not_fcall(insn);
}

void
etfg_insn_edge_t::set_pcrels(vector<valtag_t> const &pcrels)
{
  m_pcrels.clear();
  for (auto const& pcrel : pcrels) {
    if (pcrel.val == 0 && pcrel.tag == tag_const) {
      continue; //this is just a fallthrough; ignore it
    }
    m_pcrels.push_back(pcrel);
  }
}

bool
etfg_insn_edge_t::etfg_insn_edge_is_indirect_function_call() const
{
  shared_ptr<tfg_edge const> e = this->get_tfg_edge();
  expr_ref nextpc = e->get_function_call_nextpc();
  if (nextpc && !nextpc->is_nextpc_const()) {
    return true;
  } else {
    return false;
  }
}

bool
etfg_insn_edge_t::etfg_insn_edge_is_indirect_branch_not_fcall() const
{
  shared_ptr<tfg_edge const> e = this->get_tfg_edge();
  //return e->get_to_pc().is_exit() && e->get_to_pc().get_index() == 0; //XXX: HACK!
  return e->get_to_pc().is_exit() && e->edge_is_indirect_branch();
}

bool
etfg_insn_edge_t::etfg_insn_edge_is_indirect_branch() const
{
  return this->etfg_insn_edge_is_indirect_function_call() || this->etfg_insn_edge_is_indirect_branch_not_fcall();
}

void
etfg_insn_set_pcrel_values(struct etfg_insn_t *insn, vector<valtag_t> const &vs)
{
  /*cout << __func__ << " " << __LINE__ << ": before: insn = " << etfg_insn_to_string(insn, as1, sizeof as1) << endl;
  cout << __func__ << " " << __LINE__ << ": vs = ";
  for (auto vt : vs) {
    cout << "(" << ((vt.tag==tag_const)?"tag_const":"tag_var") << ", " << vt.val << "), ";
  }
  cout << "\n";*/
  //tfg const *t = (tfg const *)insn->m_tfg;
  ASSERT(insn->m_edges.size() > 0);
  size_t pcrel_num = 0;
  for (size_t i = 0; i < insn->m_edges.size(); i++) {
    //ASSERT(pcrel_num >= i);
    /*if (   vs.size() > pcrel_num
        && insn->m_edges[i].etfg_insn_edge_set_fcall_nextpcs_as_pcrels(t->get_context(), vs.at(pcrel_num))) {
      pcrel_num++;
    }*/
    vector<valtag_t> new_pcrels;
    for (size_t p = 0; p < insn->m_edges[i].get_pcrels().size(); p++) {
      new_pcrels.push_back(vs.at(pcrel_num));
      pcrel_num++;
    }
    insn->m_edges[i].set_pcrels(new_pcrels);
  }
  //cout << __func__ << " " << __LINE__ << ": after: insn = " << etfg_insn_to_string(insn, as1, sizeof as1) << endl;
}

long etfg_insn_put_jump_to_nextpc(etfg_insn_t *insns, size_t insns_size, long nextpc_num)
{
  char const *format =
"=etfg_insn jump_to_nextpc {{{\n"
"=tfg_edge: L0%%1%%0 => E0%%0%%1\n"
"=tfg_edge.EdgeCond:\n"
"1 : 1 : BOOL\n"
"=tfg_edge.StateTo:\n"
"=state_end\n"
"=tfg_edge.Assumes.begin:\n"
"=tfg_edge.Assumes.end\n"
"=tfg_edge.te_comment\n"
"0:-1:jump_to_nextpc\n"
"tfg_edge_comment end\n"
"=pcrel_val\n"
"%d\n"
"=pcrel_tag\n"
"tag_var\n"
"=comment\n"
"branch:1\n"
"}}}\n";
  size_t str_size = strlen(format) + 256;
  char str[str_size];
  snprintf(str, str_size, format, nextpc_num);

  long ret = src_iseq_from_string(NULL, insns, insns_size, str, etfg_get_i386_as_mode());
  ASSERT(ret <= insns_size);
  ASSERT(ret == 1);
  return ret;
}

class expr_appears_as_memory_operand_in_expr_visitor : public expr_visitor {
public:
  expr_appears_as_memory_operand_in_expr_visitor(expr_ref const &haystack, expr_ref const &needle) : m_ctx(haystack->get_context()), m_haystack(haystack), m_needle(needle), m_result(false)
  {
    visit_recursive(m_haystack);
  }

  bool get_result() const
  {
    return m_result;
  }

private:
  virtual void visit(expr_ref const &);
  struct context *m_ctx;
  expr_ref const &m_haystack;
  expr_ref const &m_needle;
  bool m_result;
  map<expr_id_t, bool> m_is_affine_expression_of_needle;
};

void
expr_appears_as_memory_operand_in_expr_visitor::visit(expr_ref const &e)
{
  if (e == m_needle) {
    m_is_affine_expression_of_needle[e->get_id()] = true;
    return;
  }
  if (e->is_const() || e->is_var()) {
    m_is_affine_expression_of_needle[e->get_id()] = false;
    return;
  }
  if (   e->get_operation_kind() == expr::OP_SELECT
      || e->get_operation_kind() == expr::OP_STORE
      || e->get_operation_kind() == expr::OP_STORE_UNINIT) {
    expr_ref const &addr = e->get_args().at(OP_SELECT_ARGNUM_ADDR);
    if (m_is_affine_expression_of_needle.at(addr->get_id())) {
      m_result = true;
    }
  }
  if (   e->get_operation_kind() == expr::OP_BVADD
      || e->get_operation_kind() == expr::OP_BVSUB) {
    for (const auto &arg : e->get_args()) {
      if (m_is_affine_expression_of_needle.at(arg->get_id())) {
        m_is_affine_expression_of_needle[e->get_id()] = true;
        return;
      }
    }
    m_is_affine_expression_of_needle[e->get_id()] = false;
    return;
  }
  m_is_affine_expression_of_needle[e->get_id()] = false;
}

static bool
expr_appears_as_memory_operand_in_expr(expr_ref const &haystack, expr_ref const &needle)
{
  expr_appears_as_memory_operand_in_expr_visitor visitor(haystack, needle);
  return visitor.get_result();
}

static bool
predicate_expr_appears_as_memory_operand(predicate_ref const &p, expr_ref const &needle)
{
  bool ret = false;
  std::function<void (const string &, expr_ref const &)> f = [&ret, &needle](string const &s, expr_ref const &e)
  {
    if (expr_appears_as_memory_operand_in_expr(e, needle)) {
      ret = true;
    }
  };
  p->visit_exprs(f);
  return ret;
}

static bool
state_expr_appears_as_memory_operand_in_expr(state const &st, expr_ref const &needle)
{
  bool ret = false;
  std::function<void (const string &, expr_ref const &)> f = [&ret, &needle](string const &s, expr_ref const &e)
  {
    if (expr_appears_as_memory_operand_in_expr(e, needle)) {
      ret = true;
    }
  };
  st.state_visit_exprs(f);
  return ret;
}

static bool
tfg_edge_expr_appears_as_memory_operand(shared_ptr<tfg_edge const> const &te, expr_ref const &e)
{
  if (expr_appears_as_memory_operand_in_expr(te->get_cond(), e)) {
    return true;
  }
  if (state_expr_appears_as_memory_operand_in_expr(te->get_to_state(), e)) {
    return true;
  }
  return false;
}

static bool
etfg_insn_expr_appears_as_memory_operand(etfg_insn_t const *insn, expr_ref const &e)
{
  for (const auto &pred : insn->m_preconditions) {
    if (predicate_expr_appears_as_memory_operand(pred, e)) {
      return true;
    }
  }
  for (const auto &ed : insn->m_edges) {
    if (tfg_edge_expr_appears_as_memory_operand(ed.get_tfg_edge(), e)) {
      return true;
    }
  }
  return false;
}

static void
etfg_insn_mark_used_vconstants(imm_vt_map_t *map, imm_vt_map_t *nomem_map,
    imm_vt_map_t *mem_map,
    etfg_insn_t const *insn)
{
  //operand_t const *op;
  //valtag_t const *vt;
  //int i;
  long num_imm_operands = etfg_insn_get_max_num_imm_operands(insn);
  for (long i = 0; i < num_imm_operands - 1; i++) { //ignore one "imm" operand  as that stands for regs
    expr_ref op_expr = etfg_insn_get_expr_at_operand_index(insn, i);
    if (op_expr->is_const()) {
      continue;
    }
    vconstants_t vconstant;
    bool is_canon = expr_is_canonical_constant(op_expr, &vconstant);
    ASSERT(is_canon);
    valtag_t vt = vconstant_to_valtag(vconstant);
    if (etfg_insn_expr_appears_as_memory_operand(insn, op_expr)) {
      imm_vt_map_mark_used_vconstant(mem_map, &vt);
    } else {
      imm_vt_map_mark_used_vconstant(nomem_map, &vt);
    }
    imm_vt_map_mark_used_vconstant(map, &vt);
  }
  /*for (i = 0; i < I386_MAX_NUM_OPERANDS; i++) {
    op = &insn->op[i];
    vt = NULL;
    if (   op->type == op_imm
        && op->valtag.imm.tag == tag_var) {
      vt = &op->valtag.imm;
      imm_vt_map_mark_used_vconstant(nomem_map, vt);
    } else if (   op->type == op_mem
               && op->valtag.mem.disp.tag == tag_var) {
      vt = &op->valtag.mem.disp;
      imm_vt_map_mark_used_vconstant(mem_map, vt);
    }
    if (vt) {
      imm_vt_map_mark_used_vconstant(map, vt);
    }
  }*/
}


void
etfg_iseq_mark_used_vconstants(struct imm_vt_map_t *map, struct imm_vt_map_t *nomem_map,
    struct imm_vt_map_t *mem_map, etfg_insn_t const *iseq, long iseq_len)
{
  long i;

  imm_vt_map_init(map);
  imm_vt_map_init(mem_map);
  imm_vt_map_init(nomem_map);
  for (i = 0; i < iseq_len; i++) {
    etfg_insn_mark_used_vconstants(map, nomem_map, mem_map, &iseq[i]);
  }
}

void
etfg_insn_get_min_max_vconst_masks(int *min_mask, int *max_mask,
    etfg_insn_t const *insn)
{
  long num_imm_operands = etfg_insn_get_max_num_imm_operands(insn);
  for (long i = 0; i < num_imm_operands - 1; i++) { //ignore one "imm" operand as that stands for regs
    expr_ref op_expr = etfg_insn_get_expr_at_operand_index(insn, i);
    if (op_expr->is_const()) {
      continue;
    }
    vconstants_t vconstant;
    bool is_canon = expr_is_canonical_constant(op_expr, &vconstant);
    ASSERT(is_canon);
    valtag_t vt = vconstant_to_valtag(vconstant);
    if (!etfg_insn_expr_appears_as_memory_operand(insn, op_expr)) {
      imm_valtag_get_min_max_vconst_masks(min_mask, max_mask, &vt);
    }
  }
}

void
etfg_insn_get_min_max_mem_vconst_masks(int *min_mask, int *max_mask,
    etfg_insn_t const *insn)
{
  long num_imm_operands = etfg_insn_get_max_num_imm_operands(insn);
  for (long i = 0; i < num_imm_operands - 1; i++) { //ignore one "imm" operand as that stands for regs
    expr_ref op_expr = etfg_insn_get_expr_at_operand_index(insn, i);
    if (op_expr->is_const()) {
      continue;
    }
    vconstants_t vconstant;
    bool is_canon = expr_is_canonical_constant(op_expr, &vconstant);
    ASSERT(is_canon);
    valtag_t vt = vconstant_to_valtag(vconstant);
    if (etfg_insn_expr_appears_as_memory_operand(insn, op_expr)) {
      imm_valtag_get_min_max_vconst_masks(min_mask, max_mask, &vt);
    }
  }
}

static long
etfg_iseq_simplify_for_template_matching(etfg_insn_t *seq, long len)
{
  //XXX: NOT_IMPLEMENTED()
  //1. eliminate any move instructions where the src of the move is not live after the instruction (nsieve:28 in utils/etfg-i386.other.ptab would need to get updated)
  return len;
}

vector<src_iseq_match_renamings_t>
etfg_iseq_matches_template(etfg_insn_t const *_template,
    etfg_insn_t const *input_seq, long len,
    //src_iseq_match_renamings_t &src_iseq_match_renamings,
    //struct regmap_t *st_regmap,
    //struct imm_vt_map_t *st_imm_map, struct nextpc_map_t *nextpc_map,
    string prefix
    )
{
  autostop_timer func_timer(__func__);
  vector<src_iseq_match_renamings_t> renamings;
  src_iseq_match_renamings_t src_iseq_match_renamings;

  string insn_match_prefix = "insn_match.";
  insn_match_prefix += prefix;

  etfg_insn_t *seq = new etfg_insn_t[len];
  ASSERT(seq);
  etfg_iseq_copy(seq, input_seq, len);
  len = etfg_iseq_simplify_for_template_matching(seq, len);
  DYN_DEBUGS2(insn_match_prefix.c_str(), cout << __func__ << " " << __LINE__ << ":\n");
  //ASSERT(nextpc_map);
  src_iseq_match_renamings.clear();
  renamings.push_back(src_iseq_match_renamings);
  //regmap_init(st_regmap);
  //imm_vt_map_init(st_imm_map);
  //nextpc_map_init(nextpc_map);
  long i;
  for (i = 0; i < len; i++) {
    DYN_DEBUGS2(insn_match_prefix.c_str(), cout << __func__ << " " << __LINE__ << ": checking instruction " << i << "\n");
    if (!etfg_insn_matches_template(&_template[i], &seq[i], renamings, insn_match_prefix, false)) {
      ASSERT(renamings.size() == 0);
      DYN_DEBUGS2(insn_match_prefix.c_str(), cout << __func__ << " " << __LINE__ << ": returning false\n");
      delete [] seq;
      return renamings;
    }
  }
  //cout << __func__ << " " << __LINE__ << ": returning true for " << _template->m_comment << " vs. " << seq->m_comment <</* ": st_regmap =\n" << regmap_to_string(st_regmap, as1, sizeof as1) << */endl;
  //cout << __func__ << " " << __LINE__ << ": nextpc_map =\n" << nextpc_map_to_string(nextpc_map, as1, sizeof as1) << endl;
  DYN_DEBUGS2(insn_match_prefix.c_str(), cout << __func__ << " " << __LINE__ << ": returning true\n");
  delete [] seq;
  return renamings;
}

/*long etfg_iseq_window_convert_pcrels_to_nextpcs(etfg_insn_t *etfg_iseq,
    long etfg_iseq_len, long start, long *nextpc2src_pcrel_vals,
    src_tag_t *nextpc2src_pcrel_tags)
{
  NOT_IMPLEMENTED();
}*/

bool etfg_iseq_execution_test_implemented(etfg_insn_t const *etfg_iseq,
    long etfg_iseq_len)
{
  NOT_IMPLEMENTED();
}

static bool
etfg_insn_is_alloca(etfg_insn_t const  *insn)
{
  if (insn->m_edges.size() != 1) {
    return false;
  }
  etfg_insn_edge_t const &ie = insn->m_edges.at(0);
  shared_ptr<tfg_edge const> const &e = ie.get_tfg_edge();
  state const &to_state = e->get_to_state();
  bool ret = false;
  std::function<void (string const &, expr_ref)> f = [&ret](string const &key, expr_ref e)
  {
    if (expr_num_op_occurrences(e, expr::OP_ALLOCA) > 0) {
      ret = true;
    }
  };
  to_state.state_visit_exprs(f);
  return ret;
}

bool etfg_insn_supports_execution_test(etfg_insn_t const *insn)
{
  return    !etfg_insn_is_function_call(insn)
         && !etfg_insn_is_function_return(insn)
         && !etfg_insn_is_alloca(insn);
}

bool etfg_iseq_supports_execution_test(etfg_insn_t const *insns, long n)
{
  return true;
}

bool etfg_insn_supports_boolean_test(etfg_insn_t const *insn)
{
  return true;
}

bool
etfg_insn_less(etfg_insn_t const *a, etfg_insn_t const *b)
{
  string a_str = etfg_insn_to_string(a, as1, sizeof as1);
  string b_str = etfg_insn_to_string(b, as1, sizeof as1);
  return a_str < b_str;
}

void etfg_insn_hash_canonicalize(struct etfg_insn_t *insn, struct regmap_t *regmap,
    struct imm_vt_map_t *imm_map)
{
}

int etfg_insn_disassemble(etfg_insn_t *insn, uint8_t const *bincode, i386_as_mode_t i386_as_mode)
{
  NOT_IMPLEMENTED();
}

class expr_strict_canonicalize_regs_visitor : public expr_visitor {
public:
  expr_strict_canonicalize_regs_visitor(context *ctx, expr_ref const &e, long num_exregs_used[ETFG_NUM_EXREG_GROUPS], regmap_t *st_regmap, fixed_reg_mapping_t const &fixed_reg_mapping) : m_ctx(ctx), m_in(e), m_num_exregs_used(num_exregs_used), m_st_regmap(st_regmap), m_fixed_reg_mapping(fixed_reg_mapping), m_cs(m_ctx->get_consts_struct())
  {
    visit_recursive(m_in);
  }

  expr_ref get_result() const
  {
    ASSERT(m_map.count(m_in->get_id()) > 0);
    expr_ref ret = m_map.at(m_in->get_id());
    return ret;
  }

private:
  virtual void visit(expr_ref const &);
  struct context *m_ctx;
  expr_ref const &m_in;
  long *m_num_exregs_used;
  regmap_t *m_st_regmap;
  fixed_reg_mapping_t const &m_fixed_reg_mapping;
  //map<nextpc_id_t, nextpc_id_t> &m_fcall_nextpc_map;
  //nextpc_id_t &m_cur_fcall_nextpc_id;
  //long m_num_nextpcs;
  consts_struct_t const &m_cs;
  map<expr_id_t, expr_ref> m_map;
};

void
expr_strict_canonicalize_regs_visitor::visit(expr_ref const &e)
{
  if (e->is_const()) {
    m_map[e->get_id()] = e;
    return;
  }
  if (e->is_var()) {
    if (e->is_array_sort() || e->is_memlabel_sort() || e->is_count_sort() || e->is_function_sort()) {
      m_map[e->get_id()] = e;
      return;
    }
    //cout << __func__ << " " << __LINE__ << ": var = " << expr_string(e) << ": is arg const = " << m_cs.expr_is_argument_constant(e) << endl;
    if (   !expr_is_consts_struct_constant(e, m_cs)
        /*|| m_cs.expr_is_argument_constant(e)*/) {
      ASSERT(e->is_bool_sort() || e->is_bv_sort());
      expr_ref e_bv = e;
      string e_str = expr_string(e_bv);
      if (e_bv->is_bool_sort()) {
        e_bv = m_ctx->mk_var(e_str, m_ctx->mk_bv_sort(1));
      }
      ASSERT(e_bv->get_sort()->get_size() <= ETFG_MAX_REG_SIZE);
      size_t e_size = e_bv->get_sort()->get_size();
      //cout << __func__ << " " << __LINE__ << ": e_str = " << e_str << endl;
      ASSERT(e_str.substr(0, strlen(G_INPUT_KEYWORD ".")) == G_INPUT_KEYWORD ".");
      string e_str_key = e_str.substr(strlen(G_INPUT_KEYWORD "."));
      exreg_group_id_t exreg_group_id = etfg_exreg_group_id_from_expression_name(e_str_key);
      reg_identifier_t r = ST_CANON_EXREG(get_reg_identifier_for_regname(e_str_key), exreg_group_id, m_num_exregs_used, m_st_regmap, m_fixed_reg_mapping);
      expr_ref r_expr = m_ctx->mk_var(string(G_INPUT_KEYWORD) + "." + G_SRC_KEYWORD + "." + etfg_regname(exreg_group_id, r), m_ctx->mk_bv_sort(ETFG_EXREG_LEN(exreg_group_id)));
      if (e_size < ETFG_EXREG_LEN(exreg_group_id)) {
        r_expr = m_ctx->mk_bvextract(r_expr, e_size - 1, 0);
      }
      ASSERT(r_expr->get_sort() == e_bv->get_sort());
      if (e->is_bool_sort()) {
        r_expr = m_ctx->mk_eq(r_expr, m_ctx->mk_bv_const(1, 1));
      }
      //cout << __func__ << " " << __LINE__ << ": r_expr = " << expr_string(r_expr) << endl;
      m_map[e->get_id()] = r_expr;
      return;
    } else {
      m_map[e->get_id()] = e;
      return;
    }
  }
  expr_vector new_args;
  for (auto arg : e->get_args()) {
    new_args.push_back(m_map.at(arg->get_id()));
  }
  m_map[e->get_id()] = m_ctx->mk_app(e->get_operation_kind(), new_args);
}

static expr_ref
expr_strict_canonicalize_regs(context *ctx, expr_ref const &e,
    long num_exregs_used[ETFG_NUM_EXREG_GROUPS], regmap_t *st_regmap,
    fixed_reg_mapping_t const &fixed_reg_mapping)
{
  //cout << __func__ << " " << __LINE__ << ": st_regmap =\n" << regmap_to_string(st_regmap, as1, sizeof as1) << endl;
  expr_strict_canonicalize_regs_visitor visitor(ctx, e, num_exregs_used, st_regmap, fixed_reg_mapping);
  expr_ref ret = visitor.get_result();
  //cout << __func__ << " " << __LINE__ << ": returning " << expr_string(ret) << " for " << expr_string(e) << endl;
  return ret;
}

static bool
is_llvm_substr(string const &haystack, string const &needle)
{
  size_t needle_len = needle.length();
  size_t pos = haystack.find(needle);
  if (pos == string::npos) {
    return false;
  }
  if (pos == 0) {
    return true;
  }
  if (haystack.size() == pos + needle_len) {
    return true;
  }
  if (haystack.at(pos + needle_len) != '.') {
    return false;
  }
  return string_is_numeric(haystack.substr(pos + needle_len + 1));
}

static bool
is_llvm_target_reg(string const &reg)
{
  string prefix = string(G_LLVM_PREFIX "-" ETFG_FBGEN_TARGET_REGNAME);
  return is_llvm_substr(reg, prefix);
}

static bool
is_llvm_ret_reg(string const &reg)
{
  string prefix = string(G_LLVM_RETURN_REGISTER_NAME);
  return is_llvm_substr(reg, prefix);
}

static bool
is_llvm_hidden_reg(string const &reg)
{
  string prefix = string(G_LLVM_HIDDEN_REGISTER_NAME);
  return is_llvm_substr(reg, prefix);
}



static bool
is_llvm_indir_target(string const &reg)
{
  string prefix = string(LLVM_STATE_INDIR_TARGET_KEY_ID);
  return is_llvm_substr(reg, prefix);
}

static bool
is_llvm_gep_reg(string const &key)
{
  return key.find(GEPOFFSET_KEYWORD) != string::npos;
}

static void
state_strict_canonicalize_regs(context *ctx, state &s,
    long num_exregs_used[ETFG_NUM_EXREG_GROUPS], regmap_t *st_regmap, fixed_reg_mapping_t const &fixed_reg_mapping, state const &start_state)
{
  //reg_identifier_t target_reg_id = reg_identifier_t::reg_identifier_invalid();
  //set<string> remove_keys;
  //string target_key;
  map<pair<exreg_group_id_t, exreg_id_t>, string> target_regs;

  std::function<expr_ref (const string&, expr_ref)> f = [ctx, &num_exregs_used, st_regmap, &fixed_reg_mapping, &target_regs/*, &remove_keys*/](const string& key, expr_ref e) -> expr_ref
  {
    if (!e->is_array_sort() && !is_llvm_indir_target(key)) {
      //char as1[4096];
      //cout << __func__ << " " << __LINE__ << ": st_regmap = " << regmap_to_string(st_regmap, as1, sizeof as1) << endl;
      reg_identifier_t rkey = get_reg_identifier_for_regname(key);
      //cout << __func__ << " " << __LINE__ << ": rkey = " << rkey.reg_identifier_to_string() << endl;
      exreg_group_id_t exreg_group_id = etfg_exreg_group_id_from_expression_name(key);
      reg_identifier_t r = ST_CANON_EXREG(rkey, exreg_group_id, num_exregs_used, st_regmap, fixed_reg_mapping);
      //target_key = key;
      //target_reg_id = r;
      ASSERT(r.reg_identifier_is_valid());
      ASSERT(!r.reg_identifier_is_unassigned());
      target_regs[make_pair(exreg_group_id, r.reg_identifier_get_id())] = key;
    }
    return expr_strict_canonicalize_regs(ctx, e, num_exregs_used, st_regmap, fixed_reg_mapping);
  };
  s.state_visit_exprs(f);
  for (auto target_reg : target_regs) {
    expr_ref target_expr = s.get_expr(target_reg.second/*, s*/);
    if (target_expr->is_bool_sort()) {
      //target_expr = ctx->mk_ite(target_expr, ctx->mk_bv_const(1, 1), ctx->mk_bv_const(1, 0));
      target_expr = expr_bool_to_bv1(target_expr);
    }
    exreg_group_id_t exreg_group_id = target_reg.first.first;
    ASSERT(target_expr->get_sort()->get_size() <= ETFG_EXREG_LEN(exreg_group_id));
    string regname = string(G_SRC_KEYWORD ".") + etfg_regname(exreg_group_id, target_reg.first.second);
    if (target_expr->get_sort()->get_size() < ETFG_EXREG_LEN(exreg_group_id)) {
      //target_expr = ctx->mk_bvuninit_ext(target_expr, ETFG_MAX_REG_SIZE - target_expr->get_sort()->get_size());
      expr_ref orig_reg = start_state.get_expr(regname/*, start_state*/);
      target_expr = ctx->mk_bvconcat(ctx->mk_bvextract(orig_reg, ETFG_EXREG_LEN(exreg_group_id) - 1, target_expr->get_sort()->get_size()), target_expr);
    }
    s.set_expr_in_map(regname, target_expr);
    s.remove_key(target_reg.second);
  }
  /*for (auto rk : remove_keys) {
    s.remove_key(rk);
  }*/
}

static shared_ptr<tfg_edge const>
tfg_edge_strict_canonicalize_regs(tfg const *t_strict, shared_ptr<tfg_edge const> const &e,
    long num_exregs_used[ETFG_NUM_EXREG_GROUPS], regmap_t *st_regmap,
    fixed_reg_mapping_t const &fixed_reg_mapping)
{
  context *ctx = t_strict->get_context();
  state state_to = e->get_to_state();
  expr_ref econd = e->get_cond();
  //expr_ref indir_tgt = e->get_indir_tgt();
  state const &strict_start_state = t_strict->get_start_state().get_state();
  state_strict_canonicalize_regs(ctx, state_to, num_exregs_used, st_regmap, fixed_reg_mapping, strict_start_state);
  econd = expr_strict_canonicalize_regs(ctx, econd, num_exregs_used, st_regmap, fixed_reg_mapping);
  unordered_set<expr_ref> assumes;
  for (auto const& assume_e : e->get_assumes()) {
    expr_ref const& new_assume_e = expr_strict_canonicalize_regs(ctx, assume_e, num_exregs_used, st_regmap, fixed_reg_mapping);
    assumes.insert(new_assume_e);
  }
  /*if (e->get_is_indir_exit()) {
    ASSERT(indir_tgt);
    indir_tgt = expr_strict_canonicalize_regs(ctx, indir_tgt, num_exregs_used, st_regmap);
  }*/
  shared_ptr<tfg_edge const> strict_e = mk_tfg_edge(mk_itfg_edge(e->get_from_pc(), e->get_to_pc(), state_to, econd/*, strict_start_state*/, assumes, e->get_te_comment()));
  return strict_e;
}

static predicate_ref
predicate_strict_canonicalize_regs(predicate_ref const &p, long num_exregs_used[ETFG_NUM_EXREG_GROUPS], regmap_t *st_regmap, fixed_reg_mapping_t const &fixed_reg_mapping)
{
  context *ctx = p->get_context();
  expr_ref new_lhs_expr = expr_strict_canonicalize_regs(ctx, p->get_lhs_expr(), num_exregs_used, st_regmap, fixed_reg_mapping);
  expr_ref new_rhs_expr = expr_strict_canonicalize_regs(ctx, p->get_rhs_expr(), num_exregs_used, st_regmap, fixed_reg_mapping);
  return p->set_lhs_and_rhs_exprs(new_lhs_expr, new_rhs_expr);
}

static void
etfg_insn_strict_canonicalize_regs(etfg_strict_canonicalization_state_t *scanon_state, struct etfg_insn_t *var_insn,
    long num_exregs_used[ETFG_NUM_EXREG_GROUPS], regmap_t *st_regmap, fixed_reg_mapping_t const &fixed_reg_mapping)
{
  tfg *t_strict = scanon_state->get_tfg();
  vector<etfg_insn_edge_t> new_edges;
  list<predicate_ref> new_ls;
  for (const auto &p : var_insn->m_preconditions) {
    predicate_ref new_pred = predicate_strict_canonicalize_regs(p, num_exregs_used, st_regmap, fixed_reg_mapping);
    new_ls.push_back(new_pred);
  }
  var_insn->m_preconditions = new_ls;
  for (const auto &ie : var_insn->m_edges) {
    //shared_ptr<tfg_edge> e = tfg_get_edge_from_edgenum(var_insn->m_tfg, edgenum);
    shared_ptr<tfg_edge const> const &e = ie.get_tfg_edge();
    shared_ptr<tfg_edge const> strict_e = tfg_edge_strict_canonicalize_regs(t_strict, e, num_exregs_used, st_regmap, fixed_reg_mapping);
    t_strict->add_edge(strict_e);
    //var_insn->m_tfg = t_strict;
    var_insn->etfg_insn_set_tfg(t_strict);
    etfg_insn_edge_t strict_ie(strict_e, ie.get_pcrels());
    new_edges.push_back(strict_ie);
  }
  var_insn->m_edges = new_edges;
}

static long
etfg_iseq_count_nextpcs(etfg_insn_t const *iseq, long iseq_len)
{
  set<nextpc_id_t> seen_nextpcs;
  for (size_t i = 0; i < iseq_len; i++) {
    for (auto e : iseq[i].m_edges) {
      for (const auto &pcrel : e.get_pcrels()) {
        if (pcrel.tag == tag_var) {
          seen_nextpcs.insert(pcrel.val);
        }
      }
    }
  }
  return seen_nextpcs.size();
}

long
etfg_operand_strict_canonicalize_regs(etfg_strict_canonicalization_state_t *scanon_state,
    long insn_index, long op_index,
    struct etfg_insn_t *iseq_var[], long iseq_var_len[], struct regmap_t *st_regmap,
    struct imm_vt_map_t *st_imm_map, fixed_reg_mapping_t const &fixed_reg_mapping,
    long num_iseq_var, long max_num_iseq_var)
{
  long i, n, num_exregs_used[ETFG_NUM_EXREG_GROUPS];
  //map<nextpc_id_t, nextpc_id_t> fcall_nextpc_map;
  //nextpc_id_t cur_fcall_nextpc_id = 0;
  etfg_insn_t *var_insn;
  regmap_t *st_map;
  ASSERT(op_index >= 0);
  if (op_index != 0) {
    return num_iseq_var; //op_index == 0 is for regs; rest are for imms
  }

  //cout << __func__ << " " << __LINE__ << ": entry\n";

  ASSERT(num_iseq_var <= max_num_iseq_var);
  for (n = 0; n < num_iseq_var; n++) {
    //long num_nextpcs = etfg_iseq_count_nextpcs(iseq_var[n], iseq_var_len[n]);
    for (i = 0; i < ETFG_NUM_EXREG_GROUPS; i++) {
      num_exregs_used[i] = regmap_count_exgroup_regs(&st_regmap[n], i);
    }

    var_insn = &iseq_var[n][insn_index];
    st_map = &st_regmap[n];
    //cout << __func__ << " " << __LINE__ << ": before canonicalize_regs:\n" << etfg_insn_to_string(var_insn, as1, sizeof as1) << endl;
    etfg_insn_strict_canonicalize_regs(scanon_state, var_insn, num_exregs_used, st_map, fixed_reg_mapping);
    //cout << __func__ << " " << __LINE__ << ": after canonicalize_regs:\n" << etfg_insn_to_string(var_insn, as1, sizeof as1) << endl;
  }
  return num_iseq_var;
}

static pair<string, tfg const *>
function_tfg_map_get_function_name_and_tfg_from_fid(map<string, tfg const *>  const *function_tfg_map, unsigned fid)
{
  ASSERT(fid < function_tfg_map->size());
  auto iter = function_tfg_map->begin();
  for (unsigned i = 0; i < fid; i++, iter++);
  return *iter;
}

static unsigned
function_tfg_map_get_fid_for_function_name(map<string, tfg const *>  const *function_tfg_map, string const &function_name)
{
  unsigned ret = 0;
  for (auto iter = function_tfg_map->begin();
       iter != function_tfg_map->end();
       iter++) {
    if (iter->first == function_name) {
      return ret;
    }
    ret++;
  }
  cout << __func__ << " " << __LINE__ << ": could not find " << function_name << " in function_tfg_map\n";
  NOT_REACHED();
}

static void
etfg_insn_init(struct etfg_insn_t *insn)
{
  //insn->m_tfg = NULL;
  //insn->m_edges.clear();
  //insn->m_preconditions.clear();
  insn->clear();
}

static void
function_tfg_map_pc_get_function_name_and_insn_num(const void *function_tfg_map_in, src_ulong pc_long, string &function_name, long &insn_num_in_function)
{
  map<string, tfg const *>  const *function_tfg_map = (map<string, tfg const *>  const *)function_tfg_map_in;
  unsigned fid = pc_long / ETFG_MAX_INSNS_PER_FUNCTION;
  unsigned insn_num = pc_long - (fid * ETFG_MAX_INSNS_PER_FUNCTION);
  pair<string, tfg const *> p = function_tfg_map_get_function_name_and_tfg_from_fid(function_tfg_map, fid);
  function_name = p.first;
  insn_num_in_function = insn_num;
}

static list<predicate_ref>
fetch_ub_preconditions_at_pc(tfg const &t, pc const &p)
{
  list<predicate_ref> ret;

  //t.get_assume_preds(p, preds);
  //unordered_set<predicate_ref> const &preds = t.get_assume_preds(p);
  unordered_set<predicate_ref> preds;// = t.get_assume_preds(p); //XXX: this should be replaced with edge

  //cout << __func__ << " " << __LINE__ << ": preds.size() = " << preds.size() << endl;
  for (const auto &pred : preds) {
    //cout << __func__ << " " << __LINE__ << ": pred = " << pred.to_string_for_eq() << endl;
    if (   pred->get_precond().precond_is_trivial()
        && pred->predicate_is_ub_assume()
        && !pred->predicate_is_ub_memaccess_langtype_assume()
        && !pred->predicate_is_ub_langaligned_assume()) {
      //cout << __func__ << " " << __LINE__ << ": adding pred = " << pred.to_string_for_eq() << endl;
      ret.push_back(pred);
    } else {
      //cout << __func__ << " " << __LINE__ << ": ignoring pred = " << pred.to_string_for_eq() << endl;
    }
  }
  return ret;
}

static bool
function_tfg_map_fetch_raw(struct etfg_insn_t *insn, const void *function_tfg_map_in, src_ulong pc_long, unsigned long *size)
{
  map<string, tfg const *>  const *function_tfg_map = (map<string, tfg const *>  const *)function_tfg_map_in;
  unsigned fid = pc_long / ETFG_MAX_INSNS_PER_FUNCTION;
  src_ulong next_function_pc = (fid + 1) * ETFG_MAX_INSNS_PER_FUNCTION;
  etfg_insn_init(insn);
  *size = (next_function_pc - pc_long);
  if (fid >= function_tfg_map->size()) {
    return false;
  }
  pair<string, tfg const *> p = function_tfg_map_get_function_name_and_tfg_from_fid(function_tfg_map, fid);
  DYN_DEBUG(ETFG_INSN_FETCH, cout << __func__ << " " << __LINE__ << ": pc_long = " << hex << pc_long << dec << ", fname = " << p.first << endl);
  string const &function_name = p.first;
  tfg const *t = p.second;
  //cout << __func__ << " " << __LINE__ << ": t =\n" << t->graph_to_string() << endl;
  unsigned insn_num = pc_long - (fid * ETFG_MAX_INSNS_PER_FUNCTION);
  list<dshared_ptr<tfg_edge const>> const &es = t->get_edges();
  pc prev_from_pc;
  int insn_num_cur = -1;
  auto iter = es.begin();
  for (; iter != es.end(); iter++) {
    dshared_ptr<tfg_edge const> e = *iter;
    pc from_pc = e->get_from_pc();
    if (prev_from_pc != from_pc) {
      insn_num_cur++;
    }
    //edgenum++;
    prev_from_pc = from_pc;
    if (insn_num_cur == insn_num) {
      break;
    }
    DYN_DEBUG(ETFG_INSN_FETCH, cout << __func__ << " " << __LINE__ << ": returning (" << p.first << ", " << e->to_string() << ") for " << hex << from_pc << dec << endl);
  }
  if (insn_num_cur != insn_num) {
    ASSERT(insn_num_cur < insn_num);
    return false;
  }
  ASSERT(insn_num_cur == insn_num);
  ASSERT(iter != es.end());
  auto iter2 = iter;
  while (   iter2 != es.end()
         && (*iter2)->get_from_pc() == prev_from_pc) {
    iter2++;
  }
  if (iter2 == es.end()) {
    *size = (next_function_pc - pc_long); //the last instruction increments pc so that the next pc points to the next function
  } else {
    *size = 1;
  }
  DYN_DEBUG(ETFG_INSN_FETCH, cout << __func__ << " " << __LINE__ << ": prev_from_pc = " << prev_from_pc.to_string() << endl);
  insn->m_preconditions = fetch_ub_preconditions_at_pc(*t, prev_from_pc);
  while (   iter != es.end()
         && (*iter)->get_from_pc() == prev_from_pc) {
    //insn->m_edgenums.push_back(edgenum);
    //edgenum++;
    pc to_pc = (*iter)->get_to_pc();
    vector<valtag_t> pcrels;
    expr_ref nextpc = (*iter)->get_function_call_nextpc();
    if (nextpc && nextpc->is_nextpc_const()) {
      nextpc_id_t nextpc_id = nextpc->get_nextpc_num();
      ASSERT(t->get_nextpc_map().count(nextpc_id));
      string callee_name = t->get_nextpc_map().at(nextpc_id);
      if (callee_name == G_SELFCALL_IDENTIFIER) {
        callee_name = function_name;
      }
      valtag_t pcrel;
      if (!function_tfg_map->count(callee_name)) {
        pcrel.tag = tag_var;
        pcrel.val = nextpc_id;
      } else {
        pcrel.tag = tag_const;
        pcrel.val = function_tfg_map_get_fid_for_function_name(function_tfg_map, callee_name) * ETFG_MAX_INSNS_PER_FUNCTION;
      }
      pcrels.push_back(pcrel);
    }
    if (to_pc.is_exit()) {
      //cout << __func__ << " " << __LINE__ << ": to_pc = " << to_pc.to_string() << endl;
      //cout << _FNLN_ << ": (*iter) = " << (*iter)->get_from_pc().to_string() << "=>" << (*iter)->get_to_pc().to_string() << ", to_state =\n" << (*iter)->get_to_state().to_string() << endl;
      if (!(*iter)->edge_is_indirect_branch()) {
        if ((*iter)->get_to_state().get_value_expr_map_ref().size() != 0) {
          cout << __func__ << " " << __LINE__ << ": from_pc = " << prev_from_pc.to_string() << endl;
          cout << __func__ << " " << __LINE__ << ": to_pc = " << to_pc.to_string() << endl;
          cout << __func__ << " " << __LINE__ << ": (*iter)->get_to_state() = " << (*iter)->get_to_state().state_to_string_for_eq() << endl;
        }
        ASSERT((*iter)->get_to_state().get_value_expr_map_ref().size() == 0); //exits edges are only allowed for instructions that are otherwise nop

        valtag_t pcrel;
        pcrel.val = atoi(to_pc.get_index());
        pcrel.tag = tag_var;
        pcrels.push_back(pcrel);
      }
    } else {
      unsigned to_insn_num = tfg_get_first_insn_num_from_pc(t, to_pc);
      if (to_insn_num != insn_num + 1) {
        if ((*iter)->get_to_state().get_value_expr_map_ref().size() != 0) {
          cout << __func__ << " " << __LINE__ << ": from_pc = " << prev_from_pc.to_string() << endl;
          cout << __func__ << " " << __LINE__ << ": (*iter)->get_to_pc() = " << (*iter)->get_to_pc().to_string() << endl;
          cout << __func__ << " " << __LINE__ << ": (*iter)->get_to_state() = " << (*iter)->get_to_state().state_to_string_for_eq() << endl;
        }
        ASSERT((*iter)->get_to_state().get_value_expr_map_ref().size() == 0); //pcrels are only allowed for instructions that are otherwise nop
        valtag_t pcrel;
        pcrel.val = fid * ETFG_MAX_INSNS_PER_FUNCTION + to_insn_num; //see COMMENT1
        //pcrel.val = to_insn_num - (insn_num + *size);
        pcrel.tag = tag_const;
        pcrels.push_back(pcrel);
      }
    }
    insn->m_edges.push_back(etfg_insn_edge_t(*iter, pcrels));
    iter++;
  }
  //insn->m_tfg = t;
  insn->etfg_insn_set_tfg(t);
  stringstream ss;
  ss << p.first << ":" << insn_num;
  insn->m_comment = ss.str();
  //insn->m_edgenums = edgenum;
  DYN_DEBUG(ETFG_INSN_FETCH, cout << __func__ << " " << __LINE__ << ": insn = " << etfg_insn_to_string(insn, as1, sizeof as1) << endl);
  return true;
}

bool
etfg_insn_fetch_raw(struct etfg_insn_t *insn, struct input_exec_t const *in,
    src_ulong pc, unsigned long *size)
{
  input_section_t const *isec = input_exec_get_text_section(in);
  void *function_tfg_map = isec->data;
  ASSERT(function_tfg_map);
  return function_tfg_map_fetch_raw(insn, function_tfg_map, pc, size);
}

dshared_ptr<state const>
etfg_pc_get_input_state(struct input_exec_t const *in, src_ulong pc)
{
  input_section_t const *isec = input_exec_get_text_section(in);
  const void *function_tfg_map_in = isec->data;
  map<string, tfg const *>  const *function_tfg_map = (map<string, tfg const *>  const *)function_tfg_map_in;
  unsigned fid = pc / ETFG_MAX_INSNS_PER_FUNCTION;
  ASSERT(fid < function_tfg_map->size());
  pair<string, tfg const *> p = function_tfg_map_get_function_name_and_tfg_from_fid(function_tfg_map, fid);
  return p.second->get_start_state_with_all_input_exprs();
}



void
etfg_insn_canonicalize(etfg_insn_t *insn)
{
  return;
}

void etfg_insn_add_to_pcrels(etfg_insn_t *insn, src_ulong pc, struct bbl_t const *bbl, long bblindex)
{
  //do nothing
}

void etfg_init(void)
{
  autostop_timer func_timer(__func__);
}

bool
etfg_infer_regcons_from_assembly(char const *assembly,
    struct regcons_t *cons)
{
  regcons_set(cons);
  return true;
}

ssize_t etfg_assemble(translation_instance_t *ti, uint8_t *bincode, size_t bincode_size,
    char const *assembly, i386_as_mode_t mode)
{
  NOT_REACHED();
}

valtag_t *
etfg_insn_get_pcrel_operands(etfg_insn_t const *insn)
{
  NOT_IMPLEMENTED();
}

void etfg_insn_inv_rename_registers(struct etfg_insn_t *insn,
    struct regmap_t const *regmap)
{
  NOT_IMPLEMENTED();
}

bool etfg_insn_inv_rename_memory_operands(struct etfg_insn_t *insn,
    struct regmap_t const *regmap, struct vconstants_t const *vconstants)
{
  NOT_IMPLEMENTED();
}

void etfg_insn_inv_rename_constants(struct etfg_insn_t *insn,
    struct vconstants_t const *vconstants)
{
  NOT_IMPLEMENTED();
}

bool
etfg_insn_terminates_bbl(etfg_insn_t const *insn)
{
  if (etfg_insn_is_unconditional_branch_not_fcall(insn)) {
    //cout << __func__ << " " << __LINE__ << ": returning true for " << etfg_insn_to_string(insn, as1, sizeof as1) << endl;
    return true;
  }
  ASSERT(insn->m_edges.size() > 0);
  if (insn->m_edges.size() == 1) {
    return false;
  }
  return true;
  /*src_ulong edgenum = insn->m_edgenums.at(0);
  shared_ptr<tfg_edge> e = tfg_get_edge_from_edgenum(insn->m_tfg, edgenum);
  pc from_pc = e->get_from_pc();
  tfg const *t = (tfg const *)insn->m_tfg;
  list<shared_ptr<tfg_edge>> outgoing;
  t->get_edges_outgoing(from_pc, outgoing);
  if (outgoing.size() == 1) {
    cout << __func__ << " " << __LINE__ << ": returning false for " << etfg_insn_to_string(insn, as1, sizeof as1) << endl;
    return false;
  }
  //cout << __func__ << " " << __LINE__ << ": from_pc = " << from_pc.to_string() << ", m_edgenum = " << insn->m_edgenum << ", outgoing.size() = " << outgoing.size() << endl;
  //return ret;
  list<shared_ptr<tfg_edge>> es = t->get_edges_ordered();
  ASSERT(insn->m_edgenum < es.size());
  unsigned next_edgenum = insn->m_edgenum + 1;
  ASSERT(next_edgenum <= es.size());
  if (next_edgenum == es.size()) {
    cout << __func__ << " " << __LINE__ << ": returning true for " << etfg_insn_to_string(insn, as1, sizeof as1) << endl;
    return true;
  }
  ASSERT(next_edgenum < es.size());
  shared_ptr<tfg_edge> next_e = tfg_get_edge_from_edgenum(insn->m_tfg, next_edgenum);
  if (next_e->get_from_pc() == from_pc) {
    cout << __func__ << " " << __LINE__ << ": returning false for " << etfg_insn_to_string(insn, as1, sizeof as1) << endl;
    return false;
  }
  cout << __func__ << " " << __LINE__ << ": returning true for " << etfg_insn_to_string(insn, as1, sizeof as1) << endl;
  return true;*/
}

bool symbol_is_contained_in_etfg_insn(struct etfg_insn_t const *insn, long symval,
    src_tag_t symtag)
{
  NOT_IMPLEMENTED();
}

bool etfg_insn_symbols_are_contained_in_map(struct etfg_insn_t const *insn, struct imm_vt_map_t const *imm_vt_map)
{
  NOT_IMPLEMENTED();
}

bool
etfg_insn_get_constant_operand(etfg_insn_t const *insn, uint32_t *constant)
{
  return false;
}

bool
etfg_input_section_pc_is_executable(struct input_section_t const *isec, src_ulong pc)
{
  map<string, tfg const *> const *function_tfg_map = (map<string, tfg const *> const *)isec->data;
  etfg_insn_t tmp_insn;
  unsigned long tmp_size;

  return function_tfg_map_fetch_raw(&tmp_insn, function_tfg_map, pc, &tmp_size);
/*
  unsigned fid = pc / ETFG_MAX_INSNS_PER_FUNCTION;
  if (fid >= function_tfg_map->size()) {
    return false;
  }
  pair<string, tfg const *> p = function_tfg_map_get_function_name_and_tfg_from_fid(function_tfg_map, fid);
  tfg const *t = p.second;
  unsigned edgenum = pc - (fid * ETFG_MAX_INSNS_PER_FUNCTION);
  set<shared_ptr<tfg_edge>> es = t->get_edges();
  if (edgenum > es.size()) {
    return false;
  }
  return true;
*/
}

bool
etfg_pc_is_valid_pc(struct input_exec_t const *in, src_ulong pc)
{
  input_section_t const *isec = input_exec_get_text_section(in);
  return etfg_input_section_pc_is_executable(isec, pc);
}

size_t
etfg_insn_put_nop(etfg_insn_t *insns, size_t insns_size)
{
  char const *etfg_nop_insn_str =
"=etfg_insn nop {{{\n"
"=tfg_edge: L0%0%1 => L0%1%1\n"
"=tfg_edge.EdgeCond:\n"
"1 : 1 : BOOL\n"
"=tfg_edge.StateTo: \n"
"=state_end\n"
"=tfg_edge.Assumes.begin:\n"
"=tfg_edge.Assumes.end\n"
"=tfg_edge.te_comment\n"
"0:-1:nop\n"
"tfg_edge_comment end\n"
"=comment\n"
"nop:1\n"
"}}}\n";
  long ret = src_iseq_from_string(NULL, insns, insns_size, etfg_nop_insn_str, etfg_get_i386_as_mode());
  ASSERT(ret <= insns_size);
  ASSERT(ret == 1);
  return ret;
}

size_t
etfg_insn_put_move_imm_to_mem(dst_ulong imm, uint32_t memaddr, int membase, etfg_insn_t *insns, size_t insns_size)
{
  char const *format =
"=etfg_insn move_imm_to_mem {{{\n"
"=tfg_edge: L0%%0%1 => L0%%1%%1\n"
"=tfg_edge.EdgeCond:\n"
"1 : 1 : BOOL\n"
"=tfg_edge.StateTo: \n"
"=src.llvm-mem\n"
"1 : input.src.llvm-mem : ARRAY[BV:32 -> BV:8]\n"
"2 : mlvar.llvm.0 : MEMLABEL\n"
"3 : %ld : BV:32\n"
"4 : %ld : BV:32\n"
"5 : 4 : INT\n"
"6 : 0 : BOOL\n"
"7 : store(1, 2, 3, 4, 5, 6) : ARRAY[BV:32 -> BV:8]\n"
"=state_end\n"
"=tfg_edge.te_comment\n"
"0:-1:move_imm_to_mem\n"
"tfg_edge_comment end\n"
"=comment\n"
"store4:1\n"
"}}}\n";
  size_t str_size = strlen(format) + 256;
  char str[str_size];
  if (membase != -1) {
    NOT_IMPLEMENTED();
  }
  snprintf(str, str_size, format, (long)memaddr, (long)imm);
  long ret = src_iseq_from_string(NULL, insns, insns_size, str, etfg_get_i386_as_mode());
  ASSERT(ret <= insns_size);
  ASSERT(ret == 1);
  return ret;
}

bool
etfg_insn_type_signature_supported(etfg_insn_t const *insn)
{
  return true;
}

bool
etfg_iseq_rename_regs_and_imms(etfg_insn_t *iseq, long iseq_len,
    regmap_t const *regmap, imm_vt_map_t const *imm_map, nextpc_t const *nextpcs,
    long num_nextpcs/*, src_ulong const *insn_pcs, long num_insns*/)
{
  NOT_IMPLEMENTED();
}

size_t
etfg_insn_put_nop_as(char *buf, size_t size)
{
  NOT_IMPLEMENTED();
}

size_t
etfg_insn_put_linux_exit_syscall_as(char *buf, size_t size)
{
  NOT_IMPLEMENTED();
}

regset_t const *
etfg_regset_live_at_jumptable()
{
  static regset_t _regset_live_at_jumptable;
  static bool init = false;
  if (!init) {
    regset_clear(&_regset_live_at_jumptable);
    init = true;
  }
  return &_regset_live_at_jumptable;
}

static tfg *
create_tfg_for_insn_edges(vector<etfg_insn_edge_t> const &edges, context *ctx)
{
  tfg *ret = new tfg_llvm_t(ETFG_ISEQ_TFG_NAME, DUMMY_FNAME, ctx/*, etfg_canonical_start_state(ctx)*/);
  ret->set_start_state(etfg_canonical_start_state(ctx));
  for (auto e : edges) {
    pc from_pc = e.get_tfg_edge()->get_from_pc();
    pc to_pc = e.get_tfg_edge()->get_to_pc();
    dshared_ptr<tfg_node> from_node = ret->find_node(from_pc);
    dshared_ptr<tfg_node> to_node = ret->find_node(to_pc);
    if (!from_node) {
      from_node = make_dshared<tfg_node>(from_pc);
      ret->add_node(from_node);
    }
    if (!to_node) {
      to_node = make_dshared<tfg_node>(to_pc);
      ret->add_node(to_node);
    }
    ret->add_edge(e.get_tfg_edge());
  }
  return ret;
}

#define parse_error() do { cout << __func__ << " " << __LINE__ << ": parse error at \"" << ptr << "\"\n"; exit(1); } while (0)

static pair<string, etfg_insn_edge_t>
etfg_insn_edge_read(istream &in, string line, context *ctx)
{
  string prefix = "=tfg_edge";
  bool end;
  ASSERT(is_line(line, prefix));
  //getline(in, line);
  //string comment;
  //pc from_pc, to_pc;
  //edge_read_pcs_and_comment(line.substr(prefix.size() + 1), from_pc, to_pc, comment);
  state const &start_state = etfg_canonical_start_state(ctx);
  //expr_ref edgecond, indir_tgt;
  //bool is_indir_exit;
  //state state_to;
  //bool end = !getline(in, line);
  //ASSERT(!end);
  shared_ptr<tfg_edge const> e;
  //line = read_tfg_edge_using_start_state(in, line, ctx, prefix, start_state, e);
  e = tfg_edge::graph_edge_from_stream(in, line, prefix, ctx);
  ASSERT(e);

  do {
    end = !getline(in, line);
    ASSERT(!end);
  } while (line == "");
  //shared_ptr<tfg_edge> e = make_shared<tfg_edge>(itfg_edge(from_pc, to_pc, state_to, edgecond, start_state/*, is_indir_exit, indir_tgt*/, comment));
  vector<valtag_t> pcrels;
  while (is_line(line, "=pcrel_val")) {
    getline(in, line);
    valtag_t pcrel;
    pcrel.val = stoi(line);
    getline(in, line);
    ASSERT(is_line(line, "=pcrel_tag"));
    getline(in, line);
    pcrel.tag = src_tag_from_string(line);
    pcrels.push_back(pcrel);
    getline(in, line);
  }
  etfg_insn_edge_t ret(e, pcrels);
  return make_pair(line, ret);
}

long
src_iseq_from_string(translation_instance_t *ti, etfg_insn_t *etfg_iseq,
    size_t etfg_iseq_size, char const *s, i386_as_mode_t i386_as_mode)
{
  DYN_DEBUG(CMN_ISEQ_FROM_STRING, cout << __func__ << " " << __LINE__ << ": string =\n" << s << endl);
  etfg_insn_t *insn_ptr = etfg_iseq;
  etfg_insn_t *insn_end = etfg_iseq + etfg_iseq_size;
  ASSERT(g_ctx);
  context *ctx = g_ctx;
  state const &start_state = etfg_canonical_start_state(ctx);
  istringstream in(s);
  string line;
  bool end;
  end = !getline(in, line);
  ASSERT(!end);
  while (is_line(line, "=etfg_insn")) {
    ASSERT(string_has_suffix(line, "{{") && string_has_suffix(line.substr(0, line.size() - 1), "{{")); //round-about way of asserting that string has three {s in the end (the round-about way ensures that this is not treated a fold marker by VIM)
    ASSERT(insn_ptr < insn_end);
    insn_ptr->m_edges.clear();
    getline(in, line);

    unordered_set<predicate_ref> theos;
    while (is_line(line, "=precondition")) {
      DYN_DEBUG2(CMN_ISEQ_FROM_STRING, cout << __func__ << " " << __LINE__ << ": line = " << line << endl);
      predicate_ref pred;
      predicate::predicate_from_stream(in/*, start_state*/, ctx, pred);
      theos.insert(pred);
      end = !getline(in, line);
      ASSERT(!end);
    }
    insn_ptr->m_preconditions.clear();
    for (const auto &theo : theos) {
      //cout << __func__ << " " << __LINE__ << ": pushing one theo\n";
      insn_ptr->m_preconditions.push_back(theo);
    }
    while (is_line(line, "=tfg_edge")) {
      pair<string, etfg_insn_edge_t> p = etfg_insn_edge_read(in, line, ctx);
      line = p.first;
      etfg_insn_edge_t insn_edge = p.second;
      insn_ptr->m_edges.push_back(insn_edge);
    }
    ASSERT(insn_ptr->m_edges.size() > 0);
    insn_ptr->etfg_insn_set_tfg(create_tfg_for_insn_edges(insn_ptr->m_edges, ctx));

    DYN_DEBUG(CMN_ISEQ_FROM_STRING, cout << __func__ << " " << __LINE__ << ": etfg_insn:\n" << etfg_insn_to_string(insn_ptr, as1, sizeof as1) << endl);
    if (!is_line(line, "=comment")) {
      cout << __func__ << " " << __LINE__ << ": line = " << line << ".\n";
    }
    ASSERT(is_line(line, "=comment"));
    getline(in, line);
    insn_ptr->m_comment = line;
    getline(in, line);
    ASSERT(line.substr(0,2) == "}}" && line.substr(1, 2) == "}}"); //round-about way of asserting that string equals three {s (the round-about way ensures that this is not treated a fold marker by VIM)
    getline(in, line);

    insn_ptr++;
  }
  if (insn_ptr == etfg_iseq) {
    cout << __func__ << " " << __LINE__ << ": s = " << s << "." << endl;
  }
  ASSERT(insn_ptr > etfg_iseq);
  return insn_ptr - etfg_iseq;
}

struct etfg_strict_canonicalization_state_t *
etfg_strict_canonicalization_state_init()
{
  context *ctx = g_ctx;
  consts_struct_t const &cs = ctx->get_consts_struct();
  state const &start_state = etfg_canonical_start_state(ctx);
  //tfg *t = new tfg(mk_string_ref("llvm"), ctx, start_state);
  tfg *t = new tfg_llvm_t("llvm", DUMMY_FNAME, ctx);
  t->set_start_state(start_state);
  etfg_strict_canonicalization_state_t *ret = new etfg_strict_canonicalization_state_t(t);
  return ret;
}

void
etfg_strict_canonicalization_state_free(struct etfg_strict_canonicalization_state_t *s)
{
  delete s;
}

string
etfg_insn_edge_t::etfg_insn_edge_to_string(context *ctx) const
{
  //context *ctx = t->get_context();
  stringstream ss;
  //ss << "=tfg_edge\n";
  ss << m_tfg_edge->to_string_for_eq("=tfg_edge") << "\n";
  //ss << "=tfg_edge_EdgeCond:\n";
  //ss << ctx->expr_to_string_table(m_tfg_edge->get_cond()) << "\n";
  //ss << "=tfg_edge_StateTo:\n";
  //ss << m_tfg_edge->get_to_state().to_string_for_eq();
  //ss << "=tfg_edge_IsIndirExit:\n";
  //ss << (m_tfg_edge->get_is_indir_exit() ? "true" : "false") << "\n";
  //if (m_tfg_edge->get_is_indir_exit()) {
    //ss << "=tfg_edge_IndirTgt:\n" << ctx->expr_to_string_table(m_tfg_edge->get_indir_tgt()) << "\n";
  //}
  for (auto pcrel : m_pcrels) {
    ss << "=pcrel_val" << endl;
    ss << pcrel.val << endl;
    ss << "=pcrel_tag" << endl;
    ss << src_tag_to_string(pcrel.tag) << endl;
  }
  return ss.str();
}

char *
etfg_insn_get_fresh_state_str()
{
  stringstream ss;
  ss << etfg_canonical_start_state(g_ctx).state_to_string_for_eq();
  return strdup(ss.str().c_str());
}

/*static void
etfg_state_to_add_callee_save_registers_and_retval_to_hidden(state &state_to)
{
  expr_ref indir_tgt = state_to.get_expr(LLVM_STATE_INDIR_TARGET_KEY_ID, state_to);
  context *ctx = indir_tgt->get_context();
  stringstream ss;
  state_to.set_expr(G_HIDDEN_REG_NAME "0", ctx->mk_var(G_LLVM_RETURN_REGISTER_NAME, ctx->mk_bv_sort(ETFG_EXREG_LEN(ETFG_EXREG_GROUP_GPRS))));
  for (size_t i = 0; i < LLVM_NUM_CALLEE_SAVE_REGS; i++) {
    stringstream ss1, ss2;
    ss1 << G_HIDDEN_REG_NAME << i + 1;
    ss2 << LLVM_CALLEE_SAVE_REGNAME "." << i;
    string hidden_regname = ss1.str();
    string llvm_callee_save_regname = ss2.str();
    state_to.set_expr(hidden_regname, ctx->mk_var(llvm_callee_save_regname, ctx->mk_bv_sort(ETFG_EXREG_LEN(ETFG_EXREG_GROUP_GPRS))));
  }
}*/

static bool
etfg_state_to_represents_function_return(state const &state_to)
{
  string k;
  if ((k = state_to.has_expr_suffix(LLVM_STATE_INDIR_TARGET_KEY_ID)) == "") {
    return false;
  }
  expr_ref indir_tgt = state_to.get_expr(k/*, state_to*/);
  context *ctx = indir_tgt->get_context();
  consts_struct_t const &cs = ctx->get_consts_struct();
  return cs.expr_is_retaddr_const(indir_tgt);
}

/*static void
etfg_state_to_add_hidden_fields(state &state_to)
{
  if (etfg_state_to_represents_function_return(state_to)) {
    etfg_state_to_add_callee_save_registers_and_retval_to_hidden(state_to);
  }
}*/

char *
etfg_insn_get_transfer_state_str(etfg_insn_t const *insn, int num_nextpcs, int nextpc_indir)
{
  //cout << __func__ << " " << __LINE__ << ": insn = " << etfg_insn_to_string(insn, as1, sizeof as1) << endl;
  //cout << __func__ << " " << __LINE__ << ": num_nextpcs = " << num_nextpcs << endl;
  //cout << __func__ << " " << __LINE__ << ": nextpc_indir = " << nextpc_indir << endl;
  stringstream ss;
  ss << "=tfg\n";
  size_t precond_index = 0;
  size_t edge_index = 0;
  size_t cur_node_id = 0;
  //tfg const *t = (tfg const *)insn->m_tfg;
  //context *ctx = t->get_context();
  context *ctx = insn->get_context();
  map<pc, size_t> pc2node_id;
  
  for (const auto &precond : insn->m_preconditions) {
    ss << "=precondition." << precond_index << endl;
    precond_index++;
    ss << precond->to_string_for_eq() << endl;
  }
  for (const auto &e : insn->m_edges) {
    ss << "=edge." << edge_index << "\n";
    size_t from_node_id, to_node_id;
    pc from_pc = e.get_tfg_edge()->get_from_pc();
    pc to_pc = e.get_tfg_edge()->get_to_pc();
    if (!pc2node_id.count(from_pc)) {
      pc2node_id.insert(make_pair(from_pc, cur_node_id));
      cur_node_id++;
    }
    from_node_id = pc2node_id.at(from_pc);
    ASSERT(from_node_id == 0);
    ss << "=from\n";
    if (from_node_id == 0) {
      ss << "=StateNodeId_start\n";
    } else {
      ss << "=StateNodeId_internal" << from_node_id << "\n";
    }
    ss << "=to\n";
    if (to_pc == from_pc) {
      NOT_IMPLEMENTED(); //self loop within an instruction; not supported
      to_node_id = from_node_id; //keep the compiler happy
    } else {
      //valtag_t pcrel = e.get_pcrel();
      if (e.get_tfg_edge()->edge_is_indirect_branch()) {
        ss << "=StateNodeId_indir\n";
        ss << ctx->expr_to_string_table(e.get_tfg_edge()->edge_get_indir_target()) << "\n";
      } else {
        //if (e.get_pcrels().size() == 0) {
        //  cout << __func__ << " " << __LINE__ << ": e.get_pcrels().size() = " << e.get_pcrels().size() << endl;
        //  cout << __func__ << " " << __LINE__ << ": e = " << e.etfg_insn_edge_to_string(ctx) << endl;
        //}
        if (e.get_pcrels().size() == 0) {
          ss << "=StateNodeId_start\n";
        } else {
          valtag_t const &pcrel = e.get_pcrels().at(e.get_pcrels().size() - 1);
          ASSERT(pcrel.tag == tag_var);
          to_node_id = pcrel.val;
          ASSERT(pcrel.tag == tag_var);
          /*if (pcrel.val == num_nextpcs - 1) {
            ss << "=StateNodeId_start\n";
          } else */{
            ss << "=StateNodeId_nextpc" << to_node_id << "\n";
          }
        }
      }
    }
    ss << "=econd\n";
    expr_ref econd = e.get_tfg_edge()->get_cond();
    ss << ctx->expr_to_string_table(econd) << "\n";
    ss << "=etf\n";
    state state_to = e.get_tfg_edge()->get_to_state();
    //etfg_state_to_add_hidden_fields(state_to);
    ss << state_to.state_to_string_for_eq();
  }
  string s = ss.str();
  char *ret = strdup(s.c_str());
  return ret;
  return NULL;
}

void
etfg_pc_get_function_name_and_insn_num(input_exec_t const *in, src_ulong pc, string &function_name, long &insn_num_in_function)
{
  input_section_t const *isec = input_exec_get_text_section(in);
  void *function_tfg_map = isec->data;
  ASSERT(function_tfg_map);
  function_tfg_map_pc_get_function_name_and_insn_num(function_tfg_map, pc, function_name, insn_num_in_function);
}

static size_t
input_exec_get_bincode_for_function(string const &filename, string function_name, uint8_t *buf, size_t buf_size)
{
  input_exec_t *in = new input_exec_t;
  read_input_file(in, filename.c_str());
  symbol_id_t found = -1;
  /*if (function_name.substr(0, 3) == "gep") {
    string_replace(function_name, ":", "__");
  } else */{
    size_t colon = function_name.find(":");
    ASSERT(colon != string::npos);
    function_name = function_name.substr(0, colon);
  }
  //int i;
  for (const auto &s : in->symbol_map) {
    cout << __func__ << " " << __LINE__ << ": checking " << function_name << " against " << s.second.name << endl;
    if (!strcmp(s.second.name, function_name.c_str())) {
      cout << __func__ << " " << __LINE__ << ": found function '" << function_name << "' in x86 file" << filename << ". size = " << s.second.size << "\n";
      found = s.first;
      break;
    }
  }
  if (found == -1) {
    cout << __func__ << " " << __LINE__ << ": could not find function '" << function_name << "' in x86 file " << filename << "\n";
    return 0;
  }
  ASSERT(found != -1);
  ASSERT(in->symbol_map.at(found).size <= buf_size);
  for (size_t c = 0; c < in->symbol_map.at(found).size; c++) {
    buf[c] = ldub_input(in, in->symbol_map.at(found).value + c);
  }
  size_t ret = in->symbol_map.at(found).size;
  input_exec_free(in);
  delete in;
  return ret;
}

static size_t
dst_iseq_trim_out_ret_insn(dst_insn_t *dst_iseq, size_t dst_iseq_len)
{
  dst_insn_t *ptr = &dst_iseq[dst_iseq_len - 1];
  ASSERT(dst_insn_is_function_return(ptr));
  return dst_iseq_len - 1;
}

static bool
etfg_iseq_is_ret_insn(etfg_insn_t const *etfg_iseq, long etfg_iseq_len)
{
  if (etfg_iseq_len != 1) {
    return false;
  }
  return etfg_insn_is_function_return(&etfg_iseq[0]);
}

static bool
etfg_insn_edge_points_to_nextpc(etfg_insn_edge_t const *insn_edge, long &nextpc_num)
{
  if (insn_edge->get_pcrels().size() == 0) {
    return false;
  }
  if (insn_edge->get_pcrels().at(insn_edge->get_pcrels().size() - 1).tag != tag_var) {
    return false;
  }
  nextpc_num = insn_edge->get_pcrels().at(insn_edge->get_pcrels().size() - 1).val;
  return true;
}

static bool
etfg_insn_points_to_nextpc(etfg_insn_t const *insn, long &nextpc_num)
{
  ASSERT(insn->m_edges.size() > 0);
  return etfg_insn_edge_points_to_nextpc(&insn->m_edges.at(insn->m_edges.size() - 1), nextpc_num);
}

static bool
etfg_iseq_points_to_nextpc(etfg_insn_t const *src_iseq, long src_iseq_len, long &nextpc_num)
{
  return etfg_insn_points_to_nextpc(&src_iseq[src_iseq_len - 1], nextpc_num);
}

static size_t
dst_iseq_append_jump_to_nextpc(dst_insn_t *dst_iseq, size_t dst_iseq_len, size_t dst_iseq_size, long nextpc_num)
{
  ASSERT(dst_iseq_len < dst_iseq_size);
  dst_iseq_len += dst_insn_put_jump_to_nextpc(&dst_iseq[dst_iseq_len], dst_iseq_size - dst_iseq_len, nextpc_num);
  ASSERT(dst_iseq_len <= dst_iseq_size);
  return dst_iseq_len;
}

static void
dst_iseq_decrement_nextpc_nums(dst_insn_t *dst_iseq, size_t dst_iseq_len)
{
  for (size_t i = 0; i < dst_iseq_len; i++) {
    valtag_t *pcrel;
    if (   (pcrel = dst_insn_get_pcrel_operand(&dst_iseq[i]))
        && pcrel->tag == tag_var) {
      ASSERT(pcrel->val >= 1);
      pcrel->val--;
    }
  }
}



static void
dst_iseq_set_fcall_nextpc_to_zero(dst_insn_t *dst_iseq, size_t dst_iseq_len)
{
  for (size_t i = 0; i < dst_iseq_len; i++) {
    if (dst_insn_is_function_call(&dst_iseq[i])) {
      dst_insn_set_nextpc(&dst_iseq[i], 0);
    }
  }
}

static bool
dst_iseq_contains_function_call(dst_insn_t const *dst_iseq, size_t dst_iseq_len)
{
  for (size_t i = 0; i < dst_iseq_len; i++) {
    if (dst_insn_is_function_call(&dst_iseq[i])) {
      return true;
    }
  }
  return false;
}

/*static reg_identifier_t
etfg_insn_edge_identify_output_reg_for_reg_idx(etfg_insn_edge_t const *ie, size_t reg_idx)
{
  shared_ptr<tfg_edge> const &te = ie->get_tfg_edge();
  state const &to_state = te->get_to_state();
  map<string, expr_ref> const &m = to_state.get_value_expr_map_ref();
  for (auto me : m) {
    if (me.second->is_bv_sort()) {
      if (reg_idx == 0) {
        return get_reg_identifier_for_regname(me.first);
      }
      reg_idx--;
    }
  }
  return reg_identifier_t::reg_identifier_invalid();
}

static reg_identifier_t
etfg_insn_identify_output_reg_for_reg_idx(etfg_insn_t const *insn, size_t reg_idx)
{
  ASSERT(insn->m_edges.size() > 0);
  return etfg_insn_edge_identify_output_reg_for_reg_idx(&insn->m_edges.at(insn->m_edges.size() - 1), reg_idx);
}

static reg_identifier_t
etfg_iseq_identify_output_reg_for_reg_idx(etfg_insn_t const *src_iseq, size_t src_iseq_len, size_t reg_idx)
{
  ASSERT(src_iseq_len > 0);
  return etfg_insn_identify_output_reg_for_reg_idx(&src_iseq[src_iseq_len - 1], reg_idx);
}*/

static bool
reg_identifier_represents_arg(reg_identifier_t const &reg_id, int argnum)
{
  string reg_str = reg_id.reg_identifier_to_string();
  stringstream ss;
  ss << "a" << argnum;
  string arg_str = ss.str();
  if (reg_str.find(arg_str) != string::npos) {
    return true;
  }
  //cout << __func__ << " " << __LINE__ << ": reg_id = " << reg_id.reg_identifier_to_string() << endl;
  //cout << __func__ << " " << __LINE__ << ": argnum = " << argnum << endl;
  return false;
}

static bool
reg_identifier_represents_callee(reg_identifier_t const &reg_id)
{
  string reg_str = reg_id.reg_identifier_to_string();
  if (reg_str.find("callee") != string::npos) {
    return true;
  }
  return false;
}

static bool
reg_identifier_represents_retreg(reg_identifier_t const &reg_id, int retreg_num)
{
  string reg_str = reg_id.reg_identifier_to_string();
  stringstream ss;
  ss << "trgt." << retreg_num;
  string retreg_str = ss.str();
  if (reg_str.find(retreg_str) != string::npos) {
    return true;
  }
  //cout << __func__ << " " << __LINE__ << ": reg_id = " << reg_id.reg_identifier_to_string() << endl;
  //cout << __func__ << " " << __LINE__ << ": argnum = " << argnum << endl;
  return false;
}



static reg_identifier_t
regmap_get_reg_id_for_argnum(regmap_t const &src_regmap, int argnum)
{
  if (!src_regmap.regmap_extable.count(ETFG_EXREG_GROUP_GPRS)) {
    return reg_identifier_t::reg_identifier_unassigned();
  }
  for (auto e : src_regmap.regmap_extable.at(ETFG_EXREG_GROUP_GPRS)) {
    if (reg_identifier_represents_arg(e.second, argnum)) {
      return e.first;
    }
  }
  return reg_identifier_t::reg_identifier_unassigned();
}

static reg_identifier_t
regmap_get_reg_id_for_retreg_num(regmap_t const &src_regmap, int retreg_num)
{
  if (!src_regmap.regmap_extable.count(ETFG_EXREG_GROUP_GPRS)) {
    return reg_identifier_t::reg_identifier_unassigned();
  }
  for (auto e : src_regmap.regmap_extable.at(ETFG_EXREG_GROUP_GPRS)) {
    if (reg_identifier_represents_retreg(e.second, retreg_num)) {
      return e.first;
    }
  }
  return reg_identifier_t::reg_identifier_unassigned();
}

static void
dst_iseq_inv_rename_imms_using_etfg_canonical_constants(dst_insn_t *dst_iseq, size_t dst_iseq_len)
{
  imm_vt_map_t imm_vt_map;
  imm_vt_map.num_imms_used = 2;
  imm_vt_map.table[0].tag = tag_const;
  imm_vt_map.table[0].val = ETFG_CANON_CONST0;
  imm_vt_map.table[1].tag = tag_const;
  imm_vt_map.table[1].val = ETFG_CANON_CONST1;
  dst_iseq_inv_rename_imms_using_imm_vt_map(dst_iseq, dst_iseq_len, &imm_vt_map);
}

static bool
dst_iseq_overwrites_gpr(dst_insn_t const *dst_iseq, long dst_iseq_len, int gpr)
{
  regset_t used_regs, def_regs;
  regset_clear(&used_regs);
  regset_clear(&def_regs);
  dst_iseq_get_usedef_regs(dst_iseq, dst_iseq_len, &used_regs, &def_regs);
  return regset_belongs_ex(&def_regs, DST_EXREG_GROUP_GPRS, gpr);
}

static void
dst_insn_put_save_and_restore_overwritten_gpr(dst_insn_t *dst_save, dst_insn_t *dst_restore, exreg_id_t gpr, regset_t &used_regs, fixed_reg_mapping_t const &fixed_reg_mapping)
{
  int found_reg = -1;
  for (size_t i = 0; i < DST_NUM_GPRS; i++) {
    if (!regset_belongs_ex(&used_regs, DST_EXREG_GROUP_GPRS, i)) {
      found_reg = i;
      regset_mark_ex_mask(&used_regs, DST_EXREG_GROUP_GPRS, i, MAKE_MASK(DST_EXREG_LEN(DST_EXREG_GROUP_GPRS)));
      break;
    }
  }
  if (found_reg == -1) {
#if ARCH_DST == ARCH_I386
    size_t ret = i386_insn_put_push_reg(gpr, fixed_reg_mapping, dst_save, 1);
    ASSERT(ret == 1);
    ret = i386_insn_put_pop_reg(gpr, fixed_reg_mapping, dst_restore, 1);
    ASSERT(ret == 1);
#else
    NOT_IMPLEMENTED();
#endif
  } else {
#if ARCH_DST == ARCH_I386
    size_t ret = i386_insn_put_movl_reg_to_reg(found_reg, gpr, dst_save, 1);
    ASSERT(ret == 1);
    ret = i386_insn_put_movl_reg_to_reg(gpr, found_reg, dst_restore, 1);
    ASSERT(ret == 1);
#else
    NOT_IMPLEMENTED();
#endif
  }
}

static void
dst_iseq_add_save_restore_reg_insns_for_overwritten_regs(dst_insn_t *dst_iseq, long *dst_iseq_len, size_t dst_iseq_size, regset_t const &overwritten_regs, regset_t used_regs, fixed_reg_mapping_t const &fixed_reg_mapping)
{
  size_t num_overwritten_gprs = regset_count_exregs(&overwritten_regs, DST_EXREG_GROUP_GPRS);
  if (num_overwritten_gprs == 0) {
    return;
  }
  ASSERT(*dst_iseq_len + 2*num_overwritten_gprs <= dst_iseq_size);
  for (ssize_t i = *dst_iseq_len; i >= 0; i--) {
    dst_insn_copy(&dst_iseq[i + num_overwritten_gprs], &dst_iseq[i]);
  }
  size_t i = 0;
  for (const auto &r : overwritten_regs.excregs.at(DST_EXREG_GROUP_GPRS)) {
    dst_insn_put_save_and_restore_overwritten_gpr(&dst_iseq[i], &dst_iseq[*dst_iseq_len + 2*num_overwritten_gprs - 1 - i], r.first.reg_identifier_get_id(), used_regs, fixed_reg_mapping);
    i++;
  }
  *dst_iseq_len += 2*num_overwritten_gprs;
}

static void
dst_iseq_add_save_restore_insns_for_overwritten_regparams(dst_insn_t *dst_iseq, long *dst_iseq_len, size_t dst_iseq_size, fixed_reg_mapping_t const &fixed_reg_mapping)
{
  regset_t used_regs, def_regs, overwritten_regparams;
  regset_clear(&used_regs);
  regset_clear(&def_regs);
  regset_clear(&overwritten_regparams);
  for (size_t i = 0; i < DST_FASTCALL_NUM_REGPARAMS; i++) {
    if (dst_iseq_overwrites_gpr(dst_iseq, *dst_iseq_len, DST_FASTCALL_REGPARAM(i))) {
      regset_mark_ex_mask(&overwritten_regparams, DST_EXREG_GROUP_GPRS, DST_FASTCALL_REGPARAM_EXT(i), MAKE_MASK(DST_EXREG_LEN(DST_EXREG_GROUP_GPRS)));
    }
  }
  dst_iseq_get_usedef_regs(dst_iseq, *dst_iseq_len, &used_regs, &def_regs);
  regset_union(&used_regs, &def_regs);
  regset_union(&used_regs, &dst_reserved_regs);
  dst_iseq_add_save_restore_reg_insns_for_overwritten_regs(dst_iseq, dst_iseq_len, dst_iseq_size, overwritten_regparams, used_regs, fixed_reg_mapping);
}

static void
fixed_reg_mapping_intersect_with_dst_iseq_touch(fixed_reg_mapping_t &fixed_reg_mapping, dst_insn_t const *dst_iseq, long dst_iseq_len)
{
  regset_t use, def;
  dst_iseq_get_usedef_regs(dst_iseq, dst_iseq_len, &use, &def);
  regset_union(&use, &def);
  if (dst_iseq_contains_function_call(dst_iseq, dst_iseq_len)) {
    regset_mark_ex_mask(&use, DST_EXREG_GROUP_GPRS, DST_STACK_REGNUM, MAKE_MASK(DST_EXREG_LEN(DST_EXREG_GROUP_GPRS)));
  }
  fixed_reg_mapping.fixed_reg_mapping_intersect_machine_regs_with_regset(use);
}

static regset_t
etfg_iseq_compute_live_in(etfg_insn_t const *iseq, size_t iseq_len, regset_t const *live_out, size_t num_live_out)
{
  //XXX: this is a tmp implementation; a full implementation to consider the control flow while computing live_in
  regset_t live_in;
  regset_clear(&live_in);
  for (long i = 0; i < num_live_out; i++) {
    regset_union(&live_in, &live_out[i]);
  }
  for (long i = iseq_len - 1; i >= 0; i--) {
    etfg_insn_t const *etfg_insn;
    regset_t use, def;
    etfg_insn = &iseq[i];
#if ARCH_SRC == ARCH_ETFG
    src_insn_get_usedef_regs(etfg_insn, &use, &def);
#else
    NOT_REACHED();
#endif
    //cout << "i = " << i << ": use = " << regset_to_string(&use, as1, sizeof as1) << endl;
    //cout << "i = " << i << ": def = " << regset_to_string(&def, as1, sizeof as1) << endl;
    regset_diff(&live_in, &def);
    regset_union(&live_in, &use);
  }
  //cout << __func__ << " " << __LINE__ << ": iseq =\n" << etfg_iseq_to_string(iseq, iseq_len, as1, sizeof as1) << endl;
  //cout << "live_in = " << regset_to_string(&live_in, as1, sizeof as1) << endl;
  return live_in;
}

void
fallback_etfg_translate(etfg_insn_t const *src_iseq, long src_iseq_len, long num_nextpcs, dst_insn_t *dst_iseq, long *dst_iseq_len, long dst_iseq_size, regmap_t const &src_regmap, transmap_t *fb_tmap, regset_t *live_outs, char const *src_filename/*, regset_t &flexible_regs*/, fixed_reg_mapping_t &fixed_reg_mapping)
{
#define MAX_BINCODE_SIZE 128
  fixed_reg_mapping = fixed_reg_mapping_t::default_dst_fixed_reg_mapping_for_fallback_translations();

  //regset_clear(&flexible_regs);
  string x86_filename = string(src_filename) + ".x86";
  string custom_x86_filename = string(src_filename) + ".custom.i386";
  uint8_t *bincode = new uint8_t[MAX_BINCODE_SIZE];
  size_t bincode_len = input_exec_get_bincode_for_function(custom_x86_filename, src_iseq[0].m_comment, bincode, MAX_BINCODE_SIZE);
  if (!bincode_len) {
    bincode_len = input_exec_get_bincode_for_function(x86_filename, src_iseq[0].m_comment, bincode, MAX_BINCODE_SIZE);
  }
  if (!bincode_len) {
    cout << __func__ << " " << __LINE__ << ": could not fallback translate " << src_iseq[0].m_comment << "\n";
  }
  ASSERT(bincode_len);
  //cout << __func__ << " " << __LINE__ << ": " << src_iseq[0].m_comment << ":";
  for (size_t i = 0; i < bincode_len; i++) {
    cout << " " << std::hex << int(bincode[i]) << std::dec;
  }
  nextpc_t dst_nextpcs[2];
  long dst_num_nextpcs;
  regmap_t dst_regmap;

  *dst_iseq_len = dst_iseq_disassemble(dst_iseq, bincode, bincode_len, NULL, dst_nextpcs, &dst_num_nextpcs, etfg_get_i386_as_mode());

  dst_iseq_convert_peeptab_mem_constants_to_symbols(dst_iseq, *dst_iseq_len);

  if (!etfg_iseq_is_ret_insn(src_iseq, src_iseq_len)) {
    *dst_iseq_len = dst_iseq_trim_out_ret_insn(dst_iseq, *dst_iseq_len);
    dst_iseq_decrement_nextpc_nums(dst_iseq, *dst_iseq_len);
  }
/*#if ARCH_DST == ARCH_I386
  //cout << __func__ << " " << __LINE__ << ":\n";
  if (dst_iseq_contains_function_call(dst_iseq, *dst_iseq_len)) {
    ASSERT(*dst_iseq_len < dst_iseq_size);
    *dst_iseq_len += i386_insn_put_xchg_reg_with_reg(R_EAX, R_EDX, &dst_iseq[*dst_iseq_len], dst_iseq_size - *dst_iseq_len);
    ASSERT(*dst_iseq_len < dst_iseq_size);
    *dst_iseq_len += i386_insn_put_xchg_reg_with_reg(R_EDX, R_EAX, &dst_iseq[*dst_iseq_len], dst_iseq_size - *dst_iseq_len);
    ASSERT(*dst_iseq_len < dst_iseq_size);
  }
  //cout << __func__ << " " << __LINE__ << ":\n";
#endif*/

  //cout << __func__ << " " << __LINE__ << ": before dst_iseq_add_save_restore(): dst_iseq = " << dst_iseq_to_string(dst_iseq, *dst_iseq_len, as1, sizeof as1) << endl;

  dst_iseq_add_save_restore_insns_for_overwritten_regparams(dst_iseq, dst_iseq_len, dst_iseq_size, fixed_reg_mapping);
  //cout << __func__ << " " << __LINE__ << ": after dst_iseq_add_save_restore(): dst_iseq = " << dst_iseq_to_string(dst_iseq, *dst_iseq_len, as1, sizeof as1) << endl;

  long nextpc_num;
  if (etfg_iseq_points_to_nextpc(src_iseq, src_iseq_len, nextpc_num)) {
    *dst_iseq_len = dst_iseq_append_jump_to_nextpc(dst_iseq, *dst_iseq_len, dst_iseq_size, nextpc_num);
  }

  fixed_reg_mapping_intersect_with_dst_iseq_touch(fixed_reg_mapping, dst_iseq, *dst_iseq_len);
  //cout << __func__ << " " << __LINE__ << ": after disassemble: dst_iseq =\n" << dst_iseq_to_string(dst_iseq, *dst_iseq_len, as1, sizeof as1) << endl;

  dst_strict_canonicalization_state_t *scanon_state = dst_strict_canonicalization_state_init();
  dst_iseq_strict_canonicalize_regs(scanon_state, dst_iseq, *dst_iseq_len, &dst_regmap);
  dst_strict_canonicalization_state_free(scanon_state);
  //cout << __func__ << " " << __LINE__ << ": after strict canon: dst_iseq =\n" << dst_iseq_to_string(dst_iseq, *dst_iseq_len, as1, sizeof as1) << endl;
  //cout << __func__ << " " << __LINE__ << ": dst_regmap =\n" << regmap_to_string(&dst_regmap, as1, sizeof as1) << endl;
  auto fixed_reg_mapping_new = fixed_reg_mapping.fixed_reg_mapping_rename_using_regmap(dst_regmap);
  fixed_reg_mapping = fixed_reg_mapping_new;
  dst_iseq_inv_rename_imms_using_etfg_canonical_constants(dst_iseq, *dst_iseq_len);


  //cout << __func__ << " " << __LINE__ << ": after inv rename imms: dst_iseq =\n" << dst_iseq_to_string(dst_iseq, *dst_iseq_len, as1, sizeof as1) << endl;
  //cout << __func__ << " " << __LINE__ << ": dst_regmap = " << regmap_to_string(&dst_regmap, as1, sizeof as1) << endl;

  //cout << __func__ << " " << __LINE__ << ": src_regmap = " << regmap_to_string(&src_regmap, as1, sizeof as1) << endl;
  transmap_copy(fb_tmap, transmap_fallback());
  //initialize fb_tmap using calling conventions and dst_regmap
  for (size_t i = 0; i < DST_FASTCALL_NUM_REGPARAMS_EXT; i++) {
    reg_identifier_t src_reg_id = regmap_get_reg_id_for_argnum(src_regmap, i);
    if (src_reg_id.reg_identifier_is_unassigned()) {
      continue;
    }
    reg_identifier_t reg_id = regmap_rename_exreg(&dst_regmap, DST_EXREG_GROUP_GPRS, DST_FASTCALL_REGPARAM_EXT(i));
    //cout << __func__ << " " << __LINE__ << ": inv_rename_exreg on " << DST_FASTCALL_REGPARAM_EXT(i) << " returned <" << reg_id.reg_identifier_to_string() << ">\n";
    if (   !reg_id.reg_identifier_is_unassigned()
        && !dst_iseq_contains_function_call(dst_iseq, *dst_iseq_len)) {
      //exreg_id_t reg_id = p.second.reg_identifier_get_id();
      //fb_tmap->extable[DST_EXREG_GROUP_GPRS][src_reg_id].num_locs = 1;
      //fb_tmap->extable[DST_EXREG_GROUP_GPRS][src_reg_id].loc[0] = TMAP_LOC_EXREG(DST_EXREG_GROUP_GPRS);
      //fb_tmap->extable[DST_EXREG_GROUP_GPRS][src_reg_id].regnum[0] = reg_id.reg_identifier_get_id();
      fb_tmap->extable[DST_EXREG_GROUP_GPRS][src_reg_id].set_to_exreg_loc(TMAP_LOC_EXREG(DST_EXREG_GROUP_GPRS), reg_id.reg_identifier_get_id(), TMAP_LOC_MODIFIER_DST_SIZED);
    }
  }
  for (size_t i = 0; i < DST_FASTCALL_NUM_RETREGS; i++) {
    reg_identifier_t src_reg_id = regmap_get_reg_id_for_retreg_num(src_regmap, i);
    //cout << __func__ << " " << __LINE__ << ": retreg idx " << i << ": src_reg_id = " << src_reg_id.reg_identifier_to_string() << endl;
    if (src_reg_id.reg_identifier_is_unassigned()) {
      continue;
    }
    //pair<bool, reg_identifier_t> p = regmap_inv_rename_exreg_try(&dst_regmap, DST_EXREG_GROUP_GPRS, DST_FASTCALL_RETREG(i));
    reg_identifier_t reg_id = regmap_rename_exreg(&dst_regmap, DST_EXREG_GROUP_GPRS, DST_FASTCALL_RETREG(i));
    //cout << __func__ << " " << __LINE__ << ": retreg idx " << i << ": p = <" << p.first << ", " << p.second.reg_identifier_to_string() << ">\n";
    if (!reg_id.reg_identifier_is_unassigned()) {
      //exreg_id_t reg_id = p.second.reg_identifier_get_id();
      //fb_tmap->extable[SRC_EXREG_GROUP_GPRS][src_reg_id].num_locs = 1;
      //fb_tmap->extable[SRC_EXREG_GROUP_GPRS][src_reg_id].loc[0] = TMAP_LOC_EXREG(DST_EXREG_GROUP_GPRS);
      //fb_tmap->extable[SRC_EXREG_GROUP_GPRS][src_reg_id].regnum[0] = reg_id.reg_identifier_get_id();
      fb_tmap->extable[SRC_EXREG_GROUP_GPRS][src_reg_id].set_to_exreg_loc(TMAP_LOC_EXREG(DST_EXREG_GROUP_GPRS), reg_id.reg_identifier_get_id(), TMAP_LOC_MODIFIER_SRC_SIZED);
    }
  }

  /*if (   src_iseq_len == 1
      && etfg_insn_is_function_return(&src_iseq[0])) {
    for (const auto &g : fb_tmap->extable) {
      for (const auto &r : g.second) {
        if (g.first == ETFG_EXREG_GROUP_GPRS) {
          regset_mark_ex_mask(&flexible_regs, ETFG_EXREG_GROUP_GPRS, r.first, ETFG_EXREG_LEN(ETFG_EXREG_GROUP_GPRS));
        }
      }
    }
  }

  if (   src_iseq_len == 1
      && src_iseq[0].m_comment.substr(0, 4) == "move") {
    for (const auto &g : fb_tmap->extable) {
      for (const auto &r : g.second) {
        regset_mark_ex_mask(&flexible_regs, g.first, r.first, ETFG_EXREG_LEN(ETFG_EXREG_GROUP_GPRS));
      }
    }
  }*/

  if (   src_iseq_len == 1
      && etfg_insn_is_function_return(&src_iseq[0])) {
    ASSERT(dst_insn_is_function_return(&dst_iseq[*dst_iseq_len - 1]));
    //fb_tmap->extable[ETFG_EXREG_GROUP_RETURN_REGS][0].num_locs = 1;
    //fb_tmap->extable[ETFG_EXREG_GROUP_RETURN_REGS][0].loc[0] = TMAP_LOC_SYMBOL;
    //fb_tmap->extable[ETFG_EXREG_GROUP_RETURN_REGS][0].regnum[0] = (dst_ulong)-1;
    for (size_t i = 0; i < regset_count_exregs(&dst_callee_save_regs, DST_EXREG_GROUP_GPRS); i++) {
      //fb_tmap->extable[ETFG_EXREG_GROUP_CALLEE_SAVE_REGS][i].num_locs = 1;
      //fb_tmap->extable[ETFG_EXREG_GROUP_CALLEE_SAVE_REGS][i].loc[0] = TMAP_LOC_EXREG(DST_EXREG_GROUP_GPRS);
#if ARCH_DST == ARCH_I386
      //fb_tmap->extable[ETFG_EXREG_GROUP_CALLEE_SAVE_REGS][i].regnum[0] = dst_iseq[*dst_iseq_len - 1].op[i].valtag.reg.val;
      fb_tmap->extable[ETFG_EXREG_GROUP_CALLEE_SAVE_REGS][i].set_to_exreg_loc(TMAP_LOC_EXREG(DST_EXREG_GROUP_GPRS), dst_iseq[*dst_iseq_len - 1].op[i].valtag.reg.val, TMAP_LOC_MODIFIER_SRC_SIZED);
#else
      NOT_IMPLEMENTED();
#endif
    }
  }
  if (   src_iseq_len == 2
      && etfg_insn_is_function_call(&src_iseq[0])) {
    if (src_regmap.regmap_extable.count(ETFG_EXREG_GROUP_GPRS)) {
      set<exreg_id_t> gpr_args;
      bool retreg_exists = false;
      bool callee_is_indirect = false;
      for (const auto &e : src_regmap.regmap_extable.at(ETFG_EXREG_GROUP_GPRS)) {
        int retreg_num = 0;
        if (   !reg_identifier_represents_retreg(e.second, retreg_num)
            && !reg_identifier_represents_callee(e.second)) {
          //regset_mark_ex_mask(&flexible_regs, ETFG_EXREG_GROUP_GPRS, e.first, ETFG_EXREG_LEN(ETFG_EXREG_GROUP_GPRS));
          //fb_tmap->extable[ETFG_EXREG_GROUP_GPRS][e.first].num_locs = 1;
          //fb_tmap->extable[ETFG_EXREG_GROUP_GPRS][e.first].loc[0] = TMAP_LOC_SYMBOL;
          //fb_tmap->extable[ETFG_EXREG_GROUP_GPRS][e.first].regnum[0] = PEEPTAB_MEM_CONSTANT(memconstant_num);
          //cout << __func__ << " " << __LINE__ << ": e.first = " << e.first.reg_identifier_to_string() << endl;
          //cout << __func__ << " " << __LINE__ << ": e.second = " << e.second.reg_identifier_to_string() << endl;
          exreg_id_t gpr_num = e.first.reg_identifier_get_id();
          gpr_args.insert(gpr_num);
        } else if (reg_identifier_represents_callee(e.second)) {
          callee_is_indirect = true;
        } else if (reg_identifier_represents_retreg(e.second, retreg_num)) {
          retreg_exists = true;
        }
      }

      if (callee_is_indirect) {
        for (const auto &e : src_regmap.regmap_extable.at(ETFG_EXREG_GROUP_GPRS)) {
          if (reg_identifier_represents_callee(e.second)) {
            if (retreg_exists) {
              cout << __func__ << " " << __LINE__ << ": setting etfg gpr 1 to fastcall param\n";
              fb_tmap->extable[ETFG_EXREG_GROUP_GPRS][1].set_to_exreg_loc(TMAP_LOC_EXREG(DST_EXREG_GROUP_GPRS), DST_FASTCALL_REGPARAM_EXT(0), TMAP_LOC_MODIFIER_DST_SIZED);
            } else {
              cout << __func__ << " " << __LINE__ << ": setting etfg gpr 0 to fastcall param\n";
              fb_tmap->extable[ETFG_EXREG_GROUP_GPRS][0].set_to_exreg_loc(TMAP_LOC_EXREG(DST_EXREG_GROUP_GPRS), DST_FASTCALL_REGPARAM_EXT(0), TMAP_LOC_MODIFIER_DST_SIZED);
            }
          }
        }
      }

      int memconstant_num = 0;
      for (const auto &gpr_num : gpr_args) {
        //fb_tmap->extable[ETFG_EXREG_GROUP_GPRS][gpr_num].set_to_symbol_loc(TMAP_LOC_SYMBOL, PEEPTAB_MEM_CONSTANT(memconstant_num));
        fb_tmap->extable[ETFG_EXREG_GROUP_GPRS][gpr_num].set_to_symbol_loc(TMAP_LOC_SYMBOL, memconstant_num);
        memconstant_num++;
      }
    }
  }
  if (   src_iseq_len == 2
      && src_iseq[0].m_comment.substr(0, 3) == "gep") {
    transmap_copy(fb_tmap, transmap_fallback());
    for (size_t i = 0; i < 3; i++) {
      //dst_ulong regnum = (i == 3) ? 0 : (i == 1) ? 2 : (i == 2) ? 1 : -1;
      dst_ulong regnum = (i == 0) ? 0 : (i == 1) ? 2 : (i == 2) ? 1 : -1;
      //fb_tmap->extable[ETFG_EXREG_GROUP_GPRS][i].num_locs = 1;
      //fb_tmap->extable[ETFG_EXREG_GROUP_GPRS][i].loc[0] = TMAP_LOC_EXREG(DST_EXREG_GROUP_GPRS);
      //fb_tmap->extable[ETFG_EXREG_GROUP_GPRS][i].regnum[0] = regnum;
      transmap_loc_modifier_t mod = (i == 3) ? TMAP_LOC_MODIFIER_SRC_SIZED : TMAP_LOC_MODIFIER_DST_SIZED;
      fb_tmap->extable[ETFG_EXREG_GROUP_GPRS][i].set_to_exreg_loc(TMAP_LOC_EXREG(DST_EXREG_GROUP_GPRS), regnum, mod);
    }
  }

  //cout << __func__ << " " << __LINE__ << ": fb_tmap =\n" << transmap_to_string(fb_tmap, as1, sizeof as1) << endl;
  //cout << __func__ << " " << __LINE__ << ": dst_iseq =\n" << dst_iseq_to_string(dst_iseq, *dst_iseq_len, as1, sizeof as1) << endl;

  delete[] bincode;
}

map<nextpc_id_t, string> const &
etfg_get_nextpc_map_at_pc(input_exec_t const *in, src_ulong pc)
{
  input_section_t const *isec = input_exec_get_text_section(in);
  map<string, tfg const *>  const *function_tfg_map = (map<string, tfg const *> const *)isec->data;
  ASSERT(function_tfg_map);
  string function_name;
  long insn_num_in_function;
  function_tfg_map_pc_get_function_name_and_insn_num(function_tfg_map, pc, function_name, insn_num_in_function);
  tfg const *t = function_tfg_map->at(function_name);
  return t->get_nextpc_map();
}

vector<string>
etfg_input_exec_get_arg_names_for_function(input_exec_t const *in, string const &function_name)
{
  input_section_t const *isec = input_exec_get_text_section(in);
  map<string, tfg const *>  const *function_tfg_map = (map<string, tfg const *> const *)isec->data;
  ASSERT(function_tfg_map);
  ASSERT(function_tfg_map->count(function_name));
  tfg const *t = function_tfg_map->at(function_name);
  vector<string> ret;
  for (const auto &e : t->get_argument_regs().get_map()) {
    ret.push_back(expr_string(e.second.get_val()));
  }
  return ret;
}

reg_identifier_t
etfg_insn_get_returned_reg(etfg_insn_t const &insn)
{
  if (!etfg_insn_is_function_return(&insn)) {
    return reg_identifier_t::reg_identifier_invalid();
  }
  ASSERT(insn.m_edges.size() == 1);
  etfg_insn_edge_t const &ie = insn.m_edges.at(0);
  shared_ptr<tfg_edge const> const &te = ie.get_tfg_edge();
  state const &to_state = te->get_to_state();
  for (const auto &kv : to_state.get_value_expr_map_ref()) {
    if (kv.first->get_str().find(G_LLVM_RETURN_REGISTER_NAME) != string::npos) {
      expr_ref const &retval = kv.second;
      ASSERT(expr_is_potential_llvm_reg_id(retval));
      return expr_to_reg_id(retval);
    }
  }
  return reg_identifier_t::reg_identifier_invalid();
}

//std::vector<std::pair<string_ref, size_t>> const &
graph_locals_map_t const&
etfg_input_exec_get_locals_map_for_function(input_exec_t const *in, string const &function_name)
{
  input_section_t const *isec = input_exec_get_text_section(in);
  map<string, tfg const *>  const *function_tfg_map = (map<string, tfg const *> const *)isec->data;
  ASSERT(function_tfg_map);
  ASSERT(function_tfg_map->count(function_name));
  tfg const *t = function_tfg_map->at(function_name);
  return t->get_locals_map();
}

void
etfg_insn_comment_get_function_name_and_insn_num(string const &comment, string &function_name, int &insn_num_in_function)
{
  size_t colon = comment.find(':');
  ASSERT(colon != string::npos);
  function_name = comment.substr(0, colon);
  insn_num_in_function = stoi(comment.substr(colon + 1));
}

static pc
pcrel_valtag_get_pc(pc const &from_pc, valtag_t const &pcrel)
{
  if (pcrel.tag == tag_var) {
    return pc(pc::exit, int_to_string(pcrel.val).c_str(), PC_SUBINDEX_START, PC_SUBSUBINDEX_DEFAULT);
  } else {
    ASSERT(stringIsInteger(from_pc.get_index()));
    int new_index = atoi(from_pc.get_index()) + 1 + pcrel.val;
    return pc(pc::insn_label, int_to_string(new_index).c_str(), PC_SUBINDEX_FIRST_INSN_IN_BB, PC_SUBSUBINDEX_DEFAULT);
  }
}

expr_ref
pcrel_valtag_to_expr(context *ctx, valtag_t const &pcrel)
{
  consts_struct_t const &cs = ctx->get_consts_struct();
  if (pcrel.tag == tag_var) {
    return cs.get_expr_value(reg_type_nextpc_const, pcrel.val);
  }
  NOT_IMPLEMENTED();
}

dshared_ptr<tfg_llvm_t>
src_iseq_get_preprocessed_tfg(etfg_insn_t const *etfg_iseq, long etfg_iseq_len, regset_t const *live_out, size_t num_live_out, map<pc, map<string_ref, expr_ref>> const &return_reg_map, context *ctx, sym_exec &se)
{
  regset_t live_in = etfg_iseq_compute_live_in(etfg_iseq, etfg_iseq_len, live_out, num_live_out);
  ASSERT(etfg_iseq_len >= 1);
  //tfg const *t = (tfg const *)etfg_iseq[0].etfg_insn_get_tfg();
  //state const &start_state = t->get_start_state();
  state const &start_state = *etfg_iseq[0].etfg_insn_get_start_state();
  //shared_ptr<state const> const &start_state = etfg_iseq[0].get_start_state();
  consts_struct_t &cs = ctx->get_consts_struct();
  dshared_ptr<tfg_llvm_t> tfg_ret = make_dshared<tfg_llvm_t>("etfg_iseq", DUMMY_FNAME, ctx);
  tfg_ret->set_start_state(start_state);
  dshared_ptr<tfg_node> start_node = make_dshared<tfg_node>(pc::start());
  tfg_ret->add_node(start_node);
  list<string> sorted_bbl_indices;
  string bbl_zero_index = "0";
  shared_ptr<tfg_edge const> start_edge = mk_tfg_edge(mk_itfg_edge(pc::start(), pc(pc::insn_label, bbl_zero_index.c_str(), PC_SUBINDEX_FIRST_INSN_IN_BB, PC_SUBSUBINDEX_DEFAULT), state(), expr_true(ctx)/*, start_state*/, unordered_set<expr_ref>(), te_comment_t::te_comment_start_pc_edge()));
  tfg_ret->add_edge(start_edge);
  sorted_bbl_indices.push_back(bbl_zero_index);

  for (long i = 0; i < etfg_iseq_len; i++) {
    etfg_insn_t const *etfg_insn = &etfg_iseq[i];
    string bbl_i_index = int_to_string(i);
    pc from_pc(pc::insn_label, bbl_i_index.c_str(), PC_SUBINDEX_FIRST_INSN_IN_BB, PC_SUBSUBINDEX_DEFAULT);
    sorted_bbl_indices.push_back(bbl_i_index);
    for (const auto &ei : etfg_insn->m_edges) {
      shared_ptr<tfg_edge const> const &e = ei.get_tfg_edge();
      if (tfg_ret->find_node(from_pc) == 0) {
        dshared_ptr<tfg_node> from_node = make_dshared<tfg_node>(from_pc);
        tfg_ret->add_node(from_node);
      }

      vector<valtag_t> const &pcrels = ei.get_pcrels();
      map<expr_id_t, pair<expr_ref, expr_ref>> submap;
      //map<string_ref, expr_ref> name_submap;
      expr_ref nextpc = e->get_function_call_nextpc();
      int cur_pcrel_index = 0;
      if (nextpc && nextpc->is_nextpc_const()) {
        expr_ref cur_pcrel_nextpc = pcrel_valtag_to_expr(ctx, pcrels.at(cur_pcrel_index));
        submap[nextpc->get_id()] = make_pair(nextpc, cur_pcrel_nextpc);
        //name_submap[nextpc->get_name()] = cur_pcrel_nextpc;
        cur_pcrel_index++;
      }
      pc to_pc = e->get_to_pc();
      //cout << __func__ << " " << __LINE__ << ": from_pc = " << from_pc.to_string() << endl;
      if (pcrels.size() > cur_pcrel_index) {
        to_pc = pcrel_valtag_get_pc(from_pc, pcrels.at(cur_pcrel_index));
        cur_pcrel_index++;
        //cout << __func__ << " " << __LINE__ << ": to_pc = " << to_pc.to_string() << endl;
      } else if (!ei.etfg_insn_edge_is_indirect_branch()) {
        valtag_t vt = {.val = 0, .tag = tag_const};
        to_pc = pcrel_valtag_get_pc(from_pc, vt);
        cur_pcrel_index++;
      }
      //cout << __func__ << " " << __LINE__ << ": to_pc = " << to_pc.to_string() << endl;
      if (tfg_ret->find_node(to_pc) == 0) {
        dshared_ptr<tfg_node> to_node = make_dshared<tfg_node>(to_pc);
        tfg_ret->add_node(to_node);
      }

      state edge_to_state(e->get_to_state());
      expr_ref edgecond = e->get_cond();
      edge_to_state.substitute_variables_using_state_submap(/*start_state, */state_submap_t(submap));
      edgecond = ctx->expr_substitute(edgecond, submap);

      unordered_set<expr_ref> assumes;
      shared_ptr<tfg_edge const> new_e = mk_tfg_edge(mk_itfg_edge(from_pc, to_pc, edge_to_state, edgecond/*, start_state*/, unordered_set<expr_ref>(), e->get_te_comment()));
      tfg_ret->add_edge(new_e);
    }
    //for (const auto &ep : etfg_insn->m_preconditions) {
    //  predicate_ref p = ep/*.pred_substitute_using_submap(ctx, submap)*/;
    //  tfg_ret->add_assume_pred(from_pc, p);
    //}
  }
  DYN_DEBUG(ISEQ_GET_TFG, cout << __func__ << " " << __LINE__ << ": tfg_ret = \n" << tfg_ret->graph_to_string() << endl);
  pc pc_cur_start(pc::insn_label, pc::start().get_index(), pc::start().get_subindex() + PC_SUBINDEX_FIRST_INSN_IN_BB, pc::start().get_subsubindex());
  tfg_ret->add_extra_node_at_start_pc(pc_cur_start);
  tfg_ret->tfg_populate_arguments_using_live_regset(live_in);
  DYN_DEBUG(ISEQ_GET_TFG, cout << __func__ << " " << __LINE__ << ": after populate_arguments, tfg_ret = \n" << tfg_ret->graph_to_string() << endl);
  //tfg_ret->populate_exit_return_values_using_live_regset(live_out, num_live_out);
  tfg_ret->set_return_regs(return_reg_map);
  tfg_ret->tfg_llvm_set_sorted_bbl_indices(sorted_bbl_indices);

  //vector<memlabel_ref> relevant_memlabels_ref;
  //tfg_ret->graph_get_relevant_memlabels(relevant_memlabels_ref);
  //vector<memlabel_t> relevant_memlabels;
  //for (auto const& mlr : relevant_memlabels_ref)
  //  relevant_memlabels.push_back(mlr->get_ml());
  //cs.solver_set_relevant_memlabels(relevant_memlabels_ref);

  //memlabel_assertions_t mlasserts(tfg_ret->get_symbol_map(), tfg_ret->get_locals_map(), ctx, true);
  //ctx->set_memlabel_assertions(mlasserts);

  //tfg_ret->collapse_tfg(true);
  tfg_ret->tfg_preprocess(false/*, dshared_ptr<tfg_llvm_t const>::dshared_nullptr()*/);
  //tfg_ret->tfg_populate_relevant_memlabels(dshared_ptr<tfg_llvm_t const>::dshared_nullptr());
  ftmap_t::tfg_run_pointsto_analysis(tfg_ssa_t::tfg_ssa_construct_from_non_ssa_tfg(tfg_ret, dshared_ptr<tfg const>::dshared_nullptr())/*, callee_rw_memlabels_t::callee_rw_memlabels_nop()*/, false, dshared_ptr<tfg_llvm_t const>::dshared_nullptr());
  //tfg_ret->tfg_postprocess_after_pointsto_analysis(false);
  DYN_DEBUG(eqgen, cout << __func__ << " " << __LINE__ << ": TFG:\n" << tfg_ret->graph_to_string(/*cs*/) << endl);
  return tfg_ret;
}

string
etfg_insn_edges_to_string(etfg_insn_t const &insn)
{
  string ret;
  for (const auto &ie : insn.m_edges) {
    ret += ie.get_tfg_edge()->to_string_concise() + ", ";
  }
  return ret;
}

size_t
etfg_insn_get_size_for_exreg(etfg_insn_t const *insn, exreg_group_id_t group, reg_identifier_t const &r)
{
  string regname = r.reg_identifier_to_string();
  if (regname.substr(0, strlen(G_SRC_KEYWORD)) != G_SRC_KEYWORD) {
    return ETFG_EXREG_LEN(group);
  }
  for (const auto &ie : insn->m_edges) {
    shared_ptr<tfg_edge const> const &e = ie.get_tfg_edge();
    size_t sz;
    if (e->edge_get_size_for_regname(regname, sz)) {
      return sz;
    }
  }
  cout << __func__ << " " << __LINE__ << ": group = " << group << ", reg = " << r.reg_identifier_to_string() << endl;
  cout << __func__ << " " << __LINE__ << ": insn = " << etfg_insn_to_string(insn, as1, sizeof as1) << endl;
  NOT_REACHED();
}

size_t
etfg_insn_put_invalid(etfg_insn_t *insn)
{
  //NOT_IMPLEMENTED();
  return 1;
}

static void
etfg_ret_insn_replace_return_value_with_constant(etfg_insn_t *insn, int retval)
{
  ASSERT(etfg_insn_is_function_return(insn));
  context *ctx = insn->get_context();
  etfg_insn_edge_t &ie = insn->m_edges.at(0);
  //shared_ptr<tfg_edge const> &te = ie.get_tfg_edge();
  dshared_ptr<state const> const &start_state = insn->etfg_insn_get_start_state();
  //state &to_state = te->get_to_state();

  std::function<void (pc const&, state&)> f = [ctx, retval, &start_state](pc const& p, state& st)
  {
    ostringstream oss1, oss2;
    oss1 << G_SRC_KEYWORD ".exreg." << ETFG_EXREG_GROUP_RETURN_REGS << ".0";
    expr_ref e;
    if(st.has_expr(oss1.str()))
      e = st.get_expr(oss1.str()/*, *start_state*/);
    else
      e = start_state->get_expr(oss1.str());
    map<expr_id_t, pair<expr_ref, expr_ref>> submap;
    expr_ref bvconst = ctx->mk_bv_const(ETFG_EXREG_LEN(ETFG_EXREG_GROUP_GPRS), retval);
    oss2 << G_SRC_KEYWORD ".exreg." << ETFG_EXREG_GROUP_GPRS << ".0";
    expr_ref reg0 = start_state->get_expr(oss2.str()/*, *start_state*/);
    submap[reg0->get_id()] = make_pair(reg0, bvconst);
    e = ctx->expr_substitute(e, submap);
    st.set_expr_in_map(oss1.str(), e);
  };
  //ASSERT(ie.etfg_insn_edge_is_atomic());
  ie.etfg_insn_edge_update_state(f);
}

size_t
etfg_insn_put_ret_insn(etfg_insn_t *insns, size_t insns_size, int retval)
{
  char const *etfg_ret_insn_str =
"=etfg_insn ret {{{\n"
"=tfg_edge: L0%1%0 => E0%0%1\n"
"=tfg_edge.EdgeCond:\n"
"1 : 1 : BOOL\n"
"=tfg_edge.StateTo:\n"
"=src.exreg.2.0\n"
"1 : input.src.exreg.0.0 : BV:32\n"
"=src.exreg.3.0\n"
"1 : input.src.exreg.3.0 : BV:32\n"
"2 : input.src.exreg.1.0 : BV:32\n"
"3 : bvxor(1, 2) : BV:32\n"
"4 : input.src.exreg.1.1 : BV:32\n"
"5 : bvxor(3, 4) : BV:32\n"
"6 : input.src.exreg.1.2 : BV:32\n"
"7 : bvxor(5, 6) : BV:32\n"
"8 : input.src.exreg.1.3 : BV:32\n"
"9 : bvxor(7, 8) : BV:32\n"
"=src." LLVM_STATE_INDIR_TARGET_KEY_ID "\n"
"1 : retaddr_const : BV:32\n"
"=state_end\n"
"=tfg_edge.Assumes.begin:\n"
"=tfg_edge.Assumes.end\n"
"=tfg_edge.te_comment\n"
"0:-1:ret\n"
"tfg_edge_comment end\n"
"=comment\n"
"retR0:1\n"
"}}}\n";

  long ret = src_iseq_from_string(NULL, insns, insns_size, etfg_ret_insn_str, etfg_get_i386_as_mode());
  ASSERT(ret <= insns_size);
  ASSERT(ret == 1);
  etfg_ret_insn_replace_return_value_with_constant(&insns[0], retval);
  return ret;
}

regset_t
etfg_get_retregs_at_pc(input_exec_t const *in, src_ulong pc_long)
{
  input_section_t const *isec = input_exec_get_text_section(in);
  map<string, tfg const *>  const *function_tfg_map = (map<string, tfg const *>  const *)isec->data;
  unsigned fid = pc_long / ETFG_MAX_INSNS_PER_FUNCTION;
  pair<string, tfg const *> p = function_tfg_map_get_function_name_and_tfg_from_fid(function_tfg_map, fid);
  tfg const *t = p.second;
  pc retpc(pc::exit, "0", PC_SUBINDEX_START, PC_SUBSUBINDEX_DEFAULT);
  map<string_ref, expr_ref> const &retregs = t->get_return_regs_at_pc(retpc);
  regset_t ret;
  regset_clear(&ret);
  for (const auto &retreg : retregs) {
    string const &name = retreg.first->get_str();
    expr_ref e = retreg.second;
    if (name.substr(0, 4) != "exvr") {
      continue;
    }
    size_t dot;
    exreg_group_id_t group = stol(name.substr(4), &dot);
    dot += 4;
    ASSERT(name.at(dot) == '.');
    int nbits = ETFG_EXREG_LEN(group);
    if (   e->get_operation_kind() == expr::OP_BVEXTRACT
        && e->get_args().at(0)->is_var()
        && e->get_args().at(2)->get_int_value() == 0) {
      nbits = e->get_args().at(1)->get_int_value() + 1;
      e = e->get_args().at(0);
    } else if (!e->is_var()) {
      continue;
    }
    exreg_id_t regnum = stol(name.substr(dot + 1));
    regset_mark_ex_mask(&ret, group, regnum, MAKE_MASK(nbits));
  }
  return ret;
}

void
etfg_insn_rename_constants(etfg_insn_t *insn, imm_vt_map_t const *imm_vt_map)
{
  context *ctx = insn->get_context();
  regmap_t regmap;
  map<expr_id_t, pair<expr_ref, expr_ref>> submap = get_submap_for_regmap_and_imm_map(ctx, &regmap, imm_vt_map);
  insn->etfg_insn_substitute_using_submap(submap);
}

static bool
etfg_iseq_all_start_states_are_equal(etfg_insn_t const *iseq, long iseq_len)
{
  if (iseq_len == 0) {
    return true;
  }
  ASSERT(iseq_len > 0);
  dshared_ptr<state const> const &start_state = iseq[0].etfg_insn_get_start_state();
  for (long i = 1; i < iseq_len; i++) {
    if (iseq[i].etfg_insn_get_start_state() != start_state) {
      return false;
    }
  }
  return true;
}

/*void
etfg_iseq_truncate_register_sizes_using_touched_regs(etfg_insn_t *iseq, size_t iseq_len)
{
#if ARCH_SRC == ARCH_ETFG
  regset_t touched;
  src_iseq_compute_touch_var(&touched, NULL, NULL, NULL, iseq, iseq_len);
  ASSERT(etfg_iseq_all_start_states_are_equal(iseq, iseq_len));
  ASSERT(iseq_len > 0);
  state start_state = *iseq[0].etfg_insn_get_start_state();
  start_state.state_truncate_register_sizes_using_regset(touched);
  for (size_t i = 0; i < iseq_len; i++) {
    iseq[i].etfg_insn_truncate_register_sizes_using_regset(touched);
  }
#else
  NOT_REACHED();
#endif
}

void
etfg_insn_t::etfg_insn_truncate_register_sizes_using_regset(regset_t const &regset)
{
  for (auto &e : m_edges) {
    e.etfg_insn_edge_truncate_register_sizes_using_regset(regset);
  }
}

void
etfg_insn_edge_t::etfg_insn_edge_truncate_register_sizes_using_regset(regset_t const &regset)
{
  m_tfg_edge->tfg_edge_truncate_register_sizes_using_regset(regset);
}*/

long
etfg_insn_t::insn_hash_func_after_canon(long hash_size) const
{
  return 0;
}

bool
etfg_insn_t::insn_matches_template(etfg_insn_t const &templ, src_iseq_match_renamings_t &src_iseq_match_renamings/*regmap_t &regmap, imm_vt_map_t &imm_vt_map, nextpc_map_t &nextpc_map*/, bool var) const
{
  autostop_timer func_timer(string("etfg.") + __func__);
  vector<src_iseq_match_renamings_t> renamings;
  src_iseq_match_renamings_t r;
  r.clear();
  renamings.push_back(r);
  //regmap_init(&regmap);
  //imm_vt_map_init(&imm_vt_map);
  //nextpc_map_init(&nextpc_map);
  ASSERT(var);
  bool ret = etfg_insn_matches_template(&templ, this, renamings, "insn_matches_template", var);
  if (!ret) {
    ASSERT(renamings.size() == 0);
    return false;
  }
  ASSERT(renamings.size() > 0);
  src_iseq_match_renamings = renamings.at(0);
  return true;
}

static map<expr_id_t, pair<expr_ref, expr_ref>>
state_get_submap(state_t const &state, context *ctx)
{
  map<expr_id_t, pair<expr_ref, expr_ref>> ret = get_submap_for_regmap_and_imm_map(ctx, NULL, &state.imm_vt_map);
  for (size_t g = 0; g < SRC_NUM_EXREG_GROUPS; g++) {
    for (size_t r = 0; r < SRC_NUM_EXREGS(g); r++) {
      ostringstream oss;
      oss << G_INPUT_KEYWORD "." G_SRC_KEYWORD "." G_REGULAR_REG_NAME << g << "." << r;
      string regname = oss.str();
      expr_ref reg = ctx->mk_var(regname, ctx->mk_bv_sort(ETFG_EXREG_LEN(g)));
      expr_ref val = ctx->mk_bv_const(ETFG_EXREG_LEN(g), state.exregs[g][r].state_reg_to_int64());
      ret[reg->get_id()] = make_pair(reg, val);
    }
  }
  return ret;
}

static uint64_t
state_load_from_memory(state_t const &istate, uint64_t addr_const, int nbytes)
{
  ASSERT(nbytes <= QWORD_LEN/BYTE_LEN);
  uint64_t ret = 0;
  for (int i = 0; i < nbytes; i++) {
    uint8_t b = ((uint8_t *)istate.memory)[(addr_const + i) & MAKE_MASK(MEMORY_SIZE_BITS)];
    ret |= (uint64_t)b << (i * BYTE_LEN);
  }
  return ret;
}

static void
state_store_to_memory(state_t &istate, uint64_t addr_const, uint64_t data_const, int nbytes)
{
  ASSERT(nbytes <= QWORD_LEN/BYTE_LEN);
  for (int i = 0; i < nbytes; i++) {
    ((uint8_t *)istate.memory)[(addr_const + i) & MAKE_MASK(MEMORY_SIZE_BITS)] = (data_const >> (i * BYTE_LEN)) & MAKE_MASK(MEMORY_SIZE_BITS);
  }
}

static expr_ref
expr_substitute_using_state(expr_ref const &e, state const &st, state_t const &istate)
{
  context *ctx = e->get_context();
  set<state::mem_access> mas;
  state::find_all_memory_exprs_for_all_ops("", e, mas);
  map<expr_id_t, pair<expr_ref, expr_ref>> submap = state_get_submap(istate, ctx);
  for (const auto &ma : mas) {
    ASSERT(ma.is_select());
    expr_ref const &addr = ma.get_address();
    uint64_t addr_const = expr_substitute_using_state(addr, st, istate)->get_int_value();
    int nbytes = ma.get_nbytes();
    uint64_t val = state_load_from_memory(istate, addr_const, nbytes);
    expr_ref const &m_expr = ma.get_expr();
    submap[m_expr->get_id()] = make_pair(m_expr, ctx->mk_bv_const(m_expr->get_sort()->get_size(), val));
  }
  return ctx->expr_do_simplify(ctx->expr_substitute(e, submap));
}

static bool
expr_bool_evaluate_on_state(expr_ref const &e, state const &st, state_t const &istate)
{
  ASSERT(e->is_bool_sort());
  expr_ref e_const = expr_substitute_using_state(e, st, istate);
  ASSERT(e_const->is_bool_sort());
  return e_const->get_bool_value();
}

static uint64_t 
expr_bv_evaluate_on_state(expr_ref const &e, state const &st, state_t const &istate)
{
  ASSERT(e->is_bv_sort());
  ASSERT(e->get_sort()->get_size() <= QWORD_LEN);
  expr_ref e_const = expr_substitute_using_state(e, st, istate);
  if (!e_const->is_const()) {
    cout << __func__ << " " << __LINE__ << ": e_const = " << expr_string(e_const) << endl;
  }
  ASSERT(e_const->is_const());
  ASSERT(e_const->get_sort() == e->get_sort());
  return e_const->get_int_value();
}

static bool
predicate_evaluates_to_false_on_state(predicate_ref const &pred, state const &st, state_t const &istate)
{
  if (!pred->get_precond().precond_is_trivial()) {
    return false;
  }
  context *ctx = pred->get_lhs_expr()->get_context();
  expr_ref e = ctx->mk_eq(pred->get_lhs_expr(), pred->get_rhs_expr());
  return !expr_bool_evaluate_on_state(e, st, istate);
}

static bool
etfg_insn_triggers_undef_behaviour_on_state(etfg_insn_t const *insn, state_t const &istate)
{
  //cout << __func__ << " " << __LINE__ << ": insn =\n" << etfg_insn_to_string(insn, as1, sizeof as1) << endl;
  //cout << __func__ << " " << __LINE__ << ": istate:\n" << state_to_string(&istate, as1, sizeof as1) << endl;
  context *ctx = insn->get_context();
  state const &st = *insn->etfg_insn_get_start_state();
  for (const auto &pred : insn->m_preconditions) {
    if (predicate_evaluates_to_false_on_state(pred, st, istate)) {
      return true;
    }
  }
  return false;
}

static void
state_evaluate_on_state(state const &st, state const &start_state, state_t &istate)
{
  for (const auto &kv : st.get_value_expr_map_ref()) {
    string const &k = kv.first->get_str();
    expr_ref const &e = kv.second;
    exreg_group_id_t group;
    reg_identifier_t reg_id = reg_identifier_t::reg_identifier_invalid();
    if (state::key_represents_exreg_key(k, group, reg_id)) {
      exreg_id_t regnum = reg_id.reg_identifier_get_id();
      uint64_t econst = expr_bv_evaluate_on_state(e, start_state, istate);
      istate.exregs[group][regnum].state_reg_from_int64(econst);
    } else if (e->is_array_sort()) {
      set<state::mem_access> mas;
      state::find_all_memory_exprs_for_all_ops("", e, mas);
      for (const auto &ma : mas) {
        ASSERT(ma.is_store());
        expr_ref const &addr = ma.get_address();
        expr_ref const &data = ma.get_store_data();
        int nbytes = ma.get_nbytes();
        uint64_t addr_const = expr_bv_evaluate_on_state(addr, start_state, istate);
        uint64_t data_const = expr_bv_evaluate_on_state(data, start_state, istate);
        state_store_to_memory(istate, addr_const, data_const, nbytes);
      }
    }
  }
}

static void
etfg_insn_execute_on_state(etfg_insn_t const *insn, valtag_t &cur_index, state_t &istate)
{
  context *ctx = insn->get_context();
  consts_struct_t const &cs = ctx->get_consts_struct();
  state const &start_state = *insn->etfg_insn_get_start_state();
  for (const auto &ie : insn->m_edges) {
    expr_ref const &econd = ie.get_tfg_edge()->get_cond();
    state const &to_state = ie.get_tfg_edge()->get_to_state();
    bool econd_const = expr_bool_evaluate_on_state(econd, to_state, istate);
    if (econd_const) {
      state_evaluate_on_state(to_state, start_state, istate);
      vector<valtag_t> const &pcrels = ie.get_pcrels();
      if (pcrels.size() > 0) {
        valtag_t const &next = pcrels.at(pcrels.size() - 1);
        ASSERT(cur_index.tag == tag_const);
        if (next.tag == tag_const) {
          cur_index.val += next.val + 1;
        } else if (next.tag == tag_var) {
          valtag_copy(&cur_index, &next);
          istate.nextpc_num = cur_index.val;
        } else NOT_REACHED();
      } else {
        cur_index.val++;
      }
      return;
    }
  }
  NOT_REACHED();
}

bool
etfg_iseq_triggers_undef_behaviour_on_state(etfg_insn_t const *src_iseq, long src_iseq_len, state_t const &state)
{
  valtag_t cur_index;
  state_t istate;

  cur_index.tag = tag_const;
  cur_index.val = 0;
  state_init(&istate);
  state_copy(&istate, &state);
  //state_t istate = state;
  while (cur_index.tag == tag_const) {
    ASSERT(cur_index.val >= 0 && cur_index.val < src_iseq_len);
    if (etfg_insn_triggers_undef_behaviour_on_state(&src_iseq[cur_index.val], istate)) {
      return true;
    }
    etfg_insn_execute_on_state(&src_iseq[cur_index.val], cur_index, istate);
  }
  state_free(&istate);
  return false;
}

bool
etfg_insn_edge_t::etfg_insn_edge_stores_top_level_op_in_reg(char const* prefix, exreg_group_id_t g, reg_identifier_t const &r, vector<expr::operation_kind> const &ops/*, state const &start_state*/) const
{
  state const &st = m_tfg_edge->get_to_state();
  expr_ref e = st.state_get_expr_value(/*start_state,*/ prefix, reg_type_exvreg, g, r.reg_identifier_get_id());
  if (!e) {
    return false;
  }
  context *ctx = e->get_context();
  e = ctx->mk_eq(ctx->mk_bvextract(e, 0, 0), ctx->mk_bv_const(1, 1));
  e = ctx->expr_do_simplify(e);
  //cout << __func__ << " " << __LINE__ << ": e = " << expr_string(e) << endl;
  for (const auto &op : ops) {
    if (e->get_operation_kind() == op) {
      return true;
    }
  }
  return false;
}

bool
etfg_insn_t::etfg_insn_stores_top_level_op_in_reg(char const* prefix, exreg_group_id_t g, reg_identifier_t const &r,
    vector<expr::operation_kind> const &ops) const
{
  for (const auto &e : m_edges) {
    if (!e.etfg_insn_edge_stores_top_level_op_in_reg(prefix, g, r, ops/*, *m_start_state*/)) {
      return false;
    }
  }
  return true;
}

static bool
etfg_iseq_stores_top_level_op_in_reg(etfg_insn_t const *iseq, long iseq_len, char const* prefix, exreg_group_id_t g, reg_identifier_t const &r, vector<expr::operation_kind> const &ops)
{
  for (size_t i = 0; i < iseq_len; i++) {
    if (iseq[i].etfg_insn_stores_top_level_op_in_reg(prefix, g, r, ops)) {
      return true;
    }
  }
  return false;
}

bool
etfg_iseq_stores_eq_comparison_in_reg(etfg_insn_t const *iseq, long iseq_len, char const* prefix, exreg_group_id_t g, reg_identifier_t const &r)
{
  return etfg_iseq_stores_top_level_op_in_reg(iseq, iseq_len, prefix, g, r, {expr::OP_EQ, expr::OP_NOT/*XXX: need to introduce OP_NE*/});
}

bool
etfg_iseq_stores_ult_comparison_in_reg(etfg_insn_t const *iseq, long iseq_len, char const* prefix, exreg_group_id_t g, reg_identifier_t const &r)
{
  return etfg_iseq_stores_top_level_op_in_reg(iseq, iseq_len, prefix, g, r, {expr::OP_BVULT, expr::OP_BVUGE});
}

bool
etfg_iseq_stores_slt_comparison_in_reg(etfg_insn_t const *iseq, long iseq_len, char const* prefix, exreg_group_id_t g, reg_identifier_t const &r)
{
  return etfg_iseq_stores_top_level_op_in_reg(iseq, iseq_len, prefix, g, r, {expr::OP_BVSLT, expr::OP_BVSGE});
}

bool
etfg_iseq_stores_ugt_comparison_in_reg(etfg_insn_t const *iseq, long iseq_len, char const* prefix, exreg_group_id_t g, reg_identifier_t const &r)
{
  return etfg_iseq_stores_top_level_op_in_reg(iseq, iseq_len, prefix, g, r, {expr::OP_BVUGT, expr::OP_BVULE});
}

bool
etfg_iseq_stores_sgt_comparison_in_reg(etfg_insn_t const *iseq, long iseq_len, char const* prefix, exreg_group_id_t g, reg_identifier_t const &r)
{
  return etfg_iseq_stores_top_level_op_in_reg(iseq, iseq_len, prefix, g, r, {expr::OP_BVSGT, expr::OP_BVSLE});
}

bool
etfg_insn_t::etfg_insn_involves_cbranch() const
{
  return m_edges.size() > 1;
}

bool
etfg_iseq_involves_cbranch(etfg_insn_t const *iseq, long iseq_len)
{
  for (long i = 0; i < iseq_len; i++) {
    if (iseq[i].etfg_insn_involves_cbranch()) {
      return true;
    }
  }
  return false;
}

static void
etfg_insn_fallback_list_remove_unnecessary_state_elements(etfg_insn_t *insn)
{
  std::function<void (pc const&, state&)> f = [](pc const&, state &s)
  {
    set<string> remove_keys;
    std::function<void (const string&, expr_ref)> f = [/*&target_regs, */&remove_keys](const string& key, expr_ref e) -> void
    {
      if (   is_llvm_target_reg(key)
          || is_llvm_ret_reg(key)
          || is_llvm_hidden_reg(key)
          || (is_llvm_gep_reg(key) && !key_represents_intermediate_value(key))) {
        //do nothing
        //target_regs[make_pair(exreg_group_id, r.reg_identifier_get_id())] = key;
      } else if (   !e->is_array_sort()
                 && !is_llvm_indir_target(key)) {
        remove_keys.insert(key);
      }
    };
    s.state_visit_exprs(f);
    for (auto rk : remove_keys) {
      s.remove_key(rk);
    }
  };

  for (auto &ie : insn->m_edges) {
    //shared_ptr<tfg_edge const> &e = ie.get_tfg_edge();
    //state &s = e->get_to_state();
    ie.etfg_insn_edge_update_state(f);
  }
}

bool
etfg_accept_insn_check_in_fallback_list(etfg_insn_t *insn)
{
  etfg_insn_fallback_list_remove_unnecessary_state_elements(insn);
  string comment = insn->m_comment;
  string function_name;
  int insn_num_in_function;
  etfg_insn_comment_get_function_name_and_insn_num(comment, function_name, insn_num_in_function);
  if (function_name == "main") {
    return insn_num_in_function == 0;
  }
  if (/*   function_name.substr(0, 5) == "call_"
      || */function_name.substr(0, 3) == "gep") {
    return insn_num_in_function == 2;
  }
  if (function_name.substr(0, 4) == "exit") {
    return insn_num_in_function == 3;
  }
  if (function_name.substr(0, 5) == "call_") {
    return etfg_insn_is_function_call(insn);
  }
  return insn_num_in_function == 1;
}

vector<translation_unit_t>
etfg_input_exec_get_translation_units(input_exec_t const  *in)
{
  input_section_t const *isec = input_exec_get_text_section(in);
  map<string, tfg const *>  const *function_tfg_map = (map<string, tfg const *>  const *)isec->data;
  vector<translation_unit_t> ret;
  for (auto ft : *function_tfg_map) {
    translation_unit_t tunit(ft.first);
    ret.push_back(tunit);
  }
  return ret;
}

static bool
function_tfg_map_pc_is_incident_on_phi_edge(map<string, tfg const *> const &function_tfg_map, src_ulong pc)
{
  bool fetch;
  struct etfg_insn_t insn;
  size_t size;
  fetch = function_tfg_map_fetch_raw(&insn, &function_tfg_map, pc, &size);
  ASSERT(fetch);
  if (insn.m_edges.size() > 1) {
    return true;
  }
  ASSERT(insn.m_edges.size() == 1);
  etfg_insn_edge_t const &e = insn.m_edges.at(0);
  shared_ptr<tfg_edge const> const &te = e.get_tfg_edge();
  if (   !pc::subsubindex_is_phi_node(te->get_from_pc().get_subsubindex())
      && !pc::subsubindex_is_phi_node(te->get_to_pc().get_subsubindex())
     ) {
    return false;
  } else {
    return true;
  }
}

bool
etfg_input_exec_edge_represents_phi_edge(input_exec_t const *in, src_ulong from_pc, src_ulong to_pc)
{
#if ARCH_SRC == ARCH_ETFG
  unsigned from_fid = from_pc / ETFG_MAX_INSNS_PER_FUNCTION;
  unsigned to_fid = to_pc / ETFG_MAX_INSNS_PER_FUNCTION;
  if (from_fid != to_fid) {
    return false;
  }
  return    set_belongs(in->get_phi_edge_incident_pcs(), from_pc)
         || set_belongs(in->get_phi_edge_incident_pcs(), to_pc);
#else
  NOT_REACHED();
#endif
}

bool
etfg_input_exec_pc_incident_on_phi_edge(input_exec_t const *in, src_ulong pc)
{
#if ARCH_SRC == ARCH_ETFG
  input_section_t const *isec = input_exec_get_text_section(in);
  map<string, tfg const *>  const *function_tfg_map = (map<string, tfg const *>  const *)isec->data;
  unsigned fid = pc / ETFG_MAX_INSNS_PER_FUNCTION;
  unsigned insn_num = pc - (fid * ETFG_MAX_INSNS_PER_FUNCTION);
  return function_tfg_map_pc_is_incident_on_phi_edge(*function_tfg_map, pc);
#else
  NOT_REACHED();
#endif
}

bool
etfg_iseq_is_function_tailcall_opt_candidate(etfg_insn_t const *iseq, long iseq_len)
{
  if (   !etfg_insn_is_function_call(&iseq[0])
      || !etfg_insn_is_function_return(&iseq[2])) {
    return false;
  }
  if (iseq[0].m_edges.size() > 1) {
    return false;
  }
  if (iseq[2].m_edges.size() > 1) {
    return false;
  }
  shared_ptr<tfg_edge const> const &fcall_edge = iseq[0].m_edges.at(0).get_tfg_edge();
  shared_ptr<tfg_edge const> const &ret_edge = iseq[2].m_edges.at(0).get_tfg_edge();
  state const &fcall_to_state = fcall_edge->get_to_state();
  state const &ret_to_state = ret_edge->get_to_state();
  map<string_ref, expr_ref> const &value_expr_map = ret_to_state.get_value_expr_map_ref();
  if (   value_expr_map.count(mk_string_ref(G_SRC_KEYWORD "." G_LLVM_RETURN_REGISTER_NAME)) == 0
      && value_expr_map.count(mk_string_ref(G_LLVM_RETURN_REGISTER_NAME)) == 0) {
    //cout << __func__ << " " << __LINE__ << ": returning true for\n" << etfg_iseq_to_string(iseq, iseq_len, as1, sizeof as1) << endl;
    return true;
  }
  expr_ref const &retval = value_expr_map.count(mk_string_ref(G_LLVM_RETURN_REGISTER_NAME)) ? value_expr_map.at(mk_string_ref(G_LLVM_RETURN_REGISTER_NAME)) : value_expr_map.at(mk_string_ref(G_SRC_KEYWORD "." G_LLVM_RETURN_REGISTER_NAME));
  if (!retval->is_var()) {
    return false;
  }
  dshared_ptr<state const> const &start_state = iseq[0].etfg_insn_get_start_state();
  map<string_ref, expr_ref> const &fcall_value_expr_map = fcall_to_state.get_value_expr_map_ref();
  for (const auto &p : fcall_value_expr_map) {
    if (p.second->is_bv_sort()) {
      //bool ret = (retval == start_state->get_value_expr_map_ref().at(p.first));
      context* ctx = p.second->get_context();
      bool ret = (retval == ctx->get_input_expr_for_key(p.first, retval->get_sort()));
      //cout << __func__ << " " << __LINE__ << ": returning " << ret << " for\n" << etfg_iseq_to_string(iseq, iseq_len, as1, sizeof as1) << endl;
      return ret;
    }
  }
  return false;
}

char *
etfg_iseq_with_relocs_to_string(etfg_insn_t const *iseq, long iseq_len,
    char const *reloc_strings, size_t reloc_strings_size,
    i386_as_mode_t mode, char *buf, size_t buf_size)
{
  NOT_IMPLEMENTED();
}

void
etfg_insn_convert_pcrels_and_symbols_to_reloc_strings(etfg_insn_t *src_insn, std::map<long, std::string> const& insn_labels, nextpc_t const* nextpcs, long num_nextpcs, input_exec_t const* in, src_ulong const* insn_pcs, int insn_index, std::set<symbol_id_t>& syms_used, char **reloc_strings, long *reloc_strings_size)
{
  NOT_IMPLEMENTED();
}

regmap_t const *
etfg_identity_regmap()
{
  NOT_IMPLEMENTED();
}

void
etfg_iseq_rename(etfg_insn_t *iseq, long iseq_len,
    struct regmap_t const *dst_regmap,
    struct imm_vt_map_t const *imm_map,
    memslot_map_t const *memslot_map,
    struct regmap_t const *src_regmap,
    struct nextpc_map_t const *nextpc_map,
    nextpc_t const *nextpcs, long num_nextpcs)
{
  NOT_IMPLEMENTED();
}

void
etfg_insn_t::serialize(ostream& os) const
{
  //uses serialization is false
}

void
etfg_insn_t::deserialize(istream& is)
{
  //uses serialization is false
}

//bool
//etfg_insn_edge_t::etfg_insn_edge_is_indirect_branch() const
//{
//  return get_tfg_edge()->edge_is_indirect_branch();
//}

string
etfg_insn_t::etfg_insn_get_short_llvm_description() const
{
  ASSERT(this->m_edges.size() > 0);
  for (etfg_insn_edge_t const& ie : this->m_edges) {
    string const& str = ie.etfg_insn_edge_get_short_llvm_description();
    if (str != "") {
      return str;
    }
  }
  return "";
}

string
etfg_insn_edge_t::etfg_insn_edge_get_short_llvm_description() const
{
  ASSERT(m_tfg_edge);
  te_comment_t const& te_comment = m_tfg_edge->get_te_comment();
  string ret = te_comment.get_code()->get_str();
  return ret;
}

bool
etfg_insn_t::etfg_insn_represents_start_or_bbl_entry_edge() const
{
  ASSERT(m_edges.size() > 0);
  if (m_edges.size() != 1) {
    return false;
  }
  etfg_insn_edge_t const& ie = m_edges.at(0);
  shared_ptr<tfg_edge const> const& te = ie.get_tfg_edge();
  pc const& from_pc = te->get_from_pc();
  return from_pc.is_start() || from_pc.get_subsubindex() == PC_SUBSUBINDEX_BASIC_BLOCK_ENTRY;
}

size_t
etfg_input_file_find_pc_for_insn_index_ignoring_start_and_bbl_start_pcs(input_exec_t const* in, size_t orig_insn_index)
{
  long num_seen = 0;
  src_ulong cur = 0;
  size_t cur_insn_index = orig_insn_index;
  while (cur_insn_index != 0) {
    struct etfg_insn_t insn;
    unsigned long size;
    //cout << __func__ << " " << __LINE__ << ": cur_insn_index = " << cur_insn_index << ", cur = " << cur << endl;
    bool fetch = etfg_insn_fetch_raw(&insn, in, cur, &size);
    ASSERT(fetch);
    if (!insn.etfg_insn_represents_start_or_bbl_entry_edge()) {
      cur_insn_index--;
    }
    cur++;
    ASSERT(num_seen++ < 2 * orig_insn_index);
  }
  return cur;
}

map<string, dshared_ptr<tfg>>
get_function_tfg_map_from_etfg_file(string const &etfg_filename, context* ctx)
{
  consts_struct_t const &cs = ctx->get_consts_struct();
  map<string, dshared_ptr<tfg>> ret;
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
    getline(in, line);
    if(!is_line(line, TFG_IDENTIFIER)) {
      cout << __func__ << " " << __LINE__ << ": parsing failed" << endl;
      NOT_REACHED();
    }
    //tfg *tfg_llvm = NULL;
    //auto pr = read_tfg(in, &tfg_llvm, "llvm", ctx, true);
    size_t start = strlen(TFG_LLVM_IDENTIFIER " ");
    size_t colon = line.find(':');
    ASSERT(colon != string::npos);
    string name = line.substr(start, colon - start);
    ASSERT(string_has_suffix(name, function_name));
    cout << _FNLN_ << ": reading " << name << " from " << etfg_filename << endl;

    dshared_ptr<tfg_llvm_t> tfg_llvm = tfg_llvm_t::tfg_llvm_from_stream(in, name, ctx);
    //dshared_ptr<tfg_llvm_t> tfg_llvm = make_dshared<tfg_llvm_t>(in, name, ctx);
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

static src_ulong
get_entry_pc_from_function_tfg_map(map<string, dshared_ptr<tfg>> const &function_tfg_map)
{
  src_ulong cur_pc = 0;
  for (const auto &f : function_tfg_map) {
    //cout << __func__ << " " << __LINE__ << ": f.first = " << f.first << endl;
    if (f.first == "main") {
      return cur_pc;
    }
    cur_pc += ETFG_MAX_INSNS_PER_FUNCTION;
  }
  cout << __func__ << " " << __LINE__ << ": Error: main function not found!\n";
  NOT_REACHED();
}


static Elf32_Word
function_tfg_map_compute_code_section_size(map<string, dshared_ptr<tfg>> const &function_tfg_map)
{
  return function_tfg_map.size() * ETFG_MAX_INSNS_PER_FUNCTION;
}

static symbol_id_t
symbol_map_find_unused_key(graph_symbol_map_t const &symbol_map, symbol_id_t prev_symbol_id)
{
  for (symbol_id_t i = prev_symbol_id;; i++) {
    if (!symbol_map.count(i)) {
      return i;
    }
  }
  NOT_REACHED();
}



static map<symbol_id_t, symbol_t>
etfg_read_symbols(input_exec_t *in, map<string, dshared_ptr<tfg>> const &function_tfg_map)
{
  graph_symbol_map_t cumulative_symbol_map;
  map<pair<symbol_id_t, offset_t>, vector<char>> cumulative_string_contents;
  map<symbol_id_t, dst_ulong> symbol_id_addr_map;
  for (auto f : function_tfg_map) {
    tfg const &t = *f.second;
    cumulative_symbol_map.symbol_map_union(t.get_symbol_map());
    map_union(cumulative_string_contents, t.get_string_contents());
  }
  dst_ulong cur_addr = ETFG_SYMBOLS_START_ADDR;
  //symbol_t *ret = (symbol_t *)malloc(cumulative_symbol_map.size() * sizeof(symbol_t));
  //ASSERT(ret);
  map<symbol_id_t, symbol_t> ret;
  map<string, symbol_id_t> symbol_name_to_id_map;
  //symbol_t *ptr = ret;
  for (auto s : cumulative_symbol_map.get_map()) {
    symbol_t sym;
    strncpy(sym.name, s.second.get_name()->get_str().c_str(), sizeof sym.name);
    sym.value = cur_addr;
    sym.size = s.second.get_size();
    sym.bind = STB_GLOBAL;
    sym.type = STT_NOTYPE;
    sym.shndx = SHN_UNDEF; //will be populated later
    sym.other = STV_DEFAULT;
    //cout << __func__ << " " << __LINE__ << ": symbol-id " << s.first << ": addr " << hex << cur_addr << dec << endl;
    symbol_id_addr_map[s.first] = cur_addr;
    cur_addr += sym.size;
    ret.insert(make_pair(s.first, sym));
    symbol_name_to_id_map[sym.name] = s.first;
    //ptr++;
  }

  unsigned fid = 0;
  symbol_id_t prev_symbol_id = ETFG_SYMBOL_ID_FUNCTION_START;
  for (const auto &f : function_tfg_map) {
    symbol_t sym;
    strncpy(sym.name, f.first.c_str(), sizeof sym.name);
    //cout << __func__ << " " << __LINE__ << ": adding symbol for function name " << sym.name << endl;
    sym.value = fid * ETFG_MAX_INSNS_PER_FUNCTION;
    sym.size = 0;
    sym.bind = STB_GLOBAL;
    sym.type = STT_FUNC;
    sym.shndx = SHN_UNDEF; //will be populated later
    sym.other = STV_DEFAULT;
    symbol_id_t symbol_id;
    if (symbol_name_to_id_map.count(sym.name)) { //if the symbol exists already, just change its value to the new value (it had a data value earlier). This can happen fofunction pointers used in the code
      symbol_id = symbol_name_to_id_map.at(sym.name);
      ret.erase(symbol_id);
    } else {
      symbol_id = symbol_map_find_unused_key(cumulative_symbol_map, prev_symbol_id + 1);
      prev_symbol_id = symbol_id;
    }
    ret.insert(make_pair(symbol_id, sym));
    fid++;
  }

  //*num_symbols = ptr - ret;
  input_section_t *isec = &in->input_sections[in->num_input_sections];
  in->num_input_sections++;

  isec->orig_index = 0;
  isec->name = strdup(MYDATA_SECTION_NAME);
  ASSERT(isec->name);
  isec->addr = ETFG_SYMBOLS_START_ADDR;
  isec->type = ETFG_SECTION_TYPE;
  isec->flags = ETFG_SYMBOLS_SECTION_FLAGS;
  isec->size = cur_addr - ETFG_SYMBOLS_START_ADDR;
  isec->info = ETFG_SECTION_INFO;
  isec->align = ETFG_SECTION_ALIGN;
  isec->entry_size = ETFG_SECTION_ENTRY_SIZE;
  isec->data = (char *)malloc(isec->size);
  ASSERT(isec->data);
  memset(isec->data, 0, isec->size);
  for (auto s : cumulative_string_contents) {
    pair<symbol_id_t, offset_t> sid = s.first;
    dst_ulong addr = symbol_id_addr_map.at(sid.first);
    char *ptr = isec->data + (addr - ETFG_SYMBOLS_START_ADDR);
    //char const *str = s.second.c_str();
    //memcpy(ptr, str, strlen(str) + 1);
    for (size_t i = 0; i < s.second.size(); i++) {
      ptr[i] = s.second.at(i);
    }
  }

  return ret;
}



void
etfg_read_input_file(input_exec_t *in, map<string, dshared_ptr<tfg>> const &function_tfg_map)
{
  autostop_timer func_timer(__func__);
  input_exec_init(in);
  in->filename = strdup("etfg");

  in->num_input_sections = 0;
  in->num_plt_sections = 0;
  in->has_dynamic_section = false;
  //in->num_dynsyms = 0;
  in->num_dyn_entries = 0;
  //in->dynsyms = NULL;
  in->dyn_entries = NULL;
  in->num_relocs = 0;
  in->relocs = NULL;

  in->type = INPUT_EXEC_TYPE_ETFG;
  in->entry = get_entry_pc_from_function_tfg_map(function_tfg_map);

  ASSERT(in->num_input_sections == 0);
  input_section_t *isec = &in->input_sections[in->num_input_sections];
  in->num_input_sections++;

  isec->orig_index = 0;
  isec->name = strdup(".text");
  ASSERT(isec->name);
  isec->addr = ETFG_SECTION_START_ADDR;
  isec->type = ETFG_SECTION_TYPE;
  isec->flags = ETFG_TEXT_SECTION_FLAGS;
  isec->size = function_tfg_map_compute_code_section_size(function_tfg_map);
  isec->info = ETFG_SECTION_INFO;
  isec->align = ETFG_SECTION_ALIGN;
  isec->entry_size = ETFG_SECTION_ENTRY_SIZE;
  isec->data = (char *)(&function_tfg_map);

  in->symbol_map = etfg_read_symbols(in, function_tfg_map);
}

void
etfg_input_file_add_symbols_for_nextpcs(input_exec_t *in, size_t const* nextpc_idx, size_t num_live_out)
{
  autostop_timer func_timer(__func__);
  //jumptable_init(in);
  size_t symbol_id = ETFG_SYMBOL_ID_NEXTPC_INDEX_START;
  for (size_t i = 0; i < num_live_out; i++) {
    size_t nextpc_code_index = nextpc_idx[i]; //nextpc_code_indices.at(i);
    stringstream ss;
    ss << ETFG_SYMBOL_NEXTPC_PREFIX << i;
    symbol_t sym;
    strncpy(sym.name, ss.str().c_str(), sizeof sym.name);
    //cout << __func__ << " " << __LINE__ << ": adding symbol for function name " << sym.name << endl;
    sym.value = /*start + */nextpc_code_index;
    sym.size = 0;
    sym.bind = STB_GLOBAL;
    sym.type = STT_NOTYPE;
    sym.shndx = SHN_UNDEF; //will be populated later
    sym.other = STV_DEFAULT;
    in->symbol_map.insert(make_pair(symbol_id, sym));
    symbol_id++;
    //jumptable_add(in->jumptable, sym.value);
  }
}


