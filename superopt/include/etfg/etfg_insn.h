#ifndef ETFG_INSN_H
#define ETFG_INSN_H

#include <stdlib.h>
#include <set>
#include <vector>
#include "support/types.h"
#include "tfg/tfg.h"
#include "tfg/tfg_llvm.h"
#include "etfg/etfg_regs.h"
#include "support/src_tag.h"
#include "valtag/fixed_reg_mapping.h"
#include "rewrite/translation_unit.h"
#include "exec/state.h"
class src_iseq_match_renamings_t;

#define ETFG_MAX_NUM_OPERANDS 6

typedef struct etfg_insn_edge_t {
  etfg_insn_edge_t(etfg_insn_edge_t const &other)
  : m_tfg_edge(other.m_tfg_edge),
    m_pcrels(other.m_pcrels)
  {
  }
  etfg_insn_edge_t(dshared_ptr<tfg_edge const> const &e, vector<valtag_t> const &pcrels)
  : m_tfg_edge(e.get_shared_ptr()),
    m_pcrels(pcrels)
  {
  }
  shared_ptr<tfg_edge const> const &get_tfg_edge() const { return m_tfg_edge; }
  shared_ptr<tfg_edge const> &get_tfg_edge() { return m_tfg_edge; }
  string etfg_insn_edge_to_string(context *ctx) const;
  //bool etfg_insn_edge_is_atomic() const { return m_tfg_edge->tfg_edge_is_atomic(); }
  valtag_t const &etfg_insn_edge_get_first_pcrel() const
  {
    ASSERT(m_pcrels.size() > 0);
    return m_pcrels.at(0);
  }
  valtag_t const &etfg_insn_edge_get_last_pcrel() const
  {
    ASSERT(m_pcrels.size() > 0);
    return m_pcrels.at(m_pcrels.size() - 1);
  }
  void etfg_insn_edge_substitute_using_submap(context *ctx/*, state const &start_state*/, map<expr_id_t, pair<expr_ref, expr_ref>> const &submap)
  {
    std::function<expr_ref (pc const&, expr_ref const&)> f = [ctx, &submap](pc const& p, expr_ref const& e)
    {
      return ctx->expr_substitute(e, submap);
    };
    this->etfg_insn_edge_update_edgecond(f);
    std::function<void (pc const&, state&)> fs = [/* &start_state, */&submap](pc const& p, state &s)
    {
      s.substitute_variables_using_state_submap(/*start_state, */state_submap_t(submap));
    };
    this->etfg_insn_edge_update_state(fs);
    //m_tfg_edge->substitute_variables_at_input(start_state, submap, ctx);
  }
  void etfg_insn_edge_update_state(std::function<void (pc const&, state&)> f);
  void etfg_insn_edge_update_edgecond(std::function<expr_ref (pc const&, expr_ref const&)> f);
  void etfg_insn_edge_set_expr_at_operand_index(context *ctx, long op_index, expr_ref const& val);
  vector<valtag_t> const &get_pcrels() const { return m_pcrels; }
  void set_pcrels(vector<valtag_t> const &pcrels);
  bool etfg_insn_edge_is_indirect_branch_not_fcall() const;
  bool etfg_insn_edge_is_indirect_function_call() const;
  void etfg_insn_edge_identify_fcall_nextpcs_as_pcrels(vector<valtag_t> &pcrels) const;
  bool etfg_insn_edge_set_fcall_nextpcs_as_pcrels(context *ctx, valtag_t const &new_pcrel);
  void etfg_insn_edge_truncate_register_sizes_using_regset(regset_t const &regset);
  bool etfg_insn_edge_stores_top_level_op_in_reg(char const* prefix, exreg_group_id_t g, reg_identifier_t const &r,
    vector<expr::operation_kind> const &ops/*, state const &start_state*/) const;
  bool etfg_insn_edge_is_indirect_branch() const;
  string etfg_insn_edge_get_short_llvm_description() const;
private:
  tfg_edge_ref m_tfg_edge;
  vector<valtag_t> m_pcrels;
} etfg_insn_edge_t;

typedef struct etfg_insn_t {
  //bool m_valid;
  etfg_insn_t() : m_ctx(NULL) {}
  list<predicate_ref> m_preconditions; //this needs to be an ordered list for functions of type  get_expr_at_operand_index()
  std::vector<etfg_insn_edge_t> m_edges;
  string m_comment;

  static bool uses_serialization() { return false; }
  void serialize(ostream& os) const;
  void deserialize(istream& is);
  //state const &get_start_state() const { return m_start_state; }
  context *get_context() const { return m_ctx; }
  void etfg_insn_copy_from(etfg_insn_t const &other)
  {
    m_preconditions = other.m_preconditions;
    m_edges.clear();
    for (const auto &e : other.m_edges) {
      etfg_insn_edge_t new_e(e);
      m_edges.push_back(new_e);
    }
    m_comment = other.m_comment;
    //m_tfg = other.m_tfg;
    m_ctx = other.m_ctx;
    m_start_state = other.m_start_state;
  }
  void etfg_insn_substitute_using_submap(map<expr_id_t, pair<expr_ref, expr_ref>> const &submap)
  {
    for (predicate_ref &precond : m_preconditions) {
      precond = precond->pred_substitute_using_submap(m_ctx, submap);
    }
    for (etfg_insn_edge_t &ie : m_edges) {
      ie.etfg_insn_edge_substitute_using_submap(m_ctx/*, *m_start_state*/, submap);
    }
  }
  void etfg_insn_set_tfg(tfg const *t)
  {
    //m_tfg = (const void *)t;
    m_ctx = t->get_context();
    m_start_state = make_dshared<state const>(t->get_start_state().get_state());
  }
  /*const void *etfg_insn_get_tfg() const
  {
    return m_tfg;
  }*/
  dshared_ptr<state const> const &etfg_insn_get_start_state() const
  {
    return m_start_state;
  }
  void etfg_insn_truncate_register_sizes_using_regset(regset_t const &regset);
  long insn_hash_func_after_canon(long hash_size) const;
  bool insn_matches_template(etfg_insn_t const &templ, src_iseq_match_renamings_t &src_iseq_match_renamings/*regmap_t &regmap, imm_vt_map_t &imm_vt_map, nextpc_map_t &nextpc_map*/, bool var) const;
  void clear()
  {
    //m_tfg = NULL;
    m_edges.clear();
    m_preconditions.clear();
    m_start_state.clear();
    m_ctx = NULL;
  }
  bool etfg_insn_stores_top_level_op_in_reg(char const* prefix, exreg_group_id_t g, reg_identifier_t const &r,
    vector<expr::operation_kind> const &ops) const;
  bool etfg_insn_involves_cbranch() const;
  string etfg_insn_get_short_llvm_description() const;
  bool etfg_insn_represents_start_or_bbl_entry_edge() const;
private:
  context *m_ctx;
  dshared_ptr<state const> m_start_state;
  //const void *m_tfg; //at some point, investigate if we can get rid of this field; instead copy the preconditions, etc. directly to this struct etfg_insn_t
} etfg_insn_t;

struct regmap_t;
struct imm_vt_map_t;
struct nextpc_map_t;
struct input_exec_t;
struct valtag_t;
struct bbl_t;
struct regcons_t;
struct translation_instance_t;
struct vconstants_t;
struct etfg_insn_t;
class fixed_reg_mapping_t;

typedef struct etfg_strict_canonicalization_state_t
{
  etfg_strict_canonicalization_state_t(tfg *t) : m_tfg(t) {}
  ~etfg_strict_canonicalization_state_t() { delete m_tfg; }
  tfg *get_tfg() { return m_tfg; }
private:
  tfg *m_tfg;
} etfg_strict_canonicalization_state_t;




long etfg_operand_strict_canonicalize_regs(etfg_strict_canonicalization_state_t *scanon_state,
    long insn_index, long op_index,
    struct etfg_insn_t *iseq_var[], long iseq_var_len[], struct regmap_t *st_regmap,
    struct imm_vt_map_t *st_imm_map,
    fixed_reg_mapping_t const &fixed_reg_mapping,
    long num_iseq_var, long max_num_iseq_var);
long etfg_operand_strict_canonicalize_imms(etfg_strict_canonicalization_state_t *scanon_state,
    long insn_index,
    long op_index,
    struct etfg_insn_t *iseq_var[], long iseq_var_len[], struct regmap_t *st_regmap,
    struct imm_vt_map_t *st_imm_map,
    fixed_reg_mapping_t const &fixed_reg_mapping,
    long num_iseq_var, long max_num_iseq_var);

char *etfg_exregname_suffix(int groupnum, int exregnum, char const *suffix, char *buf, size_t size);

char *etfg_insn_to_string(etfg_insn_t const *insn, char *buf, size_t buf_size);
char *etfg_iseq_to_string(etfg_insn_t const *iseq, long iseq_len,
    char *buf, size_t buf_size);
std::string etfg_insn_edges_to_string(etfg_insn_t const &insn);


size_t etfg_insn_to_int_array(etfg_insn_t const *insn, int64_t arr[], size_t len);
void etfg_insn_rename_addresses_to_symbols(struct etfg_insn_t *insn,
    struct input_exec_t const *in, src_ulong pc);
void etfg_insn_copy_symbols(struct etfg_insn_t *dst, struct etfg_insn_t const *src);
bool etfg_insns_equal(struct etfg_insn_t const *i1, struct etfg_insn_t const *i2);
size_t etfg_insn_get_symbols(struct etfg_insn_t const *insn, long const *start_symbol_ids, long *symbol_ids, size_t max_symbol_ids);
bool etfg_iseq_matches_template_var(etfg_insn_t const *_template, etfg_insn_t const *iseq,
    long len,
    src_iseq_match_renamings_t &src_iseq_match_renamings,
    //struct regmap_t *st_regmap, struct imm_vt_map_t *st_imm_vt_map,
    //struct nextpc_map_t *nextpc_map,
    char const *prefix);
void etfg_insn_copy(struct etfg_insn_t *dst, struct etfg_insn_t const *src);
unsigned long etfg_insn_hash_func(etfg_insn_t const *insn);
bool etfg_insn_fetch_raw(struct etfg_insn_t *insn, struct input_exec_t const *in,
    src_ulong pc, unsigned long *size);
bool etfg_insn_merits_optimization(etfg_insn_t const *insn);
bool etfg_insn_is_nop(etfg_insn_t const *insn);
void etfg_insn_canonicalize(etfg_insn_t *insn);
void etfg_insn_canonicalize_locals_symbols(struct etfg_insn_t *etfg_insn,
    struct imm_vt_map_t *imm_vt_map, src_tag_t tag);
void etfg_insn_fetch_preprocess(etfg_insn_t *insn, struct input_exec_t const *in,
    src_ulong pc, unsigned long size);
bool etfg_iseq_supports_execution_test(etfg_insn_t const *insns, long n);
std::vector<nextpc_t> etfg_insn_branch_targets(etfg_insn_t const *insn, src_ulong nip);
nextpc_t etfg_insn_fcall_target(etfg_insn_t const *insn, src_ulong nip, struct input_exec_t const *in);
long etfg_insn_put_jump_to_pcrel(nextpc_t const& target, etfg_insn_t *insns,
    size_t insns_size);
vector<valtag_t> etfg_insn_get_pcrel_values(struct etfg_insn_t const *insn);

bool etfg_insn_is_function_return(etfg_insn_t const *insn);
bool etfg_insn_is_indirect_branch(etfg_insn_t const *insn);
bool etfg_insn_is_indirect_branch_not_fcall(etfg_insn_t const *insn);
bool etfg_insn_is_conditional_indirect_branch(etfg_insn_t const *insn);
bool etfg_insn_is_function_call(etfg_insn_t const *insn);
std::vector<std::string> etfg_insn_get_fcall_argnames(etfg_insn_t const *insn);
bool etfg_insn_is_direct_function_call(etfg_insn_t const *insn);
bool etfg_insn_is_indirect_function_call(etfg_insn_t const *insn);
src_ulong etfg_insn_branch_target_var(etfg_insn_t const *insn);

bool etfg_insn_is_branch(struct etfg_insn_t const *insn);
bool etfg_insn_is_branch_not_fcall(etfg_insn_t const *insn);
bool etfg_insn_is_unconditional_indirect_branch_not_fcall(etfg_insn_t const *insn);
bool etfg_insn_is_direct_branch(etfg_insn_t const *insn);
bool etfg_insn_is_conditional_direct_branch(etfg_insn_t const *insn);
bool etfg_insn_is_unconditional_direct_branch(etfg_insn_t const *insn);
bool etfg_insn_is_unconditional_direct_branch_not_fcall(etfg_insn_t const *insn);
bool etfg_insn_is_unconditional_branch(etfg_insn_t const *insn);
bool etfg_insn_is_unconditional_branch_not_fcall(etfg_insn_t const *insn);

void etfg_insn_set_pcrel_values(struct etfg_insn_t *insn, std::vector<valtag_t> const &vs);

long etfg_insn_put_jump_to_nextpc(etfg_insn_t *insns, size_t insns_size, long nextpc_num);

void etfg_iseq_mark_used_vconstants(struct imm_vt_map_t *map, struct imm_vt_map_t *nomem_map,
    struct imm_vt_map_t *mem_map, etfg_insn_t const *iseq, long iseq_len);
void etfg_insn_get_min_max_vconst_masks(int *min_mask, int *max_mask,
    etfg_insn_t const *src_insn);
void etfg_insn_get_min_max_mem_vconst_masks(int *min_mask, int *max_mask,
    etfg_insn_t const *src_insn);
vector<src_iseq_match_renamings_t> etfg_iseq_matches_template(
    etfg_insn_t const *_template,
    etfg_insn_t const *seq, long len,
    //src_iseq_match_renamings_t &src_iseq_match_renamings,
    //struct regmap_t *st_regmap,
    //struct imm_vt_map_t *st_imm_map, struct nextpc_map_t *nextpc_map,
    string prefix);

long etfg_iseq_window_convert_pcrels_to_nextpcs(etfg_insn_t *etfg_iseq,
    long etfg_iseq_len, long start, long *nextpc2src_pcrel_vals,
    src_tag_t *nextpc2src_pcrel_tags);
bool etfg_iseq_execution_test_implemented(etfg_insn_t const *etfg_iseq,
    long etfg_iseq_len);
bool etfg_insn_supports_execution_test(etfg_insn_t const *insn);
bool etfg_insn_supports_boolean_test(etfg_insn_t const *insn);

bool etfg_insn_less(etfg_insn_t const *a, etfg_insn_t const *b);

bool symbol_is_contained_in_etfg_insn(struct etfg_insn_t const *insn, long symval,
    src_tag_t symtag);

void etfg_insn_hash_canonicalize(struct etfg_insn_t *insn, struct regmap_t *regmap,
    struct imm_vt_map_t *imm_map);
int etfg_insn_disassemble(etfg_insn_t *insn, uint8_t const *bincode, i386_as_mode_t i386_as_mode);
//long etfg_operand_strict_canonicalize_regs(long insn_index, long op_index,
//    struct etfg_insn_t *iseq_var[], long iseq_var_len[], struct regmap_t *st_regmap,
//    struct imm_vt_map_t *st_imm_map, long num_iseq_var, long max_num_iseq_var/*,
//    bool canonicalize_imms_using_syms, struct input_exec_t *in*/);

bool etfg_insn_fetch_raw(struct etfg_insn_t *insn, struct input_exec_t const *in,
    src_ulong pc, unsigned long *size);

void etfg_insn_add_to_pcrels(etfg_insn_t *insn, src_ulong pc, struct bbl_t const *bbl, long bblindex);
struct valtag_t *etfg_insn_get_pcrel_operands(etfg_insn_t const *insn);
void etfg_init(void);
bool etfg_infer_regcons_from_assembly(char const *assembly,
    struct regcons_t *cons);
ssize_t etfg_assemble(struct translation_instance_t *ti, uint8_t *bincode, size_t bincode_size,
    char const *assembly, i386_as_mode_t mode);
void etfg_insn_inv_rename_registers(struct etfg_insn_t *insn,
    struct regmap_t const *regmap);
bool etfg_insn_inv_rename_memory_operands(struct etfg_insn_t *insn,
    struct regmap_t const *regmap, struct vconstants_t const *vconstants);
void etfg_insn_inv_rename_constants(struct etfg_insn_t *insn,
    struct vconstants_t const *vconstants);
bool etfg_insn_terminates_bbl(etfg_insn_t const *insn);
bool etfg_iseq_rename_regs_and_imms(etfg_insn_t *iseq, long iseq_len,
    struct regmap_t const *etfg_regmap, struct imm_vt_map_t const *imm_map,
    nextpc_t const *nextpcs, long num_nextpcs);
size_t etfg_insn_put_nop_as(char *buf, size_t size);
size_t etfg_insn_put_linux_exit_syscall_as(char *buf, size_t size);



bool etfg_insn_symbols_are_contained_in_map(struct etfg_insn_t const *insn, struct imm_vt_map_t const *imm_vt_map);
bool etfg_insn_is_function_return(etfg_insn_t const *insn);
bool etfg_insn_get_constant_operand(etfg_insn_t const *insn, uint32_t *constant);

bool etfg_pc_is_valid_pc(struct input_exec_t const *in, src_ulong pc);
bool etfg_input_section_pc_is_executable(struct input_section_t const *isec, src_ulong pc);
size_t etfg_insn_put_nop(etfg_insn_t *insns, size_t insns_size);
size_t etfg_insn_put_move_imm_to_mem(dst_ulong imm, uint32_t memaddr, int membase, etfg_insn_t *insns, size_t insns_size);
bool etfg_insn_type_signature_supported(etfg_insn_t const *insn);
void etfg_insn_get_min_max_mem_vconst_masks(int *min_mask, int *max_mask,
    etfg_insn_t const *insn);
long src_iseq_from_string(translation_instance_t *ti, etfg_insn_t *etfg_iseq,
    size_t etfg_iseq_size, char const *s, i386_as_mode_t i386_as_mode);



regset_t const *etfg_regset_live_at_jumptable();

struct etfg_strict_canonicalization_state_t *etfg_strict_canonicalization_state_init();
void etfg_strict_canonicalization_state_free(struct etfg_strict_canonicalization_state_t *);
long etfg_insn_get_max_num_imm_operands(etfg_insn_t const *insn);
char *etfg_insn_get_fresh_state_str();
char *etfg_insn_get_transfer_state_str(etfg_insn_t const *insn, int num_nextpcs, int nextpc_indir);
void etfg_pc_get_function_name_and_insn_num(input_exec_t const *in, src_ulong pc, std::string &function_name, long &insn_num_in_function);
void fallback_etfg_translate(etfg_insn_t const *src_iseq, long src_iseq_len, long num_nextpcs, dst_insn_t *dst_iseq, long *dst_iseq_len, long dst_iseq_size, regmap_t const &src_regmap, struct transmap_t *fb_tmap, regset_t *live_outs, char const *src_filename/*, regset_t &flexible_used_regs*/, fixed_reg_mapping_t &fixed_reg_mapping);
map<nextpc_id_t, string> const &etfg_get_nextpc_map_at_pc(input_exec_t const *in, src_ulong pc);
vector<string> etfg_input_exec_get_arg_names_for_function(input_exec_t const *in, string const &function_name);
reg_identifier_t etfg_insn_get_returned_reg(etfg_insn_t const &insn);
graph_locals_map_t const &etfg_input_exec_get_locals_map_for_function(input_exec_t const *in, string const &function_name);
void etfg_insn_comment_get_function_name_and_insn_num(string const &comment, string &function_name, int &insn_num_in_function);
//eq::tfg *etfg_iseq_get_preprocessed_tfg(etfg_insn_t const *iseq, long iseq_len);
dshared_ptr<eqspace::tfg_llvm_t> src_iseq_get_preprocessed_tfg(src_insn_t const *iseq, long iseq_len, regset_t const *live_out, size_t num_live_out, std::map<eqspace::pc, std::map<string_ref, expr_ref>> const &return_reg_map, context *ctx, eqspace::sym_exec &se);

size_t etfg_insn_get_size_for_exreg(etfg_insn_t const *insn, exreg_group_id_t group, reg_identifier_t const &r);
dshared_ptr<eqspace::state const> etfg_pc_get_input_state(struct input_exec_t const *in, src_ulong pc);
//void etfg_insn_rename_nextpcs(etfg_insn_t *insn, nextpc_map_t const *nextpc_map);
size_t etfg_insn_put_invalid(etfg_insn_t *insn);
size_t etfg_insn_put_ret_insn(etfg_insn_t *insns, size_t insns_size, int retval);
state const &etfg_canonical_start_state(context *ctx);
regset_t etfg_get_retregs_at_pc(input_exec_t const *in, src_ulong pc);
void etfg_insn_rename_constants(etfg_insn_t *insn, imm_vt_map_t const *imm_vt_map);
void etfg_iseq_truncate_register_sizes_using_touched_regs(etfg_insn_t *iseq, size_t iseq_size);
bool etfg_iseq_triggers_undef_behaviour_on_state(etfg_insn_t const *src_iseq, long src_iseq_len, state_t const &state);
bool etfg_iseq_stores_eq_comparison_in_reg(etfg_insn_t const *iseq, long iseq_len, char const* prefix, exreg_group_id_t g, reg_identifier_t const &r);
bool etfg_iseq_stores_ult_comparison_in_reg(etfg_insn_t const *iseq, long iseq_len, char const* prefix, exreg_group_id_t g, reg_identifier_t const &r);
bool etfg_iseq_stores_slt_comparison_in_reg(etfg_insn_t const *iseq, long iseq_len, char const* prefix, exreg_group_id_t g, reg_identifier_t const &r);
bool etfg_iseq_stores_ugt_comparison_in_reg(etfg_insn_t const *iseq, long iseq_len, char const* prefix, exreg_group_id_t g, reg_identifier_t const &r);
bool etfg_iseq_stores_sgt_comparison_in_reg(etfg_insn_t const *iseq, long iseq_len, char const* prefix, exreg_group_id_t g, reg_identifier_t const &r);
bool etfg_iseq_involves_cbranch(etfg_insn_t const *iseq, long iseq_len);
bool dst_insn_is_related_to_etfg_iseq(dst_insn_t const &dst_insn, etfg_insn_t const *etfg_iseq, long etfg_iseq_len);
bool etfg_accept_insn_check_in_fallback_list(etfg_insn_t *insn);
src_ulong etfg_input_exec_get_start_pc_for_function_name(struct input_exec_t const *in, string const &function_name);
src_ulong etfg_input_exec_get_stop_pc_for_function_name(struct input_exec_t const *in, string const &function_name);
std::vector<translation_unit_t> etfg_input_exec_get_translation_units(struct input_exec_t const *in);
bool etfg_input_exec_edge_represents_phi_edge(input_exec_t const *in, src_ulong from_pc, src_ulong to_pc);
bool etfg_input_exec_pc_incident_on_phi_edge(input_exec_t const *in, src_ulong pc);
bool etfg_iseq_is_function_tailcall_opt_candidate(etfg_insn_t const *iseq, long iseq_len);
char *etfg_iseq_with_relocs_to_string(etfg_insn_t const *iseq, long iseq_len,
    char const *reloc_strings, size_t reloc_strings_size,
    i386_as_mode_t mode, char *buf, size_t buf_size);
void etfg_insn_convert_pcrels_and_symbols_to_reloc_strings(etfg_insn_t *src_insn, std::map<long, std::string> const& insn_labels, nextpc_t const* nextpcs, long num_nextpcs, input_exec_t const* in, src_ulong const* insn_pcs, int insn_index, std::set<symbol_id_t>& syms_used, char **reloc_strings, long *reloc_strings_size);
regmap_t const *etfg_identity_regmap();
void etfg_iseq_rename(etfg_insn_t *iseq, long iseq_len,
    struct regmap_t const *dst_regmap,
    struct imm_vt_map_t const *imm_map,
    memslot_map_t const *memslot_map,
    struct regmap_t const *src_regmap,
    struct nextpc_map_t const *nextpc_map,
    nextpc_t const *nextpcs, long num_nextpcs);

size_t etfg_input_file_find_pc_for_insn_index_ignoring_start_and_bbl_start_pcs(input_exec_t const* in, size_t orig_insn_index);
map<string, dshared_ptr<tfg>> get_function_tfg_map_from_etfg_file(string const &etfg_filename, context* ctx);
i386_as_mode_t etfg_get_i386_as_mode();


#endif /* etfg_insn.h */
