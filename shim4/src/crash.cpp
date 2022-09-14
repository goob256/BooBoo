// Portions of this file are (c) David Capello. Please see LICENSE.txt for details.

#include "shim4/crash.h"
#include "shim4/util.h"

#include "shim4/internal/util.h"

using namespace noo;

#ifdef _WIN32
#include <dbghelp.h>
#elif defined __linux__ && !defined ANDROID
#include <execinfo.h>
#include <signal.h>
#endif

namespace noo {

namespace util {

#ifdef _WIN32
static LONG WINAPI unhandled_exception(_EXCEPTION_POINTERS *exception_pointers)
{
	errormsg("Generating crash dump...\n");

	std::string appdata = get_appdata_dir();
	appdata += "/crashdump.dmp";

	HANDLE handle = ::CreateFile(
		appdata.c_str(),
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if (handle != INVALID_HANDLE_VALUE) {
		MINIDUMP_EXCEPTION_INFORMATION ei;
		ei.ThreadId = GetCurrentThreadId();
		ei.ExceptionPointers = exception_pointers;
		ei.ClientPointers = FALSE;

		MiniDumpWriteDump(
			GetCurrentProcess(),
			GetCurrentProcessId(),
			handle,
			MiniDumpNormal,
			(exception_pointers ? &ei: NULL),
			NULL,
			NULL
		);

		CloseHandle(handle);

		errormsg("Crash dump written to " + appdata + ".\n");
	}

	internal::close_log_file();

	return EXCEPTION_EXECUTE_HANDLER;
}
#endif

#if defined __linux__ && !defined ANDROID
static void unhandled_exception(int signum)
{
	errormsg("Generating crash dump...\n");
	
	std::string appdata = get_appdata_dir();
	appdata += "/crashdump.txt";

	FILE *f = fopen(appdata.c_str(), "w");
	if (f == NULL) {
		errormsg("Couldn't open %s to write crash dump!\n", appdata.c_str());
		exit(1);
	}

	const int num = 1000;
	void *buffer[num];
	int filled = backtrace(buffer, num);
	char **strings = backtrace_symbols(buffer, filled);

	for (int i = 0; i < filled; i++) {
		fprintf(f, "%s\n", strings[i]);
	}

	fclose(f);
	
	internal::close_log_file();
	
	exit(1);
}
#endif

void start_crashdumps()
{
#ifdef _WIN32
	SetUnhandledExceptionFilter(unhandled_exception);
#elif defined __linux__ && !defined ANDROID
	signal(SIGSEGV, unhandled_exception);
	signal(SIGABRT, unhandled_exception);
#endif
}

void end_crashdumps()
{
#ifdef _WIN32
	SetUnhandledExceptionFilter(0);
#endif
}

} // End namespace util

} // End namespace noo
