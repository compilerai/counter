#ifndef EQCHECKCOMMAND_H
#define EQCHECKCOMMAND_H

#include <list>

namespace eqspace {

class command
{
public:
  virtual ~command() {}
  virtual void apply() {}
  virtual void undo() {}
};

class command_stack
{
public:
  void push(command* cmd);
  void undo_all();

private:
  std::list<command*> m_stack;
};

}

#endif
