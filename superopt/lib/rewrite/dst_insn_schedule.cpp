#include "rewrite/dst_insn_schedule.h"
#include "insn/dst-insn.h"
#include <stdlib.h>
#include <stdio.h>
#include "support/src-defs.h"
#include "support/debug.h"
#include "i386/insn.h"
#include "x64/insn.h"
#include "ppc/insn.h"
#include "valtag/valtag.h"
#include "support/utils.h"

static char as1[409600];

static string
move_out_intervals_to_string(set<pair<long, long>> const &move_out_intervals)
{
  stringstream ss;
  for (const auto &mo : move_out_intervals) {
    ss << "(" << mo.first << ", " << mo.second << ")\n";
  }
  return ss.str();
}

long
dst_insns_schedule(struct dst_insn_t *dst_iseq, long dst_iseq_len, long dst_iseq_size, flow_t const *estimated_flow)
{
  dst_iseq_convert_pcrel_inums_to_abs_inums(dst_iseq, dst_iseq_len);
  set<pair<long, long>> move_out_intervals;
  for (long i = 0; i < dst_iseq_len; i++) {
    dst_insn_t *dst_insn = &dst_iseq[i];
    unsigned long pcrel_val;
    src_tag_t pcrel_tag;
    if (   !dst_insn_get_pcrel_value(dst_insn, &pcrel_val, &pcrel_tag)
        || pcrel_tag != tag_abs_inum
        || !dst_insn_is_conditional_direct_branch(dst_insn)) {
      continue;
    }
    ASSERT(pcrel_tag == tag_abs_inum);
    ASSERT(pcrel_val >= 0 && pcrel_val < dst_iseq_len);
    if (   i + 1 < dst_iseq_len - 1  //not the last insn
        && i + 1 < pcrel_val         //forward jump
        && estimated_flow[pcrel_val] > estimated_flow[i + 1]) {
      move_out_intervals.insert(make_pair(i + 1, pcrel_val));
    }
  }
  CPP_DBG_EXEC(DST_INSN_SCHEDULE,
    cout << __func__ << " " << __LINE__ << ": dst_iseq =\n" << dst_iseq_to_string(dst_iseq, dst_iseq_len, as1, sizeof as1) << endl;
    cout << "move_out_intervals:\n" << move_out_intervals_to_string(move_out_intervals) << endl;
  );
  dst_iseq_convert_abs_inums_to_pcrel_inums(dst_iseq, dst_iseq_len);
  return dst_iseq_len;
}
