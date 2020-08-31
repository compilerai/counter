#ifndef ITABLE_STOPPING_COND_H
#define ITABLE_STOPPING_COND_H

typedef enum itable_stopping_cond_enum_t
{
  ITABLE_STOPPING_COND_EQUIV,
  ITABLE_STOPPING_COND_COUNTER,
  ITABLE_STOPPING_COND_LEN,
  ITABLE_STOPPING_COND_NEVER
} itable_stopping_cond_enum_t;

class itable_stopping_cond_t
{
private:
  itable_stopping_cond_enum_t m_val;
  int m_count;
public:
  itable_stopping_cond_t(string const &str)
  {
    m_count = 0;
    if (str == "never") {
      m_val = ITABLE_STOPPING_COND_NEVER;
    } else if (str == "equiv") {
      m_val = ITABLE_STOPPING_COND_EQUIV;
    } else if (str.at(0) == 'L' && string_is_numeric(str.substr(1))) {
      m_val = ITABLE_STOPPING_COND_LEN;
      m_count = string_to_int(str.substr(1));
    } else if (string_is_numeric(str)) {
      m_val = ITABLE_STOPPING_COND_COUNTER;
      m_count = string_to_int(str);
    } else NOT_REACHED();
  }
  string itable_stopping_cond_to_string() const
  {
    if (m_val == ITABLE_STOPPING_COND_NEVER) {
      return "never";
    } else if (m_val == ITABLE_STOPPING_COND_COUNTER) {
      return int_to_string(m_count);
    } else if (m_val == ITABLE_STOPPING_COND_EQUIV) {
      return "equiv";
    } else NOT_REACHED();
  }
  bool operator==(itable_stopping_cond_enum_t ie) const
  {
    return m_val == ie && m_count == 0;
  }
  bool is_counter(int i) const
  {
    return m_val == ITABLE_STOPPING_COND_COUNTER && m_count == i;
  }
  bool is_len() const
  {
    return m_val == ITABLE_STOPPING_COND_LEN;
  }
  int get_length() const
  {
    ASSERT(is_len());
    return m_count;
  }
};

#endif
