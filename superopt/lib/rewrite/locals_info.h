#pragma once

struct imm_vt_map_t;
struct locals_info_t;
struct context;
struct regmap_t;

struct locals_info_t *locals_info_new_using_g_ctx();
struct locals_info_t *locals_info_new(struct context *ctx);
struct locals_info_t *locals_info_copy(struct locals_info_t const *li);
void locals_info_delete(struct locals_info_t *);
size_t locals_info_to_string(struct locals_info_t const *li, char *buf, size_t size);
void fscanf_locals_info(FILE *fp, struct locals_info_t *li);
bool locals_info_equal(struct locals_info_t const *a, struct locals_info_t const *b);
bool imm_vt_map_rename_locals_using_locals_info(struct imm_vt_map_t *imm_vt_map,
    struct locals_info_t const *li1, struct locals_info_t const *li2);
void locals_info_rename_local_nums_using_imm_vt_map(struct locals_info_t *li, struct imm_vt_map_t const *locals_map);
void locals_info_rename_exprs_using_regmap(struct locals_info_t *li, struct regmap_t const *regmap);
