#include "config-host.h"
#include <time.h>
#include <support/debug.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include "superoptdb_rpc.h"
#include "support/c_utils.h"
#include "support/src-defs.h"
#include "i386/insn.h"
#include "x64/insn.h"
#include "etfg/etfg_insn.h"
#include "ppc/insn.h"
#include "i386/execute.h"
#include "ppc/execute.h"
#include "valtag/transmap.h"
#include "superopt/superopt.h"
#include "equiv/equiv.h"
#include "equiv/bequiv.h"
//#include "rewrite/harvest.h"
#include "rewrite/tagline.h"
#include "superopt/superoptdb.h"
#include "cutils/chash.h"
#include "rewrite/transmap_set.h"
#include "valtag/regset.h"

#define MAX_ITABLES_PER_SRC_ISEQ 16
#define MAX_ID_SIZE 256

static char *superoptdb_path = NULL;
//static char *superoptdb_sequences_path = NULL;
//static char *superoptdb_itables_path = NULL;

static char as1[4096];
static char as2[4096];

static char **superoptdb_src_iseq = NULL;
static long superoptdb_src_iseq_size = 0;
static long superoptdb_num_src_iseq = 0;
static char **superoptdb_itables = NULL;
static long superoptdb_itables_size = 0;
static long superoptdb_num_itables = 0;

static char *
superoptdb_sequences_path(void)
{
  static char pathname[4096];
  ASSERT(superoptdb_path);
  snprintf(pathname, sizeof pathname, "%s/sequences", superoptdb_path);
  ASSERT(strlen(pathname) < sizeof pathname);
  return pathname;
}

static char *
superoptdb_itables_path(void)
{
  static char pathname[4096];
  ASSERT(superoptdb_path);
  snprintf(pathname, sizeof pathname, "%s/itables", superoptdb_path);
  ASSERT(strlen(pathname) < sizeof pathname);
  return pathname;
}

static void
list_matching_src_iseqs(char ***ids, char const *cksum_prefix)
{
  list_all_dirents(ids, superoptdb_sequences_path(), cksum_prefix);
}

static void
list_src_iseqs_for_itable(char ***ids, char const *itable_id)
{
  char fpath[strlen(superoptdb_itables_path()) + 2 * MAX_ID_SIZE + 1024];
  snprintf(fpath, sizeof fpath, "%s/%s", superoptdb_itables_path(), itable_id);
  list_all_dirents(ids, fpath, "S");
}

/*static void
list_matching_transmaps_for_src_iseq(char ***ids, char const *src_iseq_id,
    char const *prefix)
{
  char src_iseq_path[strlen(superoptdb_sequences_path()) + MAX_ID_SIZE + 256];
  snprintf(src_iseq_path, sizeof src_iseq_path, superoptdb_sequences_path() "/%s",
      src_iseq_id);

  list_all_dirents(ids, src_iseq_path, prefix);
}

static void
list_all_transmaps_for_src_iseq(char ***ids, char const *src_iseq_id)
{
  list_matching_transmaps_for_src_iseq(ids, src_iseq_id, "tmap.");
}*/

static void
list_matching_windfalls_for_src_iseq_and_transmap(char ***ids, char const *src_iseq_id,
    /*char const *transmap_id, */char const *prefix)
{
  char path[strlen(superoptdb_sequences_path()) + MAX_ID_SIZE * 8 + 256];
  /*snprintf(path, sizeof path, "%s/%s/%s", superoptdb_sequences_path(),
      src_iseq_id, transmap_id);*/
  snprintf(path, sizeof path, "%s/%s", superoptdb_sequences_path(),
      src_iseq_id);

  list_all_dirents(ids, path, prefix);
}

static void
list_all_windfalls_for_src_iseq_and_transmap(char ***ids, char const *src_iseq_id/*,
    char const *transmap_id*/)
{
  list_matching_windfalls_for_src_iseq_and_transmap(ids, src_iseq_id,/* transmap_id,*/
      "windfall.");
}

static void
list_all_itables_for_src_iseq_and_transmap(char ***ids, char const *src_iseq_id/*,
    char const *transmap_id*/)
{
  char src_iseq_path[strlen(superoptdb_sequences_path()) + MAX_ID_SIZE + 256];
  /*snprintf(src_iseq_path, sizeof src_iseq_path, superoptdb_sequences_path() "/%s/%s",
      src_iseq_id, transmap_id);*/
  snprintf(src_iseq_path, sizeof src_iseq_path, "%s/%s", superoptdb_sequences_path(), 
      src_iseq_id/*, transmap_id*/);
  list_all_dirents(ids, src_iseq_path, "itable.");
}

static void
list_all_src_iseqs(char ***ids)
{
  list_all_dirents(ids, superoptdb_sequences_path(), NULL);
}

static void
list_all_itables(char ***ids)
{
  list_all_dirents(ids, superoptdb_itables_path(), NULL);
}

static uint64_t
time_elapsed_in_seconds_since(char const *timestamp)
{
  uint64_t prev, cur;
  char buf[256];
  prev = time_since_beginning(timestamp);
  cur = time_since_beginning(get_cur_timestamp(buf, sizeof buf));
  return cur - prev;
}

static bool
xget_id(char *id, char const *data, char const *path, char const *filename,
    char const *prefix)
{
  char fpath[strlen(path) + 1024];
  char cksum_str[256];
  struct stat statbuf;
  bool found = false;
  char **ids, **cur;
  uint64_t cksum;
  FILE *fp;
  long i;

  cksum = myhash_string(data);
  snprintf(cksum_str, sizeof cksum_str, "%s%" PRIx64, prefix, cksum);
  id[0] = '\0';
  list_all_dirents(&ids, path, cksum_str);
  for (cur = ids; *cur; cur++) {

    snprintf(id, MAX_ID_SIZE, "%s", *cur);
    snprintf(fpath, sizeof fpath, "%s/%s", path, id);

    char file[strlen(fpath) + 256];
    char tbuf[strlen(data) + 16];

    snprintf(file, sizeof file, "%s/%s", fpath, filename);
    fp = fopen(file, "r");
    ASSERT(fp);
    fread(tbuf, sizeof(char), strlen(data), fp);
    fclose(fp);
    if (!memcmp(tbuf, data, strlen(data))) {
      found = true;
    }
  }
  free_listing(ids);

  if (found) {
    return true;
  }
  for (i = 0;; i++) {
    snprintf(id, MAX_ID_SIZE, "%s%" PRIx64 ".%ld", prefix, cksum, i);
    snprintf(fpath, sizeof fpath, "%s/%s", path, id);
    if (stat(fpath, &statbuf) < 0) {
      char file[strlen(fpath) + 256];
      mkdir(fpath, 0755);
      snprintf(file, sizeof file, "%s/%s", fpath, filename);
      fp = fopen(file, "w");
      if (!fp) {
        printf("Could not open '%s' in write mode: %s\n", file, strerror(errno));
      }
      ASSERT(fp);
      fwrite(data, sizeof(char), strlen(data), fp);
      fclose(fp);
      break;
    }
  }
  return false;
}

static bool
itable_is_running(char const *itable_id)
{
  char fpath[strlen(superoptdb_sequences_path()) + 2 * MAX_ID_SIZE + 256];
  char const *timestamp;
  char status[256];
  size_t len;

  snprintf(fpath, sizeof fpath, "%s/%s/status", superoptdb_itables_path(), itable_id);
  len = xread_file(status, sizeof status, fpath);
  ASSERT(len < sizeof status);
  status[len] = '\0';
  if (strstart(status, "running since ", &timestamp)) {
    if (time_elapsed_in_seconds_since(timestamp) <= 100 * DEFAULT_YIELD_SECS) {
      return true;
    }
  }
  return false;
}

static bool
itable_cur_position(itable_position_t *ipos, char const *src_iseq_id,
    //char const *transmap_id,
    char const *itable_id)
{
  char fpath[strlen(superoptdb_sequences_path()) + 2 * MAX_ID_SIZE + 256];
  char optimal_found[256];
  char line[5120];
  uint64_t ret;

  snprintf(fpath, sizeof fpath, "%s/%s/itable.%s", superoptdb_sequences_path(),
      src_iseq_id, itable_id);
  FILE *fp = fopen(fpath, "r");
  if (!fp) {
    fprintf(stderr, "Could not open file %s/%s/itable.%s:"
        " %s\n", superoptdb_sequences_path(), src_iseq_id, itable_id, strerror(errno));
  }
  ASSERT(fp);
  fscanf(fp, "optimal found = %s\n", optimal_found);
  if (!strcmp(optimal_found, "true")) {
    fclose(fp);
    return false;
  }
  fscanf(fp, "max_vector: %[^\n]\n", line);
  fclose(fp);
  itable_position_from_string(ipos, line);
  return true;
}

static void
superoptdb_src_iseq_free(void)
{
  long i;
  for (i = 0; i < superoptdb_num_src_iseq; i++) {
    ASSERT(superoptdb_src_iseq[i]);
    free(superoptdb_src_iseq[i]);
    superoptdb_src_iseq[i] = NULL;
  }
  free(superoptdb_src_iseq);
}

static void
superoptdb_itables_free(void)
{
  long i;
  for (i = 0; i < superoptdb_num_itables; i++) {
    ASSERT(superoptdb_itables[i]);
    free(superoptdb_itables[i]);
    superoptdb_itables[i] = NULL;
  }
  free(superoptdb_itables);
}

static void
superoptdb_src_iseq_add(char const *src_iseq_id)
{
  ASSERT(superoptdb_num_src_iseq <= superoptdb_src_iseq_size);
  if (superoptdb_num_src_iseq == superoptdb_src_iseq_size) {
    superoptdb_src_iseq_size = (superoptdb_src_iseq_size + 100) * 2;
    superoptdb_src_iseq = (char **)realloc(superoptdb_src_iseq,
        superoptdb_src_iseq_size * sizeof(char *));
  }
  ASSERT(superoptdb_num_src_iseq < superoptdb_src_iseq_size);
  superoptdb_src_iseq[superoptdb_num_src_iseq] =
      (char *)malloc(MAX_ID_SIZE * sizeof(char));
  ASSERT(superoptdb_src_iseq[superoptdb_num_src_iseq]);
  strncpy(superoptdb_src_iseq[superoptdb_num_src_iseq], src_iseq_id, MAX_ID_SIZE);
  superoptdb_num_src_iseq++;
}

static void
superoptdb_itable_add(char const *itable_id)
{
  ASSERT(superoptdb_num_itables <= superoptdb_itables_size);
  if (superoptdb_num_itables == superoptdb_itables_size) {
    superoptdb_itables_size = (superoptdb_itables_size + 100) * 2;
    superoptdb_itables = (char **)realloc(superoptdb_itables,
        superoptdb_itables_size * sizeof(char *));
  }
  ASSERT(superoptdb_num_itables < superoptdb_itables_size);
  superoptdb_itables[superoptdb_num_itables] =
      (char *)malloc(MAX_ID_SIZE * sizeof(char));
  ASSERT(superoptdb_itables[superoptdb_num_itables]);
  strncpy(superoptdb_itables[superoptdb_num_itables], itable_id, MAX_ID_SIZE);
  superoptdb_num_itables++;
}

static bool
src_iseq_get_id(char *src_iseq_id, char const *harvested_iseq_string)
{
  bool found;
  found = xget_id(src_iseq_id, harvested_iseq_string, superoptdb_sequences_path(),
      "src_iseq", "");
  if (!found) {
    superoptdb_src_iseq_add(src_iseq_id);
  }
  return found;
}

static void
itable_associate(char const *itable_id, char const *src_iseq_id)
{
  size_t max_alloc = strlen(superoptdb_itables_path()) + MAX_ID_SIZE * 8 + 1024;
  char fpath[max_alloc], lpath[max_alloc];
  FILE *fp;
  int ret;

  snprintf(fpath, sizeof fpath, "%s/%s/itable.%s", superoptdb_sequences_path(),
      src_iseq_id, itable_id);
  if (!file_exists(fpath)) {
    fp = fopen(fpath, "w");
    fprintf(fp, "optimal found = false\n");
    fprintf(fp, "max_vector: %s\n",
        itable_position_to_string(itable_position_zero(), as1, sizeof as1));
    fclose(fp);
  }
  snprintf(lpath, sizeof lpath, "%s/%s/S%s", superoptdb_itables_path(), itable_id,
      src_iseq_id);
  if (!file_exists(lpath)) {
    ret = symlink(fpath, lpath);
    ASSERT(ret == 0);
  }
}

static bool
itable_get_id(char *itable_id, char const *itable_string)
{
  size_t max_alloc = strlen(superoptdb_itables_path()) + MAX_ID_SIZE * 8 + 1024;
  char fpath[max_alloc], lpath[max_alloc];
  char timestamp[256];
  bool found;
  FILE *fp;

  found = xget_id(itable_id, itable_string, superoptdb_itables_path(),
      "itab", "itab.");
  snprintf(fpath, sizeof fpath, "%s/%s/status", superoptdb_itables_path(), itable_id);
  if (!found) {
    superoptdb_itable_add(itable_id);
    fp = fopen(fpath, "w");
    ASSERT(fp);
    get_cur_timestamp(timestamp, sizeof timestamp);
    fprintf(fp, "stopped since %s\n", timestamp);
    fclose(fp);
  }
  return found;
}

static bool
windfall_get_id(char *windfall_id, char const *src_iseq_id,// char const *transmap_id,
    char const *windfall_string)
{
  char fpath[strlen(superoptdb_sequences_path()) + MAX_ID_SIZE * 8 + 1024];

  //snprintf(fpath, sizeof fpath, "%s/%s/%s", superoptdb_sequences_path(), src_iseq_id,
  //    transmap_id);
  snprintf(fpath, sizeof fpath, "%s/%s", superoptdb_sequences_path(), src_iseq_id);
  return xget_id(windfall_id, windfall_string, fpath, "dst_iseq", "windfall.");
}

/*#define ITABLE_STATUS_STOPPED 0
#define ITABLE_STATUS_RUNNING 1

static int
itable_get_status(char const *itable_id)
{
  char fpath[strlen(superoptdb_sequences_path()) + 2 * MAX_ID_SIZE + 256];
  char status[256];
  FILE *fp;

  snprintf(fpath, sizeof fpath, superoptdb_itables_path() "/%s/status", itable_id);
  fp = fopen(fpath, "r");
  ASSERT(fp);
  fscanf(fp, "%s", status);
  fclose(fp);
  ASSERT(!strcmp(status, "stopped") || !strcmp(status, "running"));
  if (!strcmp(status, "stopped")) {
    return ITABLE_STATUS_STOPPED;
  }
  return ITABLE_STATUS_RUNNING;
}*/

static bool
superoptdb_choose_itable(char *itable_id)
{
  long r, num_trials;

  if (!superoptdb_num_itables) {
    return false;
  }
#define MAX_CHOOSE_ITABLE_TRIALS 100
  num_trials = 0;
  do {
    r = rand() % superoptdb_num_itables;
    if (!itable_is_running(superoptdb_itables[r])) {
      strncpy(itable_id, superoptdb_itables[r], MAX_ID_SIZE);

      DBG2(SUPEROPTDB_SERVER, "Chosen itable %s randomly from "
          "%ld src_iseq choices.\n", itable_id, superoptdb_num_itables);
      return true;
    }
    num_trials++;
  } while (num_trials < MAX_CHOOSE_ITABLE_TRIALS);
  return false;
}

static bool
superoptdb_choose_src_iseq(char *src_iseq_id)
{
  long r;

  if (!superoptdb_num_src_iseq) {
    return false;
  }
  r = rand() % superoptdb_num_src_iseq;
  strncpy(src_iseq_id, superoptdb_src_iseq[r], MAX_ID_SIZE);
  DBG2(SUPEROPTDB_SERVER, "Chosen src_iseq %s randomly from "
      "%ld src_iseq choices.\n", src_iseq_id,
      superoptdb_num_src_iseq);
  return true;
}

/*
static int
count_src_ids(char **ids)
{
  char **cur = ids;
  int ret = 0;
  while (cur) {
    cur++;
    ret++;
  }
  return ret;
}

static bool
src_iseq_uses_itable(char const *iseq_id, char const *itable_id)
{
  char filename[MAX_ID_SIZE + 256];
  struct dirent *dirent;
  char path[512];
  bool ret;
  DIR *dir;

  snprintf(path, sizeof path, superoptdb_sequences_path() "/%s", iseq_id);
  dir = opendir(path);
  ret = false;
  snprintf(filename, sizeof filename, "itable.%s", itable_id);

  while (dirent = readdir(dir)) {
    if (!strcmp(dirent->d_name, filename)) {
      ret = true;
      break;
    }
  }
  closedir(dir);
  return ret;
}*/

static void
itable_mark_status(char const *itable_id, char const *status)
{
  char fpath[strlen(superoptdb_sequences_path()) + 2 * MAX_ID_SIZE + 256];
  char timestamp[256];
  FILE *fp;

  snprintf(fpath, sizeof fpath, "%s/%s/status", superoptdb_itables_path(), itable_id);
  fp = fopen(fpath, "w");
  ASSERT(fp);
  get_cur_timestamp(timestamp, sizeof timestamp);
  ASSERT(!strcmp(status, "running") || !strcmp(status, "stopped"));
  fprintf(fp, "%s since %s", status, timestamp);
  fclose(fp);
}

static size_t
superoptdb_output_peeptab_entry(char *buf, size_t size, char const *src_iseq_id,
    char const *windfall_id)
{
  char fpath[strlen(superoptdb_sequences_path()) + MAX_ID_SIZE * 8 + 512];
  char *ptr, *end;
  size_t sz;

  ptr = buf;
  end = ptr + size;
  ptr += snprintf(ptr, end - ptr, "ENTRY:\n");

  snprintf(fpath, sizeof fpath, "%s/%s/src_iseq", superoptdb_sequences_path(), src_iseq_id);
  sz = xread_file(ptr, end - ptr, fpath);
  ASSERT(ptr + sz < end);
  ptr += sz;
  //tagline = strstr(ptr, "\n== ");
  //ASSERT(tagline);
  //ptr = tagline + 1;
  //ASSERT(ptr < end);
  
  ptr += snprintf(ptr, end - ptr, "--\n");
  ASSERT(ptr < end);

  if (windfall_id) {
    snprintf(fpath, sizeof fpath, "%s/%s/%s/dst_iseq", superoptdb_sequences_path(),
        src_iseq_id, windfall_id);
    ptr += xread_file(ptr, end - ptr, fpath);
    ASSERT(ptr < end);
  }
  ptr += snprintf(ptr, end - ptr, "==\n\n");
  ASSERT(ptr < end);

  return ptr - buf;
}

static void
check_exists(char const *path)
{
  struct stat statbuf;
  if (stat(path, &statbuf) < 0) {
    char cmd[4096];
    snprintf(cmd, sizeof cmd, "mkdir -p %s", path);
    system(cmd);
  }
}

static char *
get_client_addr(struct svc_req *req, char *buf, size_t size)
{
  NOT_IMPLEMENTED();
  //struct hostent ret, *result;
  //struct sockaddr_in *clnt;
  //char tbuf[512];
  //int herrno, s;

  //ASSERT(req);
  //ASSERT(req->rq_xprt);
  //clnt = svc_getcaller(req->rq_xprt);
  //s = gethostbyaddr_r(&clnt->sin_addr, sizeof(struct in_addr), AF_INET, &ret, tbuf,
  //    sizeof tbuf, &result, &herrno);
  //ASSERT(s == 0);
  //DBG2(SUPEROPTDB_SERVER, "gethostbyaddr returned '%s'\n", ret.h_name);
  //ASSERT(strlen(ret.h_name) < size);
  //strncpy(buf, ret.h_name, size);
  //return buf;
}

int *
superoptdb_server_init_1_svc(superoptdb_init_args *args, struct svc_req *req)
{
  static int success;
  char **ids, **cur;

  if (superoptdb_path) {
    if (!strcmp(superoptdb_path, args->superoptdb_path)) {
      success = 1;
    } else {
      printf("error: init called with wrong superoptdb_path. cur: %s. new: %s.\n",
          superoptdb_path, args->superoptdb_path);
      success = 0;
    }
    return &success;
  }
  //pthread_mutex_init(&superoptdb_mutex, NULL);

  DBG(SUPEROPTDB_SERVER, "init called by %s. superoptdb_path %s.\n",
      get_client_addr(req, as1, sizeof as1), args->superoptdb_path);

  check_exists(args->superoptdb_path);
  superoptdb_path = strdup(args->superoptdb_path);
  ASSERT(superoptdb_path);

  check_exists(superoptdb_sequences_path());
  check_exists(superoptdb_itables_path());

  list_all_src_iseqs(&ids);
  for (cur = ids; *cur; cur++) {
    superoptdb_src_iseq_add(*cur);
  }
  free_listing(ids);
  list_all_itables(&ids);
  for (cur = ids; *cur; cur++) {
    superoptdb_itable_add(*cur);
  }
  free_listing(ids);
  success = 1;
  return &success;
}

void *
superoptdb_server_shutdown_1_svc(void *unused, struct svc_req *req)
{
  static int dummy;

  if (!superoptdb_path) {
    exit(0);
  }

  DBG(SUPEROPTDB_SERVER, "shutdown called by %s\n",
      get_client_addr(req, as1, sizeof as1));

  superoptdb_src_iseq_free();
  superoptdb_itables_free();
  free(superoptdb_path);
  superoptdb_path = NULL;
  exit(0);
  return &dummy;
}

static void
src_iseq_id_add_tagline(char const *src_iseq_id, char const *tagline)
{
  size_t max_alloc, len;
  char fpath[4096];
  char *buf;
  FILE *fp;

  max_alloc = 409600;
  buf = (char *)malloc(max_alloc * sizeof(char));
  ASSERT(buf);

  snprintf(fpath, sizeof fpath, "%s/%s/tagline", superoptdb_sequences_path(),
      src_iseq_id);
  if (file_exists(fpath)) {
    len = xread_file(buf, max_alloc, fpath);
    ASSERT(len < max_alloc);
  } else {
    len = 0;
  }
  buf[len] = '\0';

  len = taglines_union(buf, tagline, max_alloc);
  fp = fopen(fpath, "w");
  ASSERT(fp);
  xfwrite(buf, 1, len, fp);
  fclose(fp);
}

int *
superoptdb_server_add_1_svc(superoptdb_add_remove_args *args, struct svc_req *req)
{
  char src_iseq_id[MAX_ID_SIZE], windfall_id[MAX_ID_SIZE];
  char itable_id[MAX_ID_SIZE];
  static int found;

  found = 1;
  ASSERT(superoptdb_path);
  DBG(SUPEROPTDB_SERVER, "add called by %s:\nsrc_iseq:\n%s\n"
      "i386_iseq:\n%s\nitable:\n%s\n",
      get_client_addr(req, as1, sizeof as1), args->src_iseq,
      args->dst_iseq, args->itable);
  src_iseq_get_id(src_iseq_id, args->src_iseq);
  ASSERT(strlen(src_iseq_id));
  src_iseq_id_add_tagline(src_iseq_id, args->tagline);
  if (strcmp(args->dst_iseq, "")) {
    found = windfall_get_id(windfall_id, src_iseq_id, args->dst_iseq);
  }
  if (strcmp(args->itable, "")) {
    itable_get_id(itable_id, args->itable);
    itable_associate(itable_id, src_iseq_id);
  }
  DBG(SUPEROPTDB_SERVER, "returning %d\n", found);
  return &found;
}

int *
superoptdb_server_remove_1_svc(superoptdb_add_remove_args *args, struct svc_req *req)
{
  char src_iseq_id[MAX_ID_SIZE], /*transmap_id[MAX_ID_SIZE], */windfall_id[MAX_ID_SIZE];
  char itable_id[MAX_ID_SIZE];
  char fpath[strlen(superoptdb_sequences_path()) + MAX_ID_SIZE + 512];
  static int found;
  int ret;

  found = 0;
  ASSERT(superoptdb_path);
  DBG(SUPEROPTDB_SERVER, "remove called by %s:\nsrc_iseq:\n%s\n"
      "dst_iseq:\n%s\nitable:\n%s\n",
      get_client_addr(req, as1, sizeof as1), args->src_iseq,// args->transmaps,
      args->dst_iseq, args->itable);
  //pthread_mutex_lock(&superoptdb_mutex);
  ASSERT(!strcmp(args->tagline, ""));
  if (strcmp(args->src_iseq, "")) {
    src_iseq_get_id(src_iseq_id, args->src_iseq);
    ASSERT(strlen(src_iseq_id));
  }
  DBG2(SUPEROPTDB_SERVER, "src_iseq_id: %s.\n", src_iseq_id);
  DBG2(SUPEROPTDB_SERVER, "dst_iseq: %s.\n", args->dst_iseq);
  if (strcmp(args->dst_iseq, "")) {
    ASSERT(strlen(src_iseq_id));
    found = windfall_get_id(windfall_id, src_iseq_id, /*transmap_id, */args->dst_iseq);
    DBG2(SUPEROPTDB_SERVER, "windfall_id: %s.\n", windfall_id);
    snprintf(fpath, sizeof fpath, "%s/%s/%s/dst_iseq", superoptdb_sequences_path(),
        src_iseq_id, windfall_id);
    DBG(SUPEROPTDB_SERVER, "Unlinking %s.\n", fpath);
    ret = unlink(fpath);
    ASSERT(ret == 0);
    snprintf(fpath, sizeof fpath, "%s/%s/%s", superoptdb_sequences_path(),
        src_iseq_id, windfall_id);
    ret = rmdir(fpath);
    ASSERT(ret == 0);
  }
  if (strcmp(args->itable, "")) {
    itable_get_id(itable_id, args->itable);
    ASSERT(strlen(src_iseq_id));
    snprintf(fpath, sizeof fpath, "%s/%s/itable.%s", superoptdb_sequences_path(),
        src_iseq_id, /*transmap_id, */itable_id);
    DBG(SUPEROPT, "unlinking %s.\n", fpath);
    ret = unlink(fpath);
    ASSERT(ret == 0);
    snprintf(fpath, sizeof fpath, "%s/%s/S%s", superoptdb_itables_path(),
        itable_id, src_iseq_id/*, transmap_id*/);
    DBG(SUPEROPT, "unlinking %s.\n", fpath);
    ret = unlink(fpath);
    ASSERT(ret == 0);
  }
  if (!strcmp(args->dst_iseq, "") && !strcmp(args->itable, "")) {
    ASSERT(strlen(src_iseq_id));
    //remove src_iseq_id. update superoptdb_src_iseq
    NOT_IMPLEMENTED();
  }
  //pthread_mutex_unlock(&superoptdb_mutex);
  return &found;
}


static char *windfalls_string = NULL, *itables_string = NULL, *tagline_string = NULL;

static char *
get_tagline_string(char *buf, size_t size, char const *src_iseq_id)
{
  char fpath[4096];
  size_t len;

  snprintf(fpath, sizeof fpath, "%s/%s/tagline", superoptdb_sequences_path(),
      src_iseq_id);
  if (file_exists(fpath)) {
    len = xread_file(buf, size, fpath);
    ASSERT(len < size);
  } else {
    len = 0;
  }
  buf[len] = '\0';
  return buf;
}

static char *
get_itables_string(char *buf, size_t size, char const *src_iseq_id)
{
  char fpath[strlen(superoptdb_sequences_path()) + MAX_ID_SIZE * 8 + 512];
  char **itable_ids, **icur;
  char *ptr, *end;

  ptr = buf;
  end = ptr + size;
  *ptr = '\0';
  list_all_itables_for_src_iseq_and_transmap(&itable_ids, src_iseq_id);
  for (icur = itable_ids; *icur; icur++) {
    char const *itab_id;
    itab_id = *icur + strlen("itable.");
    DBG2(SUPEROPTDB_SERVER, "returning itab id %s.\n", itab_id);
    snprintf(fpath, sizeof fpath, "%s/%s/itab", superoptdb_itables_path(), itab_id);
    ptr += xread_file(ptr, end - ptr, fpath);
    ptr += snprintf(ptr, end - ptr, "\n==\n");
    ASSERT(ptr < end);
  }
  free_listing(itable_ids);
  return buf;
}

static char *
get_windfalls_string(char *buf, size_t size, char const *src_iseq_id)
{
  char fpath[strlen(superoptdb_sequences_path()) + MAX_ID_SIZE * 8 + 512];
  char **windfall_ids, **wcur;
  char *ptr, *end;

  ptr = buf;
  end = ptr + size;
  *ptr = '\0';
  list_all_windfalls_for_src_iseq_and_transmap(&windfall_ids, src_iseq_id);
  for (wcur = windfall_ids; *wcur; wcur++) {
    snprintf(fpath, sizeof fpath, "%s/%s/%s/dst_iseq", superoptdb_sequences_path(),
        src_iseq_id, *wcur);
    ptr += xread_file(ptr, end - ptr, fpath);
    ptr += snprintf(ptr, end - ptr, "\n==\n");
    ASSERT(ptr < end);
  }
  free_listing(windfall_ids);
  return buf;
}

superoptdb_get_src_iseq_details_retval *
superoptdb_server_get_src_iseq_details_1_svc(superoptdb_get_src_iseq_details_args *args,
    struct svc_req *req)
{
  static superoptdb_get_src_iseq_details_retval ret;
  long max_alloc = 128 * 1024 * 1024; //100MB
  char src_iseq_id[MAX_ID_SIZE];
  bool found;

  ASSERT(superoptdb_path);
  if (!windfalls_string) {
    windfalls_string = (char *)malloc(max_alloc * sizeof(char));
    ASSERT(windfalls_string);
  }
  if (!itables_string) {
    itables_string = (char *)malloc(max_alloc * sizeof(char));
    ASSERT(itables_string);
  }
  if (!tagline_string) {
    tagline_string = (char *)malloc(max_alloc * sizeof(char));
    ASSERT(tagline_string);
  }

  found = src_iseq_get_id(src_iseq_id, args->src_iseq);

  ret.found = found;
  ret.windfalls = get_windfalls_string(windfalls_string, max_alloc, src_iseq_id);
  ret.itables = get_itables_string(itables_string, max_alloc, src_iseq_id);
  ret.tagline = get_tagline_string(tagline_string, max_alloc, src_iseq_id);
  DBG2(SUPEROPTDB_SERVER, "args.src_iseq:\n%s\n", args->src_iseq);
  DBG2(SUPEROPTDB_SERVER, "ret.found = %d\nret.windfalls:\n%s\nret.itables:\n%s\n"
      "ret.tagline:\n%s\n", ret.found, ret.windfalls, ret.itables, ret.tagline);
  return &ret;
}

superoptdb_get_src_iseq_work_retval *
superoptdb_server_get_src_iseq_work_1_svc(void *unused, struct svc_req *req)
{
  char fpath[strlen(superoptdb_sequences_path()) + MAX_ID_SIZE * 8 + 512];
  static superoptdb_get_src_iseq_work_retval ret;
  static char *src_iseq_string = NULL;
  long max_alloc = 128 * 1024 * 1024; //100MB
  char src_iseq_id[MAX_ID_SIZE];
  char *ptr, *end;
  size_t len;

  ASSERT(superoptdb_path);
  if (!src_iseq_string) {
    src_iseq_string = (char *)malloc(max_alloc * sizeof(char));
    ASSERT(src_iseq_string);
  }
  if (!windfalls_string) {
    windfalls_string = (char *)malloc(max_alloc * sizeof(char));
    ASSERT(windfalls_string);
  }
  if (!itables_string) {
    itables_string = (char *)malloc(max_alloc * sizeof(char));
    ASSERT(itables_string);
  }
  if (!tagline_string) {
    tagline_string = (char *)malloc(max_alloc * sizeof(char));
    ASSERT(tagline_string);
  }

  //pthread_mutex_lock(&superoptdb_mutex);
  if (!superoptdb_choose_src_iseq(src_iseq_id)) {
    ret.src_iseq = "";
    ret.windfalls = "";
    ret.itables = "";
    ret.tagline = "";
    //pthread_mutex_unlock(&superoptdb_mutex);
    return &ret;
  }

  DBG2(SUPEROPTDB_SERVER, "returning src_iseq_id %s.\n", src_iseq_id);
  snprintf(fpath, sizeof fpath, "%s/%s/src_iseq", superoptdb_sequences_path(),
      src_iseq_id);
  len = xread_file(src_iseq_string, max_alloc, fpath);
  src_iseq_string[len] = '\0';

  ret.src_iseq = src_iseq_string;
  ret.windfalls = get_windfalls_string(windfalls_string, max_alloc, src_iseq_id);
  ret.itables = get_itables_string(itables_string, max_alloc, src_iseq_id);
  ret.tagline = get_tagline_string(tagline_string, max_alloc, src_iseq_id);
  //pthread_mutex_unlock(&superoptdb_mutex);

  return &ret;
}

static void
itable_vector_min(long *vector1, size_t *vector1_len, long const *vector2,
    size_t vector2_len)
{
  size_t i;

  if (*vector1_len == -1) {
    ASSERT(vector2_len >= 0);
    *vector1_len = vector2_len;
    memcpy(vector1, vector2, vector2_len * sizeof(long));
    return;
  }
  if (*vector1_len < vector2_len) {
    return;
  }
  if (*vector1_len > vector2_len) {
    memcpy(vector1, vector2, vector2_len * sizeof(long));
    *vector1_len = vector2_len;
    return;
  }
  ASSERT(*vector1_len == vector2_len);
  for (i = 0; i < *vector1_len; i++) {
    if (vector1[i] < vector2[i]) {
      return;
    }
    if (vector2[i] < vector1[i]) {
      memcpy(vector1, vector2, vector2_len * sizeof(long));
      return;
    }
  }
}

static void
src_iseq_transmap_id_break(char *src_iseq_id, char const *combined_id)
{
  ASSERT(combined_id[0] == 'S');
  strncpy(src_iseq_id, combined_id + 1, MAX_ID_SIZE);
}

static size_t
fingerprintdb_str_add(char *buf, size_t size, char const *src_iseq_id)
{
  char fpath[strlen(superoptdb_sequences_path()) + 2 * MAX_ID_SIZE + 256];
  char *ptr, *end;

  ptr = buf;
  end = buf + size;

  snprintf(fpath, sizeof fpath, "%s/%s/src_iseq", superoptdb_sequences_path(),
      src_iseq_id);
  ptr += xread_file(ptr, end - ptr, fpath);
  ptr += snprintf(ptr, end - ptr, "==\n");
  return ptr - buf;
}

superoptdb_get_itable_work_retval *
superoptdb_server_get_itable_work_1_svc(void *unused, struct svc_req *req)
{
  long start_sequence_len, itable_max_alloc, fingerprintdb_max_alloc;
  char fpath[strlen(superoptdb_sequences_path()) + 8 * MAX_ID_SIZE + 256];
  static char *itable_str = NULL, *fingerprintdb_str = NULL;
  char src_iseq_id[MAX_ID_SIZE], itable_id[MAX_ID_SIZE];
  static superoptdb_get_itable_work_retval ret;
  char **src_iseq_transmap_ids, **scur;
  //long start_vector[ENUM_MAX_LEN];
  itable_position_t ipos_start;
  char start_sequence_str[5120];
  //long start_vector_len;
  char *ptr, *end;
  long num_tries;

  ASSERT(superoptdb_path);
  //pthread_mutex_lock(&superoptdb_mutex);
  DBG(SUPEROPTDB_SERVER, "get_itable_work called by %s\n",
      get_client_addr(req, as1, sizeof as1));

  fingerprintdb_max_alloc = 4096UL * 1024 * 1024;
  itable_max_alloc = 4096 * 1024 * 16;

  if (!itable_str) {
    itable_str = (char *)malloc(itable_max_alloc * sizeof(char));
    ASSERT(itable_str);
  }
  if (!fingerprintdb_str) {
    fingerprintdb_str = (char *)malloc(fingerprintdb_max_alloc * sizeof(char));
    ASSERT(fingerprintdb_str);
  }

#define MAX_NUM_TRIES 10
  num_tries = 0;
  fingerprintdb_str[0] = '\0';
  do {
    if (num_tries >= MAX_NUM_TRIES || !superoptdb_choose_itable(itable_id)) {
      //printf("%s(): No remaining work!\n", __func__);
      ret.fingerprintdb = "";
      ret.itable = "";
      ret.start_sequence = "";
      DBG(SUPEROPTDB_SERVER, "returning %p: ret.max_len = -1\n", &ret);
      //pthread_mutex_unlock(&superoptdb_mutex);
      return &ret;
    }

    //start_vector_len = -1;
    itable_position_copy(&ipos_start, itable_position_zero());
    ptr = fingerprintdb_str;
    end = fingerprintdb_str + fingerprintdb_max_alloc;

    list_src_iseqs_for_itable(&src_iseq_transmap_ids, itable_id);
    for (scur = src_iseq_transmap_ids; *scur; scur++) {
      //long sequence_len, sequence[ENUM_MAX_LEN];
      itable_position_t ipos;

      src_iseq_transmap_id_break(src_iseq_id, *scur);
      if (!itable_cur_position(&ipos, src_iseq_id, itable_id)) {
        continue;
      }
      ptr += fingerprintdb_str_add(ptr, end - ptr, src_iseq_id);
      itable_position_min(&ipos_start, &ipos);
    }
    free_listing(src_iseq_transmap_ids);
    num_tries++;
  } while (fingerprintdb_str[0] == '\0');

  snprintf(fpath, sizeof fpath, "%s/%s/itab", superoptdb_itables_path(), itable_id);
  xread_file(itable_str, itable_max_alloc, fpath);
  itable_str[strlen(itable_str)] = '\0';

  itable_mark_status(itable_id, "running");
  //pthread_mutex_unlock(&superoptdb_mutex);

  itable_position_to_string(&ipos_start, start_sequence_str, sizeof start_sequence_str);
  ret.fingerprintdb = fingerprintdb_str;
  ret.itable = itable_str;
  ret.start_sequence = start_sequence_str;

  DBG(SUPEROPTDB_SERVER, "returning %p:\nfingerprintdb:\n%s\nitable:\n%s\n"
      "start_sequence: %s\n", &ret,
      ret.fingerprintdb, ret.itable, ret.start_sequence);
  return &ret;
}

static void
src_iseq_itable_record_max_vector(char const *src_iseq_id, char const *itable_id,
    char const *itable_sequence, bool optimal_found_flag)
{
  char fpath[strlen(superoptdb_sequences_path()) + 2 * MAX_ID_SIZE + 256];
  long max_sequence_alloc = strlen(itable_sequence)/2 + 100;
  char status_line[80], max_vector_line[5120];
  itable_position_t old_ipos, new_ipos;
  //size_t old_sequence_len, new_sequence_len;
  //long old_sequence[max_sequence_alloc];
  //long new_sequence[max_sequence_alloc];
  char const *max_vector_str;
  char const *optimal_found;
  FILE *fp;

  snprintf(fpath, sizeof fpath, "%s/%s/itable.%s", superoptdb_sequences_path(),
      src_iseq_id, itable_id);
  fp = fopen(fpath, "r+");
  if (!fp) {
    return;
  }
  fscanf(fp, "%[^\n]\n", status_line);
  if (!strstart(status_line, "optimal found = ", &optimal_found)) {
    printf("Error: Malformed file '%s'. 'optimal found' entry not found. Exiting.\n",
        fpath);
    NOT_REACHED();
  }
  ASSERT(!strcmp(optimal_found, "true") || !strcmp(optimal_found, "false"));
  if (!strcmp(optimal_found, "true")) {
    fclose(fp);
    return;
  }
  fscanf(fp, "%[^\n]\n", max_vector_line);
  if (!strstart(max_vector_line, "max_vector: ", &max_vector_str)) {
    printf("Error: Malformed file '%s'. 'max_vector' entry not found. Exiting.\n",
        fpath);
    NOT_REACHED();
  }
  ASSERT(max_vector_str);
  itable_position_from_string(&old_ipos, max_vector_str);
  itable_position_from_string(&new_ipos, itable_sequence);
  if (itable_positions_compare(&old_ipos, &new_ipos) >= 0) {
    fclose(fp);
    return;
  }
  fseek(fp, 0, SEEK_SET);
  ftruncate(fileno(fp), 0);
  fprintf(fp, "optimal found = %s\n", optimal_found_flag?"true":"false");
  fprintf(fp, "max_vector: %s\n", itable_sequence);
  fclose(fp);
}

void *
superoptdb_server_itable_work_done_1_svc(superoptdb_itable_work_done_args *args,
    struct svc_req *req)
{
  char src_iseq_id[MAX_ID_SIZE], itable_id[MAX_ID_SIZE];
  char *ptr, *end, *entry_end;
  int optimal_found;
  static int dummy;
  bool exists;
  long n;

  ASSERT(superoptdb_path);
  //pthread_mutex_lock(&superoptdb_mutex);
  DBG(SUPEROPTDB_SERVER, "itable_work_done called by %s:\nfingerprintdb:\n%s\n"
      "itable_sequence: %s\nitable:\n%s\n",
      get_client_addr(req, as1, sizeof as1), args->fingerprintdb,
      args->itable_sequence, args->itable);

  ptr = args->fingerprintdb;
  end = ptr + strlen(ptr);

  exists = itable_get_id(itable_id, args->itable);
  ASSERT(exists);

  n = 0;
  while (ptr < end) {
    entry_end = strstr(ptr, "==");
    ASSERT(entry_end);
    *entry_end = '\0';
    exists = src_iseq_get_id(src_iseq_id, ptr);
    ASSERT(exists);
    *entry_end = '=';
    sscanf(entry_end, "== %d ==\n", &optimal_found);
    //ASSERT(optimal_found == 0 || optimal_found == 1);
    ASSERT(optimal_found == 0);
    ptr = strchr(entry_end, '\n');
    ASSERT(ptr);
    ptr++;

    src_iseq_itable_record_max_vector(src_iseq_id, itable_id, args->itable_sequence,
        optimal_found?true:false);
    n++;
  }
  itable_mark_status(itable_id, "stopped");

  //pthread_mutex_unlock(&superoptdb_mutex);
  DBG(SUPEROPTDB_SERVER, "returning dummy %p\n", &dummy);
  return &dummy;
}

superoptdb_write_peeptab_retval *
superoptdb_server_write_peeptab_1_svc(superoptdb_write_peeptab_args *args,
    struct svc_req *req)
{
  char **ids, **cur, **windfall_ids, **wcur;
  //long dst_iseq_size, src_iseq_size, dst_iseq_len, c;
  static superoptdb_write_peeptab_retval ret;
  static char *peeptab_str = NULL;
  unsigned long max_alloc;
  //dst_insn_t *dst_iseq;
  char *ptr, *end;

  ASSERT(superoptdb_path);
  DBG2(SUPEROPTDB_SERVER, "write_peeptab called by %s\n",
      get_client_addr(req, as1, sizeof as1));

  max_alloc = 4096UL * 1024 * 1024; //4GB
  if (!peeptab_str) {
    peeptab_str = (char *)malloc(max_alloc * sizeof(char));
  }
  ASSERT(peeptab_str);
  //pthread_mutex_lock(&superoptdb_mutex);
  ptr = peeptab_str;
  end = ptr + max_alloc;
  list_all_src_iseqs(&ids);
  for (cur = ids; *cur; cur++) {
    list_all_windfalls_for_src_iseq_and_transmap(&windfall_ids, *cur);
    bool empty = true;
    for (wcur = windfall_ids; *wcur; wcur++) {
      ptr += superoptdb_output_peeptab_entry(ptr, end - ptr, *cur, *wcur);
      empty = false;
    }
    free_listing(windfall_ids);
    if (empty && args->write_empty_entries) {
      ptr += superoptdb_output_peeptab_entry(ptr, end - ptr, *cur, NULL);
    }
  }
  free_listing(ids);
  ret.peeptab = peeptab_str;
  DBG2(SUPEROPTDB_SERVER, "returning %p:\n%s\n", &ret, ret.peeptab);
  //pthread_mutex_unlock(&superoptdb_mutex);
  return &ret;
}
