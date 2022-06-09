#pragma once

typedef enum tfg_alias_result_t {
  TfgMayAlias = 0,
  TfgMustNotAlias,
  TfgMustAlias,
  TfgPartialAlias
} tfg_alias_result_t;
