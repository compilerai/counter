#include "support/dgraph.h"
#include <set>
#include <utility>

using namespace std;

int main()
{
  set<unsigned> nodes = { 1, 2, 3, 4, 5, 6, 7, 8 };
  set<pair<unsigned,unsigned>> edges = {
    { 1, 2 },
    { 2, 3 },
    { 3, 1 },
    { 4, 2 },
    { 4, 3 },
    { 4, 5 },
    { 5, 4 },
    { 5, 6 },
    { 6, 3 },
    { 6, 7 },
    { 7, 6 },
    { 8, 7 },
    { 8, 8 },
    { 8, 5 }
  };

  dgraph_t<unsigned> dg(nodes, edges);
  scc_t<unsigned> scc(dg);
  scc.compute_scc();
  auto sccs = scc.get_scc();
  cout << "SCC:\n";
  unsigned i = 0;
  for (auto const& cc : sccs) {
    cout << i++ << ": ";
    for (auto const& n : cc) {
      cout << n << " ";
    }
    cout << endl;
  }

  return 0;
}
