#include <fstream>
#include <iostream>
#include <string>

#include "support/binary_search.h"

using namespace std;

class binary_search_on_bv_exprs_t : public binary_search_t
{
public:
  binary_search_on_bv_exprs_t()
  { }

private:
  bool test(set<size_t> const& indices) const override
  {
    cout << _FNLN_ << ": indices (size " << indices.size() << ") =";
    for (auto i : indices) { cout << " " << i; }
    cout << "... ";

    int x[] = { 0, 1, 2, 3, 4, 5, 6, 7, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66 };

    int xn = (sizeof x / sizeof x[0]);

    for (int i = 0; i < xn; i++) {
      if (!set_belongs(indices, (size_t)x[i])) {
        cout << "false\n";
        return false;
      }
    }
    cout << "true\n";
    return true;
  }
private:
};

int main(int argc, char **argv)
{
  binary_search_on_bv_exprs_t bsearch;
  set<size_t> minimized_bv_exprs = bsearch.do_binary_search(67);

    cout << _FNLN_ << ": solution indices (size " << minimized_bv_exprs.size() << ") =";
    for (auto i : minimized_bv_exprs) { cout << " " << i; }
    cout << "...\n";
  
  exit(0);
}
