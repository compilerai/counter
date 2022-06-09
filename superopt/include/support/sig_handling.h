#ifndef SIG_HANDLING_H
#define SIG_HANDLING_H

namespace eqspace
{

// TODO make it singleton
class sig_handling
{
public:
  void set_alarm_handler(unsigned timeout_secs, void (*handler)(int));
  void set_int_handler(void (*handler)(int));
};

}
#endif
