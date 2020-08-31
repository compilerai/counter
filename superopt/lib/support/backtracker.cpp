#include "support/backtracker.h"

backtracker_node_t::backtracker_explore_retval_t
backtracker_node_t::explore(backtracker &bt)
{
  ASSERT(this->m_children.size() == 0);
  std::list<backtracker_node_t *> children;
  backtracker_explore_retval_t status = this->get_next_level_possibilities(children, bt);
  if (status == backtracker_node_t::FOUND_SOLUTION) {
    return backtracker_node_t::FOUND_SOLUTION;
  }
  if (status == backtracker_node_t::DELAYED_EXPLORATION) {
    return backtracker_node_t::DELAYED_EXPLORATION;
  }
  if (!children.size()) {
    if (!this->m_parent) {
      ASSERT(this == bt.m_root);
      bt.m_root = nullptr;
      delete this;
    } else {
      this->m_parent->remove_child(this, bt);
    }
    return backtracker_node_t::ENUMERATED_CHILDREN;
  }
  for (const auto &child : children) {
    //backtracker_node_t *cnode = new backtracker_node_t(child);
    //backtracker_node_t *cnode = child->clone();
    //ASSERT(cnode);
    backtracker_node_t *cnode = child;
    cnode->m_parent = this;
    cnode->m_children = std::list<backtracker_node_t *>();
    this->m_children.push_back(cnode);
  }
  return backtracker_node_t::ENUMERATED_CHILDREN;
}


