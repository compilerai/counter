#pragma once

#include <iostream>    
#include <list>
#include <string>
#include <map>
#include <chrono>

using namespace std;

class manager_base;

class mytimer
{
public:
  mytimer() : m_name(""), m_start(0), m_num_entries(0), m_elapsed(0), m_running(0) {}
  mytimer(string const& name) : m_name(name), m_start(0), m_num_entries(0), m_elapsed(0), m_running(0) {}
  void clear();
  void start();
  void stop();
  void accumulate(long long elapsed);
  void accumulate_and_increment_num_entries(long long elapsed);
  string get_name() const;
  friend ostream& operator<<(ostream& os, const mytimer& t);
  string to_string(string const& result_suffix = "") const;
  bool is_running() { return m_running > 0; }
  long long get_elapsed() const { return m_elapsed; }
  float get_elapsed_secs() const { return m_elapsed / 1.0e6; }
  long long get_elapsed_so_far() const;
  long long get_elapsed_in_current_run() const;

private:
  string m_name;
  long long m_start;
  long long m_num_entries;
  long long m_elapsed;
  int m_running;
};

class stats
{
public:
  ~stats();
  stats(const stats&) = delete;
  stats& operator=(const stats&) = delete;

  static stats& get() {
    static stats instance;
    return instance;
  }

  void clear();
  void set_result_suffix(string const &result_suffix)
  {
    m_result_suffix = result_suffix;
  }
  mytimer* get_timer(string name);
  friend ostream& operator<<(ostream& os, stats& tf);
  void clear_counter(string const &name);
  void inc_counter(string const &name);
  void add_counter(string name, long long elapsed);
  void set_flag(string name, bool f);
  void set_value(string name, unsigned val);
  void set_value(string name, string val);
  void add_timer(mytimer *);
  static string mem_stats_to_string();
  void print_smt_solver_counters() const;

  void register_manager(string const& name, manager_base const& m);
  void deregister_manager(string const& name);

  void register_object(string const& name);
  void deregister_object(string const& name);

  unsigned get_counter_value(string name) const;

  void new_bv_solve_input(int rows, int cols) {
    m_bv_solve_input_sizes.push_back(make_pair(rows,cols));
  }

  pair<int,int> get_max_bv_solve_matrix_size() const
  {
    pair<int,int> ret{0,0};
    for (auto const& p : m_bv_solve_input_sizes) {
      if (p.first*p.second > ret.first*ret.second) {
        ret = p;
      }
    }
    return ret;
  }

  pair<double,double> get_avg_bv_solve_matrix_rows_and_cols() const
  {
    if (m_bv_solve_input_sizes.empty()) {
      return {-1,-1};
    }
    double row_sum = 0;
    double col_sum = 0;
    for (auto const& p : m_bv_solve_input_sizes) {
      row_sum += p.first;
      col_sum += p.second;
    }

    int n = m_bv_solve_input_sizes.size();
    return make_pair(row_sum/n, col_sum/n);
  }

  void print_object_counts(ostream& os) const;
  void print_manager_sizes(ostream& os) const;

  bool have_bv_solve_counters() const
  {
    return !m_bv_solve_input_sizes.empty();
  }


private:
  stats() {}
  void stop_all_timers();
  unsigned stats_get_counter_val(string const& counter_name) const;

  map<string, mytimer*> m_timers;
  map<string, unsigned> m_counters;
  map<string, unsigned> m_flags;
  map<string, string> m_info;
  string m_result_suffix;
  list<pair<int,int>> m_bv_solve_input_sizes;
  map<string, manager_base const*> m_managers;
  map<string, size_t> m_num_object_constructions;
  map<string, size_t> m_num_object_destructions;
};

//extern stats g_stats;

class autostop_timer
{
public:
  autostop_timer(string const& name) : m_name(name)
  {
    //g_stats.create_timer(m_name)->start();
    //cout << "autostop_timer constructor called with name " << name << " calling timer.start()" << endl;
    stats::get().get_timer(m_name)->start();
    //cout << "autostop_timer constructor done with name " << name << endl;
  }

  ~autostop_timer()
  {
    //cout << "autostop_timer destructor called with name " << m_name << " calling timer.stop()" << endl;
    stats::get().get_timer(m_name)->stop();
    //cout << "autostop_timer destructor done with name " << m_name << endl;
    //g_stats.create_timer(m_name)->stop();
  }
  void accumulate_current_elapsed_time_to_another_timer(string const& other_timer_name) const;


  string m_name;
};

class autostop_timer2
{
public:
  autostop_timer2(mytimer* t) : m_t(t)
  {
    m_t->start();
  }

  ~autostop_timer2()
  {
    m_t->stop();
  }

private:
  mytimer* m_t;
  string m_name;
};

class autostop_timer_cb : public autostop_timer
{
public:
  autostop_timer_cb(string const& name, function<void(chrono::duration<double> const&)> cb)
   : autostop_timer(name),
     m_start(chrono::system_clock::now()),
     m_callback(cb)
  { }

  ~autostop_timer_cb()
  {
    m_callback(chrono::system_clock::now()-m_start);
  }

private:
  chrono::time_point<std::chrono::system_clock> m_start;
  function<void(chrono::duration<double> const&)> m_callback;
};

//#define FUNCTION_TIMER static mytimer fun_n_a_m_e(__func__); autostop_timer2 fun_n_a_m_e2(&fun_n_a_m_e)

ostream& operator<<(ostream& os, autostop_timer const &t);
void g_stats_print(void);
