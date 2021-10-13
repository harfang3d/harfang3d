// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/assert.h"
#include "foundation/vector_list.h"

namespace hg {

struct gen_ref {
	uint32_t idx{0xffffffff}, gen{0xffffffff};
};

static const gen_ref invalid_gen_ref;

inline bool operator==(gen_ref a, gen_ref b) { return a.idx == b.idx && a.gen == b.gen; }
inline bool operator!=(gen_ref a, gen_ref b) { return a.idx != b.idx || a.gen != b.gen; }

inline bool operator<(gen_ref a, gen_ref b) { return a.gen == b.gen ? a.idx < b.idx : a.gen < b.gen; }

template <typename T> class generational_vector_list : public vector_list<T> {
public:
	gen_ref add_ref(T &v) {
		auto idx = vector_list<T>::add(v);
		if (idx >= generations.size())
			generations.resize(size_t(idx) + 64);
		return {idx, generations[idx]};
	}

	gen_ref add_ref(T &&v) {
		auto idx = vector_list<T>::add(std::forward<T>(v));
		if (idx >= generations.size())
			generations.resize(size_t(idx) + 64);
		return {idx, generations[idx]};
	}

	void remove_ref(gen_ref ref) {
		if (is_valid(ref)) {
			++generations[ref.idx]; // increase generation for this index
			this->remove(ref.idx);
		}
	}

	gen_ref first_ref() const {
		auto idx = this->first();
		return {idx, idx == 0xffffffff ? 0xffffffff : generations[idx]};
	}

	gen_ref next_ref(gen_ref ref) const {
		auto idx = this->next(ref.idx);
		return {idx, idx == 0xffffffff ? 0xffffffff : generations[idx]};
	}

	T &get_safe(gen_ref ref, T &dflt) { return is_valid(ref) ? (*this)[ref.idx] : dflt; }
	const T &get_safe(gen_ref ref, const T &dflt) const { return is_valid(ref) ? (*this)[ref.idx] : dflt; }

	gen_ref get_ref(uint32_t idx) const { return this->is_used(idx) && idx < generations.size() ? gen_ref{idx, generations[idx]} : invalid_gen_ref; }

	bool is_valid(gen_ref ref) const { return this->is_used(ref.idx) && ref.idx < generations.size() && ref.gen == generations[ref.idx]; }

	void clear() {
		vector_list<T>::clear();
		generations.clear();
	}

private:
	std::vector<uint32_t> generations;
};

} // namespace hg

namespace std {

template <> struct hash<hg::gen_ref> {
	size_t operator()(const hg::gen_ref &ref) const {
		// FNV
		auto h = ref.idx, x = ref.gen;
		for (int i = 0; i < 4; ++i) {
			h ^= x & 255;
			x >>= 8;
			h = (h << 24) + h * 0x193;
		}
		return h;
	}
};

} // namespace std
