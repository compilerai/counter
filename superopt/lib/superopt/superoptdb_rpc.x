#include "config-host.h"

struct superoptdb_add_remove_args {
  string src_iseq<>;
  string tagline<>;
  //string transmaps<>;
  string dst_iseq<>;
  string itable<>;
};

struct superoptdb_get_itable_work_retval {
  string fingerprintdb<>;
  string itable<>;
  string start_sequence<>;
  //int max_len;
  //string src_iseq<>;
  //string transmaps<>;
};

struct superoptdb_get_src_iseq_work_retval {
  string src_iseq<>;
  //string transmaps<>;
  string windfalls<>;
  string itables<>;
  string tagline<>;
};

struct superoptdb_get_src_iseq_details_args {
  string src_iseq<>;
};

struct superoptdb_get_src_iseq_details_retval {
  int found;
  string tagline<>;
  string windfalls<>;
  string itables<>;
};

struct superoptdb_itable_work_done_args {
  string fingerprintdb<>;
  string itable<>;
  string itable_sequence<>;
};

struct superoptdb_write_peeptab_args {
  int write_empty_entries;
};

struct superoptdb_write_peeptab_retval {
  string peeptab<>;
};

struct superoptdb_init_args {
  string superoptdb_path<>;
};

program SUPEROPTDB_SERVER_RPC {
  version SUPEROPTDB_SERVER_RPC_V1 {
    int SUPEROPTDB_SERVER_INIT(superoptdb_init_args) = 1;
    void SUPEROPTDB_SERVER_SHUTDOWN(void) = 2;
    int SUPEROPTDB_SERVER_ADD(superoptdb_add_remove_args) = 3;
    int SUPEROPTDB_SERVER_REMOVE(superoptdb_add_remove_args) = 4;
    superoptdb_get_itable_work_retval SUPEROPTDB_SERVER_GET_ITABLE_WORK(void) = 5;
    superoptdb_get_src_iseq_work_retval SUPEROPTDB_SERVER_GET_SRC_ISEQ_WORK(void) = 6;
    superoptdb_get_src_iseq_details_retval SUPEROPTDB_SERVER_GET_SRC_ISEQ_DETAILS(superoptdb_get_src_iseq_details_args) = 7;
    void SUPEROPTDB_SERVER_ITABLE_WORK_DONE(superoptdb_itable_work_done_args) = 8;
    superoptdb_write_peeptab_retval SUPEROPTDB_SERVER_WRITE_PEEPTAB(superoptdb_write_peeptab_args) = 9;
  } = 1;
} = XXX_PROGNUM_XXX; /* program number : must be between 0x20000000-0x3fffffff */
