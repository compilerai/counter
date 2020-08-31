#include "log.h"
#include <string>
#include <list>
#include <iostream>

#define DEBUG_TYPE "log"

bool enable_debug = true;

static std::list<std::string> enabled_debug_types =
{
  "info",
//  "eq",
//  "correlate",
  "guessing",
//  "tfg",
//  "correlate-edge-details",
//  "corr_graph",
  "impli",
//  "equality",
  "run-hodini",
//  "guessers",
//  "expr",
//  "expr_simplify",
//  "relation",
//  "cp",
//  "parse-eq",
};

bool is_debug_type_enabled(const char* type_name)
{
  for(auto dt : enabled_debug_types)
  {
    if(type_name == dt)
    {
      return true;
    }
  }

  return false;
}

