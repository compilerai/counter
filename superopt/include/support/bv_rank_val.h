#pragma once

#include "support/utils.h"
#include "support/debug.h"
#include "support/dyn_debug.h"

#include "expr/expr.h"
#include "expr/context.h"

class bv_rank_exprs_t
{
private:
  set<expr_ref> m_src_ranking_exprs;
  set<expr_ref> m_src_tie_breaking_exprs;
  set<expr_ref> m_dst_ranking_exprs;
  set<expr_ref> m_dst_tie_breaking_exprs;

public:
  bv_rank_exprs_t(set<expr_ref> const& src_ranking_exprs, set<expr_ref> const& src_tie_breaking_exprs, set<expr_ref> const& dst_ranking_exprs, set<expr_ref> const& dst_tie_breaking_exprs):
    m_src_ranking_exprs(src_ranking_exprs), m_src_tie_breaking_exprs(src_tie_breaking_exprs), m_dst_ranking_exprs(dst_ranking_exprs), m_dst_tie_breaking_exprs(dst_tie_breaking_exprs)
    {}
  set<expr_ref> const& get_src_ranking_exprs() const { return m_src_ranking_exprs;}
  set<expr_ref> const& get_src_tie_breaking_exprs() const { return m_src_tie_breaking_exprs;}
  set<expr_ref> const& get_dst_ranking_exprs() const { return m_dst_ranking_exprs;}
  set<expr_ref> const& get_dst_tie_breaking_exprs() const { return m_dst_tie_breaking_exprs;}
  
  void bv_rank_exprs_to_stream(ostream& os) const
  {
    int i = 0;
    for (auto const& e : m_src_ranking_exprs) {
      os << "=SRC Ranking expr " << i << endl;
      i++;
      context* ctx = e->get_context();
      ctx->expr_to_stream(os, e); 
      os << "\n";
    }
    i = 0;
    for (auto const& e : m_src_tie_breaking_exprs) {
      os << "=SRC Tie Breaking expr " << i << endl;
      i++;
      context* ctx = e->get_context();
      ctx->expr_to_stream(os, e); 
      os << "\n";
    }
    i = 0;
    for (auto const& e : m_dst_ranking_exprs) {
      os << "=DST Ranking expr " << i << endl;
      i++;
      context* ctx = e->get_context();
      ctx->expr_to_stream(os, e); 
      os << "\n";
    }
    i = 0;
    for (auto const& e : m_dst_tie_breaking_exprs) {
      os << "=DST Tie Breaking expr " << i << endl;
      i++;
      context* ctx = e->get_context();
      ctx->expr_to_stream(os, e); 
      os << "\n";
    }
    os << "=bv_rank_exprs done\n";
  }
  
  static bv_rank_exprs_t bv_rank_exprs_from_stream(istream& is/*, string &next_line*/, context* ctx)
  {
    string line;
    bool end;

    end = !getline(is, line);
    ASSERT(!end);
    set<expr_ref> src_ranking_exprs;
    string prefix1 = "=SRC Ranking expr";
    while (string_has_prefix(line, prefix1)) {
      expr_ref e;
      line = read_expr(is, e, ctx);
      src_ranking_exprs.insert(e);
    }
    set<expr_ref> src_tie_breaking_exprs;
    prefix1 = "=SRC Tie Breaking expr";
    while (string_has_prefix(line, prefix1)) {
      expr_ref e;
      line = read_expr(is, e, ctx);
      src_tie_breaking_exprs.insert(e);
    }
    set<expr_ref> dst_ranking_exprs;
    prefix1 = "=DST Ranking expr";
    while (string_has_prefix(line, prefix1)) {
      expr_ref e;
      line = read_expr(is, e, ctx);
      dst_ranking_exprs.insert(e);
    }
    set<expr_ref> dst_tie_breaking_exprs;
    prefix1 = "=DST Tie Breaking expr";
    while (string_has_prefix(line, prefix1)) {
      expr_ref e;
      line = read_expr(is, e, ctx);
      dst_tie_breaking_exprs.insert(e);
    }
    string next_line = line;
    if (next_line != "=bv_rank_exprs done") {
      cout << _FNLN_ << ": next_line =\n" << next_line << ".\n";
    }
    ASSERT(next_line == "=bv_rank_exprs done");

    return bv_rank_exprs_t(src_ranking_exprs, src_tie_breaking_exprs, dst_ranking_exprs, dst_tie_breaking_exprs);
  }

  string bv_rank_exprs_to_string() const
  {
    stringstream ss;
    bv_rank_exprs_to_stream(ss);
    return ss.str();
  }

};

class bv_rank_val_t
{

private:
  unsigned int m_num_dst_exprs_with_no_predicate;
  unsigned int m_num_src_dst_exprs_with_no_predicate;
  //unsigned int m_tot_num_bv_eq_exprs;

public:

  bv_rank_val_t(unsigned int dst_rank, unsigned int dst_src_rank/*, unsigned int tot_exprs*/):
    m_num_dst_exprs_with_no_predicate(dst_rank), m_num_src_dst_exprs_with_no_predicate(dst_src_rank)//, m_tot_num_bv_eq_exprs(tot_exprs)
    {}
  bv_rank_val_t():
    m_num_dst_exprs_with_no_predicate(0), m_num_src_dst_exprs_with_no_predicate(0)//, m_tot_num_bv_eq_exprs(0)
    {}

  bv_rank_val_t(bv_rank_val_t const& other):
    m_num_dst_exprs_with_no_predicate(other.m_num_dst_exprs_with_no_predicate), m_num_src_dst_exprs_with_no_predicate(other.m_num_src_dst_exprs_with_no_predicate)
    {}
  unsigned int get_dst_rank() const { return m_num_dst_exprs_with_no_predicate;}
  unsigned int get_src_rank() const { return m_num_src_dst_exprs_with_no_predicate;}
  bv_rank_val_t operator+(bv_rank_val_t const& rhs) const
  {
    return bv_rank_val_t((this->m_num_dst_exprs_with_no_predicate + rhs.m_num_dst_exprs_with_no_predicate), (this->m_num_src_dst_exprs_with_no_predicate + rhs.m_num_src_dst_exprs_with_no_predicate)/*, (this->m_tot_num_bv_eq_exprs + rhs.m_tot_num_bv_eq_exprs)*/);
  }
  bv_rank_val_t operator-(bv_rank_val_t const& rhs) const
  {
    return bv_rank_val_t((this->m_num_dst_exprs_with_no_predicate - rhs.m_num_dst_exprs_with_no_predicate), (this->m_num_src_dst_exprs_with_no_predicate - rhs.m_num_src_dst_exprs_with_no_predicate)/*, (this->m_tot_num_bv_eq_exprs - rhs.m_tot_num_bv_eq_exprs)*/);
  }
  //bool is_less(bv_rank_val_t const& other) const
  //{
  //  if (this->get_dst_rank() != other.get_dst_rank()) {
  //    return this->get_dst_rank() < other.get_dst_rank();
  //  }
  //  if (this->get_src_rank() != other.get_src_rank()) {
  //    return this->get_src_rank() < other.get_src_rank();
  //  }
  //  return 0;
  //}

  void bv_rank_val_to_stream(ostream& os) const
  {
    os << "m_dst_rank: " << this->get_dst_rank() << ", m_src_rank: " << this->get_src_rank() << '\n';
  }
  static bv_rank_val_t bv_rank_val_from_stream(istream& is)
  {
    string line;
    bool end;

    end = !getline(is, line);
    ASSERT(!end);

    ASSERT(string_has_prefix(line, "m_dst_rank: "));
    size_t dst_rank_start = strlen("m_dst_rank: ");
    size_t comma = line.find(',');
    ASSERT(comma != string::npos);
    unsigned dst_rank = string_to_int(line.substr(dst_rank_start, comma - dst_rank_start));
    ASSERT(string_has_prefix(line.substr(comma + 1), " m_src_rank: "));
    size_t src_rank_start = comma + 1 + strlen(" m_src_rank: ");
    size_t newline = line.find('\n', src_rank_start);
    unsigned src_rank = string_to_int(line.substr(src_rank_start, newline - src_rank_start));

    return bv_rank_val_t(dst_rank, src_rank);
  }

  string bv_rank_val_to_string() const
  {
    stringstream ss;
    bv_rank_val_to_stream(ss);
    return ss.str();
  }
};
