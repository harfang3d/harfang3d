// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

namespace hg {

template <typename T> struct intrusive_shared_ptr_st {
	inline intrusive_shared_ptr_st() = default;
	inline intrusive_shared_ptr_st(T *t) : t_(t) { acquire(); }
	inline intrusive_shared_ptr_st(const intrusive_shared_ptr_st<T> &o) : t_(o.t_) { acquire(); }
	inline intrusive_shared_ptr_st(intrusive_shared_ptr_st<T> &&o) : t_(o.t_) { o.t_ = nullptr; }
	inline ~intrusive_shared_ptr_st() { release(); }

	inline intrusive_shared_ptr_st<T> &operator=(const intrusive_shared_ptr_st<T> &o) {
		release();
		t_ = o.t_;
		acquire();
		return *this;
	}

	inline intrusive_shared_ptr_st<T> &operator=(intrusive_shared_ptr_st<T> &&o) {
		release();
		t_ = o.t_;
		o.t_ = nullptr;
		return *this;
	}

	inline bool operator==(const intrusive_shared_ptr_st &p) const { return t_ == p.t_; }
	inline bool operator!=(const intrusive_shared_ptr_st &p) const { return t_ != p.t_; }

	inline T *operator->() const { return t_; }
	inline T *get() const { return t_; }

	inline operator bool() const { return t_ != nullptr; }

	inline T &operator[](const size_t i) const { return t_[i]; }

	inline void reset() {
		release();
		t_ = nullptr;
	}

private:
	T *t_{nullptr};

	inline void acquire() const {
		if (t_)
			++t_->ref_count;
	}
	inline void release() const {
		if (t_)
			if (--t_->ref_count == 0)
				delete t_;
	}
};

} // namespace hg
