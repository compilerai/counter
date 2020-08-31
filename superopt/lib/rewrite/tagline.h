#ifndef TAGLINE_H
#define TAGLINE_H

long count_tags(char const *tagline);
size_t taglines_union(char *a, char const *b, size_t asize);
void pick_random_tag(char *buf, size_t size, char const *tagline);

#endif /* tagline.h */
