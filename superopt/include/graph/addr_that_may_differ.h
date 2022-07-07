#pragma once

#include "graph/graph_cp_location.h"
#include "graph/graph_locs_map.h"

namespace eqspace {

using namespace std;

class addr_that_may_differ_t
{
private:
  expr_ref m_addr;
  memlabel_ref m_memlabel;
  size_t m_nbytes;
public:
  addr_that_may_differ_t(expr_ref const& addr, memlabel_ref const& ml, size_t nbytes) : m_addr(addr), m_memlabel(ml), m_nbytes(nbytes)
  { }
  expr_ref const& get_addr() const { return m_addr; }
  memlabel_ref const& get_memlabel() const { return m_memlabel; }
  size_t get_nbytes() const { return m_nbytes; }
  bool equals(addr_that_may_differ_t const& other) const { return m_addr == other.m_addr && m_memlabel == other.m_memlabel && m_nbytes == other.m_nbytes; }
  string to_string() const {
    stringstream ss;
    ss << "addr: " << expr_string(m_addr) << " m_memlabel: " << m_memlabel->get_ml().to_string() << " nbytes " << m_nbytes << endl;
    return ss.str();
  }
  addr_that_may_differ_t(istream& is, context* ctx) 
  {
    string line;
    bool end;

    end = !getline(is, line);
    ASSERT(!end);
    ASSERT(is_line(line, "=addr"));
    line = read_expr(is, m_addr, ctx);
    ASSERT(is_line(line, "=memlabel"));
    end = !getline(is, line);
    ASSERT(!end);
    memlabel_t ml;
    memlabel_t::memlabel_from_string(&ml, line.c_str());
    m_memlabel = mk_memlabel_ref(ml);
    end = !getline(is, line);
    ASSERT(!end);
    ASSERT(is_line(line, "=nbytes"));
    end = !getline(is, line);
    ASSERT(!end);
    istringstream (line) >> m_nbytes;
    ASSERT(m_nbytes > 0);
  }

  void addr_that_may_differ_to_stream(ostream& os) const
  {
    context* ctx = m_addr->get_context();
    os << "=addr\n";
    ctx->expr_to_stream(os, m_addr);
    os << "\n=memlabel\n";
    m_memlabel->get_ml().memlabel_to_stream(os);
    os << "\n=nbytes\n" << m_nbytes << endl;
  }
  static vector<addr_that_may_differ_t> compute_addrs_that_may_differ(memlabel_t const& ml, graph_locs_map_t const& locs, context *ctx);
};

}
