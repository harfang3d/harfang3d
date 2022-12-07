// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#include <math.h>

#define TEST_NO_MAIN
#include "acutest.h"

#include "foundation/rand.h"

#include "foundation/math.h"

using namespace hg;

void test_rand() {
	Seed(0);

	uint32_t u0, u1;

	TEST_CHECK(Rand(0) == 0);

	u0 = Rand();
	TEST_CHECK(u0 <= RAND_MAX);
	for (int i = 0; i < 256; i++, u0 = u1) {
		u1 = Rand();
		TEST_CHECK(u1 <= RAND_MAX);
		TEST_CHECK(u0 != u1);
	}

	u0 = Rand(32);
	TEST_CHECK(u0 <= 32);

	u0 = Rand(10000);
	TEST_CHECK(u0 <= 10000);

	float f0, f1;

	f0 = FRand();
	TEST_CHECK((f0 >= 0.f) && TEST_CHECK(f0 <= 1.f));
	for (int i = 0; i < 256; i++, f0 = f1) {
		f1 = FRand();
		TEST_CHECK((f1 >= 0.f) && TEST_CHECK(f1 <= 1.f));
		TEST_CHECK(f0 != f1);
	}

	f0 = FRand(0.5f);
	TEST_CHECK((f0 >= 0.f) && TEST_CHECK(f0 <= 0.5f));

	f0 = FRand(200.125f);
	TEST_CHECK((f0 >= 0.f) && TEST_CHECK(f0 <= 200.125f));

	f0 = FRRand();
	TEST_CHECK((f0 >= -1.f) && TEST_CHECK(f0 <= 1.f));
	for (int i = 0; i < 256; i++, f0 = f1) {
		f1 = FRRand();
		TEST_CHECK((f1 >= -1.f) && TEST_CHECK(f1 <= 1.f));
		TEST_CHECK(f0 != f1);
	}

	f0 = FRRand(-10.f, 10.f);
	TEST_CHECK((f0 >= -10.f) && TEST_CHECK(f0 <= 10.f));

	f0 = FRRand(9.5f, 11.75f);
	TEST_CHECK((f0 >= 9.5f) && TEST_CHECK(f0 <= 11.75f));
}