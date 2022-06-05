// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include <assimp/Importer.hpp>
#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

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
#include <foundation/file.h>
#include <foundation/format.h>
#include <foundation/log.h>
#include <foundation/math.h>
#include <foundation/matrix3.h>
#include <foundation/matrix4.h>
#include <foundation/pack_float.h>
#include <foundation/path_tools.h>
#include <foundation/projection.h>
#include <foundation/string.h>
#include <foundation/time.h>
#include <foundation/vector3.h>

#include "fabgen.h"

#include "bind_Lua.h"

#include <iostream>
#include <mutex>

#include "fbx_optimizegraph.h"

enum class ImportPolicy { SkipExisting, Overwrite, Rename, SkipAlways };

struct Config {
	ImportPolicy import_policy_geometry{ImportPolicy::SkipExisting}, import_policy_material{ImportPolicy::SkipExisting},
		import_policy_texture{ImportPolicy::SkipExisting}, import_policy_scene{ImportPolicy::SkipExisting}, import_policy_anim{ImportPolicy::SkipExisting};

	std::string input_file; // store the input file in the config too, this is used to resolve texture paths

	std::string name; // output name (may be empty)
	std::string base_output_path{"./"};
	std::string prj_path;
	std::string prefix;
	std::string profile, shader;

	float scene_scale{1.f};

	bool import_animation{true};
	float anim_simplify_translation_tolerance = 0.001f;
	float anim_simplify_rotation_tolerance = 0.05f; // in degrees
	float anim_simplify_scale_tolerance = 0.001f;

	float max_smoothing_angle{0.7f};
	bool recalculate_normal{false}, recalculate_tangent{false};
	bool calculate_normal_if_missing{false}, calculate_tangent_if_missing{false};
	// bool detect_geometry_instances{false};
	// bool anim_to_file{false};
	bool merge_meshes{false};

	std::string finalizer_script;
};

struct ExportMap {
	std::vector<const aiNode *> all_nodes;

	std::set<std::string> exported_object_names;
	std::map<const aiNode *, hg::Node> exported_nodes;
	std::map<std::string, hg::TextureRef> exported_textures;

	// FBX/gltf seem to be in ms. // see https://github.com/assimp/assimp/issues/3462
	bool fps_workaround = false;
};

static hg::Node ExportNode(
	const aiScene *ai_scene, const aiNode *ai_node, hg::Scene &scene, ExportMap &export_map, const Config &config, hg::PipelineResources &resources);

static hg::Mat4 AIMatrixToMatrix4(const aiMatrix4x4 &ai_m) {
	hg::Mat4 m;
	for (int i = 0; i < 3; ++i)
		for (int j = 0; j < 4; ++j)
			m.m[i][j] = float(ai_m[i][j]);
	return m;
}

static bool GetOutputPath(
	std::string &path, const std::string &base, const std::string &name, const std::string &prefix, const std::string &ext, ImportPolicy import_policy) {
	if (base.empty())
		return false;

	const auto filename = hg::CleanFileName(name.empty() ? prefix : (prefix.empty() ? name : prefix + "-" + name));

	path = hg::CleanPath(base + "/" + filename + "." + ext);

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
static std::string MakeRelativeResourceName(const std::string &name, const std::string &base_path, const std::string &prefix) {
	if (hg::starts_with(name, base_path, hg::case_sensitivity::insensitive)) {
		const auto stripped_name = hg::lstrip(hg::slice(name, base_path.length()), "/");
		return prefix.empty() ? stripped_name : prefix + "/" + stripped_name;
	}
	return name;
}

static void ExportMotions(const aiScene *ai_scene, hg::Scene &scene, ExportMap &export_map, const Config &config, hg::PipelineResources &resources) {
	if (!config.import_animation || config.merge_meshes)
		return;

	for (unsigned int n = 0; n < ai_scene->mNumAnimations; ++n) {

		const aiAnimation *ai_anim = ai_scene->mAnimations[n];

		std::map<std::string, const aiNodeAnim *> channels;
		std::map<std::string, int> channels_indices;

		for (unsigned int i = 0; i < ai_anim->mNumChannels; i++) {
			auto channel = ai_anim->mChannels[i];
			std::string channelname = channel->mNodeName.C_Str();
			__ASSERT__(channels.find(channelname) == channels.end());
			channels[channelname] = channel;
			channels_indices[channelname] = i;
		}

		double start = 0.0;
		const double duration = ai_anim->mDuration;

		// create Harfang take
		hg::SceneAnim scene_anim;

		scene_anim.name = ai_anim->mName.C_Str();

		double ticksPerSecond = ai_anim->mTicksPerSecond;
		if (ticksPerSecond == 0)
			ticksPerSecond = 30.0; // TODO: fps import option in case this is not set?

		// FBX/gltf seem to be in ms. // see https://github.com/assimp/assimp/issues/3462
		if (export_map.fps_workaround) {
			ticksPerSecond = 1000.0;
		}

		const double tStart = 0.0f;
		double tEnd = ai_anim->mDuration / ticksPerSecond;

		scene_anim.t_start = hg::time_from_sec_f(float(tStart));
		scene_anim.t_end = hg::time_from_sec_f(float(tEnd));

		for (auto i : export_map.exported_nodes) {
			if (!i.second)
				continue;

			auto node_channel_it = channels.find(i.second.GetName());
			if (node_channel_it == channels.end())
				continue; // no channel found for this node
			auto node_channel = node_channel_it->second;

			// create animation default tracks
			auto pos_track = hg::AnimTrackHermiteT<hg::Vec3>(); // or AnimKeyHermiteT?
			pos_track.target = "Position";
			// auto rot_track = std::make_shared<hg::AnimTrackRotation>("Transform.Rotation");
			auto rot_track = hg::AnimTrackT<hg::Quaternion>();
			rot_track.target = "Rotation";
			auto scl_track = hg::AnimTrackHermiteT<hg::Vec3>();
			scl_track.target = "Scale";

			for (unsigned int k = 0; k < node_channel->mNumPositionKeys; k++) {
				const aiVectorKey &ai_key = node_channel->mPositionKeys[k];

				hg::time_ns hg_t = hg::time_from_sec_f(float(ai_key.mTime / ticksPerSecond));

				hg::AnimKeyHermiteT<hg::Vec3> pos_key;
				pos_key.v = hg::Vec3(ai_key.mValue.x, ai_key.mValue.y, ai_key.mValue.z);
				pos_key.t = hg_t;
				pos_track.keys.push_back(pos_key);
			}

			for (unsigned int k = 0; k < node_channel->mNumRotationKeys; k++) {
				const aiQuatKey &ai_key = node_channel->mRotationKeys[k];

				hg::time_ns hg_t = hg::time_from_sec_f(float(ai_key.mTime / ticksPerSecond));

				hg::AnimKeyT<hg::Quaternion> rot_key;
				rot_key.v = hg::Quaternion(ai_key.mValue.x, ai_key.mValue.y, ai_key.mValue.z, ai_key.mValue.w);
				rot_key.t = hg_t;
				rot_track.keys.push_back(rot_key);
			}

			for (unsigned int k = 0; k < node_channel->mNumScalingKeys; k++) {
				const aiVectorKey &ai_key = node_channel->mScalingKeys[k];

				hg::time_ns hg_t = hg::time_from_sec_f(float(ai_key.mTime / ticksPerSecond));

				hg::AnimKeyHermiteT<hg::Vec3> scl_key;
				scl_key.v = hg::Vec3(ai_key.mValue.x, ai_key.mValue.y, ai_key.mValue.z);
				scl_key.t = hg_t;
				scl_track.keys.push_back(scl_key);
			}

			hg::ConformAnimTrackKeys(rot_track);

			// cleanup tracks
			float simplify_translation_tolerance = config.anim_simplify_translation_tolerance;
			float simplify_rotation_tolerance = config.anim_simplify_rotation_tolerance * hg::Pi / 180.0f;
			float simplify_scale_tolerance = config.anim_simplify_scale_tolerance;

			if (simplify_translation_tolerance > 0) {
				auto removed = hg::SimplifyAnimTrackT<hg::AnimTrackHermiteT<hg::Vec3>, hg::Vec3>(pos_track, simplify_translation_tolerance);
				hg::debug(hg::format("Clean position track: %1").arg(removed));
			}
			if (simplify_rotation_tolerance > 0) {
				auto removed = hg::SimplifyAnimTrackT<hg::AnimTrackT<hg::Quaternion>, hg::Quaternion>(rot_track, simplify_rotation_tolerance);
				hg::debug(hg::format("Clean rotation track: %1").arg(removed));
			}
			if (simplify_scale_tolerance > 0) {
				auto removed = hg::SimplifyAnimTrackT<hg::AnimTrackHermiteT<hg::Vec3>, hg::Vec3>(scl_track, simplify_scale_tolerance);
				hg::debug(hg::format("Clean rotation track: %1").arg(removed));
			}

			// create node animation
			auto anim = hg::Anim();

			anim.vec3_tracks.push_back(std::move(pos_track));
			anim.quat_tracks.push_back(std::move(rot_track));
			anim.vec3_tracks.push_back(std::move(scl_track));

			bool is_empty_anim = true;
			for (auto &track : anim.vec3_tracks)
				if (track.keys.size() > 1) {
					is_empty_anim = false;
					break;
				}
			for (auto &track : anim.quat_tracks)
				if (track.keys.size() > 1) {
					anim.flags |= hg::AF_UseQuaternionForRotation;
					is_empty_anim = false;
					break;
				}
			for (auto &track : anim.color_tracks)
				if (track.keys.size() > 1) {
					is_empty_anim = false;
					break;
				}

			if (is_empty_anim) {
				hg::debug(hg::format("Skipping animation for target '%1' as it contains no animation").arg(i.second.GetName().c_str()));
				continue;
			}

			// store animation
			auto anim_ref = scene.AddAnim(std::move(anim));

			// add anim_node to the scene_anim
			hg::NodeAnim node_anim;
			node_anim.anim = anim_ref;
			node_anim.node = i.second.ref;
			scene_anim.node_anims.push_back(node_anim);
		}

		// add animation take to the scene
		scene.AddSceneAnim(scene_anim);
	}
}

//----
static void AppendGeometrySkin(const aiScene *ai_scene, hg::Geometry &geo, aiMesh *ai_mesh, const std::map<const aiBone *, unsigned int> &bone_indices) {

	auto start_skin = geo.skin.size();

	geo.skin.resize(geo.vtx.size());
	for (size_t n = start_skin; n < geo.vtx.size(); ++n)
		for (int j = 0; j < 4; ++j) {
			geo.skin[n].index[j] = 0;
			geo.skin[n].weight[j] = 0;
		}

	const auto bone_count = ai_mesh->mNumBones;

	for (unsigned int n = 0; n < bone_count; ++n) {
		const auto bone = ai_mesh->mBones[n];
		auto bone_idx = bone_indices.find(bone)->second;

		// import weights
		for (unsigned int i = 0; i < bone->mNumWeights; i++) {

			const aiVertexWeight bw = bone->mWeights[i];

			auto skin = &geo.skin[start_skin + bw.mVertexId];
			auto weight = hg::pack_float<uint8_t>(float(bw.mWeight));

			for (int c = 0; c < 4; ++c)
				if (weight > skin->weight[c]) {
					// shift the lower influences out
					for (int j = 4 - 1; j > c; --j) {
						skin->index[j] = skin->index[j - 1];
						skin->weight[j] = skin->weight[j - 1];
					}

					// insert new influence
					skin->index[c] = hg::numeric_cast<uint16_t>(bone_idx);
					skin->weight[c] = weight;
					break;
				}
		}
	}
}

static hg::TextureRef ExportTexture(const aiScene *ai_scene, const aiString &ai_path, uint32_t bgfx_flags, const std::string &meta, ExportMap &export_map,
	const Config &config, hg::PipelineResources &resources) {

	if (ai_path.length == 0)
		return hg::TextureRef();

	// have we already exported this texture?
	auto it = export_map.exported_textures.find(ai_path.C_Str());
	if (it != export_map.exported_textures.end()) {
		return it->second;
	}

	std::string dst_path;

	// is this an embedded texture?
	const aiTexture *embedded_texture = ai_scene->GetEmbeddedTexture(ai_path.C_Str());
	if (embedded_texture != nullptr) {
		if (embedded_texture->mHeight == 0) {
			// If mHeight = 0 this is a pointer to a memory buffer of size mWidth containing the compressed texture
			// and achFormatHint contains the file extension

			std::string src_path = ai_path.C_Str();

			// happens for running_man.glb
			if (hg::GetFileExtension(src_path) == "") {
				src_path += "." + std::string(embedded_texture->achFormatHint);
			}

			bool should_write =
				GetOutputPath(dst_path, config.base_output_path, hg::GetFileName(src_path), {}, hg::GetFileExtension(src_path), config.import_policy_texture);

			if (should_write) {
				hg::ScopedFile file(hg::OpenWrite(dst_path.c_str()));
				hg::Write(file, embedded_texture->pcData, embedded_texture->mWidth);
			}
		} else {
			// TODO: handle various raw formats depending on the metadata in achFormatHint
			hg::error(hg::format("Unsupported file format '%1'").arg(ai_path.C_Str()));
		}
	} else {
		if (ai_path.length > 0) {

			std::string src_path = ai_path.C_Str();

			// if src_path is relative, make it absolute
			if (!hg::IsPathAbsolute(src_path.c_str())) {
				src_path = hg::GetFilePath(config.input_file) + src_path;
			}

			if (!hg::Exists(src_path.c_str())) {
				src_path = hg::CutFilePath(src_path);

				if (!hg::Exists(src_path.c_str())) {
					hg::error(hg::format("Missing texture file '%1'").arg(src_path));
					return {};
				}
			}

			bool should_write =
				GetOutputPath(dst_path, config.base_output_path, hg::GetFileName(src_path), {}, hg::GetFileExtension(src_path), config.import_policy_texture);

			if (should_write)
				if (!hg::CopyFile(hg::ansi_to_utf8(src_path).c_str(), hg::ansi_to_utf8(dst_path).c_str())) {
					hg::error(hg::format("Failed to copy texture file '%1' to '%2'").arg(src_path).arg(dst_path));
					return {};
				}
		}
	}

	dst_path = MakeRelativeResourceName(dst_path, config.prj_path, config.prefix);

	auto result = resources.textures.Add(dst_path.c_str(), {bgfx_flags, BGFX_INVALID_HANDLE});
	export_map.exported_textures[ai_path.C_Str()] = result;
	return result;
}

//
enum class TextureType {
	Diffuse = 0,
	Lightmap,
	Emissive,
	Ambient,
	Specular,
	Normal,
	Shininess,
	Bump,
	TransparentColor,
	Reflection,
	RoughnessMap,
	MetalnessMap,
	BaseColorMap,
	BumpMap,
	ReflectivityMap,
	EmissionMap,
	EmitColorMap,
	ReflColorMap,
	Last
};

static const std::map<aiTextureType, TextureType> &GetTextureTypeMap() {
	static std::map<aiTextureType, TextureType> map = {
		{aiTextureType_DIFFUSE, TextureType::Diffuse}, {aiTextureType_SPECULAR, TextureType::Specular}, {aiTextureType_AMBIENT, TextureType::Ambient},
		{aiTextureType_EMISSIVE, TextureType::Emissive},
		{aiTextureType_HEIGHT, TextureType::Normal}, // found a normal map exported as "height" while exporting a .obj from blender
		{aiTextureType_NORMALS, TextureType::Normal}, {aiTextureType_SHININESS, TextureType::Shininess},
		{aiTextureType_OPACITY, TextureType::TransparentColor}, // check if this is fit
		{aiTextureType_DISPLACEMENT, TextureType::BumpMap}, // check if this is fit
		{aiTextureType_LIGHTMAP, TextureType::Lightmap}, {aiTextureType_REFLECTION, TextureType::ReflColorMap},
		{aiTextureType_BASE_COLOR, TextureType::BaseColorMap}, {aiTextureType_NORMAL_CAMERA, TextureType::Last}, // not supported
		{aiTextureType_EMISSION_COLOR, TextureType::Emissive}, {aiTextureType_METALNESS, TextureType::MetalnessMap},
		{aiTextureType_DIFFUSE_ROUGHNESS, TextureType::RoughnessMap}, {aiTextureType_AMBIENT_OCCLUSION, TextureType::Last}, // not supported
	};

	return map;
}

//
static const bool PictureHasTransparency(const hg::Picture &pic) {
	if (pic.GetFormat() != hg::PF_RGBA32)
		return false;

	const size_t size = size_of(pic.GetFormat());
	uint8_t *data = pic.GetData();
	for (int i = 0; i < pic.GetWidth() * pic.GetHeight(); ++i)
		if (data[3] < 255)
			return true;
		else
			data += size;
	return false;
}

//
static hg::Material ExportMaterial(
	const aiScene *ai_scene, const aiMaterial *ai_material, bool use_skin, ExportMap &export_map, const Config &config, hg::PipelineResources &resources) {
	hg::Color diffuse = {0.5f, 0.5f, 0.5f, 1.f}, emissive = {0, 0, 0, 1}, specular = {0.5f, 0.5f, 0.5f, 1.f}, ambient = {0, 0, 0, 1};

	float glossiness{1.f};
	float reflection{1.f};

	hg::debug(hg::format("Exporting material '%1'").arg(ai_material->GetName().C_Str()));
	aiColor3D color(0.f, 0.f, 0.f);
	float flt = 0.0f;

	if (AI_SUCCESS == ai_material->Get(AI_MATKEY_OPACITY, flt)) {
		diffuse.a = flt;
	}
	// we skip this if Opacity is set. fbx_converter seems to use it anyway
	// see "diffuse.a = 1.f - static_cast<float>(fbx_lambert->TransparencyFactor.Get());"
	// but this causes the spaceship assemble-demo-assets\space_station\work\space_ship.fbx to be transparent
	// when according to the fbx viewer, it shouldn't
	else if (AI_SUCCESS == ai_material->Get(AI_MATKEY_TRANSPARENCYFACTOR, flt)) {
		diffuse.a = 1.0f - flt;
	}
	if (AI_SUCCESS == ai_material->Get(AI_MATKEY_SHININESS, flt)) {
		glossiness = hg::Clamp(flt / 64.f, 0.01f, 0.5f); // completely random conversion factor
	}
	if (AI_SUCCESS == ai_material->Get(AI_MATKEY_REFLECTIVITY, flt)) {
		reflection = flt;
	}
	if (AI_SUCCESS == ai_material->Get(AI_MATKEY_SHININESS_STRENGTH, flt)) {
		glossiness *= hg::Clamp(glossiness * flt, 0.01f, 0.5f);
	}

	if (AI_SUCCESS == ai_material->Get(AI_MATKEY_REFRACTI, flt)) {}

	if (AI_SUCCESS == ai_material->Get(AI_MATKEY_COLOR_DIFFUSE, color)) {
		diffuse.r = color.r;
		diffuse.g = color.g;
		diffuse.b = color.b;
	}

	if (AI_SUCCESS == ai_material->Get(AI_MATKEY_COLOR_AMBIENT, color)) {
		ambient.r = color.r;
		ambient.g = color.g;
		ambient.b = color.b;
	}

	if (AI_SUCCESS == ai_material->Get(AI_MATKEY_COLOR_SPECULAR, color)) {
		specular.r = color.r;
		specular.g = color.g;
		specular.b = color.b;
	}

	if (AI_SUCCESS == ai_material->Get(AI_MATKEY_COLOR_EMISSIVE, color)) {
		emissive.r = color.r;
		emissive.g = color.g;
		emissive.b = color.b;
	}

	// pbr
	if (AI_SUCCESS == ai_material->Get(AI_MATKEY_BASE_COLOR, color)) {
		diffuse.r = color.r;
		diffuse.g = color.g;
		diffuse.b = color.b;
	}

	std::array<hg::TextureRef, (size_t)TextureType::Last> texture;

	aiString path;
	aiTextureMapMode mapMode[3] = {aiTextureMapMode_Wrap};
	const auto &textureTypeMap = GetTextureTypeMap();

	for (aiTextureType ai_tex_type = aiTextureType_NONE; ai_tex_type < (int)aiTextureType_UNKNOWN; ai_tex_type = (aiTextureType)((int)ai_tex_type + 1)) {
		auto it = textureTypeMap.find(ai_tex_type);
		if (it == textureTypeMap.end())
			continue;
		TextureType hg_textype = it->second;
		if (hg_textype == TextureType::Last)
			continue;

		aiTextureMapping mapping = (aiTextureMapping)0;
		unsigned int uvindex = 0;
		float blend = 0.0f;
		aiTextureOp op = (aiTextureOp)0;

		// TODO: handle multiple textures if needed
		// TODO: handle modulate with lightmaps

		if (AI_SUCCESS == ai_material->GetTexture(ai_tex_type, 0, &path, &mapping, &uvindex, &blend, &op, mapMode)) {

			uint32_t flags = BGFX_SAMPLER_NONE;

			for (int i = 0; i < 3; i++) {
				if (mapMode[i] == aiTextureMapMode::aiTextureMapMode_Wrap) {
					flags |= 0; // default
				} else if (mapMode[i] == aiTextureMapMode::aiTextureMapMode_Clamp) {
					switch (i) {
						case 0:
							flags |= BGFX_SAMPLER_U_CLAMP;
							break;
						case 1:
							flags |= BGFX_SAMPLER_V_CLAMP;
							break;
						case 2:
							flags |= BGFX_SAMPLER_W_CLAMP;
							break;
					}
				} else if (mapMode[i] == aiTextureMapMode::aiTextureMapMode_Decal) {
					hg::warn(hg::format("Unsupported mapping mode decal '%1'").arg(path.C_Str()));
				} else if (mapMode[i] == aiTextureMapMode::aiTextureMapMode_Mirror) {
					switch (i) {
						case 0:
							flags |= BGFX_SAMPLER_U_MIRROR;
							break;
						case 1:
							flags |= BGFX_SAMPLER_V_MIRROR;
							break;
						case 2:
							flags |= BGFX_SAMPLER_W_MIRROR;
							break;
					}
				} else {
					hg::warn(hg::format("Unsupported mapping mode decal '%1'").arg(path.C_Str()));
				}
			}

			std::string meta_RAW_text("{\"profiles\": {\"default\": {\"compression\": \"RAW\"}}}");
			std::string meta_BC1_text("{\"profiles\": {\"default\": {\"compression\": \"BC1\"}}}");
			std::string meta_BC3_text("{\"profiles\": {\"default\": {\"compression\": \"BC3\"}}}");
			std::string meta_BC4_text("{\"profiles\": {\"default\": {\"compression\": \"BC4\"}}}");
			std::string meta_BC5_text("{\"profiles\": {\"default\": {\"compression\": \"BC5\"}}}");
			std::string meta_BC6_text("{\"profiles\": {\"default\": {\"compression\": \"BC6\"}}}");
			std::string meta_BC7_text("{\"profiles\": {\"default\": {\"compression\": \"BC7\"}}}");
			std::string meta_BC7_srgb_text("{\"profiles\": {\"default\": {\"compression\": \"BC7\", \"sRGB\": true}}}");

			std::string meta;
			if (hg_textype == TextureType::Diffuse || hg_textype == TextureType::BaseColorMap || hg_textype == TextureType::Emissive ||
				hg_textype == TextureType::Emissive) {
				meta = meta_BC7_srgb_text;
			}
			if (hg_textype == TextureType::Normal) {
				meta = meta_BC5_text;
			}

			texture[(size_t)hg_textype] = ExportTexture(ai_scene, path, flags, meta, export_map, config, resources);
		}
	}

	//
	hg::Material mat;

	if (use_skin)
		mat.flags |= hg::MF_EnableSkinning;

	hg::TextureRef diffuse_map = hg::InvalidTextureRef;
	if (diffuse_map == hg::InvalidTextureRef)
		diffuse_map = texture[(size_t)TextureType::Diffuse];
	if (diffuse_map == hg::InvalidTextureRef)
		diffuse_map = texture[(size_t)TextureType::BaseColorMap];

	hg::TextureRef specular_map = hg::InvalidTextureRef;
	if (specular_map == hg::InvalidTextureRef)
		specular_map = texture[(size_t)TextureType::Specular];
	if (specular_map == hg::InvalidTextureRef)
		specular_map = texture[(size_t)TextureType::ReflColorMap];

	hg::TextureRef normal_map = texture[(size_t)TextureType::Normal];
	hg::TextureRef light_map = texture[(size_t)TextureType::Lightmap];

	hg::TextureRef self_map = hg::InvalidTextureRef;
	if (self_map == hg::InvalidTextureRef)
		self_map = texture[(size_t)TextureType::Emissive];
	if (self_map == hg::InvalidTextureRef)
		self_map = texture[(size_t)TextureType::EmissionMap];
	if (self_map == hg::InvalidTextureRef)
		self_map = texture[(size_t)TextureType::EmitColorMap];

	hg::TextureRef opacity_map = texture[(size_t)TextureType::TransparentColor];
	hg::TextureRef shininess_map = texture[(size_t)TextureType::Shininess];
	hg::TextureRef reflection_map = texture[(size_t)TextureType::Reflection];
	hg::TextureRef ambient_map = texture[(size_t)TextureType::Ambient];

	hg::TextureRef bump_map = hg::InvalidTextureRef;
	if (bump_map == hg::InvalidTextureRef)
		bump_map = texture[(size_t)TextureType::Bump];
	if (bump_map == hg::InvalidTextureRef)
		bump_map = texture[(size_t)TextureType::BumpMap];

	hg::TextureRef roughness_map = texture[(size_t)TextureType::RoughnessMap];
	hg::TextureRef metalness_map = texture[(size_t)TextureType::MetalnessMap];

	std::string shader;

	if (config.profile == "pbr_default") {
		shader = "core/shader/pbr.hps";

		mat.values["uBaseOpacityColor"] = {bgfx::UniformType::Vec4, {diffuse.r, diffuse.g, diffuse.b, diffuse.a}};
		mat.values["uOcclusionRoughnessMetalnessColor"] = {bgfx::UniformType::Vec4, {1.f, 0.5f, 0.f, 0.f}};
		mat.values["uSelfColor"] = {bgfx::UniformType::Vec4, {emissive.r, emissive.g, emissive.b, -1.f}};

		if (diffuse_map != hg::InvalidTextureRef) {
			hg::debug(hg::format("    - uBaseOpacityMap: %1").arg(resources.textures.GetName(diffuse_map)));
			mat.textures["uBaseOpacityMap"] = {diffuse_map, 0};
		} else if (diffuse.a < 1.f) {
			SetMaterialBlendMode(mat, hg::BM_Alpha);
		}

		if (specular_map != hg::InvalidTextureRef) {
			hg::debug(hg::format("    - uOcclusionRoughnessMetalnessMap: %1").arg(resources.textures.GetName(specular_map)));
			mat.textures["uOcclusionRoughnessMetalnessMap"] = {specular_map, 1};
		}

		if (normal_map != hg::InvalidTextureRef) {
			hg::debug(hg::format("    - uNormalMap: %1").arg(resources.textures.GetName(normal_map)));
			mat.textures["uNormalMap"] = {normal_map, 2};
		}

		if (self_map != hg::InvalidTextureRef) {
			hg::debug(hg::format("    - uSelfMap: %1").arg(resources.textures.GetName(self_map)));
			mat.textures["uSelfMap"] = {self_map, 4};
		}
	} else if (config.profile == "pbr_physical") {
		shader = "core/shader/pbr.hps";

		// pbr
		if (AI_SUCCESS == ai_material->Get(AI_MATKEY_BASE_COLOR, color)) {
			diffuse.r = color.r;
			diffuse.g = color.g;
			diffuse.b = color.b;
		}

		float emission_factor = 1.0f;
		if (AI_SUCCESS == ai_material->Get(AI_MATKEY_EMISSIVE_INTENSITY, emission_factor)) {
			emissive = {emissive.r * emission_factor, emissive.g * emission_factor, emissive.b * emission_factor,
				-1.f}; // the -1 was there in the fbx_converter, not sure about its purpose
		}

		// set occlusion to 1
		specular.r = 1.f;

		float roughness_factor = 1.0f;
		if (AI_SUCCESS == ai_material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness_factor)) {
			specular.g = roughness_factor;
		}

		float metallic_factor = 0.0f;
		if (AI_SUCCESS == ai_material->Get(AI_MATKEY_METALLIC_FACTOR, metallic_factor)) {
			specular.b = metallic_factor;
		}

		mat.values["uBaseOpacityColor"] = {bgfx::UniformType::Vec4, {diffuse.r, diffuse.g, diffuse.b, diffuse.a}};
		mat.values["uOcclusionRoughnessMetalnessColor"] = {bgfx::UniformType::Vec4, {specular.r, specular.g, specular.b, glossiness}};
		mat.values["uSelfColor"] = {bgfx::UniformType::Vec4, {emissive.r, emissive.g, emissive.b, -1.f}};

		if (diffuse_map != hg::InvalidTextureRef) {
			hg::debug(hg::format("    - uBaseOpacityMap: %1").arg(resources.textures.GetName(diffuse_map)));
			mat.textures["uBaseOpacityMap"] = {diffuse_map, 0};
		} else if (diffuse.a < 1.f) {
			SetMaterialBlendMode(mat, hg::BM_Alpha);
		}
		if (opacity_map != hg::InvalidTextureRef)
			SetMaterialBlendMode(mat, hg::BM_Alpha);
		else {
			// check if there is alpha in the base color map
			hg::Picture pic;
			if (LoadPicture(pic, (config.prj_path + "/" + resources.textures.GetName(diffuse_map)).c_str()) && PictureHasTransparency(pic))
				SetMaterialBlendMode(mat, hg::BM_Alpha);
		}

		if (roughness_map != hg::InvalidTextureRef || metalness_map != hg::InvalidTextureRef) {
			if (roughness_map != hg::InvalidTextureRef) {
				hg::debug(hg::format("    - uOcclusionRoughnessMetalnessMap: %1").arg(resources.textures.GetName(roughness_map)));
				mat.textures["uOcclusionRoughnessMetalnessMap"] = {roughness_map, 1};
			} else if (metalness_map != hg::InvalidTextureRef) {
				hg::debug(hg::format("    - uOcclusionRoughnessMetalnessMap: %1").arg(resources.textures.GetName(metalness_map)));
				mat.textures["uOcclusionRoughnessMetalnessMap"] = {metalness_map, 1};
			}
		}

		if (bump_map != hg::InvalidTextureRef) {
			hg::debug(hg::format("    - uNormalMap: %1").arg(resources.textures.GetName(bump_map)));
			mat.textures["uNormalMap"] = {bump_map, 2};
		} else if (normal_map != hg::InvalidTextureRef) {
			hg::debug(hg::format("    - uNormalMap: %1").arg(resources.textures.GetName(normal_map)));
			mat.textures["uNormalMap"] = {normal_map, 2};
		}

		if (self_map != hg::InvalidTextureRef) {
			hg::debug(hg::format("    - uSelfMap: %1").arg(resources.textures.GetName(self_map)));
			mat.textures["uSelfMap"] = {self_map, 4};
		}
	} else {
		if (config.profile != "default")
			hg::warn(hg::format("Unknown material profile '%1', using 'default' profile").arg(config.profile));

		shader = "core/shader/default.hps";

		mat.values["uDiffuseColor"] = {bgfx::UniformType::Vec4, {diffuse.r, diffuse.g, diffuse.b, diffuse.a}};
		mat.values["uSpecularColor"] = {bgfx::UniformType::Vec4, {specular.r, specular.g, specular.b, glossiness}};
		mat.values["uSelfColor"] = {bgfx::UniformType::Vec4, {emissive.r, emissive.g, emissive.b, -1.f}};

		hg::debug(hg::format("    - uDiffuseColor: %1, %2, %3 - Alpha: %4").arg(diffuse.r).arg(diffuse.g).arg(diffuse.b).arg(diffuse.a));
		hg::debug(hg::format("    - SpecularColor: %1, %2, %3 - Glossiness: %4").arg(specular.r).arg(specular.g).arg(specular.b).arg(glossiness));
		hg::debug(hg::format("    - uSelfColor: %1, %2, %3 - Reserved").arg(emissive.r).arg(emissive.g).arg(emissive.b));

		if (diffuse_map != hg::InvalidTextureRef) {
			hg::debug(hg::format("    - uDiffuseMap: %1").arg(resources.textures.GetName(diffuse_map)));
			mat.textures["uDiffuseMap"] = {diffuse_map, 0};
		} else if (diffuse.a < 1.f) {
			SetMaterialBlendMode(mat, hg::BM_Alpha);
		}

		if (specular_map != hg::InvalidTextureRef) {
			hg::debug(hg::format("    - uSpecularMap: %1").arg(resources.textures.GetName(specular_map)));
			mat.textures["uSpecularMap"] = {specular_map, 1};
		}

		if (normal_map != hg::InvalidTextureRef) {
			hg::debug(hg::format("    - uNormalMap: %1").arg(resources.textures.GetName(normal_map)));
			mat.textures["uNormalMap"] = {normal_map, 2};
		}

		if (light_map != hg::InvalidTextureRef) {
			hg::debug(hg::format("    - uLightMap: %1").arg(resources.textures.GetName(light_map)));
			mat.textures["uLightMap"] = {light_map, 3};
		}

		if (self_map != hg::InvalidTextureRef) {
			hg::debug(hg::format("    - uSelfMap: %1").arg(resources.textures.GetName(self_map)));
			mat.textures["uSelfMap"] = {self_map, 4};
		}

		if (opacity_map != hg::InvalidTextureRef) {
			hg::debug(hg::format("    - uOpacityMap: %1").arg(resources.textures.GetName(opacity_map)));
			mat.textures["uOpacityMap"] = {opacity_map, 5};
			SetMaterialBlendMode(mat, hg::BM_Alpha);
		}

		if (ambient_map != hg::InvalidTextureRef) {
			hg::debug(hg::format("    - uAmbientMap: %1").arg(resources.textures.GetName(ambient_map)));
			mat.textures["uAmbientMap"] = {ambient_map, 6};
		}

		if (reflection_map != hg::InvalidTextureRef) {
			hg::debug(hg::format("    - uReflectionMap: %1").arg(resources.textures.GetName(reflection_map)));
			mat.textures["uReflectionMap"] = {reflection_map, 7};
		}

		if (shininess_map != hg::InvalidTextureRef) { // UNMAPPED
			hg::debug(hg::format("    - uShininessMap: %1 (IGNORED, use alpha channel of diffuse map instead)").arg(resources.textures.GetName(shininess_map)));
			// mat.textures.push_back({"uShininessMap", shininess_map, 7});
		}
		if (bump_map != hg::InvalidTextureRef) { // UNMAPPED
			hg::debug(hg::format("    - uBumpMap: %1 (IGNORED, use normal map instead").arg(resources.textures.GetName(bump_map)));
			// mat.textures.push_back({"uBumpMap", bump_map, 9});
		}
	}

	if (!config.shader.empty())
		shader = config.shader; // use override

	hg::debug(hg::format("    - Using pipeline shader '%1'").arg(shader));
	mat.program = resources.programs.Add(shader.c_str(), {});

	// FinalizeMaterial(mat, ai_material->GetName(), geo_name);

	return mat;
}

static void AppendGeometry(const aiScene *ai_scene, hg::Geometry &geo, aiMesh *ai_mesh, std::map<const aiMaterial *, int> &material_indices) {
	aiMatrix4x4 mesh_matrix; //, mesh_rmatrix;

	auto mat = ai_scene->mMaterials[ai_mesh->mMaterialIndex];
	int mat_idx = 0;
	auto it = material_indices.find(mat);
	if (it == material_indices.end()) {
		mat_idx = (int)material_indices.size();
		material_indices[mat] = mat_idx;
	} else {
		mat_idx = it->second;
	}

	auto start_vtx = geo.vtx.size();
	auto start_faces = geo.pol.size();

	geo.vtx.resize(geo.vtx.size() + ai_mesh->mNumVertices);
	for (size_t n = start_vtx; n < geo.vtx.size(); ++n) {
		const aiVector3D v = mesh_matrix * ai_mesh->mVertices[n - start_vtx];
		geo.vtx[n] = {float(v[0]), float(v[1]), float(v[2])};
	}
	geo.pol.resize(geo.pol.size() + ai_mesh->mNumFaces);
	for (size_t n = start_faces; n < geo.pol.size(); ++n) {
		geo.pol[n].vtx_count = uint8_t(ai_mesh->mFaces[n - start_faces].mNumIndices);
		geo.pol[n].material = mat_idx;
	}

	auto start_binding = geo.binding.size();
	const auto pol_index = hg::ComputePolygonIndex(geo);
	geo.binding.resize(hg::ComputeBindingCount(geo));

	for (size_t p = start_faces; p < geo.pol.size(); ++p)
		for (auto v = 0; v < geo.pol[p].vtx_count; ++v) {
			auto v_idx = ai_mesh->mFaces[p - start_faces].mIndices[v];
			geo.binding[size_t(pol_index[p]) + v] = v_idx + (unsigned int)start_vtx;
		}

	// normal
	if (ai_mesh->HasNormals()) {
		hg::debug("    - Has normal layer");

		geo.normal.resize(geo.binding.size());

		for (size_t p = start_faces; p < geo.pol.size(); ++p)
			for (size_t v = 0; v < geo.pol[p].vtx_count; ++v) {
				auto v_idx = ai_mesh->mFaces[p - start_faces].mIndices[v];
				auto N = ai_mesh->mNormals[v_idx];
				geo.normal[size_t(pol_index[p]) + v] = {N.x, N.y, N.z};
			}
	}

	// tangent and bi-normal
	if (ai_mesh->HasTangentsAndBitangents()) {
		hg::debug("    - Has tangent and binormal layer");

		geo.tangent.resize(geo.binding.size());
		for (size_t p = start_faces; p < geo.pol.size(); ++p)
			for (size_t v = 0; v < geo.pol[p].vtx_count; ++v) {
				auto v_idx = ai_mesh->mFaces[p - start_faces].mIndices[v];

				auto T = ai_mesh->mTangents[v_idx];
				auto B = ai_mesh->mBitangents[v_idx];

				geo.tangent[size_t(pol_index[p]) + v].T = {float(T[0]), float(T[1]), float(T[2])};
				geo.tangent[size_t(pol_index[p]) + v].B = {float(B[0]), float(B[1]), float(B[2])};
			}
	}

	if (ai_mesh->GetNumColorChannels() > 1) {
		hg::warn(hg::format("Only one color layer is supported"));
	}

	if (ai_mesh->GetNumColorChannels() > 0) {
		hg::debug("    - Has color layer");

		geo.color.resize(geo.binding.size());

		auto colors = ai_mesh->mColors[0];
		for (size_t p = start_faces; p < geo.pol.size(); ++p)
			for (size_t v = 0; v < geo.pol[p].vtx_count; ++v) {
				auto v_idx = ai_mesh->mFaces[p - start_faces].mIndices[v];
				auto cl = colors[v_idx];
				geo.color[size_t(pol_index[p]) + v] = {cl.r, cl.g, cl.b, cl.a};
			}
	}

	// UV channel
	const auto layer_count_to_export = hg::Min<size_t>(ai_mesh->GetNumUVChannels(), geo.uv.size());

	for (size_t l = 0; l < layer_count_to_export; ++l) {
		auto &uv = geo.uv[l];

		hg::debug(hg::format("    - Has UV%1").arg(l));

		const auto uv_layer = ai_mesh->mTextureCoords[l];

		uv.resize(geo.binding.size());

		auto colors = ai_mesh->mColors[0];
		for (size_t p = start_faces; p < geo.pol.size(); ++p)
			for (size_t v = 0; v < geo.pol[p].vtx_count; ++v) {
				auto v_idx = ai_mesh->mFaces[p - start_faces].mIndices[v];
				auto UV = uv_layer[v_idx];
				uv[pol_index[p] + v] = {float(UV[0]), float(UV[1])};
			}
	}
}

static hg::ModelRef ExportGeometry(const aiScene *ai_scene, hg::Node &node, const aiNode *ai_node, hg::Object &object, hg::Scene &scene, ExportMap &export_map,
	const Config &config, hg::PipelineResources &resources) {

	__ASSERT__(ai_node->mNumMeshes > 0);

	hg::Geometry geo;

	std::string mesh_name =
		ai_node->mName.C_Str(); // can't really use aiMesh::mName as there can be more than 1 aiMesh per node and they split meshes into n aiMesh by materials

	if (mesh_name.empty()) {
		int unnamed_obj_idx = 0;
		std::string obj_name = "Object_" + std::to_string(unnamed_obj_idx);
		while (export_map.exported_object_names.find(obj_name) != export_map.exported_object_names.end()) {
			unnamed_obj_idx++;
			obj_name = "Object_" + std::to_string(unnamed_obj_idx);
		}
		mesh_name = obj_name;
	}

	export_map.exported_object_names.insert(mesh_name);

	hg::debug(hg::format("Exporting geometry '%1'").arg(mesh_name));

	std::map<const aiMaterial *, int> material_indices;

	unsigned int total_vertices = 0;
	unsigned int total_faces = 0;
	for (unsigned int i = 0; i < ai_node->mNumMeshes; i++) {
		auto ai_mesh = ai_scene->mMeshes[ai_node->mMeshes[i]];
		total_vertices += ai_mesh->mNumVertices;
		total_faces += ai_mesh->mNumFaces;
	}
	geo.vtx.reserve(total_vertices);
	geo.pol.reserve(total_faces);

	// build bone list
	std::map<const aiBone *, unsigned int> bone_indices;
	for (unsigned int i = 0; i < ai_node->mNumMeshes; i++) {
		auto ai_mesh = ai_scene->mMeshes[ai_node->mMeshes[i]];
		for (unsigned int b = 0; b < ai_mesh->mNumBones; b++) {
			const auto bone = ai_mesh->mBones[b];
			auto it = bone_indices.find(bone);
			if (it == bone_indices.end()) {
				auto idx = (unsigned int)bone_indices.size();
				bone_indices[bone] = idx;
			}
		}
	}

	const bool use_skin = !bone_indices.empty();

	if (!bone_indices.empty()) {

		const auto bone_count = bone_indices.size();
		geo.bind_pose.resize(bone_count);

		for (auto it = bone_indices.begin(); it != bone_indices.end(); it++) {
			const auto bone = it->first;
			const auto bone_idx = it->second;

			const hg::Mat4 offset_matrix = AIMatrixToMatrix4(bone->mOffsetMatrix);
			geo.bind_pose[bone_idx] = offset_matrix;
		}
	}

	for (unsigned int i = 0; i < ai_node->mNumMeshes; i++) {
		auto ai_mesh = ai_scene->mMeshes[ai_node->mMeshes[i]];
		AppendGeometry(ai_scene, geo, ai_mesh, material_indices);

		if (use_skin)
			AppendGeometrySkin(ai_scene, geo, ai_mesh, bone_indices);
	}

	object.SetBoneCount(bone_indices.size());
	for (auto it = bone_indices.begin(); it != bone_indices.end(); it++) {
		const auto bone = it->first;
		const auto bone_idx = it->second;

		// check bone->mNode is an actual node
		// An invalid node should not happen, but happens with doberman_animation.FBX
		// note: ideally, assimp should initialize this to nullptr but it doesn't
		// let's check against our list of nodes
		bool is_valid_node = false;

		const aiNode *ai_bone_node = bone->mNode;

		for (auto some_node : export_map.all_nodes) {
			if (some_node == bone->mNode) {
				is_valid_node = true;
				break;
			}
		}

		if (!is_valid_node) { // should not happen, but happens with doberman_animation.FBX
			// search the scene for a node with the same name
			ai_bone_node = nullptr;

			for (auto some_node : export_map.all_nodes) {
				if (some_node->mName == bone->mName) {
					ai_bone_node = some_node;
					break;
				}
			}
		}

		if (ai_bone_node == nullptr) { // should not happen, but happens with doberman_animation.FBX

		} else {
			auto bone_node = ExportNode(ai_scene, ai_bone_node, scene, export_map, config, resources);
			if (bone_node.IsValid()) {
				object.SetBone(bone_idx, bone_node.ref);
			} else {
			}
		}
	}

	const auto vtx_to_pol = hg::ComputeVertexToPolygon(geo);
	const auto vtx_normal = hg::ComputeVertexNormal(geo, vtx_to_pol, hg::Deg(45.f));

	// recalculate normals
	bool recalculate_normal = config.recalculate_normal;
	if (geo.normal.empty() || config.calculate_normal_if_missing)
		recalculate_normal = true;

	if (recalculate_normal) {
		hg::debug("    - Recalculate normals");
		geo.normal = vtx_normal;
	}

	// recalculate tangent frame
	bool recalculate_tangent = config.recalculate_tangent;
	if (geo.tangent.empty() || config.calculate_normal_if_missing)
		recalculate_tangent = true;

	if (recalculate_tangent) {
		hg::debug("    - Recalculate tangent frames (MikkT)");
		if (!geo.uv[0].empty())
			geo.tangent = hg::ComputeVertexTangent(geo, vtx_normal, 0, hg::Deg(45.f));
	}

	object.SetMaterialCount(material_indices.size());
	for (auto it : material_indices) {
		auto ai_mat = it.first;
		auto mat_idx = it.second;

		auto mat = ExportMaterial(ai_scene, ai_mat, use_skin, export_map, config, resources);
		object.SetMaterial(mat_idx, std::move(mat));
		object.SetMaterialName(mat_idx, std::string(ai_mat->GetName().C_Str()));
	}

	//
	// FinalizeGeometry(geo, name);

	std::string path = mesh_name;

	if (GetOutputPath(path, config.base_output_path, mesh_name, {}, "geo", config.import_policy_geometry)) {
		hg::debug(hg::format("    - Saving to '%1'").arg(path));
		hg::SaveGeometryToFile(path.c_str(), geo);
	}

	path = MakeRelativeResourceName(path, config.prj_path, config.prefix);

	return resources.models.Add(path.c_str(), {}); // note: no need to perform any conversion, use a mock model
}

//
static hg::Object ExportObject(const aiScene *ai_scene, hg::Node &node, const aiNode *ai_node, hg::Scene &scene, ExportMap &export_map, const Config &config,
	hg::PipelineResources &resources) {

	__ASSERT__(ai_node->mNumMeshes > 0);
	__ASSERT__(!node.GetObject().IsValid());

	auto object = scene.CreateObject();
	node.SetObject(object);

	auto mdl = ExportGeometry(ai_scene, node, ai_node, object, scene, export_map, config, resources);
	object.SetModelRef(mdl);
	return object;
}

static void ExportCamera(
	const aiScene *ai_scene, const aiCamera *ai_camera, hg::Scene &scene, ExportMap &export_map, const Config &config, hg::PipelineResources &resources) {
	// find corresponding node or create one if missing
	auto camera = scene.CreateCamera();

	const aiNode *camera_ai_node = nullptr;
	for (auto it : export_map.exported_nodes) {
		auto ai_node = it.first;
		if (ai_node->mName == ai_camera->mName) {
			camera_ai_node = ai_node;
			break;
		}
	}

	hg::Node camera_node;
	if (camera_ai_node == nullptr) {
		camera_node = scene.CreateNode(ai_camera->mName.C_Str());
		auto transform = scene.CreateTransform();
		camera_node.SetTransform(transform);
	} else {
		camera_node = export_map.exported_nodes[camera_ai_node];
	}

	camera.SetZNear(ai_camera->mClipPlaneNear);
	camera.SetZFar(ai_camera->mClipPlaneFar);

	// compute vertical fov from horizontal fov
	// auto fovy = ai_camera->mHorizontalFOV / ai_camera->mAspect;
	auto aspect = ai_camera->mAspect;
	if (aspect <= 0.0f) // == 0 on test asset node_parent_light.gltf
		aspect = 1.0f;
	auto fov = ai_camera->mHorizontalFOV;
	fov = 2.f * atan(tan(fov * 0.5f) / aspect); // copied from the fbx converter code
	camera.SetFov(fov);

	if (ai_camera->mOrthographicWidth > 0) {
		camera.SetIsOrthographic(true);
		camera.SetSize(aspect / ai_camera->mOrthographicWidth);
	}

	auto transform = camera_node.GetTransform();

	hg::Vec3 pos, rot, scl;
	auto mat = camera_ai_node != nullptr ? AIMatrixToMatrix4(camera_ai_node->mTransformation) : hg::Mat4::Identity;

	// not sure why the lookat vector changes between fbx and gltf
	// we hardcode these to make the test assets pass but some of the importers are buggy
	if (ai_camera->mLookAt.x == 1.0f) {
		// seem to be the case for fbx files
		// see // node_parent_light.fbx
		auto rot90 = (hg::Mat4)hg::RotationMatY(hg::DegreeToRadian(90.0f));
		mat = mat * rot90;
	} else {
		// see node_parent_light.gltf
		hg::Mat4 lookat_mat = hg::Mat4LookTowardUp(hg::Vec3(0), hg::Vec3(-ai_camera->mLookAt.x, -ai_camera->mLookAt.y, -ai_camera->mLookAt.z),
			hg::Vec3(ai_camera->mUp.x, ai_camera->mUp.y, ai_camera->mUp.z));
		mat = mat * lookat_mat;
	}

	Decompose(mat, &pos, &rot, &scl);
	transform.SetTRS({pos, rot, scl});

	// do we have a target?
	const aiNode *camera_ai_target_node = nullptr;
	const std::string target_name = ai_camera->mName.C_Str() + std::string(".Target");

	if (camera_ai_node != nullptr) {
		// search the parent node
		auto parent = camera_ai_node->mParent;
		for (size_t i = 0; i < parent->mNumChildren; i++) {
			if (target_name == parent->mChildren[i]->mName.C_Str()) {
				camera_ai_target_node = parent->mChildren[i];
				break;
			}
		}
	}
	if (camera_ai_target_node != nullptr) {
		aiVector3D target_scale, target_rot, target_pos;
		camera_ai_target_node->mTransformation.Decompose(target_scale, target_rot, target_pos);
		hg::Vec3 at(target_pos.x - pos.x, target_pos.y - pos.y, target_pos.z - pos.z);
		mat = hg::Mat4LookTowardUp(pos, at, hg::Vec3(ai_camera->mUp.x, ai_camera->mUp.y, ai_camera->mUp.z));
		Decompose(mat, &pos, &rot, &scl);
		transform.SetTRS({pos, rot, scl});
	} else {
		// according to the documentation, ai_camera->mPosition is supposed to be added to the node pos
		// for node_parent_light.gltf it appears that it contains the world position
		// so just fallback to the node position, hoping it works for most cases.
		// if not, either make a fix for assimp gltf, or a patch for this specific format

		// transform.SetPos(transform.GetPos() + hg::Vec3(ai_camera->mPosition.x, ai_camera->mPosition.y, ai_camera->mPosition.z));
	}

	transform.SetScale(hg::Vec3(1, 1, 1));
	camera_node.SetCamera(camera);
	camera_node.SetTransform(transform);
}

static void ExportLight(
	const aiScene *ai_scene, const aiLight *ai_light, hg::Scene &scene, ExportMap &export_map, const Config &config, hg::PipelineResources &resources) {

	// find corresponding node or create one if missing
	auto light = scene.CreateLight(); // TODO: handle different types of lights, like linear lights

	const aiNode *light_ai_node = nullptr;
	for (auto it : export_map.exported_nodes) {
		auto ai_node = it.first;
		if (ai_node->mName == ai_light->mName) {
			light_ai_node = ai_node;
			break;
		}
	}

	hg::Node light_node;
	if (light_ai_node == nullptr) {
		light_node = scene.CreateNode(ai_light->mName.C_Str());
		auto transform = scene.CreateTransform();
		light_node.SetTransform(transform);
	} else {
		light_node = export_map.exported_nodes[light_ai_node];
	}

	switch (ai_light->mType) {
		case aiLightSource_DIRECTIONAL: {
			light.SetType(hg::LT_Linear);
			break;
		}
		case aiLightSource_POINT: {
			light.SetType(hg::LT_Point);
			break;
		}
		case aiLightSource_SPOT: {
			light.SetType(hg::LT_Spot);
			break;
		}
		case aiLightSource_AMBIENT: {
			hg::warn(hg::format("Unsupported light type : aiLightSource_AMBIENT"));
			break;
		}
		case aiLightSource_AREA: {
			hg::warn(hg::format("Unsupported light type : aiLightSource_AREA"));
			break;
		}
	}
	float intensity_factor = 0.2f; // of course all this is wrong but it's to get something good enough

	light.SetDiffuseColor(hg::Color(ai_light->mColorDiffuse.r, ai_light->mColorDiffuse.g, ai_light->mColorDiffuse.b));
	light.SetDiffuseIntensity(intensity_factor);
	light.SetSpecularColor(hg::Color(ai_light->mColorSpecular.r, ai_light->mColorSpecular.g, ai_light->mColorSpecular.b));
	light.SetSpecularIntensity(intensity_factor);

	if (ai_light->mAttenuationLinear > 0) {
		light.SetRadius(1.0f / ai_light->mAttenuationLinear);
		float intensity = ai_light->mAttenuationLinear * intensity_factor;
		light.SetDiffuseColor(hg::Color(ai_light->mColorDiffuse.r, ai_light->mColorDiffuse.g, ai_light->mColorDiffuse.b));
		light.SetDiffuseIntensity(intensity);
		light.SetSpecularColor(hg::Color(ai_light->mColorSpecular.r, ai_light->mColorSpecular.g, ai_light->mColorSpecular.b));
		light.SetSpecularIntensity(intensity);
	}
	if (ai_light->mAttenuationQuadratic > 0) {

		float intensity_factor = 0.1f; // of course all this is wrong but it's to get something good enough

		light.SetRadius(1.0f / sqrtf(ai_light->mAttenuationQuadratic));
		float intensity = sqrtf(ai_light->mAttenuationQuadratic) * intensity_factor;
		light.SetDiffuseColor(hg::Color(ai_light->mColorDiffuse.r, ai_light->mColorDiffuse.g, ai_light->mColorDiffuse.b));
		light.SetDiffuseIntensity(intensity);
		light.SetSpecularColor(hg::Color(ai_light->mColorSpecular.r, ai_light->mColorSpecular.g, ai_light->mColorSpecular.b));
		light.SetSpecularIntensity(intensity);
	}

	if (ai_light->mAngleInnerCone > 0)
		light.SetInnerAngle(ai_light->mAngleInnerCone / 2.f);
	if (ai_light->mAngleOuterCone > 0)
		light.SetOuterAngle(ai_light->mAngleOuterCone / 2.f);

	auto transform = light_node.GetTransform();

	// for now we assume the light direction is fixed given its node transform
	__ASSERT__(ai_light->mUp.z == -1.0f);
	__ASSERT__(ai_light->mDirection.y == -1.0f);

	// HG lights look at z, so add a 90deg rotation here
	hg::Vec3 pos, rot, scl;
	auto mat = light_ai_node != nullptr ? AIMatrixToMatrix4(light_ai_node->mTransformation) : hg::Mat4::Identity;
	auto rot90 = (hg::Mat4)hg::RotationMatX(hg::DegreeToRadian(90.0f));
	mat = mat * rot90;
	Decompose(mat, &pos, &rot, &scl);
	transform.SetTRS({pos, rot, scl});
	transform.SetPos(transform.GetPos() + hg::Vec3(ai_light->mPosition.x, ai_light->mPosition.y, ai_light->mPosition.z));
	transform.SetScale(hg::Vec3(1, 1, 1));
	light_node.SetLight(light);
	light_node.SetTransform(transform);
}

//
static hg::Node ExportNode(
	const aiScene *ai_scene, const aiNode *ai_node, hg::Scene &scene, ExportMap &export_map, const Config &config, hg::PipelineResources &resources) {
	auto i = export_map.exported_nodes.find(ai_node);
	if (i != std::end(export_map.exported_nodes))
		return i->second;

	hg::Node node;
	if (ai_node != ai_scene->mRootNode || !ai_node->mTransformation.IsIdentity() || ai_node->mNumMeshes > 0) {

		std::string node_name = ai_node->mName.C_Str();
		if (node_name.empty()) {
			node_name = "Node";
		}

		node = scene.CreateNode(node_name);

		auto transform = scene.CreateTransform();

		hg::Vec3 pos, rot, scl;
		Decompose(AIMatrixToMatrix4(ai_node->mTransformation), &pos, &rot, &scl);
		transform.SetTRS({pos, rot, scl});
		node.SetTransform(transform);

		if (ai_node->mNumMeshes > 0) {
			ExportObject(ai_scene, node, ai_node, scene, export_map, config, resources);
		}
	}

	export_map.exported_nodes[ai_node] = node;

	for (unsigned int i = 0; i < ai_node->mNumChildren; ++i) {
		auto child = ExportNode(ai_scene, ai_node->mChildren[i], scene, export_map, config, resources);
		if (child && node)
			child.GetTransform().SetParent(node.ref);
	}

	return node;
}

//
static const aiScene *LoadAssimpScene(Assimp::Importer *importer, const Config &config, const std::string &ai_path) {

	importer->SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);
	importer->SetPropertyBool(AI_CONFIG_IMPORT_COLLADA_IGNORE_UP_DIRECTION, true);

	if (!config.merge_meshes)
		importer->SetPropertyBool(AI_CONFIG_PP_PTV_KEEP_HIERARCHY, true);

	// importer->SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false); // see
	// https://gamedev.stackexchange.com/questions/175044/assimp-skeletal-animation-with-some-fbx-files-has-issues-weird-node-added

	importer->SetPropertyFloat(AI_CONFIG_GLOBAL_SCALE_FACTOR_KEY, config.scene_scale);

	auto scene = importer->ReadFile(
		hg::ansi_to_utf8(ai_path), aiProcess_PopulateArmatureData | aiProcess_ConvertToLeftHanded | aiProcess_CalcTangentSpace | aiProcess_GlobalScale |
									   aiProcess_FindInstances | aiProcess_OptimizeMeshes | aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality |
									   aiProcess_SortByPType | (config.merge_meshes ? aiProcess_PreTransformVertices : 0)
		// aiProcess_Triangulate
	);

	if (scene == nullptr) {
		auto str = importer->GetErrorString();
		if (str != nullptr) {
			hg::error(hg::format("Error while loading file '%1' :").arg(ai_path.c_str()));
			hg::error(hg::format("%1").arg(str));
		}
	}

	return scene;
}

static std::vector<const aiNode *> ListAllNodes(const aiScene *ai_scene) {

	std::vector<const aiNode *> all_nodes;

	const std::function<void(aiNode const *node)> traverse_nodes = [&traverse_nodes, &all_nodes](aiNode const *node) {
		all_nodes.push_back(node);
		for (size_t i = 0; i < node->mNumChildren; i++) {
			traverse_nodes(node->mChildren[i]);
		}
	};
	traverse_nodes(ai_scene->mRootNode);

	return all_nodes;
}

static bool ImportAssimpScene(const std::string &path, const Config &config) {

	const auto t_start = hg::time_now();

	// create output directory if missing
	if (hg::Exists(config.base_output_path.c_str())) {
		if (!hg::IsDir(config.base_output_path.c_str()))
			return false; // can't output to this path
	} else {
		if (!hg::MkDir(config.base_output_path.c_str()))
			return false;
	}

	if (config.base_output_path.empty())
		return false;

	if (!config.finalizer_script.empty())
		if (!LoadFinalizerScript(config.finalizer_script))
			return false;

	Assimp::Importer importer;

	auto ai_scene = LoadAssimpScene(&importer, config, path);
	if (ai_scene == nullptr || ai_scene->mRootNode == nullptr)
		return false;

	ExportMap export_map;

	// make a list of all nodes
	export_map.all_nodes = ListAllNodes(ai_scene);

	bool has_assimpfbx_nodes = false;
	for (auto node : export_map.all_nodes) {
		if (strstr(node->mName.C_Str(), "$AssimpFbx$") != nullptr) {
			has_assimpfbx_nodes = true;
			break;
		}
	}

	if (has_assimpfbx_nodes) {
		Assimp::FbxOptimizeGraphProcess fbx_optimizegraph;
		ai_scene = importer.ApplyCustomizedPostProcessing(&fbx_optimizegraph, false);

		// relist the nodes
		export_map.all_nodes = ListAllNodes(ai_scene);
	}

	if (config.merge_meshes) {
		ai_scene = importer.ApplyPostProcessing(aiProcess_OptimizeGraph); // can't combine it with aiProcess_PreTransformVertices so we use a 2nd pass
	}

	// FBX/gltf seem to be in ms. // see https://github.com/assimp/assimp/issues/3462
	export_map.fps_workaround =
		hg::ends_with(path, ".fbx", hg::insensitive) || hg::ends_with(path, ".gltf", hg::insensitive) || hg::ends_with(path, ".glb", hg::insensitive);

	hg::Scene scene;
	hg::PipelineResources resources;

	ExportNode(ai_scene, ai_scene->mRootNode, scene, export_map, config, resources);

	// export cameras
	for (unsigned int i = 0; i < ai_scene->mNumCameras; i++) {
		auto ai_camera = ai_scene->mCameras[i];
		ExportCamera(ai_scene, ai_camera, scene, export_map, config, resources);
	}

	// export lights
	for (unsigned int i = 0; i < ai_scene->mNumLights; i++) {
		auto ai_light = ai_scene->mLights[i];
		ExportLight(ai_scene, ai_light, scene, export_map, config, resources);
	}

	ExportMotions(ai_scene, scene, export_map, config, resources);

	FinalizeScene(scene);

	// add default pbr map
	scene.environment.brdf_map = resources.textures.Add("core/pbr/brdf.dds", {BGFX_SAMPLER_NONE, BGFX_INVALID_HANDLE});
	scene.environment.probe = {};
	scene.environment.probe.irradiance_map = resources.textures.Add("core/pbr/probe.hdr.irradiance", {BGFX_SAMPLER_NONE, BGFX_INVALID_HANDLE});
	scene.environment.probe.radiance_map = resources.textures.Add("core/pbr/probe.hdr.radiance", {BGFX_SAMPLER_NONE, BGFX_INVALID_HANDLE});

	std::string out_path;
	if (GetOutputPath(out_path, config.base_output_path, config.name.empty() ? hg::GetFileName(path) : config.name, {}, "scn", config.import_policy_scene))
		SaveSceneJsonToFile(out_path.c_str(), scene, resources);

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
	std::cout << "Usage: ai_converter " << hg::word_wrap(hg::FormatCmdLineArgs(cmd_format), 80, 21) << std::endl << std::endl;
	std::cout << hg::FormatCmdLineArgsDescription(cmd_format);
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

	std::cout << hg::format("Assimp Converter %1 (%2)").arg(hg::get_version_string()).arg(hg::get_build_sha()).str() << std::endl;

	hg::CmdLineFormat cmd_format = {
		{
			{"-recalculate-normal", "Recreate the vertex normals of exported geometries"},
			{"-recalculate-tangent", "Recreate the vertex tangent frames of exported geometries"},
			{"-calculate-normal-if-missing", "Compute missing vertex normals"},
			{"-calculate-tangent-if-missing", "Compute missing vertex tangents"},
			//{"-detect-geometry-instances", "Detect and optimize geometry instances"},
			{"-import-animation", "Detect and optimize geometry instances"},
			//{"-anim-to-file", "Scene animations will be exported to separate files and not embedded in scene", true}, // not supported for now
			{"-merge-meshes", "Merge meshes if possible"},
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
			{"-scene-scale", "Factor used to scale the scene", true},
			{"-max-smoothing-angle", "Maximum smoothing angle between two faces when computing vertex normals", true},
			{"-anim-simplify-translation-tolerance", "Tolerance on translation animations [default=0.001]", true},
			{"-anim-simplify-rotation-tolerance", "Tolerance on rotation animations[default=0.1]", true},
			{"-anim-simplify-scale-tolerance", "Tolerance on scale animations[default=0.001]", true},
			{"-finalizer-script", "Path to the Lua finalizer script", true},
			{"-profile", "Material conversion profile (default or pbr_default or pbr_physical) [default=default]", true},
			{"-shader", "Material pipeline shader [default=core/shader/<profile>.hps]", true},
		},
		{
			{"input", "Input file to convert"},
		},
		{
			{"-o", "-out"},
			{"-h", "-help"},
			{"-q", "-quiet"},
			{"-p", "-profile"},
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

	config.scene_scale = hg::GetCmdLineSingleValue(cmd_content, "-scene-scale", 1.f);

	config.recalculate_normal = hg::GetCmdLineFlagValue(cmd_content, "-recalculate-normal");
	config.recalculate_tangent = hg::GetCmdLineFlagValue(cmd_content, "-recalculate-tangent");
	config.calculate_normal_if_missing = hg::GetCmdLineFlagValue(cmd_content, "-calculate-normal-if-missing");
	config.calculate_tangent_if_missing = hg::GetCmdLineFlagValue(cmd_content, "-calculate-tangent-if-missing");

	config.max_smoothing_angle = hg::GetCmdLineSingleValue(cmd_content, "-max-smoothing-angle", 0.7f);

	// config.detect_geometry_instances = hg::GetCmdLineFlagValue(cmd_content, "-detect-geometry-instances");
	config.merge_meshes = hg::GetCmdLineFlagValue(cmd_content, "-merge-meshes");

	config.finalizer_script = hg::GetCmdLineSingleValue(cmd_content, "-finalizer-script", "");

	config.profile = hg::GetCmdLineSingleValue(cmd_content, "-profile", "default");
	config.shader = hg::GetCmdLineSingleValue(cmd_content, "-shader", "");

	config.import_animation = hg::GetCmdLineFlagValue(cmd_content, "-import-animation");
	config.anim_simplify_translation_tolerance =
		hg::GetCmdLineSingleValue(cmd_content, "-anim-simplify-translation-tolerance", config.anim_simplify_translation_tolerance);
	config.anim_simplify_rotation_tolerance =
		hg::GetCmdLineSingleValue(cmd_content, "-anim-simplify-rotation-tolerance", config.anim_simplify_rotation_tolerance);
	config.anim_simplify_scale_tolerance = hg::GetCmdLineSingleValue(cmd_content, "-anim-simplify-scale-tolerance", config.anim_simplify_scale_tolerance);

	quiet = hg::GetCmdLineFlagValue(cmd_content, "-quiet");

	//
	if (cmd_content.positionals.size() != 1) {
		std::cout << "No input file" << std::endl;
		OutputUsage(cmd_format);
		return -2;
	}

	config.input_file = cmd_content.positionals[0];

	const auto res = ImportAssimpScene(cmd_content.positionals[0], config);

	const auto msg = std::string("[ImportScene") + std::string(res ? ": OK]" : ": KO]");
	hg::log(msg.c_str());

	return res ? 0 : 1;
}
