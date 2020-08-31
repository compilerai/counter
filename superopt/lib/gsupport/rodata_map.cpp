#include "gsupport/rodata_map.h"
#include "expr/sprel_map_pair.h"
//#include "graph/tfg.h"
#include "expr/expr.h"
#include "expr/solver_interface.h"
#include "support/debug.h"

namespace eqspace {

rodata_map_t::rodata_map_t(istream& in)
{
  string line;
  bool end;
  end = !getline(in, line);
  ASSERT(!end);
  ASSERT(line == "=rodata_map begin");
  end = !getline(in, line);
  ASSERT(!end);
  //cout << __func__ << " " << __LINE__ << ": line = '" << line << "'\n";
  string const prefix = "C_SYMBOL";
  while (string_has_prefix(line, prefix)) {
    istringstream iss(line.substr(prefix.length()));
    symbol_id_t symbol_id;
    iss >> symbol_id;
    size_t colon = line.find(':');
    ASSERT(colon != string::npos);
    string addrs = line.substr(colon + 1);
    vector<string> addrs_vec = splitOnChar(addrs, ',');
    for (string addr_str : addrs_vec) {
      trim(addr_str);
      size_t colon = addr_str.find(':');
      ASSERT(colon != string::npos);
      string symbol_id_str = addr_str.substr(0, colon);
      string offset_str = addr_str.substr(colon + 1);
      symbol_id_t rodata_symbol_id = string_to_int(symbol_id_str, -1);
      ASSERT(rodata_symbol_id != -1);
      src_ulong addr = string_to_int(offset_str, -1);
      ASSERT(addr != (src_ulong)-1);
      m_map[symbol_id].insert(rodata_offset_t(rodata_symbol_id, addr));
    }
    end = !getline(in, line);
    ASSERT(!end);
  }
  ASSERT(line == "=rodata_map end");
}

void
rodata_map_t::rodata_map_to_stream(ostream& ss) const
{
  ss << "=rodata_map begin\n";
  for (auto rm : m_map) {
    ss << "C_SYMBOL" << rm.first;
    bool first = true;
    for (auto symaddr : rm.second) {
      ss << (first ? ": " : ", ") << symaddr.rodata_offset_to_string();
      first = false;
    }
    ss << endl;
  }
  ss << "=rodata_map end\n";
}

string
rodata_map_t::to_string_for_eq() const
{
  stringstream ss;
  rodata_map_to_stream(ss);
  return ss.str();
}

/*
map<expr_id_t, pair<expr_ref, expr_ref>>
rodata_map_t::rodata_map_get_submap(context *ctx, tfg const& src_tfg) const
{
  autostop_timer func_timer(__func__);
  consts_struct_t const& cs = ctx->get_consts_struct();
  map<expr_id_t, pair<expr_ref, expr_ref>> rename_submap;
  for (auto rm : m_map) {
    ASSERT(rm.second.size() >= 1);
    if (rm.second.size() > 1) {
      cout << "\n======== WARNING ========\n\n";
      cout << "Looks like you have multiple possibilities for RODATA symbol C_SYMBOL" << rm.first << ": ";
      for (auto const& symaddr : rm.second) { cout << symaddr.rodata_offset_to_string() << ", "; }
      cout << '\n';
      cout << "A possible fix is to replace the corresponding string in the C source\n";
      cout << "The symbol name in LLVM is " << src_tfg.get_symbol_map().at(rm.first).get_name()->get_str() << endl;
      cout << "\n======== WARNING ========\n\n";
      cout.flush();
      NOT_IMPLEMENTED();
    }
    // pick the first and return it
    expr_ref from = cs.get_expr_value(reg_type_symbol, rm.first);
    expr_ref to = rm.second.begin()->get_expr(ctx);
    rename_submap.insert(make_pair(from->get_id(), make_pair(from, to)));
    //tfg::update_symbol_rename_submap(rename_submap, rm.first, SYMBOL_ID_DST_RODATA, *rm.second.begin(), ctx);
  }
  return rename_submap;
}
*/

bool
rodata_map_t::prune_using_expr_pair(expr_ref const &src, expr_ref const &dst)
{
  expr_find_op src_fcalls_visitor(src, expr::OP_FUNCTION_CALL);
  expr_find_op dst_fcalls_visitor(dst, expr::OP_FUNCTION_CALL);
  expr_vector src_fcalls = src_fcalls_visitor.get_matched_expr();
  expr_vector dst_fcalls = dst_fcalls_visitor.get_matched_expr();
  context *ctx = src->get_context();
  consts_struct_t const &cs = ctx->get_consts_struct();
  /* failure if no function call in src or dst expr */
  if (src_fcalls.size() == 0 || dst_fcalls.size() == 0) {
    return false;
  }
  ASSERT(src_fcalls.size() > 0);
  ASSERT(dst_fcalls.size() > 0);
  /* XXX: looking only at the first function call */
  expr_ref src_fcall = src_fcalls.at(0);
  expr_ref dst_fcall = dst_fcalls.at(0);
  ASSERT(src_fcall->get_operation_kind() == expr::OP_FUNCTION_CALL);
  ASSERT(dst_fcall->get_operation_kind() == expr::OP_FUNCTION_CALL);
  expr_vector src_fcall_args = src_fcall->get_args();
  expr_vector dst_fcall_args = dst_fcall->get_args();
  /* mismatched function call */
  if (src_fcall_args.size() != dst_fcall_args.size()) {
    return false;
  }
  for (size_t i = OP_FUNCTION_CALL_ARGNUM_FARG0; i < src_fcall_args.size(); i++) {
    expr_ref src_arg = src_fcall_args.at(i);
    if (!cs.is_symbol(src_arg)) {
      continue;
    }
    symbol_id_t symbol_id = cs.get_symbol_id_from_expr(src_arg);
    if (m_map.count(symbol_id) == 0) {
      continue;
    }
    expr_ref dst_arg = dst_fcall_args.at(i);
    set<rodata_offset_t> rodata_offsets = m_map.at(symbol_id); // potential offsets for this symbol
    set<rodata_offset_t> pruned_rodata_offsets;
    //expr_ref symbol_dst_rodata_const = cs.get_expr_value(reg_type_symbol, SYMBOL_ID_DST_RODATA);
    for (auto const& rodata_offset : rodata_offsets) {
      expr_ref addr = rodata_offset.get_expr(ctx);
      addr = ctx->expr_do_simplify(addr);
      if (addr == dst_arg) {
        pruned_rodata_offsets.insert(rodata_offset);
      }
    }
    if (pruned_rodata_offsets.size() == 0) {
      m_map.erase(symbol_id); // no matching offset found
    } else {
      m_map[symbol_id] = pruned_rodata_offsets;
    }
    //cout << __func__ << " " << __LINE__ << ": symbol_id = " << symbol_id << endl;
    //cout << __func__ << " " << __LINE__ << ": dst_arg = " << expr_string(dst_arg) << endl;
  }
  return true;
}

void
rodata_map_t::rodata_map_add_mapping(symbol_id_t symbol_id, set<rodata_offset_t> const &sym_addrs)
{
  m_map[symbol_id] = sym_addrs;
}

void
rodata_map_t::rodata_map_add_mapping(symbol_id_t symbol_id, rodata_offset_t const& sym_addr)
{
  if (m_map.count(symbol_id)) {
    m_map[symbol_id].insert(sym_addr);
  } else {
    set<rodata_offset_t> sym_addr_set;
    sym_addr_set.insert(sym_addr);
    m_map[symbol_id] = sym_addr_set;
  }
}

void
rodata_map_t::prune_non_const_symbols(graph_symbol_map_t const& symbol_map)
{
  for (auto iter = m_map.begin(); iter != m_map.end(); ) {
    ASSERT(symbol_map.count(iter->first));
    if (!symbol_map.at(iter->first).is_const()) { // not a const
      iter = m_map.erase(iter);
    }
    else {
      ++iter;
    }
  }
}

expr_ref
rodata_offset_t::get_expr(context* ctx) const
{
  consts_struct_t const& cs = ctx->get_consts_struct();
  expr_ref symbol_dst_rodata_const = cs.get_expr_value(reg_type_symbol, m_symbol_id);
  expr_ref addr = ctx->mk_bvadd(symbol_dst_rodata_const, ctx->mk_bv_const(DWORD_LEN, (int)m_offset));
  return addr;
}

expr_ref
rodata_map_t::get_assertions(context* ctx) const
{
  consts_struct_t const& cs = ctx->get_consts_struct();
  expr_ref ret = expr_true(ctx);
  for (auto const& sym_entry : m_map) {
    if (sym_entry.second.size() > 1) {
      cout << "\n======== WARNING ========\n\n";
      cout << "Looks like you have multiple possibilities for RODATA symbol C_SYMBOL" << sym_entry.first << ": ";
      for (auto const& symaddr : sym_entry.second) { cout << symaddr.rodata_offset_to_string() << ", "; }
      cout << '\n';
      cout << "A possible fix is to replace the corresponding string in the C source\n";
      //cout << "The symbol name in LLVM is " << src_tfg.get_symbol_map().at(rm.first).get_name()->get_str() << endl;
      cout << "\n======== WARNING ========\n\n";
      cout.flush();
    }
    ASSERT(sym_entry.second.size() == 1);
    expr_ref sym = cs.get_expr_value(reg_type_symbol, sym_entry.first);
    //expr_ref sret = expr_false(ctx);
    //expr_ref sym = cs.get_expr_value(reg_type_symbol, sym_entry.first);
    //for (auto const& ro_off : sym_entry.second) {
    //  sret = expr_or(sret, expr_eq(sym, ro_off.get_expr(ctx)));
    //}
    //ret = expr_and(ret, sret);
    ret = expr_and(ret, expr_eq(sym, sym_entry.second.begin()->get_expr(ctx)));
  }
  return ret;
}

}
