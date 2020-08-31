#include <future>
#include "support/utils.h"

//run f() for timeout_secs; kill it otherwise
//caution: killing is dangerous as it may not release resources; so need to be careful while using this construct
bool timeout(std::function<void ()> f, long timeout_secs)
{
  std::packaged_task<void()> task(f);
  auto future = task.get_future();
  std::thread thr(std::move(task));

  thr.detach(); // we leave the thread still running; terminate will be called on it when we return (through thr's destructor)

  if (future.wait_for(std::chrono::seconds(timeout_secs)) != std::future_status::timeout) {
    future.get(); // this will propagate exception from f() if any
    return true;
  } else {
    return false;
  }
}
