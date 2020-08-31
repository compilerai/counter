#include "command.h"

namespace eqspace {

void command_stack::push(command* cmd)
{
  m_stack.push_back(cmd);
  cmd->apply();
}

void command_stack::undo_all()
{
  while(!m_stack.empty())
  {
    command* cmd = m_stack.back();
    m_stack.pop_back();
    cmd->undo();
    delete cmd;
  }
}
}
