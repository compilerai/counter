#pragma once

struct peep_entry_t;

class peep_match_attrs_t
{
private:
  transmap_t m_in_tmap;
  vector<transmap_t> m_out_tmaps;
  regcount_t m_touch_not_live_in;
  struct peep_entry_t const *m_peep_entry_id;
  cost_t m_peep_match_cost;
public:
  peep_match_attrs_t(transmap_t const &in_t, vector<transmap_t> const &out_t, regcount_t const &l, struct peep_entry_t const *id, cost_t peep_match_cost) : m_in_tmap(in_t), m_out_tmaps(out_t), m_touch_not_live_in(l), m_peep_entry_id(id), m_peep_match_cost(peep_match_cost)
  {
  }
  transmap_t const &get_in_tmap() const { return m_in_tmap; }
  vector<transmap_t> const &get_out_tmaps() const { return m_out_tmaps; }
  regcount_t const &get_touch_not_live_in() const { return m_touch_not_live_in; }
  struct peep_entry_t const *get_peep_entry_id() const { return m_peep_entry_id; }
  cost_t get_peep_match_cost() const { return m_peep_match_cost; }
  bool match_and_update_transmaps_and_touch(transmap_t const &in_t, vector<transmap_t> const &out_t, regcount_t const &l, struct peep_entry_t const *id, cost_t cost)
  {
    if (!transmaps_equal(&m_in_tmap, &in_t)) {
      return false;
    }
    ASSERT(m_out_tmaps.size() == out_t.size());
    for (size_t i = 0; i < m_out_tmaps.size(); i++) {
      if (!transmaps_equal(&m_out_tmaps.at(i), &out_t.at(i))) {
        return false;
      }
    }
    if (!regcounts_equal(&m_touch_not_live_in, &l)) {
      return false;
    }
    if (m_peep_match_cost > cost) {
      m_peep_match_cost = cost;
      m_peep_entry_id = id;
    }
    return true;
  }
};
