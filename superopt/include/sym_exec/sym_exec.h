#pragma once

#include <list>

#include "support/src-defs.h"
#include "support/dshared_ptr.h"

#include "valtag/regmap.h"
#include "valtag/imm_vt_map.h"
#include "valtag/nextpc_map.h"

#include "expr/state.h"
#include "expr/pc.h"

#include "insn/src-insn.h"
#include "insn/dst-insn.h"
#include "i386/regs.h"
#include "x64/regs.h"
#include "i386/insn.h"
#include "x64/insn.h"
#include "ppc/insn.h"
#include "etfg/etfg_insn.h"


#include "rewrite/src_iseq_match_renamings.h"
#include "rewrite/insn_db.h"

using namespace std;
class pc_pred_state;

/*
struct expr_ro {
  int dummy;
};

struct tfg_ro {
  int dummy;
};

typedef struct src_m_db_entry_t {
  cspace::src_insn_t src_insn;
  tfg_ro tfg;
} src_m_db_entry_t;

typedef struct dst_m_db_entry_t {
  cspace::dst_insn_t insn;
  tfg_ro tfg;
} dst_m_db_entry_t;

class sym_exec_reader
{
public:
  sym_exec_reader();
  ~sym_exec_reader();

  void parse_consts_db(eq::context *ctx);
  void src_parse_sym_exec_db(eq::context* ctx);
  void dst_parse_sym_exec_db(eq::context* ctx);

  //void src_add(cspace::src_insn_t* iseq, const machine_state& st);
  //void dst_add(cspace::dst_insn_t* iseq, const machine_state& st);
  void src_add(cspace::src_insn_t* iseq, const trans_fun_graph& tfg);
  void dst_add(cspace::dst_insn_t* iseq, const trans_fun_graph& tfg);

  //void write_consts_struct_as_ro(void);

#ifndef SRC_M_DB_RO_NUM_ENTRIES
#define SRC_M_DB_RO_NUM_ENTRIES 0
#endif

#ifndef DST_M_DB_RO_NUM_ENTRIES
#define DST_M_DB_RO_NUM_ENTRIES 0
#endif

#ifndef CONSTS_RO_NUM_ENTRIES
#define CONSTS_RO_NUM_ENTRIES 0
#endif

#ifndef SYMBOLS_RO_NUM_ENTRIES
#define SYMBOLS_RO_NUM_ENTRIES 0
#endif

#ifndef LOCALS_RO_NUM_ENTRIES
#define LOCALS_RO_NUM_ENTRIES 0
#endif

#ifndef NEXTPC_CONSTS_RO_NUM_ENTRIES
#define NEXTPC_CONSTS_RO_NUM_ENTRIES 0
#endif

#ifndef INSN_ID_CONSTS_RO_NUM_ENTRIES
#define INSN_ID_CONSTS_RO_NUM_ENTRIES 0
#endif

  static struct src_m_db_entry_t const src_m_db_ro_entries[SRC_M_DB_RO_NUM_ENTRIES];
  static struct dst_m_db_entry_t const dst_m_db_ro_entries[DST_M_DB_RO_NUM_ENTRIES];
  static struct expr_ro const consts_ro[CONSTS_RO_NUM_ENTRIES];
  static struct expr_ro const symbols_ro[SYMBOLS_RO_NUM_ENTRIES];
  static struct expr_ro const locals_ro[LOCALS_RO_NUM_ENTRIES];
  static struct expr_ro const nextpc_consts_ro[NEXTPC_CONSTS_RO_NUM_ENTRIES];
  static struct expr_ro const insn_id_consts_ro[INSN_ID_CONSTS_RO_NUM_ENTRIES];

  std::list<pair<cspace::src_insn_t*, trans_fun_graph> > src_m_db;
  std::list<pair<cspace::dst_insn_t*, trans_fun_graph> > dst_m_db;
  machine_state src_m_base_state;
  machine_state dst_m_base_state;
  consts_struct_t m_consts_struct;
};
*/

namespace eqspace {

class tfg;

class sym_exec
{
public:
  sym_exec(bool cache_tfgs = true);
  ~sym_exec();

  //static string get_keyword_from_regtype_indices(reg_type rt, int i, int j);
  void src_parse_sym_exec_db(context* ctx);
  void dst_parse_sym_exec_db(context* ctx);
  //void set_consts_struct(consts_struct_t &cs) { m_consts_struct = &cs; }
  //void init_using_sym_exec_reader(sym_exec_reader const &se_reader);
  void gen_sym_exec_reader_file_consts(FILE *ofp);
  void gen_sym_exec_reader_file_base_state(FILE *ofp);
  void gen_sym_exec_reader_file_tfg(tfg const *tfg, FILE *ofp);

  //void src_apply_transfer_fun(const src_insn_t*, int cur_index, tfg * tfg/*, bool use_memaccess_comments*/) const;
  //void dst_apply_transfer_fun(const dst_insn_t*, int cur_index, tfg * tfg/*, bool use_memaccess_comments*/) const;

  const state& src_get_base_state() const;
  const state& dst_get_base_state() const;

  consts_struct_t const &get_consts_struct() const;
  expr_ref i386_get_pred(expr_ref arg1, expr_ref arg2, string opc, expr_ref existing_pred) const;

  void clear();

  dshared_ptr<tfg> src_get_tfg(src_insn_t const iseq[], long iseq_len, context *ctx, string const& name, string const& fname);
  dshared_ptr<tfg> dst_get_tfg(dst_insn_t const iseq[], long iseq_len, context *ctx, string const& name, string const& fname);

  void src_get_next_pc_pred_states(src_insn_t const iseq[], long iseq_len,
                             context *ctx, pc p, stack<expr_ref> pred,
                             int nextpc_indir, list<pc_pred_state>& next, bool add_memaccess_comments);

  void dst_get_next_pc_pred_states(dst_insn_t const iseq[], long iseq_len,
                             context *ctx, pc p, stack<expr_ref> pred,
                             int nextpc_indir, list<pc_pred_state>& next, bool add_memaccess_comments);

  dshared_ptr<tfg> src_insn_get_tfg(src_insn_t const *insn, long cur_index, context *ctx/*, bool src, bool use_memaccess_comments*/);
  dshared_ptr<tfg> dst_insn_get_tfg(dst_insn_t const *insn, long cur_index, context *ctx/*, bool src, bool use_memaccess_comments*/);

  void src_get_usedef_using_transfer_function(src_insn_t const *insn,
    regset_t *use, regset_t *def, memset_t *memuse,
    memset_t *memdef);

  dshared_ptr<tfg> src_insn_get_tfg_from_insn_db_match(src_insn_t const &insn, insn_db_match_t<src_insn_t, pair<streampos, dshared_ptr<tfg const>>> const &m,  long cur_index, context *ctx);
  dshared_ptr<tfg> dst_insn_get_tfg_from_insn_db_match(dst_insn_t const &insn, insn_db_match_t<dst_insn_t, pair<streampos, dshared_ptr<tfg const>>> const &m,  long cur_index, context *ctx);


  void dst_get_usedef_using_transfer_function(dst_insn_t const *insn,
    regset_t *use, regset_t *def, memset_t *memuse,
    memset_t *memdef);


private:
  dshared_ptr<tfg const> src_get_tfg_from_insn_db_list_elem(insn_db_list_elem_t<src_insn_t, pair<streampos, dshared_ptr<tfg const>>> &e, string const &db_file, state const &base_state);
  dshared_ptr<tfg const> dst_get_tfg_from_insn_db_list_elem(insn_db_list_elem_t<dst_insn_t, pair<streampos, dshared_ptr<tfg const>>> &e, string const &db_file, state const &base_state);

  dshared_ptr<tfg> src_insn_get_tfg_matching_at_iter(src_insn_t const *insn, long cur_index, context *ctx, std::list<pair<src_insn_t *, pair<streampos, dshared_ptr<tfg const>>>>::iterator iter);
  dshared_ptr<tfg> dst_insn_get_tfg_matching_at_iter(dst_insn_t const *insn, long cur_index, context *ctx, std::list<pair<dst_insn_t *, pair<streampos, dshared_ptr<tfg const>>>>::iterator iter);

  //std::list<pair<src_insn_t *, pair<streampos, shared_ptr<tfg const>>>>::iterator src_insn_get_next_matching_iterator(src_insn_t const *insn, long cur_index, context *ctx, std::list<pair<src_insn_t *, pair<streampos, shared_ptr<tfg const>>>>::iterator iter);
  //std::list<pair<dst_insn_t *, pair<streampos, tfg const *>>>::iterator dst_insn_get_next_matching_iterator(dst_insn_t const *insn, long cur_index, context *ctx, std::list<pair<dst_insn_t *, pair<streampos, tfg const *>>>::iterator iter);

  dshared_ptr<tfg> src_get_tfg_full(src_insn_t const iseq[], long iseq_len, context *ctx, string const& name, string const& fname);
  dshared_ptr<tfg> dst_get_tfg_full(dst_insn_t const iseq[], long iseq_len, context *ctx, string const& name, string const& fname);
  void src_add(src_insn_t* iseq, streampos off, dshared_ptr<tfg const> const &tfg);
  void dst_add(dst_insn_t* iseq, streampos off, dshared_ptr<tfg const> const &tfg);

  bool m_cache_tfgs;
  string src_m_db_file, dst_m_db_file;
  //std::list<pair<src_insn_t*, pair<streampos, tfg const *> > > src_m_db;
  //std::list<pair<dst_insn_t*, pair<streampos, tfg const *> > > dst_m_db;
  insn_db_t<src_insn_t, pair<streampos, dshared_ptr<tfg const>>> src_m_db;
  insn_db_t<dst_insn_t, pair<streampos, dshared_ptr<tfg const>>> dst_m_db;
  state src_m_base_state;
  state dst_m_base_state;
  //consts_struct_t *m_consts_struct;
};

extern sym_exec *g_se;

ts_tab_entry_cons_t *tfg_get_type_constraints(tfg const *tfg, char const *progress_str);
ts_tab_entry_transfer_t *tfg_get_type_transfers(tfg const *tfg, char const *progress_str);
void tfg_get_usedef_regs(tfg const *tfg, regset_t *use, regset_t *def);
void tfg_get_usedef_mem(tfg const *tfg, memset_t *memuse, memset_t *memdef);
void tfg_get_usedef(tfg const *tfg, regset_t *use, regset_t *def, memset_t *memuse, memset_t *memdef);

void state_elem_desc_parse_mem(state_elem_desc_t *se, expr_ref addr, int nbytes,
    state const &in, context *ctx);
}

void g_se_init(void);
#if ARCH_SRC == ARCH_ETFG
map<string, dshared_ptr<tfg>>
etfg_iseq_get_function_tfg_map(etfg_insn_t const *src_iseq, long src_iseq_len, long num_live_out);
#endif
