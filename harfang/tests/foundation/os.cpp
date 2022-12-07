// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#include <math.h>

#define TEST_NO_MAIN
#include "acutest.h"

#include "foundation/os.h"

using namespace hg;

void test_os() {
#if WIN32
	SetLastError(ERROR_FILE_NOT_FOUND);
	TEST_CHECK(OSGetLastError() == "The system cannot find the file specified.\r\n");
#else
	errno = ENOENT;
	TEST_CHECK(OSGetLastError() == "No such file or directory");
#endif
}