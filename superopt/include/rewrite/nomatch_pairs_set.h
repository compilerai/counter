#ifndef NOMATCH_PAIRS_SET_H
#define NOMATCH_PAIRS_SET_H
#include "rewrite/nomatch_pairs.h"

struct nomatch_pairs_set_t
{
  nomatch_pairs_t *nomatch_pairs;
  long num_nomatch_pairs;
  nomatch_pairs_set_t() : nomatch_pairs(NULL), num_nomatch_pairs(0) {}
  nomatch_pairs_set_t(std::vector<nomatch_pairs_t> const &v)
  {
    num_nomatch_pairs = v.size();
    nomatch_pairs = new nomatch_pairs_t[num_nomatch_pairs];
    for (size_t i = 0; i < num_nomatch_pairs; i++) {
      nomatch_pair_copy(&nomatch_pairs[i], &v.at(i));
    }
  }
  void operator=(nomatch_pairs_set_t const &other)
  {
    this->free();
    this->nomatch_pairs = new nomatch_pairs_t[other.num_nomatch_pairs];
    this->num_nomatch_pairs = other.num_nomatch_pairs;
    for (long i = 0; i < other.num_nomatch_pairs; i++) {
      nomatch_pair_copy(&this->nomatch_pairs[i], &other.nomatch_pairs[i]);
    }
  }
  bool equals(nomatch_pairs_set_t const &other) const
  {
    if (this->num_nomatch_pairs != other.num_nomatch_pairs) {
      return false;
    }
    for (long i = 0; i < this->num_nomatch_pairs; i++) {
      if (!nomatch_pair_equals(&this->nomatch_pairs[i], &other.nomatch_pairs[i])) {
        return false;
      }
    }
    return true;
  }
  bool belongs(exreg_group_id_t group1, reg_identifier_t const &reg1, exreg_group_id_t group2, reg_identifier_t const &reg2) const
  {
    for (long i = 0; i < num_nomatch_pairs; i++) {
      if (nomatch_pair_belongs(&nomatch_pairs[i], group1, reg1, group2, reg2)) {
        return true;
      }
    }
    return false;
  }
  void free()
  {
    if (nomatch_pairs) {
      delete [] nomatch_pairs;
      nomatch_pairs = NULL;
      num_nomatch_pairs = 0;
    }
    ASSERT(!nomatch_pairs);
    ASSERT(num_nomatch_pairs == 0);
  }
};

char *nomatch_pairs_set_to_string(nomatch_pairs_set_t const &nomatch_pairs_set, char *buf, size_t size);
void nomatch_pairs_set_copy_using_regmap(nomatch_pairs_set_t *out, nomatch_pairs_set_t const  *in, regmap_t const *regmap);

#endif
