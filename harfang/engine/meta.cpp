// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/meta.h"
#include "engine/assets_rw_interface.h"

#include "foundation/file.h"
#include "foundation/file_rw_interface.h"
#include "foundation/format.h"
#include "foundation/log.h"
#include "foundation/string.h"

#include <json.hpp>

namespace hg {

template <typename T> bool TestValueType(const json &js) { return false; }

template <> bool TestValueType<bool>(const json &js) { return js.is_boolean(); }
template <> bool TestValueType<int>(const json &js) { return js.is_number_integer(); }
template <> bool TestValueType<float>(const json &js) { return js.is_number_float(); }
template <> bool TestValueType<std::string>(const json &js) { return js.is_string(); }

//
template <typename T> bool GetJsonValueT(const json &js, const std::string &key, T &value) {
	const auto elms = split(key, "/");

	const auto *c_js = &js;
	for (const auto &elm : elms) {
		const auto i = c_js->find(elm);
		if (i == std::end(*c_js))
			return false;
		c_js = &(*i);
	}

	if (!TestValueType<T>(*c_js))
		return false;

	value = c_js->get<T>();
	return true;
}

bool GetJsonValue(const json &js, const std::string &key, bool &value) { return GetJsonValueT(js, key, value); }
bool GetJsonValue(const json &js, const std::string &key, int &value) { return GetJsonValueT(js, key, value); }
bool GetJsonValue(const json &js, const std::string &key, float &value) { return GetJsonValueT(js, key, value); }
bool GetJsonValue(const json &js, const std::string &key, std::string &value) { return GetJsonValueT(js, key, value); }

//
template <typename T> void SetJsonValueT(json &js, const std::string &key, const T &v) {
	const auto elms = split(key, "/");

	auto *c_js = &js;
	for (const auto &elm : elms) {
		auto i = c_js->find(elm);
		if (i == std::end(*c_js))
			c_js = &((*c_js)[elm]); // create key
		else
			c_js = &(*i);
	}

	*c_js = v;
}

void SetJsonValue(json &js, const std::string &key, const std::string &value) { SetJsonValueT(js, key, value); }
void SetJsonValue(json &js, const std::string &key, const char *value) { SetJsonValueT(js, key, std::string(value)); }
void SetJsonValue(json &js, const std::string &key, bool value) { SetJsonValueT(js, key, value); }
void SetJsonValue(json &js, const std::string &key, int value) { SetJsonValueT(js, key, value); }
void SetJsonValue(json &js, const std::string &key, float value) { SetJsonValueT(js, key, value); }

//
json LoadJson(const Reader &ir, const Handle &h, bool *result) {
	if (result)
		*result = false;

	json js;
	if (!ir.is_valid(h))
		return js;

	try {
		js = json::parse(LoadString(ir, h));
		if (result)
			*result = true;
	} catch (const json::parse_error &e) { warn(format("JSON error: %1").arg(e.what())); }

	return js;
}

json LoadJsonFromFile(const char *path, bool *result) { return LoadJson(g_file_reader, ScopedReadHandle(g_file_read_provider, path, true), result); }
json LoadJsonFromAssets(const char *name, bool *result) { return LoadJson(g_assets_reader, ScopedReadHandle(g_assets_read_provider, name, true), result); }

bool SaveJsonToFile(const json &js, const char *path) {
	const auto js_ = js.dump(1, '\t', true);
	return StringToFile(path, js_.c_str());
}

//
static const std::string profile_prefix = "profiles/";

bool GetMetaValue(const json &js, const std::string &key, std::string &value, const std::string &profile) {
	return GetJsonValue(js, profile_prefix + profile + "/" + key, value) || GetJsonValue(js, profile_prefix + "default/" + key, value);
}
bool GetMetaValue(const json &js, const std::string &key, bool &value, const std::string &profile) {
	return GetJsonValue(js, profile_prefix + profile + "/" + key, value) || GetJsonValue(js, profile_prefix + "default/" + key, value);
}
bool GetMetaValue(const json &js, const std::string &key, int &value, const std::string &profile) {
	return GetJsonValue(js, profile_prefix + profile + "/" + key, value) || GetJsonValue(js, profile_prefix + "default/" + key, value);
}
bool GetMetaValue(const json &js, const std::string &key, float &value, const std::string &profile) {
	return GetJsonValue(js, profile_prefix + profile + "/" + key, value) || GetJsonValue(js, profile_prefix + "default/" + key, value);
}

//
bool GetMetaTag(const json &js, const std::string &key, const json *&out, const std::string &profile) {
	out = nullptr;

	auto i_profiles = js.find("profiles");
	if (i_profiles == std::end(js))
		return false;

	auto i_profile = i_profiles->find(profile);
	if (i_profile == std::end(*i_profiles))
		return false;

	auto i_key = i_profile->find(key);
	if (i_key == std::end(*i_profile))
		return false;

	out = &(*i_key);
	return true;
}

//
void SetMetaValue(json &js, const std::string &key, const std::string &value, const std::string &profile) {
	SetJsonValue(js, profile_prefix + profile + "/" + key, value);
}
void SetMetaValue(json &js, const std::string &key, const char *value, const std::string &profile) {
	SetJsonValue(js, profile_prefix + profile + "/" + key, value);
}
void SetMetaValue(json &js, const std::string &key, bool value, const std::string &profile) { SetJsonValue(js, profile_prefix + profile + "/" + key, value); }
void SetMetaValue(json &js, const std::string &key, int value, const std::string &profile) { SetJsonValue(js, profile_prefix + profile + "/" + key, value); }
void SetMetaValue(json &js, const std::string &key, float value, const std::string &profile) { SetJsonValue(js, profile_prefix + profile + "/" + key, value); }

} // namespace hg
