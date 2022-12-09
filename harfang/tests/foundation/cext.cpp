// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#define TEST_NO_MAIN
#include "acutest.h"

#include "foundation/cext.h"

using namespace hg;

static void test_asbool() {
	TEST_CHECK(asbool<int>(0) == false);
	TEST_CHECK(asbool<int>(1) == true);
	TEST_CHECK(asbool<int>(-1) == true);

	TEST_CHECK(asbool<float>(0.f) == false);
	TEST_CHECK(asbool<float>(0.0001f) == true);
	TEST_CHECK(asbool<float>(-0.00001f) == true);
}

static void test_set_if_not_equal() {
	int i0 = 3, i1 = -16, i2 = i0;
	TEST_CHECK(set_if_not_equal(i0, i2) == false);
	TEST_CHECK(set_if_not_equal(i0, i1) == true);
	TEST_CHECK(i0 == i1);

	float f0 = 0.5f, f1 = 0.25f, f2 = f0;
	TEST_CHECK(set_if_not_equal(f0, f2) == false);
	TEST_CHECK(set_if_not_equal(f0, f1) == true);
	TEST_CHECK(f0 == f1);

	std::string s0("string"), s1("hammer"), s2(s0);
	TEST_CHECK(set_if_not_equal(s0, s2) == false);
	TEST_CHECK(set_if_not_equal(s0, s1) == true);
	TEST_CHECK(s0 == s1);
}

static void test_set_bool_gate() {
	bool b0 = true;
	TEST_CHECK(bool_gate(b0) == true);
	TEST_CHECK(b0 == false);

	bool b1 = false;
	TEST_CHECK(bool_gate(b1) == false);
	TEST_CHECK(b1 == false);
}

#if __cplusplus < 201103L
static void test_array() { 
	{
		std::array<int, 5> i5; 
		TEST_CHECK(i5.size() == 5);
		TEST_CHECK(i5.empty() == false);

		i5.fill(4);
		for (size_t i = 0; i < i5.size(); i++) {
			TEST_CHECK(i5[i] == 4);
			i5[i] = i;
		}
		for (size_t i = 0; i < i5.size(); i++) {
			TEST_CHECK(i5[i] == i);
			TEST_CHECK(i5[i] == i5.at(i));
		}

		TEST_CHECK(i5.front() == 0);
		TEST_CHECK(i5.back() == 4);

		i5.front() = -1;
		i5.back() = 10;

		TEST_CHECK(i5.front() == -1);
		TEST_CHECK(i5.back() == 10);

		std::array<int, 5> j5(32);
		size_t j = 0;
		for (std::array<int, 5>::iterator it = j5.begin(); it != j5.end(); it++, j++) {
			TEST_CHECK(*it == 32);
			TEST_CHECK((it - j5.begin()) == j);
		}
		
		for (std::array<int, 5>::reverse_iterator it = i5.rbegin(); it != i5.rend(); it++, j--) {
			TEST_CHECK(*it == i5[j-1]);
			*it = j;
		}

		const std::array<int, 5> &k5 = i5;
		j = 0;
		for (std::array<int, 5>::const_iterator it = k5.begin(); it != k5.end(); it++, j++) {
			TEST_CHECK(*it == k5[j]);
		}

		for (std::array<int, 5>::const_reverse_iterator it = k5.rbegin(); it != k5.rend(); it++, j--) {
			TEST_CHECK(*it == k5[j - 1]);
		}

		TEST_CHECK(k5.front() == i5.front());
		TEST_CHECK(k5.back() == i5.back());
	}

	{ 
		const char *data = "0123456789ABCDEF";
		std::array<char, 16> c16(data);

		for (size_t i = 0; i < c16.size(); i++) {
			TEST_CHECK(c16[i] == data[i]);
		}

		std::array<char, 16> d16(c16);
		TEST_CHECK(d16.data() != c16.data());
		for (size_t i = 0; i < c16.size(); i++) {
			TEST_CHECK(d16[i] == c16[i]);
		}
		TEST_CHECK(d16 == c16);
		TEST_CHECK((d16 != c16) == false);

		std::array<char, 16> e16('#');
		TEST_CHECK((e16 == d16) == false);
		TEST_CHECK((e16 != c16) == true);

		const char *other = "0123456789abcdef";
		std::array<char, 16> f16(other);
		TEST_CHECK((c16 != f16) == true);
		TEST_CHECK((c16 < f16) == true);
		TEST_CHECK((c16 > f16) == false);
		TEST_CHECK((f16 > c16) == true);
		TEST_CHECK((f16 >= c16) == true);
		TEST_CHECK((c16 <= f16) == true);
		TEST_CHECK((d16 <= c16) == true);
		TEST_CHECK((d16 >= c16) == true);

		e16 = f16;
		TEST_CHECK(e16 == f16);
	}
}

static void test_is_integral() {
	TEST_CHECK(std::is_integral<bool>() == true);
	TEST_CHECK(std::is_integral<char>() == true);
	TEST_CHECK(std::is_integral<signed char>() == true);
	TEST_CHECK(std::is_integral<unsigned char>() == true);
	TEST_CHECK(std::is_integral<short>() == true);
	TEST_CHECK(std::is_integral<unsigned short>() == true);
	TEST_CHECK(std::is_integral<int>() == true);
	TEST_CHECK(std::is_integral<unsigned int>() == true);
	TEST_CHECK(std::is_integral<long>() == true);
	TEST_CHECK(std::is_integral<unsigned long>() == true);
	TEST_CHECK(std::is_integral<long long>() == true);
	TEST_CHECK(std::is_integral<unsigned long long>() == true);
	TEST_CHECK(std::is_integral<float>() == false);
	TEST_CHECK(std::is_integral<double>() == false);
	TEST_CHECK(std::is_integral<long double>() == false);


	TEST_CHECK(std::is_floating_point<bool>() == false);
	TEST_CHECK(std::is_floating_point<char>() == false);
	TEST_CHECK(std::is_floating_point<signed char>() == false);
	TEST_CHECK(std::is_floating_point<unsigned char>() == false);
	TEST_CHECK(std::is_floating_point<short>() == false);
	TEST_CHECK(std::is_floating_point<unsigned short>() == false);
	TEST_CHECK(std::is_floating_point<int>() == false);
	TEST_CHECK(std::is_floating_point<unsigned int>() == false);
	TEST_CHECK(std::is_floating_point<long>() == false);
	TEST_CHECK(std::is_floating_point<unsigned long>() == false);
	TEST_CHECK(std::is_floating_point<long long>() == false);
	TEST_CHECK(std::is_floating_point<unsigned long long>() == false);
	TEST_CHECK(std::is_floating_point<float>() == true);
	TEST_CHECK(std::is_floating_point<double>() == true);
	TEST_CHECK(std::is_floating_point<long double>() == true);

	TEST_CHECK(std::is_arithmetic<bool>() == true);
	TEST_CHECK(std::is_arithmetic<char>() == true);
	TEST_CHECK(std::is_arithmetic<signed char>() == true);
	TEST_CHECK(std::is_arithmetic<unsigned char>() == true);
	TEST_CHECK(std::is_arithmetic<short>() == true);
	TEST_CHECK(std::is_arithmetic<unsigned short>() == true);
	TEST_CHECK(std::is_arithmetic<int>() == true);
	TEST_CHECK(std::is_arithmetic<unsigned int>() == true);
	TEST_CHECK(std::is_arithmetic<long>() == true);
	TEST_CHECK(std::is_arithmetic<unsigned long>() == true);
	TEST_CHECK(std::is_arithmetic<long long>() == true);
	TEST_CHECK(std::is_arithmetic<unsigned long long>() == true);
	TEST_CHECK(std::is_arithmetic<float>() == true);
	TEST_CHECK(std::is_arithmetic<double>() == true);
	TEST_CHECK(std::is_arithmetic<long double>() == true);

	TEST_CHECK(std::is_signed<bool>() == false);
	TEST_CHECK(std::is_signed<char>() == true);
	TEST_CHECK(std::is_signed<signed char>() == true);
	TEST_CHECK(std::is_signed<unsigned char>() == false);
	TEST_CHECK(std::is_signed<short>() == true);
	TEST_CHECK(std::is_signed<unsigned short>() == false);
	TEST_CHECK(std::is_signed<int>() == true);
	TEST_CHECK(std::is_signed<unsigned int>() == false);
	TEST_CHECK(std::is_signed<long>() == true);
	TEST_CHECK(std::is_signed<unsigned long>() == false);
	TEST_CHECK(std::is_signed<long long>() == true);
	TEST_CHECK(std::is_signed<unsigned long long>() == false);
	TEST_CHECK(std::is_signed<float>() == true);
	TEST_CHECK(std::is_signed<double>() == true);
	TEST_CHECK(std::is_signed<long double>() == true);
}
#endif

void test_cext() {
	test_asbool();
	test_set_if_not_equal();
	test_set_bool_gate();
#if __cplusplus < 201103L
	test_array();
	test_is_integral();
#endif
}