#include <fstream>
#include <iostream>
#include <string>

#include "gsupport/pc.h"
#include "graph/edge_id.h"
#include "gsupport/parse_edge_composition.h"

using namespace std;

int main(int argc, char **argv)
{
  string line;
  getline(cin, line);
  graph_edge_composition_ref<pc,edge_id_t<pc>> ecid = parse_edge_composition<pc,edge_id_t<pc>>(line.c_str());
  ASSERT(ecid);
  cout << ecid->to_string_concise() << endl;

  return 0;
}
