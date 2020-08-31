#pragma once

template <typename T>
class matrix_t
{
private:
  vector<vector<T>> m_elems;
public:
  void clear() { m_elems.clear(); }
  size_t get_rows() const { return m_elems.size(); }
  void set_elems(vector<vector<T>> const &elems) { m_elems = elems; }
	void push_back(vector<T> const &row) { m_elems.push_back(row); }
  bool matrix_column_is_non_zero(size_t idx) const
  {
    ASSERT(idx >= 0 && idx < m_elems.at(0).size());
    for (auto const& row : m_elems) {
      if (row.at(idx) != 0) {
        return true;
      }
    }
    return false;
  }

  string to_string(string const& prefix) const
  {
    stringstream ss;
    for (size_t i = 0; i < m_elems.size(); ++i) {
      auto const& row = m_elems.at(i);
      ss << prefix << i << ":\t";
      for (auto const& e : row)
        ss << e << " ";
      ss << '\n';
    }
    return ss.str();
  }
};
