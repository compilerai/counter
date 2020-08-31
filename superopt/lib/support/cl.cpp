#include "cl.h"

#include <string>
#include <iostream>
#include <vector>
#include <boost/algorithm/string/predicate.hpp>

namespace cl
{

using namespace std;

bool is_prefix(const string& str)
{
  return (boost::algorithm::starts_with(str, "--") || boost::algorithm::starts_with(str, "-"));
}

string remove_dashes_and_assignment(const string& str, string& assigned_value)
{
  string ret;
  if(boost::algorithm::starts_with(str, "--"))
    ret = str.substr(2);
  else if(boost::algorithm::starts_with(str, "-"))
    ret = str.substr(1);
  else {
    ret = "";
  }
  size_t found;
  if ((found = ret.find('=')) != string::npos) {
    assigned_value = ret.substr(found + 1);
    ret = ret.substr(0, found);
  }
  return ret;
}

arg_base* cl::get_first_unmatched_arg(const std::string& prefix)
{
  for(arg_base* a : m_args)
  {
    if(!a->get_set() && a->get_prefix() == prefix)
      return a;
  }
  return 0;
}

arg_base* cl::get_first_arg(const std::string& prefix)
{
  for(arg_base* a : m_args)
  {
    if(a->get_prefix() == prefix)
      return a;
  }
  return 0;
}



void cl::process_input(int argc, char **argv)
{
  int i = 1;
  while(i < argc)
  {
    //cout << __func__ << " " << __LINE__ << ": i = " << i << ", argv[i] = " << argv[i] << endl;
    string tok(argv[i]);
    i++;

    string prefix, assigned_value;
    if(is_prefix(tok))
    {
      prefix = remove_dashes_and_assignment(tok, assigned_value);
      if(prefix.empty()) {
        print_help_and_exit(string("Invalid argument: ") + argv[i - 1]);
      }
    }

    arg_base* a = get_first_unmatched_arg(prefix);

    if(!a)
      a = get_first_arg(prefix);

    if(!a)
      print_help_and_exit(string("Invalid argument: ") + argv[i - 1] + ", prefix = " + prefix);

    //cout << __func__ << " " << __LINE__ << ": prefix = " << prefix << endl;
    if(!a->is_flag())
    {
      if(prefix.empty()) {
        a->parse(tok);
      } else if (assigned_value.size()) {
        a->parse(assigned_value);
      } else {
        string tok(argv[i]);
        //cout << __func__ << " " << __LINE__ << ": parsed " << tok << " for prefix = " << prefix << endl;
        i++;
        a->parse(tok);
      }
    }
    else
      a->parse("1");
    a->set_set(true);
  }
}

void cl::print_help()
{
  cout << m_help_string;
}

void cl::generate_help_string()
{
  stringstream ss;
  ss << "Overview:\n  " << m_overview << "\n\n";
  ss << "Usage:\n";

  stringstream ss_prefix_optional;
  stringstream ss_prefix_compulsory;
  stringstream ss_implicit_compulsory;
  for(arg_base* a : m_args)
  {
    if(a->get_type() == explicit_prefix || a->get_type() == explicit_flag)
    {
      if(a->is_optional())
        ss_prefix_optional << "\t" << a->help_string() << "\n";
      if(!a->is_optional())
        ss_prefix_compulsory << "\t" << a->help_string() << "\n";
    }
    else
    {
      ss_implicit_compulsory << "\t" << a->help_string() << "\n";
    }
  }

  ss << "  Optional:\n";
  ss << ss_prefix_optional.str();
  ss << "  Required:\n";
  ss << ss_prefix_compulsory.str();
  ss << ss_implicit_compulsory.str();

  m_help_string = ss.str();
}

void cl::print_help_and_exit(const string& err)
{
  if(!err.empty())
    cout << err << "\n";
  print_help();
  exit(1);
}

void cl::print_parsed_args() const
{
  /*cout << "cmd debug:\n";
  for(arg_base* a : m_args)
  {
    if(a->get_set())
    {
      cout << "prefix: " << a->get_prefix() << ", is_optional: " << a->m_is_optional << "\n";
    }
  }*/
}

bool cl::is_all_compulsory_args_set() const
{
  for(arg_base* a : m_args)
  {
    if(!a->is_optional() && !a->get_set())
      return false;
  }
  return true;
}

vector<int>
parse_int_list(string const& s)
{
  istringstream iss(s);
  cout << __func__ << " " << __LINE__ << ": s = " << s << endl;
  vector<int> ret;
  int i;
  while (iss >> i) {
    ret.push_back(i);
    cout << __func__ << " " << __LINE__ << ": adding " << i << endl;
    char ch;
    if (iss >> ch) {
      assert(ch == ',');
    } else {
      break;
    }
  }
  return ret;
}

}

