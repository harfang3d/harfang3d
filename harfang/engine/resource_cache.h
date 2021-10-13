// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/cext.h"
#include "foundation/generational_vector_list.h"

#include <map>

namespace hg {

template <typename T> struct ResourceRef {
	gen_ref ref;
	bool operator==(const ResourceRef &o) const { return ref == o.ref; }
	bool operator!=(const ResourceRef &o) const { return ref != o.ref; }
};

template <typename T, typename R> class ResourceCache {
private:
	struct name_T {
		std::string name;
		T T_;
	};

public:
	ResourceCache(void (*destroy)(T &)) : _destroy(destroy) {}

	R Add(const char *name, const T &res) {
		auto i = name_to_ref.find(name);
		if (i != std::end(name_to_ref))
			return i->second;

		R ref = {resources.add_ref({name, res})};
		name_to_ref[name] = ref;
		return ref;
	}

	R Add(const char *name, T &&res) {
		auto i = name_to_ref.find(name);
		if (i != std::end(name_to_ref))
			return i->second;

		R ref = {resources.add_ref({name, std::move(res)})};
		name_to_ref[name] = ref;
		return ref;
	}

	void Update(R ref, const T &res) {
		if (resources.is_valid(ref.ref)) {
			_destroy(resources[ref.ref.idx].T_);
			resources[ref.ref.idx].T_ = res;
		}
	}

	void Update(R ref, T &&res) {
		if (resources.is_valid(ref.ref)) {
			_destroy(resources[ref.ref.idx].T_);
			resources[ref.ref.idx].T_ = std::move(res);
		}
	}

	void Destroy(R ref) {
		if (resources.is_valid(ref.ref)) {
			_destroy(resources[ref.ref.idx].T_);
			name_to_ref.erase(resources[ref.ref.idx].name); // drop from cache
			resources.remove_ref(ref.ref);
		}
	}

	void DestroyAll() {
		for (auto &i : resources)
			_destroy(i.T_);
		resources.clear();
		name_to_ref.clear();
	}

	bool IsValidRef(R ref) const { return resources.is_valid(ref.ref); }

	// get a resource index for code that does not carry a full reference to the resource.
	uint16_t GetValidatedRefIndex(R ref) const { return resources.is_valid(ref.ref) ? numeric_cast<uint16_t>(ref.ref.idx) : 0xffff; }

	R Has(const char *name) const {
		auto i = name_to_ref.find(name);
		return i != std::end(name_to_ref) ? i->second : R{};
	}

	size_t GetCount() const { return resources.size(); }

	void SetName(R ref, const char *name) {
		if (resources.is_valid(ref.ref))
			resources[ref.ref.idx].name = name;
	}

	void SetName_unsafe_(uint16_t idx, const char *name) {
		if (idx != 0xffff) {
			__ASSERT__(resources.is_used(idx));
			resources[idx].name = name;
		}
	}

	std::string GetName(R ref) const { return resources.is_valid(ref.ref) ? resources[ref.ref.idx].name : std::string{}; }

	std::string GetName_unsafe_(uint16_t idx) const {
		if (idx != 0xffff) {
			__ASSERT__(resources.is_used(idx));
			return resources[idx].name;
		}
		return {};
	}

	const T &Get(R ref) const { return resources.is_valid(ref.ref) ? resources[ref.ref.idx].T_ : dflt; }

	const T &Get_unsafe_(uint16_t idx) const {
		if (idx != 0xffff) {
			__ASSERT__(resources.is_used(idx));
			return resources[idx].T_;
		}
		return dflt;
	}

	const T &Get(const char *name) const {
		auto i = name_to_ref.find(name);
		return i != std::end(name_to_ref) ? resources[i->second.idx].T_ : dflt;
	}

	T &Get(R ref) {
		__ASSERT__(IsValidRef(ref));
		return resources[ref.ref.idx].T_;
	}

	gen_ref first_ref() const { return resources.first_ref(); }
	gen_ref next_ref(gen_ref ref) const { return resources.next_ref(ref); }

private:
	T dflt;

	generational_vector_list<name_T> resources;
	std::map<const std::string, R> name_to_ref;

	void (*_destroy)(T &) = nullptr;
};

} // namespace hg
