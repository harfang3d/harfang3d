// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#define TEST_NO_MAIN
#include "acutest.h"

#include "foundation/pack_float.h"

//#include "../utils.h"

using namespace hg;

void test_pack_float() {
	TEST_CHECK(pack_float<float>(0.1f, 10.f) == 0.1f);
	TEST_CHECK(pack_float<float>(-1.f) == -1.f);
	TEST_CHECK(unpack_float<float>(-100.2f, 2.f) == -100.2f);
	TEST_CHECK(unpack_float<float>(0.05f) == 0.05f);
		
	TEST_CHECK(pack_float<uint8_t>(0.5f) == 128);
	TEST_CHECK(AlmostEqual(unpack_float<uint8_t>(128), 0.5f, 1.f/255.f));

	TEST_CHECK(pack_float<uint8_t>(-0.8f, 2.f) == 0);
	
	TEST_CHECK(pack_float<uint16_t>(1.f) == 65535);
	TEST_CHECK(unpack_float<uint16_t>(65535) == 1.f);

	TEST_CHECK(pack_float<uint16_t>(40.f, 100.f) == 26214);
	TEST_CHECK(unpack_float<uint16_t>(26214, 100.f) == 40.f);

	TEST_CHECK(pack_float<int8_t>(-1.f) == -127);
	TEST_CHECK(unpack_float<int8_t>(-127) == -1.f);

	TEST_CHECK(pack_float<int8_t>(0.5f) == 64);
	TEST_CHECK(AlmostEqual(unpack_float<int8_t>(64), 0.5f, 1.f / 127.f));

	TEST_CHECK(pack_float<int8_t>(-0.8f, 2.f) == -51);
	TEST_CHECK(AlmostEqual(unpack_float<int8_t>(-51, 2.f), -0.8f, 1.f / 127.f));

	TEST_CHECK(pack_float<int16_t>(1.f) == 32767);
	TEST_CHECK(unpack_float<int16_t>(32767) == 1.f);

	TEST_CHECK(pack_float<int16_t>(40.f, 100.f) == 13107);
	TEST_CHECK(AlmostEqual(unpack_float<int16_t>(13107, 100.f), 40.f, 0.001f));

	TEST_CHECK(pack_float<int16_t>(-0.8f, 2.f) == -13107);
	TEST_CHECK(AlmostEqual(unpack_float<int16_t>(-13107, 2.f), -0.8f, 1.f / 32767.f));
}
