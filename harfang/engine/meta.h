// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/rw_interface.h"

#include <json/json_fwd.hpp>

namespace hg {

using json = nlohmann::json;

bool GetJsonValue(const json &js, const std::string &key, std::string &value);
bool GetJsonValue(const json &js, const std::string &key, bool &value);
bool GetJsonValue(const json &js, const std::string &key, int &value);
bool GetJsonValue(const json &js, const std::string &key, float &value);

void SetJsonValue(json &js, const std::string &key, const std::string &value);
void SetJsonValue(json &js, const std::string &key, const char *value);
void SetJsonValue(json &js, const std::string &key, bool value);
void SetJsonValue(json &js, const std::string &key, int value);
void SetJsonValue(json &js, const std::string &key, float value);

json LoadJson(const Reader &ir, const Handle &h, bool *result = nullptr);
json LoadJsonFromFile(const char *path, bool *result = nullptr);
json LoadJsonFromAssets(const char *name, bool *result = nullptr);

bool SaveJsonToFile(const json &js, const char *path);

//
bool GetMetaValue(const json &js, const std::string &key, std::string &value, const std::string &profile = "default");
bool GetMetaValue(const json &js, const std::string &key, bool &value, const std::string &profile = "default");
bool GetMetaValue(const json &js, const std::string &key, int &value, const std::string &profile = "default");
bool GetMetaValue(const json &js, const std::string &key, float &value, const std::string &profile = "default");

void SetMetaValue(json &js, const std::string &key, const std::string &value, const std::string &profile = "default");
void SetMetaValue(json &js, const std::string &key, const char *value, const std::string &profile = "default");
void SetMetaValue(json &js, const std::string &key, bool value, const std::string &profile = "default");
void SetMetaValue(json &js, const std::string &key, int value, const std::string &profile = "default");
void SetMetaValue(json &js, const std::string &key, float value, const std::string &profile = "default");

bool GetMetaTag(const json &js, const std::string &key, const json *&out, const std::string &profile = "default");

} // namespace hg
