#ifndef __PROGRESS_H
#define __PROGRESS_H

struct translation_instance_t;

void init_progress_counters (struct translation_instance_t const *ti);
void register_progress (unsigned long pc);

#endif
