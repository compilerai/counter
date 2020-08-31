#include "support/debug.h"
#include "expr/graph_local.h"
#include "rewrite/translation_instance.h"
#include "gsupport/rodata_map.h"

namespace eqspace {

local_id_t const graph_locals_map_t::m_vararg_local_id = 0;

void
graph_locals_map_t::graph_locals_map_to_stream(ostream& ss) const
{
  ss << "=Locals-map:\n";
  for (auto local : m_map) {
    ss << "C_LOCAL" << local.first << " : " << local.second.get_name()->get_str() << " : " << local.second.get_size() << " : " << local.second.get_alignment() << " : " << (local.second.is_varsize() ? '1' : '0') << endl;
  }
  ss << "=Locals-map done\n";
}


graph_locals_map_t::graph_locals_map_t(istream& in)
{
  string line;
  bool end;

  end = !getline(in, line);
  ASSERT(!end);
  ASSERT(line == "=Locals-map:");

  getline(in, line);
  while (!is_line(line, "=Locals-map done")) {
    size_t keyword_len = strlen("C_LOCAL");
    ASSERT(line.substr(0, keyword_len) == "C_LOCAL");
    size_t colon = line.find(':');
    ASSERT(colon != string::npos);
    local_id_t local_id = string_to_int(line.substr(keyword_len, colon - keyword_len));
    size_t colon2 = line.find(':', colon + 1);
    ASSERT(colon2 != string::npos);
    string local_name = line.substr(colon + 1, colon2 - (colon + 1));
    trim(local_name);
    string local_size = line.substr(colon2 + 1);
    size_t size = stoi(local_size);
    size_t colon3 = line.find(':', colon2 + 1);
    ASSERT(colon3 != string::npos);
    string local_align = line.substr(colon3 + 1);
    align_t align = stoi(local_align);
    size_t colon4 = line.find(':', colon3 + 1);
    ASSERT(colon4 != string::npos);
    string local_is_varsize = line.substr(colon4 + 1);
    bool is_varsize = (stoi(local_is_varsize) != 0);
    m_map.insert(make_pair(local_id, graph_local_t(local_name, size, align, is_varsize)));
    getline(in, line);
    local_id++;
  }
  ASSERT(line == "=Locals-map done");
  //return ret;
}

size_t
graph_locals_map_t::graph_locals_map_get_stack_size() const
{
  size_t ret = 0;
  for (const auto &l : m_map) {
    if (l.second.is_varsize()) {
      NOT_IMPLEMENTED();
    }
    ret += l.second.get_size();
  }
  return ret;
}

}
