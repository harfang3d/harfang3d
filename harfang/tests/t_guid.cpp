// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/format.h"
#include "foundation/guid.h"
#include "foundation/log.h"
#include "platform/platform.h"
#include "gtest/gtest.h"
#include <algorithm>

using namespace hg;

TEST(Guid, Compare) {
	auto u = MakeGuid(), v = u;
	EXPECT_EQ(u, v);
}

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

TEST(Guid, Generate100KUniqueGuid) {
	static size_t count = 100000; // tested up to 500M with no collision

	std::vector<Guid> guids;
	for (size_t n = 0; n < count; ++n)
		guids.push_back(MakeGuid());

	for (size_t n = 0; n < 30; ++n)
		log(ToString(guids[n]).c_str());
	log("...");

	std::sort(guids.begin(), guids.end(), &compare_guid);

	log("Sorting...");
	for (size_t n = 0; n < 30; ++n)
		log(ToString(guids[n]).c_str());
	log("...");

	size_t collision = 0;
	for (size_t n = 1; n < count; ++n)
		if (guids[n - 1] == guids[n])
			++collision;

	log(format("%1 collision(s)").arg(collision));

	EXPECT_EQ(0, collision);
}

TEST(Guid, ToFromString) {
	Guid u = MakeGuid();
	Guid v = MakeGuid(ToString(u));
	EXPECT_EQ(u, v);

	Guid w = MakeGuid(ToString(u, false));
	EXPECT_EQ(u, w);

	EXPECT_EQ(MakeGuid("6ED1A50D-C72F-4643-BDC4-2BD23689B861"), MakeGuid("6ed1a50d-c72f-4643-bdc4-2bd23689b861"));

	EXPECT_TRUE(IsValid(MakeGuid("6ED1A50D-C72F-4643-BDC4-2BD23689B861")));
	EXPECT_TRUE(IsValid(MakeGuid("6ed1a50d-c72f-4643-bdc4-2bd23689b861")));

	EXPECT_FALSE(IsValid(MakeGuid("that's not a guid")));
	EXPECT_FALSE(IsValid(MakeGuid("Xed1a50dc72f4643bdc42bd23689b861"))); // note the X
	EXPECT_FALSE(IsValid(MakeGuid("6ed1a50d-c72f-4643-bdc4+2bd23689b861"))); // note the last separator
}
