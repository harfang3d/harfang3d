// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#include <math.h>

#define TEST_NO_MAIN
#include "acutest.h"

#include "foundation/cext.h"

#include "foundation/intrusive_shared_ptr_st.h"

using namespace hg;

struct DummyRef {
	explicit DummyRef(int d, bool *w) : ref_count(0), watch(w), data(d) {}
	DummyRef(const DummyRef &r) : ref_count(r.ref_count), watch(r.watch), data(r.data) {}
	~DummyRef() {
		if (watch)
			*watch = false;
	}
	DummyRef &operator=(const DummyRef &r) {
		ref_count = r.ref_count;
		data = r.data;
		watch = r.watch;
		return *this;
	}
	int data;
	bool *watch;
	uint32_t ref_count;
};

void test_intrusive_shared_ptr_st() {
	{
		intrusive_shared_ptr_st<DummyRef> ptr;
		TEST_CHECK(!ptr);
	}
	{
		bool allocated = true;
		{
			DummyRef *ref = new DummyRef(0xcafe, &allocated);
			intrusive_shared_ptr_st<DummyRef> ptr(ref);
			TEST_CHECK(ptr == true);
			TEST_CHECK(ptr.get() == ref);
			TEST_CHECK(ptr->data == 0xcafe);
			TEST_CHECK(allocated == true);
		}
		TEST_CHECK(allocated == false);
	}
	{
		bool a0 = true;
		bool a1 = true;
		DummyRef *r0 = new DummyRef(0x7c, &a0);
		DummyRef *r1 = new DummyRef(0xa9, &a1);

		intrusive_shared_ptr_st<DummyRef> p0(r0);
		intrusive_shared_ptr_st<DummyRef> p1(r1);
		intrusive_shared_ptr_st<DummyRef> p2(p0);
		intrusive_shared_ptr_st<DummyRef> p3(r1);

		TEST_CHECK((p0 == p1) == false);
		TEST_CHECK((p0 != p1) == true);
		TEST_CHECK((p0 == p2) == true);
		TEST_CHECK((p2 != p0) == false);
		TEST_CHECK((p1 == p3) == true);
		TEST_CHECK((p3 != p1) == false);

		p1 = p2;
		TEST_CHECK(p0->ref_count == 3);
		TEST_CHECK(p3->ref_count == 1);
		p1 = nullptr;
		TEST_CHECK(p0->ref_count == 2);
		TEST_CHECK(p3->ref_count == 1);
		p3 = p0;
		TEST_CHECK(a0 == true);
		TEST_CHECK(a1 == false);
		TEST_CHECK(p0->ref_count == 3);
	}
}