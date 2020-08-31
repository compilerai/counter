#ifndef CL_COMMAND_LINE_H
#define CL_COMMAND_LINE_H

#include <vector>
#include <string>
#include <sstream>
#include <list>
#include <cassert>

namespace cl
{

enum arg_type
{
  explicit_prefix = 0,
  explicit_flag,
  implicit_prefix
};

class cl;

class arg_base
{
public:
  friend cl;

  arg_base(arg_type t, const std::string& prefix, const std::string& help, bool default_set) :
    m_type(t), m_prefix(prefix), m_set(false), m_help(help), m_is_optional(default_set) { }
  virtual ~arg_base() { }

  const std::string& get_prefix() const
  {
    return m_prefix;
  }

  arg_type get_type() const
  {
    return m_type;
  }

  void set_set(bool val)
  {
    m_set = val;
  }

  bool get_set() const
  {
    return m_set;
  }

  bool is_flag() const
  {
    return m_type == explicit_flag;
  }

  bool is_optional()
  {
    return m_is_optional;
  }

  virtual std::string help_string() = 0;
  virtual bool parse(const std::string& s) = 0;

protected:
  arg_type m_type;
  std::string m_prefix;
  bool m_set;
  std::string m_help;
  bool m_is_optional;
  std::string m_default;
};


template <typename T>
class arg : public arg_base
{
public:
  arg(arg_type t, const std::string& prefix, const T& default_val, const std::string& help) :
    arg_base(t, prefix, help, true), m_value(default_val) { }
  arg(arg_type t, const std::string& prefix, const std::string& help) :
    arg_base(t, prefix, help, false) { }

  bool parse(const std::string& s) override
  {
    std::istringstream iss;
    iss.str(s);
    iss >> m_value;
    return true;
  }

  T get_value() const
  {
    return m_value;
  }

  void set_value(T t)
  {
    m_value = t;
  }

  virtual std::string help_string() override
  {
    std::stringstream ss;
    if(get_type() == explicit_prefix || get_type() == explicit_flag)
      ss << "--" << get_prefix();
    else if(get_type() == implicit_prefix)
      ss << "<" << m_help << ">";
    if(is_optional())
    {
      if(get_type() != explicit_flag)
        ss << " <Default: " << m_value << ">" << "\t: " << m_help;
      else
        ss << "\t: " << m_help;
    }

    return ss.str();
  }

private:
  T m_value;
};

class cl
{
public:
  cl(const std::string& overview) : m_overview(overview)
  {
    m_help_arg = new arg<bool>(explicit_flag, "help", false, "Prints this help message");
    add_arg(m_help_arg);
  }
  ~cl()
  {
    delete m_help_arg;
  }

  void add_arg(arg_base* a)
  {
    if(!a->get_prefix().empty())
    {
      for(arg_base* curr : m_args)
      {
        if(curr->get_prefix() == a->get_prefix())
          assert(false && "duplicate prefix used");
      }
    }
    m_args.push_back(a);
  }

  void parse(int argc, char **argv)
  {
    generate_help_string();
    process_input(argc, argv);
    if(m_help_arg->get_set())
      print_help_and_exit("");

    if(!is_all_compulsory_args_set())
      print_help_and_exit("Required argument(s) not set.");

    print_parsed_args();
  }

  bool is_all_compulsory_args_set() const;

  void print_parsed_args() const;

private:
  void process_input(int argc, char **argv);
  void print_help_and_exit(const std::string& err);
  void print_help();
  void generate_help_string();

  arg_base* get_first_unmatched_arg(const std::string& str);
  arg_base* get_first_arg(const std::string& str);

private:
  std::string m_overview;
  std::vector<arg_base*> m_args;
  std::list<std::string> m_input;
  std::string m_help_string;
  arg<bool>* m_help_arg;
};

std::vector<int> parse_int_list(std::string const& s);


}

#endif

