#include "nextpc_function_name_map.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "support/c_utils.h"
#include "support/debug.h"
#include <iostream>

using namespace std;
static char as1[4096];

void
nextpc_function_name_map_init(nextpc_function_name_map_t *map)
{
  map->num_nextpcs = 0;
}

void
nextpc_function_name_map_from_string(nextpc_function_name_map_t *map, char const *buf)
{
  char const *ptr, *remaining, *endline;
  int nextpc_num, i;
  char tmpbuf[512];

  //cout << __func__ << " " << __LINE__ << ": buf =\n" << buf << endl;
  ptr = buf;
  i = 0;
  map->num_nextpcs = 0;
  while (strstart(ptr, "C_NEXTPC", &remaining)) {
    char const *colon;
    colon = strchr(ptr, ':');
    ASSERT(colon);
    ASSERT(colon - remaining < sizeof tmpbuf);
    memcpy(tmpbuf, remaining, colon- remaining);
    ASSERT(tmpbuf[colon - remaining - 1] == ' ');
    tmpbuf[colon - remaining - 1] = '\0';
    //cout << __func__ << " " << __LINE__ << ": i = " << i << ", tmpbuf = " << tmpbuf << endl;
    nextpc_num = strtol(tmpbuf, nullptr, 0);
    ASSERT(nextpc_num == i);
    ASSERT(colon[1] == ' ');
    endline = strchr(colon, '\n');
    ASSERT(endline - colon <= sizeof map->table[i].name);
    memcpy(map->table[i].name, colon + 1, endline - colon - 1);
    map->table[i].name[endline - colon - 1] = '\0';
    map->num_nextpcs++;
    ptr = endline + 1;
    i++;
  }
  //cout << __func__ << " " << __LINE__ << ": map =\n" << nextpc_function_name_map_to_string(map, as1, sizeof as1) << endl;
}

char *
nextpc_function_name_map_to_string(nextpc_function_name_map_t const *map, char *buf, size_t size)
{
  char *ptr, *end;
  int i;

  ptr = buf;
  end = buf + size;
  *ptr = '\0';
  for (i = 0; i < map->num_nextpcs; i++) {
    if (map->table[i].name[0] == '\0') {
      continue;
    }
    ptr += snprintf(ptr, end - ptr, "C_NEXTPC%d : %s\n", i, map->table[i].name);
    ASSERT(ptr < end);
  }
  return buf;
}

void
nextpc_function_name_map_copy(nextpc_function_name_map_t *dst, nextpc_function_name_map_t const *src)
{
  memcpy(dst, src, sizeof(nextpc_function_name_map_t));
}

//void
//nextpc_function_name_map_from_peep_comment(nextpc_function_name_map_t *map, char const *comment)
//{
//  char *remaining;
//  char const *ptr = comment;
//
//  nextpc_function_name_map_init(map);
//  while ((ptr = strstr(ptr, "NEXTPC")) != NULL) {
//    long nextpc_num;
//    ptr += strlen("NEXTPC");
//    nextpc_num = strtol(ptr, &remaining, 0);
//    ASSERT(nextpc_num >= 0);
//    ASSERT(*remaining == ' ');
//    remaining++;
//    char const *comma = strchr(remaining, ',');
//    ASSERT(comma != NULL);
//    if (comma - remaining >= MAX_FUNCTION_NAME_LEN) {
//      comma = remaining + (MAX_FUNCTION_NAME_LEN - 1);
//    }
//    ASSERT(comma - remaining >= 0);
//    ASSERT(comma - remaining < MAX_FUNCTION_NAME_LEN - 1);
//    memcpy(map->table[map->num_nextpcs].name, remaining, comma - remaining);
//    map->table[map->num_nextpcs].name[comma - remaining] = '\0';
//    map->num_nextpcs++;
//  }
//}

static bool
nextpc_names_match(char const *truncated, char const *full, size_t truncate_size)
{
  return   !strcmp(full, truncated)
        || (   (strlen(truncated) == (truncate_size - 1))
            && !memcmp(full, truncated, strlen(truncated)));
}

int
nextpc_function_name_map_find_function_name(nextpc_function_name_map_t const *map, char const *function_name)
{
  for (int i = 0; i < map->num_nextpcs; i++) {
    CPP_DBG_EXEC(NEXTPC_MAP, printf("function_name = %s\n", function_name));
    CPP_DBG_EXEC(NEXTPC_MAP, printf("map->table[%d].name = %s\n", i, map->table[i].name));
    if (   nextpc_names_match(map->table[i].name, function_name, sizeof map->table[i].name)
        || (   map->table[i].name[0] == '_'
            && map->table[i].name[1] == '_'
            && nextpc_names_match(&map->table[i].name[2], function_name, (sizeof map->table[i].name) - 2))
        /*|| (   strstart(map->table[i].name, "__ctype_", &remaining) //XXX: need to handle toupper vs. __ctype_toupper_loc. testcase: perlbmk/Perl_pp_ucfirst
            && strstop(map->table[i].name, "_loc", &remaining)
            && nextpc_names_match(remaining, function_name, ))*/) {
      CPP_DBG_EXEC(NEXTPC_MAP, cout << __func__ << ':' << __LINE__ << ": returning " << i << endl);
      return i;
    }
  }
  CPP_DBG_EXEC(NEXTPC_MAP, cout <<  __func__ << ':' << __LINE__ << ": returning -1\n");
  return -1;
}

int
nextpc_function_name_map_add(nextpc_function_name_map_t *map, char const *function_name)
{
  int ret;
  ret = nextpc_function_name_map_find_function_name(map, function_name);
  if (ret != -1) {
    return ret;
  }
  ASSERT(map->num_nextpcs < NEXTPC_FUNCTION_NAME_MAP_MAX_NUM_NEXTPCS);
  int i = map->num_nextpcs;
  strncpy(map->table[i].name, function_name, sizeof map->table[i].name);
  map->table[i].name[(sizeof map->table[i].name) - 1] = '\0';
  map->num_nextpcs++;
  return i;
}
