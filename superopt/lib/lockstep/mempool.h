#ifndef EXECUTIONENGINE_INTERPRETER_MEMPOOL_H
#define EXECUTIONENGINE_INTERPRETER_MEMPOOL_H
//#include "llvm/ExecutionEngine/memspace.h"

using namespace std;

class mempool_snapshot_t
{
private:
  map<void *, vector<uint8_t>> m_mallocs;
  friend class mempool_t;
public:
  map<void *, vector<uint8_t>> const &get_mallocs() const
  {
    return m_mallocs;
  }
};

class mempool_t
{
private:
  //unsigned long m_begin, m_end;
  void *mem_start;
  void *mem_stop;

  set<void *> m_alloc_set;

  struct mem_control_block {
    bool is_available;
    size_t size;
  };
public:
  mempool_t() = delete;
  //mempool_t(unsigned long start, unsigned long stop) : mem_start(memspace_t::global_memspace.convert_to_ptr((void *)start)), mem_stop(memspace_t::global_memspace.convert_to_ptr((void *)stop))
  mempool_t(unsigned long start, unsigned long stop) : mem_start((void *)start), mem_stop((void *)stop)
  {
    mem_control_block *mcb = (struct mem_control_block *)mem_start;
    mcb->is_available = true;
    mcb->size = stop - start - sizeof(struct mem_control_block);
  }

  void mempool_free(void *firstbyte) {
    struct mem_control_block *mcb;
    mcb = (struct mem_control_block *)((char *)firstbyte - sizeof(struct mem_control_block));
    mcb->is_available = true;
    ASSERT(m_alloc_set.count(firstbyte));
    m_alloc_set.erase(firstbyte);

    //merge forward
    char *m = (char *)mcb;
    m += mcb->size;
    struct mem_control_block *next_mcb;
    next_mcb = (struct mem_control_block *)m;
    if (next_mcb->is_available) {
      mcb->size += next_mcb->size + sizeof(struct mem_control_block);
    }

    //TODO: merge backward
    return;
  }

  void *mempool_malloc(size_t numbytes) {
    char *cur;
    cur = (char *)mem_start;
    /* Keep going until we have searched all allocated space */
    while (cur < mem_stop) {
      struct mem_control_block *cur_mcb;
      cur_mcb = (struct mem_control_block *)cur;
      if (cur_mcb->is_available && cur_mcb->size > numbytes + sizeof(struct mem_control_block)) {
        int orig_size = cur_mcb->size;
        cur_mcb->size = numbytes;
        cur_mcb->is_available = false;
        void *next = cur + sizeof(struct mem_control_block) + numbytes;
        struct mem_control_block *next_mcb = (struct mem_control_block *)next;
        next_mcb->is_available = true;
        next_mcb->size = orig_size - numbytes - sizeof(struct mem_control_block);
        char *ret = cur + sizeof(struct mem_control_block);
        m_alloc_set.insert((void *)ret);
        //return memspace_t::global_memspace.convert_to_addr((void *)ret);
        return (void *)ret;
      } else if (cur_mcb->is_available && cur_mcb->size == numbytes) { //exact match
        cur_mcb->is_available = false;
        char *ret = cur + sizeof(struct mem_control_block);
        m_alloc_set.insert((void *)ret);
        //return memspace_t::global_memspace.convert_to_addr((void *)ret);
        return (void *)ret;
      } else {
        cur = cur + sizeof(struct mem_control_block) + cur_mcb->size;
      }
    }
    ASSERT(cur == mem_stop);
    NOT_REACHED(); //out of memory
  }
  mempool_snapshot_t
  mempool_get_snapshot() const
  {
    mempool_snapshot_t ret;
    for (auto addr : m_alloc_set) {
      struct mem_control_block *mcb = (struct mem_control_block *)((char *)addr - sizeof(struct mem_control_block));
      ASSERT(!mcb->is_available);
      size_t sz = mcb->size;
      vector<uint8_t> data;
      for (size_t i = 0; i < sz; i++) {
        data.push_back(((uint8_t *)addr)[i]);
      }
      ret.m_mallocs.insert(make_pair(addr, data));
    }
    return ret;
  }
};

#endif
