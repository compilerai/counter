#include <sstream>
#include "support/scatter_gather.h"

scatter_gather_t::scatter_gather_t(size_t size)
{
  this->set_size(size);
}

scatter_gather_t::scatter_gather_t(vector<bool> const& bitmap)
{
  this->set(bitmap.begin(), bitmap.end());
}

scatter_gather_t::scatter_gather_t(vector<size_t> const& idx)
: scatter_gather_t(idx.begin(), idx.end())
{ }

scatter_gather_t::scatter_gather_t(vector<size_t>::const_iterator const& begin, vector<size_t>::const_iterator const& end)
{
  auto itr = begin;
  while (itr != end) {
    m_idx.push_back(*itr);
    ++itr;
  }
}

scatter_gather_t::scatter_gather_t(list<size_t> const& idx)
: scatter_gather_t(idx.begin(), idx.end())
{ }

scatter_gather_t::scatter_gather_t(list<size_t>::const_iterator const& begin, list<size_t>::const_iterator const& end)
{
  auto itr = begin;
  while (itr != end) {
    m_idx.push_back(*itr);
    ++itr;
  }
}

scatter_gather_t::scatter_gather_t(vector<bool>::const_iterator const& begin, vector<bool>::const_iterator const& end)
{
  this->set(begin, end);
}

void
scatter_gather_t::swap(vector<size_t>& idx_vec)
{
  m_idx.swap(idx_vec);
}

vector<bool>
scatter_gather_t:: as_bitmap() const
{
  vector<bool> ret(m_idx.back(), false);
  for (size_t i = 0; i < m_idx.size(); ++i) {
    ret[m_idx[i]] = true;
  }
  return ret;
}

void
scatter_gather_t::set(vector<bool>::const_iterator const& begin, vector<bool>::const_iterator const& end)
{
  m_idx.clear();
  size_t i = 0;
  for (auto itr = begin; itr != end; ++itr) {
    if (*itr) m_idx.push_back(i);
    ++i;
  }

}

scatter_gather_t
scatter_gather_t::get_subindex(size_t l) const
{
  return scatter_gather_t(m_idx.begin()+l, m_idx.end());
}

scatter_gather_t
scatter_gather_t::get_subindex(size_t l, size_t r) const
{
  ASSERT(l <= r);
  ASSERT(m_idx.begin()+l <= m_idx.end());

  return scatter_gather_t(m_idx.begin()+l, m_idx.begin()+r);
}

size_t
scatter_gather_t::find(size_t v) const
{
  auto new_idx = lower_bound(m_idx.begin(), m_idx.end(), v);
  return new_idx - m_idx.begin();
}

void
scatter_gather_t::set_size(size_t size)
{
  m_idx.resize(size);
  for (size_t i = 0; i < size; ++i)
    m_idx[i] = i;
}

void
scatter_gather_t::update(vector<bool> const& bitmap)
{
  ASSERT(bitmap.size() == m_idx.size());
  vector<size_t> new_idx;
  for (size_t i = 0; i < bitmap.size(); ++i)
    if (bitmap[i])
      new_idx.push_back(m_idx[i]);
  new_idx.swap(m_idx);
}

string
scatter_gather_t::to_string() const
{
  stringstream ss;
  for (const auto& i : m_idx) {
    ss << i << ' ';
  }
  return ss.str();
}
