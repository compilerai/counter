#include <stdio.h>
#include <stdlib.h>
#include <yices.h>
#include "yices_stub.h"
#include "support/debug.h"

//yices src includes
#include "parser_utils/lexer.h"
#include "parser_utils/term_stack2.h"
#include "parser_utils/parser.h"
#include "frontend/smt2/smt2_parser.h"

static void
yices_out_of_memory_callback(void)
{
  printf("YICES OUT OF MEMORY\n");
  exit(1);//YICES_OUT_OF_MEMORY;
}

void
yices_initialize(void)
{
  //printf("Using yices %s (%s)\n", yices_version, yices_build_arch);
  //yices_set_out_of_memory_callback(&yices_out_of_memory_callback);
}

bool
yices_test_smt2_expression(char const *filename)
{
  lexer_t lexer;
  tstack_t stack;
  parser_t parser;
  pvector_t trace_tags;

  yices_init();
  /*incremental = false;
  interactive = false;
  mcsat = false;
  show_stats = false;*/
  init_pvector(&trace_tags, 5);
  init_cmdline_parser(&parser, NULL, 0, NULL, 0);
  
  //printf("calling init_smt2_file_lexer\n");
  if (init_smt2_file_lexer(&lexer, filename) < 0) {
    NOT_IMPLEMENTED();
  }
  //printf("done calling init_smt2_file_lexer\n");
  init_smt2(true, false);
  init_smt2_tstack(&stack);
  init_parser(&parser, &lexer, &stack);
  init_parameter_name_table();
  parse_smt2_command_result = NULL;
  while (smt2_active()) {
    int32_t code;
    code = parse_smt2_command(&parser);
    //printf("code = %d, parse_smt2_command_result = %s.\n", (int)code, parse_smt2_command_result);
    if (code < 0) {
      break;
    }
  }
  //printf("exited smt2_active() loop\n");
  delete_parser(&parser);
  close_lexer(&lexer);
  delete_tstack(&stack);
  delete_smt2();
  delete_pvector(&trace_tags);

  yices_exit();

  if (!strcmp(parse_smt2_command_result, "sat")) {
    return true;
  } else if (!strcmp(parse_smt2_command_result, "unsat")) {
    return false;
  } else NOT_REACHED();
/*
  context *ctx = yices_new_context(NULL);
  term_t f = yices_parse_term(smt2_expr);
  code = yices_assert_formula(ctx, f);
  if (code < 0) {
    fprintf(stderr, "Assert failed: code = %"PRId32", error = %"PRId32"\n",
        code, yices_error_code());
    yices_print_error(stderr);
  }
  switch (yices_check_context(ctx, NULL)) {
    case STATUS_SAT:
      break;
    case STATUS_UNSAT:
      break;
    case STATUS_UNKNOWN:
      NOT_REACHED();
      break;
    case STATUS_IDLE:
    case STATUS_SEARCHING:
    case STATUS_INTERRUPTED:
    case STATUS_ERROR:
      fprintf(stderr, "%s(): error in check_context\n", __func__);
      yices_print_error(stderr);
      break;
  }
  yices_free_context(ctx);
*/
}

void
yices_done(void)
{
}
