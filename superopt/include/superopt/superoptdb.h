#ifndef SUPEROPTDB_H
#define SUPEROPTDB_H
#include "support/src-defs.h"
#include <stdlib.h>
#include "valtag/transmap.h"
#include "valtag/regmap.h"
#include "superopt/typestate.h"
#include "equiv/equiv.h"

#define CMN COMMON_SRC
#include "cmn/cmn_fingerprintdb.h"
#undef CMN

#define CMN COMMON_DST
#include "cmn/cmn_fingerprintdb.h"
#undef CMN

//#define SUPEROPTDB_SERVER_DEFAULT "superoptdb.cse.iitd.ernet.in"
#define SUPEROPTDB_SERVER_DEFAULT "localhost"
#define DEFAULT_YIELD_SECS (30 * 60 * 60 * 60)   //1800 hours

//#define FINGERPRINT_BOOLTEST_ONLY 0x1234567812345678ULL

struct regset_t;
struct regmap_t;
struct src_insn_t;
struct dst_insn_t;
struct state_t;
struct transmap_t;
struct itable_t;
struct itable_position_t;
struct src_fingerprintdb_elem_t;
struct translation_instance_t;
struct typestate_t;

bool superoptdb_init(char const *server, char const *superoptdb_path);
void superoptdb_shutdown(void);

/* The following four functions are implemented using client/server model. The
   client is stateless and makes no assumption about the structure/naming of
   the database at the server. */
void superoptdb_add(struct translation_instance_t *ti,
    struct src_insn_t const *iseq, long iseq_len,
    struct transmap_t const *in_tmap, struct transmap_t const *out_tmaps,
    struct regset_t const *live_out, long num_live_out, bool mem_live_out,
    char const *tagline,
    struct dst_insn_t const *dst_iseq, long dst_iseq_len,
    struct itable_t const *itable);

void superoptdb_add_after_rationalizing(struct translation_instance_t *ti,
    struct src_insn_t const *iseq, long iseq_len,
    struct transmap_t const *in_tmap, struct transmap_t const *out_tmaps,
    struct regset_t const *live_out, long num_live_out, bool mem_live_out,
    //struct regmap_t const *dst_regmap,
    struct dst_insn_t const *dst_iseq, long dst_iseq_len);

void superoptdb_remove(struct translation_instance_t *ti,
    struct src_insn_t const *iseq, long iseq_len,
    struct transmap_t const *in_tmap, struct transmap_t const *out_tmap,
    struct regset_t const *live_out, long num_live_out, bool mem_live_out,
    struct dst_insn_t const *dst_iseq, long dst_iseq_len,
    struct itable_t const *itable);

void superoptdb_shutdown(void);

struct src_fingerprintdb_elem_t **superoptdb_get_itable_work(struct itable_t *itable,
    struct itable_position_t *ipos,// bool use_types,
    struct typestate_t *typestate_initial, struct typestate_t *typestate_final);

void superoptdb_itable_work_done(src_fingerprintdb_elem_t * const *fingerprintdb,
    struct itable_t const *itable, struct itable_position_t const *ipos);

bool superoptdb_get_src_iseq_work(struct translation_instance_t *ti,
    struct src_insn_t *src_iseq, long src_iseq_size,
    long *src_iseq_len,
    struct regset_t *live_out,
    struct transmap_t *in_tmap, struct transmap_t *out_tmaps, long out_tmaps_size,
    size_t *num_out_tmaps, bool *mem_live_out, char *tagline, size_t tagline_size,
    struct dst_insn_t **windfalls, long *windfall_lens, long windfalls_size,
    long *num_windfalls,
    struct itable_t *itables, long itables_size,
    size_t *num_itables);

bool superoptdb_get_src_iseq_details(struct translation_instance_t *ti,
    struct src_insn_t const *iseq, long iseq_len,
    struct transmap_t const *in_tmap, struct transmap_t const *out_tmaps,
    struct regset_t const *live_out,
    long num_live_out, bool mem_live_out,
    char *tagline, size_t tagline_size, struct dst_insn_t **windfalls,
    long *windfall_lens, long windfall_size, long *num_windfalls,
    struct itable_t *itables, long itables_size,
    size_t *num_itables);

void superoptdb_record_max_vector(struct src_insn_t const *src_iseq, long src_iseq_len,
    struct regset_t const *live_out,
    struct transmap_t const *in_tmap, struct transmap_t const *out_tmaps,
    long num_live_out, bool mem_live_out,
    struct itable_t const *itable, long const *sequence,
    long sequence_len);

void free_get_src_iseq_retvals(struct itable_t *itables[], size_t num_itables[],
    struct dst_insn_t **windfalls[], long num_windfalls[], long num_choices);
    
/*
struct fingerprintdb_elem_t **superoptdb_get_work_to_do(struct itable_entry_t *itable,
    long *itable_size, long *start_sequence, long *start_sequence_len,
    transmap_t *tmap, transmap_t *out_tmap, long max_out_tmaps,
    struct regset_t *live_regs, long live_out_size, long *num_live_out, long *max_len);*/
void superoptdb_write_peeptab(char const *peeptab, bool write_empty_entries);
void peep_table_load(translation_instance_t *ti);

#define SUPEROPTDB_PATH_DEFAULT ABUILD_DIR "/superoptdb"
//#define SUPEROPTDB_SEQUENCES_PATH SUPEROPTDB_PATH "/sequences"
//#define SUPEROPTDB_ITABLES_PATH SUPEROPTDB_PATH "/itables"

extern char const *superoptdb_server;

#endif /* superoptdb.h */
