#include "sig_handling.h"

#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <assert.h>

namespace eqspace
{

void sig_handling::set_alarm_handler(unsigned timeout_secs, void (*handler)(int))
{
  signal(SIGALRM, handler);
  struct itimerval timer;
  timer.it_value.tv_sec = timeout_secs;
  timer.it_value.tv_usec = 0;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 0;

  int set_handler = setitimer(ITIMER_REAL, &timer, 0);
  assert(!set_handler);
}

void sig_handling::set_int_handler(void (*handler)(int))
{
  signal(SIGINT, handler);
}

}
