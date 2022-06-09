#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <iostream>
#include <inttypes.h>
#include <map>
#include <set>
#include <iostream>
#include "ppc/insn.h"
#include "i386/insn.h"
#include "x64/insn.h"
#include "etfg/etfg_insn.h"
#include "i386/regs.h"
#include "x64/regs.h"
#include "insn/src-insn.h"
#include "insn/dst-insn.h"
#include "valtag/regcons.h"
#include "valtag/transmap.h"
#include "i386/cpu_consts.h"
#include "valtag/regmap.h"
#include "superopt/src_env.h"
#include "insn/live_ranges.h"
#include "rewrite/static_translate.h"
#include "valtag/regcount.h"
#include "support/debug.h"
#include "support/c_utils.h"
#include "rewrite/bbl.h"
#include "support/timers.h"
#include "rewrite/peephole.h"

//#define I386_NUM_XMM_REGS 8

static char as1[409600]/*, as2[40960]*/;
//int use_xmm_regs = 0;

static long
loc_translation_cost(int loc1, transmap_loc_modifier_t modifier1, int loc2, transmap_loc_modifier_t modifier2)
{
  int group1, group2;

  if (loc1 == loc2) {
    return 0;
  }
  if (loc1 == TMAP_LOC_SYMBOL) {
    if (   loc2 >= TMAP_LOC_EXREG(0)
        && loc2 < TMAP_LOC_EXREG(DST_NUM_EXREG_GROUPS)) {
      group2 = loc2 - TMAP_LOC_EXREG(0);
      long ret = DST_MEM_TO_EXREG_COST(group2);
      //cout << __func__ << " " << __LINE__ << ": ret = " << ret << endl;
      return ret;
    } else NOT_REACHED();
  }
  /*if (loc1 == TMAP_LOC_REG) {
    if (loc2 == TMAP_LOC_MEM) {
      return DST_REG_TO_MEM_COST;
    } else if (   loc2 >= TMAP_LOC_EXREG(0)
               && loc2 < TMAP_LOC_EXREG(DST_NUM_EXREG_GROUPS)) {
      group2 = loc2 - TMAP_LOC_EXREG(0);
      return DST_REG_TO_EXREG_COST(group2);
    } else NOT_REACHED();
  }*/
  if (   loc1 >= TMAP_LOC_EXREG(0)
      && loc1 < TMAP_LOC_EXREG(DST_NUM_EXREG_GROUPS)) {
    group1 = loc1 - TMAP_LOC_EXREG(0);
    if (loc2 == TMAP_LOC_SYMBOL) {
      long ret = DST_EXREG_TO_MEM_COST(group1);
      //cout << __func__ << " " << __LINE__ << ": ret = " << ret << endl;
      return ret;
    /*} else if (loc2 == TMAP_LOC_REG) {
      return DST_EXREG_TO_REG_COST(group1);*/
    } else if (   loc2 >= TMAP_LOC_EXREG(0)
               && loc2 < TMAP_LOC_EXREG(DST_NUM_EXREG_GROUPS)) {
      group2 = loc2 - TMAP_LOC_EXREG(0);
      long modifier_cost = 0;
      if (   group1 == group2
          && modifier1 == TMAP_LOC_MODIFIER_SRC_SIZED
          && modifier2 == TMAP_LOC_MODIFIER_DST_SIZED) {
        modifier_cost = DST_EXREG_MODIFIER_TRANSLATION_COST(group1);
      }
      long ret = DST_EXREG_TO_EXREG_COST(group1, group2);
      //cout << __func__ << " " << __LINE__ << ": ret = " << ret << endl;
      return ret;
    } else NOT_REACHED();
  }
  NOT_REACHED();
}

class tmap_loc_move_loc1_t {
private:
  transmap_loc_t m_mloc;
  bool m_loc_is_scratch;
  size_t m_loc_size_src;
  bool m_loc_src;
public:
  tmap_loc_move_loc1_t() : m_mloc(transmap_loc_symbol_t(TMAP_LOC_SYMBOL, INT_MAX)), m_loc_is_scratch(false), m_loc_size_src(0), m_loc_src(false) {}
  tmap_loc_move_loc1_t(transmap_loc_t const &l, bool is_scratch, size_t size_src, bool is_src) : m_mloc(l), m_loc_is_scratch(is_scratch), m_loc_size_src(size_src), m_loc_src(is_src) { ASSERT((size_src % BYTE_LEN) == 0); }
  transmap_loc_t const &get_mloc() const { return m_mloc; }
  bool get_loc_is_scratch() const { return m_loc_is_scratch; }
  size_t get_loc_size_src() const { return m_loc_size_src; }
  bool get_loc_src() const { return m_loc_src; }

  void set_mloc(transmap_loc_t const &l) { m_mloc = l; }
  void set_loc_is_scratch(bool b) { m_loc_is_scratch = b; }
  void set_loc_size_src(size_t s) { m_loc_size_src = s; }
  void set_loc_src(bool b) { m_loc_src = b; }

  string tmap_loc_move_loc1_to_string() const
  {
    stringstream ss;
    ss << m_mloc.transmap_loc_to_string() << " [scratch " << m_loc_is_scratch << ", is_src " << m_loc_src << ", size_src " << m_loc_size_src <<"]";
    return ss.str();
  }
};


class tmap_loc_move_t {
private:
  vector<tmap_loc_move_loc1_t> loc1s;
  transmap_loc_t loc2;
  bool loc2_is_scratch;
  bool use_xchg;
public:
  vector<tmap_loc_move_t *> next;
  vector<tmap_loc_move_t *> prev;

  tmap_loc_move_t() = delete;
  tmap_loc_move_t(transmap_loc_t const &l1, transmap_loc_t const &l2, size_t l1_size_src, bool l1_src, bool l1_is_scratch, bool l2_is_scratch, bool u_xchg) : loc2(l2)/*, loc1_size_src(l1_size_src), loc1_src(l1_src), loc1_is_scratch(l1_is_scratch)*/, loc2_is_scratch(l2_is_scratch), use_xchg(u_xchg)
  {
    ASSERT((l1_size_src % BYTE_LEN) == 0);
    tmap_loc_move_loc1_t loc1(l1, l1_is_scratch, l1_size_src, l1_src);
    loc1s.push_back(loc1);
  }
  tmap_loc_move_t(vector<tmap_loc_move_loc1_t> const &l1s, transmap_loc_t const &l2) : loc1s(l1s), loc2(l2), loc2_is_scratch(false), use_xchg(false)
  {
    for (const auto &l1 : l1s) {
      ASSERT(l1.get_loc_size_src() % BYTE_LEN == 0);
    }
  }

  void set_loc1(transmap_loc_t const &l1, bool l1_is_scratch)
  {
    ASSERT(loc1s.size() == 1);
    loc1s.at(0).set_mloc(l1);
    loc1s.at(0).set_loc_is_scratch(l1_is_scratch);
  }
  bool has_single_loc1() const
  {
    ASSERT(loc1s.size() >= 1);
    return loc1s.size() == 1;
  }
  transmap_loc_t const &get_loc1() const
  {
    ASSERT(loc1s.size() == 1);
    return loc1s.at(0).get_mloc();
  }
  bool get_loc1_is_scratch() const
  {
    ASSERT(loc1s.size() == 1);
    return loc1s.at(0).get_loc_is_scratch();
  }

  void set_loc1_src(bool l1_src, size_t l1_size_src)
  {
    ASSERT(loc1s.size() == 1);
    ASSERT(l1_size_src % BYTE_LEN == 0);
    loc1s.at(0).set_loc_src(l1_src);
    loc1s.at(0).set_loc_size_src(l1_size_src);
  }
  bool get_loc1_src() const
  {
    ASSERT(loc1s.size() == 1);
    return loc1s.at(0).get_loc_src();
  }
  size_t get_loc1_size_src() const
  {
    ASSERT(loc1s.size() == 1);
    return loc1s.at(0).get_loc_size_src();
  }

  void set_loc2(transmap_loc_t const &l2, bool l2_is_scratch) { loc2 = l2; loc2_is_scratch = l2_is_scratch; }
  transmap_loc_t const &get_loc2() const { return loc2; }
  bool get_loc2_is_scratch() const { return loc2_is_scratch; }

  void set_use_xchg(bool b) { use_xchg = b; }
  bool get_use_xchg() const { return use_xchg; }

  //int loc1, loc2;
  //int reg1, reg2;
  static int const MEMSTOP_NO_CYCLE = 1;
  static int const CYCLE = 2;
  static int const ANY = 3;

  string move_to_string() const
  {
    stringstream ss;
    ss << "loc " << this->loc1s.at(0).tmap_loc_move_loc1_to_string() << "   " << (use_xchg ? "<->" : "-->") << "   loc " << this->loc2.transmap_loc_to_string();
    return ss.str();
  }
  bool move_overlaps(tmap_loc_move_t const &other) const
  {
    for (const auto &oloc1 : other.loc1s) {
      if (this->get_loc2().overlaps(oloc1.get_mloc()) && oloc1.get_loc_src()) {
        return true;
      }
    }
    return false;
  }
};

static long
transmap_entry_translation_cost(transmap_entry_t const *tmap1,
    transmap_entry_t const *tmap2, size_t src_size,
    vector<tmap_loc_move_t> *moves)
{
  long total_cost, loc_cost[tmap2->get_num_locs()], loc12_cost;
  vector<tmap_loc_move_t> loc_move; //[tmap2->get_num_locs()];
  bool locs_done[tmap2->get_num_locs()];
  int l1, l2, min_cost_loc, i;

  DYN_DEBUG3(tmap_trans, cout << __func__ << " " << __LINE__ << ": tmap1 = " << tmap1->transmap_entry_to_string("") << endl);
  DYN_DEBUG3(tmap_trans, cout << __func__ << " " << __LINE__ << ": tmap2 = " << tmap2->transmap_entry_to_string("") << endl);

  /*if (num_moves) {
    *num_moves = 0;
  }*/

  if (!tmap1->get_num_locs()) {
    return 0;
  }
  if (!tmap2->get_num_locs()) {
    return 0;
  }

  for (i = 0; i < tmap2->get_num_locs(); i++) {
    locs_done[i] = false;
    transmap_loc_symbol_t dummy_loc(TMAP_LOC_SYMBOL, INT_MAX);
    loc_move.push_back(tmap_loc_move_t(dummy_loc, dummy_loc, DWORD_LEN, false, false, false, false));
  }

  total_cost = 0;
  while (!array_all_equal(locs_done, tmap2->get_num_locs(), true)) {
    for (l2 = 0; l2 < tmap2->get_num_locs(); l2++) {
      loc_cost[l2] = COST_INFINITY;
      if (locs_done[l2]) {
        continue;
      }
      for (l1 = 0; l1 < tmap1->get_num_locs(); l1++) {
        loc12_cost = loc_translation_cost(tmap1->get_loc(l1).get_loc(), tmap1->get_loc(l1).get_modifier(), tmap2->get_loc(l2).get_loc(), tmap2->get_loc(l2).get_modifier());
        //cout << __func__ << " " << __LINE__ << ": loc12_cost = " << loc12_cost << "\n";
        if (loc12_cost < loc_cost[l2]) {
          loc_cost[l2] = loc12_cost;
          //loc_move[l2].loc1 = tmap1->get_loc(l1).get_loc();
          //loc_move[l2].reg1 = tmap1->get_loc(l1).get_regnum();
          loc_move[l2].set_loc1(tmap1->get_loc(l1), false);
          //loc_move[l2].loc2 = tmap2->get_loc(l2).get_loc();
          //loc_move[l2].reg2 = tmap2->get_loc(l2).get_regnum();
          loc_move[l2].set_loc2(tmap2->get_loc(l2), false);
          loc_move[l2].set_loc1_src(true, src_size);
          //loc_move[l2].loc1_src = true;
          //loc_move[l2].loc1_is_scratch = false;
          //loc_move[l2].loc2_is_scratch = false;
        }
      }
      for (l1 = 0; l1 < tmap2->get_num_locs(); l1++) {
        if (!locs_done[l1]) {
          continue;
        }
        ASSERT(l1 != l2);
        loc12_cost = loc_translation_cost(tmap2->get_loc(l1).get_loc(), tmap2->get_loc(l1).get_modifier(), tmap2->get_loc(l2).get_loc(), tmap2->get_loc(l2).get_modifier());
        if (loc12_cost < loc_cost[l2]) {
          loc_cost[l2] = loc12_cost;
          //loc_move[l2].loc1 = tmap2->get_loc(l1).get_loc();
          //loc_move[l2].reg1 = tmap2->get_loc(l1).get_regnum();
          loc_move[l2].set_loc1(tmap2->get_loc(l1), false);
          //loc_move[l2].loc2 = tmap2->get_loc(l2).get_loc();
          //loc_move[l2].reg2 = tmap2->get_loc(l2).get_regnum();
          loc_move[l2].set_loc2(tmap2->get_loc(l2), false);
          loc_move[l2].set_loc1_src(false, src_size);
          //loc_move[l2].loc1_src = false;
          //loc_move[l2].loc1_is_scratch = false;
          //loc_move[l2].loc2_is_scratch = false;
        }
      }
      //ASSERT(loc_cost[l2] != COST_INFINITY);
      if (loc_cost[l2] == COST_INFINITY) {
        //cout << __func__ << " " << __LINE__ << ": returning COST_INFINITY\n";
        return COST_INFINITY;
      }
    }
    min_cost_loc = array_min(loc_cost, tmap2->get_num_locs());
    ASSERT(min_cost_loc >= 0 && min_cost_loc < tmap2->get_num_locs());
    ASSERT(loc_cost[min_cost_loc] != COST_INFINITY);
    ASSERT(!locs_done[min_cost_loc]);
    locs_done[min_cost_loc] = true;
    total_cost += loc_cost[min_cost_loc];
    if (moves) {
      //ASSERT(num_moves);
      /*if (   loc_move[min_cost_loc].loc1 != loc_move[min_cost_loc].loc2
          || loc_move[min_cost_loc].reg1 != loc_move[min_cost_loc].reg2) */{
        //memcpy(&moves[*num_moves], &loc_move[min_cost_loc], sizeof(tmap_loc_move_t));
        moves->push_back(loc_move[min_cost_loc]);
        DYN_DEBUG3(tmap_trans, cout << __func__ << " " << __LINE__ << ": adding move: loc1 = " << loc_move[min_cost_loc].get_loc1().transmap_loc_to_string() << ", loc2 = " << loc_move[min_cost_loc].get_loc2().transmap_loc_to_string() << /*", reg1 = " << loc_move[min_cost_loc].reg1 << ", reg2 = " << loc_move[min_cost_loc].reg2 << */", loc1_size_src = " << loc_move[min_cost_loc].get_loc1_size_src() << ", loc1_src = " << loc_move[min_cost_loc].get_loc1_src() << "\n");
        //(*num_moves)++;
      }
    }
  }

  //cout << __func__ << " " << __LINE__ << ": returning " << total_cost << "\n";
  return total_cost;
}

long
transmap_translation_cost(transmap_t const *tmap1, transmap_t const *tmap2/*,
    regset_t const *live_in*/, bool is_loop_edge_with_permuted_phi_assignment)
{
  long cost = 0;

  for (const auto &g : tmap1->extable/*live_in->excregs*/) {
    for (const auto &r : g.second) {
      exreg_group_id_t i = g.first;
      reg_identifier_t const &j = r.first;
      //if (r.second == 0) {
      //  continue;
      //}
      long ecost = transmap_entry_translation_cost(transmap_get_elem(tmap1, i, j),
          transmap_get_elem(tmap2, i, j), DWORD_LEN, NULL);
      if (ecost == COST_INFINITY) {
        return COST_INFINITY;
      }
      cost += ecost;
    }
  }
  int num_dst_reserved_gprs = regset_count_exregs(&dst_reserved_regs, DST_EXREG_GROUP_GPRS);
  if (is_loop_edge_with_permuted_phi_assignment) {
    if (   tmap1->transmap_count_dst_regs_for_group(DST_EXREG_GROUP_GPRS) + num_dst_reserved_gprs == DST_NUM_GPRS
        && tmap2->transmap_count_dst_regs_for_group(DST_EXREG_GROUP_GPRS) + num_dst_reserved_gprs == DST_NUM_GPRS) {
      cost += DST_AVG_REG_MOVE_COST_FOR_LOOP_EDGE_AND_ZERO_DEAD_REGS; //account for the possibility that we may need to move among registers, and in some cases may need to swap registers resulting in extra memory accesses (to store temporary register). This cost should be less than 2*REG_TO_MEM_COST, so that the DP formulation tries to try and keep one register vacant here, but not at the cost of spilling a register within the loop; spilling a register outside the loop should be acceptable. e.g., DST_AVG_REG_MOVE_COST_FOR_LOOP_EDGE_AND_ZERO_DEAD_REGS = REG_TO_MEM_COST
    }
  }
  return cost;
/*
#define MAX_TMAP_TRANS_INSNS 128
  dst_insn_t *buf = new dst_insn_t[MAX_TMAP_TRANS_INSNS];
  ASSERT(buf);
  long n_insns = transmap_translation_insns(tmap1, tmap2, NULL, buf, MAX_TMAP_TRANS_INSNS, -1, fixed_reg_mapping_t(), I386_AS_MODE_32);
  long cost = dst_iseq_compute_cost(buf, n_insns, dst_costfns[0]);
  delete [] buf;
  return cost;
*/
}

#define REG_MOVE 0
#define REG_XCHG 1
#define REG_TO_MEM 2
#define MEM_TO_REG 3
typedef struct moves_t
{
  int type:2;  /* move/xchg/swapout/swapin */
  int src;
  int dst;
} moves_t;

/*static int
gen_moves(int num_regs, int *dep_graph, char *is_dead_reg,
    moves_t *moves, int max_num_moves)
{
  //XXX: need to handle exregs
  int num_moves = 0;

  int i;

  // handle dependency-chains
  int some_change = 1;
  while (some_change) {
    some_change = 0;

    int i;
    for (i = 0; i < num_regs; i++) {
      if (   dep_graph[i] != -1
          && dep_graph[dep_graph[i]] == -1) {
        DBG(PEEP_TRANS, "generating register mov from %d-->%d\n", i,
            dep_graph[i]);
      
        ASSERT((num_moves+1) <= max_num_moves);
        moves[num_moves].type = REG_MOVE;
        moves[num_moves].src = i;
        moves[num_moves].dst = dep_graph[i];
        num_moves++;

        if (is_dead_reg[dep_graph[i]]) {
          is_dead_reg[dep_graph[i]] = 0;
          is_dead_reg[i] = 1;
        }
        dep_graph[i] = -1;
        some_change = 1;
      }
    }
  }

  // calculate num_temporaries
  int num_temporaries = 0;
  char temporaries[I386_NUM_GPRS];
  for (i = 0; i < I386_NUM_GPRS; i++) {
    ASSERT (i < num_regs);
    if (is_dead_reg[i]) temporaries[num_temporaries++] = i;
  }

  // handle dependency loops
  for (i = 0; i < num_regs; i++) {
    if (dep_graph[i] != -1 && i < I386_NUM_GPRS && dep_graph[i] < I386_NUM_GPRS) {
      assert (dep_graph[dep_graph[i]] != -1);
      if (!num_temporaries) {
        int j = dep_graph[i];
        if (i != j) {
          ASSERT((num_moves+1) <= max_num_moves);
          moves[num_moves].type = REG_XCHG;
          DBG (PEEP_TRANS, "generating register xchg %d<->%d\n", i,j);
          moves[num_moves].src = i;
          moves[num_moves].dst = j;
          num_moves++;

          dep_graph[i] = dep_graph[j];
          dep_graph[j] = -1;
        } else {
          dep_graph[i] = -1;
        }

        j = i;
        while (dep_graph[j] != -1 && dep_graph[j]<I386_NUM_GPRS) {
          int k = dep_graph[j];
          if (j != k)
          {
            assert ((num_moves+1) <= max_num_moves);
            DBG (PEEP_TRANS, "generating register xchg %d<->%d\n", j,k);
            moves[num_moves].type = REG_XCHG;
            moves[num_moves].src = j;
            moves[num_moves].dst = k;
            num_moves++;

            dep_graph[j] = dep_graph[k];
            dep_graph[k] = -1;
          } else {
            dep_graph[j] = -1;
          }
        }
      } else {
        int j;
        j = temporaries[0];
        ASSERT(j < num_regs);
        DBG (PEEP_TRANS, "generating register mov %d-->%d\n", dep_graph[i], j);

        ASSERT((num_moves+1) <= max_num_moves);
        moves[num_moves].type = REG_MOVE;
        moves[num_moves].src = dep_graph[i];
        moves[num_moves].dst = j;
        num_moves++;

        DBG (PEEP_TRANS, "generating register mov %d-->%d\n", i, dep_graph[i]);

        ASSERT((num_moves+1) <= max_num_moves);
        moves[num_moves].type = REG_MOVE;
        moves[num_moves].src = i;
        moves[num_moves].dst = dep_graph[i];
        num_moves++;

        dep_graph[j] = dep_graph[dep_graph[i]];
        dep_graph[dep_graph[i]] = -1;
        dep_graph[i] = -1;

        int chain[num_regs];
        int chain_len=0;
        memset(chain, 0xff, sizeof chain);
        while (j != -1)
        {
          DBG (PEEP_TRANS, "chain[%d] = %d\n", chain_len, j);
          chain[chain_len++] = j;
          j = dep_graph[j];
        }
        ASSERT(chain_len >= 2);
        for (j = chain_len - 2; j >= 0; j--) {
          ASSERT(dep_graph[chain[j]] == chain[j+1]);
          ASSERT(dep_graph[chain[j+1]] == -1);

          ASSERT((num_moves+1) <= max_num_moves);
          moves[num_moves].type = REG_MOVE;
          moves[num_moves].src = chain[j];
          moves[num_moves].dst = chain[j+1];
          num_moves++;

          dep_graph[chain[j]] = -1;
        }
      }
    }
  }
  return num_moves;
}*/

static long
gen_swap_out_insns(transmap_entry_t const *tmap1, transmap_entry_t const *tmap2,
    char *is_dead_reg, char is_dead_exreg[][I386_NUM_EXREGS(0)],
    long moffset, i386_insn_t *insns, i386_as_mode_t mode)
{
  NOT_IMPLEMENTED();
/*
  i386_insn_t *insn_ptr = insns;
  int i;

  if (   tmap1->loc == TMAP_LOC_EXREG(I386_EXREG_GROUP_EFLAGS_UNSIGNED)
      && tmap2->loc == TMAP_LOC_MEM) {
    if (tmap1->reg.dirty) {
      int off;
      off = tmap2->reg.rf.moffset;
      ASSERT(off == moffset);
      NOT_IMPLEMENTED();
      //insn_ptr += i386_gen_flags_unsigned_to_mem_insns(SRC_ENV_ADDR + off, insn_ptr,
      //  mode);
    }
  }

  if (   (tmap1->loc == TMAP_LOC_REG || tmap1->loc == TMAP_LOC_RF)
      && tmap2->loc == TMAP_LOC_MEM) {
    if (tmap1->reg.dirty) {
      int off;
      off = tmap2->reg.rf.moffset;
      ASSERT(SRC_ENV_ADDR + off == moffset);
      ASSERT(tmap1->reg.rf.regnum >= 0);
      insn_ptr += i386_gen_swap_out_insns(SRC_ENV_ADDR + off,
          tmap1->reg.rf.regnum, insn_ptr);
    }
    is_dead_reg[tmap1->reg.rf.regnum] = 1;
  }

  // dirty reg --> clean reg transition 
  if (   tmap1->loc != TMAP_LOC_MEM && tmap1->reg.dirty
      && tmap2->loc != TMAP_LOC_MEM && !tmap2->reg.dirty) {
    if (tmap1->loc == TMAP_LOC_REG || tmap1->loc == TMAP_LOC_RF) {
      ASSERT(tmap1->reg.rf.regnum >= 0);
      insn_ptr += i386_gen_swap_out_insns(moffset,
          tmap1->reg.rf.regnum, insn_ptr);
    } else if (tmap1->loc == TMAP_LOC_EXREG(I386_EXREG_GROUP_EFLAGS_UNSIGNED)) {
      //int sign_ov = tmap1->reg.rf.sign_ov;
      NOT_IMPLEMENTED();
      //insn_ptr += i386_gen_flags_unsigned_to_mem_insns(moffset, insn_ptr, mode);
    } else if (tmap1->loc == TMAP_LOC_EXREG(I386_EXREG_GROUP_EFLAGS_SIGNED)) {
      NOT_IMPLEMENTED();
      //insn_ptr += i386_gen_flags_signed_to_mem_insns(moffset, insn_ptr, mode);
    }
  }
  return insn_ptr - insns;
*/
}

static long
i386_gen_flag_to_reg_transfer_insns(transmap_entry_t const *tmap1,
    transmap_entry_t const *tmap2, char *is_dead_reg,
    char is_dead_exreg[][I386_NUM_EXREGS(0)], long moffset,
    i386_insn_t *insns, i386_as_mode_t mode)
{
  NOT_IMPLEMENTED();
/*
  i386_insn_t *insn_ptr = insns;
  bool dead_reg_found = false;
  //int tmp_tmap1_flags_src_reg = -1;
  //int tmp_tmap1_flags_store = -1;

  if (   tmap1->loc == TMAP_LOC_EXREG(I386_EXREG_GROUP_EFLAGS_UNSIGNED)
      && (tmap2->loc == TMAP_LOC_REG || tmap2->loc == TMAP_LOC_RF)
      && !is_dead_reg[tmap2->reg.rf.regnum]) {
#if 0
    if (is_dead_reg[tmap2->reg.rf.regnum]) {
      gen_flags_to_reg_code (tmap1->reg.rf.sign_ov & 0x1,
          tmap1->reg.rf.sign_ov >> 1,
          tmap2->reg.rf.regnum, ptr);
      is_dead_reg[tmap2->reg.rf.regnum] = 0;
      ASSERT (ptr <= end);
      if (tmap1->reg.rf.sign_ov != -1 && tmap2->loc == TMAP_LOC_RF) {
        // to neutralize the sign of the flags 
        gen_reg_to_rf_code(tmap2->reg.rf.regnum, ptr);
        ASSERT (ptr <= end);
      }
    } else {
#endif
      // this can only happen if there is a loop of the form:
      // flags->regnum and regnum->flags 
      int j;
      for (j = 0; j < I386_NUM_GPRS; j++) {
        if (is_dead_reg[j]) {
          insn_ptr += i386_gen_flags_to_reg_insns(tmap1->reg.rf.sign_ov & 0x1,
              tmap1->reg.rf.sign_ov >> 1, j, insn_ptr, mode);
          //tmp_tmap1_flags_src_reg = i;
          //tmp_tmap1_flags_store = j;
          dead_reg_found = true;
          insn_ptr += i386_gen_register_move_insns(j,
              tmap2->reg.rf.regnum, insn_ptr);
          break;
        }
      }

      if (!dead_reg_found) {
        insn_ptr += i386_gen_flags_to_mem_insns(tmap1->reg.rf.sign_ov & 0x1,
            tmap1->reg.rf.sign_ov >> 1, moffset, insn_ptr, mode);
        //tmp_tmap1_flags_src_reg = i;
        //tmp_tmap1_flags_store = -1;

        insn_ptr += i386_gen_swap_in_insns(moffset,
            tmap2->reg.rf.regnum, insn_ptr);
      }
      is_dead_reg[tmap2->reg.rf.regnum] = 0;
    //}

    if (tmap1->loc == TMAP_LOC_RF && tmap2->loc == TMAP_LOC_REG) {
      //do nothing
    }
  }
  return insn_ptr - insns;
*/
}

static long
i386_gen_reg_to_flag_transfer_insns(transmap_entry_t const *tmap1,
    transmap_entry_t const *tmap2, i386_insn_t *insns, i386_as_mode_t mode)
{
  NOT_IMPLEMENTED();
/*
  i386_insn_t *insn_ptr = insns;

  if (   tmap1->loc == TMAP_LOC_REG
      && tmap2->loc == TMAP_LOC_EXREG(I386_EXREG_GROUP_EFLAGS_UNSIGNED)) {
    insn_ptr += i386_gen_reg_to_flags_insns(tmap1->reg.rf.regnum, insn_ptr, mode);
  }
  return insn_ptr - insns;
*/
}

static long
i386_gen_reg_to_rf_transfer_insns(transmap_entry_t const *tmap1,
    transmap_entry_t const *tmap2, i386_insn_t *insns)
{
  NOT_IMPLEMENTED();
/*
  i386_insn_t *insn_ptr = insns;

  if (tmap1->loc == TMAP_LOC_REG && tmap2->loc == TMAP_LOC_RF) {
    // notice that we transfer tmap2->table[i].reg (not tmap1) because
    // reg transfer must already have taken place
    insn_ptr += i386_gen_reg_to_rf_insns(tmap2->reg.rf.regnum, insn_ptr);
  }
  return insn_ptr - insns;
*/
}

static long
i386_gen_mem_to_reg_flag_transfer_insns(transmap_entry_t const *tmap1,
    transmap_entry_t const *tmap2, long moffset, i386_insn_t *insns, i386_as_mode_t mode)
{
  NOT_IMPLEMENTED();
/*
  i386_insn_t *insn_ptr = insns;

  if (   tmap1->loc == TMAP_LOC_MEM
      && (tmap2->loc == TMAP_LOC_REG || tmap2->loc == TMAP_LOC_RF)) {
    int off;
    off = tmap1->reg.rf.moffset;
    DBG2(EQUIV, "Generating swap-in from %d to reg %d\n", off,
        tmap2->reg.rf.regnum);
    insn_ptr += i386_gen_swap_in_insns(moffset, tmap2->reg.rf.regnum, insn_ptr);
  }

  if (   tmap1->loc == TMAP_LOC_MEM
      && tmap2->loc == TMAP_LOC_EXREG(I386_EXREG_GROUP_EFLAGS_UNSIGNED)) {
    NOT_IMPLEMENTED();
    //insn_ptr += i386_gen_mem_to_flags_unsigned_insns(moffset, insn_ptr, mode);
  }

  if (tmap2->loc == TMAP_LOC_RF) {
    insn_ptr += i386_gen_reg_to_rf_insns(tmap2->reg.rf.regnum, insn_ptr);
  }

  return insn_ptr - insns;
*/
}



static int
tmap_loc_pick_from_regset_not_present(regset_t const *regs, int loc)
{
  int i, group;

  /*if (loc == TMAP_LOC_REG) {
    for (i = 0; i < DST_NUM_REGS; i++) {
      if (!regset_belongs(regs, i)) {
        return i;
      }
    }
    return -1;
  } else */if (   loc >= TMAP_LOC_EXREG(0)
             && loc < TMAP_LOC_EXREG(DST_NUM_EXREG_GROUPS)) {
    group = loc - TMAP_LOC_EXREG(0);
    for (i = 0; i < DST_NUM_EXREGS(group); i++) {
      if (!regset_belongs_ex(regs, group, i)) {
        return i;
      }
    }
    return -1;
  } else NOT_REACHED();
}

static bool
is_src_env_addr(src_ulong addr)
{
  return    addr >= SRC_ENV_ADDR && addr < SRC_ENV_ADDR + SRC_ENV_SIZE
         || addr >= TYPE_ENV_INITIAL_ADDR && addr < TYPE_ENV_INITIAL_ADDR + TYPE_ENV_INITIAL_SIZE
         || addr >= TYPE_ENV_FINAL_ADDR && addr < TYPE_ENV_FINAL_ADDR + TYPE_ENV_FINAL_SIZE
         //|| addr >= TYPE_ENV_VALUE_ADDR && addr < TYPE_ENV_VALUE_ADDR + TYPE_ENV_VALUE_SIZE
         || addr >= TYPE_ENV_SCRATCH_ADDR && addr < TYPE_ENV_SCRATCH_ADDR + TYPE_ENV_SCRATCH_SIZE
  ;
}

static long
transmap_translation_move_gen_insns(dst_insn_t *buf, size_t size,
    tmap_loc_move_t const *cur_move, int const memloc_offset_reg,
    int const dead_dst_reg,
    fixed_reg_mapping_t const &dst_fixed_reg_mapping,
    i386_as_mode_t mode)
{
  dst_insn_t *ptr, *end;
  int group1, group2;
  long xchg_ret;
  //bool use_xchg;
/*#if SRC == ARCH_ETFG
  static int const memloc_offset_reg = R_ESP;
#else
  static int const memloc_offset_reg = -1;
#endif*/
  transmap_loc_t loc1 = cur_move->get_loc1();
  transmap_loc_t loc2 = cur_move->get_loc2();
  if (memloc_offset_reg == -1) {
    loc1.scale_and_add_src_env_addr_to_memlocs();
    loc2.scale_and_add_src_env_addr_to_memlocs();
  }

  ptr = buf;
  end = buf + size;

  if (loc2.get_loc() == TMAP_LOC_EXREG(I386_EXREG_GROUP_SEGS)) {
    return 0;
  }
  if (loc1.equals(loc2)) {
    return 0;
  }
  DYN_DEBUG2(tmap_trans, cout << __func__ << " " << __LINE__ << ": loc1 = " << loc1.transmap_loc_to_string() << ", loc2 = " << loc2.transmap_loc_to_string() << endl);
  if (   loc1.get_loc() == TMAP_LOC_SYMBOL
      && loc2.get_loc() == TMAP_LOC_SYMBOL) {
    if (loc1.get_regnum() != loc2.get_regnum()) {
      int loc1_offset_reg = (cur_move->get_loc1_is_scratch() || is_src_env_addr(loc1.get_regnum())) ? -1 : memloc_offset_reg;
      int loc2_offset_reg = (cur_move->get_loc2_is_scratch() || is_src_env_addr(loc2.get_regnum())) ? -1 : memloc_offset_reg;
      if (mode == I386_AS_MODE_32) {
        ptr += dst_insn_put_move_mem_to_mem(loc1.get_regnum(), loc2.get_regnum(), loc1_offset_reg, loc2_offset_reg, dead_dst_reg, dst_fixed_reg_mapping, ptr, end - ptr, mode);
      } else {
        //in 64-bit mode, we are usually looking to sandbox the generated iseq, so do not use stack in this case
        ptr += dst_insn_put_xchg_reg_with_mem(R_EAX, loc1.get_regnum(), loc1_offset_reg, ptr, end - ptr);
        ptr += dst_insn_put_move_exreg_to_mem(loc2.get_regnum(), loc2_offset_reg, DST_EXREG_GROUP_GPRS, R_EAX, dst_fixed_reg_mapping, ptr, end - ptr, mode);
        ptr += dst_insn_put_xchg_reg_with_mem(R_EAX, loc1.get_regnum(), loc1_offset_reg, ptr, end - ptr);
      }
    }
  } else if (   loc1.get_loc() >= TMAP_LOC_EXREG(0)
             && loc1.get_loc() < TMAP_LOC_EXREG(DST_NUM_EXREG_GROUPS)
             && loc2.get_loc() == TMAP_LOC_SYMBOL) {
    group1 = loc1.get_loc() - TMAP_LOC_EXREG(0);
    int loc2_offset_reg = (cur_move->get_loc2_is_scratch() || is_src_env_addr(loc2.get_regnum())) ? -1 : memloc_offset_reg;
    reg_identifier_t const &loc1_reg_id = loc1.get_reg_id();
    ASSERT(!loc1_reg_id.reg_identifier_is_unassigned());
    if (loc1.get_modifier() == TMAP_LOC_MODIFIER_SRC_SIZED) {
      ptr += dst_insn_put_zextend_exreg(group1, loc1.get_reg_id().reg_identifier_get_id(), cur_move->get_loc1_size_src(), ptr, end - ptr);
    }
    ptr += dst_insn_put_move_exreg_to_mem(loc2.get_regnum(), loc2_offset_reg, group1, loc1_reg_id.reg_identifier_get_id(),
        dst_fixed_reg_mapping, ptr, end - ptr, mode);
  } else if (   loc1.get_loc() == TMAP_LOC_SYMBOL
             && loc2.get_loc() >= TMAP_LOC_EXREG(0)
             && loc2.get_loc() < TMAP_LOC_EXREG(DST_NUM_EXREG_GROUPS)) {
    int loc1_offset_reg = (cur_move->get_loc1_is_scratch() || is_src_env_addr(loc1.get_regnum())) ? -1 : memloc_offset_reg;
    //ASSERT(!use_xchg);
    group2 = loc2.get_loc() - TMAP_LOC_EXREG(0);
    reg_identifier_t const &loc2_reg = loc2.get_reg_id();
    //cout << __func__ << " " << __LINE__ << ": loc2_reg = " << loc2_reg.reg_identifier_to_string() << endl;
    ptr += dst_insn_put_move_mem_to_exreg(group2, loc2_reg.reg_identifier_get_id(), loc1.get_regnum(),
        loc1_offset_reg, dst_fixed_reg_mapping, ptr, end - ptr, mode);
    //tmap_loc_mark_regset(live_regs, loc2, cur_move->reg2);
  /*} else if (   loc1 == TMAP_LOC_REG
             && loc2 >= TMAP_LOC_EXREG(0)
             && cur_move->loc2 < TMAP_LOC_EXREG(DST_NUM_EXREG_GROUPS)) {
    ASSERT(!use_xchg);
    group2 = loc2 - TMAP_LOC_EXREG(0);
    ptr += dst_insn_put_move_reg_to_exreg(group2, cur_move->reg2, cur_move->reg1,
        ptr, end - ptr, mode);
    tmap_loc_unmark_regset(live_regs, loc1, cur_move->reg1);
    tmap_loc_mark_regset(live_regs, loc2, cur_move->reg2);*/
  } else if (   loc1.get_loc() >= TMAP_LOC_EXREG(0)
             && loc1.get_loc() < TMAP_LOC_EXREG(DST_NUM_EXREG_GROUPS)
             && loc2.get_loc() >= TMAP_LOC_EXREG(0)
             && loc2.get_loc() < TMAP_LOC_EXREG(DST_NUM_EXREG_GROUPS)) {
    group1 = loc1.get_loc() - TMAP_LOC_EXREG(0);
    group2 = loc2.get_loc() - TMAP_LOC_EXREG(0);
    if (cur_move->get_use_xchg()) {
      ASSERT(group1 == group2);
      if (   loc1.get_modifier() == TMAP_LOC_MODIFIER_SRC_SIZED
          && loc2.get_modifier() == TMAP_LOC_MODIFIER_DST_SIZED
          && cur_move->get_loc1_size_src() != DWORD_LEN) {
        return -1;//NOT_IMPLEMENTED();
      }
      if (   loc2.get_modifier() == TMAP_LOC_MODIFIER_SRC_SIZED
          && loc1.get_modifier() == TMAP_LOC_MODIFIER_DST_SIZED) {
        //do nothing. XXX: not sure about this
      }

      xchg_ret = dst_insn_put_xchg_exreg_with_exreg(group1, loc1.get_reg_id().reg_identifier_get_id(),
              group2, loc2.get_reg_id().reg_identifier_get_id(), ptr, end - ptr, mode);
      ASSERT(xchg_ret);
      ptr += xchg_ret;
    } else {
      if (!(loc1.get_reg_id().reg_identifier_get_id() >= 0 && loc1.get_reg_id().reg_identifier_get_id() < DST_NUM_EXREGS(group1))) {
        cout << __func__ << " " << __LINE__ << ": loc1 = " << loc1.transmap_loc_to_string() << endl;
        cout << __func__ << " " << __LINE__ << ": loc2 = " << loc2.transmap_loc_to_string() << endl;
      }
      ASSERT(loc1.get_reg_id().reg_identifier_get_id() >= 0 && loc1.get_reg_id().reg_identifier_get_id() < DST_NUM_EXREGS(group1));
      //cout << __func__ << " " << __LINE__ << ": loc1 = " << loc1.transmap_loc_to_string() << endl;
      //cout << __func__ << " " << __LINE__ << ": loc2 = " << loc2.transmap_loc_to_string() << endl;
      ASSERT(loc2.get_reg_id().reg_identifier_get_id() >= 0 && loc2.get_reg_id().reg_identifier_get_id() < DST_NUM_EXREGS(group2));
      if (   loc1.get_modifier() == TMAP_LOC_MODIFIER_SRC_SIZED
          && loc2.get_modifier() == TMAP_LOC_MODIFIER_DST_SIZED) {
        group1 = loc1.get_loc() - TMAP_LOC_EXREG(0);
        size_t src_size = cur_move->get_loc1_size_src();
        DYN_DEBUG2(tmap_trans, cout << __func__ << " " << __LINE__ << ": calling dst_insn_put_zextend_exreg; src_size = " << src_size << endl);
        ptr += dst_insn_put_zextend_exreg(group1, loc1.get_reg_id().reg_identifier_get_id(), src_size, ptr, end - ptr);
      }
      ptr += dst_insn_put_move_exreg_to_exreg(group2, loc2.get_reg_id().reg_identifier_get_id(), group1,
          loc1.get_reg_id().reg_identifier_get_id(), dst_fixed_reg_mapping, ptr, end - ptr, mode);
      //tmap_loc_unmark_regset(live_regs, loc1, cur_move->reg1);
      //tmap_loc_mark_regset(live_regs, loc2, cur_move->reg2);
    }
  } else NOT_REACHED();
  ASSERT(ptr < end);
  return ptr - buf;
}

static string
moves_to_string(vector<tmap_loc_move_t> const &moves)
{
  //char *ptr, *end;
  stringstream ss;
  long i;

  //ptr = buf;
  //end = buf + size;
  //ptr += snprintf(ptr, end - ptr, "moves:\n");
  ss << "moves:\n";
  for (i = 0; i < moves.size(); i++) {
    ss << i << ". " << moves[i].move_to_string() << "\n"; //"%ld.  %s\n", i, move_to_string(&moves[i], tbuf1, sizeof tbuf1));
    //ptr += snprintf(ptr, end - ptr, "%ld.  %s\n", i, move_to_string(&moves[i], tbuf1, sizeof tbuf1));
    //ASSERT(ptr < end);
  }
  return ss.str();
}



void
collect_prev_moves_in_order_helper(tmap_loc_move_t *cur, vector<tmap_loc_move_t *> &ret, bool &found_cycle, set<tmap_loc_move_t *> &seen_path, set<tmap_loc_move_t *> &seen_all)
{
  //cout << __func__ << " " << __LINE__ << ": cur = " << move_to_string(cur, as1, sizeof as1) << endl;
  seen_path.insert(cur);
  seen_all.insert(cur);
  for (auto p : cur->prev) {
    //cout << __func__ << " " << __LINE__ << ": p = " << move_to_string(p, as1, sizeof as1) << endl;
    if (set_belongs(seen_path, p)) {
      ASSERT(set_belongs(seen_all, p));
      found_cycle = true;
    }
    if (set_belongs(seen_all, p)) {
      continue;
    }
    set<tmap_loc_move_t *> seen_path_copy = seen_path;
    collect_prev_moves_in_order_helper(p, ret, found_cycle, seen_path_copy, seen_all);
  }
  ret.push_back(cur);
}

vector<tmap_loc_move_t *>
collect_prev_moves_in_order(tmap_loc_move_t *cur, bool &found_cycle)
{
  vector<tmap_loc_move_t *> ret;
  set<tmap_loc_move_t *> seen_path, seen_all;
  found_cycle = false;
  collect_prev_moves_in_order_helper(cur, ret, found_cycle, seen_path, seen_all);
  //cout << __func__ << " " << __LINE__ << ": found_cycle = " << found_cycle << endl;
  return ret;
}

#if 0
static long
transmap_translation_move_gen_insns_for_start_stop_fn(dst_insn_t *buf, size_t buf_size,
    tmap_loc_move_t *moves, long num_moves, i386_as_mode_t mode, regset_t *live_regs,
    int which_moves_to_consider/*, bool is_cycle*/)
{
  tmap_loc_move_t *cur, *prev, *start, *stop;
  dst_insn_t *ptr, *end;
  bool is_cycle;
  long i;

  ptr = buf;
  end = buf + buf_size;

  for (i = 0; i < num_moves; i++) {
    if (moves[i].done) {
      continue;
    }
    if (   which_moves_to_consider == tmap_loc_move_t::MEMSTOP_NO_CYCLE
        && moves[i].loc2 != TMAP_LOC_SYMBOL) {
      continue;
    }
    bool found_cycle;
    cur = &moves[i];
    vector<tmap_loc_move_t *> ordered_moves = collect_prev_moves_in_order(cur, found_cycle);

    if (   found_cycle
        && which_moves_to_consider == tmap_loc_move_t::MEMSTOP_NO_CYCLE) {
      continue;
    }
    if (   !found_cycle
        && which_moves_to_consider == tmap_loc_move_t::CYCLE) {
      continue;
    }

    //ASSERT(prev);
    //start = prev;
    //cur = start;
    /* then iterate forwards to find the stop. */
    /*do {
      prev = cur;
      cur = cur->next;
    } while (cur && cur != start);
    ASSERT(prev);
    stop = prev;*/

    /*if (stop->next == NULL) {
      is_cycle = false;
    } else {
      is_cycle = true;
    }

    if (   start_stop_fn
        && !((*start_stop_fn)(start->loc1, start->reg1, stop->loc2, stop->reg2,
                start->loc1_src, is_cycle))) {
      continue;
    }*/

    for (auto i = ordered_moves.begin(); i != ordered_moves.end(); i++) {
      tmap_loc_move_t *cur = *i;
      if (cur->done) {
        continue;
      }
      ptr += transmap_translation_move_gen_insns(ptr, end - ptr, cur,
          live_regs, moves, num_moves, mode);
      cur->done = true;
    }
    //ASSERT(cur == stop);
    //ASSERT(!cur->done);
    ASSERT(ptr < end);
    /*ptr += transmap_translation_move_gen_insns(ptr, end - ptr, cur,
        live_regs, moves, num_moves, mode);
    ASSERT(!cur->done);
    cur->done = true;*/
  }
  return ptr - buf;
}

static bool
all_moves_done(tmap_loc_move_t const *moves, long num_moves)
{
  long i;
  for (i = 0; i < num_moves; i++) {
    if (!moves[i].done) {
      return false;
    }
  }
  return true;
}
#endif

static bool
tmap_loc_move_depends(tmap_loc_move_t const *a, tmap_loc_move_t const *b, size_t &dep_path_len)
{
  set<tmap_loc_move_t const *> seen;
  if (a->next.size() > 1) {
    cout << __func__ << " " << __LINE__ << ": a->next.size() > 1. a = " << a->move_to_string() << endl;
  }
  ASSERT(a->next.size() <= 1);
  dep_path_len = 1;
  while (a->next.size() != 0) {
    ASSERT(a->next.size() == 1);
    a = a->next.at(0);
    if (a == b) {
      return true;
    }
    if (set_belongs(seen, a)) {
      break;
    }
    seen.insert(a);
    dep_path_len++;
  }
  return false;
}

static void
move_ptrs_vec_replace_loc1(vector<tmap_loc_move_t *> const &move_ptrs_vec, size_t cur_index, transmap_loc_t const &loc1/*, int reg1*/, transmap_loc_t const &new_loc1/*, int new_reg1*/, bool new_loc1_is_scratch)
{
  for (size_t i = cur_index; i < move_ptrs_vec.size(); i++) {
    if (move_ptrs_vec[i]->get_loc1().overlaps(loc1)) {
        //&& move_ptrs_vec[i]->reg1 == reg1)
      move_ptrs_vec[i]->set_loc1(new_loc1, new_loc1_is_scratch);
      //move_ptrs_vec[i]->reg1 = new_reg1;
      //move_ptrs_vec[i]->loc1_is_scratch = new_loc1_is_scratch;
    }
  }
}

static exreg_id_t
identify_dead_dst_reg(vector<tmap_loc_move_t *> const &move_ptrs_vec, size_t cur_index)
{
  regset_t dst_live_regs;
  regset_clear(&dst_live_regs);
  regset_copy(&dst_live_regs, &dst_reserved_regs);
  for (size_t i = 0; i < cur_index; i++) {
    if (   move_ptrs_vec.at(i)->get_loc2().get_loc() >= TMAP_LOC_EXREG(0)
        && move_ptrs_vec.at(i)->get_loc2().get_loc() < TMAP_LOC_EXREG(DST_NUM_EXREG_GROUPS)) {
      exreg_group_id_t dst_reg_group = move_ptrs_vec.at(i)->get_loc2().get_loc() - TMAP_LOC_EXREG(0);
      regset_mark_ex_mask(&dst_live_regs, dst_reg_group, move_ptrs_vec.at(i)->get_loc2().get_reg_id(), MAKE_MASK(DST_EXREG_LEN(dst_reg_group)));
    }
  }
  for (size_t i = cur_index; i < move_ptrs_vec.size(); i++) {
    if (   move_ptrs_vec.at(i)->get_loc1().get_loc() >= TMAP_LOC_EXREG(0)
        && move_ptrs_vec.at(i)->get_loc1().get_loc() < TMAP_LOC_EXREG(DST_NUM_EXREG_GROUPS)) {
      exreg_group_id_t dst_reg_group = move_ptrs_vec.at(i)->get_loc1().get_loc() - TMAP_LOC_EXREG(0);
      regset_mark_ex_mask(&dst_live_regs, dst_reg_group, move_ptrs_vec.at(i)->get_loc1().get_reg_id(), MAKE_MASK(DST_EXREG_LEN(dst_reg_group)));
    }
  }
  for (exreg_id_t r = 0; r < DST_NUM_EXREGS(DST_EXREG_GROUP_GPRS); r++) {
    if (!regset_belongs_ex(&dst_live_regs, DST_EXREG_GROUP_GPRS, reg_identifier_t(r))) {
      return r;
    }
  }
  return -1;
}

static exreg_id_t
identify_dead_dst_reg_for_moves(vector<tmap_loc_move_t> &moves, size_t cur_index)
{
  vector<tmap_loc_move_t *> move_ptrs_vec;
  for (size_t i = 0; i < moves.size(); i++) {
    move_ptrs_vec.push_back(&moves.at(i));
  }
  return identify_dead_dst_reg(move_ptrs_vec, cur_index);
}

static void/*size_t*/
tmap_loc_moves_break_cycles(vector<pair<tmap_loc_move_t *, int>> &move_ptrs, vector<tmap_loc_move_t> const &moves/*, size_t num_moves*/, vector<tmap_loc_move_t> &extra_moves)
{
  vector<tmap_loc_move_t *> move_ptrs_vec;
  vector<pair<bool, size_t>> new_move_ptrs_vec; //bool says whether old (false) or new (true); size_t is the index in the respective vector
  DYN_DEBUG3(tmap_trans, cout << __func__ << " " << __LINE__ << ": num_moves = " << moves.size() << endl);
  for (size_t i = 0; i < moves.size(); i++) {
    move_ptrs_vec.push_back(move_ptrs[i].first);
  }
  //size_t num_extra_moves = 0;
  for (size_t i = 0; i < moves.size(); i++) {
    auto m = move_ptrs_vec.at(i);
    size_t cycle_len;
    bool retain_orig_move = true;
    if (tmap_loc_move_depends(m, m, cycle_len)) {
      DYN_DEBUG2(tmap_trans, cout << __func__ << " " << __LINE__ << ": found cycle node: " << m->move_to_string() << endl);
      exreg_id_t dead_dst_reg = identify_dead_dst_reg(move_ptrs_vec, i);
      //tmap_loc_move_t *em = &extra_moves[num_extra_moves];
      ASSERT(cycle_len >= 2);
      bool extra_move_added;
      if (dead_dst_reg == -1/* || cycle_len == 2*/) {
        if (   m->get_loc1().get_loc() == m->get_loc2().get_loc()
            && m->get_loc1().get_loc() == TMAP_LOC_EXREG(DST_EXREG_GROUP_GPRS)) {
          tmap_loc_move_t em(m->get_loc2(), m->get_loc1(), DWORD_LEN, false, false, false, true);
          //em->loc1 = m->loc2;
          //em->reg1 = m->reg2;
          //em->loc2 = m->loc1;
          //em->reg2 = m->reg1;
	  //em->loc1_is_scratch = false;
	  //em->loc2_is_scratch = false;
          extra_moves.push_back(em);
          extra_move_added = true;
          retain_orig_move = false;
          //m->use_xchg = true;
          DYN_DEBUG2(tmap_trans, cout << __func__ << " " << __LINE__ << ": changed move to use xchg: " << em.move_to_string() << endl);
        } else {
          tmap_loc_move_t em(m->get_loc2(), transmap_loc_symbol_t(TMAP_LOC_SYMBOL, SRC_ENV_SCRATCH(0)), DWORD_LEN, true, false, true, false);
          extra_moves.push_back(em);
          //em->loc1 = m->loc2;
          //em->reg1 = m->reg2;
          //em->loc2 = transmap_loc_symbol_t(TMAP_LOC_SYMBOL, SRC_ENV_SCRATCH(0));
          //em->reg2 = SRC_ENV_SCRATCH(0);
	  //em->loc1_is_scratch = false;
	  //em->loc2_is_scratch = true;
          //em->loc1_src = true;
          //em->use_xchg = false;
          extra_move_added = true;
          DYN_DEBUG2(tmap_trans, cout << __func__ << " " << __LINE__ << ": added extra move " << em.move_to_string() << endl);
        }
      } else {
        DYN_DEBUG2(tmap_trans, cout << __func__ << " " << __LINE__ << ": dead_dst_reg = " << dead_dst_reg << endl);
        tmap_loc_move_t em(m->get_loc2(), transmap_loc_exreg_t(TMAP_LOC_EXREG(DST_EXREG_GROUP_GPRS), dead_dst_reg, TMAP_LOC_MODIFIER_SRC_SIZED), DWORD_LEN, true, false, false, false);
        extra_moves.push_back(em);
        //em->loc1 = m->loc2;
        //em->reg1 = m->reg2;
        //em->loc2 = TMAP_LOC_EXREG(DST_EXREG_GROUP_GPRS);
        //em->reg2 = dead_dst_reg;
        //em->loc2 = transmap_loc_exreg_t(TMAP_LOC_EXREG(DST_EXREG_GROUP_GPRS), dead_dst_reg, TMAP_LOC_MODIFIER_DST_SIZED);
	//em->loc1_is_scratch = false;
	//em->loc2_is_scratch = false;
        //em->loc1_src = true;
        //em->use_xchg = false;
        extra_move_added = true;
        DYN_DEBUG2(tmap_trans, cout << __func__ << " " << __LINE__ << ": added extra move " << em.move_to_string() << endl);
      }
      //move_ptrs_vec_replace_loc1(move_ptrs_vec, i, em->loc1/*, em->reg1*/, em->loc2/*, em->reg2*/, em->loc2_is_scratch);
      move_ptrs_vec_replace_loc1(move_ptrs_vec, i, extra_moves.at(extra_moves.size() - 1).get_loc1()/*, em->reg1*/, extra_moves.at(extra_moves.size() - 1).get_loc2()/*, em->reg2*/, extra_moves.at(extra_moves.size() - 1).get_loc2_is_scratch());
      for (auto p : m->prev) {
        p->next.clear();
      }
      if (extra_move_added) {
        //num_extra_moves++;
        //new_move_ptrs_vec.push_back(em);
        ASSERT(extra_moves.size() > 0);
        //new_move_ptrs_vec.push_back(&extra_moves.at(extra_moves.size() - 1));
        new_move_ptrs_vec.push_back(make_pair(true, extra_moves.size() - 1));
      }
    }
    //new_move_ptrs_vec.push_back(m);
    if (retain_orig_move) {
      new_move_ptrs_vec.push_back(make_pair(false, i));
    }
  }
  DYN_DEBUG2(tmap_trans, cout << __func__ << " " << __LINE__ << ": extra_moves.size() = " << extra_moves.size() << endl);
  DYN_DEBUG2(tmap_trans, cout << __func__ << " " << __LINE__ << ": move_ptrs_vec.size() = " << move_ptrs_vec.size() << endl);
  DYN_DEBUG2(tmap_trans, cout << __func__ << " " << __LINE__ << ": new_move_ptrs_vec.size() = " << new_move_ptrs_vec.size() << endl);

  move_ptrs.clear();
  for (size_t i = 0; i < new_move_ptrs_vec.size(); i++) {
    //move_ptrs.push_back(make_pair(new_move_ptrs_vec.at(i), 0));
    auto &p = new_move_ptrs_vec.at(i);
    if (p.first) {
      move_ptrs.push_back(make_pair(&extra_moves.at(p.second), 0));
    } else {
      move_ptrs.push_back(make_pair(move_ptrs_vec.at(p.second), 0));
    }
  }
  //return new_move_ptrs_vec.size();
}

class move_attribute_t {
private:
  enum { MOVE_ATTR_FREES_A_REG = 0, MOVE_ATTR_CYCLE, MOVE_ATTR_OTHER } m_type;
  size_t m_cycle_id, m_elem_id;
public:
  move_attribute_t() : m_type(MOVE_ATTR_OTHER) {}
  string to_string() const
  {
    if (m_type == MOVE_ATTR_FREES_A_REG) {
      return "frees_a_reg";
    }
    if (m_type == MOVE_ATTR_CYCLE) {
      stringstream ss;
      ss << "cycle." << m_cycle_id << "." << m_elem_id;
      return ss.str();
    }
    if (m_type == MOVE_ATTR_OTHER) {
      return "other";
    }
    cout << __func__ << " " << __LINE__ << ": m_type = " << m_type << endl;
    NOT_REACHED();
  }
  bool belongs_to_same_cycle(move_attribute_t const &other) const
  {
    if (   m_type != MOVE_ATTR_CYCLE
        || other.m_type != MOVE_ATTR_CYCLE) {
      return false;
    }
    return m_cycle_id == other.m_cycle_id;
  }
  bool operator<(move_attribute_t const &other) const
  {
    if (m_type < other.m_type) {
      return true;
    }
    if (m_type > other.m_type) {
      return false;
    }
    if (m_type == MOVE_ATTR_CYCLE) {
      if (m_cycle_id < other.m_cycle_id) {
        return true;
      }
      if (m_cycle_id > other.m_cycle_id) {
        return false;
      }
      return m_elem_id < other.m_elem_id;
    } else {
      return false;
    }
  }
  void set_to_frees_reg()
  {
    m_type = MOVE_ATTR_FREES_A_REG;
  }
  bool frees_reg() const
  {
    return m_type == MOVE_ATTR_FREES_A_REG;
  }
  void set_to_cycle(size_t cycle_id, size_t elem_id)
  {
    m_type = MOVE_ATTR_CYCLE;
    m_cycle_id = cycle_id;
    m_elem_id = elem_id;
  }
  bool is_cycle() const
  {
    return m_type == MOVE_ATTR_CYCLE;
  }
  size_t get_cycle_id() const
  {
    ASSERT(m_type == MOVE_ATTR_CYCLE);
    return m_cycle_id;
  }
  bool is_other() const
  {
    return m_type == MOVE_ATTR_OTHER;
  }
};

static bool
move_chain_leads_to_free_reg(tmap_loc_move_t const *move)
{
  ASSERT(move->next.size() <= 1);
  set<tmap_loc_move_t const *> seen;
  while (move->next.size() == 1) {
    if (set_belongs(seen, move)) {
      return false;
    }
    seen.insert(move);
    move = move->next.at(0);
  }
  ASSERT(move->next.size() == 0);
  if (   move->has_single_loc1()
      && move->get_loc1().get_loc() == TMAP_LOC_EXREG(DST_EXREG_GROUP_GPRS)
      && move->get_loc2().get_loc() == TMAP_LOC_SYMBOL) {
    return true;
  }
  return false;
}

static bool
move_belongs_to_cycle(tmap_loc_move_t const *move)
{
  set<tmap_loc_move_t const *> seen;
  seen.insert(move);
  ASSERT(move->next.size() <= 1);
  auto cur_move = move;
  while (cur_move->next.size() == 1) {
    cur_move = cur_move->next.at(0);
    if (cur_move == move) {
      return true;
    }
    if (set_belongs(seen, cur_move)) {
      return false; //seen cycle but not with move
    }
    ASSERT(cur_move->next.size() <= 1);
    seen.insert(cur_move);
  }
  return false;
}

static size_t
move_ptrs_get_index_for_move(vector<pair<tmap_loc_move_t *, int>> const &move_ptrs/*, size_t num_moves*/, tmap_loc_move_t const *move)
{
  for (size_t i = 0; i < move_ptrs.size(); i++) {
    if (move_ptrs[i].first == move) {
      return i;
    }
  }
  NOT_REACHED();
}

static void
merge_to_flag_moves_into_one(vector<tmap_loc_move_t> &moves)
{
  DYN_DEBUG2(tmap_trans,
    cout << __func__ << " " << __LINE__ << ": before:\n";
    for (size_t i = 0; i < moves.size(); i++) {
      cout << i << ". " << moves[i].move_to_string() << "\n";
    }
  );

  vector<tmap_loc_move_t> new_moves;
  vector<tmap_loc_move_loc1_t> to_flag_moves;
  tmap_loc_move_loc1_t flag_other_loc1;
  bool flag_other_loc1_found = false;
  for (const auto &move : moves) {
    if (   move.get_loc2().get_loc() >= TMAP_LOC_EXREG(I386_EXREG_GROUP_EFLAGS_OTHER)
        && move.get_loc2().get_loc() <= TMAP_LOC_EXREG(I386_EXREG_GROUP_EFLAGS_SGE)) {
      tmap_loc_move_loc1_t loc1(move.get_loc1(), move.get_loc1_is_scratch(), move.get_loc1_size_src(), move.get_loc1_src());
      to_flag_moves.push_back(loc1);
      if (move.get_loc2().get_loc() == TMAP_LOC_EXREG(I386_EXREG_GROUP_EFLAGS_OTHER)) {
        flag_other_loc1 = loc1;
        flag_other_loc1_found = true;
      }
    } else {
      new_moves.push_back(move);
    }
  }
  if (to_flag_moves.size() <= 1) {
    return;
  }
  if (flag_other_loc1_found) {
    //transmap_loc_exreg_t flag_loc(TMAP_LOC_EXREG(I386_EXREG_GROUP_EFLAGS_OTHER), 0, TMAP_LOC_MODIFIER_SRC_SIZED);
    /*if (!flag_other_loc1_found) {
      ASSERT(to_flag_moves.size() == 1);
      flag_other_loc1 = to_flag_moves.at(0);
    }*/
    bool is_scratch = flag_other_loc1.get_loc_is_scratch();
    bool is_src = flag_other_loc1.get_loc_src();
    size_t size_src = flag_other_loc1.get_loc_size_src();
    DYN_DEBUG3(tmap_trans, cout << __func__ << " " << __LINE__ << ": size_src = " << size_src << endl);
    //tmap_loc_move_loc1_t loc1(flag_loc, is_scratch, is_src, size_src);
    transmap_loc_exreg_t loc2(TMAP_LOC_EXREG(I386_EXREG_GROUP_EFLAGS_OTHER), 0, TMAP_LOC_MODIFIER_SRC_SIZED);
    tmap_loc_move_t flag_move(flag_other_loc1.get_mloc(), loc2, size_src, is_src, is_scratch, false, false);
    new_moves.push_back(flag_move);
  } else {
    cout << __func__ << " " << __LINE__ << ": moves:\n";
    for (size_t i = 0; i < moves.size(); i++) {
      cout << i << ". " << moves[i].move_to_string() << "\n";
    }
    NOT_IMPLEMENTED();
  }
  moves = new_moves;

  DYN_DEBUG2(tmap_trans,
    cout << __func__ << " " << __LINE__ << ": after:\n";
    for (size_t i = 0; i < moves.size(); i++) {
      cout << i << ". " << moves[i].move_to_string() << "\n";
    }
  );
}

static void
order_moves_adding_extra_moves_as_needed(vector<tmap_loc_move_t> &moves/*, size_t num_moves, size_t moves_size*/)
{
  vector<pair<tmap_loc_move_t *, int>> move_ptrs;
  int num_moves_done = 0;
  for (size_t i = 0; i < moves.size(); i++) {
    move_ptrs.push_back(make_pair(&moves[i], moves[i].prev.size()));
    DYN_DEBUG3(tmap_trans, cout << __func__ << " " << __LINE__ << ": move_ptrs[" << i << "].first = " << &move_ptrs[i].first << endl);
  }
  bool done;
  int found;
  do {
    found = -1;
    for (size_t i = num_moves_done; i < moves.size(); i++) {
      if (move_ptrs[i].second == 0) {
        found = i;
        break;
      }
    }
    DYN_DEBUG3(tmap_trans, cout << __func__ << " " << __LINE__ << ": found = " << found << endl);
    if (found != -1) {
      DYN_DEBUG3(tmap_trans, cout << __func__ << " " << __LINE__ << ": next move: " << move_ptrs[found].first->move_to_string() << endl);
      auto tmp = move_ptrs[num_moves_done];
      move_ptrs[num_moves_done] = move_ptrs[found];
      move_ptrs[found] = tmp;
      num_moves_done++;
      for (size_t i = num_moves_done; i < moves.size(); i++) {
        if (vector_contains(move_ptrs[i].first->prev, move_ptrs[num_moves_done - 1].first)) {
          DYN_DEBUG3(tmap_trans, cout << __func__ << " " << __LINE__ << ": decrementing prev count for move " << i << ": " << move_ptrs[i].first->move_to_string() << ". before decrementing, prev count = " << move_ptrs[i].second << endl);
          move_ptrs[i].second--;
        }
      }
    }
  } while (found != -1);
  DYN_DEBUG3(tmap_trans, cout << __func__ << " " << __LINE__ << ": found = " << found << endl);

  //invariant: only cycles and their dependencies remain between num_moves_done..num_moves entries in move_ptrs
  //rearrange moves so that all moves with loc1=REG(r) where there exists a move from loc1=REG(r)->loc2=MEM on the left of the cycles [move_frees_a_reg==true] and all others on the right of the cycles. This will ensure minimum register pressure during code generation for cycles.
  move_attribute_t move_attribute[moves.size()];
  for (size_t i = 0; i < moves.size(); i++) {
    if (move_chain_leads_to_free_reg(move_ptrs[i].first)) {
      move_attribute[i].set_to_frees_reg();
      for (size_t j = 0; j < moves.size(); j++) {
        if (   !move_attribute[j].frees_reg()
            && move_ptrs[i].first->has_single_loc1()
            && move_ptrs[j].first->has_single_loc1()
            && move_ptrs[j].first->get_loc1().overlaps(move_ptrs[i].first->get_loc1())
            //&& move_ptrs[j].first->reg1 == move_ptrs[i].first->reg1
            && move_ptrs[j].first->get_loc1_src() == move_ptrs[i].first->get_loc1_src()) {
          move_attribute[j].set_to_frees_reg();
        }
      }
    } else if (   move_attribute[i].is_other()
               && move_belongs_to_cycle(move_ptrs[i].first)) {
      tmap_loc_move_t const *move = move_ptrs[i].first;
      size_t elem_id = 0;
      DYN_DEBUG2(tmap_trans, cout << __func__ << " " << __LINE__ << ": marking cycle for move: " << move->move_to_string() << endl);
      do {
        size_t move_index = move_ptrs_get_index_for_move(move_ptrs/*, num_moves*/, move);
        ASSERT(move_attribute[move_index].is_other());
        move_attribute[move_index].set_to_cycle(i, elem_id);
        elem_id++;
        DYN_DEBUG2(tmap_trans, cout << __func__ << " " << __LINE__ << ": set cycle " << i << " for move: " << move->move_to_string() << endl);
        ASSERT(move->next.size() <= 1);
        if (move->next.size() == 1) {
          move = move->next.at(0);
        } else {
          move = NULL;
        }
      } while (move && move != move_ptrs[i].first);
    }
  }

  DYN_DEBUG3(tmap_trans, cout << __func__ << " " << __LINE__ << ": moves.size() = " << moves.size() << endl);
  DYN_DEBUG3(tmap_trans,
    cout << "after topsort but before sort, moves:\n";
    for (size_t i = 0; i < moves.size(); i++) {
      cout << i << ". " << move_ptrs[i].first->move_to_string() << " [" << move_attribute[i].to_string() << "]\n";
    }
  );
  //bubble-sort like algorithm to 
  for (size_t i = 0; i < moves.size(); i++) {
    for (size_t j = 0; j < moves.size() - 1 - i; j++) {
      //swap(j, j+1) if possible (i.e., no transitive dependency between j and j+1) and profitable (i.e., if move_frees_a_reg[j] < move_frees_a_reg[j+1])
      //also swap move_attribute to keep it consistent with move_ptrs
      size_t dummy;
      if (   move_attribute[j + 1] < move_attribute[j]
          && (   !tmap_loc_move_depends(move_ptrs[j].first, move_ptrs[j + 1].first, dummy)
              || move_attribute[j + 1].belongs_to_same_cycle(move_attribute[j]))) {
        auto tmp = move_ptrs[j + 1];
        move_ptrs[j + 1] = move_ptrs[j];
        move_ptrs[j] = tmp;
        auto tmp2 = move_attribute[j + 1];
        move_attribute[j + 1] = move_attribute[j];
        move_attribute[j] = tmp2;
      }
    }
  }
  DYN_DEBUG3(tmap_trans,
    cout << "after sort, moves:\n";
    for (size_t i = 0; i < moves.size(); i++) {
      cout << i << ". " << move_ptrs[i].first->move_to_string() << " [" << move_attribute[i].to_string() << "]\n";
    }
  );

  //tmap_loc_move_t extra_moves[num_moves];
  vector<tmap_loc_move_t> extra_moves;
  /*size_t num_new_moves = */tmap_loc_moves_break_cycles(move_ptrs, moves/*, num_moves*/, extra_moves);
  //DYN_DEBUG3(tmap_trans, cout << __func__ << " " << __LINE__ << ": num_new_moves = " << num_new_moves << endl);
  DYN_DEBUG3(tmap_trans,
    cout << "after break cycles, moves:\n";
    for (size_t i = 0; i < moves.size(); i++) {
      cout << i << ". " << move_ptrs[i].first->move_to_string() << " [" << move_attribute[i].to_string() << "]\n";
    }
  );



  vector<tmap_loc_move_t> tmp_moves;//[num_new_moves];
  for (size_t i = 0; i < move_ptrs.size(); i++) {
    tmp_moves.push_back(*move_ptrs[i].first);
    tmp_moves[i].next.clear();
    tmp_moves[i].prev.clear();
  }
  //copy tmp_moves to moves
  moves.clear();
  for (size_t i = 0; i < move_ptrs.size(); i++) {
    moves.push_back(tmp_moves[i]);
    //clear dependency links as they are no longer needed
  }
  DYN_DEBUG3(tmap_trans, cout << __func__ << " " << __LINE__ << ": returning " << moves.size() << endl);
  //return num_new_moves;
}

long
transmap_translation_insns(transmap_t const *tmap1,
    transmap_t const *tmap2, eqspace::state const *start_state,
    dst_insn_t *buf,
    size_t buf_size, int memloc_offset_reg,
    fixed_reg_mapping_t const &dst_fixed_reg_mapping,
    i386_as_mode_t mode)
{
//#define MAX_MOVES 2048
  vector<tmap_loc_move_t> moves;// = new tmap_loc_move_t[MAX_MOVES];
  //regset_t dst_live_regs;
  dst_insn_t *ptr, *end;
  //long num_moves;
  //int i, j;

  stopwatch_run(&transmap_translation_insns_timer);
  ptr = buf;
  end = buf + buf_size;
  //regset_clear(&dst_live_regs);
  //regset_copy(&dst_live_regs, &dst_reserved_regs);


  DYN_DBG(tmap_trans, "start_state:\n%s\n", start_state ? start_state->state_to_string_for_eq().c_str() : nullptr);
  DYN_DBG(tmap_trans, "tmap1:\n%s\n", transmap_to_string(tmap1, as1, sizeof as1));
  DYN_DBG(tmap_trans, "tmap2:\n%s\n", transmap_to_string(tmap2, as1, sizeof as1));
  context* ctx = g_ctx;

  //num_moves = 0;
  for (const auto &g : tmap1->extable) {
    for (const auto &r : g.second) {
      exreg_group_id_t i = g.first;
      reg_identifier_t const &j = r.first;
      size_t src_size = DWORD_LEN;
      if (start_state) {
        string key = j.reg_identifier_to_string();
        expr_ref e;
        if (start_state->has_expr(key)) {
          e = start_state->get_expr(key/*, *start_state*/);
        } else {
          //e = start_state->state_get_expr_value(*start_state, G_SRC_KEYWORD, reg_type_exvreg, i, j.reg_identifier_get_id());
          e = ctx->get_input_expr_for_key(mk_string_ref(string(G_SRC_KEYWORD ".") + state::reg_name(i, j.reg_identifier_get_id())), ctx->mk_bv_sort(SRC_EXREG_LEN(i)));
        }
        ASSERT(e->is_bool_sort() || e->is_bv_sort());
        if (e->is_bool_sort()) {
          src_size = ROUND_UP(1, BYTE_LEN);
        } else {
          src_size = ROUND_UP(e->get_sort()->get_size(), BYTE_LEN);
        }
      }
      DYN_DEBUG2(tmap_trans, cout << i << "." << j.reg_identifier_to_string() << ": src_size " << src_size << endl);
      //long tnum_moves = 0;
      //tmap_entry_mark_regset(&dst_live_regs, transmap_get_elem(tmap1, i, j));

      //DYN_DEBUG3(tmap_trans, cout << __func__ << " " << __LINE__ << ": i = " << i << ", j = " << j.reg_identifier_to_string() << ", tmap1 entry = " << ({transmap_entry_to_string(transmap_get_elem(tmap1, i, j), "", as1, sizeof as1); as1;}) << endl);
      //DYN_DEBUG3(tmap_trans, cout << __func__ << " " << __LINE__ << ": i = " << i << ", j = " << j.reg_identifier_to_string() << ", tmap2 entry = " << ({transmap_entry_to_string(transmap_get_elem(tmap2, i, j), "", as1, sizeof as1); as1;}) << endl);
      long cost = transmap_entry_translation_cost(transmap_get_elem(tmap1, i, j), transmap_get_elem(tmap2, i, j), src_size, &moves);
      if (cost == COST_INFINITY) {
        cout << __func__ << " " << __LINE__ << ": tmap1 =\n" << transmap_to_string(tmap1, as1, sizeof as1) << endl;
        cout << __func__ << " " << __LINE__ << ": tmap2 =\n" << transmap_to_string(tmap2, as1, sizeof as1) << endl;
        cout << __func__ << " " << __LINE__ << ": i = " << i << ", j = " << j.reg_identifier_to_string() << endl;
      }
      ASSERT(cost != COST_INFINITY);
          //&moves[num_moves], &tnum_moves
      //DYN_DEBUG3(tmap_trans, cout << __func__ << " " << __LINE__ << ": tnum_moves = " << tnum_moves << endl);
      //num_moves += tnum_moves;
      //ASSERT(num_moves <= MAX_MOVES);
    }
  }

  for (long i = 0; i < moves.size(); i++) {
    moves[i].prev.clear();
    moves[i].next.clear();
    moves[i].set_use_xchg(false);
    //moves[i].done = false;
  }

  merge_to_flag_moves_into_one(moves);

  DYN_DBG(tmap_trans, "moves:\n%s\n", moves_to_string(moves).c_str());
  for (long i = 0; i < moves.size(); i++) {
    for (long j = 0; j < moves.size(); j++) {
      if (i == j) {
        continue;
      }
      if (/*   moves[i].get_loc2().overlaps(moves[j].get_loc1())
          && moves[j].get_loc1_src()*/
         moves[i].move_overlaps(moves[j])
         ) {
        //ASSERT(moves[i].loc2.get_regnum() != -1);
        moves[j].next.push_back(&moves[i]);
        moves[i].prev.push_back(&moves[j]);
        DYN_DBG(tmap_trans, "move %ld can happen only after move %ld.\n", i, j);
        ASSERT(moves[j].next.size() == 1);
        //continue;
      }
    }
  }

  /*num_moves = */order_moves_adding_extra_moves_as_needed(moves/*, num_moves, MAX_MOVES*/);
  for (size_t i = 0; i < moves.size(); i++) {
    exreg_id_t dead_dst_reg = identify_dead_dst_reg_for_moves(moves, i);
    long ret = transmap_translation_move_gen_insns(ptr, end - ptr, &moves[i], memloc_offset_reg, dead_dst_reg, dst_fixed_reg_mapping, mode);
    if (ret == -1) {
      printf("tmap1:\n%s\n", transmap_to_string(tmap1, as1, sizeof as1));
      printf("tmap2:\n%s\n", transmap_to_string(tmap2, as1, sizeof as1));
      printf("moves:\n%s\n", moves_to_string(moves).c_str());
      fflush(stdout);
      NOT_REACHED();
    }
    ptr += ret;
    ASSERT(ptr - buf < buf_size);
  }

  //ptr += transmap_translation_move_gen_insns_for_start_stop_fn(ptr, end - ptr,
  //    moves, num_moves, mode, &dst_live_regs, tmap_loc_move_t::MEMSTOP_NO_CYCLE); //to free up any registers
  //DYN_DBG2(tmap_trans, "insns:\n%s\n", dst_iseq_to_string(buf, ptr - buf, as1, sizeof as1));
  //ptr += transmap_translation_move_gen_insns_for_start_stop_fn(ptr, end - ptr,
  //    moves, num_moves, mode, &dst_live_regs, tmap_loc_move_t::CYCLE);
  //DYN_DBG2(tmap_trans, "insns:\n%s\n", dst_iseq_to_string(buf, ptr - buf, as1, sizeof as1));
  //ptr += transmap_translation_move_gen_insns_for_start_stop_fn(ptr, end - ptr,
  //    moves, num_moves, mode, &dst_live_regs, tmap_loc_move_t::ANY);
  //DYN_DBG2(tmap_trans, "insns:\n%s\n", dst_iseq_to_string(buf, ptr - buf, as1, sizeof as1));
  //ptr += transmap_translation_move_gen_insns_for_start_stop_fn(ptr, end - ptr,
  //    moves, num_moves, mode, &dst_live_regs, &start_dst);
  //DYN_DBG2(tmap_trans, "insns:\n%s\n", dst_iseq_to_string(buf, ptr - buf, as1, sizeof as1));
  //ASSERT(all_moves_done(moves, num_moves));

  DYN_DBG(tmap_trans, "tmap1:\n%s\n", transmap_to_string(tmap1, as1, sizeof as1));
  DYN_DBG(tmap_trans, "tmap2:\n%s\n", transmap_to_string(tmap2, as1, sizeof as1));
  DYN_DBG(tmap_trans, "moves:\n%s\n", moves_to_string(moves).c_str());
  DYN_DBG(tmap_trans, "insns:\n%s\n", dst_iseq_to_string(buf, ptr - buf, as1, sizeof as1));
  ASSERT(ptr - buf < buf_size);

  stopwatch_stop(&transmap_translation_insns_timer);
  //delete[] moves;
  return ptr - buf;
}

long
transmap_translation_code(struct transmap_t const *tmap1,
    struct transmap_t const *tmap2,
    int memloc_offset_reg,
    fixed_reg_mapping_t const &dst_fixed_reg_mapping,
    i386_as_mode_t mode,
    uint8_t *buf, size_t size)
{
#define MAX_TRANSMAP_TRANSLATION_INSNS 128
  char assembly[MAX_TRANSMAP_TRANSLATION_INSNS * 32];
  dst_insn_t *insns;
  long n, bn;

  stopwatch_run(&transmap_translation_code_timer);
  //insns = (dst_insn_t *)malloc(MAX_TRANSMAP_TRANSLATION_INSNS * sizeof(dst_insn_t));
  insns = new dst_insn_t[MAX_TRANSMAP_TRANSLATION_INSNS];
  ASSERT(insns);
  n = transmap_translation_insns(tmap1, tmap2, NULL,
      insns, MAX_TRANSMAP_TRANSLATION_INSNS, memloc_offset_reg,
      dst_fixed_reg_mapping, mode);
  ASSERT(n < MAX_TRANSMAP_TRANSLATION_INSNS);
  //cout << __func__ << " " << __LINE__ << ": insns =\n" << dst_iseq_to_string(insns, n, as1, sizeof as1) << endl;
  
  dst_iseq_to_string(insns, n, assembly, sizeof assembly);
  bn = dst_assemble(NULL, buf, size, assembly, mode);
  ASSERT(bn < size);
  //free(insns);
  delete [] insns;
  stopwatch_stop(&transmap_translation_code_timer);
  return bn;
}

static void
temporaries_count_fill_used_regs(regcount_t const *temps_count, regset_t *used)
{
  int i, j, num_temps;

  /*num_temps = temps_count->reg_count;
  for (i = 0; i < DST_NUM_REGS && num_temps > 0; i++) {
    if (!used->cregs[i]) {
      used->cregs[i] = -1;
      num_temps--;
    }
  }
  ASSERT(num_temps == 0);*/
  for (i = 0; i < DST_NUM_EXREG_GROUPS; i++) {
    num_temps = temps_count->exreg_count[i];
    for (j = 0; j < DST_NUM_EXREGS(i) && num_temps > 0; j++) {
      if (!used->excregs[i][j]) {
        used->excregs[i][j] = -1;
        num_temps--;
      }
    }
    ASSERT(num_temps == 0);
  }
}



//void
//transmaps_assign_regs_where_unassigned(transmap_t *in_tmap, transmap_t *out_tmaps,
//    long num_out_tmaps, regcount_t const *temps_count)
//{
//  //regset_t used;
//  int n;
//  cout << __func__ << " " << __LINE__ << ": transmaps =\n" << transmaps_to_string(in_tmap, out_tmaps, num_out_tmaps, as1, sizeof as1) << endl;
//  cout << __func__ << " " << __LINE__ << ": temps_count =\n" << regcount_to_string(temps_count, as1, sizeof as1) << endl;
//
//  regset_t used = transmaps_get_used_dst_regs(in_tmap, out_tmaps, num_out_tmaps);
//  regset_union(&used, &dst_reserved_regs);
//  temporaries_count_fill_used_regs(temps_count, &used);
//
//  /*for (i = 0; i < SRC_NUM_REGS; i++) {
//    transmap_entry_assign_reg_where_unassigned(&in_tmap->table[i], &used,
//        SRC_ENV_ADDR + SRC_ENV_OFF(i));
//    for (n = 0; n < num_out_tmaps; n++) {
//      transmap_entry_assign_reg_where_unassigned(&out_tmaps[n].table[i], &used,
//          SRC_ENV_ADDR + SRC_ENV_OFF(i));
//    }
//    transmap_entry_get_used_dst_regs(&in_tmap->table[i], &used);
//  }*/
//
//  //stack_slot_map_t ssm = transmap_assign_stack_slots_where_unassigned(in_tmap);
//
//  for (const auto &g : in_tmap->extable) {
//    for (const auto &r : g.second) {
//      exreg_group_id_t i = g.first;
//      reg_identifier_t const &j = r.first;
//      in_tmap->extable[i][j].transmap_entry_assign_reg_where_unassigned(&used);
//      for (n = 0; n < num_out_tmaps; n++) {
//        out_tmaps[n].extable[i][j].transmap_entry_assign_reg_where_unassigned(&used);
//      }
//      transmap_get_elem(in_tmap, i, j)->transmap_entry_get_used_dst_regs(&used);
//    }
//  }
//}


