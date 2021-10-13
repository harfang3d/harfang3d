// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once;

#include "foundation/assert.h"
#include "foundation/intrusive_shared_ptr_st.h"
#include "foundation/generational_vector_list.h"

namespace hg {

using ref_counted_vector_list_remove_idx = void (*)(void *list, uint32_t idx);

struct control_block {
	control_block(void *list, ref_counted_vector_list_remove_idx remove_op) : list_(list), remove_op_(remove_op) {}

	inline void fit_T_ref_count_(size_t idx) {
		if (idx >= T_ref_count.size())
			T_ref_count.resize(idx + 16, 0);
	}

	inline void add_ref_(uint32_t idx) {
		__ASSERT__(idx != 0xffffffff);
		fit_T_ref_count_(idx);
		++T_ref_count[idx];
	}

	inline void remove_ref_(uint32_t idx) {
		__ASSERT__(idx != 0xffffffff && idx < T_ref_count.size() && T_ref_count[idx] > 0);
		if (--T_ref_count[idx] == 0)
			if (list_)
				remove_op_(list_, idx);
	}

	uint32_t ref_count{0};

	void *list_;
	ref_counted_vector_list_remove_idx remove_op_;

	std::vector<uint32_t> T_ref_count;
};

struct counted_ref {
	inline counted_ref() = default;
	inline counted_ref(intrusive_shared_ptr_st<control_block> cb, uint32_t idx) : cb_(std::move(cb)), idx_(idx) { acquire(); }
	inline counted_ref(counted_ref &&ref) : cb_(std::forward<intrusive_shared_ptr_st<control_block>>(ref.cb_)), idx_(ref.idx_) {
		ref.idx_ = 0xffffffff;
		ref.cb_.reset();
	}
	inline counted_ref(const counted_ref &ref) : cb_(ref.cb_), idx_(ref.idx_) { acquire(); }

	inline ~counted_ref() { release(); }

	inline uint32_t idx() const { return idx_; }

	inline counted_ref &operator=(const counted_ref &ref) {
		release();
		cb_ = ref.cb_;
		idx_ = ref.idx_;
		acquire();
		return *this;
	}

	inline counted_ref &operator=(counted_ref &&ref) {
		release();
		cb_ = std::move(ref.cb_);
		idx_ = ref.idx_;
		ref.idx_ = 0xffffffff;
		return *this;
	}

private:
	intrusive_shared_ptr_st<control_block> cb_;

	uint32_t idx_{0xffffffff};

	inline void acquire() const {
		if (idx_ != 0xffffffff)
			cb_->add_ref_(idx_);
	}

	inline void release() const {
		if (idx_ != 0xffffffff)
			cb_->remove_ref_(idx_);
	}
};

inline bool operator==(const counted_ref &a, const counted_ref &b) { return a.idx() == b.idx(); }
inline bool operator!=(const counted_ref &a, const counted_ref &b) { return a.idx() != b.idx(); }

static const counted_ref invalid_counted_ref;

template <typename T> class ref_counted_vector_list : public vector_list<T> {
public:
	static void remove_idx(void *list, uint32_t idx) { reinterpret_cast<ref_counted_vector_list *>(list)->remove(idx); }

	ref_counted_vector_list() : cb(new control_block(this, remove_idx)) {}
	~ref_counted_vector_list() { cb->list_ = nullptr; }

	inline counted_ref add_ref(T &&v) { return {cb, add(std::forward<T>(v))}; }
	inline counted_ref get_ref(uint32_t idx) const { return {cb, idx}; }

	inline bool is_valid(const counted_ref &ref) const { return ref.cb_ == cb; }

private:
	intrusive_shared_ptr_st<control_block> cb;
};

} // namespace hg
