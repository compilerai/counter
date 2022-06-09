#pragma once
#include <string>
#include <list>

namespace eqspace {

using namespace std;

class function_signature_t {
private:
  string m_return_type;
  list<string> m_argument_types;
  list<string> m_argument_names;
  bool m_is_vararg;
public:
  function_signature_t(string const& return_type, list<string> const& argument_types, list<string> const& argument_names, bool is_vararg) : m_return_type(return_type), m_argument_types(argument_types), m_argument_names(argument_names), m_is_vararg(is_vararg)
  { }

  static function_signature_t invalid_function_signature()
  {
    return function_signature_t("invalid", {}, {}, false);
  }
  string const& get_return_type() const { return m_return_type; }
  list<string> const& get_arg_types() const { return m_argument_types; }
  list<string> const& get_arg_names() const { return m_argument_names; }
  string function_signature_to_string_for_eq() const;
  static function_signature_t read_from_string(string const& line);
private:
  static pair<string, string> get_type_and_name(string const& s);
};

}
