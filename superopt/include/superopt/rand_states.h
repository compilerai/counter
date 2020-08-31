#ifndef RAND_STATES_H
#define RAND_STATES_H
#include "support/consts.h"
#include "exec/state.h"
#include "equiv/equiv.h"

class transmap_t;
class fixed_reg_mapping_t;

class rand_states_t
{
private:
  state_t states[NUM_STATES];
  size_t num_states;
  //int imm_map_index[NUM_IMM_MAPS];

  //void compute_imm_map_index_map();
  void rand_states_free() { states_free(states, num_states); }
public:
  rand_states_t() {}
  ~rand_states_t();
  rand_states_t(state_t const &st)
  {
    states[0] = st;
    num_states = 1;
  }
  rand_states_t rand_states_copy() const
  {
    rand_states_t ret;
    for (size_t i = 0; i < num_states; i++) {
      state_init(&ret.states[i]);
      state_copy(&ret.states[i], &states[i]);
    }
    ret.num_states = num_states;
    //for (size_t i = 0; i < NUM_IMM_MAPS; i++) {
    //  ret.imm_map_index[i] = this->imm_map_index[i];
    //}
    return ret;
  }
  uint32_t gen_rand_reg_value(int state_num, int regnum, int special_reg1, int special_reg2);
  bool init_rand_states_using_src_iseq(src_insn_t const *src_iseq, long src_iseq_len, transmap_t const &in_tmap, transmap_t const *out_tmaps, regset_t const *live_out, size_t num_out_tmaps);
  //void update_rand_states_using_src_iseq(src_insn_t const *src_iseq, long src_iseq_len, transmap_t const &in_tmap, transmap_t const *out_tmaps, regset_t const *live_out, size_t num_out_tmaps, size_t min_thresh);
  state_t const &rand_states_get_state0() const { return states[0]; }
  state_t const &rand_states_get_state(int i) const { if (i >= num_states) { cout << "i = " << i << ", num_states = " << num_states << endl; } ASSERT(i >= 0 && i < num_states); return states[i]; }
  state_t &rand_states_get_state_ref(int i) { return states[i]; }
  void set_num_states(size_t in_num_states) { num_states = in_num_states; }
  size_t get_num_states() const { return num_states; }
  string rand_states_to_string() const;
  //void shuffle_states();
  void reorder_states_using_indices(size_t const *indices);
  static rand_states_t *read_rand_states(istream &iss);
  rand_states_t rand_states_rename_using_transmap(transmap_t const &tmap) const;
  rand_states_t rand_states_rename_using_transmap_inverse(transmap_t const &tmap) const;
  void make_dst_stack_regnum_point_to_stack_top(fixed_reg_mapping_t const &dst_fixed_reg_mapping);
};

#endif
