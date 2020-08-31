
class bv_rank_val_t
{

private:
  unsigned int m_num_dst_exprs_with_no_predicate;
  unsigned int m_num_src_dst_exprs_with_no_predicate;
  unsigned int m_tot_num_bv_eq_exprs;

public:

  bv_rank_val_t(unsigned int dst_rank, unsigned int dst_src_rank, unsigned int tot_exprs):
    m_num_dst_exprs_with_no_predicate(dst_rank), m_num_src_dst_exprs_with_no_predicate(dst_src_rank), m_tot_num_bv_eq_exprs(tot_exprs)
    {}
  bv_rank_val_t():
    m_num_dst_exprs_with_no_predicate(0), m_num_src_dst_exprs_with_no_predicate(0), m_tot_num_bv_eq_exprs(0)
    {}

  unsigned int get_dst_rank() const { return m_num_dst_exprs_with_no_predicate;}
  unsigned int get_src_rank() const { return m_num_src_dst_exprs_with_no_predicate;}
  bv_rank_val_t operator+(bv_rank_val_t const& rhs) const
  {
    return bv_rank_val_t((this->m_num_dst_exprs_with_no_predicate + rhs.m_num_dst_exprs_with_no_predicate), (this->m_num_src_dst_exprs_with_no_predicate + rhs.m_num_src_dst_exprs_with_no_predicate), (this->m_tot_num_bv_eq_exprs + rhs.m_tot_num_bv_eq_exprs));
  }
  string to_string() const
  {
    stringstream ss;
    ss << " m_dst_rank: " << this->get_dst_rank() << " , m_src_rank: " << this->get_src_rank() << '\n';
    return ss.str();
  }
};
