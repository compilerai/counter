#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
//using namespace std;

//extern "C" {
//#include "cpu.h"

#include "valtag/regset.h"
#include "support/debug.h"
#include "config-host.h"
#include "rewrite/jumptable.h"
//#include "imm_map.h"
#include "i386/disas.h"
#include "i386/insn.h"
#include "codegen/etfg_insn.h"
#include "rewrite/ldst_input.h"
#include "superopt/superoptdb.h"
#include "valtag/nextpc_map.h"
#include "valtag/symbol_set.h"
//#include "rewrite/locals_info.h"

#include "cutils/imap_elem.h"
#include "valtag/elf/elf.h"
#include "support/globals.h"

#include "i386/opctable.h"
#include "support/c_utils.h"
#include "cutils/pc_elem.h"

#include "ppc/regs.h"
#include "i386/regs.h"
#include "rewrite/rdefs.h"
#include "valtag/memset.h"
#include "ppc/insn.h"
#include "rewrite/src-insn.h"

#include "rewrite/edge_table.h"

#include "valtag/elf/elftypes.h"
#include "rewrite/translation_instance.h"
#include "rewrite/static_translate.h"
#include "rewrite/transmap.h"
#include "rewrite/peephole.h"
//#include "rewrite/harvest.h"
//}

static char as1[40960];
static char as2[40960];
static char as3[1024];

//void g_ctx_init(void);

static void
usage(void)
{
  printf("h2p:\n"
      "usage: harvest [-live_all] [-o outfile] [-function_per_file] program1 program2\n"
      "\n"
      "-live_all      enumerate all subsets of touched registers as live\n"
      "\n");
}

/*
typedef struct nonvoid_fun_t {
  char *name;
  int size;
} nonvoid_fun_t;


static int nonvoid_fun_size(char const *file_name, nonvoid_fun_t const *nonvoid_funs)
{
  int i = 0;
  char const * fun_name = strchr(file_name, '/');
  if (fun_name) {
    fun_name++;
  } else {
    fun_name = file_name;
  }
  while(nonvoid_funs[i].name) {
    if(!strcmp(nonvoid_funs[i].name, fun_name)) {
      return nonvoid_funs[i].size;
    }
    ++i;
  }
  return 0;
}

static size_t
eqcheck_regsets_to_string(regset_t const *regset, long n, char *buf, size_t size, char const *fun_name, nonvoid_fun_t const *nonvoid_funs)
{
  char tmpbuf[4096];
  char *ptr, *end;
  int func_retsize;
  long i;

  ptr = buf;
  end = buf + size;
  for (i = 0; i < n; i++) {
    if(   i == 0
       && (func_retsize = nonvoid_fun_size(fun_name, nonvoid_funs))) {
      if (func_retsize > DWORD_LEN) {
        ptr += snprintf(ptr, end - ptr, "exr0.5[0x%llx], exr0.6[0x%llx],",
            MAKE_MASK(DWORD_LEN), MAKE_MASK(func_retsize - DWORD_LEN));
      } else {
        ptr += snprintf(ptr, end - ptr, "exr0.5[0x%llx],", MAKE_MASK(func_retsize));
      }
    } else {
      ptr += snprintf(ptr, end - ptr, "%s", "");
    }
    ASSERT(ptr < end);
    ptr += snprintf(ptr, end - ptr, "#");
    ASSERT(ptr < end);
  }
  return ptr - buf;
}
*/

static void
gen_peeptab_entry_for_fingerprintdb_elems(char const *ofile,
    /*src_fingerprintdb_elem_t const *e1, */dst_fingerprintdb_elem_t const *e2,
    /*symbol_set_t const *symbol_set1, */symbol_set_t const *symbol_set2)
{
  char const /* *nextpcs1_start, */*nextpcs2_start, /* *nextpcs1_end, */*nextpcs2_end, *end;
  static imm_vt_map_t /*src_symbols_map, */dst_symbols_map, locals_map;
  static nextpc_map_t /*nextpc_map1, */nextpc_map2/*, nextpc_map_peep*/;
  long /*src_iseq_len, */dst_iseq_len;
  size_t peep_comment_size;
  //src_insn_t *src_iseq;
  dst_insn_t *dst_iseq;
  char *peep_comment;
  char *ptr;
  FILE *ofp;

  ofp = fopen(ofile, "a");
  if (!ofp) {
    printf("fopen() failed on '%s'\n", ofile);
  }
  ASSERT(ofp);

  peep_comment_size = strlen(e2->comment) * 2 + 102400 * sizeof(char);

  //src_iseq = (src_insn_t *)malloc(e1->cmn_iseq_len * sizeof(src_insn_t));
  //ASSERT(src_iseq);
  dst_iseq = (dst_insn_t *)malloc(e2->cmn_iseq_len * sizeof(dst_insn_t));
  ASSERT(dst_iseq);
  peep_comment = (char *)malloc(peep_comment_size * sizeof(char));
  ASSERT(peep_comment);

  //src_iseq_copy(src_iseq, e1->cmn_iseq, e1->cmn_iseq_len);
  dst_iseq_copy(dst_iseq, e2->cmn_iseq, e2->cmn_iseq_len);

  //DBG(TMP, "before canonicalize, dst_iseq:\n%s\n", dst_iseq_to_string(dst_iseq, e2->cmn_iseq_len, as1, sizeof as1));
  src_iseq_canonicalize_symbols(dst_iseq, e2->cmn_iseq_len, &dst_symbols_map);

  //DBG(TMP, "after canonicalize, dst_iseq:\n%s\n", dst_iseq_to_string(dst_iseq, e2->cmn_iseq_len, as1, sizeof as1));
  //src_iseq_canonicalize_locals(src_iseq, e1->cmn_iseq_len, &locals_map);
  //locals_info_rename_local_nums_using_imm_vt_map(e1->locals_info, &locals_map);

  //imm_vt_map_copy(&dst_symbols_map, &src_symbols_map);
  //imm_vt_map_rename_symbols_using_symbol_sets(&dst_symbols_map, symbol_set1, symbol_set2);
  //imm_vt_map_rename_locals_using_locals_info(&locals_map, e1->locals_info, e2->locals_info);

  //locals_info_rename_local_nums_using_imm_vt_map(e2->locals_info, &locals_map);

  /*if (!dst_iseq_inv_rename_symbols(dst_iseq, e2->cmn_iseq_len, &dst_symbols_map)) {
    printf("Warning: dst_iseq_inv_rename_symbols() failed\n");
  }

  if (!dst_iseq_inv_rename_locals(dst_iseq, e2->cmn_iseq_len, &locals_map)) {
    NOT_REACHED();
  }


  nextpcs1_start = strchr(e1->comment, '#');
  ASSERT(nextpcs1_start);*/
  nextpcs2_start = strchr(e2->comment, '#');
  ASSERT(nextpcs2_start);

  //nextpcs1_end = strchr(nextpcs1_start, '=');
  //ASSERT(nextpcs1_end);
  nextpcs2_end = strchr(nextpcs2_start, '=');
  ASSERT(nextpcs2_end);

  //char nextpcs1_string[nextpcs1_end - nextpcs1_start + 1];
  char nextpcs2_string[nextpcs2_end - nextpcs2_start + 1];
  //memcpy(nextpcs1_string, nextpcs1_start + 1, nextpcs1_end - nextpcs1_start);
  //nextpcs1_string[nextpcs1_end - nextpcs1_start] = '\0';
  memcpy(nextpcs2_string, nextpcs2_start + 1, nextpcs2_end - nextpcs2_start);
  nextpcs2_string[nextpcs2_end - nextpcs2_start] = '\0';

  //nextpc_map_from_short_string_using_symbols(&nextpc_map1, nextpcs1_string, symbol_set1);
  nextpc_map_from_short_string_using_symbols(&nextpc_map2, nextpcs2_string, symbol_set2);

  /*if (!nextpc_maps_join(&nextpc_map_peep, &nextpc_map1, symbol_set1, &nextpc_map2, symbol_set2)) {
    printf("nextpcs don't match for: %s <-> %s\n", e1->comment, e2->comment);
    free(src_iseq);
    free(dst_iseq);
    free(peep_comment);
    fclose(ofp);
    return;
  }

  dst_iseq_rename_nextpcs(dst_iseq, e2->cmn_iseq_len, &nextpc_map_peep);*/

  fprintf(ofp, "ENTRY:\n--\n");
  /*fprintf(ofp, "ENTRY:\n%s--\n",
      src_iseq_to_string(src_iseq, e1->cmn_iseq_len, as1, sizeof as1));*/
  //eqcheck_regsets_to_string(e1->live_out, e1->num_live_out, as1, sizeof as1, ofile, nonvoid_funs);
  regsets_to_string(e2->live_out, e2->num_live_out, as1, sizeof as1);
  fprintf(ofp, "live : %s\nmemlive : true\n--\n", as1);
  fprintf(ofp, "%s--\n",
      transmaps_to_string(&e2->in_tmap, e2->out_tmaps, e2->num_live_out, as1, sizeof as1));

  //imm_vt_map_to_string_using_symbols(&src_symbols_map, symbol_set1, as1, sizeof as1),
  //fprintf(ofp, "%s--\n", as1);
  fprintf(ofp, "--\n");
  imm_vt_map_to_string_using_symbols(&dst_symbols_map, symbol_set2, as1, sizeof as1),
  fprintf(ofp, "%s--\n", as1);

  /*locals_info_to_string(e1->locals_info, as1, sizeof as1);
  fprintf(ofp, "%s--\n", as1);*/
  fprintf(ofp, "--\n");
  locals_info_to_string(e2->locals_info, as1, sizeof as1);
  fprintf(ofp, "%s--\n", as1);

  //insn_line_map_to_string(&e1->insn_line_map, as1, sizeof as1);
  insn_line_map_t insn_line_map;
  insn_line_map_init(&insn_line_map);
  insn_line_map_to_string(&insn_line_map, as1, sizeof as1);
  fprintf(ofp, "%s--\n", as1);
  insn_line_map_to_string(&e2->insn_line_map, as1, sizeof as1);
  fprintf(ofp, "%s--\n", as1);

  //strncpy(peep_comment, e1->comment, peep_comment_size);
  strncpy(peep_comment, e2->comment, peep_comment_size);

  end = peep_comment + peep_comment_size;
  ptr = strrchr(peep_comment, '=');
  ASSERT(ptr && ptr > peep_comment + 1 && ptr[-1] == '=');
  ptr--;

  /*ptr += snprintf(ptr, end - ptr, "# src-symbols %s dst-symbols %s ==",
      imm_vt_map_to_short_string_using_symbols(&src_symbols_map, symbol_set1, as1, sizeof as1),
      imm_vt_map_to_short_string_using_symbols(&dst_symbols_map, symbol_set2, as2, sizeof as2));*/
  ptr += snprintf(ptr, end - ptr, "==");
  ASSERT(ptr < end);
  
  fprintf(ofp, "%s%s\n\n",
      dst_iseq_to_string(dst_iseq, e2->cmn_iseq_len, as1, sizeof as1), peep_comment);

  //free(src_iseq);
  free(dst_iseq);
  free(peep_comment);
  fclose(ofp);
}

static char *
get_peeptab_filename(char const *dirname, char const *comment,//src_fingerprintdb_elem_t const *iter1,
    char *buf, size_t size)
{
  char const *func_name, *colon;
  char *ptr, *end;

  ptr = buf;
  end = buf + size;

  //func_name = get_function_name_from_fdb_comment(iter1->comment);
  func_name = get_function_name_from_fdb_comment(comment);
  colon = strchr(func_name, ':');
  ASSERT(colon);

  ptr += snprintf(ptr, end - ptr, "%s/", dirname);
  ASSERT(ptr < end);

  memcpy(ptr, func_name, colon - func_name);
  ptr[colon - func_name] = '\0';
  ptr += colon - func_name;
  ASSERT(ptr < end);
  return buf;
}

static void
generate_peeptabs_from_fdb_and_symbol_set(char const *ofile, bool function_per_file,
    /*src_fingerprintdb_elem_t * const *fdb1, */dst_fingerprintdb_elem_t *  const *fdb2,
    /*symbol_set_t const *symbol_set1, */symbol_set_t const *symbol_set2/*,
    nonvoid_fun_t const *nonvoid_funs*/)
{
  char peeptab_filename_store[strlen(ofile) + MAX_SYMNAME_LEN + 256];
  //src_fingerprintdb_elem_t const *iter1;
  dst_fingerprintdb_elem_t const *iter2;
  char const *peeptab_filename;
  imm_vt_map_t symbols_map;
  src_insn_t *src_iseq;
  long i;

  for (i = 0; i < FINGERPRINTDB_SIZE; i++) {
    for (iter2 = fdb2[i]; iter2; iter2 = iter2->next) {
      DBG(TMP, "iter2 %s\n", iter2->comment);
      //src_iseq = (src_insn_t *)malloc(iter2->cmn_iseq_len * sizeof(src_insn_t));
      //ASSERT(src_iseq);
      //src_iseq_copy(src_iseq, iter2->cmn_iseq, iter2->cmn_iseq_len);
      //src_iseq_canonicalize_symbols(src_iseq, iter2->cmn_iseq_len, &symbols_map);
#if 0
      iter2 = dst_fingerprintdb_search_for_matching_comment(iter1->comment,
          /*iter1->num_live_out, &imm_vt_map, */fdb2);
      if (!iter2) {
        printf("matching comment not found for %s\n", iter1->comment);
        free(src_iseq);
        continue;
      }
#endif
#if ARCH_SRC == ARCH_DST
/*
      if (src_iseqs_equal_mod_imms(iter1->cmn_iseq, iter1->cmn_iseq_len,
          iter2->cmn_iseq, iter2->cmn_iseq_len)) {
        DBG(TMP, "identical sequences found for %s <-> %s\n", iter1->comment, iter2->comment);
        free(src_iseq);
        continue;
      }
*/
#endif
      /*if (iter1->num_live_out != iter2->num_live_out) {
        printf("num_nextpcs not equal (%ld <-> %ld) for %s <-> %s.\n",
            iter1->num_live_out, iter2->num_live_out,
            iter1->comment, iter2->comment);
        free(src_iseq);
        continue;
      }*/
#if 0
      if (!imm_vt_map_rename_symbols_using_symbol_sets(&symbols_map, symbol_set1, symbol_set2)) {
        /*printf("symbols rename failed for %s <-> %s\n", iter1->comment, iter2->comment);
        free(src_iseq);
        continue;*/
      }
#endif
#if 0
      if (!dst_iseq_matches_with_symbols(iter2->cmn_iseq, iter2->cmn_iseq_len,
              &symbols_map)) {
        printf("Warning: symbols don't match for %s <-> %s.\n", iter1->comment,
            iter2->comment);
        /*free(src_iseq);
        continue;*/
      }
#endif
      if (function_per_file) {
        peeptab_filename = get_peeptab_filename(ofile, iter2->comment, peeptab_filename_store,
            sizeof peeptab_filename_store);
      } else {
        peeptab_filename = ofile;
      }
      gen_peeptab_entry_for_fingerprintdb_elems(peeptab_filename, /*iter1, */iter2,
          /*symbol_set1, */symbol_set2);
      //free(src_iseq);
    }
  }
}

/*static nonvoid_fun_t *
read_nonvoid_file(char const *nonvoid_file)
{
  long length;
  char* buffer, *token;
  nonvoid_fun_t *nonvoid_funs;
  int num_funs, i;
  FILE * f = fopen(nonvoid_file, "rb");

  if(f)
  {
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);
    buffer = malloc(length + 1);
    ASSERT(buffer);
    fread(buffer, 1, length, f);
    buffer[length] = 0;
    fclose(f);

    num_funs = 0;
    for(i = 0; buffer[i]; ++i)
    {
      if(buffer[i] == '\n') {
        num_funs++;
      }
    }
    
    nonvoid_funs = (nonvoid_fun_t *) malloc(sizeof (nonvoid_fun_t) * (num_funs + 2));
    i = 0;
    while(token = strsep(&buffer, "\n")) {
      char *space;
      if (*token == '\0') {
        continue;
      }
      nonvoid_funs[i].name = token;
      DBG(H2P, "token = %s.\n", token);
      space = strchr(token, ' ');
      ASSERT(space);
      nonvoid_funs[i].size = atoi(space + 1);
      *space = '\0';
      ++i;
    }
    nonvoid_funs[i].name = 0;
    nonvoid_funs[i].size = 0;
    return nonvoid_funs;
  }
  else {
    ASSERT(false && "Can't open nonvoid file");
    return NULL;
  }
}

static void
free_nonvoid_funs(nonvoid_fun_t *nonvoid_funs)
{
  free(nonvoid_funs[0].name);
  free(nonvoid_funs);
}*/

int
main(int argc, char **argv)
{
  bool function_per_file, sort_by_occurrences, add_transmap_identity, sort_increasing;
  char const *ofile, *r/*, *filename1*/, *filename2/*, *logfile1*/, *logfile2, *function_name;
  static symbol_set_t /*symbol_set1, */symbol_set2;
  //src_fingerprintdb_elem_t **fdb1;
  dst_fingerprintdb_elem_t **fdb2;
  FILE /**fp1, */*fp2;
  int i, optind;
  //nonvoid_fun_t *nonvoid_funs;
  //nonvoid_file = 0;

  optind = 1;
  ofile = "peeptab.out";
  //logfile1 = "rewrite.log.1";
  logfile2 = "rewrite.log.2";
  function_per_file = false;
  sort_by_occurrences = true;
  add_transmap_identity = false;
  sort_increasing = false;
  function_name = NULL;

  g_exec_name = argv[0];
  for (;;) {
    if (optind >= argc) {
      break;
    }
    r = argv[optind];
    if (r[0] != '-') {
      break;
    }
    optind++;
    r++;
    if (!strcmp(r, "-")) {
      break;
    } else if (!strcmp(r, "function_name")) {
      ASSERT(optind < argc);
      function_name = argv[optind++];
    } else if (!strcmp(r, "function_per_file")) {
      function_per_file = true;
    } else if (!strcmp(r, "no_sort_occur")) {
      sort_by_occurrences = false;
    } else if (!strcmp(r, "add_transmap_identity")) {
      add_transmap_identity = true;
    } else if (!strcmp(r, "sort_inc")) {
      sort_increasing = true;
    } else if (!strcmp(r, "o")) {
      ASSERT(optind < argc);
      ofile = argv[optind++];
    /*} else if (!strcmp(r, "log1")) {
      ASSERT(optind < argc);
      logfile1 = argv[optind++];*/
    } else if (!strcmp(r, "log2")) {
      ASSERT(optind < argc);
      logfile2 = argv[optind++];
    /*} else if (!strcmp(r, "nonvoid")) {
      ASSERT(optind < argc);
      nonvoid_file = argv[optind++];*/
    } else if (!strcmp(r, "help")) {
      usage();
      exit(0);
    } else {
      printf("Could not parse arg: %s.\n", r);
      usage();
      exit(1);
    }
  }

  if (argc != optind + 1) {
    usage();
    exit(1);
  }
  //filename1 = argv[optind];
  //optind++;
  filename2 = argv[optind];
  optind++;

  src_init();
  dst_init();
  usedef_init();
  g_ctx_init();

  /*fp1 = fopen(filename1, "r");
  if (!fp1) {
    printf("Could not open harvest filename filename1 '%s'\n", filename1);
  }*/
  fp2 = fopen(filename2, "r");
  if (!fp2) {
    printf("Could not open harvest filename filename2 '%s'\n", filename2);
  }

  //ASSERT(fp1);
  ASSERT(fp2);

  //fdb1 = src_fingerprintdb_init();
  //ASSERT(fdb1);
  //DBG(TMP, "%s", "calling src_fingerprintdb_add_from_fp\n");
  //src_fingerprintdb_add_from_fp(fdb1, fp1, USE_TYPES_NO_FINGERPRINT, NULL, NULL, function_name);
  //DBG(TMP, "%s", "done calling src_fingerprintdb_add_from_fp\n");
  //fclose(fp1);

  DBG(TMP, "%s", "calling dst_fingerprintdb_add_from_fp\n");
  fdb2 = dst_fingerprintdb_init();
  ASSERT(fdb2);
  DBG(TMP, "%s", "done calling dst_fingerprintdb_add_from_fp\n");
  cmn_fingerprintdb_add_from_fp(fdb2, fp2, USE_TYPES_NO_FINGERPRINT, NULL, NULL, function_name);
  fclose(fp2);

  if (!function_per_file && file_exists(ofile)) {
    unlink(ofile);
  }
  if (function_per_file) {
    int rc;

    rmrf(ofile);
    unlink(ofile);
    rc = mkdir(ofile, 0777);
    if (rc != 0) {
      perror("mkdir");
    }
    ASSERT(rc == 0);
  }
  //DBG(TMP, "H2P: filename1 %s, filename2 %s\n", filename1, filename2);
  DBG(TMP, "H2P: filename %s\n", filename2);
  //symbol_set_init(&symbol_set1);
  symbol_set_init(&symbol_set2);
  //get_symbol_set_from_logfile(&symbol_set1, logfile1);
  get_symbol_set_from_logfile(&symbol_set2, logfile2);
  //get_rodata_ptrs_from_logfile(&rodata_ptr_set, logfile1);
  //get_rodata_ptrs_from_logfile(&rodata_ptr_set, logfile2);
  //nonvoid_funs = read_nonvoid_file(nonvoid_file);
  generate_peeptabs_from_fdb_and_symbol_set(ofile, function_per_file, fdb2, &symbol_set2);

  //free_nonvoid_funs(nonvoid_funs);
  //symbol_set_free(&symbol_set1);
  symbol_set_free(&symbol_set2);
  //src_fingerprintdb_free(fdb1);
  dst_fingerprintdb_free(fdb2);
  return 0;
}
