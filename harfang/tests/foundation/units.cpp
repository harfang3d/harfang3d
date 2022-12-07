// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#include <math.h>

#define TEST_NO_MAIN 
#include "acutest.h"

#include "foundation/math.h"
#include "foundation/unit.h"
#include "foundation/vector3.h"

using namespace hg;

void test_units() {
	TEST_CHECK(Equal(Deg(60.f), Pi/3.f));
	TEST_CHECK(Equal(Deg(90.f), HalfPi));
	TEST_CHECK(Equal(Deg(120.f), TwoPi / 3.f));
	TEST_CHECK(Equal(Deg(180.f), Pi));
	TEST_CHECK(Equal(Deg(315.f), 7.f*Pi/4.f));
	TEST_CHECK(Equal(Deg(360.f), TwoPi));

	TEST_CHECK(Equal(DegreeToRadian(60.f), Pi / 3.f));
	TEST_CHECK(Equal(DegreeToRadian(90.f), HalfPi));
	TEST_CHECK(Equal(DegreeToRadian(120.f), TwoPi / 3.f));
	TEST_CHECK(Equal(DegreeToRadian(180.f), Pi));
	TEST_CHECK(AlmostEqual(DegreeToRadian(315.f), 7.f * Pi / 4.f));
	TEST_CHECK(Equal(DegreeToRadian(360.f), TwoPi));

	TEST_CHECK(DegreeToRadian(Vec3(60.f, 90.f, 120.f)) == Vec3(Pi / 3.f, HalfPi, TwoPi/3.f));

	TEST_CHECK(Equal(RadianToDegree(Pi / 3.f), 60.f));
	TEST_CHECK(Equal(RadianToDegree(HalfPi), 90.f));
	TEST_CHECK(Equal(RadianToDegree(TwoPi / 3.f), 120.f));
	TEST_CHECK(Equal(RadianToDegree(Pi), 180.f));
	TEST_CHECK(Equal(RadianToDegree(7.f * Pi / 4.f), 315.f));
	TEST_CHECK(Equal(RadianToDegree(TwoPi), 360.f));

	TEST_CHECK(Equal(Csec(1.0f), 0.01f));
	TEST_CHECK(Equal(Csec(100.f), 1.f));

	TEST_CHECK(Equal(Sec(1.f), 1.f));
	TEST_CHECK(Equal(Sec(60.f), 60.f));
	TEST_CHECK(Equal(Sec(0.5f), 0.5f));

	TEST_CHECK(Equal(Ms(1.0f), 0.001f));
	TEST_CHECK(Equal(Ms(1000.f), 1.f));

	TEST_CHECK(Equal(Ton(2.f), 2000.f));
	TEST_CHECK(Equal(Ton(0.001f), 1.f));

	TEST_CHECK(Equal(Kg(2.f), 2.f));

	TEST_CHECK(Equal(G(2.f), 0.002f));
	TEST_CHECK(Equal(G(1000.f), 1.f));

	TEST_CHECK(Equal(Km(1.f), 1000.f));
	TEST_CHECK(Equal(Km(0.001f), 1.f));

	TEST_CHECK(Equal(Mtr(1.f), 1.f));

	TEST_CHECK(Equal(Cm(1.f), 0.01f));
	TEST_CHECK(Equal(Cm(100.f), 1.f));

	TEST_CHECK(Equal(Mm(1.f), 0.001f));
	TEST_CHECK(Equal(Mm(1000.f), 1.f));
	
	TEST_CHECK(Equal(Inch(1.f), 0.0254f));
	TEST_CHECK(AlmostEqual(Inch(39.3701f), 1.f));
	
	TEST_CHECK(Equal(KB(1), 1024));
	TEST_CHECK(Equal(KB(1024), MB(1)));

	TEST_CHECK(FormatMemorySize(512) == "512B");
	TEST_CHECK(FormatMemorySize(KB(1)) == "1.0KB");
	TEST_CHECK(FormatMemorySize(MB(1)) == "1.0MB");
	TEST_CHECK(FormatMemorySize(MB(1024)) == "1.0GB");
	TEST_CHECK(FormatMemorySize(MB(1024*1024)) == "1.0TB");
	TEST_CHECK(FormatMemorySize(KB(1) + 512) == "1.5KB");
	TEST_CHECK(FormatMemorySize(MB(2048+768)) == "2.7GB");
	TEST_CHECK(FormatMemorySize(MB(1) + KB(512)) == "1.5MB");
	TEST_CHECK(FormatMemorySize(-KB(4)) == "-4.0KB");

	TEST_CHECK(FormatCount(1) == "1");
	TEST_CHECK(FormatCount(10) == "10");
	TEST_CHECK(FormatCount(1000) == "1.0K");
	TEST_CHECK(FormatCount(12345) == "12.3K");
	TEST_CHECK(FormatCount(2000000) == "2.0M");
	TEST_CHECK(FormatCount(5555555) == "5.5M");
	TEST_CHECK(FormatCount(3100000000) == "3.1G");
	TEST_CHECK(FormatCount(-2300) == "-2.3K");

	TEST_CHECK(FormatDistance(Mm(1.f)) == "1.0mm");
	TEST_CHECK(FormatDistance(Mm(8.2f)) == "8.2mm");
	TEST_CHECK(FormatDistance(Cm(1.f)) == "10.0mm");
	TEST_CHECK(FormatDistance(Cm(53.5f)) == "53.5cm");
	TEST_CHECK(FormatDistance(45.3f) == "45.3m");
	TEST_CHECK(FormatDistance(Km(1.f)) == "1.0km");
	TEST_CHECK(FormatDistance(Km(12.345)) == "12.3km");
	TEST_CHECK(FormatDistance(Cm(-0.1f)) == "-1.0mm");

	TEST_CHECK(FormatTime(time_from_ms(20)) == "20 ms");
	TEST_CHECK(FormatTime(time_from_sec(1) + time_from_ms(100)) == "1 sec 100 ms");
	TEST_CHECK(FormatTime(time_from_min(30) + time_from_sec(26) + time_from_ms(3)) == "30 min 26 sec 3 ms");
	TEST_CHECK(FormatTime(time_from_hour(2) + time_from_min(15) + time_from_sec(30) + time_from_ms(222)) == "2 hour 15 min 30 sec 222 ms");
}
