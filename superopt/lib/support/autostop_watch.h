#pragma once

#include "support/stopwatch.h"

class autostop_watch {
  struct time *m_t;
  public:
    autostop_watch(struct time* t) : m_t(t) { stopwatch_run(m_t); }
    ~autostop_watch() { stopwatch_stop(m_t); }
};
