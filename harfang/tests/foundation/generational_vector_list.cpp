// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#include <math.h>

#define TEST_NO_MAIN
#include "acutest.h"

#include "foundation/generational_vector_list.h"

using namespace hg;

void test_generational_vector_list() {
	{ 
		gen_ref ref;
		TEST_CHECK(ref.idx == invalid_gen_ref.idx);
		TEST_CHECK(ref.gen == invalid_gen_ref.gen);
	}
	{
		gen_ref r0, r1, r2, r3;
		r0.idx = 1, r0.gen = 0xc07;
		r1.idx = 1, r1.gen = 0xc07;
		r2.idx = 2, r2.gen = 0xc07;
		r3.idx = 1, r3.gen = 0x9e0;
		
		TEST_CHECK(r0 == r1);
		TEST_CHECK((r0 == r2) == false);
		TEST_CHECK((r1 == r3) == false);
		TEST_CHECK(r0 != r2);
		TEST_CHECK(r0 != r3);
		TEST_CHECK((r0 != r1) == false);
	}
	{
		gen_ref r0, r1, r2, r3;
		r0.idx = 1, r0.gen = 0xc07;
		r1.idx = 2, r1.gen = 0xc07;
		r2.idx = 3, r2.gen = 0xc07;
		r3.idx = 2, r3.gen = 0x9e0;

		TEST_CHECK(r0 < r1);
		TEST_CHECK(r1 < r2);
		TEST_CHECK(r3 < r2);
		TEST_CHECK(r3 < r0);
	}
	{
		generational_vector_list<int> container;
		gen_ref r0 = container.add_ref(5);
		gen_ref r1 = container.add_ref(7);
		gen_ref r2 = container.add_ref(9);
		container.remove_ref(r1);
		gen_ref r3 = container.add_ref(11);

		TEST_CHECK(container.is_valid(r0) == true);
		TEST_CHECK(container.is_valid(r1) == false);
		TEST_CHECK(container.is_valid(r2) == true);
		TEST_CHECK(container.is_valid(r3) == true);

		int dflt = -29;
		TEST_CHECK(container.get_safe(r0, dflt) == 5);
		TEST_CHECK(container.get_safe(r1, dflt) == dflt);
		TEST_CHECK(container.get_safe(r2, dflt) == 9);
		TEST_CHECK(container.get_safe(r3, dflt) == 11);

		container.remove_ref(r0);
		gen_ref r4 = container.add_ref(17);

		{
			const generational_vector_list<int> &const_container = container;
			int dflt = -31;
			TEST_CHECK(container.get_safe(r0, dflt) == dflt);
			TEST_CHECK(container.get_safe(r1, dflt) == dflt);
			TEST_CHECK(container.get_safe(r2, dflt) == 9);
			TEST_CHECK(container.get_safe(r3, dflt) == 11);
			TEST_CHECK(container.get_safe(r4, dflt) == 17);
		}

		const int value[3] = {17, 9, 11};
		int count[3] = {0, 0, 0};
		int i;
		gen_ref ref;
		for (ref = container.first_ref(), i = 0; ref != invalid_gen_ref; ref = container.next_ref(ref), i++) {
			TEST_CHECK(i < 3);
			if (i < 3) {
				for (int j = 0; j < 3; j++) {
					if (value[j] == container.get_safe(ref, 97)) {
						count[i]++;
						break;
					}
				}
			}
		}
		for (int i = 0; i < 3; i++) {
			TEST_CHECK(count[i] == 1);
		}

		container.remove_ref(r4);
		TEST_CHECK(container.get_ref(r4.idx) == invalid_gen_ref);
		TEST_CHECK(container.get_ref(r3.idx) == r3);
		TEST_CHECK(container.get_ref(r2.idx) == r2);
		TEST_CHECK(container.get_ref(r1.idx) == r3);
		TEST_CHECK(container.get_ref(r0.idx) == invalid_gen_ref);

		container.clear();
		TEST_CHECK(container.is_valid(r4) == false);
		TEST_CHECK(container.is_valid(r3) == false);
		TEST_CHECK(container.is_valid(r2) == false);
		TEST_CHECK(container.is_valid(r1) == false);
		TEST_CHECK(container.is_valid(r0) == false);
	}
}