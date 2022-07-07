#pragma once

namespace eqspace {

using namespace std;

string sp_version_label_from_pc_string(string const& pc_str);
bool key_refers_to_sp_version(string_ref const& key);
string expr_sp_version_get_label(expr_ref const& e);

template<typename T_PC>
string get_key_for_sp_version_at_pc(T_PC const& p) { return sp_version_label_from_pc_string(p.to_string()); }

expr_ref get_sp_version_at_entry_for_addr_size(context* ctx, size_t addr_size);
expr_ref get_sp_version_at_entry(context* ctx);

bool expr_is_sp_version(expr_ref const& e);
bool expr_is_sp_version_at_entry(expr_ref const& e);

class graph_arg_regs_t;
bool expr_contains_only_constants_or_sp_versions(expr_ref const& e/*, graph_arg_regs_t const &argument_regs*/);
bool expr_contains_only_sp_versions(expr_ref const &e/*, graph_arg_regs_t const &argument_regs*/);

}
