// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/file_format.h"
#include "engine/render_pipeline.h"
#include "engine/scene.h"
#include "engine/to_json.h"

#include "foundation/format.h"
#include "foundation/log.h"
#include "foundation/matrix4.h"
#include "foundation/string.h"

#include <json/json.hpp>
#include <set>

namespace hg {

// [EJ] note: SaveComponent/LoadComponent are friend of class Scene and cannot be made static (breaks clang/gcc builds)

void SaveComponent(const Scene::Transform_ *data_, json &js) {
	js["pos"] = data_->TRS.pos;
	js["rot"] = RadianToDegree(data_->TRS.rot);
	js["scl"] = data_->TRS.scl;
	js["parent"] = data_->parent;
}

void SaveComponent(const Scene::Camera_ *data_, json &js) {
	js["zrange"] = data_->zrange;
	js["fov"] = data_->fov;
	js["ortho"] = data_->ortho;
	js["size"] = data_->size;
}

void SaveComponent(const Scene::Object_ *data_, json &js, const PipelineResources &resources) {
	js["name"] = resources.models.GetName(data_->model);

	const auto mat_count = uint16_t(data_->materials.size());

	std::vector<json> materials(mat_count);
	for (size_t j = 0; j < mat_count; ++j)
		SaveMaterial(data_->materials[j], materials[j], resources);
	js["materials"] = materials;

	std::vector<json> material_infos(mat_count);
	for (size_t j = 0; j < mat_count; j++)
		if (j < data_->material_infos.size()) {
			material_infos[j]["name"] = data_->material_infos[j].name;
		} else {
			material_infos[j]["name"] = std::string();
		}
	js["material_infos"] = material_infos;

	json bones = json::array();
	for (const auto &bone : data_->bones)
		bones.push_back(bone);
	js["bones"] = bones;
}

void SaveComponent(const Scene::Light_ *data_, json &js) {
	js["type"] = data_->type;
	js["shadow_type"] = data_->shadow_type;
	js["diffuse"] = data_->diffuse;
	js["diffuse_intensity"] = data_->diffuse_intensity;
	js["specular"] = data_->specular;
	js["specular_intensity"] = data_->specular_intensity;
	js["radius"] = data_->radius;
	js["inner_angle"] = data_->inner_angle;
	js["outer_angle"] = data_->outer_angle;
	js["pssm_split"] = data_->pssm_split;
	js["priority"] = data_->priority;
	js["shadow_bias"] = data_->shadow_bias;
}

void SaveComponent(const Scene::RigidBody_ *data_, json &js) {
	js["type"] = data_->type;
	js["linear_damping"] = unpack_float(data_->linear_damping);
	js["angular_damping"] = unpack_float(data_->angular_damping);
	js["restitution"] = unpack_float(data_->restitution);
	js["friction"] = unpack_float(data_->friction);
	js["rolling_friction"] = unpack_float(data_->rolling_friction);
}

void SaveComponent(const Scene::Collision_ *data_, json &js) {
	js["type"] = data_->type;
	js["mass"] = data_->mass;
	js["path"] = data_->resource_path;
	js["pos"] = data_->trs.pos;
	js["rot"] = data_->trs.rot;
	js["scl"] = data_->trs.scl;
}

void SaveComponent(const Scene::Script_ *data_, json &js) {
	js["path"] = data_->path;

	for (const auto &i : data_->params) {
		if (i.second.type == SPT_Bool)
			js["parameters"][i.first] = {{"type", "bool"}, {"value", i.second.bv}};
		else if (i.second.type == SPT_Int)
			js["parameters"][i.first] = {{"type", "int"}, {"value", i.second.iv}};
		else if (i.second.type == SPT_Float)
			js["parameters"][i.first] = {{"type", "float"}, {"value", i.second.fv}};
		else if (i.second.type == SPT_String)
			js["parameters"][i.first] = {{"type", "string"}, {"value", i.second.sv}};
	}
}

void SaveComponent(const Scene::Instance_ *data_, json &js) {
	js["name"] = data_->name;

	if (!data_->anim.empty()) {
		js["anim"] = data_->anim;
		js["loop_mode"] = data_->loop_mode;
	}
}

static void SaveProbe(const Probe &probe, json &js, const PipelineResources &resources) {
	js["irradiance_map"] = resources.textures.GetName(probe.irradiance_map);
	js["radiance_map"] = resources.textures.GetName(probe.radiance_map);

	js["type"] = probe.type;
	js["parallax"] = unpack_float(probe.parallax);

	js["pos"] = probe.trs.pos;
	js["rot"] = probe.trs.rot;
	js["scl"] = probe.trs.scl;
}

//
void LoadComponent(Scene::Transform_ *data_, const json &js) {
	data_->TRS.pos = js.at("pos");

	data_->TRS.rot = DegreeToRadian(js.at("rot").get<Vec3>());
	data_->TRS.scl = js.at("scl");
	data_->parent = js.at("parent");
}

void LoadComponent(Scene::Camera_ *data_, const json &js) {
	js_at_safe(js, "zrange", data_->zrange);
	js_at_safe(js, "fov", data_->fov);
	js_at_safe(js, "ortho", data_->ortho);
	js_at_safe(js, "size", data_->size);
}

void LoadComponent(Scene::Object_ *data_, const json &js, const Reader &deps_ir, const ReadProvider &deps_ip, PipelineResources &resources,
	const PipelineInfo &pipeline, bool queue_model_loads, bool queue_texture_loads, bool do_not_load_resources) {
	const std::string name = js.at("name");

	if (!name.empty())
		data_->model = SkipLoadOrQueueModelLoad(deps_ir, deps_ip, name.c_str(), resources, queue_model_loads, do_not_load_resources);

	const auto &i_mats = js.find("materials");
	if (i_mats != std::end(js)) {
		const auto &mats = *i_mats;
		const auto mat_count = mats.size();
		data_->materials.resize(mat_count);
		for (size_t i = 0; i < mat_count; ++i)
			data_->materials[i] = LoadMaterial(mats[i], deps_ir, deps_ip, resources, pipeline, queue_texture_loads, do_not_load_resources);
	}

	const auto &i_mat_infos = js.find("material_infos");
	if (i_mat_infos != std::end(js)) {
		const auto &mat_infos = *i_mat_infos;
		const auto mat_count = data_->materials.size();
		data_->material_infos.resize(mat_count);
		for (size_t i = 0; i < mat_count; ++i) {
			if (i == mat_infos.size())
				break;
			mat_infos[i]["name"].get_to(data_->material_infos[i].name);
		}
	}

	const auto &i_bones = js.find("bones");
	if (i_bones != std::end(js)) {
		const auto &bones = *i_bones;
		const auto bone_count = bones.size();
		data_->bones.resize(bone_count);
		for (size_t i = 0; i < bone_count; ++i)
			data_->bones[i] = bones[i];
	}
}

void LoadComponent(Scene::Light_ *data_, const json &js) {
	data_->type = js["type"];
	if (js.find("shadow_type") != std::end(js))
		data_->shadow_type = js["shadow_type"];
	data_->diffuse = js["diffuse"];
	if (js.contains("diffuse_intensity"))
		data_->diffuse_intensity = js["diffuse_intensity"];
	data_->specular = js["specular"];
	if (js.contains("specular_intensity"))
		data_->specular_intensity = js["specular_intensity"];
	data_->radius = js["radius"];
	data_->inner_angle = js["inner_angle"];
	data_->outer_angle = js["outer_angle"];
	if (js.find("pssm_split") != std::end(js))
		data_->pssm_split = js["pssm_split"];
	data_->priority = js["priority"];
	if (js.find("shadow_bias") != std::end(js))
		data_->shadow_bias = js["shadow_bias"];
}

void LoadComponent(Scene::RigidBody_ *data_, const json &js) {
	data_->type = js.at("type");
	if (js.find("linear_damping") != std::end(js))
		data_->linear_damping = pack_float<uint8_t>(js.at("linear_damping").get<float>());
	if (js.find("angular_damping") != std::end(js))
		data_->angular_damping = pack_float<uint8_t>(js.at("angular_damping").get<float>());
	if (js.find("restitution") != std::end(js))
		data_->restitution = pack_float<uint8_t>(js.at("restitution").get<float>());
	if (js.find("friction") != std::end(js))
		data_->friction = pack_float<uint8_t>(js.at("friction").get<float>());
	if (js.find("rolling_friction") != std::end(js))
		data_->rolling_friction = pack_float<uint8_t>(js.at("rolling_friction").get<float>());
}

void LoadComponent(Scene::Collision_ *data_, const json &js) {
	data_->type = js["type"];
	data_->mass = js["mass"];
	data_->resource_path = js["path"].get<std::string>();
	data_->trs.pos = js["pos"].get<Vec3>();
	data_->trs.rot = js["rot"].get<Vec3>();
	data_->trs.scl = js["scl"].get<Vec3>();
}

void LoadComponent(Scene::Script_ *data_, const json &js) {
	data_->path = js["path"].get<std::string>();

	const auto j = js.find("parameters");

	if (j != std::end(js))
		for (auto i : j->items()) {
			auto &v = i.value();
			const auto type = v["type"].get<std::string>();

			ScriptParam parm{};

			if (type == "bool") {
				parm.type = SPT_Bool;
				parm.bv = v["value"].get<bool>();
			} else if (type == "int") {
				parm.type = SPT_Int;
				parm.iv = v["value"].get<int>();
			} else if (type == "float") {
				parm.type = SPT_Float;
				parm.fv = v["value"].get<float>();
			} else if (type == "string") {
				parm.type = SPT_String;
				parm.sv = v["value"].get<std::string>();
			}

			data_->params[i.key()] = std::move(parm);
		}
}

void LoadComponent(Scene::Instance_ *data_, const json &js) {
	js["name"].get_to(data_->name);

	if (js.find("anim") != std::end(js)) {
		js["anim"].get_to(data_->anim);
		data_->loop_mode = js["loop_mode"];
	}
}

static void LoadProbe(Probe &probe, const json &js, const Reader &deps_ir, const ReadProvider &deps_ip, PipelineResources &resources, bool queue_texture_loads,
	bool do_not_load_resources, bool silent) {
	const auto irradiance_map = js["irradiance_map"].get<std::string>();
	const auto radiance_map = js["radiance_map"].get<std::string>();

	probe.irradiance_map = SkipLoadOrQueueTextureLoad(deps_ir, deps_ip, irradiance_map.c_str(), resources, queue_texture_loads, do_not_load_resources, silent);
	probe.radiance_map = SkipLoadOrQueueTextureLoad(deps_ir, deps_ip, radiance_map.c_str(), resources, queue_texture_loads, do_not_load_resources, silent);

	probe.type = js["type"];
	probe.parallax = pack_float<uint8_t>(js["parallax"].get<float>());

	probe.trs.pos = js["pos"].get<Vec3>();
	probe.trs.rot = js["rot"].get<Vec3>();
	probe.trs.scl = js["scl"].get<Vec3>();
}

//
bool Scene::Save_json(json &js, const PipelineResources &resources, uint32_t save_flags, const std::vector<NodeRef> *nodes_to_save) const {
	// prepare list of nodes to save
	std::vector<NodeRef> node_refs;

	if (save_flags & LSSF_Nodes) {
		if (nodes_to_save) {
			node_refs = *nodes_to_save;
		} else {
			node_refs.reserve(nodes.size());
			for (auto ref = nodes.first_ref(); ref != InvalidNodeRef; ref = nodes.next_ref(ref))
				node_refs.push_back(ref);
		}
	}

	// [EJ-10082019] do not save instantiated nodes
	node_refs.erase(std::remove_if(std::begin(node_refs), std::end(node_refs),
						[&](const NodeRef &ref) {
							if (const auto *node_ = GetNode_(ref))
								if (node_->flags & NF_Instantiated)
									return true;
							return false;
						}),
		std::end(node_refs));

	//
	std::array<std::set<ComponentRef>, 5> used_component_refs;
	std::set<ComponentRef> used_script_refs, used_instance_refs, used_collision_refs;

	if (save_flags & LSSF_Nodes)
		for (const auto &ref : node_refs)
			if (const auto *node_ = GetNode_(ref)) {
				if (transforms.is_valid(node_->components[NCI_Transform]))
					used_component_refs[NCI_Transform].insert(node_->components[NCI_Transform]);
				if (cameras.is_valid(node_->components[NCI_Camera]))
					used_component_refs[NCI_Camera].insert(node_->components[NCI_Camera]);
				if (objects.is_valid(node_->components[NCI_Object]))
					used_component_refs[NCI_Object].insert(node_->components[NCI_Object]);
				if (lights.is_valid(node_->components[NCI_Light]))
					used_component_refs[NCI_Light].insert(node_->components[NCI_Light]);
				if (rigid_bodies.is_valid(node_->components[NCI_RigidBody]))
					used_component_refs[NCI_RigidBody].insert(node_->components[NCI_RigidBody]);

				{
					const auto &i = node_collisions.find(ref);
					if (i != std::end(node_collisions))
						for (const auto &col_ref : i->second)
							used_collision_refs.insert(col_ref);
				}

				{
					const auto &i = node_scripts.find(ref);
					if (i != std::end(node_scripts))
						for (const auto &script_ref : i->second)
							used_script_refs.insert(script_ref);
				}

				{
					const auto &i = node_instance.find(ref);
					if (i != std::end(node_instance))
						used_instance_refs.insert(i->second);
				}
			}

	if (save_flags & LSSF_Scene)
		for (auto &ref : scene_scripts)
			used_script_refs.insert(ref); // flag scene scripts as in-use

	//
	if (!used_component_refs[NCI_Transform].empty()) {
		auto &js_trsf = js["transforms"];
		for (const auto &ref : used_component_refs[NCI_Transform]) {
			if (transforms.is_valid(ref)) {
				json js_comp;
				SaveComponent(&transforms[ref.idx], js_comp);
				js_trsf.push_back(js_comp);
			}
		}
	}

	if (!used_component_refs[NCI_Camera].empty()) {
		auto &js_cams = js["cameras"];
		for (const auto &ref : used_component_refs[NCI_Camera]) {
			if (cameras.is_valid(ref)) {
				json js_comp;
				SaveComponent(&cameras[ref.idx], js_comp);
				js_cams.push_back(js_comp);
			}
		}
	}

	if (!used_component_refs[NCI_Object].empty()) {
		auto &js_objs = js["objects"];
		for (const auto &ref : used_component_refs[NCI_Object]) {
			if (objects.is_valid(ref)) {
				json js_comp;
				SaveComponent(&objects[ref.idx], js_comp, resources);
				js_objs.push_back(js_comp);
			}
		}
	}

	if (!used_component_refs[NCI_Light].empty()) {
		auto &js_lgts = js["lights"];
		for (const auto &ref : used_component_refs[NCI_Light]) {
			if (lights.is_valid(ref)) {
				json js_comp;
				SaveComponent(&lights[ref.idx], js_comp);
				js_lgts.push_back(js_comp);
			}
		}
	}

	if (!used_component_refs[NCI_RigidBody].empty()) {
		auto &js_bodies = js["rigid_bodies"];
		for (const auto &ref : used_component_refs[NCI_RigidBody]) {
			if (rigid_bodies.is_valid(ref)) {
				json js_comp;
				SaveComponent(&rigid_bodies[ref.idx], js_comp);
				js_bodies.push_back(js_comp);
			}
		}
	}

	if (!used_collision_refs.empty()) {
		auto &js_cols = js["collisions"];
		for (const auto &ref : used_collision_refs) {
			if (collisions.is_valid(ref)) {
				json js_comp;
				SaveComponent(&collisions[ref.idx], js_comp);
				js_cols.push_back(js_comp);
			}
		}
	}

	if (!used_script_refs.empty()) {
		auto &js_scripts = js["scripts"];
		for (const auto &ref : used_script_refs) {
			if (scripts.is_valid(ref)) {
				json js_comp;
				SaveComponent(&scripts[ref.idx], js_comp);
				js_scripts.push_back(js_comp);
			}
		}
	}

	if (!used_instance_refs.empty()) {
		auto &js_inss = js["instances"];
		for (const auto &ref : used_instance_refs) {
			if (instances.is_valid(ref)) {
				json js_comp;
				SaveComponent(&instances[ref.idx], js_comp);
				js_inss.push_back(js_comp);
			}
		}
	}

	//
	if (save_flags & LSSF_Nodes) {
		auto &js_nodes = js["nodes"];

		for (const auto ref : node_refs)
			if (const auto *node_ = GetNode_(ref)) {
				json js_node;

				js_node["idx"] = ref;
				js_node["name"] = node_->name;
				js_node["disabled"] = node_->flags & NF_Disabled ? true : false;

				std::array<uint32_t, 5> idxs = {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff};

				if (transforms.is_valid(node_->components[NCI_Transform])) {
					const auto i = used_component_refs[NCI_Transform].find(node_->components[NCI_Transform]);
					idxs[NCI_Transform] = numeric_cast<uint32_t>(std::distance(std::begin(used_component_refs[NCI_Transform]), i));
				}

				if (cameras.is_valid(node_->components[NCI_Camera])) {
					const auto i = used_component_refs[NCI_Camera].find(node_->components[NCI_Camera]);
					idxs[NCI_Camera] = numeric_cast<uint32_t>(std::distance(std::begin(used_component_refs[NCI_Camera]), i));
				}

				if (objects.is_valid(node_->components[NCI_Object])) {
					const auto i = used_component_refs[NCI_Object].find(node_->components[NCI_Object]);
					idxs[NCI_Object] = numeric_cast<uint32_t>(std::distance(std::begin(used_component_refs[NCI_Object]), i));
				}

				if (lights.is_valid(node_->components[NCI_Light])) {
					const auto i = used_component_refs[NCI_Light].find(node_->components[NCI_Light]);
					idxs[NCI_Light] = numeric_cast<uint32_t>(std::distance(std::begin(used_component_refs[NCI_Light]), i));
				}

				if (rigid_bodies.is_valid(node_->components[NCI_RigidBody])) {
					const auto i = used_component_refs[NCI_RigidBody].find(node_->components[NCI_RigidBody]);
					idxs[NCI_RigidBody] = numeric_cast<uint32_t>(std::distance(std::begin(used_component_refs[NCI_RigidBody]), i));
				}

				js_node["components"] = idxs;

				{
					const auto &c = node_collisions.find(ref);

					if (c != std::end(node_collisions)) {
						auto &js_node_cols = js_node["collisions"];

						for (const auto col_ref : c->second) {
							const auto &i = used_collision_refs.find(col_ref);

							json col;
							col["idx"] = std::distance(std::begin(used_collision_refs), i);

							js_node_cols.push_back(col);
						}
					}
				}

				{
					const auto &c = node_scripts.find(ref);

					if (c != std::end(node_scripts)) {
						auto &js_node_scripts = js_node["scripts"];

						for (const auto &script_ref : c->second) {
							const auto &i = used_script_refs.find(script_ref);

							json script;
							script["idx"] = std::distance(std::begin(used_script_refs), i);

							js_node_scripts.push_back(script);
						}
					}
				}

				{
					const auto &c = node_instance.find(ref);
					if (c != std::end(node_instance)) {
						const auto &i = used_instance_refs.find(c->second);
						js_node["instance"] = std::distance(std::begin(used_instance_refs), i);
					}
				}

				js_nodes.push_back(js_node);
			}
	}

	if (save_flags & LSSF_Scene) {
		auto &env = js["environment"];

		if (save_flags & LSSF_Nodes)
			env["current_camera"] = current_camera;

		env["ambient"] = environment.ambient;
		env["fog_near"] = environment.fog_near;
		env["fog_far"] = environment.fog_far;
		env["fog_color"] = environment.fog_color;

		SaveProbe(environment.probe, env["probe"], resources);

		if (environment.brdf_map != InvalidTextureRef)
			env["brdf_map"] = resources.textures.GetName(environment.brdf_map);

		auto &cvs = js["canvas"];

		cvs["clear_z"] = canvas.clear_z;
		cvs["clear_color"] = canvas.clear_color;
		cvs["color"] = canvas.color;
	}

	if (save_flags & LSSF_Anims) {
		{
			json anims_js;

			for (auto ref = anims.first_ref(); ref != InvalidAnimRef; ref = anims.next_ref(ref)) {
				const auto &anim = anims[ref.idx];

				if (anim.flags & AF_Instantiated)
					continue;

				json anim_js = {{"idx", ref.idx}};
				SaveAnimToJson(anim_js["anim"], anim);

				anims_js.push_back(std::move(anim_js));
			}

			if (!anims_js.empty())
				js["anims"] = std::move(anims_js);
		}

		{
			json scene_anims_js;

			for (auto ref = scene_anims.first_ref(); ref != InvalidAnimRef; ref = scene_anims.next_ref(ref)) {
				const auto &scene_anim = scene_anims[ref.idx];

				if (scene_anim.flags & SAF_Instantiated)
					continue;

				json scene_anim_js = {
					{"name", scene_anim.name},
					{"t_start", scene_anim.t_start},
					{"t_end", scene_anim.t_end},
					{"anim", scene_anim.scene_anim.idx},
					{"frame_duration", scene_anim.frame_duration},
				};

				{
					auto &node_anims_js = scene_anim_js["node_anims"];

					for (const auto &node_anim : scene_anim.node_anims)
						node_anims_js.push_back({
							{"node", node_anim.node.idx},
							{"anim", node_anim.anim.idx},
						});
				}

				scene_anims_js.push_back(std::move(scene_anim_js));
			}

			if (!scene_anims_js.empty())
				js["scene_anims"] = std::move(scene_anims_js);
		}
	}

	if (save_flags & LSSF_KeyValues) {
		auto &kv = js["key_values"];
		for (const auto &i : key_values)
			kv[i.first] = i.second;
	}

	return true;
}

bool Scene::Load_json(const json &js, const char *name, const Reader &deps_ir, const ReadProvider &deps_ip, PipelineResources &resources,
	const PipelineInfo &pipeline, LoadSceneContext &ctx, uint32_t load_flags) {
	if (js.empty()) {
		warn(format("Cannot load scene '%1', empty JSON").arg(name));
		return false;
	}

	const auto t_start = time_now();

	//
	std::vector<ComponentRef> transform_refs;
	{
		const auto &i = js.find("transforms");
		if (i != std::end(js)) {
			const auto transform_count = i->size();
			transform_refs.resize(transform_count);

			int n = 0;
			for (const auto &j : *i) {
				const auto ref = transform_refs[n++] = CreateTransform().ref;
				LoadComponent(&transforms[ref.idx], j);
			}
		}
	}

	std::vector<ComponentRef> camera_refs;
	{
		const auto &i = js.find("cameras");
		if (i != std::end(js)) {
			const auto camera_count = i->size();
			camera_refs.resize(camera_count);

			int n = 0;
			for (const auto &j : *i) {
				const auto ref = camera_refs[n++] = CreateCamera().ref;
				LoadComponent(&cameras[ref.idx], j);
			}
		}
	}

	std::vector<ComponentRef> object_refs;
	{
		const auto &i = js.find("objects");
		if (i != std::end(js)) {
			const auto object_count = i->size();
			object_refs.resize(object_count);

			int n = 0;
			for (const auto &j : *i) {
				const auto ref = object_refs[n++] = CreateObject().ref;
				LoadComponent(&objects[ref.idx], j, deps_ir, deps_ip, resources, pipeline, load_flags & LSSF_QueueModelLoads,
					load_flags & LSSF_QueueTextureLoads, load_flags & LSSF_DoNotLoadResources);
			}
		}
	}

	std::vector<ComponentRef> light_refs;
	{
		const auto &i = js.find("lights");
		if (i != std::end(js)) {
			const auto light_count = i->size();
			light_refs.resize(light_count);

			int n = 0;
			for (const auto &j : *i) {
				const auto ref = light_refs[n++] = CreateLight().ref;
				LoadComponent(&lights[ref.idx], j);
			}
		}
	}

	std::vector<ComponentRef> rigid_body_refs;
	{
		const auto &i = js.find("rigid_bodies");
		if (i != std::end(js)) {
			const auto rigid_body_count = i->size();
			rigid_body_refs.resize(rigid_body_count);

			int n = 0;
			for (const auto &j : *i) {
				const auto ref = rigid_body_refs[n++] = CreateRigidBody().ref;
				LoadComponent(&rigid_bodies[ref.idx], j);
			}
		}
	}

	std::vector<ComponentRef> col_refs;
	{
		const auto &i = js.find("collisions");
		if (i != std::end(js)) {
			const auto col_count = i->size();
			col_refs.resize(col_count);

			int n = 0;
			for (const auto &j : *i) {
				const auto ref = col_refs[n++] = CreateCollision().ref;
				LoadComponent(&collisions[ref.idx], j);
			}
		}
	}

	std::vector<ComponentRef> script_refs;
	{
		const auto &i = js.find("scripts");
		if (i != std::end(js)) {
			const auto scripts_count = i->size();
			script_refs.resize(scripts_count);

			int n = 0;
			for (const auto &j : *i) {
				const auto ref = script_refs[n++] = CreateScript().ref;
				LoadComponent(&scripts[ref.idx], j);
			}
		}
	}

	std::vector<ComponentRef> instance_refs;
	{
		const auto &i = js.find("instances");
		if (i != std::end(js)) {
			const auto instance_count = i->size();
			instance_refs.resize(instance_count);

			int n = 0;
			for (const auto &j : *i) {
				const auto ref = instance_refs[n++] = CreateInstance().ref;
				LoadComponent(&instances[ref.idx], j);
			}
		}
	}

	//
	std::vector<NodeRef> nodes_to_disable;

	const auto &js_nodes = js.find("nodes");

	if (js_nodes != std::end(js)) {
		std::vector<NodeRef> node_with_instance_to_setup;
		node_with_instance_to_setup.reserve(64);

		const auto node_count = js_nodes->size();
		for (const auto &js_node : *js_nodes) {
			auto node = CreateNode();
			auto node_ = nodes[node.ref.idx];

			ctx.node_refs[js_node["idx"].get<uint32_t>()] = node.ref;
			ctx.view.nodes.push_back(node.ref);

			node.SetName(js_node["name"].get<std::string>());

			{
				const auto &i = js_node.find("disabled");
				if ((i != std::end(js_node)) && (i->get<bool>() == true))
					nodes_to_disable.push_back(node.ref);
			}

			{
				const auto &js_node_components = js_node["components"];

				const auto transform_idx = js_node_components[NCI_Transform].get<ComponentRef>().idx;
				if (transform_idx != 0xffffffff)
					nodes[node.ref.idx].components[NCI_Transform] = transform_refs[transform_idx];

				const auto camera_idx = js_node_components[NCI_Camera].get<ComponentRef>().idx;
				if (camera_idx != 0xffffffff)
					nodes[node.ref.idx].components[NCI_Camera] = camera_refs[camera_idx];

				const auto object_idx = js_node_components[NCI_Object].get<ComponentRef>().idx;
				if (object_idx != 0xffffffff)
					nodes[node.ref.idx].components[NCI_Object] = object_refs[object_idx];

				const auto light_idx = js_node_components[NCI_Light].get<ComponentRef>().idx;
				if (light_idx != 0xffffffff)
					nodes[node.ref.idx].components[NCI_Light] = light_refs[light_idx];

				const auto rigid_body_idx = js_node_components[NCI_RigidBody].get<ComponentRef>().idx;
				if (rigid_body_idx != 0xffffffff)
					nodes[node.ref.idx].components[NCI_RigidBody] = rigid_body_refs[rigid_body_idx];
			}

			{
				const auto &js_cols = js_node.find("collisions");

				if (js_cols != std::end(js_node)) {
					const auto node_col_count = js_cols->size();
					node_collisions[node.ref].reserve(node_col_count);
					for (const auto &js_col : *js_cols) {
						const auto col_idx = js_col["idx"].get<ComponentRef>().idx;

						if (col_idx != 0xffffffff) {
							const auto col_ref = col_refs[col_idx];
							node_collisions[node.ref].push_back(col_ref);
						} else {
							node_collisions[node.ref].push_back(InvalidComponentRef);
						}
					}
				}
			}

			{
				const auto &js_scripts = js_node.find("scripts");

				if (js_scripts != std::end(js_node)) {
					const auto node_script_count = js_scripts->size();
					node_scripts[node.ref].reserve(node_script_count);
					for (const auto &js_script : *js_scripts) {
						const auto script_idx = js_script["idx"].get<ComponentRef>().idx;

						if (script_idx != 0xffffffff) {
							const auto script_ref = script_refs[script_idx];
							node_scripts[node.ref].push_back(script_ref);
						} else {
							node_scripts[node.ref].push_back(InvalidComponentRef);
						}
					}
				}
			}

			{
				const auto &js_node_instance = js_node.find("instance");
				if (js_node_instance != std::end(js_node)) {
					const auto instance_idx = js_node_instance->get<ComponentRef>().idx;
					if (instance_idx != 0xffffffff) {
						node_instance[node.ref] = instance_refs[instance_idx];
						node_with_instance_to_setup.push_back(node.ref);
					}
				}
			}
		}

		// setup instances
		if (!(load_flags & LSSF_DoNotLoadResources))
			for (const auto ref : node_with_instance_to_setup) {
				NodeSetupInstance(ref, deps_ir, deps_ip, resources, pipeline, LSSF_AllNodeFeatures | (load_flags & LSSF_OptionsMask), ctx.recursion_level + 1);
				NodeStartOnInstantiateAnim(ref);
			}

		// fix parent references
		for (const auto ref : transform_refs) {
			auto &c = transforms[ref.idx];
			if (c.parent != InvalidNodeRef) {
				const auto &i = ctx.node_refs.find(c.parent.idx);
				c.parent = i != std::end(ctx.node_refs) ? i->second : InvalidNodeRef;
			}
		}

		// fix bone references
		for (const auto ref : object_refs) {
			auto &c = objects[ref.idx];
			for (auto &ref : c.bones)
				if (ref != InvalidNodeRef) {
					const auto &i = ctx.node_refs.find(ref.idx);
					ref = i != std::end(ctx.node_refs) ? i->second : InvalidNodeRef;
				}
		}
	}

	if (load_flags & LSSF_Scene) {
		{
			const auto &js_env = js.find("environment");

			if (js_env != std::end(js)) {
				const bool can_change_current_camera = current_camera == InvalidNodeRef || !(load_flags & LSSF_DoNotChangeCurrentCameraIfValid);

				if (can_change_current_camera) {
					const auto &js_current_camera = js_env->find("current_camera");
					if (js_current_camera != std::end(*js_env)) {
						const auto current_camera_idx = js_current_camera->get<NodeRef>().idx;
						current_camera = ctx.node_refs[current_camera_idx];
					}
				}

				environment.ambient = (*js_env)["ambient"];
				environment.fog_near = (*js_env)["fog_near"];
				environment.fog_far = (*js_env)["fog_far"];
				environment.fog_color = (*js_env)["fog_color"];

				{
					const auto &i = js_env->find("probe");

					if (i != std::end(*js_env)) {
						LoadProbe(environment.probe, *i, deps_ir, deps_ip, resources, load_flags & LSSF_QueueTextureLoads, load_flags & LSSF_DoNotLoadResources,
							load_flags & LSSF_Silent);
					} else {
						environment.probe = {};

						{
							const auto &i = js_env->find("irradiance_map");
							if (i != std::end(*js_env))
								environment.probe.irradiance_map = SkipLoadOrQueueTextureLoad(deps_ir, deps_ip, i->get<std::string>().c_str(), resources,
									load_flags & LSSF_QueueTextureLoads, load_flags & LSSF_DoNotLoadResources, load_flags & LSSF_Silent);
						}

						{
							const auto &i = js_env->find("radiance_map");
							if (i != std::end(*js_env))
								environment.probe.radiance_map = SkipLoadOrQueueTextureLoad(deps_ir, deps_ip, i->get<std::string>().c_str(), resources,
									load_flags & LSSF_QueueTextureLoads, load_flags & LSSF_DoNotLoadResources, load_flags & LSSF_Silent);
						}
					}
				}

				{
					const auto &i = js_env->find("brdf_map");
					if (i != std::end(*js_env))
						environment.brdf_map = SkipLoadOrQueueTextureLoad(deps_ir, deps_ip, i->get<std::string>().c_str(), resources,
							load_flags & LSSF_QueueTextureLoads, load_flags & LSSF_DoNotLoadResources, load_flags & LSSF_Silent);
				}
			}
		}

		{
			const auto &js_cvs = js.find("canvas");

			if (js_cvs != std::end(js)) {
				canvas.clear_z = (*js_cvs)["clear_z"];
				canvas.clear_color = (*js_cvs)["clear_color"];
				canvas.color = (*js_cvs)["color"];
			}
		}
	}

	if (load_flags & LSSF_Anims) {
		std::map<uint32_t, AnimRef> anim_refs;

		{
			const auto &js_anims = js.find("anims");

			if (js_anims != std::end(js))
				for (const auto &js_anim : *js_anims) {
					Anim anim;
					LoadAnimFromJson(js_anim["anim"], anim);

					const auto anim_ref = AddAnim(anim);
					ctx.view.anims.push_back(anim_ref);

					const uint32_t idx = js_anim["idx"];
					anim_refs[idx] = anim_ref;
				}
		}

		{
			const auto &js_scene_anims = js.find("scene_anims");

			if (js_scene_anims != std::end(js)) {
				for (const auto &js_scene_anim : *js_scene_anims) {
					SceneAnim scene_anim;

					js_scene_anim.at("name").get_to(scene_anim.name);
					scene_anim.t_start = js_scene_anim.at("t_start");
					scene_anim.t_end = js_scene_anim.at("t_end");
					scene_anim.frame_duration = js_scene_anim.at("frame_duration");

					uint32_t scene_anim_idx = invalid_gen_ref.idx;
					const auto &i_anim = js_scene_anim.find("anim");
					if (i_anim != std::end(js_scene_anim))
						if (!i_anim->is_null()) // [EJ] legacy format field was broken
							scene_anim_idx = i_anim->get<uint32_t>();

					const auto &i_scene_anim = anim_refs.find(scene_anim_idx);
					if (i_scene_anim != std::end(anim_refs))
						scene_anim.scene_anim = i_scene_anim->second;

					const auto &js_node_anims = js_scene_anim.find("node_anims");
					if (js_node_anims != std::end(js_scene_anim))
						for (const auto js_node_anim : *js_node_anims) {
							const uint32_t node_idx = js_node_anim["node"];
							const uint32_t anim_idx = js_node_anim["anim"];

							const auto &i_node_ref = ctx.node_refs.find(node_idx); // remap node
							const auto &i_anim_ref = anim_refs.find(anim_idx); // remap anim

							if (i_node_ref != std::end(ctx.node_refs) && i_anim_ref != std::end(anim_refs))
								scene_anim.node_anims.push_back({i_node_ref->second, i_anim_ref->second});
						}

					const auto scene_anim_ref = AddSceneAnim(scene_anim);
					ctx.view.scene_anims.push_back(scene_anim_ref);
				}
			}
		}
	}

	if (load_flags & LSSF_KeyValues) {
		const auto &js_key_values = js.find("key_values");

		if (js_key_values != std::end(js))
			for (const auto &i : js_key_values->items())
				key_values[i.key()] = i.value().get<std::string>();
	}

	//
	for (const auto &ref : nodes_to_disable)
		DisableNode(ref);

	//
	ReadyWorldMatrices();
	ComputeWorldMatrices();

	//
	debug(format("Load scene '%1' took %2 ms").arg(name).arg(time_to_ms(time_now() - t_start)));
	return true;
}

} // namespace hg
