#include "support/utils.h"
#include "support/log.h"
//#include "expr/expr.h"
//#include "eqcheck/state.h"
//#include "eqcheck/eqcheck.h"
//#include <openssl/md5.h>
#include <stdlib.h>
#include <cmath>
#include <cassert>
#include <iostream>
#include <fstream>
#include <limits>
#include "support/mytimer.h"
#include "support/debug.h"
#include "support/globals_cpp.h"
#include "support/globals.h"
#include <bits/stdc++.h>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/range.hpp>

static char as1[40960];

#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>

#define DEBUG_TYPE "utils"

string g_query_dir;
int gas_inited = 0; /* defined in globals.h */

int random_number(int max)
{
  return rand() % max;
}

string int_to_string(int i)
{
  stringstream ss;
  ss << i;
  return ss.str();
}

string uint_to_string(unsigned int i)
{
  stringstream ss;
  ss << i;
  return ss.str();
}

string
int64_to_string(int64_t i)
{
  stringstream ss;
  ss << i;
  return ss.str();
}

string
uint64_to_string(uint64_t i)
{
  stringstream ss;
  ss << i;
  return ss.str();
}

/*void error(error_code ec)
{
//  LOG(g_eqcheck->get_context()->stat());
  LOG(stats::get());
  string str;
  switch(ec)
  {
  case ERROR_TIMEOUT_OVERALL: str = "ERROR_TIMEOUT_OVERALL"; break;
  case ERROR_TIMEOUT_QUERY: str = "ERROR_TIMEOUT_QUERY"; break;
  case ERROR_QUERY_FAILED: str = "ERROR_QUERY_FAILED"; break;
  case ERROR_OPCODE_NOT_SUPPORTED: str = "ERROR_OPCODE_NOT_SUPPORTED"; break;
  case ERROR_OPCODE_FP: str = "ERROR_OPCODE_FP"; break;
  case ERROR_CONSTANT_LOOPS: str = "ERROR_CONSTANT_LOOPS"; break;
  case ERROR_INFINITE_LOOP_IN_GET_TFG: str = "ERROR_INFINITE_LOOP_IN_GET_TFG"; break;
  case ERROR_CORRELATION_NOT_FOUND: str = "ERROR_CORRELATION_NOT_FOUND"; break;
  case ERROR_SIMULATION_NOT_FOUND: str = "ERROR_SIMULATION_NOT_FOUND"; break;
  case ERROR_PARTIAL_EQ_FAILED: str = "ERROR_PARTIAL_EQ_FAILED"; break;
  case ERROR_SIGNATURE_MISMATCH: str = "ERROR_SIGNATURE_MISMATCH"; break;
  case ERROR_EXIT_PC_NOT_RET: str = "ERROR_EXIT_PC_NOT_RET"; break;
  default: assert(false && "error case not found");
  }

  LOG(endl << str << endl);

  exit(0);
}

void exit_with_msg(const string& msg)
{
  LOG(stats::get());
  LOG(msg);
  exit(0);
}*/

string int_to_hexstring(int i)
{
  stringstream ss;
  ss << "0x" << hex << i;
  return ss.str();
}

string int_to_hexstring(int64_t i)
{
  stringstream ss;
  ss << "0x" << hex << i;
  return ss.str();
}

string int_to_hexstring(cpp_int i)
{
  stringstream ss;
  ss << "0x" << hex << i;
  return ss.str();
}

int string_to_int(string s, int def)
{
  if (s.empty())
    return def;
  return atoi(s.c_str());
}

bool
string_is_numeric(string const &s)
{
  std::string::const_iterator it = s.begin();
  while (it != s.end() && std::isdigit(*it)) ++it;
  return !s.empty() && it == s.end();
}

bool is_co_prime(unsigned a, unsigned b)
{
  unsigned c;
  while(a != 0)
  {
     c = a;
     a = b % a;
     b = c;
  }
  return b == 1;
}

static string
get_symbol_name_from_addr(string exec_name, const void *addr)
{
  stringstream stream;
  stream << std::hex << (unsigned long)addr;
  string cmd = "addr2line -f -e " + exec_name + " " + stream.str();
  //DEBUG("cmd: " << cmd << endl);
  FILE *p = popen(cmd.c_str(), "r");
  ASSERT(p);
  stringstream ss;
  char *ret = fgets(as1, sizeof(as1), p);
  ASSERT(ret);
  pclose(p);
  char *newline = strchr(as1, '\n');
  ASSERT(newline);
  *newline = '\0';

  //DEBUG(__func__ << ": returning " << as1 << " for (" << exec_name << ", " << addr << ")" << endl);
  return string(as1);
}

static vector<string>
backtrace_symbols_my(const void *tracePtrs[], int count)
{
  static map<const void *, string> symbol_cache;
  vector<string> ret;
  for (int i = 0; i < count; i++) {
    if (symbol_cache.count(tracePtrs[i]) == 0) {
      ASSERT(g_exec_name);
      string symbol = get_symbol_name_from_addr(g_exec_name, tracePtrs[i]);
      symbol_cache[tracePtrs[i]] = symbol;
    }
    ret.push_back(symbol_cache.at(tracePtrs[i]));
  }
  return ret;
}

string get_backtrace_string()
{
  string ret = "";

  static const void* tracePtrs[1024];
  int count = backtrace((void **)tracePtrs, 1024);
  //char** funcNames = backtrace_symbols(tracePtrs, count);
  vector<string> funcNames = backtrace_symbols_my(tracePtrs, count);

  bool first = true;
  // Print the stack trace
  for( int ii = 1; ii < count; ii++ ) {
    //ret += (first ? "" : "-") + string(funcNames[ii]);
    ret += (first ? "" : "-") + funcNames[ii];
    first = false;
  }
  // Free the string pointers
  //free(funcNames);

  return ret;
}

unsigned get_compressed_string_zlib_size(const string& str)
{
  stringstream decompressed;
  decompressed << str;

  boost::iostreams::filtering_streambuf<boost::iostreams::input> out;

  out.push(boost::iostreams::zlib_compressor());
  out.push(decompressed);

  stringstream compressed;
  boost::iostreams::copy(out, compressed);
  return compressed.str().length();
}

string join_stringlist(const list<string>& strs, const string& separator)
{
  string ret = "";
  bool first = true;
  for(const auto& str : strs)
  {
    if(first)
    {
      ret += str;
      first = false;
    }
    else
      ret += (separator + str);
  }
  return ret;
}

string get_unique_name(const char* prefix)
{
  static map<string, unsigned> count;
  if(count.count(prefix) > 0)
    count[prefix]++;
  else
    count[prefix] = 0;
  return prefix + to_string(count[prefix]);
}

void remove_space(string &str)
{
  std::string::iterator end_pos = std::remove(str.begin(), str.end(), ' ');
  str.erase(end_pos, str.end());
}

size_t
vector_max(vector<size_t> const &v)
{
  size_t ret = 0;
  for (auto e : v) {
    ret = max(e, ret);
  }
  return ret;
}

bool
stob(string const &s)
{
  ASSERT(s == "true" || s == "false");
  return s == "true";
}

unsigned long
round_up_to_nearest_power_of_ten(unsigned long n)
{
  long ret = 1;
  while (n) {
    n = n/10;
    ret = ret*10;
  }
  return ret;
}

void
string_replace(string &haystack, string const &pattern, string const &replacement)
{
  string::size_type pos = 0u;
  while ((pos = haystack.find(pattern, pos)) != string::npos) {
    haystack.replace(pos, pattern.length(), replacement);
    pos += replacement.length();
  }
}

bool
string_contains(string const &haystack, string const &pattern)
{
  return haystack.find(pattern) != string::npos;
}

bool
string_has_suffix(string const &s, string const &expected_suffix)
{
  if (s.length() < expected_suffix.length()) {
    return false;
  }
  return strncmp(s.c_str()+s.length()-expected_suffix.length(), expected_suffix.c_str(), expected_suffix.length()) == 0;
}

bool is_line(const string& line, const string& keyword)
{
  return line.find(keyword) != string::npos;
}

bool
stringIsInteger(string const &s)
{
  if (   s.empty()
      || (   (!isdigit(s[0]))
          && (s[0] != '-') && (s[0] != '+'))) {
    return false ;
  }
  char *p;
  strtol(s.c_str(), &p, 10) ;
  return (*p == 0) ;
}

bool
stringStartsWith(string const &haystack, string const &needle)
{
  return haystack.substr(0, needle.size()) == needle;
}

string
string_get_maximal_alpha_prefix(string const& s)
{
  string ret;
  for (auto const& c : s) {
    if (!isalpha(c))
      return ret;
    ret += c;
  }
  return ret;
}

string
read_section(ifstream &infile, char const *endmarker)
{
  string line, ret;
  while (getline(infile, line)) {
    if (line == endmarker) {
      break;
    }
    ret += line + "\n";
  }
  return ret;
}

string
chrono_duration_to_string(chrono::duration<double> const &d, streamsize const& precision)
{
  stringstream ss;
  long long msecs = chrono::duration_cast<chrono::microseconds>(d).count();
  double secs = msecs/1e+6;
  ss.setf(ios::fixed,ios::floatfield);
  ss.precision(precision);
  ss << secs;
  return ss.str();
}

string
file_to_string(string const &filename)
{
  ifstream in(filename);
  ASSERT(in.is_open());
  string ret((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
  in.close();
  return ret;
}

unsigned long round_up_to_multiple_of(unsigned long x, unsigned long m)
{
  return ((x+m-1) / m) * m;
}

void
file_truncate(string const &filename)
{
  ofstream ofs;
  ofs.open(filename, ofstream::out);
  ofs.close();
}

list<string>
splitLines(string const& str)
{
  istringstream iss(str);
  list<string> ret;
  string line;
  while (getline(iss, line)) {
    ret.push_back(line);
  }
  return ret;
}

vector<string>
splitOnChar(string const& str, char ch)
{
  vector<string> ret;
  string chars(1, ch);
  boost::split(ret, str, boost::is_any_of(chars));
  return ret;
}

string
increment_marker_number(string const& filename)
{
  boost::filesystem::path filepath(filename);
  ASSERT(filepath.has_extension());
  boost::filesystem::path extpath = filepath.extension();
  boost::filesystem::path stempath = filepath.stem();
  boost::filesystem::path dirpath = filepath.parent_path();
  string ext = extpath.native();
  if (ext != ".bc") {
    cout << __func__ << " " << __LINE__ << ": filename = " << filename << ", ext = " << ext << endl;
  }
  ASSERT(ext == ".bc");
  if (   !stempath.has_extension()
      || !string_has_prefix(stempath.extension().native(), "." QCC_MARKER_PREFIX)) {
    return dirpath.native() + "/" + stempath.native() + "." + QCC_MARKER_PREFIX + "0" + ext;
  } else {
    ext = stempath.extension().native();
    stempath = stempath.stem();
    string marker_num_str = ext.substr(strlen("." QCC_MARKER_PREFIX));
    int marker_num = string_to_int(marker_num_str, -1);
    ASSERT(marker_num != -1);
    marker_num++;
    stringstream ss;
    ss << dirpath.native() + "/" + stempath.native() << "." QCC_MARKER_PREFIX << marker_num << ".bc";
    return ss.str();
  }
}

string
gen_ascii_string_for_characters(vector<char> const& chrs)
{
  stringstream ss;
  ss << "\"";
  for (auto const& chr : chrs) {
    ss << "\\" << int(chr);
  }
  ss << "\"";
  return ss.str();
}

bool
files_differ(string const& p1, string const& p2)
{
  std::ifstream f1(p1, std::ifstream::binary|std::ifstream::ate);
  std::ifstream f2(p2, std::ifstream::binary|std::ifstream::ate);

  if (f1.fail() || f2.fail()) {
    return true; //file problem
  }

  if (f1.tellg() != f2.tellg()) {
    return true; //size mismatch
  }

  //seek back to beginning and use std::equal to compare contents
  f1.seekg(0, std::ifstream::beg);
  f2.seekg(0, std::ifstream::beg);
  return !std::equal(std::istreambuf_iterator<char>(f1.rdbuf()),
                    std::istreambuf_iterator<char>(),
                    std::istreambuf_iterator<char>(f2.rdbuf()));
}

list<string>
file_get_lines(string const& filename)
{
  ifstream ifs(filename);
  list<string> ret;
  string line;
  while (getline(ifs, line)) {
    ret.push_back(line);
  }
  ifs.close();
  return ret;
}

void
file_copy(string const& src, string const& dst)
{
  boost::filesystem::path src_path(src);
  boost::filesystem::path dst_path(dst);
  boost::filesystem::copy_file(src_path, dst_path, boost::filesystem::copy_option::overwrite_if_exists);
}

string
convert_malloc_callee_to_mymalloc(string const& callee_name)
{
  if (callee_name == "malloc") {
    return "my_malloc";
  } else if (callee_name == "free") {
    return "my_free";
  } else if (callee_name == "realloc") {
    return "my_realloc";
  } else {
    return callee_name;
  }
}

long
fscanf_entry_section(FILE *fp, char *src_assembly, size_t src_assembly_size)
{
  //autostop_timer func_timer(__func__);
  char *ptr, *end, chr;
  long linenum;
  int ret;

  ptr = src_assembly + 1;
  end = src_assembly + src_assembly_size;
  linenum = 0;
  src_assembly[0] = '\0';

  //printf("hello. fp = %p.\n", fp);
  //fflush(stdout);
  while (ret = fread(ptr, 1, 1, fp)) {
    //printf("*ptr = %c.\n", *ptr);
    ASSERT(ret == 1);
    //fflush(stdout);
    if (*ptr == '-' && *(ptr - 1) == '-') {
      ptr--;
      *ptr = '\0';
      break;
    }
    if (*ptr == '=' && *(ptr - 1) == '=') {
      ptr--;
      *ptr = '\0';
      break;
    }
    if (*ptr == '\n') {
      linenum++;
    }
    if (*ptr == ';') {
      *ptr = '\n';
    }
    ptr++;
    ASSERT(ptr < end);
  }
  ASSERT(ret == 1 || feof(fp));
  if (feof(fp)) {
    return EOF;
  }
  ASSERT(ptr > src_assembly);
  ASSERT(ptr < end);
  ASSERT(*ptr == '\0');
  do {
    ret = fread(&chr, 1, 1, fp);
    ASSERT(ret == 1);
  } while (chr != '\n' && chr != ';');
  ASSERT(chr == '\n' || chr == ';');
  memmove(src_assembly, src_assembly + 1, strlen(src_assembly + 1) + 1);
  ASSERT(strlen(src_assembly) < src_assembly_size);
  return linenum;
/*
  char line[2048];
  long linenum = 0;

  ASSERT(fp);
  src_assembly[0] = '\0';
  for (;;) {
    int disas, num_read;
    num_read = fscanf(fp, "%[^\n]\n", line);
    if (num_read == EOF) {
      return EOF;
    }
    ASSERT(strlen(line) < sizeof line);
    linenum++;
    DBG2(PEEP_TAB, "line = %s\n", line);
    ASSERT(num_read == 1);

    if (!strcmp(line, "--") || !strcmp(line, "==")) {
      break;
    }
    strcat(src_assembly, line);
    strcat(src_assembly, "\n");
    ASSERT(strlen(src_assembly) < src_assembly_size);
  }
  return linenum;
*/
}


void
clean_query_files()
{
  rmrf(g_query_dir.c_str());
}

void
g_query_dir_init()
{
  stringstream ss;
  pid_t pid = getpid();
  char* uname = getenv("LOGNAME"); // get user name
  ASSERT(uname);
  ss << SMT_SOLVER_TMP_FILES << "/" << uname << "." << pid;
  g_query_dir = ss.str();
  mkdir(SMT_SOLVER_TMP_FILES, S_IRWXU | S_IRWXG | S_IRWXO); // allow access to everyone
  mkdir(g_query_dir.c_str(), S_IRUSR | S_IWUSR | S_IXUSR); // private to user
}


