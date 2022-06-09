#pragma once

namespace eqspace {

using namespace std;

template <typename T_PC>
string get_root_varname_for_ssa_renamed_var(string const& ssa_vname)
{
//  cout << _FNLN_ << ": " << varname << endl;
  string root_vname = ssa_vname;
  size_t dot = ssa_vname.find_last_of('.');
  while (dot != string::npos && T_PC::string_represents_pc(root_vname.substr(dot+1)).first) {
    root_vname = root_vname.substr(0, dot);
    dot = root_vname.find_last_of('.');
  }
//  cout << _FNLN_ << ": " << root_vname << endl;
  return root_vname;
}

template <typename T_PC>
bool string_has_pc_suffix(string const& varname)
{
  return get_root_varname_for_ssa_renamed_var<T_PC>(varname) != varname;
}

template <typename T_PC>
string get_ssa_renamed_varname(string const& varname, T_PC const& to_pc)
{
  ASSERT(!string_has_pc_suffix<T_PC>(varname));
  stringstream ss;
  ss << varname << "." << to_pc.to_string();
  return ss.str();
}

template <typename T_PC>
bool expr_represents_ssa_renamed_var(expr_ref const& e)
{
  if (!e->is_var()) {
    return false;
  }
  string vname = e->get_name()->get_str();
  return vname != get_root_varname_for_ssa_renamed_var<T_PC>(vname);
}

template <typename T_PC>
expr_ref get_ssa_renamed_var(expr_ref const& e, T_PC const& to_pc)
{
  if (!e->is_var())
    return nullptr;
  string varname = e->get_name()->get_str();
  if (expr_represents_ssa_renamed_var<T_PC>(e)) {
    varname = get_root_varname_for_ssa_renamed_var<T_PC>(varname);
  }
  varname = get_ssa_renamed_varname(varname, to_pc);
  return e->get_context()->mk_var(varname, e->get_sort());
}

}
