#pragma once

#ifdef __cplusplus
#include <string>
#include <iostream>

#endif

extern bool dyn_debug_flag;

void disable_dyn_debug();
void enable_dyn_debug();

#ifdef __cplusplus
void init_dyn_debug_from_string(std::string const& s);
std::string get_dyn_debug_cmdline_args();
#endif

#ifdef __cplusplus
extern "C"
#endif
void print_debug_class_levels();

#ifdef __cplusplus
extern "C"
#endif
int get_debug_class_level(const char* dbgclass);

#ifdef __cplusplus
extern "C"
#endif
int get_debug_class_prefix_level(const char* dbgclass);

#ifdef __cplusplus
extern "C"
#endif
void set_debug_class_level(const char* dbgclass, int lvl);

#ifdef __cplusplus
extern "C"
#endif
int elevate_debug_class_level(const char* dbgclass, int lvl);

#ifndef NDEBUG

#define DYN_DBG_LVL(LVL, DBGCLASS, ...)                                             \
  do {                                                                              \
    if (dyn_debug_flag && get_debug_class_prefix_level(DBGCLASS) >= LVL) {          \
      /*std::cout << DBGCLASS << " " << __FILE__ << " " << __func__ << " " << __LINE__ << ":\n"; */\
      printf(__VA_ARGS__);                                                          \
    }                                                                               \
  } while (0)

#define DYN_DBGS(DBGCLASS, ...)  DYN_DBG_LVL(1, DBGCLASS, __VA_ARGS__)
#define DYN_DBGS2(DBGCLASS, ...)  DYN_DBG_LVL(2, DBGCLASS, __VA_ARGS__)
#define DYN_DBGS3(DBGCLASS, ...)  DYN_DBG_LVL(3, DBGCLASS, __VA_ARGS__)
#define DYN_DBGS4(DBGCLASS, ...)  DYN_DBG_LVL(4, DBGCLASS, __VA_ARGS__)

#define DYN_DBG(DBGCLASS, ...)  DYN_DBGS(#DBGCLASS, __VA_ARGS__)
#define DYN_DBG2(DBGCLASS, ...)  DYN_DBGS2(#DBGCLASS, __VA_ARGS__)
#define DYN_DBG3(DBGCLASS, ...)  DYN_DBGS3(#DBGCLASS, __VA_ARGS__)
#define DYN_DBG4(DBGCLASS, ...)  DYN_DBGS4(#DBGCLASS, __VA_ARGS__)


#define DYN_DEBUG_LVL(LVL, MUTE, DBGCLASS, X)                            \
  do {                                                                   \
    /*cout << __func__ << " " << __LINE__ << ": dyn_debug_flag = " << dyn_debug_flag << endl; */\
    if (dyn_debug_flag && get_debug_class_prefix_level(DBGCLASS) >= LVL) { \
      if (!(MUTE)) { std::cout << DBGCLASS << " " << __FILE__ << " " << __func__ << " " << __LINE__ << ":\n"; } \
      g_expect_no_deterministic_malloc_calls = true; \
      X; \
      g_expect_no_deterministic_malloc_calls = false; \
      fflush(stdout); \
    } \
  } while (false)

#define DYN_DEBUGS(DBGCLASS, X)  DYN_DEBUG_LVL(1, 0, DBGCLASS, X)
#define DYN_DEBUGS2(DBGCLASS, X) DYN_DEBUG_LVL(2, 0, DBGCLASS, X)
#define DYN_DEBUGS3(DBGCLASS, X) DYN_DEBUG_LVL(3, 0, DBGCLASS, X)
#define DYN_DEBUGS4(DBGCLASS, X) DYN_DEBUG_LVL(4, 0, DBGCLASS, X)

#define DYN_DEBUGS_MUTE(DBGCLASS, X)  DYN_DEBUG_LVL(1, 1, DBGCLASS, X)
#define DYN_DEBUGS2_MUTE(DBGCLASS, X) DYN_DEBUG_LVL(2, 1, DBGCLASS, X)
#define DYN_DEBUGS3_MUTE(DBGCLASS, X) DYN_DEBUG_LVL(3, 1, DBGCLASS, X)
#define DYN_DEBUGS4_MUTE(DBGCLASS, X) DYN_DEBUG_LVL(4, 1, DBGCLASS, X)

#define DYN_DEBUG(DBGCLASS, X)  DYN_DEBUGS(#DBGCLASS, X)
#define DYN_DEBUG2(DBGCLASS, X) DYN_DEBUGS2(#DBGCLASS, X)
#define DYN_DEBUG3(DBGCLASS, X) DYN_DEBUGS3(#DBGCLASS, X)
#define DYN_DEBUG4(DBGCLASS, X) DYN_DEBUGS4(#DBGCLASS, X)

#define DYN_DEBUG_MUTE(DBGCLASS, X)  DYN_DEBUGS_MUTE(#DBGCLASS, X)
#define DYN_DEBUG_MUTE2(DBGCLASS, X) DYN_DEBUGS2_MUTE(#DBGCLASS, X)
#define DYN_DEBUG_MUTE3(DBGCLASS, X) DYN_DEBUGS3_MUTE(#DBGCLASS, X)
#define DYN_DEBUG_MUTE4(DBGCLASS, X) DYN_DEBUGS4_MUTE(#DBGCLASS, X)

#define DYN_DBG_SET(l,i) set_debug_class_level(#l,i)
#define DYN_DBG_ELEVATE(l,i) elevate_debug_class_level(#l,i)


#define _FNLN_ __func__ << ' ' << __LINE__

#else

#define DYN_DEBUG(DBGCLASS, X)  ;
#define DYN_DEBUG2(DBGCLASS, X) ;
#define DYN_DEBUG3(DBGCLASS, X) ;
#define DYN_DEBUG4(DBGCLASS, X) ;

#endif
