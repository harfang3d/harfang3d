// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include <algorithm>
#include <vector>

namespace hg {

/**
	@short Compute average value excluding outliers of a fixed number of values.
	Values are shifted out to make room for the new value when the container
	capacity is reached.
*/
template <class T, int Size = 8, int Outliers = 2>
class SmartMedianAverage {
public:
	static const int size = Size;
	static const int outliers = Outliers;

	SmartMedianAverage() : values(Size) {}

	/// Add a value to the container.
	void Add(T v) {
		if (count < Size)
			count++; // fill up
		else
			for (int n = 1; n < Size; ++n) // shift
				values[n - 1] = values[n];

		values[count - 1] = v;
	}

	/// Return the average value.
	T Get() {
		if (!count)
			return T(0);
		if (count < Size)
			return values[count - 1]; // unfiltered

		auto values_sort = values;
		std::sort(values_sort.begin(), values_sort.end());

		T avg(0);
		for (int n = Outliers; n < Size - Outliers; ++n)
			avg += values_sort[n];

		return avg / (Size - Outliers * 2);
	}

	/// Remove all values from the container.
	void Reset() { count = 0; }

private:
	int count{0};
	std::vector<T> values;
};

} // namespace hg
