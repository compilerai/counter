#include <execinfo.h>  // for backtrace
#include <dlfcn.h>     // for dladdr
#include <cxxabi.h>    // for __cxa_demangle

#include <string>
#include <sstream>
#include "support/stacktrace.h"

// This function produces a stack backtrace with demangled function & method names.
std::string backtrace(int skip)
{
	void *callstack[128];
	const int nMaxFrames = sizeof(callstack) / sizeof(callstack[0]);
	char buf[1024];
	int nFrames = backtrace(callstack, nMaxFrames);
	char **symbols = backtrace_symbols(callstack, nFrames);

	std::ostringstream trace_buf;
	for (int i = skip; i < nFrames; i++) {
		Dl_info info;
		if (dladdr(callstack[i], &info)) {
			char *demangled = NULL;
			int status;
			demangled = abi::__cxa_demangle(info.dli_sname, NULL, 0, &status);
			snprintf(buf, sizeof(buf), "%-3d %*p %s + %zd\n",
					 i, 2 + sizeof(void*) * 2, callstack[i],
					 status == 0 ? demangled : info.dli_sname,
					 (char *)callstack[i] - (char *)info.dli_saddr);
			free(demangled);
		} else {
			snprintf(buf, sizeof(buf), "%-3d %*p\n",
					 i, 2 + sizeof(void*) * 2, callstack[i]);
		}
		trace_buf << buf;

		snprintf(buf, sizeof(buf), "%s\n", symbols[i]);
		trace_buf << buf;
	}
	free(symbols);
	if (nFrames == nMaxFrames)
		trace_buf << "[truncated]\n";
	return trace_buf.str();
}
