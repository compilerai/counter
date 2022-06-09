#pragma once 

#include <vector>
#include <list>
#include <string>
#include "support/debug.h"

using namespace std;

class scatter_gather_t
{
private:
  vector<size_t> m_idx;

  void set(vector<bool>::const_iterator const& begin, vector<bool>::const_iterator const& end);

public:
  scatter_gather_t(size_t size);
  scatter_gather_t(vector<bool> const& bitmap);
  scatter_gather_t(vector<size_t> const& idx);
  scatter_gather_t(list<size_t> const& idx);
  scatter_gather_t(vector<size_t>::const_iterator const& begin, vector<size_t>::const_iterator const& end);
  scatter_gather_t(list<size_t>::const_iterator const& begin, list<size_t>::const_iterator const& end);
  scatter_gather_t(vector<bool>::const_iterator const& begin, vector<bool>::const_iterator const& end);

  size_t size() const { return m_idx.size(); }
  void clear() { m_idx.clear(); }

  scatter_gather_t get_subindex(size_t l) const;
  scatter_gather_t get_subindex(size_t l, size_t r) const;

  // find in index
  // if not found, return the location in index where it would be inserted
  size_t find(size_t v) const;

  void set_size(size_t size);
  void swap(vector<size_t>& idx_vec);
  void update(vector<bool> const& bitmap);

  vector<bool> as_bitmap() const;
  string to_string() const;

  template<typename T>
  vector<T> gather_vector(vector<T> const& input)
  {
    vector<T> out(m_idx.size());
    for (size_t i = 0; i < m_idx.size(); ++i)
      out[i] = input[m_idx[i]];
    return out;
  }

  template<typename T>
  vector<vector<T>> apply_gather_vector(vector<vector<T>> const& input)
  {
    vector<vector<T>> out(input.size());
    for (size_t i = 0; i < input.size(); ++i) {
      out[i] = this->gather_vector(input[i]);
    }
    return out;
  }
};
