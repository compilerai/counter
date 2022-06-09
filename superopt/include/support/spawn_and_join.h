#pragma once

#include <functional>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>

#include "support/debug.h"

template<typename T_RET>
T_RET spawn_and_join_first_and_cancel_others(std::function<T_RET (std::atomic<bool>&)> const& f1, std::function<T_RET (std::atomic<bool>&)> const& f2)
{
  std::mutex m;
  std::condition_variable cond;
  //std::atomic<std::thread::id> val;
  T_RET ret1, ret2;
  bool done1 = false, done2 = false;
  std::atomic<bool> tasks_should_exit = false;

  auto task1 = [&] {
      //std::this_thread::sleep_for(1s); // Some work
      //val = std::this_thread::get_id();
      ret1 = f1(tasks_should_exit);
      unique_lock<mutex> lock{m};
      done1 = true;
      cond.notify_all();
  };

  auto task2 = [&] {
      //std::this_thread::sleep_for(1s); // Some work
      //val = std::this_thread::get_id();
      ret2 = f2(tasks_should_exit);
      unique_lock<mutex> lock{m};
      done2 = true;
      cond.notify_all();
  };

  DYN_DEBUG(qd_process_dbg, cout << _FNLN_ << ": spawning thread1\n");
  std::thread thread1(task1);
  DYN_DEBUG(qd_process_dbg, cout << _FNLN_ << ": spawning thread2\n");
  std::thread thread2(task2);

  std::unique_lock<std::mutex> lock{m};
  cond.wait(lock, [&] { return done1 || done2; });
  lock.unlock();

  tasks_should_exit = true;

  DYN_DEBUG(qd_process_dbg, cout << _FNLN_ << ": joining thread1\n");
  thread1.join();
  DYN_DEBUG(qd_process_dbg, cout << _FNLN_ << ": joined thread1\n");
  DYN_DEBUG(qd_process_dbg, cout << _FNLN_ << ": joining thread2\n");
  thread2.join();
  DYN_DEBUG(qd_process_dbg, cout << _FNLN_ << ": joined thread2\n");

  if (done1) return ret1;
  if (done2) return ret2;
  NOT_REACHED();
}
