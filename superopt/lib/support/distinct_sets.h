#ifndef DISTINCT_SETS_H
#define DISTINCT_SETS_H
#include <boost/pending/disjoint_sets.hpp>

template<typename VALUE>
class distinct_set_t
{
private:
  typedef std::map<VALUE, size_t> rank_t;
  typedef std::map<VALUE, VALUE> parent_t;
  typedef boost::associative_property_map<rank_t> arank_t;
  typedef boost::associative_property_map<parent_t> aparent_t;
  rank_t m_rank_map_helper;
  parent_t m_parent_map_helper;
  arank_t m_rank_map;//map used to store the rank/size of the set	(see log0's answer at https://stackoverflow.com/questions/4134703/understanding-boostdisjoint-sets)
  aparent_t m_parent_map;
  boost::disjoint_sets<arank_t, aparent_t> m_dsets;
public:
  distinct_set_t() : m_rank_map(m_rank_map_helper), m_parent_map(m_parent_map_helper), m_dsets(m_rank_map, m_parent_map)
  {
  }
  void distinct_set_make(VALUE const &v)
  {
    m_dsets.make_set(v);
  }
  void distinct_set_union(VALUE const &v1, VALUE const &v2)
  {
    m_dsets.union_set(v1, v2);
  }
  VALUE distinct_set_find(VALUE const &v) const
  {
    auto &dsets = const_cast<boost::disjoint_sets<arank_t, aparent_t> &>(m_dsets);
    return dsets.find_set(v); //find_set can modify dsets (path compression during find in union-find algo); but we pretend that it does not (because logically it does not)
  }
  void clear()
  {
    m_rank_map_helper.clear();
    m_parent_map_helper.clear();
    m_rank_map = arank_t(m_rank_map_helper);
    m_parent_map = aparent_t(m_parent_map_helper);
    m_dsets = boost::disjoint_sets<arank_t, aparent_t>(m_rank_map, m_parent_map);
  }
};

#endif
