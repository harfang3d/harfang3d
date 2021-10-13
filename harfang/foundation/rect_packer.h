// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/cext.h"
#include "foundation/assert.h"

#include <vector>

namespace hg {

/// Simple KD-Tree rect packer (sizeof(T) + 8 bytes per node).
template <class T>
struct RectPacker {
	typedef uint16_t split_index;

	static const split_index invalid_split = 32767;

	RectPacker() { splits.reserve(32); }
	RectPacker(const Rect<T> &rc) : root_rect(rc) { splits.reserve(32); }

	void Reset(const Rect<T> &rc) {
		root_rect = rc;
		splits.clear();
	}

	bool Pack(const tVec2<T> &size, Rect<T> &out) {
		if (!splits.size())
			return PackCell(size, root_rect, out) != invalid_split;
		return PackSplit(0, size, root_rect, out);
	}

	const Rect<T> &GetRootRect() const { return root_rect; }

private:
	enum Axis {
		X,
		Y
	};

	struct Split { // sizeof(T) + 4 bytes
		Split() : axis(X), leaf(1), left(invalid_split), right(invalid_split) {}
		Split(T p, Axis x) : pos(p), axis(x), leaf(0), left(invalid_split), right(invalid_split) {}

		T pos; // split position (belongs to left)

		uint8_t axis : 1; // 0 - X, 1 - Y
		uint8_t leaf : 1; // leaf split
		split_index left : 15; // or up
		split_index right : 15; // or down
	};

	split_index PackCell(const tVec2<T> &size, Rect<T> cell_rect, Rect<T> &out) {
		if (size.x > GetWidth(cell_rect) || size.y > GetHeight(cell_rect))
			return invalid_split;

		split_index res;
		if (cell_rect.sy + size.y < cell_rect.ey) {
			auto split_x = NewSplit(cell_rect.sy + size.y, X),
				 split_y = NewSplit(cell_rect.sx + size.x, Y);

			splits[split_x].left = split_y;
			splits[split_y].left = NewLeaf();
			res = split_x;
		} else {
			res = NewSplit(cell_rect.sx + size.x, Y);
			splits[res].left = NewLeaf();
		}

		out = {cell_rect.sx, cell_rect.sy, cell_rect.sx + size.x, cell_rect.sy + size.y};
		return res;
	}

	bool PackSplit(split_index idx, const tVec2<T> &size, Rect<T> cell_rect, Rect<T> &out) {
		// WARNING! Do not hold a pointer to a split when calling PackCell!
		if (splits[idx].leaf)
			return false;

		Rect<T> left_cell_rect, right_cell_rect;

		switch (splits[idx].axis) {
			case X:
				left_cell_rect = {cell_rect.sx, cell_rect.sy, cell_rect.ex, splits[idx].pos};
				right_cell_rect = {cell_rect.sx, splits[idx].pos + 1, cell_rect.ex, cell_rect.ey};
				break;

			case Y:
				left_cell_rect = {cell_rect.sx, cell_rect.sy, splits[idx].pos, cell_rect.ey};
				right_cell_rect = {splits[idx].pos + 1, cell_rect.sy, cell_rect.ex, cell_rect.ey};
				break;
		}

		if (splits[idx].left == invalid_split) {
			auto new_idx = PackCell(size, left_cell_rect, out);

			if (new_idx != invalid_split) {
				splits[idx].left = new_idx;
				return true;
			}
		} else {
			if (PackSplit(splits[idx].left, size, left_cell_rect, out))
				return true;
		}

		if (splits[idx].right == invalid_split) {
			auto new_idx = PackCell(size, right_cell_rect, out);

			if (new_idx != invalid_split) {
				splits[idx].right = new_idx;
				return true;
			}
		} else {
			if (PackSplit(splits[idx].right, size, right_cell_rect, out))
				return true;
		}

		return false;
	}

	split_index NewSplit(T pos, Axis axis) {
		splits.emplace_back(pos, axis);
		return numeric_cast<split_index>(splits.size() - 1);
	}

	split_index NewLeaf() {
		splits.emplace_back();
		return numeric_cast<split_index>(splits.size() - 1);
	}

	Rect<T> root_rect{};

	std::vector<Split> splits;
};

} // namespace hg
