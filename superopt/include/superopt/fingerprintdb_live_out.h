#ifndef FINGERPRINTDB_LIVE_OUT_H
#define FINGERPRINTDB_LIVE_OUT_H
#include "support/minmax.h"
#include "etfg/etfg_regs.h"
#include "i386/regs.h"
#include "x64/regs.h"
#include "support/src-defs.h"
#include "support/consts.h"
#include "support/types.h"

struct src_fingerprintdb_elem_t;
class regset_t;
class memslot_set_t;

class fingerprintdb_live_out_t
{
private:
  minmax_t m_regset_masks[MAX_NUM_EXREG_GROUPS][MAX_NUM_EXREGS(0)];
  minmax_t m_memloc_masks[MAX_NUM_MEMLOCS];
  bool m_mem_live_out;
public:
  fingerprintdb_live_out_t() : m_mem_live_out(false) {}
  static fingerprintdb_live_out_t src_fingerprintdb_compute_live_out(src_fingerprintdb_elem_t const * const *fingerprintdb);
  bool fingerprintdb_live_out_agrees_with_regset(regset_t const &regs) const;
  bool fingerprintdb_live_out_agrees_with_memslot_set(memslot_set_t const &memlocs) const;
  void fingerprintdb_live_out_update_using_fingerprintdb_elem(src_fingerprintdb_elem_t const &e);
  bool fingerprintdb_live_out_regset_bitmap_downgrade(uint64_t &bitmap, exreg_group_id_t g, exreg_id_t r) const;
  void fingerprintdb_live_out_regset_truncate(regset_t *regs) const;
  bool get_mem_live_out() const { return m_mem_live_out; }
  memslot_set_t get_live_memslots() const;
  string fingerprintdb_live_out_to_string() const;
};

#endif
