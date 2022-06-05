// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#define FBXSDK_NEW_API
#include <fbxsdk.h>

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

/*
	FBX matrix order: C = A * B => C = B then A
	The FBX SDK documentation says the opposite and is wrong, demonstration:

	FbxAMatrix _A(FbxVector4(0, 0, 0), FbxVector4(0, 0, 0), FbxVector4(2, 1, 1));
	FbxAMatrix _B(FbxVector4(0, 0, 0), FbxVector4(0, 0, 90), FbxVector4(1, 1, 1));

	auto _C = _A * _B; // B then A
	auto _D = _B * _A; // A then B
*/

enum class ImportPolicy { SkipExisting, Overwrite, Rename, SkipAlways };

struct Config {
	ImportPolicy import_policy_geometry{ImportPolicy::SkipExisting}, import_policy_material{ImportPolicy::SkipExisting},
		import_policy_texture{ImportPolicy::SkipExisting}, import_policy_scene{ImportPolicy::SkipExisting}, import_policy_anim{ImportPolicy::SkipExisting};

	std::string name; // output name (may be empty)
	std::string base_output_path{"./"};
	std::string prj_path;
	std::string prefix;
	std::string profile, shader;

	float scale{1.f};
	float geometry_scale{1.f};

	bool import_animation{true};
	int frames_per_second{24};
	float anim_simplify_translation_tolerance = 0.001f;
	float anim_simplify_rotation_tolerance = 0.05f; // in degrees
	float anim_simplify_scale_tolerance = 0.001f;
	float anim_simplify_color_tolerance = 0.001f;

	bool fix_geo_orientation{false}; // FBX fix
	float max_smoothing_angle{45.f};
	bool recalculate_normal{false}, recalculate_tangent{false};
	bool calculate_normal_if_missing{false}, calculate_tangent_if_missing{false};
	bool detect_geometry_instances{false};
	bool anim_to_file{false};

	std::string finalizer_script;
};

// static hg::LuaVM vm;

// [EJ] Y-Up is handled by the FBX SDK, RHS->LHS is handled by these matrices
FbxAMatrix scn_export_mtx(FbxVector4(0, 0, 0), FbxVector4(0, 0, 0), FbxVector4(1, 1, -1)); // convert scene from RHS to LHS
FbxAMatrix msh_export_mtx(FbxVector4(0, 0, 0), FbxVector4(-90, 0, 0), FbxVector4(1, -1, 1));
FbxAMatrix node_export_mtx(FbxVector4(0, 0, 0), FbxVector4(-90, 0, 0), FbxVector4(1, -1, 1));

static void SetMeshExportMatrix(bool fix, float scale) {
	node_export_mtx = FbxAMatrix(FbxVector4(0, 0, 0), FbxVector4(0, 0, 0), FbxVector4(1, 1, -1) * scale);
	if (fix)
		msh_export_mtx = FbxAMatrix(FbxVector4(0, 0, 0), FbxVector4(-90, 0, 0), FbxVector4(1, -1, 1) * scale);
	else
		msh_export_mtx = node_export_mtx;
}

static FbxAMatrix ConvertGlobalMatrix(const FbxAMatrix &m) { return scn_export_mtx * m * msh_export_mtx.Inverse(); }

static hg::Mat4 FBXMatrixToMatrix4(const FbxAMatrix &fbx_m) {
	hg::Mat4 m;
	for (int i = 0; i < 3; ++i)
		for (int j = 0; j < 4; ++j)
			m.m[i][j] = float(fbx_m[j][i]);
	return m;
}

static bool GetOutputPath(
	std::string &path, const std::string &base, const std::string &name, const std::string &prefix, const std::string &ext, ImportPolicy import_policy) {
	if (base.empty())
		return false;

	const auto filename = name.empty() ? prefix : (prefix.empty() ? name : prefix + "-" + name);
	path = hg::CleanPath(base + "/" + hg::CleanFileName(filename) + "." + ext);

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

//
static void ExportMotions(
	FbxScene *fbx_scene, std::map<FbxNode *, hg::Node> &exported_nodes, hg::Scene &scene, const Config &config, hg::PipelineResources &resources) {
	if (!config.import_animation)
		return;

	for (int n = 0; n < fbx_scene->GetSrcObjectCount<FbxAnimStack>(); ++n) {
		// switch to FBX animation stack
		FbxAnimStack *anim_stack = FbxCast<FbxAnimStack>(fbx_scene->GetSrcObject<FbxAnimStack>(n));
		fbx_scene->SetCurrentAnimationStack(anim_stack);
		auto anim_eval = fbx_scene->GetAnimationEvaluator();

		// determine motion take range
		FbxTime tStart = anim_stack->LocalStart.Get(), tEnd = anim_stack->LocalStop.Get(), tStep;
		tStep.SetSecondDouble(1.0 / double(config.frames_per_second));

		// create Harfang take
		hg::SceneAnim scene_anim;

		scene_anim.name = anim_stack->GetNameOnly();

		const bool force_start_at_0 = true;

		if (force_start_at_0) {
			scene_anim.t_start = 0;
			scene_anim.t_end = hg::time_from_ms((tEnd - tStart).GetMilliSeconds());
		} else {
			scene_anim.t_start = hg::time_from_ms(tStart.GetMilliSeconds());
			scene_anim.t_end = hg::time_from_ms(tEnd.GetMilliSeconds());
		}

		// for each exported node, bake motion
		hg::Vec3 p, s;
		hg::Mat3 r;

		for (auto i : exported_nodes) {
			if (!i.second)
				continue;

			// create animation default tracks
			auto pos_track = hg::AnimTrackHermiteT<hg::Vec3>(); // or AnimKeyHermiteT?
			pos_track.target = "Position";
			// auto rot_track = std::make_shared<hg::AnimTrackRotation>("Transform.Rotation");
			auto rot_track = hg::AnimTrackT<hg::Quaternion>();
			rot_track.target = "Rotation";
			auto scl_track = hg::AnimTrackHermiteT<hg::Vec3>();
			scl_track.target = "Scale";

			auto light = i.first->GetLight();
			auto diffuse_track = hg::AnimTrackHermiteT<hg::Color>();
			diffuse_track.target = "Light.Diffuse";
			auto specular_track = hg::AnimTrackHermiteT<hg::Color>();
			specular_track.target = "Light.Specular";

			auto diffuse_intensity_track = hg::AnimTrackHermiteT<float>();
			diffuse_intensity_track.target = "Light.DiffuseIntensity";
			auto specular_intensity_track = hg::AnimTrackHermiteT<float>();
			specular_intensity_track.target = "Light.SpecularIntensity";

			// bake the animation track
			for (FbxTime t = tStart; t < tEnd + tStep; t += tStep) {
				FbxAMatrix m;
				FbxAMatrix node_global_transform = i.first->EvaluateGlobalTransform(t);

				if (i.first->GetParent()) {
					FbxAMatrix parent_global_transform = i.first->GetParent()->EvaluateGlobalTransform(t);
					m = ConvertGlobalMatrix(parent_global_transform).Inverse() * ConvertGlobalMatrix(node_global_transform);
				} else {
					m = ConvertGlobalMatrix(node_global_transform);
				}

				Decompose(FBXMatrixToMatrix4(m), &p, &r, &s);

				// output transformation to animation tracks
				hg::time_ns hg_t = force_start_at_0 ? hg::time_from_ms((t - tStart).GetMilliSeconds()) : hg::time_from_ms(t.GetMilliSeconds());

				hg::AnimKeyHermiteT<hg::Vec3> pos_key;
				pos_key.v = p;
				pos_key.t = hg_t;
				pos_track.keys.push_back(pos_key);

				hg::AnimKeyT<hg::Quaternion> rot_key;
				rot_key.v = QuaternionFromMatrix3(r);
				rot_key.t = hg_t;
				rot_track.keys.push_back(rot_key);

				hg::AnimKeyHermiteT<hg::Vec3> scl_key;
				scl_key.v = s;
				scl_key.t = hg_t;
				scl_track.keys.push_back(scl_key);

				if (light != nullptr) {
					auto light_color = light->Color.EvaluateValue(t);

					// float hsv[3];
					// float rgb[3] = {light_color[0], light_color[1], light_color[2]};
					// bx::rgbToHsv(hsv, rgb);

					// auto light_intensity = light->Intensity.EvaluateValue(t);
					// hsv[2] *= light_intensity / 100.0f; // make this an importer option ?

					// bx::hsvToRgb(rgb, hsv);
					// light_color[0] = rgb[0];
					// light_color[1] = rgb[1];
					// light_color[2] = rgb[2];

					hg::AnimKeyHermiteT<hg::Color> diffuse_key;
					diffuse_key.v.r = light_color[0];
					diffuse_key.v.g = light_color[1];
					diffuse_key.v.b = light_color[2];
					diffuse_key.v.a = 1.0f;
					diffuse_key.t = hg_t;
					diffuse_track.keys.push_back(diffuse_key);

					hg::AnimKeyHermiteT<hg::Color> specular_key;
					specular_key.v = diffuse_key.v;
					specular_key.t = hg_t;
					specular_track.keys.push_back(specular_key);

					auto light_intensity = light->Intensity.EvaluateValue(t);

					hg::AnimKeyHermiteT<float> diffuse_intensity_key;
					diffuse_intensity_key.v = light_intensity;
					diffuse_intensity_key.t = hg_t;
					diffuse_intensity_track.keys.push_back(diffuse_intensity_key);

					hg::AnimKeyHermiteT<float> specular_intensity_key;
					specular_intensity_key.v = light_intensity;
					specular_intensity_key.t = hg_t;
					specular_intensity_track.keys.push_back(specular_intensity_key);
				}
			}

			hg::ConformAnimTrackKeys(rot_track);

			// cleanup tracks
			float simplify_translation_tolerance = config.anim_simplify_translation_tolerance;
			float simplify_rotation_tolerance = config.anim_simplify_rotation_tolerance * hg::Pi / 180.0f;
			float simplify_scale_tolerance = config.anim_simplify_scale_tolerance;
			float simplify_color_tolerance = config.anim_simplify_color_tolerance;

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
			if (light != nullptr && simplify_color_tolerance > 0) {
				auto removed0 = hg::SimplifyAnimTrackT<hg::AnimTrackHermiteT<hg::Color>, hg::Color>(diffuse_track, simplify_color_tolerance);
				auto removed1 = hg::SimplifyAnimTrackT<hg::AnimTrackHermiteT<hg::Color>, hg::Color>(specular_track, simplify_color_tolerance);
				hg::debug(hg::format("Clean light tracks: %1").arg(removed0 + removed1));
			}

			// create node animation
			auto anim = hg::Anim();

			anim.vec3_tracks.push_back(std::move(pos_track));
			anim.quat_tracks.push_back(std::move(rot_track));
			anim.vec3_tracks.push_back(std::move(scl_track));

			if (light != nullptr) {
				anim.color_tracks.push_back(std::move(diffuse_track));
				anim.color_tracks.push_back(std::move(specular_track));
				anim.float_tracks.push_back(std::move(diffuse_intensity_track));
				anim.float_tracks.push_back(std::move(specular_intensity_track));
			}

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

	// TODO
	// if (config.anim_to_file) {
	//	std::string anim_output_path;

	//	if (GetOutputPath(anim_output_path, config.base_output_path, "all", {}, "anm", config.import_policy_anim)) {
	//		std::unique_ptr<DocumentWriter> doc(NewResourceDocumentWriter(anim_output_path));
	//		if (hg::SerializeAnimationTakes(*doc, scene->anim_takes, nullptr))
	//			doc->Save(anim_output_path);
	//	}

	//	scene->anim_takes.clear();
	//}
}

//----
static void ExportGeometrySkin(FbxSkin *fbx_skin, hg::Geometry &geo) {
	geo.skin.resize(geo.vtx.size());
	for (size_t n = 0; n < geo.vtx.size(); ++n)
		for (int j = 0; j < 4; ++j) {
			geo.skin[n].index[j] = 0;
			geo.skin[n].weight[j] = 0;
		}

	// for each skin entry select the clusters with the largest weight
	const auto bone_count = fbx_skin->GetClusterCount();
	geo.bind_pose.resize(bone_count);

	for (int n = 0; n < bone_count; ++n) {
		const auto cluster = fbx_skin->GetCluster(n);

		// import bind pose
		FbxAMatrix cluster_matrix, bind_matrix;
		cluster->GetTransformMatrix(cluster_matrix);
		cluster->GetTransformLinkMatrix(bind_matrix);

		geo.bind_pose[n] = FBXMatrixToMatrix4((ConvertGlobalMatrix(cluster_matrix).Inverse() * ConvertGlobalMatrix(bind_matrix)).Inverse());

		// import weights
		const auto fbx_index = cluster->GetControlPointIndices();
		const auto fbx_weight = cluster->GetControlPointWeights();

		for (int i = 0; i < cluster->GetControlPointIndicesCount(); ++i) {
			auto skin = &geo.skin[fbx_index[i]];

			// perform insertion
			uint8_t weight = hg::pack_float<uint8_t>(float(fbx_weight[i]));

			for (int c = 0; c < 4; ++c)
				if (weight > skin->weight[c]) {
					// shift the lower influences out
					for (int j = 4 - 1; j > c; --j) {
						skin->index[j] = skin->index[j - 1];
						skin->weight[j] = skin->weight[j - 1];
					}

					// insert new influence
					skin->index[c] = hg::numeric_cast<uint16_t>(n);
					skin->weight[c] = weight;
					break;
				}
		}
	}
}

//
static hg::TextureRef ExportTexture(FbxFileTexture *fbx_texture, const Config &config, hg::PipelineResources &resources) {
	std::string src_path = fbx_texture->GetFileName();

	if (!hg::Exists(src_path.c_str())) {
		src_path = hg::CutFilePath(src_path);

		if (!hg::Exists(src_path.c_str())) {
			hg::error(hg::format("Missing texture file '%1'").arg(src_path));
			return {};
		}
	}

	uint32_t flags = BGFX_SAMPLER_NONE;

	switch (fbx_texture->GetWrapModeU()) {
		case FbxTexture::eRepeat:
			break; // default
		case FbxTexture::eClamp:
			flags |= BGFX_SAMPLER_U_CLAMP;
			break;
	}

	switch (fbx_texture->GetWrapModeV()) {
		case FbxTexture::eRepeat:
			break; // default
		case FbxTexture::eClamp:
			flags |= BGFX_SAMPLER_V_CLAMP;
			break;
	}

	std::string dst_path;
	if (GetOutputPath(dst_path, config.base_output_path, hg::GetFileName(src_path), {}, hg::GetFileExtension(src_path), config.import_policy_texture))
		if (!hg::CopyFile(src_path.c_str(), dst_path.c_str())) {
			hg::error(hg::format("Failed to copy texture file '%1' to '%2'").arg(src_path).arg(dst_path));
			return {};
		}

	dst_path = MakeRelativeResourceName(dst_path, config.prj_path, config.prefix);

	return resources.textures.Add(dst_path.c_str(), {flags, BGFX_INVALID_HANDLE});
}

//
enum Property {
	Diffuse,
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
	LastProperty
};

static const char *fbx_properties[LastProperty] = {FbxSurfaceMaterial::sDiffuse, FbxSurfaceMaterial::sDiffuseFactor, FbxSurfaceMaterial::sEmissive,
	FbxSurfaceMaterial::sAmbient, FbxSurfaceMaterial::sSpecular, FbxSurfaceMaterial::sNormalMap, FbxSurfaceMaterial::sShininess, FbxSurfaceMaterial::sBump,
	FbxSurfaceMaterial::sTransparentColor, FbxSurfaceMaterial::sReflection, "roughness_map", "metalness_map", "base_color_map", "bump_map", "reflectivity_map",
	"emission_map", "emit_color_map", "refl_color_map"};

//
static void _ExportLayeredTexture(
	FbxLayeredTexture *object, Property property, std::array<hg::TextureRef, LastProperty> &texture, const Config &config, hg::PipelineResources &resources) {
	int texture_count = object->GetSrcObjectCount<FbxTexture>();

	if (property == Diffuse) {
		if (const auto tex = object->GetSrcObject<FbxFileTexture>(0))
			texture[property] = ExportTexture(tex, config, resources);

		if (const auto tex = object->GetSrcObject<FbxFileTexture>(1)) {
			FbxLayeredTexture::EBlendMode blend_mode;
			if (object->GetTextureBlendMode(1, blend_mode))
				if (blend_mode == FbxLayeredTexture::eModulate)
					texture[Lightmap] = ExportTexture(tex, config, resources); // second multiplicative layer over diffuse is considered light map
		}
	} else {
		if (const auto tex = object->GetSrcObject<FbxFileTexture>(0))
			texture[property] = ExportTexture(tex, config, resources);
	}
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

static FbxProperty FindObjectProperty(FbxObject *fbx_object, const char *name) {
	auto prop = fbx_object->GetFirstProperty();
	while (true) {
		if (prop.GetName() == name)
			return prop;

		prop = fbx_object->GetNextProperty(prop);
		if (!prop.IsValid())
			return FbxProperty();
	}
}

//
static hg::Material ExportMaterial(FbxSurfaceMaterial *fbx_material, FbxMesh *fbx_mesh, bool use_skin, const Config &config, hg::PipelineResources &resources) {
	hg::Color diffuse = {0.5f, 0.5f, 0.5f, 1.f}, emissive = {0, 0, 0, 1}, specular = {0.5f, 0.5f, 0.5f, 1.f}, ambient = {0, 0, 0, 1};

	float glossiness{1.f};
	float reflection{1.f};

	hg::debug(hg::format("Exporting material '%1'").arg(fbx_material->GetName()));

	auto class_id = fbx_material->GetClassId();

	if (fbx_material->GetClassId().Is(FbxSurfacePhong::ClassId)) {
		hg::debug("    - Has Phong");

		const auto fbx_phong = (FbxSurfacePhong *)fbx_material;
		specular = {float(fbx_phong->Specular.Get()[0] * fbx_phong->SpecularFactor.Get()),
			float(fbx_phong->Specular.Get()[1] * fbx_phong->SpecularFactor.Get()), float(fbx_phong->Specular.Get()[2] * fbx_phong->SpecularFactor.Get())};
		glossiness = hg::Clamp(float(fbx_phong->Shininess.Get()) / 64.f, 0.01f, 0.5f); // completely random conversion factor
		reflection = static_cast<float>(fbx_phong->ReflectionFactor);
	}

	if (fbx_material->GetClassId().Is(FbxSurfacePhong::ClassId) || fbx_material->GetClassId().Is(FbxSurfaceLambert::ClassId)) {
		hg::debug("    - Has Lambert");

		const auto fbx_lambert = (FbxSurfaceLambert *)fbx_material;
		ambient = {float(fbx_lambert->Ambient.Get()[0] * fbx_lambert->AmbientFactor.Get()),
			float(fbx_lambert->Ambient.Get()[1] * fbx_lambert->AmbientFactor.Get()), float(fbx_lambert->Ambient.Get()[2] * fbx_lambert->AmbientFactor.Get())};
		diffuse = {float(fbx_lambert->Diffuse.Get()[0] * fbx_lambert->DiffuseFactor.Get()),
			float(fbx_lambert->Diffuse.Get()[1] * fbx_lambert->DiffuseFactor.Get()), float(fbx_lambert->Diffuse.Get()[2] * fbx_lambert->DiffuseFactor.Get())};

		const hg::Color emissive_color = {float(fbx_lambert->Emissive.Get()[0]), float(fbx_lambert->Emissive.Get()[1]), float(fbx_lambert->Emissive.Get()[2])};
		const auto emissive_factor = float(fbx_lambert->EmissiveFactor.Get());
		emissive = emissive_color * emissive_factor;

		diffuse.a = 1.f - static_cast<float>(fbx_lambert->TransparencyFactor.Get());
	}

	std::array<hg::TextureRef, LastProperty> texture;

	for (auto n = 0; n < LastProperty; ++n) {
		if (fbx_properties[n] == nullptr)
			continue;

		const auto fbx_texture_prop = FindObjectProperty(fbx_material, fbx_properties[n]);
		if (const auto fbx_texture = fbx_texture_prop.GetSrcObject<FbxTexture>(0)) {
			if (const auto tex = fbx_texture_prop.GetSrcObject<FbxFileTexture>(0)) {
				// check if it's a color correction and get the map (for roughnness, metalness, ao)
				const auto tex_prop = FindObjectProperty(tex, "map");
				if (const auto sub_tex = tex_prop.GetSrcObject<FbxFileTexture>(0)) {
					texture[n] = ExportTexture(sub_tex, config, resources);
				} else {
					const auto tex1_prop = FindObjectProperty(tex, "map1");
					if (const auto sub1_tex = tex1_prop.GetSrcObject<FbxFileTexture>(0)) {
						texture[n] = ExportTexture(sub1_tex, config, resources);
					} else {
						texture[n] = ExportTexture(tex, config, resources);
					}
				}
			}
			if (const auto tex = fbx_texture_prop.GetSrcObject<FbxLayeredTexture>(0))
				_ExportLayeredTexture(tex, Property(n), texture, config, resources);
		}
	}

	//
	hg::Material mat;

	if (use_skin)
		mat.flags |= hg::MF_EnableSkinning;

	const bool has_diffuse_map = texture[Diffuse] != hg::InvalidTextureRef || texture[BaseColorMap] != hg::InvalidTextureRef;
	const bool has_specular_map = texture[Specular] != hg::InvalidTextureRef || texture[ReflColorMap] != hg::InvalidTextureRef;
	const bool has_normal_map = texture[Normal] != hg::InvalidTextureRef;
	const bool has_light_map = texture[Lightmap] != hg::InvalidTextureRef;
	const bool has_self_map =
		texture[Emissive] != hg::InvalidTextureRef || texture[EmissionMap] != hg::InvalidTextureRef || texture[EmitColorMap] != hg::InvalidTextureRef;
	const bool has_opacity_map = texture[TransparentColor] != hg::InvalidTextureRef;
	const bool has_shininess_map = texture[Shininess] != hg::InvalidTextureRef;
	const bool has_reflection_map = texture[Reflection] != hg::InvalidTextureRef;
	const bool has_ambient_map = texture[Ambient] != hg::InvalidTextureRef;
	const bool has_bump_map = texture[Bump] != hg::InvalidTextureRef || texture[BumpMap] != hg::InvalidTextureRef;
	const bool has_roughness_map = texture[RoughnessMap] != hg::InvalidTextureRef;
	const bool has_metalness_map = texture[MetalnessMap] != hg::InvalidTextureRef;

	std::string shader;

	if (config.profile == "pbr_default") {
		shader = "core/shader/pbr.hps";

		const auto occlusion = 1.f;
		const auto roughness = 0.5f;
		const auto metalness = 0.25f;

		mat.values["uBaseOpacityColor"] = {bgfx::UniformType::Vec4, {diffuse.r, diffuse.g, diffuse.b, diffuse.a}};
		mat.values["uOcclusionRoughnessMetalnessColor"] = {bgfx::UniformType::Vec4, {occlusion, roughness, metalness, 0.f}};
		mat.values["uSelfColor"] = {bgfx::UniformType::Vec4, {emissive.r, emissive.g, emissive.b, -1.f}};

		if (has_diffuse_map) {
			hg::debug(hg::format("    - uBaseOpacityMap: %1").arg(resources.textures.GetName(texture[Diffuse])));
			mat.textures["uBaseOpacityMap"] = {texture[Diffuse], 0};
		} else if (diffuse.a < 1.f) {
			SetMaterialBlendMode(mat, hg::BM_Alpha);
		}

		if (has_specular_map) {
			hg::debug(hg::format("    - uOcclusionRoughnessMetalnessMap: %1").arg(resources.textures.GetName(texture[Specular])));
			mat.textures["uOcclusionRoughnessMetalnessMap"] = {texture[Specular], 1};
		}

		if (has_normal_map) {
			hg::debug(hg::format("    - uNormalMap: %1").arg(resources.textures.GetName(texture[Normal])));
			mat.textures["uNormalMap"] = {texture[Normal], 2};
		}

		if (has_self_map) {
			hg::debug(hg::format("    - uSelfMap: %1").arg(resources.textures.GetName(texture[Emissive])));
			mat.textures["uSelfMap"] = {texture[Emissive], 4};
		}
	} else if (config.profile == "pbr_physical") {
		shader = "core/shader/pbr.hps";

		// get the values from the properties
		const auto base_color_prop = FindObjectProperty(fbx_material, "base_color");
		if (base_color_prop.IsValid())
			diffuse = {float(base_color_prop.Get<FbxColor>().mRed), float(base_color_prop.Get<FbxColor>().mGreen), float(base_color_prop.Get<FbxColor>().mBlue),
				float(base_color_prop.Get<FbxColor>().mAlpha)};

		const auto emit_color_prop = FindObjectProperty(fbx_material, "emit_color");
		const auto emission_prop = FindObjectProperty(fbx_material, "emission");
		if (emit_color_prop.IsValid() && emission_prop.IsValid()) {
			auto emission_factor = float(emission_prop.Get<FbxDouble>());
			emissive = {float(emit_color_prop.Get<FbxColor>().mRed) * emission_factor, float(emit_color_prop.Get<FbxColor>().mGreen) * emission_factor,
				float(emit_color_prop.Get<FbxColor>().mBlue) * emission_factor, -1.f};
		}

		// set occlusion to 1
		const auto occlusion = 1.f;

		const auto roughness_prop = FindObjectProperty(fbx_material, "roughness");
		const auto roughness = roughness_prop.IsValid() ? float(roughness_prop.Get<FbxDouble>()) : 0.5f;

		const auto metalness_prop = FindObjectProperty(fbx_material, "metalness");
		const auto metalness = metalness_prop.IsValid() ? float(metalness_prop.Get<FbxDouble>()) : 0.25f;

		mat.values["uBaseOpacityColor"] = {bgfx::UniformType::Vec4, {diffuse.r, diffuse.g, diffuse.b, diffuse.a}};
		mat.values["uOcclusionRoughnessMetalnessColor"] = {bgfx::UniformType::Vec4, {occlusion, roughness, metalness, 0.f}};
		mat.values["uSelfColor"] = {bgfx::UniformType::Vec4, {emissive.r, emissive.g, emissive.b, -1.f}};

		if (has_diffuse_map) {
			hg::debug(hg::format("    - uBaseOpacityMap: %1").arg(resources.textures.GetName(texture[BaseColorMap])));
			mat.textures["uBaseOpacityMap"] = {texture[BaseColorMap], 0};
		} else if (diffuse.a < 1.f) {
			SetMaterialBlendMode(mat, hg::BM_Alpha);
		}
		if (has_opacity_map)
			SetMaterialBlendMode(mat, hg::BM_Alpha);
		else {
			// check if there is alpha in the base color map
			hg::Picture pic;
			if (LoadPicture(pic, (config.prj_path + "/" + resources.textures.GetName(texture[BaseColorMap])).c_str()) && PictureHasTransparency(pic))
				SetMaterialBlendMode(mat, hg::BM_Alpha);
		}

		if (has_roughness_map || has_metalness_map) {
			hg::debug(hg::format("    - uOcclusionRoughnessMetalnessMap: %1").arg(resources.textures.GetName(texture[RoughnessMap])));
			mat.textures["uOcclusionRoughnessMetalnessMap"] = {texture[RoughnessMap], 1};
		}

		if (has_bump_map) {
			hg::debug(hg::format("    - uNormalMap: %1").arg(resources.textures.GetName(texture[BumpMap])));
			mat.textures["uNormalMap"] = {texture[BumpMap], 2};
		}

		if (has_self_map) {
			if (texture[EmissionMap] != hg::InvalidTextureRef) {
				hg::debug(hg::format("    - uSelfMap: %1").arg(resources.textures.GetName(texture[EmissionMap])));
				mat.textures["uSelfMap"] = {texture[EmissionMap], 4};
			} else {
				hg::debug(hg::format("    - uSelfMap: %1").arg(resources.textures.GetName(texture[EmitColorMap])));
				mat.textures["uSelfMap"] = {texture[EmitColorMap], 4};
			}
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

		if (has_diffuse_map) {
			hg::debug(hg::format("    - uDiffuseMap: %1").arg(resources.textures.GetName(texture[Diffuse])));
			mat.textures["uDiffuseMap"] = {texture[Diffuse], 0};
		} else if (diffuse.a < 1.f) {
			SetMaterialBlendMode(mat, hg::BM_Alpha);
		}

		if (has_specular_map) {
			hg::debug(hg::format("    - uSpecularMap: %1").arg(resources.textures.GetName(texture[Specular])));
			mat.textures["uSpecularMap"] = {texture[Specular], 1};
		}

		if (has_normal_map) {
			hg::debug(hg::format("    - uNormalMap: %1").arg(resources.textures.GetName(texture[Normal])));
			mat.textures["uNormalMap"] = {texture[Normal], 2};
		}

		if (has_light_map) {
			hg::debug(hg::format("    - uLightMap: %1").arg(resources.textures.GetName(texture[Lightmap])));
			mat.textures["uLightMap"] = {texture[Lightmap], 3};
		}

		if (has_self_map) {
			hg::debug(hg::format("    - uSelfMap: %1").arg(resources.textures.GetName(texture[Emissive])));
			mat.textures["uSelfMap"] = {texture[Emissive], 4};
		}

		if (has_opacity_map) {
			hg::debug(hg::format("    - uOpacityMap: %1").arg(resources.textures.GetName(texture[TransparentColor])));
			mat.textures["uOpacityMap"] = {texture[TransparentColor], 5};
			SetMaterialBlendMode(mat, hg::BM_Alpha);
		}

		if (has_ambient_map) {
			hg::debug(hg::format("    - uAmbientMap: %1").arg(resources.textures.GetName(texture[Ambient])));
			mat.textures["uAmbientMap"] = {texture[Ambient], 6};
		}

		if (has_reflection_map) {
			hg::debug(hg::format("    - uReflectionMap: %1").arg(resources.textures.GetName(texture[Reflection])));
			mat.textures["uReflectionMap"] = {texture[Reflection], 7};
		}

		if (has_shininess_map) { // UNMAPPED
			hg::debug(
				hg::format("    - uShininessMap: %1 (IGNORED, use alpha channel of diffuse map instead)").arg(resources.textures.GetName(texture[Shininess])));
			// mat.textures.push_back({"uShininessMap", texture[Shininess], 7});
		}
		if (has_bump_map) { // UNMAPPED
			hg::debug(hg::format("    - uBumpMap: %1 (IGNORED, use normal map instead").arg(resources.textures.GetName(texture[Bump])));
			// mat.textures.push_back({"uBumpMap", texture[Bump], 9});
		}
	}

	if (!config.shader.empty())
		shader = config.shader; // use override

	hg::debug(hg::format("    - Using pipeline shader '%1'").arg(shader));
	mat.program = resources.programs.Add(shader.c_str(), {});

	// FinalizeMaterial(mat, fbx_material->GetName(), geo_name);
	return mat;
}

#define __PolIndex (pol_index[p] + v)
#define __PolRemapIndex (pol_index[p] + (geo.pol[p].vtx_count - 1 - v))

hg::ModelRef DoExportGeometry(FbxMesh *fbx_mesh, FbxNode *pNode, hg::Object &object, FbxAMatrix mesh_matrix, FbxAMatrix mesh_rmatrix, const Config &config,
	hg::PipelineResources &resources) {
	hg::Geometry geo;

	hg::debug(hg::format("Exporting geometry '%1'").arg(pNode->GetName()));

	// transfer topology
	geo.vtx.resize(fbx_mesh->GetControlPointsCount());
	for (size_t n = 0; n < geo.vtx.size(); ++n) {
		const FbxVector4 v = mesh_matrix.MultT(fbx_mesh->GetControlPoints()[n]);
		geo.vtx[n] = {float(v[0]), float(v[1]), float(v[2])};
	}

	geo.pol.resize(fbx_mesh->GetPolygonCount());
	for (size_t n = 0; n < geo.pol.size(); ++n) {
		geo.pol[n].vtx_count = uint8_t(fbx_mesh->GetPolygonSize(n));
		geo.pol[n].material = 0;
	}

	const auto pol_index = hg::ComputePolygonIndex(geo);
	geo.binding.resize(hg::ComputeBindingCount(geo));

	for (size_t p = 0; p < geo.pol.size(); ++p)
		for (auto v = 0; v < geo.pol[p].vtx_count; ++v)
			geo.binding[pol_index[p] + v] = fbx_mesh->GetPolygonVertices()[__PolRemapIndex];

	// export materials
	const auto fbx_layer = fbx_mesh->GetLayer(0);

	// normal
	if (const auto normal_layer = fbx_layer->GetNormals()) {
		hg::debug("    - Has normal layer");

		geo.normal.resize(geo.binding.size());
		for (size_t p = 0; p < geo.pol.size(); ++p)
			for (auto v = 0; v < geo.pol[p].vtx_count; ++v) {
				FbxVector4 N;
				fbx_mesh->GetPolygonVertexNormal(p, v, N);
				N = mesh_rmatrix.MultT(N);
				N.Normalize();
				geo.normal[__PolRemapIndex] = {float(N[0]), float(N[1]), float(N[2])};
			}
	}

	// tangent and bi-normal
	const auto tangent_layer = fbx_layer->GetTangents();
	const auto binormal_layer = fbx_layer->GetBinormals();

	if (tangent_layer && binormal_layer) {
		hg::debug("    - Has tangent and binormal layer");

		if (tangent_layer->GetMappingMode() == FbxLayerElement::eByPolygonVertex && binormal_layer->GetMappingMode() == FbxLayerElement::eByPolygonVertex) {
			geo.tangent.resize(geo.binding.size());
			for (size_t p = 0; p < geo.pol.size(); ++p)
				for (auto v = 0; v < geo.pol[p].vtx_count; ++v) {
					auto T = tangent_layer->GetReferenceMode() == FbxLayerElement::eIndexToDirect
								 ? tangent_layer->GetDirectArray()[tangent_layer->GetIndexArray()[__PolRemapIndex]]
								 : tangent_layer->GetDirectArray()[__PolRemapIndex];
					T = mesh_rmatrix.MultT(T);
					T.Normalize();
					geo.tangent[__PolIndex].T = {float(T[0]), float(-T[1]), float(T[2])}; // this is UV dependent and textures are reversed on V

					auto B = binormal_layer->GetReferenceMode() == FbxLayerElement::eIndexToDirect
								 ? binormal_layer->GetDirectArray()[binormal_layer->GetIndexArray()[__PolRemapIndex]]
								 : binormal_layer->GetDirectArray()[__PolRemapIndex];
					B = mesh_rmatrix.MultT(T);
					B.Normalize();
					geo.tangent[__PolIndex].B = {float(B[0]), float(-B[1]), float(B[2])}; // this is UV dependent and textures are reversed on V
				}
		} else {
			hg::warn(hg::format("Unsupported tangent layer mapping mode (%1)").arg(tangent_layer->GetMappingMode()));
		}
	}

	// vertex color
	if (const auto color_layer = fbx_layer->GetVertexColors()) {
		hg::debug("    - Has color layer");

		geo.color.resize(geo.binding.size());
		switch (color_layer->GetMappingMode()) {
			case FbxLayerElement::eByControlPoint:
				for (size_t p = 0; p < geo.pol.size(); ++p)
					for (auto v = 0; v < geo.pol[p].vtx_count; ++v) {
						const auto v_idx = geo.binding[pol_index[p] + v];
						const auto &cl = color_layer->GetReferenceMode() == FbxLayerElement::eIndexToDirect
											 ? color_layer->GetDirectArray()[color_layer->GetIndexArray()[v_idx]]
											 : color_layer->GetDirectArray()[v_idx];
						geo.color[__PolIndex] = {float(cl.mRed), float(cl.mGreen), float(cl.mBlue)};
					}
				break;

			case FbxLayerElement::eByPolygonVertex:
				for (size_t p = 0; p < geo.pol.size(); ++p)
					for (auto v = 0; v < geo.pol[p].vtx_count; ++v) {
						const auto &cl = color_layer->GetReferenceMode() == FbxLayerElement::eIndexToDirect
											 ? color_layer->GetDirectArray()[color_layer->GetIndexArray()[__PolRemapIndex]]
											 : color_layer->GetDirectArray()[__PolRemapIndex];
						geo.color[__PolIndex] = {float(cl.mRed), float(cl.mGreen), float(cl.mBlue)};
					}
				break;

			default:
				hg::warn(hg::format("Unsupported vertex color layer mapping mode (%1)").arg(color_layer->GetMappingMode()));
				break;
		}
	}

	// UV channel (searched for on all available layers)
	const auto layer_count_to_export = hg::Min<size_t>(fbx_mesh->GetLayerCount(), geo.uv.size());

	for (size_t l = 0; l < layer_count_to_export; ++l) {
		auto fbx_layer = fbx_mesh->GetLayer(l);
		auto &uv = geo.uv[l];

		for (auto n = 0; n < fbx_layer->GetUVSetCount(); ++n) {
			hg::debug(hg::format("    - Has UV%1").arg(l));

			const auto uv_layer = fbx_layer->GetUVSets()[n];

			uv.resize(geo.binding.size());
			switch (uv_layer->GetMappingMode()) {
				case FbxLayerElement::eByControlPoint:
					for (size_t p = 0; p < geo.pol.size(); ++p)
						for (auto v = 0; v < geo.pol[p].vtx_count; ++v) {
							const auto v_idx = geo.binding[pol_index[p] + v];
							const auto UV = uv_layer->GetReferenceMode() == FbxLayerElement::eIndexToDirect
												? uv_layer->GetDirectArray()[uv_layer->GetIndexArray()[v_idx]]
												: uv_layer->GetDirectArray()[v_idx];
							uv[__PolIndex] = {float(UV[0]), 1.f - float(UV[1])};
						}
					break;

				case FbxLayerElement::eByPolygonVertex:
					for (size_t p = 0; p < geo.pol.size(); ++p)
						for (auto v = 0; v < geo.pol[p].vtx_count; ++v) {
							const auto &UV = uv_layer->GetReferenceMode() == FbxLayerElement::eIndexToDirect
												 ? uv_layer->GetDirectArray()[uv_layer->GetIndexArray()[__PolRemapIndex]]
												 : uv_layer->GetDirectArray()[__PolRemapIndex];
							uv[__PolIndex] = {float(UV[0]), 1.f - float(UV[1])};
						}
					break;

				default:
					hg::warn(hg::format("Unsupported UV layer mapping mode (%1)").arg(uv_layer->GetMappingMode()));
					break;
			}
		}
	}

	//
	const auto fbx_skin = static_cast<FbxSkin *>(fbx_mesh->GetDeformer(0, FbxDeformer::eSkin));
	const bool use_skin = fbx_skin != nullptr;

	if (use_skin)
		ExportGeometrySkin(fbx_skin, geo);

	float max_smoothing_angle = hg::Deg(config.max_smoothing_angle);

	const auto vtx_to_pol = hg::ComputeVertexToPolygon(geo);
	const auto vtx_normal = hg::ComputeVertexNormal(geo, vtx_to_pol, max_smoothing_angle);

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
			geo.tangent = hg::ComputeVertexTangent(geo, vtx_normal, 0, max_smoothing_angle);
	}

	// materials
	const int material_count = fbx_mesh->GetNode()->GetMaterialCount();
	if (material_count > 0) {
		hg::debug(hg::format("    - Has %1 material").arg(material_count));

		object.SetMaterialCount(material_count);

		for (int i = 0; i < material_count; ++i)
			if (auto fbx_mat = static_cast<FbxSurfaceMaterial *>(fbx_mesh->GetNode()->GetMaterial(i))) {
				auto mat = ExportMaterial(fbx_mat, fbx_mesh, use_skin, config, resources);
				object.SetMaterial(i, std::move(mat));
				object.SetMaterialName(i, std::string(fbx_mat->GetNameOnly()));
			}

		// export the material mapping to polygon
		const auto material_layer = fbx_layer->GetMaterials();

		if (material_layer)
			switch (material_layer->GetMappingMode()) {
				case FbxLayerElement::eByPolygon: {
					if (material_layer->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
						for (size_t n = 0; n < geo.pol.size(); ++n)
							geo.pol[n].material = uint8_t(material_layer->GetIndexArray().GetAt(n));
					else
						for (size_t n = 0; n < geo.pol.size(); ++n)
							geo.pol[n].material = uint8_t(n);
				} break;

				case FbxLayerElement::eAllSame: {
					if (material_layer->GetReferenceMode() == FbxLayerElement::eIndexToDirect)
						for (size_t n = 0; n < geo.pol.size(); ++n)
							geo.pol[n].material = uint8_t(material_layer->GetIndexArray().GetAt(0));
					else
						for (size_t n = 0; n < geo.pol.size(); ++n)
							geo.pol[n].material = 0;
				} break;

				default:
					hg::warn(hg::format("Unsupported material mapping mode (%1)").arg(material_layer->GetMappingMode()));
					break;
			}
	} else { // make a dummy material to see the object in the engine
		hg::debug(hg::format("    - Has no material, set a dummy one"));

		object.SetMaterialCount(1);

		hg::Material mat;
		std::string shader;

		if (config.profile == "default")
			shader = "core/shader/default.hps";
		else
			shader = "core/shader/pbr.hps";

		if (!config.shader.empty())
			shader = config.shader; // use override

		hg::debug(hg::format("    - Using pipeline shader '%1'").arg(shader));
		mat.program = resources.programs.Add(shader.c_str(), {});

		object.SetMaterial(0, std::move(mat));
		object.SetMaterialName(0, "dummy_mat");

		// connect material to polygons
		for (size_t n = 0; n < geo.pol.size(); ++n)
			geo.pol[n].material = 0;
	}

	//
	// FinalizeGeometry(geo, name);
	const std::string name = hg::format("%1_%2").arg(pNode->GetName()).arg(pNode->GetUniqueID()).str();

	auto path = name;
	if (GetOutputPath(path, config.base_output_path, name, {}, "geo", config.import_policy_geometry)) {
		hg::debug(hg::format("    - Saving to '%1'").arg(path));
		hg::SaveGeometryToFile(path.c_str(), geo);
	}

	path = MakeRelativeResourceName(path, config.prj_path, config.prefix);

	return resources.models.Add(path.c_str(), {}); // note: no need to perform any conversion, use a mock model
}

static hg::ModelRef ExportGeometry(FbxMesh *fbx_mesh, FbxNode *pNode, hg::Object &object, const Config &config, hg::PipelineResources &resources) {
	FbxAMatrix mesh_matrix, mesh_rmatrix;

	if (pNode) {
		mesh_matrix.SetTRS(pNode->GetGeometricTranslation(FbxNode::eSourcePivot), pNode->GetGeometricRotation(FbxNode::eSourcePivot),
			pNode->GetGeometricScaling(FbxNode::eSourcePivot));
		mesh_rmatrix.SetR(pNode->GetGeometricRotation(FbxNode::eSourcePivot));

		mesh_matrix = msh_export_mtx * mesh_matrix;

		FbxAMatrix msh_export_rmatrix = msh_export_mtx;
		FbxVector4 S = msh_export_mtx.GetS();
		S.Normalize();
		msh_export_rmatrix.SetS(S);
		mesh_rmatrix = msh_export_rmatrix * mesh_rmatrix;
	}

	// TODO detect instancing (using SHA1 on model)

	return DoExportGeometry(fbx_mesh, pNode, object, mesh_matrix, mesh_rmatrix, config, resources);
}

//
static hg::Node ExportObject(FbxNodeAttribute *pAttr, FbxNode *pNode, hg::Scene &scene, const Config &config, hg::PipelineResources &resources) {
	auto fbx_mesh = static_cast<FbxMesh *>(pAttr);

	auto node = scene.CreateNode(pNode->GetNameOnly().Buffer());
	node.SetTransform(scene.CreateTransform());
	auto object = scene.CreateObject();
	node.SetObject(object);

	auto mdl = ExportGeometry(fbx_mesh, pNode, object, config, resources);
	object.SetModelRef(mdl);

	return node;
}

static hg::Node ExportCamera(FbxNodeAttribute *pAttr, FbxNode *pNode, hg::Scene &scene, const Config &config, hg::PipelineResources &resources) {
	auto fbx_camera = static_cast<FbxCamera *>(pAttr);

	auto node = scene.CreateNode(pNode->GetNameOnly().Buffer());
	auto transform = scene.CreateTransform();
	auto camera = scene.CreateCamera();

	node.SetTransform(transform);
	node.SetCamera(camera);

	if (fbx_camera->GetNearPlane() != 10)
		camera.SetZNear(float(fbx_camera->GetNearPlane()));
	if (fbx_camera->GetFarPlane() != 4000)
		camera.SetZFar(float(fbx_camera->GetFarPlane()));

	auto aperture_mode = fbx_camera->ApertureMode.Get();

	auto fov = hg::DegreeToRadian(float(fbx_camera->FieldOfView.Get()));
	auto aspect = float(fbx_camera->GetApertureWidth()) / float(fbx_camera->GetApertureHeight()); // TODO use PixelAspectRatio.Get() ?

	if (aperture_mode == FbxCamera::eHorizontal) {
		fov = 2.f * atan(tan(fov * 0.5f) / aspect);
	} else if (aperture_mode == FbxCamera::eVertical) {
		// nothing to do
	} else if (aperture_mode == FbxCamera::eFocalLength) {
		fov = hg::DegreeToRadian(float(fbx_camera->ComputeFieldOfView(fbx_camera->FocalLength.Get()))); // always returns fovx
		fov = 2.f * atan(tan(fov * 0.5f) / aspect);
	} else if (aperture_mode == FbxCamera::eHorizAndVert) {
		// TODO Unhandled case atm. fovx is ignored.
		hg::warn(hg::format("Unsupported aperture mode. Fov X will be ignored."));
		fov = hg::DegreeToRadian(float(fbx_camera->FieldOfViewY.Get()));
	}

	camera.SetFov(float(fov));
	camera.SetIsOrthographic(fbx_camera->ProjectionType.Get() == FbxCamera::eOrthogonal);

	// TODO hack [EJ] Why is this a hack?
	if (!pNode->GetTarget()) {
		FbxAMatrix r;
		r.SetR(pNode->PostRotation.Get());
		pNode->PostRotation.Set(r.MultR(FbxVector4(0., 90., 0.)));
	}

	return node;
}

static hg::Node ExportLight(FbxNodeAttribute *pAttr, FbxNode *pNode, hg::Scene &scene, const Config &config, hg::PipelineResources &resources) {
	auto fbx_light = static_cast<FbxLight *>(pAttr);

	auto node = scene.CreateNode(pNode->GetNameOnly().Buffer());
	auto transform = scene.CreateTransform();
	auto light = scene.CreateLight();

	node.SetTransform(transform);
	node.SetLight(light);

	const auto type = fbx_light->LightType.Get();

	if (type == FbxLight::ePoint)
		light.SetType(hg::LT_Point);
	else if (type == FbxLight::eDirectional)
		light.SetType(hg::LT_Linear);
	else if (type == FbxLight::eSpot)
		light.SetType(hg::LT_Spot);

	auto intensity = float(fbx_light->Intensity.Get()) / 100.f;
	if (!fbx_light->CastLight.Get())
		intensity = 0.f; // force intensity to 0 on disabled lights

	light.SetDiffuseColor({float(fbx_light->Color.Get()[0]), float(fbx_light->Color.Get()[1]), float(fbx_light->Color.Get()[2])});
	light.SetDiffuseIntensity(intensity);
	light.SetSpecularColor(light.GetDiffuseColor());
	light.SetSpecularIntensity(intensity);

	if (fbx_light->EnableFarAttenuation.Get())
		light.SetRadius(float(fbx_light->FarAttenuationEnd.Get()));

	// if (fbx_light->CastShadows.Get())
	//	light->SetShadow(Light::ShadowMap);
	// light->SetShadowColor(Color(float(fbx_light->ShadowColor.Get()[0]), float(fbx_light->ShadowColor.Get()[1]), float(fbx_light->ShadowColor.Get()[2])));

	light.SetInnerAngle(float(FBXSDK_DEG_TO_RAD * fbx_light->InnerAngle / 2.));
	light.SetOuterAngle(float(FBXSDK_DEG_TO_RAD * fbx_light->OuterAngle / 2.));

	FbxAMatrix r;
	r.SetR(pNode->PostRotation.Get());
	pNode->PostRotation.Set(r.MultR(FbxVector4(90., 0., 0.)));
	return node;
}

//
static FbxAMatrix ComputeLocalMatrix(const FbxNode *pNode) {
	FbxVector4 t = pNode->LclTranslation.Get();
	FbxVector4 rOff = pNode->RotationOffset.Get();
	FbxVector4 rPiv = pNode->RotationPivot.Get();
	FbxVector4 rPre = pNode->PreRotation.Get();
	FbxVector4 r = pNode->LclRotation.Get();
	FbxVector4 rPost = pNode->PostRotation.Get();
	FbxVector4 rPivInv = -rPiv;
	FbxVector4 sOff = pNode->ScalingOffset.Get();
	FbxVector4 sPiv = pNode->ScalingPivot.Get();
	FbxVector4 s = pNode->LclScaling.Get();
	FbxVector4 sPivInv = -sPiv;

	FbxAMatrix m;
	m.SetIdentity();

	if (t.SquareLength() > 1e-6f) {
		FbxAMatrix n;
		n.SetT(t);
		m = m * n;
	}
	if (rOff.SquareLength() > 1e-6f) {
		FbxAMatrix n;
		n.SetT(rOff);
		m = m * n;
	}
	if (rPiv.SquareLength() > 1e-6f) {
		FbxAMatrix n;
		n.SetT(rPiv);
		m = m * n;
	}
	if (rPre.SquareLength() > 1e-6f) {
		FbxAMatrix n;
		n.SetR(rPre);
		m = m * n;
	}
	if (r.SquareLength() > 1e-6f) {
		FbxAMatrix n;
		n.SetR(r);
		m = m * n;
	}
	if (rPost.SquareLength() > 1e-6f) {
		FbxAMatrix n;
		n.SetR(rPost);
		m = m * n;
	}
	if (rPivInv.SquareLength() > 1e-6f) {
		FbxAMatrix n;
		n.SetT(rPivInv);
		m = m * n;
	}
	if (sOff.SquareLength() > 1e-6f) {
		FbxAMatrix n;
		n.SetT(sOff);
		m = m * n;
	}
	if (sPiv.SquareLength() > 1e-6f) {
		FbxAMatrix n;
		n.SetT(sPiv);
		m = m * n;
	}
	if (std::fabs(s.SquareLength() - 1.f)) {
		FbxAMatrix n;
		n.SetS(s);
		m = m * n;
	}
	if (sPivInv.SquareLength() > 1e-6f) {
		FbxAMatrix n;
		n.SetT(sPivInv);
		m = m * n;
	}

	return m;
}

static FbxAMatrix GetNodeLocalMatrix(FbxScene *scene, FbxNode *pNode) {
	const FbxAMatrix m = ComputeLocalMatrix(pNode);
	const FbxNode *pParent = pNode->GetParent();

	FbxAMatrix prefix;

	if (pParent == scene->GetRootNode()) {
		prefix = scn_export_mtx * ComputeLocalMatrix(scene->GetRootNode());
	} else {
		if (pParent->GetNodeAttribute() && (pParent->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh))
			prefix = msh_export_mtx;
		else
			prefix = node_export_mtx;
	}

	if (pNode->GetNodeAttribute() && (pNode->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh))
		return prefix * m * msh_export_mtx.Inverse();

	return prefix * m * node_export_mtx.Inverse();
}

//
static hg::Node ExportNode(FbxScene *fbx_scene, FbxNode *pNode, std::map<FbxNode *, hg::Node> &exported_nodes, hg::Scene &scene, const Config &config,
	hg::PipelineResources &resources) {
	auto i = exported_nodes.find(pNode);
	if (i != std::end(exported_nodes))
		return i->second;

	hg::Node node;
	if (pNode != fbx_scene->GetRootNode()) {
		if (pNode->GetNodeAttribute()) {
			FbxNodeAttribute::EType type = pNode->GetNodeAttribute()->GetAttributeType();

			switch (type) {
				default:
				case FbxNodeAttribute::eUnknown:
				case FbxNodeAttribute::eNull:
				case FbxNodeAttribute::eMarker:
				case FbxNodeAttribute::eNurbs:
				case FbxNodeAttribute::ePatch:
				case FbxNodeAttribute::eCameraStereo:
				case FbxNodeAttribute::eCameraSwitcher:
				case FbxNodeAttribute::eOpticalReference:
				case FbxNodeAttribute::eOpticalMarker:
				case FbxNodeAttribute::eNurbsCurve:
				case FbxNodeAttribute::eTrimNurbsSurface:
				case FbxNodeAttribute::eBoundary:
				case FbxNodeAttribute::eNurbsSurface:
				case FbxNodeAttribute::eShape:
				case FbxNodeAttribute::eLODGroup:
				case FbxNodeAttribute::eSubDiv:
				case FbxNodeAttribute::eSkeleton: {
					node = scene.CreateNode(pNode->GetNameOnly().Buffer());
					node.SetTransform(scene.CreateTransform());
				} break;

				case FbxNodeAttribute::eMesh:
					node = ExportObject(pNode->GetNodeAttribute(), pNode, scene, config, resources);

					// export object skin binding
					if (auto object = node.GetObject())
						if (auto fbx_mesh = static_cast<FbxMesh *>(pNode->GetNodeAttribute()))
							if (auto fbx_skin = static_cast<FbxSkin *>(fbx_mesh->GetDeformer(0, FbxDeformer::eSkin))) {
								object.SetBoneCount(fbx_skin->GetClusterCount());
								for (int i = 0; i < fbx_skin->GetClusterCount(); ++i)
									if (auto bone = ExportNode(fbx_scene, fbx_skin->GetCluster(i)->GetLink(), exported_nodes, scene, config, resources))
										object.SetBone(i, bone.ref);
							}
					break;

				case FbxNodeAttribute::eCamera:
					node = ExportCamera(pNode->GetNodeAttribute(), pNode, scene, config, resources);
					break;
				case FbxNodeAttribute::eLight:
					node = ExportLight(pNode->GetNodeAttribute(), pNode, scene, config, resources);
					break;
			}
		} else {
			node = scene.CreateNode(pNode->GetNameOnly().Buffer());
			node.SetTransform(scene.CreateTransform());
		}
	}

	// if (!pNode->Show.Get()) // TODO
	//	node->SetEnabled(false);

	exported_nodes[pNode] = node;

	if (node) {
		hg::Vec3 pos, rot, scl;
		Decompose(FBXMatrixToMatrix4(GetNodeLocalMatrix(fbx_scene, pNode)), &pos, &rot, &scl);
		node.GetTransform().SetTRS({pos, rot, scl});
		// FinalizeNode(node);
	}

	for (int i = 0; i < pNode->GetChildCount(); ++i) {
		auto child = ExportNode(fbx_scene, pNode->GetChild(i), exported_nodes, scene, config, resources);
		if (child && node)
			child.GetTransform().SetParent(node.ref);
	}

	return node;
}

//
static FbxScene *LoadFbxScene(FbxManager *manager, const Config &config, const std::string &fbx_path) {
	auto ios = FbxIOSettings::Create(manager, IOSROOT);

	ios->SetBoolProp(IMP_FBX_MATERIAL, true);
	ios->SetBoolProp(IMP_FBX_TEXTURE, true);
	ios->SetBoolProp(IMP_FBX_LINK, false);
	ios->SetBoolProp(IMP_FBX_SHAPE, false);
	ios->SetBoolProp(IMP_FBX_GOBO, false);
	ios->SetBoolProp(IMP_FBX_ANIMATION, true);
	ios->SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);

	auto fbx_scene = FbxScene::Create(manager, "");
	auto fbx_importer = FbxImporter::Create(manager, "");

	if (fbx_importer->Initialize(fbx_path.c_str(), -1, ios) && fbx_importer->Import(fbx_scene)) {
		FbxAxisSystem axis_system(FbxAxisSystem::eYAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eRightHanded);
		axis_system.ConvertScene(fbx_scene); // convert to Harfang axis system and scale (attempt to actually)

		const FbxSystemUnit::ConversionOptions options = {false, true, true, true, true, true};

		FbxSystemUnit unit_system(100.f / config.scale);
		unit_system.ConvertScene(fbx_scene, options);
	} else {
		fbx_scene->Destroy();
		fbx_scene = nullptr;
	}

	// FbxGeometryConverter converter(sdk_manager);
	// converter.Triangulate(fbx_scene, true, false);

	fbx_importer->Destroy();
	return fbx_scene;
}

static void ExportEnvironment(FbxScene *fbx_scene, hg::Scene &scene) {
	const auto &gsettings = fbx_scene->GetGlobalSettings();
	scene.environment.ambient = {(float)gsettings.GetAmbientColor().mRed, (float)gsettings.GetAmbientColor().mGreen, (float)gsettings.GetAmbientColor().mBlue};

	const auto &glsettings = fbx_scene->GlobalLightSettings();
	scene.environment.fog_color = {(float)glsettings.GetFogColor().mRed, (float)glsettings.GetFogColor().mGreen, (float)glsettings.GetFogColor().mBlue};
	if (glsettings.GetFogEnable()) {
		scene.environment.fog_near = (float)glsettings.GetFogStart();
		scene.environment.fog_far = (float)glsettings.GetFogEnd();
	}
}

static bool ImportFbxScene(const std::string &path, const Config &config) {
	const auto t_start = hg::time_now();

	// create output directory if missing
	if (hg::Exists(config.base_output_path.c_str())) {
		if (!hg::IsDir(config.base_output_path.c_str()))
			return false; // can't output to this path
	} else {
		if (!hg::MkDir(config.base_output_path.c_str()))
			return false;
	}

	//
	auto sdk_manager = FbxManager::Create();
	auto ios = FbxIOSettings::Create(sdk_manager, IOSROOT);

	FbxProperty prop;
	FbxDataType type;

	prop = ios->GetProperty(IMP_UP_AXIS);
	type = prop.GetPropertyDataType();
	hg::warn(hg::format("IMP_UP_AXIS %1 %2").arg(type.GetName()).arg(type.GetType()));

	prop = ios->GetProperty(IMP_AXISCONVERSION);
	type = prop.GetPropertyDataType();
	hg::warn(hg::format("IMP_AXISCONVERSION %1 %2").arg(type.GetName()).arg(type.GetType()));

	prop = ios->GetProperty(IMP_AUTO_AXIS);
	type = prop.GetPropertyDataType();
	hg::warn(hg::format("IMP_AUTO_AXIS %1 %2").arg(type.GetName()).arg(type.GetType()));

	if (config.base_output_path.empty())
		return false;

	if (!config.finalizer_script.empty())
		if (!LoadFinalizerScript(config.finalizer_script))
			return false;

	SetMeshExportMatrix(config.fix_geo_orientation, config.geometry_scale);

	FbxScene *fbx_scene;
	if ((fbx_scene = LoadFbxScene(sdk_manager, config, path)) == nullptr)
		return false;

	std::map<FbxNode *, hg::Node> exported_nodes;

	hg::Scene scene;
	hg::PipelineResources resources;

	ExportNode(fbx_scene, fbx_scene->GetRootNode(), exported_nodes, scene, config, resources);
	ExportMotions(fbx_scene, exported_nodes, scene, config, resources);
	ExportEnvironment(fbx_scene, scene);

	FinalizeScene(scene);

	// add default pbr map
	scene.environment.brdf_map = resources.textures.Add("core/pbr/brdf.dds", {BGFX_SAMPLER_NONE, BGFX_INVALID_HANDLE});
	scene.environment.probe = {};
	scene.environment.probe.irradiance_map = resources.textures.Add("core/pbr/probe.hdr.irradiance", {BGFX_SAMPLER_NONE, BGFX_INVALID_HANDLE});
	scene.environment.probe.radiance_map = resources.textures.Add("core/pbr/probe.hdr.radiance", {BGFX_SAMPLER_NONE, BGFX_INVALID_HANDLE});

	std::string out_path;
	if (GetOutputPath(out_path, config.base_output_path, config.name.empty() ? hg::GetFileName(path) : config.name, {}, "scn", config.import_policy_scene))
		SaveSceneJsonToFile(out_path.c_str(), scene, resources);

	fbx_scene->Destroy();
	sdk_manager->Destroy();

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
	std::cout << "Usage: fbx_converter " << hg::word_wrap(hg::FormatCmdLineArgs(cmd_format), 80, 21) << std::endl << std::endl;
	std::cout << hg::FormatCmdLineArgsDescription(cmd_format);
	std::cout
		<< std::endl
		<< hg::word_wrap(
			   "This software contains Autodesk(r) FBX(r) code developed by Autodesk, Inc. Copyright 2014 Autodesk, Inc. All rights, reserved. Such code is "
			   "provided \"as is\" and Autodesk, Inc. disclaims any and all warranties, whether express or implied, including without limitation the implied "
			   "warranties of merchantability, fitness for a particular purpose or non-infringement of third party rights. In no event shall Autodesk, Inc. be "
			   "liable for any direct, indirect, incidental, special, exemplary, or consequential damages (including, but not limited to, procurement of "
			   "substitute goods or services; loss of use, data, or profits; or business interruption) however caused and on any theory of liability, whether "
			   "in contract, strict liability, or tort (including negligence or otherwise) arising in any way out of such code.",
			   120)
		<< std::endl;
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

	std::cout << hg::format("FBX->GS Converter %1 (%2)").arg(hg::get_version_string()).arg(hg::get_build_sha()).str() << std::endl;

	hg::CmdLineFormat cmd_format = {
		{
			{"-fix-geometry-orientation", "Bake a 90° rotation on the X axis of exported geometries"},
			{"-recalculate-normal", "Recreate the vertex normals of exported geometries"},
			{"-recalculate-tangent", "Recreate the vertex tangent frames of exported geometries"},
			{"-calculate-normal-if-missing", "Compute missing vertex normals"},
			{"-calculate-tangent-if-missing", "Compute missing vertex tangents"},
			{"-detect-geometry-instances", "Detect and optimize geometry instances"},
			{"-import-animation", "Detect and optimize geometry instances"},
			//{"-anim-to-file", "Scene animations will be exported to separate files and not embedded in scene", true}, // not supported for now
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
			{"-scale", "Factor used to scale the scene nodes", true},
			{"-geometry-scale", "Factor used to scale exported geometries", true},
			{"-max-smoothing-angle", "Maximum smoothing angle between two faces when computing vertex normals", true},
			{"-frames-per-second", "Frames per second [default=24]", true},
			{"-anim-simplify-translation-tolerance", "Tolerance on translation animations [default=0.001]", true},
			{"-anim-simplify-rotation-tolerance", "Tolerance on rotation animations[default=0.1]", true},
			{"-anim-simplify-scale-tolerance", "Tolerance on scale animations[default=0.001]", true},
			{"-anim-simplify-color-tolerance", "Tolerance on color animations[default=0.001]", true},
			{"-finalizer-script", "Path to the Lua finalizer script", true},
			{"-profile", "Material conversion profile (default or pbr_default or pbr_physical) [default=default]", true},
			{"-shader", "Material pipeline shader [default=core/shader/<profile>.hps]", true},
		},
		{
			{"input", "Input FBX file to convert"},
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

	config.scale = hg::GetCmdLineSingleValue(cmd_content, "-scale", 1.f);
	config.geometry_scale = hg::GetCmdLineSingleValue(cmd_content, "-geometry-scale", 1.f);

	config.fix_geo_orientation = hg::GetCmdLineFlagValue(cmd_content, "-fix-geometry-orientation");

	config.recalculate_normal = hg::GetCmdLineFlagValue(cmd_content, "-recalculate-normal");
	config.recalculate_tangent = hg::GetCmdLineFlagValue(cmd_content, "-recalculate-tangent");
	config.calculate_normal_if_missing = hg::GetCmdLineFlagValue(cmd_content, "-calculate-normal-if-missing");
	config.calculate_tangent_if_missing = hg::GetCmdLineFlagValue(cmd_content, "-calculate-tangent-if-missing");

	config.max_smoothing_angle = hg::GetCmdLineSingleValue(cmd_content, "-max-smoothing-angle", config.max_smoothing_angle);

	config.calculate_tangent_if_missing = hg::GetCmdLineFlagValue(cmd_content, "-calculate-tangent-if-missing");
	config.detect_geometry_instances = hg::GetCmdLineFlagValue(cmd_content, "-detect-geometry-instances");

	config.finalizer_script = hg::GetCmdLineSingleValue(cmd_content, "-finalizer-script", "");

	config.profile = hg::GetCmdLineSingleValue(cmd_content, "-profile", "default");
	config.shader = hg::GetCmdLineSingleValue(cmd_content, "-shader", "");

	config.import_animation = hg::GetCmdLineFlagValue(cmd_content, "-import-animation");
	config.anim_to_file = hg::GetCmdLineFlagValue(cmd_content, "-anim-to-file");
	config.frames_per_second = hg::GetCmdLineSingleValue(cmd_content, "-frames-per-second", config.frames_per_second);
	config.anim_simplify_translation_tolerance =
		hg::GetCmdLineSingleValue(cmd_content, "-anim-simplify-translation-tolerance", config.anim_simplify_translation_tolerance);
	config.anim_simplify_rotation_tolerance =
		hg::GetCmdLineSingleValue(cmd_content, "-anim-simplify-rotation-tolerance", config.anim_simplify_rotation_tolerance);
	config.anim_simplify_scale_tolerance = hg::GetCmdLineSingleValue(cmd_content, "-anim-simplify-scale-tolerance", config.anim_simplify_scale_tolerance);
	config.anim_simplify_color_tolerance = hg::GetCmdLineSingleValue(cmd_content, "-anim-simplify-color-tolerance", config.anim_simplify_color_tolerance);

	quiet = hg::GetCmdLineFlagValue(cmd_content, "-quiet");

	//
	if (cmd_content.positionals.size() != 1) {
		std::cout << "No input file" << std::endl;
		OutputUsage(cmd_format);
		return -2;
	}

	const auto res = ImportFbxScene(cmd_content.positionals[0], config);

	const auto msg = std::string("[ImportScene") + std::string(res ? ": OK]" : ": KO]");
	hg::log(msg.c_str());

	return res ? 0 : 1;
}
