#undef DEBUG_TYPE
#define DEBUG_TYPE "graph_cp"

#include "support/types.h"
#include "expr/expr.h"
#include "expr/context.h"
#include "graph/graph_cp_location.h"
#include "expr/expr_utils.h"
#include "support/debug.h"
#include "expr/solver_interface.h"
#include "expr/consts_struct.h"


static char as1[40960];

namespace eqspace {

string
graph_cp_location::to_string() const
{
  string ret;
  if (m_type == GRAPH_CP_LOCATION_TYPE_REGMEM) {
    //ASSERT(m_reg_type == reg_type_exvreg);
    //ret = G_REGULAR_REG_NAME;
    //ret += boost::lexical_cast<std::string>(m_reg_index_i) + "." + boost::lexical_cast<std::string>(m_reg_index_j);
    ret = m_varname->get_str();
    ASSERT(m_var->is_var());
    //ret = expr_string(m_var);
  } else if (m_type == GRAPH_CP_LOCATION_TYPE_LLVMVAR) {
    //ret = string("llvmvar.") + expr_string(m_addr);
    //ASSERT(string_has_prefix(m_varname, G_LLVM_PREFIX "-"));
    ret = m_varname->get_str();
    ASSERT(m_var->is_var());
    //ret = expr_string(m_var);
  /*} else if (m_type == GRAPH_CP_LOCATION_TYPE_IO) {
    ret = string("IO.") + expr_string(m_addr);*/
  } else if (m_type == GRAPH_CP_LOCATION_TYPE_MEMSLOT) {
    ret = reg_type_to_string(m_reg_type) + "." + boost::lexical_cast<std::string>(m_reg_index_i) + ".SLOT[" + m_memname->get_str() + "+" + m_memlabel->get_ml().to_string() + ", " + expr_string(m_addr) + ", " + boost::lexical_cast<std::string>(m_nbytes) + "]";
  } else if (m_type == GRAPH_CP_LOCATION_TYPE_MEMMASKED) {
    ret = reg_type_to_string(m_reg_type) + "." + boost::lexical_cast<std::string>(m_reg_index_i) + ".MASKED[" + m_memname->get_str() + ", " + memlabel_t::memlabel_to_string(&m_memlabel->get_ml(), as1, sizeof as1) + ", " + expr_string(m_addr) + ", " + boost::lexical_cast<std::string>(m_nbytes) + "]";
  }
  return ret;
}

string
graph_cp_location::to_string_for_eq() const
{
  string ret;
  if (m_type == GRAPH_CP_LOCATION_TYPE_REGMEM) {
    //ASSERT(m_reg_type == reg_type_exvreg);
    //ret = G_REGULAR_REG_NAME;
    //ret += boost::lexical_cast<std::string>(m_reg_index_i) + "." + boost::lexical_cast<std::string>(m_reg_index_j);
    //ret = m_varname->get_str();
    context* ctx = m_var->get_context();
    ret = string("REGMEM\n") + m_varname->get_str() + "\n" + ctx->expr_to_string_table(m_var);
  } else if (m_type == GRAPH_CP_LOCATION_TYPE_LLVMVAR) {
    //ASSERT(string_has_prefix(m_varname, G_LLVM_PREFIX "-"));
    //ret = m_varname->get_str();
    context* ctx = m_var->get_context();
    ret = string("LLVMVAR\n") + m_varname->get_str() + "\n" + ctx->expr_to_string_table(m_var);
  /*} else if (m_type == GRAPH_CP_LOCATION_TYPE_IO) {
    context *ctx = m_addr->get_context();
    ret = string("IO\n") + ctx->expr_to_string_table(m_addr);*/
  } else if (m_type == GRAPH_CP_LOCATION_TYPE_MEMSLOT) {
    context *ctx = m_addr->get_context();
    ret = string("SLOT\n=memname\n") + m_memname->get_str() + "\n=addr\n" + ctx->expr_to_string_table(m_addr) + "\n=nbytes\n" + boost::lexical_cast<std::string>(m_nbytes) + "\n=bigendian\n" + (m_bigendian ? "true" : "false");
  } else if (m_type == GRAPH_CP_LOCATION_TYPE_MEMMASKED) {
    //ret = reg_type_to_string(m_reg_type) + "." + boost::lexical_cast<std::string>(m_reg_index_i) + ".MASKED[" + memlabel_to_string(&m_memlabel, as1, sizeof as1) + ", " + expr_string(m_addr) + ", " + boost::lexical_cast<std::string>(m_nbytes) + "]";
    //cout << __func__ << " " << __LINE__ << ": m_memlabel = " << memlabel_to_string(&m_memlabel, as1, sizeof as1) << endl;
    ret = string("MASKED\n=memname\n") + m_memname->get_str() + "\n=memlabel\n" + memlabel_t::memlabel_to_string(&m_memlabel->get_ml(), as1, sizeof as1);
  }
  return ret;
}

bool
graph_cp_location::equals_mod_comment(graph_cp_location const &other) const
{
  if (m_type != other.m_type) {
    return false;
  }
  if (   m_type == GRAPH_CP_LOCATION_TYPE_LLVMVAR
      || m_type == GRAPH_CP_LOCATION_TYPE_REGMEM) {
    //return m_varname == other.m_varname;
    return m_var == other.m_var;
  }
  //if (m_type == GRAPH_CP_LOCATION_TYPE_IO) {
  //  if (!is_expr_equal_syntactic(m_addr, other.m_addr)) {
  //    //cout << __func__ << " " << __LINE__ << endl;
  //    return false;
  //  }
  //  return true;
  //}

  if (m_memname != other.m_memname) {
    return false;
  }
  if (m_type == GRAPH_CP_LOCATION_TYPE_MEMMASKED) {
    if (m_memlabel !=  other.m_memlabel) {
      return false;
    }
    return true;
  }
  ASSERT(m_type == GRAPH_CP_LOCATION_TYPE_MEMSLOT);
  if (m_nbytes != other.m_nbytes) {
    return false;
  }
  if (m_bigendian != other.m_bigendian) {
    return false;
  }
  if (!is_expr_equal_syntactic(m_addr, other.m_addr)) {
    return false;
  }
  return true;
}

bool
graph_cp_location::graph_cp_location_represents_hidden_var(context* ctx) const
{
  if (this->m_type != GRAPH_CP_LOCATION_TYPE_REGMEM) {
    return false;
  }
  return ctx->key_represents_hidden_var(this->m_varname);
}

}
