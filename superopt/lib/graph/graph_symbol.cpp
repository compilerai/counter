#include "support/debug.h"
#include "expr/graph_symbol.h"
#include "rewrite/translation_instance.h"
#include "gsupport/rodata_map.h"

namespace eqspace {

//void
//graph_symbol_map_t::add_dst_symbols_referenced_in_rodata_map(rodata_map_t const& rodata_map, struct input_exec_t const* in)
//{
//  map<symbol_id_t, set<rodata_offset_t>> const& rm = rodata_map.get_map();
//  symbol_id_t cur_symbol_id = this->find_max_symbol_id() + 1;
//  map<string, symbol_id_t> dst_symbol_map;
//  for (auto const& rme : rm) {
//    for (auto const& ro_offset : rme.second) {
//      if (!dst_symbol_map.count(ro_offset.get_name())) {
//        dst_symbol_map.insert(make_pair(ro_offset.get_name(), cur_symbol_id));
//        size_t sz = in->get_symbol_size(ro_offset.get_name().c_str());
//        align_t align = in->get_symbol_alignment(ro_offset.get_name().c_str());
//        graph_symbol_t gsym(mk_string_ref(ro_offset.get_name()), sz, align, true);
//        this->insert(make_pair(cur_symbol_id, gsym));
//        cur_symbol_id++;
//      }
//    }
//  }
//}

graph_symbol_map_t::graph_symbol_map_t(istream& in)
{
  string line;
  bool end;

  end = !getline(in, line);
  ASSERT(!end);

  if (!is_line(line, "=Symbol-map:")) {
    cout << __func__ << " " << __LINE__ << ": line =\n" << line << endl;
  }
  ASSERT(is_line(line, "=Symbol-map:"));

  end = !getline(in, line);
  ASSERT(!end);
  while (string_has_prefix(line, "C_SYMBOL")) {
    //ASSERT(line.substr(0, strlen("C_SYMBOL")) == "C_SYMBOL");
    size_t remaining;
    int symbol_id = stoi(line.substr(strlen("C_SYMBOL")), &remaining);
    ASSERT(symbol_id >= 0);
    size_t colon = line.find(':');
    //ASSERT(remaining  + strlen("C_SYMBOL") == colon);
    ASSERT(colon != string::npos);
    size_t colon2 = line.find(':', colon + 1);
    ASSERT(colon2 != string::npos);
    string symbol_name = line.substr(colon + 1, colon2 - (colon + 1));
    trim(symbol_name);
    size_t colon3 = line.find(':', colon2 + 1);
    ASSERT(colon3 != string::npos);
    string symbol_size = line.substr(colon2 + 1, colon3 - (colon2 + 1));
    trim(symbol_size);
    size_t size = stoi(symbol_size);
    size_t colon4 = line.find(':', colon3 + 1);
    ASSERT(colon4 != string::npos);
    string symbol_align = line.substr(colon3 + 1, colon4 - (colon3 + 1));
    trim(symbol_align);
    align_t align = stoi(symbol_align);
    string symbol_is_const = line.substr(colon4 + 1);
    trim(symbol_is_const);
    bool is_const = (stoi(symbol_is_const) != 0);
    ASSERT(!this->count(symbol_id));
    this->insert(make_pair(symbol_id, graph_symbol_t(mk_string_ref(symbol_name), size, align, is_const)));
    end = !getline(in, line);
    ASSERT(!end);
  }
  ASSERT(line == "=Symbol-map done");
}

symbol_id_t
graph_symbol_map_t::find_max_symbol_id() const
{
  symbol_id_t ret = -1;
  for (auto const& s : m_map) {
    ret = max(ret, s.first);
  }
  return ret;
}

void
graph_symbol_map_t::graph_symbol_map_to_stream(ostream& os) const
{
  os << "=Symbol-map:\n";
  for (auto symbol : this->get_map()) {
    os << "C_SYMBOL" << symbol.first << " : " << symbol.second.get_name()->get_str() << " : " << symbol.second.get_size() << " : " << symbol.second.get_alignment() << " : " << symbol.second.is_const() << endl;
  }
  os << "=Symbol-map done\n";
}

bool
graph_symbol_map_t::symbol_is_rodata(symbol_id_t symbol_id) const
{
  if (this->count(symbol_id) == 0) {
    //cout << __func__ << " " << __LINE__ << ": returning TRUE for symbol_id " << symbol_id << endl;
    return true; //this symbol is not even getting used; assume rodata
  }
  //ASSERT(symbol_id >= 0 && symbol_id < (int)m_symbol_map.size());
  string s = this->at(symbol_id).get_name()->get_str();
  bool ret = memlabel_t::is_rodata_symbol_string(s);
  //cout << __func__ << " " << __LINE__ << ": returning " << ret << " for " << s << endl;
  return ret;
}

size_t
graph_symbol_map_t::symbol_map_get_size_for_symbol(symbol_id_t symbol_id) const
{
  ASSERT(this->count(symbol_id));
  return this->at(symbol_id).get_size();
}



}
