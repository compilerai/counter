#pragma once

#include <memory>

#include "support/manager.h"

namespace eqspace {

class axpred_desc_t;

using axpred_desc_ref = std::shared_ptr<const axpred_desc_t>;

class axpred_desc_t {
private:
  std::string m_axpred_name;
  bool m_is_managed = false;

public:
  axpred_desc_t(const std::string &axpred_name);

  const std::string &get_axpred_name() const;

  std::string to_string() const;

  static manager<axpred_desc_t> *get_manager();

  bool id_is_zero() const;

  void set_id_to_free_id(unsigned suggested_id);

  bool operator==(const axpred_desc_t &other) const;

  ~axpred_desc_t();
};

axpred_desc_ref mk_axpred_desc_ref(const std::string &axpred_name);

} // namespace eqspace

namespace std {

template <> struct hash<eqspace::axpred_desc_t> {
  std::size_t operator()(const eqspace::axpred_desc_t &a) const { return 0; }
};

} // namespace std