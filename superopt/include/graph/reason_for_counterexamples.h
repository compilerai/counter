#pragma once

namespace eqspace {

class reason_for_counterexamples_t {
private:
  typedef enum { reason_inductive_invariants = 0, reason_correctness_covers } reason_id_t;
  reason_id_t m_reason_id;
public:
  static reason_for_counterexamples_t inductive_invariants()
  {
    return reason_for_counterexamples_t(reason_inductive_invariants);
  }
  static reason_for_counterexamples_t correctness_covers()
  {
    return reason_for_counterexamples_t(reason_correctness_covers);
  }
  static list<reason_for_counterexamples_t> get_all_reasons()
  {
    list<reason_for_counterexamples_t> ret;
    ret.push_back(inductive_invariants());
    ret.push_back(correctness_covers());
    return ret;
  }
  bool operator<(reason_for_counterexamples_t const& other) const
  {
    return this->m_reason_id < other.m_reason_id;
  }
  string reason_for_counterexamples_to_string() const
  {
    switch (m_reason_id) {
      case reason_inductive_invariants: {
        return "inductive-invariants";
      }
      case reason_correctness_covers: {
        return "correctness-covers";
      }
      default: {
        NOT_REACHED();
      }
    }
  }
  static reason_for_counterexamples_t reason_for_counterexamples_from_string(string const& s)
  {
    if (s == "inductive-invariants") {
      return inductive_invariants();
    } else if (s == "correctness-covers") {
      return correctness_covers();
    } else NOT_REACHED();
  }
private:
  reason_for_counterexamples_t(reason_id_t reason_id) : m_reason_id(reason_id)
  { }
};

}
