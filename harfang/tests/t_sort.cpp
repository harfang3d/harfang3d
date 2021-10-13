// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/byte_sort.h"
#include "foundation/rand.h"
#include "gtest/gtest.h"

using namespace hg;

TEST(Sort, ByteSortFloat) {
	std::vector<float> values(65536);
	for (auto &v : values)
		v = FRand();

	std::vector<bytesort_entry<bytesort_float, int>> in(values.size()), out(values.size());

	for (size_t i = 0; i < values.size(); ++i) {
		in[i].v = to_bytesort_float(values[i]);
		in[i].o = int(i);
	}

	const auto res = bytesort<bytesort_float, int>(values.size(), in.data(), out.data());

	for (auto i = 1; i < values.size(); ++i)
		EXPECT_LE(values[res[i - 1].o], values[res[i].o]);
}
