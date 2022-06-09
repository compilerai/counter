#include "support/debug.h"
#include "support/types.h"
#include "support/mytimer.h"

namespace cspace {
extern "C" {
//#include "rewrite/locals_info.h"
#include "valtag/imm_vt_map.h"
#include "valtag/regmap.h"
}
}

#include "expr/expr.h"
#include "expr/consts_struct.h"
#include "expr/expr_utils.h"
#include "expr/solver_interface.h"
#include "expr/sp_version.h"

static char as1[40960];

typedef struct _locals_info_t {
//functions
  _locals_info_t(struct context *ctx) : m_ctx(ctx), m_index2info() {
    //m_ctx = ctx;
    //m_index2info.clear();
  }

  /*_locals_info_t(struct _locals_info_t const *li) {
    m_ctx = li->m_ctx;
    m_index2info = li->m_index2info;
  }*/

  ~_locals_info_t() {
    /*if (m_private_context) {
      //delete m_ctx;
    }*/
  }

//members
  context *m_ctx;
  //bool m_private_context;
  map<unsigned, pair<string, pair<size_t, expr_ref>>> m_index2info; //map from dwarf_map_index to string
} _locals_info_t;

static expr_ref
fscanf_expr(FILE *fp, context *ctx)
{
  static char line[40960];
  string expr_str = "";
  size_t ret;

  while ((ret = fscanf(fp, "%[^\n]%*c", line)) == 1) {
    //cout << "line: " << line << endl;
    expr_str = expr_str + "\n" + line;
  }
  int status = fscanf(fp, "\n");
  ASSERT(status != -1);
  //cout << "expr_str = " << expr_str << endl;
  return parse_expr(ctx, expr_str.c_str()).first;
}

void
locals_info_add(cspace::locals_info_t *_li, unsigned dwarf_map_index, string name, size_t size, expr_ref expr)
{
  DBG(LOCALS_INFO, "locals_info_add(): %u : %s : %s\n", dwarf_map_index, name.c_str(), expr_string(expr).c_str());
  _locals_info_t *li = (_locals_info_t *)_li;
  li->m_index2info[dwarf_map_index] = make_pair(name, make_pair(size, expr));
}

void
locals_info_clear(cspace::locals_info_t *_li)
{
  DBG(LOCALS_INFO, "%s", "locals_info_clear()\n");
  _locals_info_t *li = (_locals_info_t *)_li;
  li->m_index2info.clear();
}

struct cspace::locals_info_t *
locals_info_new(struct context *ctx)
{
  _locals_info_t *ret;
  ret = new _locals_info_t(ctx);
  return (cspace::locals_info_t *)ret;
}

extern "C" struct cspace::locals_info_t *
locals_info_new_using_g_ctx()
{
  ASSERT(g_ctx);
  return locals_info_new(g_ctx);
}



extern "C" struct cspace::locals_info_t *
locals_info_copy(struct cspace::locals_info_t const *_li)
{
  _locals_info_t *ret, *li;
  li = (_locals_info_t *)_li;
  ret = new _locals_info_t(li->m_ctx);
  ASSERT(ret);
  ret->m_index2info = li->m_index2info;
  //DBG(TMP, "ret = %p\n", ret);
  return (cspace::locals_info_t *)ret;
}



/*struct cspace::locals_info_t *
locals_info_new_using_context(struct context *ctx)
{
  _locals_info_t *ret;
  ret = new _locals_info_t(ctx);
  return (cspace::locals_info_t *)ret;
}*/

extern "C" void
locals_info_delete(struct cspace::locals_info_t *_li)
{
  _locals_info_t *li = (_locals_info_t *)_li;
  //DBG(TMP, "_li = %p\n", _li);
  locals_info_clear(_li);
  delete li;
}

extern "C" size_t
cspace::locals_info_to_string(struct cspace::locals_info_t const *_li, char *buf, size_t size)
{
  _locals_info_t const *li = (_locals_info_t const *)_li;
  char *ptr, *end;
  ptr = buf;
  end = buf + size;
  *ptr = '\0';
  if (!li) {
    return ptr - buf;
  }
  for (map<unsigned, pair<string, pair<size_t, expr_ref>>>::const_iterator iter = li->m_index2info.begin();
       iter != li->m_index2info.end();
       iter++) {
    eqspace::context *ctx = iter->second.second.second->get_context();
    ptr += snprintf(ptr, end - ptr, "LOCAL%u : %s : %zd\n%s\n\n", (*iter).first, (*iter).second.first.c_str(), iter->second.second.first, ctx->expr_to_string_table((*iter).second.second.second).c_str());
    ASSERT(ptr < end);
  }
  return ptr - buf;
}

extern "C" void
cspace::fscanf_locals_info(FILE *fp, struct cspace::locals_info_t *_li)
{
  unsigned dwarf_map_index;
  _locals_info_t *li;
  size_t ret, size;

  li = (_locals_info_t *)_li;
  while ((ret = fscanf(fp, "LOCAL%u : %[^:] : %zd\n", &dwarf_map_index, as1, &size)) == 3) {
    expr_ref expr = fscanf_expr(fp, li->m_ctx);
    li->m_index2info[dwarf_map_index] = make_pair(string(as1), make_pair(size, expr));
  }
  //ASSERT(ret == EOF);
}

extern "C" bool
cspace::locals_info_equal(struct cspace::locals_info_t const *_a, struct cspace::locals_info_t const *_b)
{
  if (_a == NULL) {
    return _a == _b;
  }

  struct _locals_info_t const *a = (struct _locals_info_t const *)_a;
  struct _locals_info_t const *b = (struct _locals_info_t const *)_b;

  for (map<unsigned, pair<string, pair<size_t, expr_ref>>>::const_iterator iter = a->m_index2info.begin();
       iter != a->m_index2info.end();
       iter++) {
    unsigned index = (*iter).first;
    string name = (*iter).second.first;
    size_t size = (*iter).second.second.first;
    expr_ref expr = (*iter).second.second.second;
    if (b->m_index2info.find(index) == b->m_index2info.end()) {
      return false;
    }
    if (b->m_index2info.at(index).first != name) {
      return false;
    }
    if (b->m_index2info.at(index).second.first != size) {
      return false;
    }
    if (b->m_index2info.at(index).second.second != expr) {
      return false;
    }
  }
  return true;
}

extern "C" bool
imm_vt_map_rename_locals_using_locals_info(cspace::imm_vt_map_t *imm_vt_map,
    cspace::locals_info_t const *_li1, cspace::locals_info_t const *_li2)
{
  _locals_info_t const *li1 = (_locals_info_t *)_li1;
  _locals_info_t const *li2 = (_locals_info_t *)_li2;
  for (long i = 0; i < imm_vt_map->num_imms_used; i++) {
    ASSERT(imm_vt_map->table[i].tag == cspace::tag_imm_local);
    //long lnum1 = imm_vt_map->table[i].val;
    //ASSERT(li1->m_index2info.find(lnum1) != li1->m_index2info.end());
    //string name1 = li1->m_index2info.at(lnum1);
    ASSERT(li1->m_index2info.find(i) != li1->m_index2info.end());
    string name1 = li1->m_index2info.at(i).first;
    expr_ref expr1 = li1->m_index2info.at(i).second.second;
    long lnum2 = -1;
    for (map<unsigned, pair<string, pair<size_t, expr_ref>>>::const_iterator iter = li2->m_index2info.begin();
         iter != li2->m_index2info.end();
         iter++) {
      if ((*iter).second.first == name1) {
        lnum2 = (*iter).first;
        break;
      }
    }
    imm_vt_map->table[i].val = lnum2;
  }
  for (map<unsigned, pair<string, pair<size_t, expr_ref>>>::const_iterator iter = li2->m_index2info.begin();
       iter != li2->m_index2info.end();
       iter++) {
    long lnum2 = (*iter).first;
    bool found = false;
    for (long i = 0; i < imm_vt_map->num_imms_used; i++) {
      if (imm_vt_map->table[i].val == lnum2) {
        found = true;
        break;
      }
    }
    if (!found) {
      ASSERT(imm_vt_map->num_imms_used < NUM_CANON_CONSTANTS);
      imm_vt_map->table[imm_vt_map->num_imms_used].val = lnum2;
      imm_vt_map->table[imm_vt_map->num_imms_used].tag = cspace::tag_imm_local;
      imm_vt_map->num_imms_used++;
    }
  }
  return true;
}

extern "C" void
locals_info_rename_local_nums_using_imm_vt_map(struct cspace::locals_info_t *_li, struct cspace::imm_vt_map_t const *locals_map)
{
  _locals_info_t *li = (_locals_info_t *)_li;
  map<unsigned, pair<string, pair<size_t, expr_ref>>> retmap;
  for (map<unsigned, pair<string, pair<size_t, expr_ref>>>::const_iterator iter = li->m_index2info.begin();
       iter != li->m_index2info.end();
       iter++) {
    for (long i = 0; i < locals_map->num_imms_used; i++) {
      if (locals_map->table[i].val == (*iter).first) {
        retmap[i] = (*iter).second;
      }
    }
  }
  li->m_index2info = retmap;
}

bool
locals_info_get_expr(expr_ref &e, struct cspace::locals_info_t const *_li, unsigned dwarf_map_index)
{
  _locals_info_t *li = (_locals_info_t *)_li;
  if (li->m_index2info.count(dwarf_map_index) == 0) {
    return false;
  }
  e = li->m_index2info[dwarf_map_index].second.second;
  return true;
}

expr_ref
expr_rename_using_regmap(expr_ref e, cspace::regmap_t const *regmap, eqspace::consts_struct_t const &cs)
{
  //XXX: NOT_IMPLEMENTED();
  map<unsigned, pair<expr_ref, expr_ref> > submap;
  context *ctx = e->get_context();
  expr_ref iesp = ctx->mk_var("input.reg.0.4", ctx->mk_bv_sort(DWORD_LEN));
  //expr_ref oesp = ctx->mk_var(input_esp_reg_keyword, ctx->mk_bv_sort(DWORD_LEN));
  //expr_ref oesp = cs.get_expr_value(reg_type_input_stack_pointer_const, -1);
  expr_ref oesp = get_sp_version_at_entry(ctx);
  submap[iesp->get_id()] = make_pair(iesp, oesp);
  return ctx->expr_substitute(e, submap);
}

extern "C" void
locals_info_rename_exprs_using_regmap(struct cspace::locals_info_t *_li, struct cspace::regmap_t const *regmap)
{
  _locals_info_t *li = (_locals_info_t *)_li;
  for (map<unsigned, pair<string, pair<size_t, expr_ref>>>::iterator iter = li->m_index2info.begin();
       iter != li->m_index2info.end();
       iter++) {
    iter->second.second.second = expr_rename_using_regmap(iter->second.second.second, regmap, *g_consts_struct);
  }
}
