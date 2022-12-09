// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#define TEST_NO_MAIN
#include "acutest.h"

#include "foundation/format.h"
#include "foundation/guid.h"
#include "foundation/log.h"

#include <algorithm>
#include <vector>

using namespace hg;

static bool compare_guid(const Guid &a, const Guid &b) {
	for (int n = 0; n < 16; ++n) {
		auto v_a = a[n], v_b = b[n];

		if (v_a < v_b)
			return true;
		if (v_a > v_b)
			return false;
	}
	return false;
}

static void Generate100KUniqueGuid() {
	static size_t count = 100000; // tested up to 500M with no collision

	std::vector<Guid> guids;
	for (size_t n = 0; n < count; ++n)
		guids.push_back(MakeGuid());

	std::sort(guids.begin(), guids.end(), &compare_guid);

	size_t collision = 0;
	for (size_t n = 1; n < count; ++n)
		if (guids[n - 1] == guids[n])
			++collision;

	TEST_CHECK(collision == 0);
}

void test_guid() {
	{
		Guid u = MakeGuid();
		Guid v = u;
		TEST_CHECK(u == v);
	}

	{
		Guid u = MakeGuid();
		Guid v = MakeGuid(ToString(u));
		TEST_CHECK(u == v);

		Guid w = MakeGuid(ToString(u, false));
		TEST_CHECK(u == w);

		TEST_CHECK(MakeGuid("6ED1A50D-C72F-4643-BDC4-2BD23689B861") == MakeGuid("6ed1a50d-c72f-4643-bdc4-2bd23689b861"));

		TEST_CHECK(IsValid(MakeGuid("6ED1A50D-C72F-4643-BDC4-2BD23689B861")));
		TEST_CHECK(IsValid(MakeGuid("6ed1a50d-c72f-4643-bdc4-2bd23689b861")));

		TEST_CHECK(IsValid(MakeGuid("that's not a guid")) == false);
		TEST_CHECK(IsValid(MakeGuid("Xed1a50dc72f4643bdc42bd23689b861")) == false); // note the X
		TEST_CHECK(IsValid(MakeGuid("6ed1a50d-c72f-4643-bdc4+2bd23689b861")) == false); // note the last separator
	}

	Generate100KUniqueGuid();
}