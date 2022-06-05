// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/scene.h"

#include <json/json.hpp>

namespace hg {

NLOHMANN_JSON_SERIALIZE_ENUM(LightType, {{LT_Point, "point"}, {LT_Spot, "spot"}, {LT_Linear, "linear"}});
NLOHMANN_JSON_SERIALIZE_ENUM(LightShadowType, {{LST_None, "none"}, {LST_Map, "map"}});
NLOHMANN_JSON_SERIALIZE_ENUM(AnimLoopMode, {{ALM_Once, "none"}, {ALM_Infinite, "infinite"}, {ALM_Loop, "loop"}});
NLOHMANN_JSON_SERIALIZE_ENUM(ProbeType, {{PT_Sphere, "sphere"}, {PT_Cube, "cube"}});

inline void to_json(json &j, const Vec2 &v) { j = {v.x, v.y}; }
inline void from_json(const json &j, Vec2 &v) { v = {j.at(0), j.at(1)}; }

inline void to_json(json &j, const Vec3 &v) { j = {v.x, v.y, v.z}; }
inline void from_json(const json &j, Vec3 &v) { v = {j.at(0), j.at(1), j.at(2)}; }

inline void to_json(json &j, const Vec4 &v) { j = {v.x, v.y, v.z, v.w}; }
inline void from_json(const json &j, Vec4 &v) { v = {j.at(0), j.at(1), j.at(2), j.at(3)}; }

inline void to_json(json &j, const Quaternion &v) { j = {v.x, v.y, v.z, v.w}; }
inline void from_json(const json &j, Quaternion &v) { v = {j.at(0), j.at(1), j.at(2), j.at(3)}; }

inline void to_json(json &j, const Color &v) { j = {int(v.r * 255.f), int(v.g * 255.f), int(v.b * 255.f), int(v.a * 255.f)}; }
inline void from_json(const json &j, Color &v) {
	v = {j.at(0).get<float>() / 255.f, j.at(1).get<float>() / 255.f, j.at(2).get<float>() / 255.f, j.at(3).get<float>() / 255.f};
}

inline void to_json(json &j, const CameraZRange &v) { j = {{"znear", v.znear}, {"zfar", v.zfar}}; }
inline void from_json(const json &j, CameraZRange &v) { v = {j.at("znear"), j.at("zfar")}; }

inline void to_json(json &j, const gen_ref &v) {
	if (v != invalid_gen_ref)
		j = v.idx;
	else
		j = nullptr;
}

inline void from_json(const json &j, gen_ref &v) {
	if (j.is_null())
		v = invalid_gen_ref;
	else
		v.idx = j;
}

NLOHMANN_JSON_SERIALIZE_ENUM(RigidBodyType, {{RBT_Dynamic, "dynamic"}, {RBT_Kinematic, "kinematic"}});
NLOHMANN_JSON_SERIALIZE_ENUM(
	CollisionType, {{CT_Sphere, "sphere"}, {CT_Cube, "cube"}, {CT_Cone, "cone"}, {CT_Capsule, "capsule"}, {CT_Cylinder, "cylinder"}, {CT_Mesh, "mesh"}});

inline void to_json(json &j, const Mat4 &v) {
	j = {v.m[0][0], v.m[0][1], v.m[0][2], v.m[0][3], v.m[1][0], v.m[1][1], v.m[1][2], v.m[1][3], v.m[2][0], v.m[2][1], v.m[2][2], v.m[2][3]};
}

inline void from_json(const json &j, Mat4 &v) {
	v.m[0][0] = j.at(0);
	v.m[0][1] = j.at(1);
	v.m[0][2] = j.at(2);
	v.m[0][3] = j.at(3);
	v.m[1][0] = j.at(4);
	v.m[1][1] = j.at(5);
	v.m[1][2] = j.at(6);
	v.m[1][3] = j.at(7);
	v.m[2][0] = j.at(8);
	v.m[2][1] = j.at(9);
	v.m[2][2] = j.at(10);
	v.m[2][3] = j.at(11);
}

inline void to_json(json &j, const Mat44 &v) {
	j = {v.m[0][0], v.m[0][1], v.m[0][2], v.m[0][3], v.m[1][0], v.m[1][1], v.m[1][2], v.m[1][3], v.m[2][0], v.m[2][1], v.m[2][2], v.m[2][3], v.m[3][0],
		v.m[3][1], v.m[3][2], v.m[3][3]};
}

inline void from_json(const json &j, Mat44 &v) {
	v.m[0][0] = j.at(0);
	v.m[0][1] = j.at(1);
	v.m[0][2] = j.at(2);
	v.m[0][3] = j.at(3);
	v.m[1][0] = j.at(4);
	v.m[1][1] = j.at(5);
	v.m[1][2] = j.at(6);
	v.m[1][3] = j.at(7);
	v.m[2][0] = j.at(8);
	v.m[2][1] = j.at(9);
	v.m[2][2] = j.at(10);
	v.m[2][3] = j.at(11);
	v.m[3][0] = j.at(12);
	v.m[3][1] = j.at(13);
	v.m[3][2] = j.at(14);
	v.m[3][3] = j.at(15);
}

//
template <typename T> void js_at_safe(const json &js, const std::string &key, T &v) {
	const auto &i = js.find(key);
	if (i != std::end(js))
		v = i->get<T>();
}

} // namespace hg
