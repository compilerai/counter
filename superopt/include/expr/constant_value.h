#pragma once
#include <memory>
#include <map>

#include "support/debug.h"
#include "support/bv_const.h"

#include "expr/langtype.h"
#include "expr/array_constant.h"
#include "expr/axpred_desc.h"
#include "expr/rounding_mode.h"
#include "expr/comment.h"

namespace eqspace {
using namespace std;

class constant_value
{
public:
  constant_value(context *ctx) : m_context(ctx), m_id(0) {isArrayConst = false;}
  constant_value(string_ref const &data, sort_ref const &s);
  constant_value(sort_ref const &s, string_ref const &data);
  constant_value(int data, sort_ref const &s);
  constant_value(uint64_t data, sort_ref const &s);
  constant_value(int64_t data, sort_ref const &s);
  constant_value(sort_ref const &s, context* ctx);
  constant_value(array_constant_ref const &a, context* ctx);
  constant_value(mybitset const &mbs, context *ctx);
  constant_value(float_max_t f, context *ctx);
  constant_value(memlabel_t const &ml, context *ctx);
  constant_value(rounding_mode_t rounding_mode, context *ctx);
  ~constant_value();

  //void canonicalize_data_for_sort();
  bool get_bool() const;
  static string constant_value_bv_to_string(mybitset const& mbs);
  string get_raw_data_in_decimal() const;
  string constant_value_to_string(sort_ref const &s) const;
  memlabel_t get_memlabel() const;
  rounding_mode_t get_rounding_mode() const;
  comment_t get_comment() const;
  axpred_desc_ref get_axpred_desc_type() const;
  langtype_ref get_langtype() const;
  int get_regid() const;
  int get_int() const;
  int get_count() const;
  int64_t get_int64() const;
  uint64_t get_uint64() const;
  float_max_t get_float() const { return m_float; }
  mybitset const &get_mybitset() const;
  bool is_array_const() const { return isArrayConst; }
  array_constant_ref const& get_array() const { return m_arr; }
  string get_memlabel_constant_str() const;
  string get_rounding_mode_constant_str() const;
  string get_comment_constant_str() const;
  //string get_container_type_constant_str() const;
  string get_axpred_desc_constant_str() const;
  string get_langtype_constant_str() const;
  string get_regid_constant_str() const;
  string get_count_constant_str() const;
  string get_bv_constant_str(unsigned bv_size, bool const_hex) const;
  string get_float_constant_str(sort_ref const& s) const;
  string get_bool_constant_str() const;
  string get_int_constant_str() const;
  string get_arr_constant_str() const;
  bool operator==(const constant_value&) const;
  bool equals(const constant_value &other) const;
  //void set_string(string s) { /*cout << __func__ << ": s = " << s << endl; if (s.find(G_SYMBOL_KEYWORD) != string::npos && s.at(s.length() - 1) == '0') { assert(0); }*/ m_string = s; };
  string_ref get_string_ref() const { return m_string_ref; };
  //expr_ref get_array_value(vector<expr_ref>);
  //void set_array_value(vector<expr_ref>, expr_ref);
  void set_id_to_free_id(constant_value_id_t suggested_id);
  bool id_is_zero() const { return m_id == 0; }
private:
  context *m_context;
  constant_value_id_t m_id;
  mybitset m_data;
  float_max_t m_float = 0;
  string_ref m_string_ref;
  // new addition for array constants
  bool isArrayConst;
  array_constant_ref m_arr;
};

using constant_value_ref = shared_ptr<constant_value>;

}
