#include <string>
#include <iostream>
#include <sstream>
#include "mytimer.h"
#include "sys/time.h"
#include "log.h"
#include "list"
//#include "gsupport/pc.h"
#include "support/manager.h"
//#include "graph/graph.h"

#include <iostream>
using namespace std;

#define DEBUG_TYPE "stats"
namespace eqspace {

long long get_time()
{
  timeval t;
  int success = gettimeofday(&t, 0);
  assert(!success);
  return t.tv_usec + t.tv_sec*1e6;
}

void mytimer::clear()
{
  ASSERT(m_running == 0);
  m_elapsed = 0;
  m_num_entries = 0;
}

void mytimer::start()
{
  if(m_running == 0)
  {
    m_start = get_time();
    m_num_entries++;
  }
  m_running++;
}

long long mytimer::get_elapsed_so_far() const
{
  if(!m_running)
  {
    //INFO("timer " << m_name << " not running" << endl);
    assert(false);
  }
  return (m_elapsed + get_time() - m_start);
}

void mytimer::stop()
{
  if(!m_running)
  {
    //INFO("timer " << m_name << " not running" << endl);
    assert(false);
  }
  m_running--;
  if (m_running == 0) {
    m_elapsed = m_elapsed + get_time() - m_start;
  }
}

string mytimer::get_name() const
{
  return m_name;
}

ostream& operator<<(ostream& os, const mytimer& t)
{
  os << t.get_name() << ": " << (long long)t.m_elapsed/1e6 << "s (num_starts " << t.m_num_entries << ")" << endl;
  return os;
}

mytimer* stats::get_timer(string name)
{
  //cout << __func__ << " " << __LINE__ << ": name = " << name << endl;
  if(m_timers.count(name) > 0)
    return m_timers[name];

  mytimer* t = new mytimer(name);
  add_timer(t);
  return t;
}

void stats::add_timer(mytimer* t)
{
  assert(m_timers.count(t->get_name()) == 0 && "duplicate timer created");
  m_timers[t->get_name()] = t;
}

void
stats::clear()
{
  m_timers.clear();
  m_bv_solve_input_sizes.clear();
}

ostream& operator<<(ostream& os, stats& tf)
{
  tf.stop_all_timers();
  list<mytimer*> list_timers;
//  os << "timers:" << endl;
//  for(map<string, mytimer*>::const_iterator iter = tf.m_timers.begin(); iter != tf.m_timers.end(); ++iter)
//  {
//    list_timers.push_back(iter->second);
//  }
//  list_timers.sort([](mytimer* a, mytimer* b) { return a->get_elapsed() < b->get_elapsed(); });
//
//  for(list<mytimer*>::const_iterator iter = list_timers.begin(); iter != list_timers.end(); ++iter)
//  {
//    os << (*iter)->to_string(tf.m_result_suffix) << endl;
//  }
//  os << endl;

  os << "counters:" << endl;
  for(map<string, unsigned>::const_iterator iter = tf.m_counters.begin(); iter != tf.m_counters.end(); ++iter)
  {
    os << iter->first << "." << tf.m_result_suffix << ": " << iter->second << endl;
  }
//  if (tf.have_bv_solve_counters()) {
//    auto const& max_bv_solve_mat = tf.get_max_bv_solve_matrix_size();
//    os << "max-bv_solve-matrix-row." << tf.m_result_suffix << ": " << max_bv_solve_mat.first << endl;
//    os << "max-bv_solve-matrix-col." << tf.m_result_suffix << ": " << max_bv_solve_mat.second << endl;
//    auto const& avg_bv_solve_mat = tf.get_avg_bv_solve_matrix_rows_and_cols();
//    os << "avg-bv_solve-matrix-rows." << tf.m_result_suffix << ": " << avg_bv_solve_mat.first << endl;
//    os << "avg-bv_solve-matrix-cols." << tf.m_result_suffix << ": " << avg_bv_solve_mat.second << endl;
//  }
  os << endl;

  os << "flags:" << endl;
  for(map<string, unsigned>::const_iterator iter = tf.m_flags.begin(); iter != tf.m_flags.end(); ++iter)
  {
    os << iter->first << "." << tf.m_result_suffix << ": " << iter->second << endl;
  }
  os << endl;

  os << "info:" << endl;
  for(const auto& kv : tf.m_info)
  {
    os << kv.first << "." << tf.m_result_suffix << ": " << kv.second << endl;
  }
  os << endl;
  return os;
}

stats::~stats()
{
  for(map<string, mytimer*>::const_iterator iter = m_timers.begin(); iter != m_timers.end(); ++iter)
  {
    //delete iter->second;
  }
}

void stats::clear_counter(string const &name)
{
  m_counters[name] = 0;
}

void stats::inc_counter(string const &name)
{
  add_counter(name, 1);
  /*if(m_counters.count(name) > 0)
    m_counters[name]++;
  else
    m_counters[name] = 1;*/
}

void stats::add_counter(string name, long long elapsed)
{
  if (m_counters.count(name) > 0) {
    m_counters[name] += elapsed;
  } else {
    m_counters[name] = elapsed;
  }
}

unsigned stats::get_counter_value(string name) const
{
  if (m_counters.count(name) > 0)
    return m_counters.at(name);
  else return 0; // safe default
}

void stats::set_flag(string name, bool f)
{
  m_flags[name] = f;
}

void stats::set_value(string name, unsigned val)
{
  m_flags[name] = val;
}

void stats::set_value(string name, string val)
{
  m_info[name] = val;
}


void stats::stop_all_timers()
{
  for(map<string, mytimer*>::const_iterator iter = m_timers.begin(); iter != m_timers.end(); ++iter)
  {
    if(iter->second->is_running())
      iter->second->stop();
  }
}

void g_stats_print(void)
{
  cout << "Printing stats::get():\n" << stats::get() << endl;
  cout << "Mem-stats:\n" << stats::mem_stats_to_string() << endl;
}


ostream& operator<<(ostream& os, autostop_timer const &t)
{
  mytimer &tt = *(stats::get().get_timer(t.m_name));
  bool was_running;
  was_running = false;
  if (tt.is_running()) {
    tt.stop();
    was_running = true;
  }
  os << tt;
  if (was_running) {
    tt.start();
  }
  return os;
}

string
mytimer::to_string(string const &result_suffix) const
{
  stringstream ss;
  ss << get_name() << "." << result_suffix << ": " << (long long)m_elapsed/1e6 << "s (num_starts " << m_num_entries << ")";
  return ss.str();
}

string
stats::mem_stats_to_string()
{
  stringstream ss;
/*#ifndef NDEBUG
  ss << "node<pc> allocated: " << node<pc>::get_num_allocated() << endl;
  ss << "edge<pc> allocated: " << edge<pc>::get_num_allocated() << endl;
  ss << "node<pcpair> allocated: " << node<pcpair>::get_num_allocated() << endl;
  ss << "edge<pcpair> allocated: " << edge<pcpair>::get_num_allocated() << endl;
#endif*/
  return ss.str();
}

void
stats::register_manager(string const& name, manager_base const& m)
{
  //cout << __func__ << " " << __LINE__ << ": name = " << name << endl;
  ASSERT(m_managers.count(name) == 0);
  m_managers.insert(make_pair(name, &m));
}

void
stats::deregister_manager(string const& name)
{
  //cout << __func__ << " " << __LINE__ << ": name = " << name << endl;
  m_managers.erase(name);
}

void
stats::print_manager_sizes(ostream& os) const
{
  os << "manager sizes:\n";
  for (auto const& m : m_managers) {
    os << "  " << m.first << " : " << m.second->get_size() << endl;
  }
}

void
stats::print_object_counts(ostream& os) const
{
  os << "object counts:\n";
  for (auto const& m : m_num_object_constructions) {
    size_t num_destructions = m_num_object_destructions.count(m.first) ? m_num_object_destructions.at(m.first) : 0;
    os << "  " << m.first << " : " << m.second << " constructions, " << num_destructions << " destructions, " << (m.second - num_destructions) << " active" << endl;
  }
}

void
stats::register_object(string const& name)
{
  m_num_object_constructions[name]++;
}

void
stats::deregister_object(string const& name)
{
  m_num_object_destructions[name]++;
}

}
