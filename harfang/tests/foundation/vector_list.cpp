// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#define TEST_NO_MAIN
#include "acutest.h"

#include <vector>

#include "foundation/cext.h"
#include "foundation/vector_list.h"
#include "foundation/vector3.h"

using namespace hg;

void test_vector_list() {
	{
		vector_list<Vec3> list;
		TEST_CHECK(list.size() == 0);
		TEST_CHECK(list.capacity() == 0);

		list.reserve(32);
		TEST_CHECK(list.size() == 0);
		TEST_CHECK(list.capacity() == 32);

		list.reserve(10);
		TEST_CHECK(list.size() == 0);
		TEST_CHECK(list.capacity() == 32);

		list.reserve(256);
		TEST_CHECK(list.size() == 0);
		TEST_CHECK(list.capacity() == 256);
	}
	{
		vector_list<Vec3> list(10);
		TEST_CHECK(list.size() == 0);
		TEST_CHECK(list.capacity() == 10);
	}
	{
		vector_list<Vec3> list(4);

		TEST_CHECK(list.add(Vec3::Back) == 0);
		TEST_CHECK(list.add(Vec3::Down) == 1);
		TEST_CHECK(list.add(Vec3::Left) == 2);
		TEST_CHECK(list.add(Vec3::Right) == 3);

		TEST_CHECK(list.size() == 4);
		TEST_CHECK(list.capacity() == 4);

		TEST_CHECK(list[0] == Vec3::Back);
		TEST_CHECK(list[1] == Vec3::Down);
		TEST_CHECK(list[2] == Vec3::Left);
		TEST_CHECK(list[3] == Vec3::Right);

		TEST_CHECK(list.add(Vec3::Up) == 4);
		TEST_CHECK(list.add(Vec3::Front) == 5);

		TEST_CHECK(list.value(4) == Vec3::Up);
		TEST_CHECK(list.value(5) == Vec3::Front);

		TEST_CHECK(list.size() == 6);
		TEST_CHECK(list.capacity() > 4);

		list.clear();

		TEST_CHECK(list.size() == 0);
		TEST_CHECK(list.capacity() == 0);
	}

	{
		vector_list<Vec3> list(512);
		TEST_CHECK(list.add(Vec3::Right) == 0);
		TEST_CHECK(list.add(Vec3::Left) == 1);
		TEST_CHECK(list.add(Vec3::Up) == 2);
		TEST_CHECK(list.add(Vec3::Down) == 3);
		TEST_CHECK(list.add(Vec3::Front) == 4);
		TEST_CHECK(list.add(Vec3::Back) == 5);
		TEST_CHECK(list.add(Vec3::Zero) == 6);
		TEST_CHECK(list.add(Vec3::One) == 7);

		TEST_CHECK(list.remove(3) == 4);
		TEST_CHECK(list.remove(4) == 5);
		TEST_CHECK(list.remove(2) == 5);

		TEST_CHECK(list.size() == 5);

		TEST_CHECK(list.is_used(0) == true);
		TEST_CHECK(list.is_used(1) == true);
		TEST_CHECK(list.is_used(2) == false);
		TEST_CHECK(list.is_used(3) == false);
		TEST_CHECK(list.is_used(4) == false);
		TEST_CHECK(list.is_used(5) == true);
		TEST_CHECK(list.is_used(6) == true);
		TEST_CHECK(list.is_used(7) == true);
		TEST_CHECK(list.is_used(8) == false);

		TEST_CHECK(list.next(0) == 1);
		TEST_CHECK(list.next(1) == 5);
		TEST_CHECK(list.next(2) == 5);
		TEST_CHECK(list.next(3) == 5);
		TEST_CHECK(list.next(4) == 5);
		TEST_CHECK(list.next(5) == 6);
		TEST_CHECK(list.next(6) == 7);
		TEST_CHECK(list.next(7) == vector_list<Vec3>::invalid_idx);

		TEST_CHECK(list.first() == 0);

		TEST_CHECK(list.remove(0) == 1);
		TEST_CHECK(list.remove(1) == 5);
		TEST_CHECK(list.first() == 5);

		{
			vector_list<Vec3>::iterator it = list.begin();
			TEST_CHECK(it != list.end());
			TEST_CHECK(it.idx() == list.first());
			TEST_CHECK(*it == Vec3::Back);
			TEST_CHECK(it->x == Vec3::Back.x);
			TEST_CHECK(it->y == Vec3::Back.y);
			TEST_CHECK(it->z == Vec3::Back.z);

			++it;

			it++;
			TEST_CHECK(*it == Vec3::One);
			TEST_CHECK(it->x == Vec3::One.x);
			TEST_CHECK(it->y == Vec3::One.y);
			TEST_CHECK(it->z == Vec3::One.z);

			TEST_CHECK(it == vector_list<Vec3>::iterator(&list, list.first() + 2));

			++it;
			TEST_CHECK(it == list.end());
		}

		{
			const vector_list<Vec3> &const_list = list;
			vector_list<Vec3>::const_iterator it = const_list.begin();
			TEST_CHECK(it != const_list.end());
			TEST_CHECK(it.idx() == const_list.first());
			TEST_CHECK(*it == Vec3::Back);
			TEST_CHECK(it->x == Vec3::Back.x);
			TEST_CHECK(it->y == Vec3::Back.y);
			TEST_CHECK(it->z == Vec3::Back.z);

			++it;

			++it;
			TEST_CHECK(*it == Vec3::One);
			TEST_CHECK(it->x == Vec3::One.x);
			TEST_CHECK(it->y == Vec3::One.y);
			TEST_CHECK(it->z == Vec3::One.z);

			TEST_CHECK(it == vector_list<Vec3>::const_iterator(&const_list, const_list.first() + 2));

			++it;
			TEST_CHECK(it == const_list.end());
		}

		TEST_CHECK(list.add(Vec3::Down) == 1);

		TEST_CHECK(list.size() == 4);

		const Vec3 expected[4] = {Vec3::Back, Vec3::Zero, Vec3::One, Vec3::Down};
		int count[4] = {0, 0, 0, 0};
		for (vector_list<Vec3>::iterator it = list.begin(); it != list.end(); ++it) {
			for (int i = 0; i < 4; i++) {
				if (*it == expected[i]) {
					count[i]++;
					break;
				}
			}
		}
		TEST_CHECK(count[0] == 1);
		TEST_CHECK(count[1] == 1);
		TEST_CHECK(count[2] == 1);
		TEST_CHECK(count[3] == 1);

		list.compact(); // [todo] there's no way to test this.
	}
	{
		vector_list<size_t> list;
		for (size_t i = 0; i < 320; i++) {
			list.add(i);
		}
		for (size_t i = 0; i < 320; i++) {
			list.remove(i);
		}
		std::vector<bool> visited(640, false);
		for (size_t i = 0; i < 320; i += 2) {
			list.add(2 * i);
			visited[2 * i] = true;
		}
		TEST_CHECK(list.size() == 160);
		for (vector_list<size_t>::iterator it = list.begin(); it != list.end(); ++it) {
			TEST_CHECK(visited[*it] == true);
			visited[*it] = false;
		}
		for (std::vector<bool>::iterator it = visited.begin(); it != visited.end(); it++) {
			TEST_CHECK(visited[*it] == false);
		}
	}
}