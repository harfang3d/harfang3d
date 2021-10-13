// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

namespace hg {

/// Return smallest number of bits that can be used to represent a value.
int get_bit_count(int value);
/// Return the position in bit of the first non-zero bit.
int get_shift_count(int value);
/// Return the number of bit set in a given value.
int count_set_bit(int value);

void write_bit(void *ptr, unsigned offset_in_bit, unsigned bit_count, unsigned value);

unsigned read_bit(const void *ptr, unsigned offset_in_bit, unsigned bit_count);

} // namespace hg
