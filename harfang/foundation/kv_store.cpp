// Key-value store

#include "foundation/kv_store.h"
#include "foundation/file_rw_interface.h"

#include <cstdio>
#include <numeric>

namespace hg {

bool KeyValueStore::Open(const Reader &ir, const Handle &h) {
	if (!ir.is_valid(h))
		return false;

	std::map<std::string, std::string> kvs_;

	std::string key, value;

	enum parser_state { prelude_key, parse_key, separator, prelude_value, parse_value, commit };

	parser_state state = prelude_key;

	bool res = true, eof = false;
	while (res && !eof) {
		char c;

		while (res && (state != commit) && !eof) {
			switch (state) {
				case prelude_key:
				case prelude_value:
					while (ir.read(h, &c, 1) == 1)
						if (c == '"') {
							state = state == prelude_key ? parse_key : parse_value;
							break;
						}

					if (ir.is_eof(h)) {
						if (state == prelude_key)
							eof = true;
						else
							res = false;
					}
					break;

				case parse_key:
				case parse_value:
					while (ir.read(h, &c, 1) == 1) {
						if (c == '"') {
							state = state == parse_key ? separator : commit;
							break;
						} else {
							if (state == parse_key)
								key += c;
							else
								value += c;
						}
					}

					if ((c != '"') || ir.is_eof(h))
						res = false;
					break;

				case separator:
					while (ir.read(h, &c, 1) == 1)
						if (c == ':') {
							state = prelude_value;
							break;
						}

					if ((c != ':') || ir.is_eof(h))
						res = false;
					break;
			}
		}

		if (res && !eof) {
			kvs_[key] = value;

			key.clear();
			value.clear();

			state = prelude_key;
		}
	}

	if (res || (state == prelude_key))
		kvs = std::move(kvs_);
	return res;
}

bool KeyValueStore::Open(const char *path, bool silent) { return Open(g_file_reader, ScopedReadHandle(g_file_read_provider, path, silent)); }

bool KeyValueStore::Save(const char *path) {
	FILE *f = fopen(path, "w");
	if (!f) {
		perror(path);
		return false;
	}
	for (auto &i : kvs)
		fprintf(f, "\"%s\":\"%s\"\n", i.first.c_str(), i.second.c_str());
	fclose(f);
	return true;
}

//
bool KeyValueStore::Get(const std::string &key, std::string &value) const {
	auto i = kvs.find(prefix + key);
	if (i == std::end(kvs))
		return false;
	value = i->second;
	return true;
}

bool KeyValueStore::Set(const std::string &key, const std::string &value) {
	kvs[prefix + key] = value;
	return true;
}

bool KeyValueStore::Set(const std::string &key, const char *value) {
	kvs[prefix + key] = value;
	return true;
}

//
bool KeyValueStore::Clear(const std::string &key) {
	const auto &i = kvs.find(prefix + key);
	if (i == std::end(kvs))
		return false;
	kvs.erase(i);
	return true;
}

//
bool KeyValueStore::Get(const std::string &key, int &value) const {
	std::string v;
	if (!Get(key, v))
		return false;
	value = std::atoi(v.c_str());
	return true;
}

bool KeyValueStore::Get(const std::string &key, float &value) const {
	std::string v;
	if (!Get(key, v))
		return false;
	value = float(std::atof(v.c_str()));
	return true;
}

bool KeyValueStore::Get(const std::string &key, bool &value) const {
	std::string v;
	if (!Get(key, v))
		return false;
	value = (v == "1") || (v == "true") || (v == "TRUE") || v == ("True");
	return true;
}

//
bool KeyValueStore::Set(const std::string &key, int value) { return Set(key, std::to_string(value)); }
bool KeyValueStore::Set(const std::string &key, float value) { return Set(key, std::to_string(value)); }
bool KeyValueStore::Set(const std::string &key, bool value) { return Set(key, std::string(value ? "1" : "0")); }

//
std::vector<std::string> KeyValueStore::FindValue(const std::string &value) const {
	std::vector<std::string> out;
	for (auto &i : kvs)
		if (i.second == value)
			out.push_back(i.first);
	return out;
}

void KeyValueStore::CommitPrefix() { prefix = std::accumulate(std::begin(prefix_stack), std::end(prefix_stack), std::string(), std::plus<std::string>()); }

void KeyValueStore::PushPrefix(const std::string &prefix) {
	prefix_stack.push_back(prefix + "|");
	CommitPrefix();
}

void KeyValueStore::PopPrefix() {
	prefix_stack.pop_back();
	CommitPrefix();
}

void KeyValueStore::Close() { kvs.clear(); }

} // namespace hg
