#ifndef EQCHECKLOG_H
#define EQCHECKLOG_H

#include <ostream>
#include <cassert>

//#define LOG_ENABLE
#ifdef LOG_ENABLE
#define LOG(str) do { std::cout << str << std::flush; } while(false)
#else
#define LOG(str) do { } while (false)
#endif

//#define NOT_TESTED(x) do { if ((x)) fprintf(stderr, "%s() %d: not-tested: %s\n", __func__, __LINE__, #x); } while (0)
#define NOT_IMPLEMENTED() do { fprintf(stderr, "%s() %d: not-implemented\n", __func__, __LINE__); assert(false); } while (0)
//#define NOT_REACHED() do { fprintf(stderr, "%s() %d: not-reached\n", __func__, __LINE__); ASSERT(0); } while (0)
#define ABORT() do { fprintf(stderr, "%s() %d: abort\n", __func__, __LINE__); ASSERT(0); } while (0)

// expensive assert, requires smt query etc
//#define Eassert_ENABLE
#ifdef Eassert_ENABLE
#define Eassert(c) do { assert(c); } while(false)
#else
#define Eassert(c) do { } while (false)
#endif

//#define NDEBUG

#ifndef NDEBUG

#define ASSERT assert
//extern bool enable_debug;

//bool is_debug_type_enabled(const char* type_name);

//#define MY_DEBUG_WITH_TYPE(type, X) do { if(enable_debug && is_debug_type_enabled(type)) std::cout << X; } while(false)

//#define DEBUG(X) MY_DEBUG_WITH_TYPE(DEBUG_TYPE, X)

//#define DEBUG_HOUDINI(X) MY_DEBUG_WITH_TYPE("run-hodini", X)
//#define DEBUG_IMPLICATION(X) MY_DEBUG_WITH_TYPE("impli", X)
//#define DEBUG_EQUALITY(X) MY_DEBUG_WITH_TYPE("equality", X)
//#define DEBUG_GUESSING(X) MY_DEBUG_WITH_TYPE("guessing", X)

//#define INFO(X) MY_DEBUG_WITH_TYPE("info", X)
//#define DEBUG1(str) do { std::cout << str << std::flush; } while(false)

#else

#define NOP do { } while (false)
#define ASSERT(X) NOP
#define INFO(X) NOP
#define DEBUG(X) NOP
#define MY_DEBUG_WITH_TYPE(type, X) NOP
#define DEBUG_HOUDINI(X) NOP
#define DEBUG_IMPLICATION(X) NOP
#define DEBUG_EQUALITY(X) NOP
#define DEBUG_GUESSING(X) NOP
#define DEBUG1(X) NOP

#endif

#endif //EQCHECKLOG_H
