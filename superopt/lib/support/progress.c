#include "support/progress.h"
#include "rewrite/translation_instance.h"

typedef struct func_node
{
  char func_name[128];
  target_ulong start, stop;
} func_node_t;

void init_progress_counters(translation_instance_t const *ti)
{
  int i;
  for (i = 0; i < ti->nb_tbs; i++)
  {
    tb_t *tb = &ti->tbs[i];
    func_node_t func_node;

    if (functions_find(tb->pc, &func_node))
    {
    }
    int j;
    for (j = 0; j < (tb->size>>2); j++)
    {
    }
  }
}
