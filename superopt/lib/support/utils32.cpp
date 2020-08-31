#include "support/utils32.h"
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
#include <assert.h>
#include "support/c_utils.h"
#include "support/types.h"


using namespace std;

void trim(string& s)
{
  while ((s.length() > 0) && isspace(s.at(0))) {
    s = s.substr(1);
  }

  while ((s.length() > 0) && (s.at(s.size() - 1) == ' ')) {
    s = s.substr(0, s.size() - 1);
  }
}

bool
string_has_prefix(string const &s, string const &expected_prefix)
{
  if (s.length() < expected_prefix.length()) {
    return false;
  }
  return strncmp(s.c_str(), expected_prefix.c_str(), expected_prefix.length()) == 0;
}

long long
string_to_longlong(string s, int def)
{
  if (s.empty()) {
    return def;
  }
  return strtoll(s.c_str(), nullptr, def);
}

string
llvm_insn_type_to_string(llvm_insn_type_t t)
{
  switch (t) {
    case llvm_insn_other: return "other";
    case llvm_insn_farg: return "farg";
    case llvm_insn_bblentry: return "bblentry";
    case llvm_insn_phinode: return "phinode";
    case llvm_insn_alloca: return "alloca";
    case llvm_insn_ret: return "ret";
    case llvm_insn_call: return "call";
    case llvm_insn_invoke: return "invoke";
    case llvm_insn_load: return "load";
    case llvm_insn_store: return "store";
    default: NOT_REACHED();
  }
}
