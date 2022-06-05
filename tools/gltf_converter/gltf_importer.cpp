// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#include <engine/animation.h>
#include <engine/geometry.h>
#include <engine/model_builder.h>
#include <engine/node.h>
#include <engine/physics.h>
#include <engine/render_pipeline.h>
#include <engine/scene.h>

#include <foundation/build_info.h>
#include <foundation/cext.h>
#include <foundation/cmd_line.h>
#include <foundation/dir.h>
#include <foundation/format.h>
#include <foundation/log.h>
#include <foundation/math.h>
#include <foundation/matrix3.h>
#include <foundation/matrix4.h>
#include <foundation/pack_float.h>
#include <foundation/path_tools.h>
#include <foundation/projection.h>
#include <foundation/sha1.h>
#include <foundation/string.h>
#include <foundation/time.h>
#include <foundation/vector3.h>

#include "json.hpp"
#include "stb_image.h"
#include "tiny_gltf.h"

#include <fstream>
#include <iostream>
#include <mutex>

#undef CopyFile
#undef GetObject

#include <foundation/file.h>

using namespace tinygltf;
using nlohmann::json;

std::map<int, hg::NodeRef> idNode_to_NodeRef;
std::vector<std::string> already_saved_picture;

std::map<std::string, std::string> primitiveIdsToGeoPath;
std::map<std::string, int> geoPathOcurrence;

static std::string Indent(const int indent) {
	std::string s;
	for (int i = 0; i < indent; i++) {
		s += "  ";
	}
	return s;
}

/// Adapts an array of bytes to an array of T. Will advace of byte_stride each
/// elements.
template <typename T> struct arrayAdapter {
	/// Pointer to the bytes
	const unsigned char *dataPtr;
	/// Number of elements in the array
	const size_t elemCount;
	/// Stride in bytes between two elements
	const size_t stride;

	/// Construct an array adapter.
	/// \param ptr Pointer to the start of the data, with offset applied
	/// \param count Number of elements in the array
	/// \param byte_stride Stride betweens elements in the array
	arrayAdapter(const unsigned char *ptr, size_t count, size_t byte_stride) : dataPtr(ptr), elemCount(count), stride(byte_stride) {}

	/// Returns a *copy* of a single element. Can't be used to modify it.
	T operator[](size_t pos) const {
		if (pos >= elemCount)
			throw std::out_of_range("Tried to access beyond the last element of an array adapter with "
									"count " +
									std::to_string(elemCount) + " while getting elemnet number " + std::to_string(pos));
		return *(reinterpret_cast<const T *>(dataPtr + pos * stride));
	}
};

/// Interface of any adapted array that returns byte data
struct byteArrayBase {
	virtual ~byteArrayBase() = default;
	virtual unsigned char operator[](size_t) const = 0;
	virtual size_t size() const = 0;
};

/// Interface of any adapted array that returns integer data
struct intArrayBase {
	virtual ~intArrayBase() = default;
	virtual unsigned int operator[](size_t) const = 0;
	virtual size_t size() const = 0;
};

/// Interface of any adapted array that returns float data
struct floatArrayBase {
	virtual ~floatArrayBase() = default;
	virtual float operator[](size_t) const = 0;
	virtual size_t size() const = 0;
};

/// An array that loads interger types, returns them as byte
template <class T> struct byteArray : public byteArrayBase {
	arrayAdapter<T> adapter;

	explicit byteArray(const arrayAdapter<T> &a) : adapter(a) {}
	unsigned char operator[](size_t position) const override { return static_cast<unsigned char>(adapter[position]); }

	size_t size() const override { return adapter.elemCount; }
};

/// An array that loads interger types, returns them as int
template <class T> struct intArray : public intArrayBase {
	arrayAdapter<T> adapter;

	explicit intArray(const arrayAdapter<T> &a) : adapter(a) {}
	unsigned int operator[](size_t position) const override { return static_cast<unsigned int>(adapter[position]); }

	size_t size() const override { return adapter.elemCount; }
};

template <class T> struct floatArray : public floatArrayBase {
	arrayAdapter<T> adapter;

	explicit floatArray(const arrayAdapter<T> &a) : adapter(a) {}
	float operator[](size_t position) const override { return static_cast<float>(adapter[position]); }

	size_t size() const override { return adapter.elemCount; }
};

struct v2fArray {
	arrayAdapter<hg::Vec2> adapter;
	explicit v2fArray(const arrayAdapter<hg::Vec2> &a) : adapter(a) {}

	hg::Vec2 operator[](size_t position) const { return adapter[position]; }
	size_t size() const { return adapter.elemCount; }
};

struct v3fArray {
	arrayAdapter<hg::Vec3> adapter;
	explicit v3fArray(const arrayAdapter<hg::Vec3> &a) : adapter(a) {}

	hg::Vec3 operator[](size_t position) const { return adapter[position]; }
	size_t size() const { return adapter.elemCount; }
};

struct v4fArray {
	arrayAdapter<hg::Vec4> adapter;
	explicit v4fArray(const arrayAdapter<hg::Vec4> &a) : adapter(a) {}

	hg::Vec4 operator[](size_t position) const { return adapter[position]; }
	size_t size() const { return adapter.elemCount; }
};

struct m44fArray {
	arrayAdapter<hg::Mat44> adapter;
	explicit m44fArray(const arrayAdapter<hg::Mat44> &a) : adapter(a) {}

	hg::Mat44 operator[](size_t position) const { return adapter[position]; }
	size_t size() const { return adapter.elemCount; }
};

enum class ImportPolicy { SkipExisting, Overwrite, Rename, SkipAlways };

struct Config {
	ImportPolicy import_policy_geometry{ImportPolicy::SkipExisting}, import_policy_material{ImportPolicy::SkipExisting},
		import_policy_texture{ImportPolicy::SkipExisting}, import_policy_scene{ImportPolicy::SkipExisting}, import_policy_anim{ImportPolicy::SkipExisting};

	std::string input_path;
	std::string name; // output name (may be empty)
	std::string base_output_path{"./"};
	std::string prj_path;
	std::string prefix;
	std::string shader;

	float geometry_scale{1.f};
	int frame_per_second{24};

	bool import_animation{true};
	bool recalculate_normal{false}, recalculate_tangent{false};

	std::string finalizer_script;
};

static bool GetOutputPath(
	std::string &path, const std::string &base, const std::string &name, const std::string &prefix, const std::string &ext, ImportPolicy import_policy) {
	if (base.empty())
		return false;

	const auto filename = name.empty() ? prefix : (prefix.empty() ? name : prefix + "-" + name);
	path = hg::CleanPath(base + "/" + filename + "." + ext);

	// check folder exists and created
	hg::MkTree(hg::CutFileName(path).c_str());

	switch (import_policy) {
		default:
			return false;

		case ImportPolicy::SkipAlways:
			return false; // WARNING do not move to the start of the function, we need the path for the resource even if it is not exported

		case ImportPolicy::SkipExisting:
			if (hg::Exists(path.c_str()))
				return false;
			break;

		case ImportPolicy::Overwrite:
			return true;

		case ImportPolicy::Rename:
			for (auto n = 0; hg::Exists(path.c_str()) && n < 10000; ++n) {
				std::ostringstream ss;
				ss << base << "/" << filename << "-" << std::setw(4) << std::setfill('0') << n << "." << ext;
				path = ss.str();
			}
			break;
	}
	return true;
}

//
static std::string MakeRelativeResourceName(const std::string &name, const std::string &base_path, const std::string &prefix) {
	if (hg::starts_with(name, base_path, hg::case_sensitivity::insensitive)) {
		const auto stripped_name = hg::lstrip(hg::slice(name, base_path.length()), "/");
		return prefix.empty() ? stripped_name : prefix + "/" + stripped_name;
	}
	return name;
}

static bool LoadFinalizerScript(const std::string &path) {
	const auto source = hg::FileToString(path.c_str());

	if (source.empty())
		return false;

	//	if (!hg_lua_bind_harfang(vm, "hg") || !hg::Execute(vm, source, path))
	//		return false;

	return true;
}

static void FinalizeMaterial(hg::Material &mat, const std::string &name, const std::string &geo_name) {
	/*
	if (vm->IsOpen()) {
	auto fn = vm->Get("FinalizeMaterial");
	if (fn.IsValidAndNonNull())
	if (!vm->Call(fn, {vm->ValueToObject(hg::TypeValue(mat)), vm->CreateObject(name), vm->CreateObject(geo_name)}))
	failed = true;
	}
	*/
}

static void FinalizeGeometry(hg::Geometry &geo, const std::string &name) {
	/*
	if (vm) {
	auto fn = ng::Get(vm, "FinalizeModel");
	if (fn.IsValidAndNonNull())
	if (!ng::Call(vm, fn, {ng::ValueToObject(vm, geo), ng::ValueToObject(vm, name)}))
	failed = true;
	}
	*/
}

static void FinalizeNode(hg::Node &node) {
	/*
	if (vm->IsOpen()) {
	auto fn = vm->Get("FinalizeNode");
	if (fn.IsValidAndNonNull())
	if (!vm->Call(fn, {vm->ValueToObject(hg::TypeValue(node))}))
	failed = true;
	}
	*/
}

static void FinalizeScene(hg::Scene &scene) {
	/*
	if (vm->IsOpen()) {
	auto fn = vm->Get("FinalizeScene");
	if (fn.IsValidAndNonNull())
	if (!vm->Call(fn, {vm->ValueToObject(hg::TypeValue(scene))}))
	failed = true;
	}
	*/
}

//
static void ExportMotions(const Model &model, const Scene &gltf_scene, hg::Scene &scene, const Config &config) {
	if (!config.import_animation)
		return;

	// all animations
	hg::debug(hg::format("animations(items=%1)").arg(model.animations.size()).c_str());
	for (size_t i = 0; i < model.animations.size(); i++) {
		const tinygltf::Animation &animation = model.animations[i];
		hg::debug(hg::format(Indent(1) + "name         : %1").arg(animation.name.empty() ? "anim_" + std::to_string(i) : animation.name).c_str());

		hg::SceneAnim scene_anim{animation.name.empty() ? "anim_" + std::to_string(i) : animation.name, hg::time_from_sec(99999), hg::time_from_sec(-99999)};

		std::vector<hg::AnimRef> anims;

		// add animations samplers
		hg::debug(hg::format(Indent(1) + "samplers(items=%1)").arg(animation.samplers.size()).c_str());
		for (size_t j = 0; j < animation.samplers.size(); j++) {
			const tinygltf::AnimationSampler &sampler = animation.samplers[j];
			hg::debug(hg::format(Indent(2) + "input         : %1").arg(sampler.input).c_str());
			hg::debug(hg::format(Indent(2) + "interpolation : %1").arg(sampler.interpolation).c_str());
			hg::debug(hg::format(Indent(2) + "output        : %1").arg(sampler.output).c_str());

			// input (usually the time) Get the good buffer from all the gltf micmac
			const auto inputAccessor = model.accessors[sampler.input];
			const auto &inputBufferView = model.bufferViews[inputAccessor.bufferView];
			const auto &inputBuffer = model.buffers[inputBufferView.buffer];
			const auto inputDataPtr = inputBuffer.data.data() + inputBufferView.byteOffset + inputAccessor.byteOffset;
			const auto input_byte_stride = inputAccessor.ByteStride(inputBufferView);
			const auto input_count = inputAccessor.count;

			//	hg::debug(hg::format("input attribute has count %1 and stride %2 bytes").arg(input_count).arg(input_byte_stride).c_str());

			// output (value for the key) Get the good buffer from all the gltf micmac
			const auto outputAccessor = model.accessors[sampler.output];
			const auto &outputBufferView = model.bufferViews[outputAccessor.bufferView];
			const auto &outputBuffer = model.buffers[outputBufferView.buffer];
			const auto outputDataPtr = outputBuffer.data.data() + outputBufferView.byteOffset + outputAccessor.byteOffset;
			const auto output_byte_stride = outputAccessor.ByteStride(outputBufferView);
			const auto output_count = outputAccessor.count;

			//	hg::debug(hg::format("output attribute has count %1 and stride %2 bytes").arg(output_count).arg(output_byte_stride).c_str());

			const auto anim_ref = scene.AddAnim({});
			anims.push_back(anim_ref);
			auto anim = scene.GetAnim(anim_ref);

			// get channel type anim
			std::string target_path = "";
			for (size_t k = 0; k < animation.channels.size(); k++)
				if (animation.channels[k].sampler == j)
					target_path = animation.channels[j].target_path;

			// Transfers the value and time from the buffer to harfang depending of the type
			switch (inputAccessor.type) {
				case TINYGLTF_TYPE_SCALAR: {
					switch (inputAccessor.componentType) {
						case TINYGLTF_COMPONENT_TYPE_FLOAT: {
							hg::debug("Input type is FLOAT");
							// vector of float
							floatArray<float> time(arrayAdapter<float>(inputDataPtr, input_count, input_byte_stride));

							hg::debug(hg::format("time's size : %1").arg(time.size()).c_str());

							switch (outputAccessor.type) {
								case TINYGLTF_TYPE_VEC4: {
									switch (outputAccessor.componentType) {
										case TINYGLTF_COMPONENT_TYPE_FLOAT: {
											hg::debug("Output type is FLOAT");

											v4fArray value(arrayAdapter<hg::Vec4>(outputDataPtr, output_count, output_byte_stride));

											hg::debug(hg::format("value's size : %1").arg(value.size()).c_str());
											anim->quat_tracks.push_back({"NO_MATCH", {}});
											anim->flags |= hg::AF_UseQuaternionForRotation;

											for (size_t k{0}; k < time.size(); ++k) {
												auto t = hg::time_from_sec_f(time[k]);
												auto v = value[k];
												// hg::debug(hg::format("t[%1]: (%2), v[%3]: (%4, %5, %6,
												// %7)").arg(k).arg(t).arg(k).arg(v.x).arg(v.y).arg(v.z).arg(v.w).c_str());
												auto r = hg::ToEuler(hg::Quaternion(v.x, v.y, v.z, v.w));
												hg::SetKey(anim->quat_tracks.back(), t, hg::QuaternionFromEuler(-r.x, -r.y, r.z));
											}
											anim->t_start = hg::time_from_sec_f(time[0]);
											anim->t_end = hg::time_from_sec_f(time[time.size() - 1]);

											scene_anim.t_start = hg::Min(scene_anim.t_start, anim->t_start);
											scene_anim.t_end = hg::Max(scene_anim.t_end, anim->t_end);
										} break;
										default:
											hg::error("Error: Animation values needs to be Float (else not implemented)");
											break;
									}

								} break;

								case TINYGLTF_TYPE_VEC3: {
									switch (outputAccessor.componentType) {
										case TINYGLTF_COMPONENT_TYPE_FLOAT: {
											hg::debug("Output type is FLOAT");

											v3fArray value(arrayAdapter<hg::Vec3>(outputDataPtr, output_count, output_byte_stride));

											hg::debug(hg::format("value's size : %1").arg(value.size()).c_str());
											anim->vec3_tracks.push_back({"NO_MATCH", {}});

											for (size_t k{0}; k < time.size(); ++k) {
												auto t = hg::time_from_sec_f(time[k]);
												auto v = value[k];

												// fix only for pos
												if (target_path == "translation")
													v.z = -v.z;

												// hg::debug(hg::format("t[%1]: (%2), v[%3]: (%4, %5,
												// %6)").arg(k).arg(t).arg(k).arg(v.x).arg(v.y).arg(v.z).c_str());
												hg::SetKey(anim->vec3_tracks.back(), t, v);
											}
											anim->t_start = hg::time_from_sec_f(time[0]);
											anim->t_end = hg::time_from_sec_f(time[time.size() - 1]);

											scene_anim.t_start = hg::Min(scene_anim.t_start, anim->t_start);
											scene_anim.t_end = hg::Max(scene_anim.t_end, anim->t_end);
										} break;
										default:
											hg::error("Error: Animation values needs to be Float (else not implemented)");
											break;
									}

								} break;
							}
						} break;
						default:
							hg::error("Error: Time values needs to be Float (else not implemented)");
							break;
					}
					break;
				}
			}
		}

		// add animation channel, link from the anim track made above to the type of animaton and to the node
		hg::debug((Indent(1) + "channels : [ ").c_str());
		for (size_t j = 0; j < animation.channels.size(); j++) {
			hg::debug(hg::format(Indent(2) + "sampler     : %1").arg(animation.channels[j].sampler).c_str());
			hg::debug(hg::format(Indent(2) + "target.id   : %1").arg(animation.channels[j].target_node).c_str());
			hg::debug(hg::format(Indent(2) + "target.path : %1").arg(animation.channels[j].target_path).c_str());
			hg::debug((j != (animation.channels.size() - 1)) ? "  , " : "");

			if (animation.channels[j].sampler < anims.size()) {
				auto anim_ref = anims[animation.channels[j].sampler];
				auto anim = scene.GetAnim(anim_ref);

				scene_anim.node_anims.push_back({idNode_to_NodeRef[animation.channels[j].target_node], anim_ref});

				std::string target;
				if (animation.channels[j].target_path == "translation")
					target = "Position";
				else if (animation.channels[j].target_path == "rotation")
					target = "Rotation";
				else if (animation.channels[j].target_path == "scale")
					target = "Scale";
				else if (animation.channels[j].target_path == "weights")
					target = "Weights";

				// find which track is not empty
				if (anim->bool_tracks.size())
					anim->bool_tracks.back().target = target;
				else if (anim->int_tracks.size())
					anim->int_tracks.back().target = target;
				else if (anim->float_tracks.size())
					anim->float_tracks.back().target = target;
				else if (anim->vec2_tracks.size())
					anim->vec2_tracks.back().target = target;
				else if (anim->vec3_tracks.size())
					anim->vec3_tracks.back().target = target;
				else if (anim->vec4_tracks.size())
					anim->vec4_tracks.back().target = target;
				else if (anim->quat_tracks.size())
					anim->quat_tracks.back().target = target;
			}
		}
		hg::debug("  ]");

		scene.AddSceneAnim(scene_anim);
	}
}

//
static void ExportSkins(const Model &model, const Scene &gltf_scene, hg::Scene &scene, const Config &config) {
	// all skins
	hg::debug(hg::format("skin(items=%1)").arg(model.skins.size()).c_str());
	for (size_t gltf_id_node = 0; gltf_id_node < model.nodes.size(); ++gltf_id_node) {
		const auto &gltf_node = model.nodes[gltf_id_node];
		if (gltf_node.skin >= 0) {
			const auto &skin = model.skins[gltf_node.skin];
			auto node = scene.GetNode(idNode_to_NodeRef[gltf_id_node]);
			if (auto object = node.GetObject()) {
				object.SetBoneCount(skin.joints.size());
				for (size_t j = 0; j < skin.joints.size(); j++)
					object.SetBone(j, idNode_to_NodeRef[skin.joints[j]]);
			}
		}
	}
}

//----
static bool ExportDeformers(hg::Model &mdl, hg::Object *object) { return false; }

static void GetTextureData(const Model &model, const int textureIndex, hg::Picture &pic, const Config &config) {
	auto texture = model.textures[textureIndex];
	auto image = model.images[texture.source];

	if (image.uri.empty()) {
		std::string mimeType = image.mimeType.replace(image.mimeType.find("image/", 0), std::string("image/").size(), "");
		std::string name = image.name.empty() ? hg::format("%1").arg(textureIndex) : image.name;

		std::string dst_path;
		GetOutputPath(dst_path, config.base_output_path, name, {}, mimeType, config.import_policy_texture);
		hg::LoadPicture(pic, dst_path.c_str());
	} else {
		// copy the image
		std::string src_path = image.uri;
		if (!hg::Exists(src_path.c_str())) {
			src_path = hg::CutFilePath(src_path);

			if (hg::Exists(hg::PathJoin({hg::CutFileName(config.input_path), src_path}).c_str()))
				src_path = hg::PathJoin({hg::CutFileName(config.input_path), src_path});
			else if (hg::Exists(hg::PathJoin({hg::CutFileName(config.input_path), image.uri}).c_str()))
				src_path = hg::PathJoin({hg::CutFileName(config.input_path), image.uri});
			else if (!hg::Exists(src_path.c_str())) {
				hg::error(hg::format("Missing texture file '%1'").arg(src_path));
			}
		}
		hg::LoadPicture(pic, src_path.c_str());
	}
}

static hg::TextureRef ExportTexture(
	const Model &model, const int &textureIndex, const std::string &dst_path, const Config &config, hg::PipelineResources &resources) {

	auto texture = model.textures[textureIndex];
	uint32_t flags = BGFX_SAMPLER_NONE;
	if (texture.sampler >= 0) {
		auto sampler = model.samplers[texture.sampler];

		switch (sampler.wrapS) {
			case TINYGLTF_TEXTURE_WRAP_REPEAT:
				break; // default
			case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
				flags |= BGFX_SAMPLER_U_CLAMP;
				break;
		}
		switch (sampler.wrapT) {
			case TINYGLTF_TEXTURE_WRAP_REPEAT:
				break; // default
			case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
				flags |= BGFX_SAMPLER_V_CLAMP;
				break;
		}
	}

	std::string dst_rel_path = MakeRelativeResourceName(dst_path, config.prj_path, config.prefix);

	return resources.textures.Add(dst_rel_path.c_str(), {flags, BGFX_INVALID_HANDLE});
}

static std::string GetTextureMime(const Model &model, const int &textureIndex) {
	if (textureIndex < 0)
		return "png"; // set default to png

	const auto &texture = model.textures[textureIndex];
	const auto &image = model.images[texture.source];

	std::string mimeType = image.mimeType;
	if (const auto indexFind = mimeType.find("image/", 0) != std::string::npos)
		mimeType = mimeType.replace(indexFind, std::string("image/").size(), "");
	return mimeType;
}
//
static hg::TextureRef ExportTexture(const Model &model, const int &textureIndex, const Config &config, hg::PipelineResources &resources) {
	if (textureIndex < 0)
		return hg::InvalidTextureRef;

	const auto &texture = model.textures[textureIndex];
	const auto &image = model.images[texture.source];

	std::string dst_path;

	if (image.uri.empty()) {
		std::string mimeType = GetTextureMime(model, textureIndex);
		std::string name = image.name.empty() ? hg::format("%1").arg(textureIndex) : image.name;

		GetOutputPath(dst_path, config.base_output_path, name, {}, mimeType, config.import_policy_texture);
	} else {
		// copy the image
		std::string src_path = image.uri;
		if (!hg::Exists(src_path.c_str())) {
			if (hg::Exists(hg::PathJoin({hg::CutFileName(config.input_path), src_path}).c_str())) {
				src_path = hg::PathJoin({hg::CutFileName(config.input_path), src_path});
			} else {
				src_path = hg::CutFilePath(src_path);

				if (hg::Exists(hg::PathJoin({hg::CutFileName(config.input_path), src_path}).c_str()))
					src_path = hg::PathJoin({hg::CutFileName(config.input_path), src_path});
				else if (!hg::Exists(src_path.c_str())) {
					hg::error(hg::format("Missing texture file '%1'").arg(src_path));
					return {};
				}
			}
		}

		GetOutputPath(dst_path, config.base_output_path, hg::GetFileName(src_path), {}, hg::GetFileExtension(src_path), config.import_policy_texture);
	}
	return ExportTexture(model, textureIndex, dst_path, config, resources);
}

//
static hg::Material ExportMaterial(const Model &model, const Material &gltf_mat, const Config &config, hg::PipelineResources &resources) {
#if 0
	// CWE 563: Variable is assigned a value that is never used
	hg::Color diffuse = {0.5f, 0.5f, 0.5f, 1.f}, emissive = {0, 0, 0, 1}, specular = {0.5f, 0.5f, 0.5f, 1.f}, ambient = {0, 0, 0, 1};
#endif

	float glossiness{1.f};
	float reflection{1.f};

	hg::debug(hg::format("Exporting material '%1'").arg(gltf_mat.name));
#if 0

	static const std::string meta_RAW_text("{\"profiles\": {\"default\": {\"compression\": \"RAW\"}}}");
	static const std::string meta_BC1_text("{\"profiles\": {\"default\": {\"compression\": \"BC1\"}}}");
	static const std::string meta_BC3_text("{\"profiles\": {\"default\": {\"compression\": \"BC3\"}}}");
	static const std::string meta_BC4_text("{\"profiles\": {\"default\": {\"compression\": \"BC4\"}}}");
	static const std::string meta_BC6_text("{\"profiles\": {\"default\": {\"compression\": \"BC6\"}}}");
	static const std::string meta_BC7_text("{\"profiles\": {\"default\": {\"compression\": \"BC7\"}}}");
#endif
	static const std::string meta_BC5_text("{\"profiles\": {\"default\": {\"compression\": \"BC5\"}}}");
	static const std::string meta_BC7_srgb_text("{\"profiles\": {\"default\": {\"compression\": \"BC7\", \"srgb\": 1}}}");

	//
	std::string dst_path;
	hg::Material mat;
	std::string shader("core/shader/pbr.hps");

	// export the texture
	auto baseColorTexture = ExportTexture(model, gltf_mat.pbrMetallicRoughness.baseColorTexture.index, config, resources);
	if (baseColorTexture != hg::InvalidTextureRef) {
		hg::debug(hg::format("    - uBaseOpacityMap: %1").arg(resources.textures.GetName(baseColorTexture)));

		if (GetOutputPath(dst_path, config.prj_path, resources.textures.GetName(baseColorTexture), {}, "meta", config.import_policy_texture)) {
			if (std::FILE *f = std::fopen(dst_path.c_str(), "w")) {
				std::fwrite(meta_BC7_srgb_text.data(), sizeof meta_BC7_srgb_text[0], meta_BC7_srgb_text.size(), f);
				std::fclose(f);
			}
		}
		mat.textures["uBaseOpacityMap"] = {baseColorTexture, 0};
	}

	// make the orm from the possible value
	hg::TextureRef metallicRoughnessTexture;
	std::string meta_occlusionTexture("{\"profiles\": {\"default\": {\"compression\": \"BC7\"}}}");
	if (gltf_mat.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0) {
		metallicRoughnessTexture = ExportTexture(model, gltf_mat.pbrMetallicRoughness.metallicRoughnessTexture.index, config, resources);

		// if there is an occlusion texture and it's not the same as the map in metallicRoughness
		if (gltf_mat.occlusionTexture.index >= 0 && gltf_mat.occlusionTexture.index != gltf_mat.pbrMetallicRoughness.metallicRoughnessTexture.index) {
			hg::TextureRef OcclusionTexture = ExportTexture(model, gltf_mat.occlusionTexture.index, config, resources);

			// if the occlusion texture use the same uv, merge into the same map as roughness metal
			if (gltf_mat.occlusionTexture.texCoord == gltf_mat.pbrMetallicRoughness.metallicRoughnessTexture.texCoord) {
				json meta_occlusionTexture_json = {{"profiles",
					{{"default", {{"compression", "BC7"}, {"preprocess", {{"construct", {resources.textures.GetName(OcclusionTexture), "G", "B"}}}}}}}}};
				meta_occlusionTexture = meta_occlusionTexture_json.dump();
			}
		} else if (gltf_mat.occlusionTexture.index < 0) { // no occlusion texture, use the var
			json meta_occlusionTexture_json = {{"profiles", {{"default", {{"compression", "BC7"}, {"preprocess", {{"construct", {255, "G", "B"}}}}}}}}};
			meta_occlusionTexture = meta_occlusionTexture_json.dump();
		}
	} else if (gltf_mat.occlusionTexture.index >= 0) {
		// no orm map but an occlusion map is here
		metallicRoughnessTexture = ExportTexture(model, gltf_mat.occlusionTexture.index, config, resources);
		json meta_occlusionTexture_json = {{"profiles",
			{{"default", {{"compression", "BC7"}, {"preprocess", {{"construct", {"R", (int)gltf_mat.pbrMetallicRoughness.roughnessFactor * 255,
																					(int)gltf_mat.pbrMetallicRoughness.metallicFactor * 255}}}}}}}}};
		meta_occlusionTexture = meta_occlusionTexture_json.dump();
	}

	if (metallicRoughnessTexture != hg::InvalidTextureRef) {
		hg::debug(hg::format("    - uOcclusionRoughnessMetalnessMap: %1").arg(resources.textures.GetName(metallicRoughnessTexture)));

		if (GetOutputPath(dst_path, config.prj_path, resources.textures.GetName(metallicRoughnessTexture), {}, "meta", config.import_policy_texture)) {
			if (std::FILE *f = std::fopen(dst_path.c_str(), "w")) {
				std::fwrite(meta_occlusionTexture.data(), sizeof meta_occlusionTexture[0], meta_occlusionTexture.size(), f);
				std::fclose(f);
			}
		}
		mat.textures["uOcclusionRoughnessMetalnessMap"] = {metallicRoughnessTexture, 1};
	}

	// normal texture
	auto normalTexture = ExportTexture(model, gltf_mat.normalTexture.index, config, resources);
	if (normalTexture != hg::InvalidTextureRef) {
		hg::debug(hg::format("    - uNormalMap: %1").arg(resources.textures.GetName(normalTexture)));

		if (GetOutputPath(dst_path, config.prj_path, resources.textures.GetName(normalTexture), {}, "meta", config.import_policy_texture)) {
			if (std::FILE *f = std::fopen(dst_path.c_str(), "w")) {
				std::fwrite(meta_BC5_text.data(), sizeof meta_BC5_text[0], meta_BC5_text.size(), f);
				std::fclose(f);
			}
		}
		mat.textures["uNormalMap"] = {normalTexture, 2};
	}

	// emissive texture
	auto emissiveTexture = ExportTexture(model, gltf_mat.emissiveTexture.index, config, resources);
	if (emissiveTexture != hg::InvalidTextureRef) {
		hg::debug(hg::format("    - uSelfMap: %1").arg(resources.textures.GetName(emissiveTexture)));

		if (GetOutputPath(dst_path, config.prj_path, resources.textures.GetName(emissiveTexture), {}, "meta", config.import_policy_texture)) {
			if (std::FILE *f = std::fopen(dst_path.c_str(), "w")) {
				std::fwrite(meta_BC7_srgb_text.data(), sizeof meta_BC7_srgb_text[0], meta_BC7_srgb_text.size(), f);
				std::fclose(f);
			}
		}
		mat.textures["uSelfMap"] = {emissiveTexture, 4};
	}

	mat.values["uBaseOpacityColor"] = {
		bgfx::UniformType::Vec4, {float(gltf_mat.pbrMetallicRoughness.baseColorFactor[0]), float(gltf_mat.pbrMetallicRoughness.baseColorFactor[1]),
									 float(gltf_mat.pbrMetallicRoughness.baseColorFactor[2]), float(gltf_mat.pbrMetallicRoughness.baseColorFactor[3])}};
	mat.values["uOcclusionRoughnessMetalnessColor"] = {
		bgfx::UniformType::Vec4, {1.f, float(gltf_mat.pbrMetallicRoughness.roughnessFactor), float(gltf_mat.pbrMetallicRoughness.metallicFactor), -1.f}};
	mat.values["uSelfColor"] = {
		bgfx::UniformType::Vec4, {float(gltf_mat.emissiveFactor[0]), float(gltf_mat.emissiveFactor[1]), float(gltf_mat.emissiveFactor[2]), -1.f}};

	if (gltf_mat.alphaMode == "BLEND" || gltf_mat.alphaMode == "MASK")
		SetMaterialBlendMode(mat, hg::BM_Alpha);

	if (gltf_mat.doubleSided)
		SetMaterialFaceCulling(mat, hg::FC_Disabled);

	if (!config.shader.empty())
		shader = config.shader; // use override

	hg::debug(hg::format("    - Using pipeline shader '%1'").arg(shader));
	mat.program = resources.programs.Add(shader.c_str(), {});

	// FinalizeMaterial(mat, fbx_material->GetName(), geo_name);
	return mat;
}

#define __PolIndex (pol_index[p] + v)
#define __PolRemapIndex (pol_index[p] + (geo.pol[p].vtx_count - 1 - v))

static void ExportGeometry(
	const Model &model, const Primitive &meshPrimitive, const int &primitiveID, const Config &config, hg::PipelineResources &resources, hg::Geometry &geo) {
	// TODO detect instancing (using SHA1 on model)

	// Boolean used to check if we have converted the vertex buffer format
	bool convertedToTriangleList = false;
	// This permit to get a type agnostic way of reading the index buffer
	std::unique_ptr<intArrayBase> indicesArrayPtr = nullptr;

	if (meshPrimitive.indices == -1) {
		hg::debug("ERROR: Can't load geometry without triangles indices");
		return;
	}

	{
		const auto &indicesAccessor = model.accessors[meshPrimitive.indices];
		const auto &bufferView = model.bufferViews[indicesAccessor.bufferView];
		const auto &buffer = model.buffers[bufferView.buffer];
		const auto dataAddress = buffer.data.data() + bufferView.byteOffset + indicesAccessor.byteOffset;
		const auto byteStride = indicesAccessor.ByteStride(bufferView);
		const auto count = indicesAccessor.count;

		// Allocate the index array in the pointer-to-base declared in the
		// parent scope
		switch (indicesAccessor.componentType) {
			case TINYGLTF_COMPONENT_TYPE_BYTE:
				indicesArrayPtr = std::unique_ptr<intArray<char>>(new intArray<char>(arrayAdapter<char>(dataAddress, count, byteStride)));
				break;

			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
				indicesArrayPtr =
					std::unique_ptr<intArray<unsigned char>>(new intArray<unsigned char>(arrayAdapter<unsigned char>(dataAddress, count, byteStride)));
				break;

			case TINYGLTF_COMPONENT_TYPE_SHORT:
				indicesArrayPtr = std::unique_ptr<intArray<short>>(new intArray<short>(arrayAdapter<short>(dataAddress, count, byteStride)));
				break;

			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
				indicesArrayPtr =
					std::unique_ptr<intArray<unsigned short>>(new intArray<unsigned short>(arrayAdapter<unsigned short>(dataAddress, count, byteStride)));
				break;

			case TINYGLTF_COMPONENT_TYPE_INT:
				indicesArrayPtr = std::unique_ptr<intArray<int>>(new intArray<int>(arrayAdapter<int>(dataAddress, count, byteStride)));
				break;

			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
				indicesArrayPtr =
					std::unique_ptr<intArray<unsigned int>>(new intArray<unsigned int>(arrayAdapter<unsigned int>(dataAddress, count, byteStride)));
				break;
			default:
				break;
		}
	}

	size_t start_id_pol = geo.pol.size();
	size_t start_id_binding = geo.binding.size();
	size_t start_id_vtx = geo.vtx.size();

	if (indicesArrayPtr) {
		const auto &indices = *indicesArrayPtr;
		// hg::debug("indices: ");
		// they are all triangles
		geo.pol.resize(start_id_pol + indices.size() / 3);
		for (size_t n = start_id_pol; n < geo.pol.size(); ++n) {
			geo.pol[n].vtx_count = uint8_t(3);
			geo.pol[n].material = primitiveID;
		}

		const auto pol_index = hg::ComputePolygonIndex(geo);
		geo.binding.resize(hg::ComputeBindingCount(geo));

		for (size_t p = start_id_pol; p < geo.pol.size(); ++p)
			for (auto v = 0; v < geo.pol[p].vtx_count; ++v) {
				geo.binding[pol_index[p] + v] = indices[pol_index[p] + (geo.pol[p].vtx_count - 1 - v) - start_id_binding] + start_id_vtx;
			}
	}

	switch (meshPrimitive.mode) {
			// We re-arrange the indices so that it describe a simple list of
			// triangles
		case TINYGLTF_MODE_TRIANGLE_FAN:
			if (!convertedToTriangleList) {
				hg::debug("TRIANGLE_FAN");
				// This only has to be done once per primitive
				convertedToTriangleList = true;

				// We steal the guts of the vector
				auto triangleFan = std::move(geo.binding);
				// geo.binding.clear();

				// Push back the indices that describe just one triangle one by one
				for (size_t i{start_id_binding + 2}; i < triangleFan.size(); ++i) {
					geo.binding.push_back(triangleFan[0]);
					geo.binding.push_back(triangleFan[i - 1]);
					geo.binding.push_back(triangleFan[i]);
				}
			}
		case TINYGLTF_MODE_TRIANGLE_STRIP:
			if (!convertedToTriangleList) {
				hg::debug("TRIANGLE_STRIP");
				// This only has to be done once per primitive
				convertedToTriangleList = true;

				auto triangleStrip = std::move(geo.binding);
				// geo.binding.clear();

				for (size_t i{start_id_binding + 2}; i < triangleStrip.size(); ++i) {
					geo.binding.push_back(triangleStrip[i - 2]);
					geo.binding.push_back(triangleStrip[i - 1]);
					geo.binding.push_back(triangleStrip[i]);
				}
			}
		case TINYGLTF_MODE_TRIANGLES: { // this is the simpliest case to handle
			hg::debug("TRIANGLES");

			using AttribWritter = std::function<void(float *w, uint32_t p)>;
			AttribWritter w_position = [](float *w, uint32_t p) {};
			AttribWritter w_normal = [](float *w, uint32_t p) {};
			AttribWritter w_texcoord0 = [](float *w, uint32_t p) {};
			AttribWritter w_texcoord1 = [](float *w, uint32_t p) {};
			AttribWritter w_tangent = [](float *w, uint32_t p) {};
			AttribWritter w_joints0 = [](float *w, uint32_t p) {};
			AttribWritter w_weights0 = [](float *w, uint32_t p) {};

			// get the accessor
			for (const auto &attribute : meshPrimitive.attributes) {
				const auto attribAccessor = model.accessors[attribute.second];
				const auto &bufferView = model.bufferViews[attribAccessor.bufferView];
				const auto &buffer = model.buffers[bufferView.buffer];
				const auto dataPtr = buffer.data.data() + bufferView.byteOffset + attribAccessor.byteOffset;
				const auto byte_stride = attribAccessor.ByteStride(bufferView);
				const bool normalized = attribAccessor.normalized;

				AttribWritter *writter = nullptr;
				unsigned int max_components = 0;
				if (attribute.first == "POSITION") {
					writter = &w_position;
					max_components = 3;
				} else if (attribute.first == "NORMAL") {
					writter = &w_normal;
					max_components = 3;
				} else if (attribute.first == "TEXCOORD_0") {
					writter = &w_texcoord0;
					max_components = 2;
				} else if (attribute.first == "TEXCOORD_1") {
					writter = &w_texcoord1;
					max_components = 2;
				} else if (attribute.first == "TANGENT") {
					writter = &w_tangent;
					max_components = 4;
				} else if (attribute.first == "JOINTS_0") {
					writter = &w_joints0;
					max_components = 4;
				} else if (attribute.first == "WEIGHTS_0") {
					writter = &w_weights0;
					max_components = 4;
				}

				if (!writter)
					continue;

				switch (attribAccessor.type) {
					case TINYGLTF_TYPE_SCALAR:
						max_components = std::min(max_components, 1u);
						break;
					case TINYGLTF_TYPE_VEC2:
						max_components = std::min(max_components, 2u);
						break;
					case TINYGLTF_TYPE_VEC3:
						max_components = std::min(max_components, 3u);
						break;
					case TINYGLTF_TYPE_VEC4:
						max_components = std::min(max_components, 4u);
						break;
				}

				switch (attribAccessor.componentType) {
					case TINYGLTF_COMPONENT_TYPE_FLOAT:
						*writter = [dataPtr, byte_stride, max_components](float *w, uint32_t p) {
							const float *f = (const float *)(dataPtr + p * byte_stride);
							for (unsigned int i = 0; i < max_components; ++i) {
								w[i] = f[i];
							}
						};
						break;
					case TINYGLTF_COMPONENT_TYPE_DOUBLE:
						*writter = [dataPtr, byte_stride, max_components](float *w, uint32_t p) {
							const double *f = (const double *)(dataPtr + p * byte_stride);
							for (unsigned int i = 0; i < max_components; ++i) {
								w[i] = (float)f[i];
							}
						};
						break;
					case TINYGLTF_COMPONENT_TYPE_BYTE:
						*writter = [dataPtr, byte_stride, max_components, normalized](float *w, uint32_t p) {
							const int8_t *f = (const int8_t *)(dataPtr + p * byte_stride);
							for (unsigned int i = 0; i < max_components; ++i) {
								w[i] = normalized ? f[i] / (float)128 : f[i];
							}
						};
						break;
					case TINYGLTF_COMPONENT_TYPE_SHORT:
						*writter = [dataPtr, byte_stride, max_components, normalized](float *w, uint32_t p) {
							const int16_t *f = (const int16_t *)(dataPtr + p * byte_stride);
							for (unsigned int i = 0; i < max_components; ++i) {
								w[i] = normalized ? f[i] / (float)32768 : f[i];
							}
						};
						break;
					case TINYGLTF_COMPONENT_TYPE_INT:
						*writter = [dataPtr, byte_stride, max_components, normalized](float *w, uint32_t p) {
							const int32_t *f = (const int32_t *)(dataPtr + p * byte_stride);
							for (unsigned int i = 0; i < max_components; ++i) {
								w[i] = normalized ? f[i] / (float)2147483648 : f[i];
							}
						};
						break;
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
						*writter = [dataPtr, byte_stride, max_components, normalized](float *w, uint32_t p) {
							const uint8_t *f = (const uint8_t *)(dataPtr + p * byte_stride);
							for (unsigned int i = 0; i < max_components; ++i) {
								w[i] = normalized ? f[i] / (float)255 : f[i];
							}
						};
						break;
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
						*writter = [dataPtr, byte_stride, max_components, normalized](float *w, uint32_t p) {
							const uint16_t *f = (const uint16_t *)(dataPtr + p * byte_stride);
							for (unsigned int i = 0; i < max_components; ++i) {
								w[i] = normalized ? f[i] / (float)65535 : f[i];
							}
						};
						break;
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
						*writter = [dataPtr, byte_stride, max_components, normalized](float *w, uint32_t p) {
							const uint32_t *f = (const uint32_t *)(dataPtr + p * byte_stride);
							for (unsigned int i = 0; i < max_components; ++i) {
								w[i] = normalized ? f[i] / (float)4294967295 : f[i];
							}
						};
						break;
					default:
						assert(!"Not supported component type (yet)");
				}
			}

			// set the values
			for (const auto &attribute : meshPrimitive.attributes) {
				const auto attribAccessor = model.accessors[attribute.second];
				const auto count = attribAccessor.count;

				// hg::debug(hg::format("current attribute has count %1 and stride %2 bytes").arg(count).arg(byte_stride).c_str());
				hg::debug(hg::format("attribute string is : %1").arg(attribute.first));
				if (attribute.first == "POSITION") {
					hg::debug("found position attribute");

					// get the position min/max for computing the boundingbox
					/*		pMin.x = attribAccessor.minValues[0];
					pMin.y = attribAccessor.minValues[1];
					pMin.z = attribAccessor.minValues[2];
					pMax.x = attribAccessor.maxValues[0];
					pMax.y = attribAccessor.maxValues[1];
					pMax.z = attribAccessor.maxValues[2];
					*/
					// 3D vector of float
					hg::Vec3 v;
					for (size_t i{0}; i < count; ++i) {
						w_position(&v.x, i);
						v.z = -v.z;
						geo.vtx.push_back(v * config.geometry_scale);
					}
				}

				if (attribute.first == "NORMAL") {
					hg::debug("found normal attribute");

					hg::Vec3 n;
					for (size_t i{start_id_binding}; i < geo.binding.size(); ++i) {
						w_normal(&n.x, geo.binding[i] - start_id_vtx);
						n.z = -n.z;
						geo.normal.push_back(n);
					}
				}

				// Face varying comment on the normals is also true for the UVs
				if (attribute.first == "TEXCOORD_0") {
					hg::debug("Found texture coordinates 0");

					hg::Vec2 uv;
					for (size_t i{start_id_binding}; i < geo.binding.size(); ++i) {
						w_texcoord0(&uv.x, geo.binding[i] - start_id_vtx);
						geo.uv[0].push_back(uv);
					}
				}
				// Face varying comment on the normals is also true for the UVs
				if (attribute.first == "TEXCOORD_1") {
					hg::debug("Found texture coordinates 1");

					hg::Vec2 uv;
					for (size_t i{start_id_binding}; i < geo.binding.size(); ++i) {
						w_texcoord1(&uv.x, geo.binding[i] - start_id_vtx);
						geo.uv[1].push_back(uv);
					}
				}

				// JOINTS_0
				if (attribute.first == "JOINTS_0") {
					hg::debug("found JOINTS_0 attribute");

					if (geo.skin.size() < count + start_id_vtx)
						geo.skin.resize(count + start_id_vtx);

					hg::Vec4 joints;
					for (size_t i{0}; i < count; ++i) {
						w_joints0(&joints.x, i);
						geo.skin[i + start_id_vtx].index[0] = hg::numeric_cast<uint16_t>((int)(joints.x));
						geo.skin[i + start_id_vtx].index[1] = hg::numeric_cast<uint16_t>((int)(joints.y));
						geo.skin[i + start_id_vtx].index[2] = hg::numeric_cast<uint16_t>((int)(joints.z));
						geo.skin[i + start_id_vtx].index[3] = hg::numeric_cast<uint16_t>((int)(joints.w));
					}
				}

				// WEIGHTS_0
				if (attribute.first == "WEIGHTS_0") {
					hg::debug("found WEIGHTS_0 attribute");

					if (geo.skin.size() < count + start_id_vtx)
						geo.skin.resize(count + start_id_vtx);

					hg::Vec4 weights;
					for (size_t i{0}; i < count; ++i) {
						w_weights0(&weights.x, i);
						geo.skin[i + start_id_vtx].weight[0] = hg::pack_float<uint8_t>(weights.x);
						geo.skin[i + start_id_vtx].weight[1] = hg::pack_float<uint8_t>(weights.y);
						geo.skin[i + start_id_vtx].weight[2] = hg::pack_float<uint8_t>(weights.z);
						geo.skin[i + start_id_vtx].weight[3] = hg::pack_float<uint8_t>(weights.w);
					}
				}
			}

			// special for the tangent, because it need the normal to compute the bitangent
			for (const auto &attribute : meshPrimitive.attributes) {
				const auto attribAccessor = model.accessors[attribute.second];
				const auto count = attribAccessor.count;

				// hg::debug(hg::format("current attribute has count %1 and stride %2 bytes").arg(count).arg(byte_stride).c_str());

				hg::debug(hg::format("attribute string is : %1").arg(attribute.first).c_str());
				if (attribute.first == "TANGENT") {
					hg::debug("found tangent attribute");

					hg::Vec4 t;
					for (size_t i{start_id_binding}; i < geo.binding.size(); ++i) {
						w_tangent(&t.x, geo.binding[i] - start_id_vtx);
						geo.tangent.push_back(hg::Geometry::TangentFrame{
							hg::Vec3(t.x, t.y, t.z), hg::Cross(geo.normal[geo.binding[i] - start_id_vtx], hg::Vec3(t.x, t.y, t.z)) * t.w});
					}
				}
			}
			break;
		}
		default:
			hg::error("primitive mode not implemented");
			break;

			// These aren't triangles:
		case TINYGLTF_MODE_POINTS:
		case TINYGLTF_MODE_LINE:
		case TINYGLTF_MODE_LINE_LOOP:
			hg::error("primitive is not triangle based, ignoring");
	}
}

//
static void ExportObject(const Model &model, const Node &gltf_node, hg::Node &node, hg::Scene &scene, const Config &config, hg::PipelineResources &resources,
	const int &gltf_id_node) {

	// if there is no mesh or no skin, nothing inside object
	if (gltf_node.mesh < 0 && gltf_node.skin < 0)
		return;

	hg::Geometry geo;
	std::string path = node.GetName();
	std::string primitiveIds;
	auto object = scene.CreateObject();

	// check path geo
	if (gltf_node.mesh >= 0) {
		auto gltf_mesh = model.meshes[gltf_node.mesh];
		if (!gltf_mesh.name.empty())
			path = hg::CleanFileName(hg::CutFileExtension(gltf_mesh.name));
		for (auto meshPrimitive : gltf_mesh.primitives) {
			// add attribute ids to be sure to have this one particular geo and material
			for (const auto &a : meshPrimitive.attributes)
				primitiveIds += std::to_string(a.second) + "_";
			primitiveIds += std::to_string(meshPrimitive.indices) + "_";
		}
	}

	primitiveIds += "_" + std::to_string(gltf_node.skin);
	bool found_geo = false;

	// if there is set of primitive ids existing
	auto primitiveIdsToGeoPath_itr = primitiveIdsToGeoPath.find(primitiveIds);
	if (primitiveIdsToGeoPath_itr != primitiveIdsToGeoPath.end()) {
		// get path to geo
		path = primitiveIdsToGeoPath_itr->second;
		GetOutputPath(path, config.base_output_path, path, {}, "geo", config.import_policy_geometry);
		path = MakeRelativeResourceName(path, config.prj_path, config.prefix);
	} else {
		// export geo mesh
		if (gltf_node.mesh >= 0) {
			auto gltf_mesh = model.meshes[gltf_node.mesh];

			int primitiveId = 0;
			for (auto meshPrimitive : gltf_mesh.primitives) {
				ExportGeometry(model, meshPrimitive, primitiveId, config, resources, geo);
				++primitiveId;
			}

			const auto vtx_to_pol = hg::ComputeVertexToPolygon(geo);
			auto vtx_normal = hg::ComputeVertexNormal(geo, vtx_to_pol, hg::Deg(45.f));

			// recalculate normals
			bool recalculate_normal = config.recalculate_normal;
			if (geo.normal.empty())
				recalculate_normal = true;

			if (recalculate_normal) {
				hg::debug("    - Recalculate normals");
				geo.normal = vtx_normal;
			} else
				vtx_normal = geo.normal;

			// recalculate tangent frame
			bool recalculate_tangent = config.recalculate_tangent;
			if (geo.tangent.empty())
				recalculate_tangent = true;
			else if (geo.tangent.size() != geo.normal.size()) { // be sure tangent is same size of normal, some strange things can happen with multiple submesh
				hg::debug("CAREFUL Normal and Tangent are not the same size, can happen if you have submesh (some with tangent and some without)");
				geo.tangent.resize(geo.normal.size());
			}

			if (recalculate_tangent) {
				hg::debug("    - Recalculate tangent frames (MikkT)");
				if (!geo.uv[0].empty())
					geo.tangent = hg::ComputeVertexTangent(geo, vtx_normal, 0, hg::Deg(45.f));
			}
		}
		// find bind pose in the skins
		if (gltf_node.skin >= 0) {
			hg::debug(hg::format("Exporting geometry skin"));

			const auto &skin = model.skins[gltf_node.skin];
			geo.bind_pose.resize(skin.joints.size());

			const auto attribAccessor = model.accessors[skin.inverseBindMatrices];
			const auto &bufferView = model.bufferViews[attribAccessor.bufferView];
			const auto &buffer = model.buffers[bufferView.buffer];
			const auto dataPtr = buffer.data.data() + bufferView.byteOffset + attribAccessor.byteOffset;
			const auto byte_stride = attribAccessor.ByteStride(bufferView);
			const auto count = attribAccessor.count;

			switch (attribAccessor.type) {
				case TINYGLTF_TYPE_MAT4: {
					switch (attribAccessor.componentType) {
						case TINYGLTF_COMPONENT_TYPE_DOUBLE:
						case TINYGLTF_COMPONENT_TYPE_FLOAT: {
							floatArray<float> value(arrayAdapter<float>(dataPtr, count * 16, sizeof(float)));

							for (size_t k{0}; k < count; ++k) {
								hg::Mat4 m_InverseBindMatrices(value[k * 16], value[k * 16 + 1], value[k * 16 + 2], value[k * 16 + 4], value[k * 16 + 5],
									value[k * 16 + 6], value[k * 16 + 8], value[k * 16 + 9], value[k * 16 + 10], value[k * 16 + 12], value[k * 16 + 13],
									value[k * 16 + 14]);

								m_InverseBindMatrices = hg::InverseFast(m_InverseBindMatrices);

								auto p = hg::GetT(m_InverseBindMatrices);
								p.z = -p.z;
								auto r = hg::GetR(m_InverseBindMatrices);
								r.x = -r.x;
								r.y = -r.y;
								auto s = hg::GetS(m_InverseBindMatrices);

								geo.bind_pose[k] = hg::InverseFast(hg::TransformationMat4(p, r, s));
							}
						} break;
						default:
							hg::error("Unhandeled component type for inverseBindMatrices");
					}
				} break;
				default:
					hg::error("Unhandeled MAT4 type for inverseBindMatrices");
			}
		}

		// check if name already taken
		auto geoPathOcurrence_itr = geoPathOcurrence.find(path);
		if (geoPathOcurrence_itr != geoPathOcurrence.end()) {
			geoPathOcurrence_itr->second++;
			path += hg::format("%1").arg(geoPathOcurrence_itr->second).str();
		} else
			geoPathOcurrence[path] = 0;

		// save it to geo to keep
		primitiveIdsToGeoPath[primitiveIds] = path;

		if (gltf_node.mesh >= 0 || gltf_node.skin >= 0) {
			if (GetOutputPath(path, config.base_output_path, path, {}, "geo", config.import_policy_geometry)) {
				hg::debug(hg::format("Export geometry to '%1'").arg(path));
				hg::SaveGeometryToFile(path.c_str(), geo);
			}

			path = MakeRelativeResourceName(path, config.prj_path, config.prefix);
		}
	}

	// add materials
	if (gltf_node.mesh >= 0) {
		auto gltf_mesh = model.meshes[gltf_node.mesh];
		int primitiveId = 0;
		for (auto meshPrimitive : gltf_mesh.primitives) {
			object.SetMaterialCount(primitiveId + 1);

			// MATERIALS
			// 1 material per primitive
			if (meshPrimitive.material >= 0) {
				auto gltf_mat = model.materials[meshPrimitive.material];
				auto mat = ExportMaterial(model, gltf_mat, config, resources);
				if (geo.skin.size())
					mat.flags |= hg::MF_EnableSkinning;

				object.SetMaterial(primitiveId, std::move(mat));
				object.SetMaterialName(primitiveId, gltf_mat.name.empty() ? hg::format("mat_%1").arg(primitiveId) : gltf_mat.name);

			} else { // make a dummy material to see the object in the engine
				hg::debug(hg::format("    - Has no material, set a dummy one"));

				hg::Material mat;
				std::string shader;

				shader = "core/shader/pbr.hps";

				if (!config.shader.empty())
					shader = config.shader; // use override

				hg::debug(hg::format("    - Using pipeline shader '%1'").arg(shader));
				mat.program = resources.programs.Add(shader.c_str(), {});

				mat.values["uBaseOpacityColor"] = {bgfx::UniformType::Vec4, {1.f, 1.f, 1.f, 1.f}};
				mat.values["uOcclusionRoughnessMetalnessColor"] = {bgfx::UniformType::Vec4, {1.f, 1.f, 0.f, -1.f}};
				mat.values["uSelfColor"] = {bgfx::UniformType::Vec4, {0.f, 0.f, 0.f, -1.f}};

				object.SetMaterial(primitiveId, std::move(mat));
				object.SetMaterialName(primitiveId, "dummy_mat");
			}
			++primitiveId;
		}
	}

	// set object
	if (gltf_node.mesh >= 0 || gltf_node.skin >= 0) {
		node.SetObject(object);
		object.SetModelRef(resources.models.Add(path.c_str(), {}));
	}
}

static void ExportCamera(const Model &model, const Node &gltf_node, hg::Node &node, hg::Scene &scene, const Config &config, hg::PipelineResources &resources) {
	auto camera = scene.CreateCamera();
	node.SetCamera(camera);

	auto gltf_camera = model.cameras[gltf_node.camera];

	if (gltf_camera.type == "perspective") {
		camera.SetZNear(gltf_camera.perspective.znear);
		camera.SetZFar(gltf_camera.perspective.zfar);
		camera.SetFov(float(gltf_camera.perspective.yfov));
		camera.SetIsOrthographic(false);
	} else if (gltf_camera.type == "orthographic") {
		camera.SetZNear(gltf_camera.orthographic.znear);
		camera.SetZFar(gltf_camera.orthographic.zfar);
		camera.SetIsOrthographic(true);
	}
}

//
static hg::Node ExportNode(const Model &model, const int &gltf_id_node, hg::Scene &scene, const Config &config, hg::PipelineResources &resources) {
	const auto &gltf_node = model.nodes[gltf_id_node];
	auto node = scene.CreateNode((gltf_node.name.empty() ? hg::format("node%1").arg(gltf_id_node) : gltf_node.name));
	idNode_to_NodeRef[gltf_id_node] = node.ref;

	// check if disable
	auto KHR_nodes_disable = gltf_node.extensions.find("KHR_nodes_disable");
	if (KHR_nodes_disable != gltf_node.extensions.end()) {
		if (KHR_nodes_disable->second.Has("visible") && KHR_nodes_disable->second.Get("visible").Get<bool>() == false)
			node.Disable();
	}

	// set transform
	node.SetTransform(scene.CreateTransform());
	if (gltf_node.matrix.size()) {
		hg::Mat4 m(gltf_node.matrix[0], gltf_node.matrix[1], gltf_node.matrix[2], gltf_node.matrix[4], gltf_node.matrix[5], gltf_node.matrix[6],
			gltf_node.matrix[8], gltf_node.matrix[9], gltf_node.matrix[10], gltf_node.matrix[12], gltf_node.matrix[13], gltf_node.matrix[14]);
		auto p = hg::GetT(m);
		p.z = -p.z;
		auto r = hg::GetR(m);
		r.x = -r.x;
		r.y = -r.y;
		auto s = hg::GetS(m);
		node.GetTransform().SetLocal(hg::TransformationMat4(p, r, s));
	} else {
		if (gltf_node.translation.size())
			node.GetTransform().SetPos(hg::Vec3(gltf_node.translation[0], gltf_node.translation[1], -gltf_node.translation[2]));
		else
			node.GetTransform().SetRot(hg::Vec3(0.f, 0.f, 0.f));

		if (gltf_node.rotation.size()) {
			auto r = hg::ToEuler(hg::Quaternion(gltf_node.rotation[0], gltf_node.rotation[1], gltf_node.rotation[2], gltf_node.rotation[3]));
			r.x = -r.x;
			r.y = -r.y;
			node.GetTransform().SetRot(r);
		} else
			node.GetTransform().SetRot(hg::Vec3(0.f, 0.f, 0.f));

		if (gltf_node.scale.size())
			node.GetTransform().SetScale(hg::Vec3(gltf_node.scale[0], gltf_node.scale[1], gltf_node.scale[2]));
		else
			node.GetTransform().SetScale(hg::Vec3(1.f, 1.f, 1.f));
	}

	// is it a camera
	if (gltf_node.camera >= 0)
		ExportCamera(model, gltf_node, node, scene, config, resources);

	// is it a mesh
	if (gltf_node.mesh >= 0 || gltf_node.skin >= 0) {
		// if the node doesn't have a name, give the geo name, if there is one
		if (gltf_node.name.empty() && gltf_node.mesh >= 0) {
			auto gltf_mesh = model.meshes[gltf_node.mesh];
			if (!gltf_mesh.name.empty())
				node.SetName(hg::CutFileExtension(gltf_mesh.name));
		}
		ExportObject(model, gltf_node, node, scene, config, resources, gltf_id_node);
	}

	// import children
	for (auto id_child : gltf_node.children) {
		auto child = ExportNode(model, id_child, scene, config, resources);
		if (child && node)
			child.GetTransform().SetParent(node.ref);
	}

	return node;
}

bool LoadImageDataEx(Image *image, const int image_idx, std::string *err, std::string *warn, int req_width, int req_height, const unsigned char *bytes,
	int size, void *user_data) {
	(void)user_data;
	(void)warn;

	Config *config = static_cast<Config *>(user_data);

	std::string dst_path;

	if (image->uri.empty()) { // copy the buffer into image
		std::string mimeType = image->mimeType.replace(image->mimeType.find("image/", 0), std::string("image/").size(), "");
		std::string name = image->name.empty() ? hg::format("%1").arg(image_idx) : image->name;

		if (GetOutputPath(dst_path, config->base_output_path, name, {}, mimeType, config->import_policy_texture)) {
			auto myfile = std::fstream(dst_path, std::ios::out | std::ios::binary);
			myfile.write((const char *)bytes, size);
			myfile.close();
		}
	} else {
		// copy the image
		std::string src_path = image->uri;
		if (!hg::Exists(src_path.c_str())) {
			if (hg::Exists(hg::PathJoin({hg::CutFileName(config->input_path), src_path}).c_str())) {
				src_path = hg::PathJoin({hg::CutFileName(config->input_path), src_path});
			} else {
				src_path = hg::CutFilePath(src_path);

				if (hg::Exists(hg::PathJoin({hg::CutFileName(config->input_path), src_path}).c_str()))
					src_path = hg::PathJoin({hg::CutFileName(config->input_path), src_path});
				else if (!hg::Exists(src_path.c_str())) {
					hg::error(hg::format("Missing texture file '%1'").arg(src_path));
					return {};
				}
			}
		}

		if (GetOutputPath(dst_path, config->base_output_path, hg::GetFileName(src_path), {}, hg::GetFileExtension(src_path), config->import_policy_texture)) {
			auto myfile = std::fstream(dst_path, std::ios::out | std::ios::binary);
			myfile.write((const char *)bytes, size);
			myfile.close();
		}
	}

	return true;
}

static bool ImportGltfScene(const std::string &path, const Config &config) {
	const auto t_start = hg::time_now();

	if (config.base_output_path.empty())
		return false;
	// create output directory if missing
	if (hg::Exists(config.base_output_path.c_str())) {
		if (!hg::IsDir(config.base_output_path.c_str()))
			return false; // can't output to this path
	} else {
		if (!hg::MkDir(config.base_output_path.c_str()))
			return false;
	}

	//
	Model model;
	TinyGLTF loader;
	std::string err;
	std::string warn;

	// set our own save picture
	loader.SetImageLoader(LoadImageDataEx, const_cast<Config *>(&config));
	bool ret;
	if (hg::tolower(hg::GetFileExtension(path)) == "gltf")
		ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);
	else
		ret = loader.LoadBinaryFromFile(&model, &err, &warn, path); // for binary glTF(.glb)
	if (!ret) {
		hg::error(hg::format("failed to load %1: %2").arg(path.c_str()).arg(err.c_str()));
		return false;
	}

	if (!warn.empty()) {
		hg::log(hg::format("warning %1: %2").arg(path.c_str()).arg(warn.c_str()));
	}

	hg::log("loaded glTF file has:");
	hg::log(hg::format("%1 accessors").arg(model.accessors.size()).c_str());
	hg::log(hg::format("%1 animations").arg(model.animations.size()).c_str());
	hg::log(hg::format("%1 buffers").arg(model.buffers.size()).c_str());
	hg::log(hg::format("%1 bufferViews").arg(model.bufferViews.size()).c_str());
	hg::log(hg::format("%1 materials").arg(model.materials.size()).c_str());
	hg::log(hg::format("%1 meshes").arg(model.meshes.size()).c_str());
	hg::log(hg::format("%1 nodes").arg(model.nodes.size()).c_str());
	hg::log(hg::format("%1 textures").arg(model.textures.size()).c_str());
	hg::log(hg::format("%1 images").arg(model.images.size()).c_str());
	hg::log(hg::format("%1 skins").arg(model.skins.size()).c_str());
	hg::log(hg::format("%1 samplers").arg(model.samplers.size()).c_str());
	hg::log(hg::format("%1 cameras").arg(model.cameras.size()).c_str());
	hg::log(hg::format("%1 scenes").arg(model.scenes.size()).c_str());
	hg::log(hg::format("%1 lights").arg(model.lights.size()).c_str());

	if (!config.finalizer_script.empty())
		if (!LoadFinalizerScript(config.finalizer_script))
			return false;

	for (auto gltf_scene : model.scenes) {
		hg::Scene scene;
		hg::PipelineResources resources;

		for (auto gltf_id_node : gltf_scene.nodes) {
			auto node = ExportNode(model, gltf_id_node, scene, config, resources);
		}

		ExportMotions(model, gltf_scene, scene, config);
		ExportSkins(model, gltf_scene, scene, config);

		FinalizeScene(scene);

		// add default pbr map
		scene.environment.brdf_map = resources.textures.Add("core/pbr/brdf.dds", {BGFX_SAMPLER_NONE, BGFX_INVALID_HANDLE});
		scene.environment.probe = {};
		scene.environment.probe.irradiance_map = resources.textures.Add("core/pbr/probe.hdr.irradiance", {BGFX_SAMPLER_NONE, BGFX_INVALID_HANDLE});
		scene.environment.probe.radiance_map = resources.textures.Add("core/pbr/probe.hdr.radiance", {BGFX_SAMPLER_NONE, BGFX_INVALID_HANDLE});

		std::string out_path;
		if (GetOutputPath(out_path, config.base_output_path,
				config.name.empty() ? hg::GetFileName(path) + (gltf_scene.name.empty() ? "" : "_" + gltf_scene.name) : config.name, {}, "scn",
				config.import_policy_scene))
			SaveSceneJsonToFile(out_path.c_str(), scene, resources);
	}

	hg::log(hg::format("Import complete, took %1 ms").arg(hg::time_to_ms(hg::time_now() - t_start)));
	return true;
}

static ImportPolicy ImportPolicyFromString(const std::string &v) {
	if (v == "skip")
		return ImportPolicy::SkipExisting;
	if (v == "overwrite")
		return ImportPolicy::Overwrite;
	if (v == "rename")
		return ImportPolicy::Rename;
	if (v == "skip_always")
		return ImportPolicy::SkipAlways;

	return ImportPolicy::SkipExisting;
}

static void OutputUsage(const hg::CmdLineFormat &cmd_format) {
	hg::debug((std::string("Usage: gltf_importer ") + hg::word_wrap(hg::FormatCmdLineArgs(cmd_format), 80, 21) + "\n").c_str());
	hg::debug((hg::FormatCmdLineArgsDescription(cmd_format)).c_str());
	hg::debug((std::string("Header-only tiny glTF 2.0 loader and serializer.The MIT License (MIT)") +
			   "Copyright (c) 2015 - 2019 Syoyo Fujita, Aur?lien Chatelain and manycontributors." +
			   "Permission is hereby granted, free of charge, to any person obtaining a copy" +
			   "of this software and associated documentation files (the \" Software \"), to deal" +
			   "in the Software without restriction, including without limitation the rights" +
			   "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell" +
			   "copies of the Software, and to permit persons to whom the Software is" +
			   "furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in" +
			   "all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED \" AS IS \", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR" +
			   "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY," +
			   "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE" +
			   "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER" +
			   "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM," +
			   "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.")
				  .c_str());
}

//
static std::mutex log_mutex;
static bool quiet = false;

int main(int argc, const char **argv) {
	hg::set_log_hook(
		[](const char *msg, int mask, const char *details, void *user) {
			if (quiet && !(mask & hg::LL_Error))
				return; // skip masked entries

			std::lock_guard<std::mutex> guard(log_mutex);
			std::cout << msg << std::endl;
		},
		nullptr);
	hg::set_log_level(hg::LL_All);

	hg::debug(hg::format("GLTF->GS Converter %1 (%2)").arg(hg::get_version_string()).arg(hg::get_build_sha()).c_str());

	hg::CmdLineFormat cmd_format = {
		{
			{"-fix-geometry-orientation", "Bake a 90? rotation on the X axis of exported geometries"},
			{"-recalculate-normal", "Recreate the vertex normals of exported geometries"},
			{"-recalculate-tangent", "Recreate the vertex tangent frames of exported geometries"},
			{"-detect-geometry-instances", "Detect and optimize geometry instances"},
			{"-anim-to-file", "Scene animations will be exported to separate files and not embedded in scene"},
			{"-quiet", "Quiet log, only log errors"},
		},
		{
			{"-out", "Output directory", true},
			{"-base-resource-path", "Transform references to assets in this directory to be relative", true},
			{"-name", "Specify the output scene name", true},
			{"-prefix", "Specify the file system prefix from which relative assets are to be loaded from", true},
			{"-all-policy", "All file output policy (skip, overwrite, rename or skip_always) [default=skip]", true},
			{"-geometry-policy", "Geometry file output policy (skip, overwrite, rename or skip_always) [default=skip]", true},
			{"-material-policy", "Material file output policy (skip, overwrite, rename or skip_always) [default=skip]", true},
			{"-texture-policy", "Texture file output policy (skip, overwrite, rename or skip_always) [default=skip]", true},
			{"-scene-policy", "Scene file output policy (skip, overwrite, rename or skip_always) [default=skip]", true},
			{"-anim-policy",
				"Animation file output policy (skip, overwrite, rename or skip_always) (note: only applies when saving animations to their own "
				"file) [default=skip]",
				true},
			{"-geometry-scale", "Factor used to scale exported geometries", true},
			{"-finalizer-script", "Path to the Lua finalizer script", true},
			{"-shader", "Material pipeline shader [default=core/shader/pbr.hps]", true},
		},
		{
			{"input", "Input FBX file to convert"},
		},
		{
			{"-o", "-out"},
			{"-h", "-help"},
			{"-q", "-quiet"},
			{"-s", "-shader"},
		},
	};

	hg::CmdLineContent cmd_content;
	if (!hg::ParseCmdLine({argv + 1, argv + argc}, cmd_format, cmd_content)) {
		OutputUsage(cmd_format);
		return -1;
	}

	//
	Config config;
	config.base_output_path = hg::CleanPath(hg::GetCmdLineSingleValue(cmd_content, "-out", "./"));
	config.prj_path = hg::CleanPath(hg::GetCmdLineSingleValue(cmd_content, "-base-resource-path", ""));
	config.name = hg::CleanPath(hg::GetCmdLineSingleValue(cmd_content, "-name", ""));
	config.prefix = hg::GetCmdLineSingleValue(cmd_content, "-prefix", "");

	config.import_policy_anim = config.import_policy_geometry = config.import_policy_material = config.import_policy_scene = config.import_policy_texture =
		ImportPolicyFromString(hg::GetCmdLineSingleValue(cmd_content, "-all-policy", "skip"));
	config.import_policy_geometry = ImportPolicyFromString(hg::GetCmdLineSingleValue(cmd_content, "-geometry-policy", "skip"));
	config.import_policy_material = ImportPolicyFromString(hg::GetCmdLineSingleValue(cmd_content, "-material-policy", "skip"));
	config.import_policy_texture = ImportPolicyFromString(hg::GetCmdLineSingleValue(cmd_content, "-texture-policy", "skip"));
	config.import_policy_scene = ImportPolicyFromString(hg::GetCmdLineSingleValue(cmd_content, "-scene-policy", "skip"));
	config.import_policy_anim = ImportPolicyFromString(hg::GetCmdLineSingleValue(cmd_content, "-anim-policy", "skip"));

	config.geometry_scale = hg::GetCmdLineSingleValue(cmd_content, "-geometry-scale", 1.f);

	config.recalculate_normal = hg::GetCmdLineFlagValue(cmd_content, "-recalculate-normal");
	config.recalculate_tangent = hg::GetCmdLineFlagValue(cmd_content, "-recalculate-tangent");

	config.finalizer_script = hg::GetCmdLineSingleValue(cmd_content, "-finalizer-script", "");

	config.shader = hg::GetCmdLineSingleValue(cmd_content, "-shader", "");

	quiet = hg::GetCmdLineFlagValue(cmd_content, "-quiet");

	//
	if (cmd_content.positionals.size() != 1) {
		hg::debug("No input file");
		OutputUsage(cmd_format);
		return -2;
	}

	//
	config.input_path = cmd_content.positionals[0];
	auto res = ImportGltfScene(cmd_content.positionals[0], config);

	const auto msg = std::string("[ImportScene") + std::string(res ? ": OK]" : ": KO]");
	hg::log(msg.c_str());

	return res ? EXIT_SUCCESS : EXIT_FAILURE;
}
