#ifndef FUNCTION_POINTERS_H
#define FUNCTION_POINTERS_H

void *function_pointer_get(char const *name);
void function_pointer_register(char const *name, void *pointer);

#endif /* function_pointers.h */
