#ifndef SUPPORT_BACKTRACKER_H
#define SUPPORT_BACKTRACKER_H
#include <list>
#include "support/debug.h"

class backtracker;

class backtracker_node_t
{
public:
  //found_solution: this node is the backtracking solution
  //enumerated_children: this node is not the solution and the children nodes were enumerated (children.size == 0 is equivalent to no children)
  //delayed_exploration: while exploring this backtracking node, we realized that this is not the best candidate for exploration currently; so we return to the caller so it may choose another candidate to explore. Care needs to be taken to ensure that there is no infinite loop here (e.g., the caller should not try to explore the same node again immediately).
  using backtracker_explore_retval_t = enum { FOUND_SOLUTION, ENUMERATED_CHILDREN, DELAYED_EXPLORATION };
  backtracker_node_t()
  {
  }
  virtual ~backtracker_node_t() {}

  backtracker_explore_retval_t explore(backtracker &bt);
  backtracker_node_t const* get_parent_node() const
  {
    return m_parent;
  }

  //virtual backtracker_node_t *clone() const = 0;
  std::list<backtracker_node_t *> get_siblings() const
  {
    std::list<backtracker_node_t *> ret = this->m_parent->m_children;
    for (std::list<backtracker_node_t *>::iterator iter = ret.begin(); iter != ret.end(); iter++) {
      if (*iter == this) {
        ret.erase(iter);
        break;
      }
    }
    return ret;
  }

  void remove_siblings(std::list<backtracker_node_t *> const& siblings) const
  {
    for(auto sibling : siblings)
    {
      ASSERT(sibling != this);
      std::list<backtracker_node_t *>::iterator found = m_parent->m_children.end();
      for (std::list<backtracker_node_t *>::iterator iter = m_parent->m_children.begin();
           iter != m_parent->m_children.end();
           iter++) {
        if (*iter == sibling) {
          found = iter;
          break;
        }
      }
      ASSERT(found != m_parent->m_children.end());
      m_parent->m_children.erase(found);
      delete sibling;
      //child->destroy();
    }
  }

private:

  //get_next_level_possibilities():
  //the method generates the next level possibilities
  //if it has exhausted all its next level possibilities, returns an empty set
  //it may modify the current node (hence non-const)
  //returns true iff the current node is the desired solution
  virtual backtracker_explore_retval_t get_next_level_possibilities(std::list<backtracker_node_t *> &children, backtracker &bt) = 0;

  void remove_child(backtracker_node_t *child, backtracker &bt)
  {
    std::list<backtracker_node_t *>::iterator found = m_children.end();
    for (std::list<backtracker_node_t *>::iterator iter = m_children.begin();
         iter != m_children.end();
         iter++) {
      if (*iter == child) {
        found = iter;
        break;
      }
    }
    ASSERT(found != m_children.end());
    m_children.erase(found);
    delete child;
    //child->destroy();
    if (m_children.size()) {
      return;// true;
    }
    this->explore(bt);
  }

  backtracker_node_t *m_parent;
  std::list<backtracker_node_t *> m_children;

  friend class backtracker;
};

class backtracker
{
public:
  backtracker(backtracker_node_t *root)
  {
    //m_root = new backtracker_node_t(root);
    //m_root = root.clone();
    m_root = root;
    ASSERT(m_root);
    m_root->m_parent = NULL;
    m_root->m_children = std::list<backtracker_node_t *>();
  }
  ~backtracker()
  {
    if (m_root) {
      backtracker_tree_free(m_root);
    }
  }

  std::list<backtracker_node_t *> get_frontier() const
  {
    std::list<backtracker_node_t *> ret;
    if (m_root) {
      get_frontier_helper_rec(m_root, ret);
    }
    return ret;
  }

private:
  static void get_frontier_helper_rec(backtracker_node_t *cur, std::list<backtracker_node_t *> &ret)
  {
    if (cur->m_children.size() == 0) {
      ret.push_back(cur);
      return;
    }
    for (auto child : cur->m_children) {
      get_frontier_helper_rec(child, ret);
    }
  }

  static void backtracker_tree_free(backtracker_node_t *cur)
  {
    for (auto child : cur->m_children) {
      backtracker_tree_free(child);
    }
    delete cur;
    //cur->destroy();
  }

  backtracker_node_t *m_root;

  friend class backtracker_node_t;
};


#endif
