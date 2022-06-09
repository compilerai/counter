#ifndef TRANSLATION_UNIT_H
#define TRANSLATION_UNIT_H

class translation_unit_t
{
private:
  std::string m_name;
public:
  translation_unit_t() = delete;
  translation_unit_t(std::string name) : m_name(name) {}
  string const &get_name() const { return m_name; }
  bool operator<(translation_unit_t const &other) const { return m_name < other.m_name; }
};

#endif
