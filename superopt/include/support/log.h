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
//#define NOT_IMPLEMENTED() do { fprintf(stderr, "%s() %d: not-implemented\n", __func__, __LINE__); assert(false); } while (0)
//#define NOT_REACHED() do { fprintf(stderr, "%s() %d: not-reached\n", __func__, __LINE__); ASSERT(0); } while (0)
//#define ABORT() do { fprintf(stderr, "%s() %d: abort\n", __func__, __LINE__); ASSERT(0); } while (0)

// expensive assert, requires smt query etc
//#define Eassert_ENABLE
#ifdef Eassert_ENABLE
#define Eassert(c) do { assert(c); } while(false)
#else
#define Eassert(c) do { } while (false)
#endif

#endif //EQCHECKLOG_H
