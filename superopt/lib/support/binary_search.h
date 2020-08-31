#ifndef EQCHECKBINARY_SEARCH_H
#define EQCHECKBINARY_SEARCH_H

namespace eqspace {

class binary_search_t {
public:
  binary_search_t(size_t size) : m_size(size) {}
  virtual bool test(size_t begin, size_t end) const = 0;
  void do_binary_search()
  {
    if (!test(0, m_size)) {
      return;
    }
    binsearch(0, m_size);
  }
  void binsearch(size_t begin, size_t end)
  {
    if (begin >= end) {
      return;
    }
    if (end == begin + 1) {
      m_result.insert(begin);
      return;
    }
    size_t mid = (begin + end)/2;
    bool left = test(begin, mid);
    bool right = test(mid, end);
    if (left) {
      binsearch(begin, mid);
    }
    if (right) {
      binsearch(mid, end);
    }
    if (!left && !right) {
      cout << __func__ << " " << __LINE__ << ": Warning: individual halves do not satisfy the test, but combined they satisfy the test." << endl;
    }
  }
  set<size_t> get_test_positive()
  {
    return m_result;
  }
private:
  size_t m_size;
  set<size_t> m_result;
};

}

#endif
