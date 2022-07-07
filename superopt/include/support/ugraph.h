#pragma once

#include "support/utils.h"
#include "support/debug.h"
#include "support/types.h"

using namespace std;

template<typename T_N>
class ugraph_t
{
private:
  map<T_N, set<T_N>> m_adj;

  void insert_edge_one_direction(T_N const &a, T_N const &b)
  {
    if (m_adj.count(a)) {
      m_adj.at(a).insert(b);
    } else {
      set<T_N> s;
      s.insert(b);
      m_adj.insert(make_pair(a, s));
    }
  }

  void delete_edge_one_direction(T_N const &a, T_N const &b)
  {
    if (!m_adj.count(a)) {
      return;
    }
    m_adj.at(a).erase(b);
  }

  bool get_path_helper(T_N const &a, T_N const &b, set<T_N> &visited, list<pair<T_N, T_N>> &path) const
  {
    if (a == b) {
      return true;
    }
    if (visited.count(a)) {
      return false;
    }
    //cout << __func__ << " " << __LINE__ << ": a = " << ssm_elem_to_string(a) << "--> b = " << ssm_elem_to_string(b) << endl;
    if (m_adj.count(a) == 0) {
      return false;
    }
    visited.insert(a);
    for (const auto &n : m_adj.at(a)) {
      //list<pair<T_N, T_N>> npath = path;
      //npath.push_back(make_pair(a, n));
      path.push_back(make_pair(a, n));
      if (get_path_helper(n, b, visited, path)) {
        //path = npath;
        return true;
      }
      path.pop_back();
    }
    return false;
  }

  void ugraph_transitive_closure_find_reachable_nodes(T_N const &a, set<T_N> &visited, ugraph_t const &ret) const
  {
    visited.insert(a);
    for (const auto &n : m_adj.at(a)) {
      if (!set_belongs(visited, n)) {
        if (ret.m_adj.count(n)) {
          set_union(visited, ret.m_adj.at(n));
        } else {
          ugraph_transitive_closure_find_reachable_nodes(n, visited, ret);
        }
      }
    }
  }
  //static std::string
  //ssm_elem_to_string(ssm_elem_t const &e)
  //{
  //  std::stringstream ss;
  //  ss << "(" << e.first.to_string() << ", " << e.second.first << "." << e.second.second.reg_identifier_to_string() << ")";
  //  return ss.str();
  //}


public:
  ugraph_t()
  {
  }
  ugraph_t(set<T_N> const &nodes, set<pair<T_N, T_N>> const &edges)
  {
    for (const auto &n : nodes) {
      m_adj.insert(make_pair(n, set<T_N>()));
    }
    for (const auto &e : edges) {
      insert_edge(e.first, e.second);
    }
  }
  void insert_edge(T_N const &a, T_N const &b)
  {
    insert_edge_one_direction(a, b);
    insert_edge_one_direction(b, a);
  }
  void delete_edge(T_N const &a, T_N const &b)
  {
    delete_edge_one_direction(a, b);
    delete_edge_one_direction(b, a);
  }

  bool edge_belongs(T_N const &a, T_N const &b) const
  {
    return m_adj.count(a) && set_belongs(m_adj.at(a), b);
  }

  //get_path returns any one path between nodes a and b; returns false if none exists
  bool get_path(T_N const &a, T_N const &b, list<pair<T_N, T_N>> &path) const
  {
    set<T_N> visited;
    path.clear();
    //cout << __func__ << " " << __LINE__ << ": get path called on a = " << ssm_elem_to_string(a) << "--> b = " << ssm_elem_to_string(b) << endl;
    return get_path_helper(a, b, visited, path);
  }

  ugraph_t ugraph_transitive_closure() const
  {
    ugraph_t<T_N> ret;
    for (const auto &m : m_adj) {
      set<T_N> visited;
      ugraph_transitive_closure_find_reachable_nodes(m.first, visited, ret);
      ret.m_adj.insert(make_pair(m.first, visited));
    }
    return ret;
  }

  static int find_min_color_not_used_by_vertices(set<T_N> const &vertices, map<T_N, int> const &color_assignment)
  {
    set<int> colors_used;
    for (const auto &v : vertices) {
      //std::cout << __func__ << " " << __LINE__ << ": color_assignment.count(v) = " << color_assignment.count(v) << "\n";
      if (color_assignment.count(v)) {
        //std::cout << __func__ << " " << __LINE__ << ": color_assignment.at(v) = " << color_assignment.at(v) << "\n";
        colors_used.insert(color_assignment.at(v));
      }
    }
    int max_color_seen_so_far = -1;
    for (auto c : colors_used) {
      //std::cout << __func__ << " " << __LINE__ << ": c = " << c << ", max_color_seen_so_far = " << max_color_seen_so_far << "\n";
      ASSERT(c >= max_color_seen_so_far + 1);
      if (c > max_color_seen_so_far + 1) {
        return max_color_seen_so_far + 1;
      } else {
        ASSERT(c == max_color_seen_so_far + 1);
        max_color_seen_so_far = c;
      }
    }
    //std::cout << __func__ << " " << __LINE__ << ":\n";
    return max_color_seen_so_far + 1;
  }

  list<T_N> get_nodes_list() const
  {
    list<T_N> ret;
    for (auto a : m_adj) {
      ret.push_back(a.first);
    }
    return ret;
  }

  template<typename COLORTYPE_T>
  class nodes_sort_on_constraints_size_and_degree_fn_t {
  private:
    map<T_N, set<COLORTYPE_T>> const &m_constraints;
    map<T_N, set<T_N>> const &m_adj;
  public:
    nodes_sort_on_constraints_size_and_degree_fn_t() = delete;
    nodes_sort_on_constraints_size_and_degree_fn_t(map<T_N, set<COLORTYPE_T>> const &constraints, map<T_N, set<T_N>> const &adj) : m_constraints(constraints), m_adj(adj)
    {
    }
    bool operator()(T_N const &a, T_N const &b)
    {
      if (m_constraints.count(a) && !m_constraints.count(b)) {
        return true;
      }
      if (!m_constraints.count(a) && m_constraints.count(b)) {
        return false;
      }
      if (   m_constraints.count(a) && m_constraints.count(b)
          && m_constraints.at(a).size() != m_constraints.at(b).size()) {
        return m_constraints.at(a).size() < m_constraints.at(b).size(); //sort in ascending order of constraints-size
      }
      ASSERT(   (!m_constraints.count(a) && !m_constraints.count(b))
             || (m_constraints.at(a).size() == m_constraints.at(b).size()));
      return m_adj.at(a).size() > m_adj.at(b).size(); //then sort in descending order of degree
    }
  };

  template<typename COLORTYPE_T>
  static set<COLORTYPE_T>
  get_neighbor_colors_for_node(T_N const &n, map<T_N, set<T_N>> const &m_adj, map<T_N, COLORTYPE_T> const &color_map)
  {
    set<COLORTYPE_T> ret;
    for (const auto &neighbor : m_adj.at(n)) {
      if (!color_map.count(neighbor)) {
        continue;
      }
      ret.insert(color_map.at(neighbor));
    }
    return ret;
  }

  template<typename COLORTYPE_T>
  static set<COLORTYPE_T>
  get_color_choices_for_node(T_N const &n, map<T_N, set<COLORTYPE_T>> const &constraints, long max_num_colors)
  {
    if (constraints.count(n)) {
      return constraints.at(n);
    }
    if (max_num_colors == -1) {
      return set<COLORTYPE_T>();
    }
    set<COLORTYPE_T> ret;
    for (long i = 0; i < max_num_colors; i++) {
      ret.insert(COLORTYPE_T::get_default_object(i));
    }
    return ret;
  }

  template<typename COLORTYPE_T>
  static bool
  choose_color(set<COLORTYPE_T> const &choose_from_colors, set<COLORTYPE_T> const &neighbor_colors, COLORTYPE_T &ret_color, vector<T_N> &uncolorable_set, map<T_N, COLORTYPE_T> const &color_assignment_so_far, T_N const &node_to_color, map<T_N, set<T_N>> const &adj)
  {
    if (choose_from_colors.size() == 0) {
      long color = 0;
      while (set_belongs(neighbor_colors, COLORTYPE_T::get_default_object(color))) {
        color++;
      }
      ret_color = COLORTYPE_T::get_default_object(color);
      return true;
    } else {
      for (const auto &c : choose_from_colors) {
        if (!set_belongs(neighbor_colors, c)) {
          ret_color = c;
          return true;
        }
      }
      uncolorable_set.clear();
      //map<COLORTYPE_T, size_t> interfering_colors_with_frequency;
      //for (const auto &neighbor : adj.at(node_to_color)) {
      //  if (color_assignment_so_far.count(neighbor)) {
      //    COLORTYPE_T const &interfering_color = color_assignment_so_far.at(neighbor);
      //    size_t frequency;
      //    if (interfering_colors_with_frequency.count(interfering_color)) {
      //      frequency = interfering_colors_with_frequency.at(interfering_color);
      //      interfering_colors_with_frequency.erase(interfering_color);
      //    } else {
      //      frequency = 0;
      //    }
      //    interfering_colors_with_frequency.insert(make_pair(interfering_color, frequency + 1));
      //  }
      //}
      //ASSERT(interfering_colors_with_frequency.size());
      //size_t min_freq = interfering_colors_with_frequency.begin()->second;
      //COLORTYPE_T min_freq_color = interfering_colors_with_frequency.begin()->first;
      //for (const auto &icf : interfering_colors_with_frequency) {
      //  if (icf.second < min_freq) {
      //    min_freq_color = icf.first;
      //    min_freq = icf.second;
      //  }
      //}
      uncolorable_set.push_back(node_to_color);
      for (const auto &neighbor : adj.at(node_to_color)) {
        if (   color_assignment_so_far.count(neighbor)
            /*&& color_assignment_so_far.at(neighbor) == min_freq_color*/) {
          uncolorable_set.push_back(neighbor);
        }
      }
      CPP_DBG_EXEC(STACK_SLOT_CONSTRAINTS,
        cout << __func__ << " " << __LINE__ << ": Printing adjacency matrix\n";
        for (const auto &n : adj) {
          cout << n.first.alloc_elem_to_string() << ":";
          for (const auto &a : n.second) {
            cout << " " << a.alloc_elem_to_string();
          }
          cout << endl;
        }

        cout << "\n" << ": Colour assignment so far:\n";
        for (const auto &n : color_assignment_so_far) {
          cout << n.first.alloc_elem_to_string() << ":";
          cout << " " << n.second << endl;
        }

        cout << "\n" << ": Could not colour node: " << node_to_color.alloc_elem_to_string() << endl;
        cout << "Color choices:";
        for (const auto &c : choose_from_colors) {
          cout << " " << c;
        }
        cout << endl;
        cout << "Neighbor colors:\n";
        for (const auto &neighbor : adj.at(node_to_color)) {
          if (color_assignment_so_far.count(neighbor)) {
            cout << neighbor.alloc_elem_to_string() << ": " << color_assignment_so_far.at(neighbor) << endl;
          }
        }
        //cout << "Interfering colors with frequency:\n";
	//for (const auto &pp : interfering_colors_with_frequency) {
        //   cout << "color " << pp.first << ": frequency " << pp.second << "\n";
	//}
	//cout << "min_freq = " << min_freq << "\nmin_freq_color: " << min_freq_color << "\n";
      );
      if (std::is_same<COLORTYPE_T, stack_offset_t>::value) {
        NOT_REACHED();
      }
      return false;

      //NOT_REACHED(); //if we reach here, that means we were unable to color the graph with the desired number of colors through our algorithm
    }
  }

  template<typename COLORTYPE_T>
  static ssize_t
  compute_headroom(T_N const &n, map<T_N, set<T_N>> const &adj, map<T_N, set<COLORTYPE_T>> const &constraints, long max_num_colors)
  {
    ssize_t node_degree = adj.at(n).size();
    ssize_t constraints_size = constraints.count(n) ? constraints.at(n).size() : (max_num_colors == -1) ? NUM_COLORS_INFINITY : max_num_colors;
    return constraints_size - node_degree;
  }

  static map<T_N, set<T_N>>
  adjacency_list_remove_node(map<T_N, set<T_N>> const &adj, T_N const &node_to_remove)
  {
    map<T_N, set<T_N>> ret = adj;
    ret.erase(node_to_remove);
    for (auto &a : ret) {
      a.second.erase(node_to_remove);
    }
    return ret;
  }

  template<typename COLORTYPE_T>
  static bool
  assign_color_for_node(T_N const &n, map<T_N, set<T_N>> const &adj, map<T_N, set<COLORTYPE_T>> const &constraints, map<T_N, COLORTYPE_T> &color_map, vector<T_N> &uncolorable_set, long max_num_colors)
  {
    set<COLORTYPE_T> neighbor_colors = get_neighbor_colors_for_node(n, adj, color_map);
    set<COLORTYPE_T> choose_from_colors = get_color_choices_for_node(n, constraints, max_num_colors);
    COLORTYPE_T color = COLORTYPE_T::get_invalid_object();
    if (!choose_color(choose_from_colors, neighbor_colors, color, uncolorable_set, color_map, n, adj)) {
      return false;
    }
    CPP_DBG_EXEC(GRAPH_COLORING, cout << __func__ << " " << __LINE__ << ": setting color of " << n.alloc_elem_to_string() << " to " << color << "\n");
    color_map.insert(make_pair(n, color));
    return true;
  }

  template<typename COLORTYPE_T>
  static bool
  ugraph_find_min_color_assignment_rec_for_adj(map<T_N, set<T_N>> const &adj, map<T_N, set<COLORTYPE_T>> const &constraints, map<T_N, COLORTYPE_T> &color_map, vector<T_N> &uncolorable_set, long max_num_colors)
  {
    if (adj.size() == 0) {
      return true;
    }
    //headroom = (size of constraints for the node) - (degree of node)
    T_N const *max_headroom_node = &adj.begin()->first;
    ssize_t max_headroom = compute_headroom(adj.begin()->first, adj, constraints, max_num_colors);
    for (const auto &a : adj) {
      ssize_t headroom = compute_headroom(a.first, adj, constraints, max_num_colors);
      if (headroom > max_headroom) {
        max_headroom = headroom;
        max_headroom_node = &a.first;
      }
    }

    map<T_N, set<T_N>> smaller_adj = adjacency_list_remove_node(adj, *max_headroom_node);
    if (!ugraph_find_min_color_assignment_rec_for_adj(smaller_adj, constraints, color_map, uncolorable_set, max_num_colors)) {
      return false;
    }
    return assign_color_for_node(*max_headroom_node, adj, constraints, color_map, uncolorable_set, max_num_colors);
  }

  template<typename COLORTYPE_T>
  //map<T_N, COLORTYPE_T>
  bool
  ugraph_find_min_color_assignment(map<T_N, set<COLORTYPE_T>> const &constraints, map<T_N, COLORTYPE_T> &color_map, vector<T_N> &uncolorable_set, long max_num_colors/* = -1*/)
  {
    //0. sort nodes based on the size of their constraints
    //greedy algorithm taken from https://www.geeksforgeeks.org/graph-coloring-set-2-greedy-algorithm/
    //1. Color first vertex with first color.
    //2. Do following for remaining V-1 vertices.
    //….. a) Consider the currently picked vertex and color it with the
    //lowest numbered color that has not been used on any previously
    //colored vertices adjacent to it. If all previously used colors
    //appear on vertices adjacent to v, assign a new color to it.
    CPP_DBG_EXEC(GRAPH_COLORING,
      cout << __func__ << " " << __LINE__ << ": Printing adjacency matrix\n";
      for (const auto &n : m_adj) {
        cout << n.first.alloc_elem_to_string() << ":";
        for (const auto &a : n.second) {
          cout << " " << a.alloc_elem_to_string();
        }
        cout << endl;
      }
    );
    CPP_DBG_EXEC(GRAPH_COLORING,
      cout << __func__ << " " << __LINE__ << ": Printing constraints\n";
      for (const auto &n : constraints) {
        cout << n.first.alloc_elem_to_string() << ":";
        for (const auto &c : n.second) {
          cout << " " << c;
        }
        cout << endl;
      }
    );
    //color_map.clear();
    //map<T_N, COLORTYPE_T> ret;
    //if (m_adj.size() == 0) {
    //  return true;
    //}
    //std::cout << __func__ << " " << __LINE__ << ": entry\n";
    color_map.clear();
    auto cur_adj = m_adj;
    return ugraph_find_min_color_assignment_rec_for_adj(cur_adj, constraints, color_map, uncolorable_set, max_num_colors);
    //list<T_N> nodes = this->get_nodes_list();
    //nodes_sort_on_constraints_size_and_degree_fn_t<COLORTYPE_T> sort_fn(constraints, m_adj);
    //nodes.sort(sort_fn);
    //
    ////int max_color_used = 0;
    //for (const auto &n : nodes) {
    //  //std::cout << __func__ << " " << __LINE__ << ":\n";
    //  set<COLORTYPE_T> neighbor_colors = get_neighbor_colors_for_node(n, m_adj, color_map);
    //  set<COLORTYPE_T> choose_from_colors = get_color_choices_for_node(n, constraints, max_num_colors);
    //  COLORTYPE_T color = COLORTYPE_T::get_invalid_object();
    //  if (!choose_color(choose_from_colors, neighbor_colors, color, uncolorable_set, color_map, n)) {
    //    return false;
    //  }
    //  CPP_DBG_EXEC(GRAPH_COLORING, cout << __func__ << " " << __LINE__ << ": setting color of " << ssm_elem_to_string(n) << " to " << color << "\n");
    //  color_map.insert(make_pair(n, color));
    //  //max_color_used = max(max_color_used, color);
    //}
    ////if (max_num_colors != -1) {
    ////  if (max_color_used >= max_num_colors) {
    ////    cout << __func__ << " " << __LINE__ << ": max_num_colors = " << max_num_colors << ", i = " << i << endl;
    ////  }
    ////  ASSERT(max_color_used < max_num_colors);
    ////}
    //return true;
  }
};
