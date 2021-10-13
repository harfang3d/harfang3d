// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/format.h"
#include "foundation/generational_vector_list.h"
#include "foundation/log.h"
#include "foundation/rand.h"
#include "foundation/string.h"
#include "foundation/time.h"
#include "foundation/vector3.h"
#include "gtest/gtest.h"

#include <list>
#include <map>
#include <vector>
#include <memory>

using namespace hg;

struct TrackedObject {
	static int count;
	TrackedObject(const TrackedObject &) { ++count; }
	TrackedObject() { ++count; }
	~TrackedObject() { --count; }
};

int TrackedObject::count = 0;

TEST(Container, VectorObjectConstructorDestructor) {
	auto sizeof_vector = sizeof(std::vector<TrackedObject>);

	TrackedObject::count = 0;

	std::vector<TrackedObject> objs;
	objs.reserve(256); // reserve space for 256 objects
	EXPECT_EQ(0, TrackedObject::count); // but should not construct them

	objs.push_back(TrackedObject()); // push one object on the array
	EXPECT_EQ(1, TrackedObject::count); // one object exists in the array

	objs.clear(); // destroy all objects
	EXPECT_EQ(0, TrackedObject::count);
}

TEST(Container, AutoVectorObjectConstructorDestructor) {
	auto sizeof_vector = sizeof(std::vector<std::unique_ptr<TrackedObject>>);

	TrackedObject::count = 0;

	std::vector<std::unique_ptr<TrackedObject>> objs;
	objs.reserve(256); // reserve space for 256 objects
	EXPECT_EQ(0, TrackedObject::count); // but should not construct them

	objs.emplace_back(new TrackedObject); // push several object on the array
	objs.emplace_back(new TrackedObject); // push several object on the array
	objs.emplace_back(new TrackedObject); // push several object on the array
	EXPECT_EQ(3, TrackedObject::count); // one object exists in the array

	objs.clear(); // destroy all objects
	EXPECT_EQ(0, TrackedObject::count);
}

//
TEST(Container, SharedVectorObjectConstructorDestructor) {
	TrackedObject::count = 0;

	auto sizeof_vector = sizeof(std::vector<std::shared_ptr<TrackedObject>>);

	std::vector<std::shared_ptr<TrackedObject>> objs;
	objs.reserve(256); // reserve space for 256 objects
	EXPECT_EQ(0, TrackedObject::count); // but should not construct them

	objs.emplace_back(new TrackedObject); // push several object on the array
	objs.emplace_back(new TrackedObject); // push several object on the array
	objs.emplace_back(new TrackedObject); // push several object on the array
	EXPECT_EQ(3, TrackedObject::count); // one object exists in the array

	objs.clear(); // destroy all objects
	EXPECT_EQ(0, TrackedObject::count);
}

//
template <class C> static void ContainerIteratorPerf() {
	C c;
	c.reserve(500000);
	for (int n = 0; n < 500000; ++n)
		c.push_back(n / 1000);

	// iterator version
	{
		auto t = time_now();

		int tt = 0;
		for (auto &i : c)
			tt += i;

		EXPECT_EQ(124750000, tt);
		log(format("Iterator -> %1ms").arg(time_to_ms(time_now() - t)));
	}

	// indexed version
	{
		auto t = time_now();

		int tt = 0;
		for (size_t n = 0; n < c.size(); ++n)
			tt += c[n];

		EXPECT_EQ(124750000, tt);
		log(format("Indexed -> %1ms").arg(time_to_ms(time_now() - t)));
	}
}

TEST(Container, VectorIteratorPerf) { ContainerIteratorPerf<std::vector<int>>(); }

//
TEST(Container, VectorMoveLeavesMovedFromEmpty) {
	std::vector<int> moved_from = {5, 6, 7};
	EXPECT_EQ(3, moved_from.size());
	auto moved_to = std::move(moved_from);
	EXPECT_EQ(3, moved_to.size());
	EXPECT_EQ(0, moved_from.size());
}

//
int alive_count = 0;

struct CountedObject {
	CountedObject(CountedObject &&o) { ++alive_count; }
	CountedObject() { ++alive_count; }
	~CountedObject() { --alive_count; }
	CountedObject &operator=(CountedObject &&o) {
		++alive_count;
		return *this;
	}
};

TEST(Container, GenRef) {
	generational_vector_list<CountedObject> obj;

	EXPECT_EQ(alive_count, 0);

	auto a = obj.add_ref({});
	auto b = obj.add_ref({});

	EXPECT_TRUE(obj.is_valid(a));
	EXPECT_TRUE(obj.is_valid(b));

	EXPECT_EQ(alive_count, 2);

	obj.remove_ref(a);

	EXPECT_FALSE(obj.is_valid(a));
	EXPECT_TRUE(obj.is_valid(b));

	obj.remove_ref(b);

	EXPECT_FALSE(obj.is_valid(a));
	EXPECT_FALSE(obj.is_valid(b));

	EXPECT_EQ(alive_count, 0);

	obj.add_ref({});
	obj.add_ref({});
	obj.add_ref({});

	EXPECT_FALSE(obj.is_valid(a));
	EXPECT_FALSE(obj.is_valid(b));

	EXPECT_EQ(alive_count, 3);

	obj.clear();

	EXPECT_EQ(alive_count, 0);

	std::list<gen_ref> refs;

	for (int j = 0; j < 16; ++j) {
		for (int i = 0; i < 6; ++i)
			refs.push_back(obj.add_ref({}));
	
		for (int i = 0; i < 4; ++i) {
			auto ref = refs.front();
			refs.pop_front();
			obj.remove_ref(ref);
		}
	}

	obj.clear();

	EXPECT_EQ(alive_count, 0);
}

TEST(Container, generational_vector_list_RefIsStableOnRemoval) {
	generational_vector_list<int> ints;

	auto a = ints.add_ref(1);
	auto b = ints.add_ref(2);
	auto c = ints.add_ref(3);

	ints.remove_ref(a);
	ints.remove_ref(b);

	EXPECT_EQ(ints.get_safe(c, -1), 3);
}

#if 0

constexpr size_t bench_size = 1000000;
constexpr size_t loop_count = 100;

/*
(0:097:266:606) Bench std::list

(3:960:787:527) std::list erase/insert *1K* entries (x100): 3804 ms
(5:805:773:862) std::list iterate 1K entries (x100): 1844 ms
(5:838:835:944) std::list remove 900 sparse entries: 32 ms
(5:838:950:879) Sparse entries count: 100
(6:474:672:206) std::list iterate 100 sparse entries: 635 ms
(6:474:768:771) Done

(6:488:112:660) Bench std::vector

(9:220:332:041) std::vector erase/insert *10* entries (x100): 2718 ms
(9:502:163:535) std::vector iterate 1M entries (x100): 281 ms
(9:515:421:285) std::vector iterate 100K entries (x100): 13 ms
(9:516:613:333) std::vector iterate 10K entries (x100): 1 ms
(9:516:646:597) Done

(9:518:956:966) Bench tsl::robin_map

(10:330:104:229) tsl::robin_map erase/insert 100K entries (x100): 728 ms
(11:074:598:081) tsl::robin_map iterate 1M entries (x100): 744 ms
(11:115:592:670) tsl::robin_map remove 900K sparse entries: 40 ms
(11:115:696:930) Sparse entries count: 100000
(11:598:149:719) tsl::robin_map iterate 100K entries (x100): 482 ms
(11:602:353:662) tsl::robin_map remove 90K sparse entries: 4 ms
(11:602:416:219) Sparse entries count: 10000
(12:050:929:895) tsl::robin_map iterate 10K entries (x100): 448 ms
(12:051:039:617) Done

(12:061:956:960) Bench vector_list

(12:559:873:110) vector_list erase/insert 100K entries (x100): 480 ms
(12:885:024:952) vector_list iterate 1M entries pre-compact (x100): 325 ms
(14:239:862:813) vector_list compact 1M entries (x100): 1354 ms
(14:548:430:739) vector_list iterate 1M entries post-compact (x100): 308 ms
(14:555:534:373) vector_list remove 900K sparse entries: 6 ms
(14:555:573:843) Sparse entries count: 100000
(14:740:094:793) vector_list iterate 100K sparse entries pre-compact (x100): 184 ms
(14:880:238:638) vector_list compact 100K entries (x100): 140 ms
(14:918:544:292) vector_list iterate 100K entries post-compact (x100): 38 ms
(14:925:715:198) vector_list remove 90K sparse entries: 7 ms
(14:925:752:931) Sparse entries count: 10000
(14:937:131:504) vector_list iterate 10K sparse entries pre-compact (x100): 11 ms
(14:950:045:689) vector_list compact 10K entries (x100): 12 ms
(14:960:516:449) vector_list iterate 10K entries post-compact (x100): 10 ms
(14:960:572:551) Done
*/

struct State {
	Vec3 x, y;
	float d;
};

void BenchStdVector() {
	log("Bench std::vector\n");

	time_ns t_start;

	std::vector<State> states;
	states.reserve(bench_size);

	for (size_t i = 0; i < bench_size; ++i)
		states.push_back({{1, 2, 3}, {3, 2, 1}});

	t_start = time_now();
	for (size_t j = 0; j < loop_count; ++j) {
		for (size_t i = 0; i < 10; ++i)
			states.erase(std::begin(states) + i * 10);
		for (size_t i = 0; i < 10; ++i)
			states.push_back({{1, 2, 3}, {3, 2, 1}});
	}
	log(format("std::vector erase/insert *10* entries (x100): %1 ms").arg(time_to_ms(time_now() - t_start)));

	t_start = time_now();
	for (size_t j = 0; j < loop_count; ++j) {
		for (size_t i = 0; i < bench_size; ++i) {
			auto &s = states[i];
			s.d = Dot(s.x, s.y);
		}
	}
	log(format("std::vector iterate 1M entries (x100): %1 ms").arg(time_to_ms(time_now() - t_start)));

	states.resize(bench_size / 10);

	t_start = time_now();
	for (size_t j = 0; j < loop_count; ++j) {
		for (size_t i = 0; i < bench_size / 10; ++i) {
			auto &s = states[i];
			s.d = Dot(s.x, s.y);
		}
	}
	log(format("std::vector iterate 100K entries (x100): %1 ms").arg(time_to_ms(time_now() - t_start)));

	states.resize(bench_size / 100);

	t_start = time_now();
	for (size_t j = 0; j < loop_count; ++j) {
		for (size_t i = 0; i < bench_size / 100; ++i) {
			auto &s = states[i];
			s.d = Dot(s.x, s.y);
		}
	}
	log(format("std::vector iterate 10K entries (x100): %1 ms").arg(time_to_ms(time_now() - t_start)));

	log("Done\n");
}

void BenchStdList() {
	log("Bench std::list\n");

	time_ns t_start;

	std::list<State> states;

	auto iter = [&]() {
		for (size_t j = 0; j < loop_count; ++j)
			for (auto &s : states)
				s.d = Dot(s.x, s.y);
	};

	for (size_t i = 0; i < bench_size; ++i)
		states.push_back({{1, 2, 3}, {3, 2, 1}});

	t_start = time_now();
	for (size_t j = 0; j < loop_count; ++j) {
		for (size_t i = 0; i < 1000; ++i) {
			auto it = std::begin(states);
			std::advance(it, i * 10);
			states.erase(it);
		}
		for (size_t i = 0; i < 1000; ++i)
			states.push_back({{1, 2, 3}, {3, 2, 1}});
	}
	log(format("std::list erase/insert *1K* entries (x100): %1 ms").arg(time_to_ms(time_now() - t_start)));

	t_start = time_now();
	iter();
	log(format("std::list iterate 1K entries (x100): %1 ms").arg(time_to_ms(time_now() - t_start)));

	t_start = time_now();
	auto it = std::begin(states);
	for (size_t i = 0; i < bench_size; ++i) {
		if (i % 10)
			states.erase(it);
		++it;
	}
	log(format("std::list remove 900 sparse entries: %1 ms").arg(time_to_ms(time_now() - t_start)));
	log(format("Sparse entries count: %1").arg(states.size()));

	t_start = time_now();
	iter();
	log(format("std::list iterate 100 sparse entries: %1 ms").arg(time_to_ms(time_now() - t_start)));

	log("Done\n");
}

#if 0
void BenchTslRobinMap() {
	log("Bench tsl::robin_map\n");

	time_ns t_start;

	tsl::robin_map<uint32_t, State> states;
	states.reserve(bench_size);

	auto iter = [&]() {
		for (size_t j = 0; j < loop_count; ++j) {
			for (auto i : states) {
				auto &s = i.second;
				s.d = Dot(s.x, s.y);
			}
		}
	};

	for (size_t i = 0; i < bench_size; ++i)
		states[i] = {{1, 2, 3}, {3, 2, 1}};

	t_start = time_now();
	for (size_t j = 0; j < loop_count; ++j) {
		for (size_t i = 0; i < bench_size / 10; ++i)
			states.erase(i * 10);
		for (size_t i = 0; i < bench_size / 10; ++i)
			states[i * 10] = {{1, 2, 3}, {3, 2, 1}};
	}
	log(format("tsl::robin_map erase/insert 100K entries (x100): %1 ms").arg(time_to_ms(time_now() - t_start)));

	t_start = time_now();
	iter();
	log(format("tsl::robin_map iterate 1M entries (x100): %1 ms").arg(time_to_ms(time_now() - t_start)));

	//
	t_start = time_now();
	for (size_t i = 0; i < bench_size; ++i)
		if (i % 10)
			states.erase(i);
	log(format("tsl::robin_map remove 900K sparse entries: %1 ms").arg(time_to_ms(time_now() - t_start)));
	log(format("Sparse entries count: %1").arg(states.size()));

	t_start = time_now();
	iter();
	log(format("tsl::robin_map iterate 100K entries (x100): %1 ms").arg(time_to_ms(time_now() - t_start)));

	//
	t_start = time_now();
	for (size_t i = 0; i < bench_size / 10; ++i)
		if (i % 10)
			states.erase(i * 10);
	log(format("tsl::robin_map remove 90K sparse entries: %1 ms").arg(time_to_ms(time_now() - t_start)));
	log(format("Sparse entries count: %1").arg(states.size()));

	t_start = time_now();
	iter();
	log(format("tsl::robin_map iterate 10K entries (x100): %1 ms").arg(time_to_ms(time_now() - t_start)));

	log("Done\n");
}
#endif

void BenchVectorList() {
	log("Bench vector_list\n");

	time_ns t_start;

	vector_list<State> states;
	states.reserve(bench_size);

	auto iter = [&]() {
		for (int j = 0; j < loop_count; ++j) {
			for (uint32_t i = states.first(); i != states.invalid_idx; i = states.next(i)) {
				auto &s = states[i];
				s.d = Dot(s.x, s.y);
			}
		}
	};

	for (int i = 0; i < bench_size; ++i)
		states.add({{1, 2, 3}, {3, 2, 1}});

	t_start = time_now();
	for (int j = 0; j < loop_count; ++j) {
		for (int i = 0; i < bench_size / 10; ++i)
			states.remove(i * 10);
		for (int i = 0; i < bench_size / 10; ++i)
			states.add({{1, 2, 3}, {3, 2, 1}});
	}
	log(format("vector_list erase/insert 100K entries (x100): %1 ms").arg(time_to_ms(time_now() - t_start)));

	t_start = time_now();
	iter();
	log(format("vector_list iterate 1M entries pre-compact (x100): %1 ms").arg(time_to_ms(time_now() - t_start)));

	t_start = time_now();
	for (int j = 0; j < loop_count; ++j)
		states.compact();
	log(format("vector_list compact 1M entries (x100): %1 ms").arg(time_to_ms(time_now() - t_start)));

	t_start = time_now();
	iter();
	log(format("vector_list iterate 1M entries post-compact (x100): %1 ms").arg(time_to_ms(time_now() - t_start)));

	//
	t_start = time_now();
	for (int i = 0; i < bench_size; ++i)
		if (i % 10)
			states.remove(i);
	log(format("vector_list remove 900K sparse entries: %1 ms").arg(time_to_ms(time_now() - t_start)));
	log(format("Sparse entries count: %1").arg(states.size()));

	t_start = time_now();
	iter();
	log(format("vector_list iterate 100K sparse entries pre-compact (x100): %1 ms").arg(time_to_ms(time_now() - t_start)));

	t_start = time_now();
	for (int j = 0; j < loop_count; ++j)
		states.compact();
	log(format("vector_list compact 100K entries (x100): %1 ms").arg(time_to_ms(time_now() - t_start)));

	t_start = time_now();
	iter();
	log(format("vector_list iterate 100K entries post-compact (x100): %1 ms").arg(time_to_ms(time_now() - t_start)));

	//
	t_start = time_now();
	for (int i = 0; i < bench_size / 10; ++i)
		if (i % 10)
			states.remove(i * 10);
	log(format("vector_list remove 90K sparse entries: %1 ms").arg(time_to_ms(time_now() - t_start)));
	log(format("Sparse entries count: %1").arg(states.size()));

	t_start = time_now();
	iter();
	log(format("vector_list iterate 10K sparse entries pre-compact (x100): %1 ms").arg(time_to_ms(time_now() - t_start)));

	t_start = time_now();
	for (int j = 0; j < loop_count; ++j)
		states.compact();
	log(format("vector_list compact 10K entries (x100): %1 ms").arg(time_to_ms(time_now() - t_start)));

	t_start = time_now();
	iter();
	log(format("vector_list iterate 10K entries post-compact (x100): %1 ms").arg(time_to_ms(time_now() - t_start)));

	log("Done\n");
}

#endif //
