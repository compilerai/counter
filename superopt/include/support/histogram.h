#pragma once

#include "support/debug.h"
#include "support/c_utils.h"

using namespace std;

enum class HistogramKind { LINEAR, LOG };

template<typename It, typename Comp, typename Getter>
class Histogram
{
  using Ty = double;
  HistogramKind const m_kind;
  It m_begin;
  It m_end;

  Ty m_min_v;
  Ty m_max_v;
  Ty m_range;
  Ty m_base;
  vector<Ty> m_hist;
  vector<Ty> m_acc;

  void init_scale(It begin, It end, Comp cmp, Getter getter, unsigned buckets)
  {
    auto [min_itr,max_itr] = minmax_element(m_begin, m_end, cmp);
    m_min_v = getter(min_itr);
    m_max_v = getter(max_itr);
    if (m_kind == HistogramKind::LINEAR) {
      m_range = DIV_ROUND_UP(m_max_v-m_min_v+1, buckets);
    } else {
      ASSERT(m_kind == HistogramKind::LOG);
      m_base = exp(log((m_max_v+1)/m_min_v)/buckets);
    }
  }

  unsigned get_bucket(Ty ent) const
  {
    if (m_kind == HistogramKind::LINEAR) {
      return (ent - m_min_v)/m_range;
    } else {
      return (log(ent)-log(m_min_v))/log(m_base);
    }
  }
  Ty get_start(unsigned i) const
  {
    if (m_kind == HistogramKind::LINEAR) {
      return m_min_v+i*m_range;
    } else {
      return m_min_v*pow(m_base,i);
    }
  }

public:
  Histogram(HistogramKind kind, It begin, It end, Comp cmp, Getter getter, unsigned num_buckets = 10)
    : m_kind(kind),
      m_begin(begin), m_end(end),
      m_hist(num_buckets, 0),
      m_acc(num_buckets, 0)
  {
    init_scale(m_begin, m_end, cmp, getter, num_buckets);
    for (auto itr = m_begin; itr != m_end; ++itr) {
      Ty ent = getter(itr);
      unsigned slot = get_bucket(ent);
      m_hist[slot]++;
      m_acc[slot] += ent;
    }
  }

  Ty get_min_value() const { return m_min_v; }
  Ty get_max_value() const { return m_max_v; }

  string to_string(function<string(Ty)> fmt, string const& prefix = "histogram:")
  {
    ostringstream ss;
    ss << prefix << '\n';
    for (size_t i = 0; i < m_hist.size(); ++i) {
      auto low = get_start(i);
      auto next_low = get_start(i+1);
      auto avg = m_hist[i] != 0 ? m_acc[i]/m_hist[i] : 0;

      ostringstream os;
      os << "[" << fmt(low) << ", " << fmt(next_low) << ") (avg. " << fmt(avg) << ')';
      ss << pad_string(os.str(), 40) << " : " << m_hist[i] << '\n';
    }
    return ss.str();
  }
};
