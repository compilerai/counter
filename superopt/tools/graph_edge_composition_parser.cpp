#include <fstream>
#include <iostream>
#include <string>

#include "support/cl.h"

#include "expr/pc.h"

#include "gsupport/edge_id_with_unroll.h"
#include "gsupport/graph_ec.h"
#include "gsupport/parse_edge_composition.h"

using namespace std;

int main(int argc, char **argv)
{
  cl::arg<string> gec_file(cl::implicit_prefix, "", "path to file containing the graph edge composition");
  cl::cl cmd("Graph edge composition Parser : Tries to parse a given expr file");
  cmd.add_arg(&gec_file);
  cmd.parse(argc, argv);

  ifstream fin;
  istream* in = &cin;
  if (gec_file.get_value() != "stdin") {
    fin.open(gec_file.get_value());
    if(!fin.is_open()) {
      cout << __func__ << " " << __LINE__ << ": could not open " << gec_file.get_value() << '\n';
      return 1;
    }
    in = &fin;
  }

  string line;
  bool end = !getline(*in, line);
  ASSERT(!end);
  graph_edge_composition_ref<pc,edge_id_with_unroll_t<pc>> g = parse_edge_composition<pc,edge_id_with_unroll_t<pc>>(line.c_str());
  ASSERT(g);

  cout << g->graph_edge_composition_to_string() << endl;

  return 0;
}
