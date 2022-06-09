#include "cmn.h"

static string
cmn_cpp_insn_to_string(cmn_insn_t const *iseq)
{
#define MAX_BUF_LEN 409600
  char* buf = new char[MAX_BUF_LEN];
  cmn_iseq_to_string(iseq, 1, buf, MAX_BUF_LEN);

  string str(buf);
  delete[] buf;
  return str;
}

/*
void
sym_exec::cmn_apply_transfer_fun(const cmn_insn_t* iseq, int cur_index, trans_fun_graph *tfg, bool use_memaccess_comments) const
{
  INFO("sym_exec at: " << cur_index << cmn_cpp_insn_to_string(iseq + cur_index) << endl);
//  cout << "input machine_state:\n" << st.to_string() << endl;
  const cmn_insn_t* input = iseq + cur_index;
  struct regmap_t regmap;
  struct imm_vt_map_t imm_vt_map;
  struct nextpc_map_t nextpc_map;

  //st.m_bt = machine_state::branch_target_invalid;
  if (cmn_insn_is_nop(input)) {
    //st.m_bt = machine_state::branch_target_none;
    return;
  }

  for(std::list<pair<cmn_insn_t*, trans_fun_graph> >::const_iterator iter = cmn_m_db.begin(); iter != cmn_m_db.end(); ++iter) {
    NOT_IMPLEMENTED(); //rename templ_tfg using nextpc_map
    //tfg->append_tfg(&templ_tfg);
    return;
  }

  fprintf(stderr, "%s: No match found for:\n%s\n", __func__,
      cmn_iseq_to_string(&iseq[cur_index], 1, as1, sizeof as1));
  NOT_REACHED();

  assert(false); // no match
}
*/

void
sym_exec::cmn_parse_sym_exec_db(context* ctx)
{
  autostop_timer timer(__func__);
  //cmn_m_db_file = ABUILD_DIR "/" cmn_sym_exec_db_filename;
  cmn_m_db_file = SUPEROPTDBS_DIR cmn_sym_exec_db_filename;
  //cout << __func__ << " " << __LINE__ << ": cmnm_db_file = " << cmn_m_db_file << endl;
  ifstream db(cmn_m_db_file.c_str());
  //cout << __func__ << " " << __LINE__ << ": cmn_m_db_file = " << cmn_m_db_file << endl;

  //expr_ref pred = ctx->mk_var("pred", ctx->mk_bool_sort());

  if (db.is_open()) {
    string line;
    bool more;
    if (!getline(db, line)) {
      more = false;
    } else {
      more = true;
    }

    //cout << __func__ << " " << __LINE__ << ": line = " << line << endl;
    if(more && line.find("=Base state") != string::npos) {
      //more = getline(db, line);
      //DEBUG("parsing base state\n");
      map<string_ref, expr_ref> value_expr_map;
      state::read_state(db, value_expr_map, ctx/*, NULL*/);
      cmn_m_base_state.set_value_expr_map(value_expr_map);
      cmn_m_base_state.populate_reg_counts(/*NULL*/);
      //cout << "parsing base state done\n";
      //cmn_m_base_state.set_branch_target(state::branch_target_none, ctx->mk_var("state.bt_pred", ctx->mk_bool_sort()), ctx->mk_var("state.bt_indir_tgt", cmn_m_base_state.m_mexvregs[0][0]->get_sort()));
      bool end = !getline(db, line);
      ASSERT(!end);
#if CMN != COMMON_SRC || ARCH_SRC != ARCH_ETFG
      cmn_m_base_state.state_introduce_src_dst_prefix(cmn_prefix, true);
      //cout << __func__ << " " << __LINE__ << ": cmn_m_base_state = " << cmn_m_base_state.to_string() << endl;
#endif
    }

    int progress = 0;
    while(more) {
      /*if(progress % 100 == 0)
        cout << "progress: " << progress << endl;*/
      //cout << __func__ << " " << __LINE__ << ": progress = " << progress << endl;
      progress++;

      string insn;
      line = read_insn(db, insn);

      cmn_insn_t* iseq = new cmn_insn_t[2];
      long iseq_len;
      ASSERT(line == "--");
      string section;
      line = read_sym_exec_section(db, section);
      ASSERT(line == "--");
      section += "\n--\n";
      bool end = !getline(db, line);
      ASSERT(!end);
      if (cmn_insn_t::uses_serialization()) {
        //cout << __func__ << " " << __LINE__ << ": section = '" << section << "'\n";
        istringstream iss(section);
        iseq_len = cmn_iseq_deserialize(iss, iseq, 2);
      } else {
        iseq_len = cmn_iseq_from_string(0, iseq, 2, insn.c_str(), I386_AS_MODE_CMN);
      }
      //cout << "iseq len:" << iseq_len << " for: " << insn << endl;
      if (iseq_len == 0 || iseq_len == -1) {
        cout << "could not parse insn from '" << insn << "' line '" << line << "'" << endl;
        cout << "str = " << insn << endl;
      }
      assert(iseq_len == 1 || iseq_len == 2);
      //cout << "insn: '" << insn << endl;
      if (line != "=tfg") {
        cout << "line : " << line << "." << endl;
      }
      ASSERT(line == "=tfg");

      CPP_DBG_EXEC(INSN_MATCH, cout << _FNLN_ << ": Adding offset " << db.tellg() << " for " << cmn_iseq_to_string(iseq, 1, as1, sizeof as1) << endl);
      cmn_add(iseq, db.tellg(), dshared_ptr<tfg const>::dshared_nullptr());

      do {
        if (getline(db, line)) {
          more = true;
        } else {
          more = false;
          break;
        }
      } while (line != "=insn");
      //while ((more = getline(db, line)) && line != "=insn");

      //more = line.length() > 0;
    }

    //DEBUG("parsing complete" << endl);
    db.close();
  } else {
    cout << "Unable to open sym_exec_db " << cmn_m_db_file << "\n";
    assert(false);
  }
}

/*
extern "C" void
cmn_gen_sym_exec_reader_file_entry_for_insn(cmn_insn_t const *insn,
    FILE *ofp)
{
  static sym_exec se;
  static bool init = false;

  if (!init) {
    //sym_exec_reader se_reader;
    ASSERT(g_ctx);
    //ctx = new context();
    se.parse_consts_db(g_ctx);
    se.cmn_parse_sym_exec_db(g_ctx);
    //se.init_using_sym_exec_reader(se_reader);
    init = true;
  }

  trans_fun_graph *tfg;

  tfg = se.cmn_insn_get_tfg(insn, 0, g_ctx, 0, true, false);

  //fprintf(ofp, "{\n.insn=");
  cmn_insn_gen_sym_exec_reader_file(insn, ofp);
  //fprintf(ofp, ".tfg=\n");
  se.gen_sym_exec_reader_file_tfg(tfg, ofp);
  delete tfg;
}
*/

void
sym_exec::cmn_get_usedef_using_transfer_function(cmn_insn_t const *insn,
    regset_t *use, regset_t *def, memset_t *memuse,
    memset_t *memdef)
{
  autostop_timer func_timer(__func__);
  g_ctx->clear_cache();
  consts_struct_t const &cs = g_ctx->get_consts_struct();
  regset_clear(use);
  regset_clear(def);
  memset_clear(memuse);
  memset_clear(memdef);
  bool regset_inited = false;

  //state st("usedef_in", g_ctx);
  //state st_copy("usedef_in", g_ctx);

  //cout << __func__ << " " << __LINE__ << ": calling insn_db_get_all_matches() for tfg\n";
  vector<insn_db_match_t<cmn_insn_t, pair<streampos, dshared_ptr<tfg const>>>> matched = cmn_m_db.insn_db_get_all_matches(*insn, true);
  //cout << __func__ << " " << __LINE__ << ": matched.size() = " << matched.size() << endl;

  //auto iter = cmn_insn_get_next_matching_iterator(insn, 0, g_ctx, cmn_m_db.begin());

  for (const auto &m : matched) {
    regset_t iuse, idef;
    memset_t imemuse, imemdef;
    //tfg *tfg = this->cmn_insn_get_tfg_matching_at_iter(insn, 0, g_ctx, iter);
    dshared_ptr<tfg> tfg = this->cmn_insn_get_tfg_from_insn_db_match(*insn, m, 0, g_ctx);
    CPP_DBG_EXEC(TFG_GET_USEDEF, cout << __func__ << " " << __LINE__ << ": insn: " << cmn_iseq_to_string(insn, 1, as1, sizeof as1) << endl);
    CPP_DBG_EXEC(TFG_GET_USEDEF, cout << "matched with template:\n" << cmn_iseq_to_string(&m.e.i, 1, as1, sizeof as1) << "\ntfg:\n" << tfg->graph_to_string() << endl);
    tfg_get_usedef(*tfg, &iuse, &idef, &imemuse, &imemdef);
    if (!regset_inited) {
      regset_copy(use, &iuse);
      regset_copy(def, &idef);
      memset_copy(memuse, &imemuse);
      memset_copy(memdef, &imemdef);
      regset_inited = true;
    } else {
      regset_intersect(use, &iuse);
      regset_intersect(def, &idef);
    }
    //CPP_DBG(TMP, "insn = %s\n", cmn_iseq_to_string(insn, 1, as1, sizeof as1));
    //CPP_DBG(TMP, "use=%s, def=%s.\n", regset_to_string(use, as1, sizeof as1), regset_to_string(def, as2, sizeof as2));
    //delete tfg;
    //iter++;
    //iter = cmn_insn_get_next_matching_iterator(insn, 0, g_ctx, iter);
  }
  /*cout << __func__ << ": usedef_in:\n" << st.to_string() << endl;
  cout << __func__ << ": usedef_out:\n" << st_copy.to_string() << endl;
  cout << __func__ << ": use: " << regset_to_string(use, as1, sizeof as1) << endl;
  cout << __func__ << ": def: " << regset_to_string(def, as1, sizeof as1) << endl;*/
}

#if 0
void
sym_exec::cmn_get_next_pc_pred_states(cmn_insn_t const iseq[], long iseq_len,
                             struct context *ctx, pc p, stack<expr_ref> pred,
                             int nextpc_indir, list<pc_pred_state>& next, bool add_memaccess_comments)
{
  if(p.is_exit())
    return;

  int cur = p.get_index();
  /*machine_state st("dummy_dfs", se.cmn_get_base_state(), ctx);
  trans_fun_graph tfg(ctx, st);
  assert(cur < iseq_len);
  se.cmn_apply_transfer_fun(iseq, cur, &tfg, add_memaccess_comments);*/
  tfg *itfg;
  itfg = cmn_insn_get_tfg(&iseq[cur], cur, ctx, nextpc_indir, true, add_memaccess_comments);

  list<shared_ptr<tfg_edge>> exiting_edges;
  itfg->find_exiting_edges(exiting_edges);

  string opc = string(opctable_name(iseq[cur].opc));

  for (list<trans_fun_graph::edge *>::const_iterator iter = exiting_edges.begin();
       iter != exiting_edges.end();
       iter++) {
    stack<expr_ref> pred_stack;
    pred_stack.push((*iter)->get_pred());
    next.push_back(pc_pred_state((*iter)->get_to_pc(), pred_stack));
  }
  delete itfg;
/*
  switch(st.m_bt)
  {
  case machine_state::branch_target_none:
    next.push_back(pc_pred_state(pc(pc::insn_label, cur+1), pred, st));
    break;
  case machine_state::branch_target_insn:
  {
    expr_ref pred_bt = st.m_bt_pred;
    //expr_ref pred_jump = expr_and(pred, pred_bt);
    stack<expr_ref> pred_jump = pred_stack_and(pred, pred_bt, ctx);
    next.push_back(pc_pred_state(pc(pc::insn_label, st.m_bt_index), pred_jump, st));
    if(!pred_bt->is_const_bool_true())
    {
      //expr_ref pred_ft = expr_and(pred, expr_not(pred_bt));
      stack<expr_ref> pred_ft = pred_stack_and(pred, expr_not(pred_bt), ctx);
      next.push_back(pc_pred_state(pc(pc::insn_label, cur+1), pred_ft, st));
    }
    break;
  }
  case machine_state::branch_target_nextpc:
  {
    expr_ref pred_bt = st.m_bt_pred;
    //expr_ref pred_jump = expr_and(pred, pred_bt);
    stack<expr_ref> pred_jump = pred_stack_and(pred, pred_bt, ctx);
    next.push_back(pc_pred_state(pc(pc::exit, st.m_bt_index), pred_jump, st));
    if(!pred_bt->is_const_bool_true())
    {
      //expr_ref pred_ft = expr_and(pred, expr_not(pred_bt));
      stack<expr_ref> pred_ft = pred_stack_and(pred, expr_not(pred_bt), ctx);
      next.push_back(pc_pred_state(pc(pc::insn_label, cur+1), pred_ft, st));
    }
    break;
  }
  case machine_state::branch_target_indir:
  {
    if(opc == "jmp") // can't handle indirect jmps
    {
      error(ERROR_EXIT_PC_NOT_RET);
    }
    expr_ref pred_bt = st.m_bt_pred;
    //expr_ref pred_jump = expr_and(pred, pred_bt);
    stack<expr_ref> pred_jump = pred_stack_and(pred, pred_bt, ctx);
    next.push_back(pc_pred_state(pc(pc::exit, nextpc_indir), pred_jump, st));
    break;
  }
  default: assert(false);
  }
*/
  //next.sort() in topsorted order XXX : TODO; this will increase opportunities for optimization in pred_stack_or()
}

static void
cmn_get_loop_heads_rec(pc p, cmn_insn_t const iseq[], long iseq_len,
                    context *ctx, sym_exec & se, int nextpc_indir, set<pc, pc_comp>& loop_heads,
                    map<pc, dfs_color, pc_comp>& visited)
{
  if(visited.count(p) == 0)
    visited[p] = dfs_color_white;

  switch(visited.at(p))
  {
  case dfs_color_black:
    break;
  case dfs_color_gray:
    break;
  case dfs_color_white:
  {
    visited[p] = dfs_color_gray;
    list<pc_pred_state> next;
    machine_state st("dummy_dfs", se.cmn_get_base_state(), ctx);
    se.cmn_get_next_pc_pred_states(iseq, iseq_len, ctx, /* st, */p, stack<expr_ref>(), nextpc_indir, next, false);

    //cout << "cur pc: " << p.to_string() << endl;
    for(list<pc_pred_state>::iterator iter = next.begin(); iter != next.end(); ++iter)
    {
      pc p_next = iter->m_pc;
      //cout << "next pc: " << p_next.to_string() << endl;

      if(visited.count(p_next) > 0 && visited.at(p_next) == dfs_color_gray)
      {
//        if(next.size() == 2)
//          labels.insert(p);
//        else
          if (p_next.is_label()) {
            loop_heads.insert(p_next);
          }
      }
      if(next.size() == 2/* && src*/)
      {
        //labels.insert(p);
        list<pc_pred_state>::iterator iter = next.begin();
        if (iter->m_pc.is_label()) {
          loop_heads.insert(iter->m_pc);
        }
        iter++;
        if (iter->m_pc.is_label()) {
          loop_heads.insert(iter->m_pc);
        }
      }

//      string opc = string(opctable_name((iseq + p.get_index())->opc));
//      if(opc == "jmp" && src)
//        labels.insert(p);

      /*if(p_next.is_exit()) 
        loop_heads.insert(p_next);
      */
      cmn_get_loop_heads_rec(p_next, iseq, iseq_len, ctx, se, nextpc_indir, loop_heads, visited/*, src*/);
    }

    visited[p] = dfs_color_black;
    break;
  }
  }
}


static void
cmn_get_loop_heads(cmn_insn_t const iseq[], long iseq_len,
                context *ctx, sym_exec & se, int nextpc_indir, set<pc, pc_comp>& loop_heads)
{
  map<pc, dfs_color, pc_comp> visited;
  cmn_get_loop_heads_rec(pc::start(), iseq, iseq_len, ctx, se, nextpc_indir, loop_heads, visited);
}


static void
cmn_get_successors(pc pc_from, cmn_insn_t const iseq[], long iseq_len,
                    context *ctx, sym_exec const &se, trans_fun_graph *tfg /*machine_state st*/,
                    const set<pc, pc_comp>& labels,
                    int nextpc_indir, list<pc_pred_state>& succs/*, bool use_memaccess_comments*/)
{
  //cout << "get_successors" << endl;
  list<pc_pred_state> todo;
  stack<expr_ref> true_pred_stack;
  long num_iter = 0;
  //true_pred_stack.push(expr_true(eq->get_context()));
  //todo.push_back(pc_pred_state(pc_from, eq->get_context()->mk_bool_true(), st));
  LOG("Calling cmn_get_successors for labels:");
  for (set<pc, pc_comp>::const_iterator iter = labels.begin(); iter != labels.end(); iter++) {
    LOG(" " << iter->to_string());
  }
  LOG(endl);

  NOT_IMPLEMENTED();
/*
  todo.push_back(pc_pred_state(pc_from, true_pred_stack, st));
  while(todo.size() > 0) {
    if (num_iter > 2 * iseq_len + 1) {
      error(ERROR_INFINITE_LOOP_IN_GET_TFG);
    }
    num_iter++;
    list<pc_pred_state>::iterator iter = todo.begin();
    pc p = iter->m_pc;
    stack<expr_ref> pred_stack = iter->m_pred_stack;
    machine_state st = iter->m_state;
    bool is_exit = iter->m_pc.is_exit();
    todo.erase(iter);

    if(!is_exit)
    {
      list<pc_pred_state> next;
      DEBUG(__func__ << ": Getting next_pc_pred_states for pc " << iter->m_pc.to_string() << endl);
      cmn_get_next_pc_pred_states(iseq, iseq_len, eq->get_context(), eq, se, st, p, pred_stack, nextpc_indir, next, use_memaccess_comments);

      for(list<pc_pred_state>::iterator iter = next.begin(); iter != next.end(); ++iter)
      {
        pc p = iter->m_pc;
        if(labels.count(p) > 0) {// label, add to succs
          add_pc_pred_state(eq->get_context(), succs, *iter);
        } else {// add to todo
          DEBUG(__func__ << ": adding pc_pred_state to " << p.to_string() << endl);
          add_pc_pred_state(eq->get_context(), todo, *iter);
        }
      }
    }
  }
*/
}



static void
cmn_get_labels_rec(pc p, cmn_insn_t const iseq[], long iseq_len,
                    context *ctx, sym_exec & se, int nextpc_indir, set<pc, pc_comp>& labels,
                    map<pc, dfs_color, pc_comp>& visited, bool src)
{
  if(visited.count(p) == 0)
    visited[p] = dfs_color_white;

  switch(visited.at(p))
  {
  case dfs_color_black:
    break;
  case dfs_color_gray:
    break;
  case dfs_color_white:
  {
    visited[p] = dfs_color_gray;
    list<pc_pred_state> next;
    machine_state st("dummy_dfs", se.cmn_get_base_state(), ctx);
    se.cmn_get_next_pc_pred_states(iseq, iseq_len, ctx, /* st, */p, stack<expr_ref>(), nextpc_indir, next, false);

    //DEBUG(__func__ << ": src = " << src << " cur pc: " << p.to_string() << endl);
    for(list<pc_pred_state>::iterator iter = next.begin(); iter != next.end(); ++iter)
    {
      pc p_next = iter->m_pc;
      //DEBUG(__func__ << ": src = " << src << " next pc: " << p_next.to_string() << endl);

      if(visited.count(p_next) > 0 && visited.at(p_next) == dfs_color_gray)
      {
//        if(next.size() == 2)
//          labels.insert(p);
//        else
          //DEBUG(__func__ << ": src = " << src << " inserting label because already visited in colour gray: " << p_next.to_string() << endl);
          labels.insert(p_next);
      }
      if(next.size() == 2 && src)
      {
        //labels.insert(p);
        list<pc_pred_state>::iterator iter = next.begin();
        labels.insert(iter->m_pc);
        //DEBUG(__func__ << ": src = " << src << " inserting label: " << iter->m_pc.to_string() << endl);
        iter++;
        labels.insert(iter->m_pc);
        //DEBUG(__func__ << ": src = " << src << " inserting label: " << iter->m_pc.to_string() << endl);
      }

//      string opc = string(opctable_name((iseq + p.get_index())->opc));
//      if(opc == "jmp" && src)
//        labels.insert(p);

      if (p_next.is_exit()) {
        labels.insert(p_next);
      }
      cmn_get_labels_rec(p_next, iseq, iseq_len, ctx, se, nextpc_indir, labels, visited, src);
    }

    visited[p] = dfs_color_black;
    break;
  }
  }
}

static void
cmn_get_labels(cmn_insn_t const iseq[], long iseq_len,
                context *ctx, sym_exec &se, int nextpc_indir, set<pc, pc_comp>& labels, bool src)
{
  //cout << "get_labels" << endl;

  labels.insert(pc::start());
  map<pc, dfs_color, pc_comp> visited;
  cmn_get_labels_rec(pc::start(), iseq, iseq_len, ctx, se, nextpc_indir, labels, visited, src);

  //print_labels(labels);
}
#endif

dshared_ptr<tfg const>
sym_exec::cmn_get_tfg_from_insn_db_list_elem(insn_db_list_elem_t<cmn_insn_t, pair<streampos, dshared_ptr<tfg const>>> &e, string const &db_file, state const &base_state)
{
  consts_struct_t const &cs = g_ctx->get_consts_struct();
  if (e.v.second) {
    CPP_DBG_EXEC(INSN_MATCH, cout << _FNLN_ << ": returning e.v.second:\n"; e.v.second->graph_to_stream(cout));
    return e.v.second;
  }
  ifstream db(db_file.c_str());
  ASSERT(db.is_open());
  CPP_DBG_EXEC(INSN_MATCH, cout << _FNLN_ << ": e.v.first = " << e.v.first << endl);
  db.seekg(e.v.first);

  dshared_ptr<tfg> tfg_var = make_dshared<tfg_asm_t>("dst", DUMMY_FNAME, g_ctx/*, cmn_m_base_state*/);
  tfg_var->set_start_state(cmn_m_base_state);
  dshared_ptr<tfg_node> n = make_dshared<tfg_node>(pc::start());
  tfg_var->add_node(n);
  tfg_var->read_from_ifstream(db, g_ctx, base_state);

#if CMN != COMMON_SRC || ARCH_SRC != ARCH_ETFG
  //cout << __func__ << " " << __LINE__ << ": calling introduce src_dst prefix\n";
  tfg_var->tfg_introduce_src_dst_prefix(cmn_prefix);
#endif
  //tfg_var->simplify_edges();
  //cout << __func__ << " " << __LINE__ << ": insn = " << cmn_iseq_to_string(iter->first, 1, as1, sizeof as1) << ", tfg_var:\n" << tfg_var->to_string(true) << endl;
  if (m_cache_tfgs) {
    e.v.second = tfg_var;
  }
  return static_pointer_cast<tfg const>(tfg_var);
}

/*std::list<pair<cmn_insn_t *, pair<streampos, tfg const *>>>::iterator
sym_exec::cmn_insn_get_next_matching_iterator(cmn_insn_t const *insn, long cur_index, context *ctx, std::list<pair<cmn_insn_t *, pair<streampos, tfg const *> > >::iterator iter)
{
  for (; iter != cmn_m_db.end(); iter++) {
    struct imm_vt_map_t imm_vt_map;
    struct nextpc_map_t nextpc_map;
    struct regmap_t regmap;
    cmn_insn_t const *iseq_db;
    bool match;

    iseq_db = iter->first;
    //cout << __func__ << " " << __LINE__ << ":\n";
    match = cmn_iseq_matches_template_var(iseq_db, insn, 1, &regmap, &imm_vt_map,
        &nextpc_map, __func__);

    //cout << __func__ << " " << __LINE__ << ": iseq_db: " << cmn_cpp_insn_to_string(iseq_db) << ", match = " << match << endl;
    if (match) {
      return iter;
      //CPP_DBG(PEEP_TAB, "matched %s\nwith\n%s\n", cmn_iseq_to_string(insn, 1, as1, sizeof as1),
      //    cmn_iseq_to_string(iseq_db, 1, as2, sizeof as2));
      //CPP_DBG(PEEP_TAB, "regmap\n%s\n", regmap_to_string(&regmap, as1, sizeof as1));
      //CPP_DBG(PEEP_TAB, "imm_vt_map\n%s\n", imm_vt_map_to_string(&imm_vt_map, as1, sizeof as1));
      //templ_tfg = cmn_get_tfg_from_db_iterator(iter, cmn_m_db_file, cmn_m_base_state);
    }
  }
  return cmn_m_db.end();
}*/

dshared_ptr<tfg>
sym_exec::cmn_insn_get_tfg_from_insn_db_match(cmn_insn_t const &insn, insn_db_match_t<cmn_insn_t, pair<streampos, dshared_ptr<tfg const>>> const &m,  long cur_index, context *ctx)
{
  //struct memlabel_map_t memlabel_map;
  consts_struct_t const &cs = ctx->get_consts_struct();
  struct imm_vt_map_t const &imm_vt_map = m.src_iseq_match_renamings.get_imm_vt_map();
  struct nextpc_map_t const &nextpc_map = m.src_iseq_match_renamings.get_nextpc_map();
  dshared_ptr<tfg const> templ_tfg;
  struct regmap_t const &regmap = m.src_iseq_match_renamings.get_regmap();
  dshared_ptr<tfg> tfg_ret;

  //cout << __func__ << " " << __LINE__ << ": insn: " << cmn_cpp_insn_to_string(insn) << endl;
  templ_tfg = dshared_ptr<tfg>::dshared_nullptr();

  //iter = cmn_insn_get_next_matching_iterator(insn, cur_index, ctx, iter);
  //ASSERT(iter != cmn_m_db.end());
  //cmn_insn_t const &iseq_db = m.i;
  //bool match = cmn_iseq_matches_template_var(&iseq_db, insn, 1, &regmap, &imm_vt_map,
  //    &nextpc_map, __func__);
  //ASSERT(match);
  //CPP_DBG(PEEP_TAB, "matched %s\nwith\n%s\n", cmn_iseq_to_string(&insn, 1, as1, sizeof as1),
  //    cmn_iseq_to_string(&iseq_db, 1, as2, sizeof as2));
  DYN_DBG(peep_tab, "regmap\n%s\n", regmap_to_string(&regmap, as1, sizeof as1));
  DYN_DBG(peep_tab, "imm_vt_map\n%s\n", imm_vt_map_to_string(&imm_vt_map, as1, sizeof as1));
  //templ_tfg = cmn_get_tfg_from_db_iterator(iter, cmn_m_db_file, cmn_m_base_state);
  templ_tfg = cmn_get_tfg_from_insn_db_list_elem(m.e, cmn_m_db_file, cmn_m_base_state);

  ASSERT(templ_tfg);
  DYN_DBG(peep_tab, "imm_vt_map:\n%s\n", imm_vt_map_to_string(&imm_vt_map, as1, sizeof as1));
  map<expr_id_t, pair<expr_ref, expr_ref>> submap;

  DYN_DEBUG(peep_tab, cout << __func__ << " " << __LINE__ << ": templ_tfg:\n" << templ_tfg->graph_to_string() << endl);

  tfg_ret = templ_tfg->tfg_copy();
  /*if (!m_cache_tfgs) {
    delete templ_tfg;
  }*/
  //tfg_ret->tfg_set_src(src);

  DYN_DEBUG(peep_tab, cout << __func__ << " " << __LINE__ << ": tfg_ret:\n" << tfg_ret->graph_to_string() << endl);
  //if (use_memaccess_comments) {
  //  tfg_ret->append_insn_id_to_comments(cur_index);
  //} else {
  //  tfg_ret->set_comments_to_zero();
  //}

  //cout << __func__ << " " << __LINE__ << ": tfg:\n" << tfg_ret->to_string(true) << endl;

  //map<unsigned, expr_ref> submap;//_consts;
  for (int i = 0; i < imm_vt_map.num_imms_used; ++i) {
    expr_ref c = cs.get_expr_value(reg_type_const, i);
    expr_ref to_c = imm_valtag_to_expr_ref(c->get_context(),
        &imm_vt_map.table[i], &cs);
    DYN_DEBUG(peep_tab, cout << __func__ << " " << __LINE__ << ": submap[" << expr_string(c) << "] = " << expr_string(to_c) << endl);
    submap[c->get_id()] = make_pair(c, to_c);
  }

  for (int i = 0; i < nextpc_map.num_nextpcs_used; i++) {
    expr_ref c = cs.get_expr_value(reg_type_nextpc_const, i);
    if (nextpc_map.table[i].tag == tag_const) {
      continue;
    }
    ASSERT(nextpc_map.table[i].tag == tag_var);
    int new_i = nextpc_map.table[i].val;
    ASSERT(new_i >= 0 && new_i < MAX_NUM_NEXTPCS);

    expr_ref to_c = cs.get_expr_value(reg_type_nextpc_const, new_i);
    DYN_DEBUG(peep_tab, cout << __func__ << " " << __LINE__ << ": submap[" << expr_string(c) << "] = " << expr_string(to_c) << endl);
    submap[c->get_id()] = make_pair(c, to_c);
  }

  //for (int i = 0; i < memlabel_map.num_entries; i++) {
  //  memlabel_t from_memlabel = { .m_type = MEMLABEL_VAR, .m_var_id = i, .m_size = MEMSIZE_MAX, .m_num_alias = 0};
  //  expr_ref ml = memlabel_to_expr_ref(ctx, &from_memlabel);
  //  expr_ref to_ml = memlabel_to_expr_ref(ctx, &memlabel_map.table[i]);
  //  submap[ml->get_id()] = make_pair(ml, to_ml);
  //}

  expr_ref from, to;
  //ASSERT(cmn_m_base_state.get_num_exreg_groups() > 0);
  for (const auto &kv : cmn_m_base_state.get_value_expr_map_ref()) {
    string const &key = kv.first->get_str();
    expr_ref const &val = kv.second;
    reg_identifier_t reg_id = reg_identifier_t::reg_identifier_invalid();
    exreg_group_id_t group;
    DYN_DEBUG(peep_tab, cout << __func__ << " " << __LINE__ << ": key = " << key << endl);
    if (state::key_represents_exreg_key(key, group, reg_id)) {
      reg_identifier_t const &mapping = regmap_get_elem(&regmap, group, reg_id);
      DYN_DEBUG(peep_tab, cout << __func__ << " " << __LINE__ << ": group = " << group << ", reg_id = " << reg_id.reg_identifier_to_string() << ", mapping = " << mapping.reg_identifier_to_string() << endl);
      if (!mapping.reg_identifier_is_unassigned()) {
        //from = cmn_m_base_state.get_expr_value(cmn_m_base_state, reg_type_exvreg, group, reg_id);
        from = val;
        if (mapping.reg_identifier_denotes_constant()) {
          valtag_t imm_valtag = mapping.reg_identifier_get_imm_valtag();
          to = imm_valtag_to_expr_ref(from->get_context(), &imm_valtag, &cs);
        } else {
          to = cmn_m_base_state.state_get_expr_value(/*cmn_m_base_state,*/ cmn_prefix, reg_type_exvreg, group, mapping.reg_identifier_get_id());
          ASSERT(to);
        }
        DYN_DEBUG(peep_tab, cout << __func__ << " " << __LINE__ << ": submap[" << expr_string(from) << "] = " << expr_string(to) << endl);
        submap[from->get_id()] = make_pair(from, to);
      }
    }
  }

  //expr_ref memfrom, memto;
  //bool ret;
  //ret = cmn_m_base_state.get_memory_expr(cmn_m_base_state, memfrom);
  //ASSERT(ret);
  //ret = cmn_m_base_state.get_memory_expr(cmn_m_base_state, memto);
  //ASSERT(ret);
  //submap[memfrom->get_id()] = make_pair(memfrom, memto);

  //from = cmn_m_base_state.get_expr_value(cmn_m_base_state, reg_type_contains_float_op, 0, 0);
  //to = cmn_m_base_state.get_expr_value(cmn_m_base_state, reg_type_contains_float_op, 0, 0);
  //submap[from->get_id()] = make_pair(from, to);

  //from = cmn_m_base_state.get_expr_value(cmn_m_base_state, reg_type_contains_unsupported_op, 0, 0);
  //to = cmn_m_base_state.get_expr_value(cmn_m_base_state, reg_type_contains_unsupported_op, 0, 0);
  //submap[from->get_id()] = make_pair(from, to);

  DYN_DEBUG2(peep_tab,
      cout << __func__ << " " << __LINE__ << ": submap = " << endl;
      for (auto sm : submap) {
        cout << expr_string(sm.second.first) << " --> " << expr_string(sm.second.second) << endl;
      }
  );
  DYN_DEBUG(peep_tab, cout << __func__ << " " << __LINE__ << ": before substitute_variables, tfg:\n" << tfg_ret->graph_to_string() << endl);
  tfg_ret->tfg_substitute_variables(submap, false);
  DYN_DEBUG(peep_tab, cout << __func__ << " " << __LINE__ << ": after substitute_variables, tfg:\n" << tfg_ret->graph_to_string() << endl);
  tfg_ret->eliminate_edges_with_false_pred();
  //cout << __func__ << " " << __LINE__ << ": tfg:\n" << tfg_ret->to_string(true) << endl;

  //cout << __func__ << " " << __LINE__ << ": tfg_ret:\n" << tfg_ret->to_string() << endl;

  //if (DISABLE_FUNCTION_CALL) {
  //  tfg_ret->disable_function_calls();
  //}

  list<dshared_ptr<tfg_edge const>> exiting_edges;
  tfg_ret->get_edges_exiting(exiting_edges);

  DYN_DEBUG(peep_tab, cout << __func__ << " " << __LINE__ << ": before renaming exit edges, tfg:\n" << tfg_ret->graph_to_string() << endl);
  DYN_DEBUG(peep_tab, cout << __func__ << " " << __LINE__ << ": nextpc_map =\n" << nextpc_map_to_string(&nextpc_map, as1, sizeof as1) << endl);

  for (auto e : exiting_edges) {
    tfg_ret->edge_rename_to_pc_using_nextpc_map(e, nextpc_map, cur_index/*, cmn_m_base_state*/);
  }
  //cout << __func__ << " " << __LINE__ << ": tfg:\n" << tfg_ret->to_string(true) << endl;
  DYN_DEBUG(peep_tab, cout << __func__ << " " << __LINE__ << ": after renaming exit edges, tfg:\n" << tfg_ret->graph_to_string() << endl);

  list<dshared_ptr<tfg_edge const>> edges, new_edges;
  tfg_ret->get_edges(edges);
  for (auto e : edges) {
    DYN_DEBUG(peep_tab, cout << "before reorder e->get_to_state(): " << e->get_to_state().state_to_string_for_eq() << endl << "regmap:\n" << regmap_to_string(&regmap, as1, sizeof as1) << endl);
    auto const& base_state = this->cmn_m_base_state;
    //ASSERT(e->tfg_edge_is_atomic());
    std::function<void (pc const&, state &)> f = [&base_state, &regmap, &e](pc const& p, state &st)
    {
      ASSERT(p == e->get_from_pc());
      st.reorder_registers_using_regmap(/*base_state,*/ regmap, base_state);
    };
    dshared_ptr<tfg_edge const> new_e = e->tfg_edge_update_state(f);
    new_edges.push_back(new_e);
    DYN_DEBUG(peep_tab, cout << "after reorder new_e->get_to_state(): " << new_e->get_to_state().state_to_string_for_eq() << endl);
  }
  DYN_DEBUG(peep_tab, cout << __func__ << " " << __LINE__ << ": after reorder, tfg:\n" << tfg_ret->graph_to_string() << endl);
  tfg_ret->set_edges(new_edges);

  for (auto e : exiting_edges) {
    if (!e->get_to_pc().is_exit()) {
      continue;
    }
    if (tfg_ret->find_node(e->get_to_pc())) {
      continue;
    }
    tfg_ret->add_node(make_dshared<tfg_node>(e->get_to_pc()));
  }
  DYN_DEBUG2(sym_exec, cout << __func__ << " " << __LINE__ << ": insn " << cur_index << cmn_iseq_to_string(&insn, 1, as1, sizeof as1) << ": before simplify: tfg_ret:\n" << tfg_ret->graph_to_string(/*true*/) << endl);
  //tfg_ret->simplify_edges();
  //CPP_DBG_EXEC2(sym_exec, cout << __func__ << " " << __LINE__ << ": insn " << cur_index << cmn_iseq_to_string(insn, 1, as1, sizeof as1) << ": after simplify: tfg_ret:\n" << tfg_ret->to_string(true) << endl);
  DYN_DEBUG(peep_tab, cout << __func__ << " " << __LINE__ << ": tfg:\n" << tfg_ret->graph_to_string() << endl);
  return tfg_ret;
}

dshared_ptr<tfg>
sym_exec::cmn_insn_get_tfg(cmn_insn_t const *insn, long cur_index, context *ctx)
{
  autostop_timer func_timer(__func__);
  vector<insn_db_match_t<cmn_insn_t, pair<streampos, dshared_ptr<tfg const>>>> matched = cmn_m_db.insn_db_get_all_matches(*insn, true);
  //auto iter = cmn_insn_get_next_matching_iterator(insn, cur_index, ctx, cmn_m_db.begin());
  if (matched.size() == 0/*iter == cmn_m_db.end()*/) {
    fprintf(stderr, "%s: No match found for:\n%s\n", __func__,
        cmn_iseq_to_string(insn, 1, as1, sizeof as1));
    NOT_REACHED();
  }
  /*return cmn_insn_get_tfg_matching_at_iter(insn, cur_index, ctx, iter);*/
  return cmn_insn_get_tfg_from_insn_db_match(*insn, matched.at(0), cur_index, ctx);
}

dshared_ptr<tfg>
sym_exec::cmn_get_tfg_full(cmn_insn_t const iseq[], long iseq_len, context* ctx, string const& name, string const& fname)
{
  consts_struct_t const &cs = ctx->get_consts_struct();
  dshared_ptr<tfg> tfg_ret;
  tfg_ret = make_dshared<tfg_asm_t>(name, fname, ctx/*, cmn_m_base_state*/);
  tfg_ret->set_start_state(cmn_m_base_state);

  for (long i = 0; i < iseq_len; i++) {
    dshared_ptr<tfg> itfg = cmn_insn_get_tfg(&iseq[i], i, ctx);
    DYN_DEBUG(sym_exec, cout << "itfg for insn " << i << ": '" << cmn_iseq_to_string(&iseq[i], 1, as1, sizeof as1) << "':\n" << itfg->graph_to_string() << endl);
    itfg->tfg_eliminate_hidden_regs();
    DYN_DEBUG(sym_exec, cout << "itfg for insn " << i << ": '" << cmn_iseq_to_string(&iseq[i], 1, as1, sizeof as1) << "':\n" << itfg->graph_to_string() << endl);
    //cout << "cur tfg:\n" << tfg_ret->to_string(true) << endl;
    //itfg->check_consistency_of_edges();
    tfg_ret->append_tfg(*itfg, pc(pc::insn_label, int_to_string(i).c_str(), PC_SUBINDEX_FIRST_INSN_IN_BB, PC_SUBSUBINDEX_DEFAULT), ctx);
    //itfg->check_consistency_of_edges();
    //tfg_ret->check_consistency_of_edges();
    DYN_DEBUG2(sym_exec, cout << "cur tfg after append:\n" << tfg_ret->graph_to_string() << endl);
    //delete itfg;
  }

  dshared_ptr<tfg> ret = tfg_asm_t::tfg_asm_copy(*tfg_ret); //so all redundant fields (e.g., m_outoing_edges_lookup, etc.) get filled up
  assert(ret);
  //delete tfg_ret;

  DYN_DEBUG(sym_exec, cout << __func__ << " " << __LINE__ << ": returning:\n" << ret->graph_to_string(/*true*/) << endl);

  return ret;
}

dshared_ptr<tfg>
sym_exec::cmn_get_tfg(cmn_insn_t const iseq[], long iseq_len, context *ctx, string const& name, string const& fname)
{
  dshared_ptr<tfg> tfg;

  assert(iseq_len > 0);

  tfg = cmn_get_tfg_full(iseq, iseq_len, ctx, name, fname);
  //tfg->set_locals_info(locals_info);

  return tfg;
}

void
sym_exec::cmn_add(cmn_insn_t* iseq, streampos off, dshared_ptr<tfg const> const &tfg)
{
  //cmn_m_db.push_back(make_pair(iseq, make_pair(off, tfg)));
  cmn_m_db.insn_db_add(*iseq, make_pair(off, tfg));
}
