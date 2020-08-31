#pragma once

#include <cstdlib>
#include <list>
#include <string>
#include <sstream>
#include <execinfo.h>
#include <functional>
#include <unordered_set>
#include <set>
#include <chrono>
#include <map>
#include <cassert>
#include <typeinfo>
#include "support/c_utils.h"
#include "support/types.h"

#include <boost/multiprecision/cpp_int.hpp>
#include <cxxabi.h>

using namespace boost::multiprecision;

using namespace std;

enum dfs_color
{
  dfs_color_white,
  dfs_color_gray,
  dfs_color_black,
};

template <typename T>
void cartesian_product(list<T> & xs, list<T> & ys, list<pair<T, T> >& out_result)
{
  out_result.clear();
  typename list<T>::iterator iter_x;
  for(iter_x = xs.begin(); iter_x != xs.end(); ++iter_x)
  {
    typename list<T>::iterator iter_y;
    for(iter_y = ys.begin(); iter_y != ys.end(); ++iter_y)
    {
      out_result.push_back(make_pair(*iter_x, *iter_y));
    }
  }
}

template <typename T>
void power_set(list<T> & xs, bool allow_empty, list<list<T> >& out_relations)
{
  out_relations.clear();
  out_relations.push_back(list<T>());

  typename list<T>::iterator iter_x;
  for(iter_x = xs.begin(); iter_x != xs.end(); ++iter_x)
  {
    T x = *iter_x;
    list<list<T> > res_copy = out_relations;
    typename list<list<T> >::iterator iter_res_copy;
    for(iter_res_copy = res_copy.begin(); iter_res_copy != res_copy.end(); ++iter_res_copy)
    {
      iter_res_copy->push_back(x);
    }
    out_relations.splice(out_relations.end(), res_copy);
  }
  if(!allow_empty)
    out_relations.pop_front();
}


template <typename T>
void list_to_list_of_list(list<T>& in, list<list<T> >& out)
{
  out.clear();
  typename list<T>::iterator iter_in;
  for(iter_in = in.begin(); iter_in != in.end(); ++iter_in)
  {
    list<T> tmp;
    tmp.push_back(*iter_in);
    out.push_back(tmp);
  }
}


template <typename T>
void cartesian_product_lists(list<list<T> >& in, list<list<T> >& out)
{
  out.clear();
  out.push_back(list<T>());
  typename list<list<T> >::iterator iter_in;
  for(iter_in = in.begin(); iter_in != in.end(); ++iter_in)
  {
    list<pair<list<T>, list<T> > > temp_res;

    list<list<T> > cur;
    list_to_list_of_list(*iter_in, cur);
    cartesian_product<list<T> >(out, cur, temp_res);

    out.clear();
    typename list<pair<list<T>, list<T> > >::const_iterator iter = temp_res.begin();
    for(; iter != temp_res.end(); ++iter)
    {
      list<T> first = iter->first;
      list<T> second = iter->second;
      first.splice(first.begin(), second);
      out.push_back(first);
    }
  }
}


int random_number(int max);

template <class T>
inline void myhash_combine(std::size_t& seed, const T& v)
{
  seed ^= std::hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

string int_to_string(int i);
string uint_to_string(unsigned int i);
string int64_to_string(int64_t i);
string uint64_to_string(uint64_t i);
string int_to_hexstring(int i);
string int_to_hexstring(int64_t i);
string int_to_hexstring(cpp_int i);
int string_to_int(string s, int def = 0);
long long string_to_longlong(string s, int def = 0);
void file_copy(string const& src, string const& dst);

bool is_co_prime(unsigned a, unsigned b);

template <class T>
bool compare_list_decreasing_size(const list<T>& first, const list<T>& second)
{
  return first.size() > second.size();
}

string get_backtrace_string();
unsigned get_compressed_string_zlib_size(const string& str);

template <class T>
bool list_contains(list<T> const &haystack, T const &needle)
{
  return std::find(haystack.begin(), haystack.end(), needle) != haystack.end();
}

template <class T>
bool list_contains_sequence(list<T> const &haystack, list<T> const &needle, std::function<bool (T const &, T const &)> f_contains)
{
  typename list<T>::const_iterator h_iter = haystack.begin();
  while (h_iter != haystack.end()) {
    typename list<T>::const_iterator h_iter_copy = h_iter;
    typename list<T>::const_iterator n_iter = needle.begin();
    while (   n_iter != needle.end()
           && h_iter_copy != haystack.end()
           && f_contains(*h_iter_copy, *n_iter)) {
      h_iter_copy++;
      n_iter++;
    }
    if (n_iter == needle.end()) {
      return true;
    }
    h_iter++;
  }
  return false;
}

template <class T>
bool sequence_is_prefix_of_list(list<T> const &haystack, list<T> const &needle, std::function<bool (T const &, T const &)> f_contains)
{
  typename list<T>::const_iterator h_iter = haystack.begin();
  typename list<T>::const_iterator n_iter = needle.begin();
  while (h_iter != haystack.end() && n_iter != needle.end()) {
    // continue till needle elems are contained in haystack elems
    if(f_contains(*h_iter, *n_iter)) {
      h_iter++;
      n_iter++;
    }
    else // needle elem not found in haystack elem (return false)
      break;
  }
  if (n_iter == needle.end()) {
    return true;
  }
  return false;
}

template<typename T>
void
push_back_if_not_already_exists(list<T> &ls, T e)
{
  for (const auto &l : ls) {
    if (l == e) {
      return;
    }
  }
  ls.push_back(e);
}

/*enum error_code
{
  ERROR_TIMEOUT_OVERALL,
  //ERROR_TIMEOUT_QUERY,
  //ERROR_QUERY_FAILED,
  //ERROR_OPCODE_NOT_SUPPORTED,
  //ERROR_OPCODE_FP,
  //ERROR_CONSTANT_LOOPS,
  //ERROR_INFINITE_LOOP_IN_GET_TFG,
  //ERROR_CORRELATION_NOT_FOUND,
  //ERROR_SIMULATION_NOT_FOUND,
  //ERROR_PARTIAL_EQ_FAILED,
  //ERROR_SIGNATURE_MISMATCH,
  //ERROR_EXIT_PC_NOT_RET,
};*/

//void error(error_code ec);
//void exit_with_msg(const string& msg);

string join_stringlist(const list<string>& strings, const string& separator);
string get_unique_name(const char* prefix);

template <typename T>
bool set_belongs(const set<T>& s, T const &elem)
{
  return s.count(elem) != 0;
}

template <typename T>
void set_intersect(const set<T>& s1, const set<T>& s2, set<T>& res)
{
  set_intersection(s1.begin(), s1.end(), s2.begin(), s2.end(), std::inserter(res, res.begin()));
}

template <typename T, typename C>
void set_intersect(set<T, C>& s1, const set<T, C>& s2)
{
  set<T, C> tmp = s1;
  s1.clear();
  set_intersection(tmp.begin(), tmp.end(), s2.begin(), s2.end(), std::inserter(s1, s1.begin()));
}

template <typename T>
bool set_intersection_is_empty(set<T> const &s1, set<T> const &s2)
{
  set<T> i;
  set_intersect(s1, s2, i);
  return i.size() == 0;
}

template <typename T>
void set_difference(const set<T>& s1, const set<T>& s2, set<T>& res)
{
  set_difference(s1.begin(), s1.end(), s2.begin(), s2.end(), std::inserter(res, res.begin()));
}

template <typename T, typename C>
void set_difference(set<T, C>& s1, const set<T, C>& s2)
{
  set<T, C> tmp = s1;
  s1.clear();
  set_difference(tmp.begin(), tmp.end(), s2.begin(), s2.end(), std::inserter(s1, s1.begin()));
}

template <typename T, typename C>
void set_union(const set<T, C>& s1, const set<T, C>& s2, set<T, C>& res)
{
  set_union(s1.begin(), s1.end(), s2.begin(), s2.end(), std::inserter(res, res.begin()));
}

template <typename T, typename C>
void set_union(set<T, C>& s1, const set<T, C>& s2)
{
  set<T, C> tmp = s1;
  set_union(tmp.begin(), tmp.end(), s2.begin(), s2.end(), std::inserter(s1, s1.begin()));
}

template <typename T>
bool set_contains(const set<T>& haystack, const set<T>& needle)
{
  return includes(haystack.begin(), haystack.end(), needle.begin(), needle.end());
}

template <typename T>
bool unordered_set_belongs(const unordered_set<T>& s, T const &elem)
{
  return s.count(elem) != 0;
}

template <typename T>
void unordered_set_union(const unordered_set<T>& s1, const unordered_set<T>& s2, unordered_set<T>& res)
{
  res.insert(s1.begin(), s1.end());
  res.insert(s2.begin(), s2.end());
}

template <typename T>
void unordered_set_union(unordered_set<T>& s1, const unordered_set<T>& s2)
{
  s1.insert(s2.begin(), s2.end());
}

template <typename T>
void unordered_set_difference(const unordered_set<T>& s1, const unordered_set<T>& s2, unordered_set<T>& res)
{
  std::copy_if(s1.begin(), s1.end(), inserter(res, res.end()),
    [&s2] (T const& needle) { return s2.find(needle) == s2.end(); });
}

template <typename K, typename T>
map<K,T> map_union(map<K, T> const&m1, map<K, T> const &m2)
{
  map<K,T> tmp = m1;
  tmp.insert(m2.begin(), m2.end());
  return tmp;
}

template <typename K, typename T>
void map_union(map<K, T> &m1, map<K, T> const &m2)
{
  m1.insert(m2.begin(), m2.end());
}

template <typename K, typename T>
void map_diff(map<K, T> &a, map<K, T> const &b)
{
  map<K, T> c;
  std::set_difference(
      a.begin(), a.end(),
      b.begin(), b.end(),
      std::inserter(c, c.begin()) );
  a = c;
}

template <typename K, typename T>
bool is_subset_of_map_domain(set<K> const& s, map<K, T> const& m)
{
  return set_contains(map_get_keys(m), s);
}

template <typename T>
int vector_find(const vector<T>& s1, T const &e)
{
  for (int i = 0; i < s1.size(); i++) {
    if (s1.at(i) == e) {
      return i;
    }
  }
  return -1;
}



template <typename T>
bool vector_contains(const vector<T>& s1, T const &e)
{
  for (const auto &s1e : s1) {
    if (s1e == e) {
      return true;
    }
  }
  return false;
}

template <typename T>
bool vector_is_subset(const vector<T>& s1, vector<T> const &s2)
{
  // check if s2 is subset of s1
  for (const auto &s2e : s2) {
    if (!vector_contains(s1, s2e)) {
      return false;
    }
  }
  return true;
}

template <typename T>
void vector_union(const vector<T>& s1, const vector<T>& s2, vector<T>& res)
{
  for (const auto &s1e : s1) {
    res.push_back(s1e);
  }
  for (const auto &s2e : s2) {
    bool exists = false;
    for (const auto &s1e : s1) {
      if (s1e == s2e) {
        exists = true;
        break;
      }
    }
    if (!exists) {
      res.push_back(s2e);
    }
  }
}

template <typename T>
void vector_union(vector<T>& s1, const vector<T>& s2)
{
  vector<T> tmp = s1;
  s1.clear();
  vector_union(tmp, s2, s1);
}

template <typename T>
vector<T> vector_minus(vector<T> const &s1, vector<T> const &s2)
{
  vector<T> ret;
  for (const auto &s1e : s1) {
    if (!vector_contains(s2, s1e)) {
      ret.push_back(s1e);
    }
  }
  return ret;
}

template <typename T>
void list_append(list<T> &a, list<T> const &b)
{
  for (auto const& b_elem : b) {
    a.push_back(b_elem);
  }
}

template <typename T>
void list_intersect(list<T> &a, list<T> const &b)
{
  for (auto iter = a.begin(); iter != a.end();) {
    bool found = false;
    auto const& a_elem = *iter;
    for (auto const& b_elem : b) {
      if (a_elem == b_elem) {
        found = true;
        break;
      }
    }
    if (found) {
      iter++;
    } else {
      iter = a.erase(iter);
    }
  }
}

template <typename T>
void list_union_append(list<T> &a, list<T> const &b)
{
  for (auto const& b_elem : b) {
    if (list_contains(a, b_elem)) {
      continue;
    }
    a.push_back(b_elem);
  }
}


template <typename T>
void
vector_append(vector<T> &a, vector<T> const &b)
{
  a.insert(a.end(), b.begin(), b.end());
}

template <typename T>
T list_get_elem_at_index(list<T> const &ls, size_t index)
{
  size_t i = 0;
  ASSERT(index < ls.size());
  for (auto iter = ls.begin(); iter != ls.end(); iter++) {
    if (i == index) {
      return *iter;
    }
    i++;
  }
  assert(0);
}

template <typename T>
string set_to_string(const set<T>& s)
{
  stringstream ss;
  ss << "{";
  for(const T& e : s)
    ss << e << ", ";
  ss << "}";
  return ss.str();
}

string get_backtrace_string();
unsigned get_compressed_string_zlib_size(const string& str);

template <typename T>
string vector_to_string(const vector<T>& s)
{
  stringstream ss;
  ss << "[";
  for(const T& e : s)
    ss << e.to_string() << ", ";
  ss << "]";
  return ss.str();
}

template <typename T>
std::list<T>
vector_to_list(std::vector<T> const &v)
{
  std::list<T> ret;
  for (const auto &e : v) {
    ret.push_back(e);
  }
  return ret;
}

template<typename T>
vector<vector<T>> get_all_subsets(vector<T> set)
{
  vector<vector<T>> subset;
  vector<T> empty;
  subset.push_back(empty);

  for (size_t i = 0; i < set.size(); i++) {
    vector<vector<T>> subsetTemp = subset;

    for (size_t j = 0; j < subsetTemp.size(); j++) {
      subsetTemp[j].push_back(set[i]);
    }
    for (size_t j = 0; j < subsetTemp.size(); j++) {
      subset.push_back(subsetTemp[j]);
    }
  }
  return subset;
}

template<typename T>
bool vector_all_elems_equal(vector<T> const &v)
{
  if (v.size() == 0) {
    return true;
  }
  T c = v.at(0);
  for (auto e : v) {
    if (c != e) {
      return false;
    }
  }
  return true;
}

template<typename T>
void dedup_vector(vector<vector<T>> & points)
{
  sort(points.begin(), points.end(), std::greater<vector<T>>());
  points.erase(unique(points.begin(), points.end()), points.end());
}

template<typename T>
void dedup_vector_preserving_order(vector<vector<T>> & points)
{
  vector<bool> to_be_deleted(points.size(), false);
  for (int i = points.size()-1; i >= 0; --i) {
    auto const& ref_point = points[i];
    for (int j = i-1; j >= 0; --j) {
      if (!to_be_deleted[j] && points[j] == ref_point) {
        to_be_deleted[j] = true;
      }
    }
  }
  size_t final_size = count_if(to_be_deleted.begin(), to_be_deleted.end(), [](bool const& b) { return !b; });
  vector<vector<T>> deduped_points(final_size);
  size_t j = 0;
  for (size_t i = 0; i < points.size(); ++i) {
    if (!to_be_deleted[i])
      deduped_points[j++] = points[i];
  }
  points.swap(deduped_points);
}

template<typename T>
size_t count_nonzero(vector<T> const& v)
{
  return count_if(v.begin(), prev(v.end()), [](T const& a) { return a != 0; });
}

template<typename T>
vector<bool>
get_nonzero_bitmap_along_row(vector<vector<T>> const& grid, size_t maxcol)
{
  vector<bool> bitmap(maxcol, 0);
  for (const auto& row : grid) {
    for (size_t i = 0; i < maxcol; ++i) {
      if (row[i] != 0) bitmap[i] = true;
    }
  }
  return bitmap;
}


void trim(string& s);
void remove_space(string& s);
bool stob(string const &s);

size_t vector_max(vector<size_t> const &v);
unsigned long round_up_to_nearest_power_of_ten(unsigned long in);
void string_replace(string &haystack, string const &pattern, string const &replacement);
bool string_contains(string const &haystack, string const &pattern);
bool string_has_prefix(string const &s, string const &expected_prefix);
bool string_has_suffix(string const &s, string const &expected_suffix);
bool is_line(const string& line, const string& keyword);
bool string_is_numeric(string const &str);
bool stringIsInteger(string const &s);
bool stringStartsWith(string const &haystack, string const &needle);
string string_get_maximal_alpha_prefix(string const& s);

inline bool file_exists(const std::string& name) {
    return ( access( name.c_str(), F_OK ) != -1 );
}

inline int bitmap_get_max_bit_set(uint64_t bitmap)
{
  int max = -1;
  size_t i;
  for (i = 0; i < 64; i++) {
    if (bitmap & (1ULL << i)) {
      max = i;
    }
  }
  return max;
}

inline uint64_t
round_up_bitmap_to_byte_size(uint64_t in)
{
  size_t bmap_len = bitmap_get_max_bit_set(in) + 1;
  size_t bmap_len_roundup = ROUND_UP(bmap_len, BYTE_LEN);
  assert(bmap_len_roundup <= 64);
  return MAKE_MASK(bmap_len_roundup);
}

inline bool
bitmap_downgrade(uint64_t &bitmap)
{
  int bitmap_numbits = bitmap_get_max_bit_set(bitmap) + 1;
  assert(bitmap_numbits <= DWORD_LEN);
  if (bitmap_numbits > WORD_LEN) {
    bitmap_numbits = WORD_LEN;
    bitmap = MAKE_MASK(bitmap_numbits);
    return true;
  } else if (bitmap_numbits > BYTE_LEN) {
    bitmap_numbits = BYTE_LEN;
    bitmap = MAKE_MASK(bitmap_numbits);
    return true;
  } else if (bitmap != 0) {
    bitmap = 0;
    return true;
  } else {
    return false;
  }
}

inline bool
bool_le(bool a, bool b)
{
  return !a || b;
}

string read_section(ifstream &infile, char const *endmarker);

template<typename ELEM, typename COST>
ELEM
list_find_min(list<ELEM> const &ls, std::function<COST (ELEM const &)> get_cost_fn)
{
  ASSERT(ls.size() > 0);
  ELEM mincost_elem = *ls.begin();
  COST mincost = get_cost_fn(mincost_elem);
  for (const auto &e : ls) {
    COST c = get_cost_fn(e);
    if (c < mincost) {
      mincost = c;
      mincost_elem = e;
    }
  }
  return mincost_elem;
}

template<typename T>
bool
operator<(vector<T> const &a, vector<T> const &b)
{
  //lexicographic comparison
  for (size_t i = 0; i < min(a.size(), b.size()); i++) {
    if (a.at(i) < b.at(i)) {
      return true;
    }
    if (a.at(i) > b.at(i)) {
      return false;
    }
  }
  if (a.size() < b.size()) {
    return true;
  }
  return false;
}

template<typename T>
ssize_t
list_get_index(std::list<T> const &haystack, T const &needle)
{
  ssize_t ret = -1;
  for (const auto &e : haystack) {
    ret++;
    if (e == needle) {
      return ret;
    }
  }
  return -1;
}


#define unreachable(X) assert(false && "unreachable" X)

template<typename K, typename V>
set<K>
map_get_keys(map<K,V> const &m)
{
  set<K> ret;
  for (const auto &e : m) {
    ret.insert(e.first);
  }
  return ret;
}

string chrono_duration_to_string(chrono::duration<double> const &d, streamsize const& precision = 2);
static inline
string pad_string(string const &in, size_t padlen)
{
  size_t len = in.size();
  if (len >= padlen) {
    return in;
  }
  string space_pad;
  char pad[padlen - len + 1];
  for (size_t i = 0; i < padlen - len; i++) {
    pad[i] = ' ';
  }
  pad[padlen - len] = '\0';
  string ret = in;
  ret.append(pad);
  return ret;
}

string file_to_string(string const &filename);

template<typename T>
void set_erase_if(set<T>& s, std::function<bool (T const &)> f_pred)
{
  for (auto itr = s.begin(); itr != s.end(); ) {
    if (f_pred(*itr)) { itr = s.erase(itr); }
    else { ++itr; }
  }
}

unsigned long round_up_to_multiple_of(unsigned long x, unsigned long m);

template<typename T_ELEM>
T_ELEM const &
list_get_element_at_index(list<T_ELEM> const &ls, size_t idx)
{
  size_t ret = 0;
  for (auto const& e : ls) {
    if (ret == idx) {
      return e;
    }
    ret++;
  }
  cout << __func__ << " " << __LINE__ << ": idx = " << idx << endl;
  cout << __func__ << " " << __LINE__ << ": ls.size() = " << ls.size() << endl;
  assert(0);
}

template<typename T_ELEM>
void print_2d_matrix(vector<vector<T_ELEM>> const &matrix)
{
  for (size_t i = 0; i < matrix.size(); i++) {
    vector<T_ELEM> const &row = matrix.at(i);
    for (const auto &e : row) {
      cout << " " << e;
    }
    cout << endl;
  }
  cout << endl;
}

template<typename T>
list<T>
list_flatten(list<list<T>> const& in)
{
  list<T> ret;
  for (auto const& elem : in) {
    list_append(ret, elem);
  }
  return ret;
}

template<typename T>
vector<T>
list_to_vector(list<T> const& ls)
{
  vector<T> ret;
  for (const auto& e : ls) {
    ret.push_back(e);
  }
  return ret;
}

template<typename T>
T *
alloc_array_from_vec(vector<T> const& v)
{
  ASSERT(v.size());
  T* a = new T[v.size()];
  ASSERT(a);
  size_t i = 0;
  for (auto const& e : v ) {
    a[i] = e;
    i++;
  }
  return a;
}

template<typename T>
vector<T>
vector_from_array(T const* a, size_t alen)
{
  vector<T> ret;
  for (size_t i = 0; i < alen; i++) {
    ret.push_back(a[i]);
  }
  return ret;
}

void file_truncate(string const &filename);
list<string> splitLines(string const& str);
vector<string> splitOnChar(string const& str, char ch);
string increment_marker_number(string const& filename);
string gen_ascii_string_for_characters(vector<char> const& chrs);
bool files_differ(string const& p1, string const& p2);
string md5_checksum(string const& str);
list<string> file_get_lines(string const& filename);
string convert_malloc_callee_to_mymalloc(string const& callee_name);
string llvm_insn_type_to_string(llvm_insn_type_t t);
long fscanf_entry_section(FILE *fp, char *src_assembly, size_t src_assembly_size);

inline string demangled_type_name(std::type_info const& ti)
{
  int status;
  char *demangled_name = abi::__cxa_demangle(ti.name(), 0, 0, &status);
  string ret(demangled_name);
  free(demangled_name);
  return ret;
}

void g_query_dir_init();
void clean_query_files();
