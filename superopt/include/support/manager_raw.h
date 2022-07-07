#pragma once


//should be used if the number of unique T objects is known to be bounded (for bounded memory consumption), e.g., reg_identifiers
template <typename T> class manager_raw
{
public:
  T const *register_object(T const &given, unsigned suggested_id = 0)
  {
    if (m_object_map.count(given) == 0) {
      //ret = make_shared<T>(given);
      T *new_elem = new T(given);
      m_object_map.insert(std::make_pair(given, new_elem));
      return new_elem;
    } else {
      T const *existing = m_object_map.at(given);
      return existing;
    }
  }

  unsigned get_size() const
  {
    return m_object_map.size();
  }

  std::string stat() const
  {
    std::stringstream ss;
    ss << "count: " << m_object_map.size();
    return ss.str();
  }

private:
  std::unordered_map<T, T const *> m_object_map;
};
