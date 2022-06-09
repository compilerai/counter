#ifndef SUPPORT_BACKTRACKER_H
#define SUPPORT_BACKTRACKER_H
#include <list>
#include <functional>
#include "support/debug.h"
#include "support/dshared_ptr.h"

class backtracker;

class backtracker_node_t : public enable_shared_from_this<backtracker_node_t>
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
  dshared_ptr<backtracker_node_t const> get_parent_node() const
  {
    return m_parent;
  }

  //virtual backtracker_node_t *clone() const = 0;
  std::list<dshared_ptr<backtracker_node_t>> get_siblings() const
  {
    std::list<dshared_ptr<backtracker_node_t>> ret = this->m_parent->m_children;
    for (std::list<dshared_ptr<backtracker_node_t>>::iterator iter = ret.begin(); iter != ret.end(); iter++) {
      if (*iter == this->dshared_from_this()) {
        ret.erase(iter);
        break;
      }
    }
    return ret;
  }

  void remove_siblings(std::list<dshared_ptr<backtracker_node_t>> const& siblings) const
  {
    for(auto sibling : siblings)
    {
      ASSERT(sibling != this->dshared_from_this());
      std::list<dshared_ptr<backtracker_node_t>>::iterator found = m_parent->m_children.end();
      for (std::list<dshared_ptr<backtracker_node_t>>::iterator iter = m_parent->m_children.begin();
           iter != m_parent->m_children.end();
           iter++) {
        if (*iter == sibling) {
          found = iter;
          break;
        }
      }
      ASSERT(found != m_parent->m_children.end());
      m_parent->m_children.erase(found);
      //delete sibling;
    }
  }
  void add_siblings(std::list<dshared_ptr<backtracker_node_t>> const& siblings);

protected:
  dshared_ptr<backtracker_node_t const> dshared_from_this() const
  {
    return dshared_ptr<backtracker_node_t const>(this->shared_from_this());
  }

  dshared_ptr<backtracker_node_t> dshared_from_this()
  {
    return dshared_ptr<backtracker_node_t>(this->shared_from_this());
  }


private:

  virtual bool backtracker_node_is_useless() const = 0;

  //get_next_level_possibilities():
  //the method generates the next level possibilities
  //if it has exhausted all its next level possibilities, returns an empty set
  //it may modify the current node (hence non-const)
  //returns true iff the current node is the desired solution
  virtual backtracker_explore_retval_t get_next_level_possibilities(std::list<dshared_ptr<backtracker_node_t>> &children/*, backtracker &bt*/) = 0;

  void set_children(list<dshared_ptr<backtracker_node_t>> new_children) { m_children = new_children; }


  dshared_ptr<backtracker_node_t> m_parent;
  std::list<dshared_ptr<backtracker_node_t>> m_children;

  friend class backtracker;
};

class backtracker
{
public:
  backtracker()
  {
    m_root = dshared_ptr<backtracker_node_t>::dshared_nullptr();
  }

  ~backtracker()
  {
    if (m_root) {
      backtracker_tree_free(m_root);
      m_root = dshared_ptr<backtracker_node_t>::dshared_nullptr();
    }
  }

  void backtracker_remove_node(dshared_ptr<backtracker_node_t> node)
  {
    if (!node->m_parent) {
      ASSERT(node == m_root);
      m_root = dshared_ptr<backtracker_node_t>::dshared_nullptr();
    } else {
       dshared_ptr<backtracker_node_t> parent = node->m_parent;
       std::list<dshared_ptr<backtracker_node_t>>::iterator found = parent->m_children.end();
       for (std::list<dshared_ptr<backtracker_node_t>>::iterator iter = parent->m_children.begin();
            iter != parent->m_children.end();
            iter++) {
         if (*iter == node) {
           found = iter;
           break;
         }
       }
       ASSERT(found != parent->m_children.end());
       parent->m_children.erase(found);
    }
    //delete node;
  }

  void backtracker_set_root(dshared_ptr<backtracker_node_t> root)
  {
    m_root = root;
    ASSERT(m_root);
    m_root->m_parent = dshared_ptr<backtracker_node_t>::dshared_nullptr();
    m_root->m_children = std::list<dshared_ptr<backtracker_node_t>>();
  }



  std::list<dshared_ptr<backtracker_node_t>> get_frontier_after_eliminating_useless_nodes()
  {
    this->eliminate_useless_nodes();
    return this->get_frontier();
  }
  
  void eliminate_useless_subtree()
  {
    std::function<bool (int, int)> eliminate_subtree = [](int num_old_children, int num_new_children){ return (num_old_children > 0 && num_new_children == 0); }; 
    if (m_root) {
      m_root = eliminate_useless_nodes_rec(m_root, eliminate_subtree);
    }
  }

private:
  void eliminate_useless_nodes()
  {
    std::function<bool (int, int)> do_not_eliminate_subtree = [](int num_old_children, int num_new_children){ return false; }; 
    if (m_root) {
      m_root = eliminate_useless_nodes_rec(m_root, do_not_eliminate_subtree);
    }
  }
  

  std::list<dshared_ptr<backtracker_node_t>> get_frontier() const
  {
    std::list<dshared_ptr<backtracker_node_t>> ret;
    if (m_root) {
      get_frontier_helper_rec(m_root, ret);
    }
    return ret;
  }


  static dshared_ptr<backtracker_node_t> eliminate_useless_nodes_rec(dshared_ptr<backtracker_node_t> cur, std::function<bool (int, int )> eliminate_subtree_cond)
  {
    if (cur->m_children.size() == 0 && cur->backtracker_node_is_useless()) {
      //delete cur;
      return dshared_ptr<backtracker_node_t>::dshared_nullptr();
    }
    list<dshared_ptr<backtracker_node_t>> new_children;
    for (auto child : cur->m_children) {
      dshared_ptr<backtracker_node_t> new_child = eliminate_useless_nodes_rec(child, eliminate_subtree_cond);
      if (!new_child) {
        continue;
      }
      new_children.push_back(new_child);
    }
    if (eliminate_subtree_cond(cur->m_children.size(), new_children.size())) { 
      //delete cur;
      return dshared_ptr<backtracker_node_t>::dshared_nullptr();
    }
    cur->set_children(new_children);
    return cur;
  }



  static void get_frontier_helper_rec(dshared_ptr<backtracker_node_t> cur, std::list<dshared_ptr<backtracker_node_t>> &ret)
  {
    if (cur->m_children.size() == 0) {
      ret.push_back(cur);
      return;
    }
    for (auto child : cur->m_children) {
      get_frontier_helper_rec(child, ret);
    }
  }

  static void backtracker_tree_free(dshared_ptr<backtracker_node_t> cur)
  {
    for (auto child : cur->m_children) {
      backtracker_tree_free(child);
    }
    //delete cur;
  }

  dshared_ptr<backtracker_node_t> m_root;

  friend class backtracker_node_t;
};


#endif
