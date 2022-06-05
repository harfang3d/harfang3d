// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/file_format.h"
#include "engine/render_pipeline.h"
#include "engine/scene.h"

#include "foundation/format.h"
#include "foundation/log.h"
#include "foundation/profiler.h"

#include <set>

namespace hg {

// [EJ] note: SaveComponent/LoadComponent are friend of class Scene and cannot be made static (breaks clang/gcc builds)

void SaveComponent(const Scene::Transform_ *data_, const Writer &iw, const Handle &h) {
	Write(iw, h, data_->TRS); // pos, rot, scl
	Write(iw, h, data_->parent.idx);
}

void SaveComponent(const Scene::Camera_ *data_, const Writer &iw, const Handle &h) {
	Write(iw, h, data_->zrange);
	Write(iw, h, data_->fov);
	Write(iw, h, data_->ortho);
	Write(iw, h, data_->size);
}

void SaveComponent(const Scene::Object_ *data_, const Writer &iw, const Handle &h, const PipelineResources &resources) {
	Write(iw, h, resources.models.GetName(data_->model));

	const auto mat_count = data_->materials.size();
	Write(iw, h, uint16_t(mat_count));
	for (size_t j = 0; j < mat_count; ++j)
		SaveMaterial(data_->materials[j], iw, h, resources);
	for (size_t j = 0; j < mat_count; ++j)
		Write(iw, h, j < data_->material_infos.size() ? data_->material_infos[j].name : std::string());

	__ASSERT__(data_->bones.size() <= std::numeric_limits<uint16_t>::max());
	Write(iw, h, uint16_t(data_->bones.size()));
	for (auto bone : data_->bones)
		Write(iw, h, bone.idx);
}

void SaveComponent(const Scene::Light_ *data_, const Writer &iw, const Handle &h) {
	Write(iw, h, data_->type);
	Write(iw, h, data_->shadow_type);
	Write(iw, h, data_->diffuse);
	Write(iw, h, data_->diffuse_intensity);
	Write(iw, h, data_->specular);
	Write(iw, h, data_->specular_intensity);
	Write(iw, h, data_->radius);
	Write(iw, h, data_->inner_angle);
	Write(iw, h, data_->outer_angle);
	Write(iw, h, data_->pssm_split);
	Write(iw, h, data_->priority);
	Write(iw, h, data_->shadow_bias);
}

void SaveComponent(const Scene::RigidBody_ *data_, const Writer &iw, const Handle &h) {
	Write(iw, h, data_->type);
	Write(iw, h, data_->linear_damping);
	Write(iw, h, data_->angular_damping);
	Write(iw, h, data_->restitution);
	Write(iw, h, data_->friction);
	Write(iw, h, data_->rolling_friction);
}

void SaveComponent(const Scene::Collision_ *data_, const Writer &iw, const Handle &h) {
	Write(iw, h, data_->type);
	Write(iw, h, data_->mass);
	Write(iw, h, data_->resource_path);
	Write(iw, h, data_->trs);
}

void SaveComponent(const Scene::Script_ *data_, const Writer &iw, const Handle &h) {
	Write(iw, h, data_->path);

	const auto &params = data_->params;
	Write(iw, h, uint16_t(params.size()));

	for (auto &i : params) {
		Write(iw, h, i.first); // name
		Write(iw, h, uint16_t(i.second.type)); // type

		if (i.second.type == SPT_Bool)
			Write(iw, h, i.second.bv);
		else if (i.second.type == SPT_Int)
			Write(iw, h, i.second.iv);
		else if (i.second.type == SPT_Float)
			Write(iw, h, i.second.fv);
		else if (i.second.type == SPT_String)
			Write(iw, h, i.second.sv);
	}
}

void SaveComponent(const Scene::Instance_ *data_, const Writer &iw, const Handle &h) {
	Write(iw, h, data_->name);
	Write(iw, h, data_->anim);
	Write(iw, h, uint8_t(data_->loop_mode));
}

static void SaveProbe(const Probe &probe, const Writer &iw, const Handle &h, const PipelineResources &resources) {
	Write(iw, h, resources.textures.GetName(probe.irradiance_map));
	Write(iw, h, resources.textures.GetName(probe.radiance_map));

	Write(iw, h, probe.type);
	Write(iw, h, probe.parallax);
	Write(iw, h, probe.trs);
}

//
void LoadComponent(Scene::Transform_ *data_, const Reader &ir, const Handle &h) {
	Read(ir, h, data_->TRS);
	Read(ir, h, data_->parent.idx);
}

void LoadComponent(Scene::Camera_ *data_, const Reader &ir, const Handle &h) {
	Read(ir, h, data_->zrange);
	Read(ir, h, data_->fov);
	Read(ir, h, data_->ortho);
	Read(ir, h, data_->size);
}

void LoadComponent(Scene::Object_ *data_, const Reader &ir, const Handle &h, const Reader &deps_ir, const ReadProvider &deps_ip,
	PipelineResources &resources, const PipelineInfo &pipeline, bool queue_model_loads, bool queue_texture_loads, bool do_not_load_resources, bool silent) {
	std::string name;
	Read(ir, h, name);

	if (!name.empty())
		data_->model = SkipLoadOrQueueModelLoad(deps_ir, deps_ip, name.c_str(), resources, queue_model_loads, do_not_load_resources, silent);

	const auto mat_count = Read<uint16_t>(ir, h);
	data_->materials.resize(mat_count);
	for (auto i = 0; i < mat_count; ++i)
		data_->materials[i] = LoadMaterial(ir, h, deps_ir, deps_ip, resources, pipeline, queue_texture_loads, do_not_load_resources, silent);

	data_->material_infos.resize(mat_count);
	for (auto i = 0; i < mat_count; ++i)
		Read(ir, h, data_->material_infos[i].name);

	const auto bone_count = Read<uint16_t>(ir, h);
	data_->bones.resize(bone_count);
	for (auto i = 0; i < bone_count; ++i)
		Read(ir, h, data_->bones[i].idx);
}

void LoadComponent(Scene::Light_ *data_, const Reader &ir, const Handle &h) {
	Read(ir, h, data_->type);
	Read(ir, h, data_->shadow_type);
	Read(ir, h, data_->diffuse);
	Read(ir, h, data_->diffuse_intensity);
	Read(ir, h, data_->specular);
	Read(ir, h, data_->specular_intensity);
	Read(ir, h, data_->radius);
	Read(ir, h, data_->inner_angle);
	Read(ir, h, data_->outer_angle);
	Read(ir, h, data_->pssm_split);
	Read(ir, h, data_->priority);
	Read(ir, h, data_->shadow_bias);
}

void LoadComponent(Scene::RigidBody_ *data_, const Reader &ir, const Handle &h) {
	Read(ir, h, data_->type);
	Read(ir, h, data_->linear_damping);
	Read(ir, h, data_->angular_damping);
	Read(ir, h, data_->restitution);
	Read(ir, h, data_->friction);
	Read(ir, h, data_->rolling_friction);
}

void LoadComponent(Scene::Collision_ *data_, const Reader &ir, const Handle &h) {
	Read(ir, h, data_->type);
	Read(ir, h, data_->mass);
	Read(ir, h, data_->resource_path);
	Read(ir, h, data_->trs);
}

void LoadComponent(Scene::Script_ *data_, const Reader &ir, const Handle &h) {
	data_->path = Read<std::string>(ir, h);

	const auto count = Read<uint16_t>(ir, h);

	for (uint16_t i = 0; i < count; ++i) {
		const auto name = Read<std::string>(ir, h);

		ScriptParam parm;
		parm.type = (ScriptParamType)Read<uint16_t>(ir, h);

		if (parm.type == SPT_Bool)
			Read(ir, h, parm.bv);
		else if (parm.type == SPT_Int)
			Read(ir, h, parm.iv);
		else if (parm.type == SPT_Float)
			Read(ir, h, parm.fv);
		else if (parm.type == SPT_String)
			Read(ir, h, parm.sv);

		data_->params[name] = std::move(parm);
	}
}

void LoadComponent(Scene::Instance_ *data_, const Reader &ir, const Handle &h) {
	data_->name = Read<std::string>(ir, h);
	data_->anim = Read<std::string>(ir, h);
	data_->loop_mode = AnimLoopMode(Read<uint8_t>(ir, h));
}

static void LoadProbe(Probe &probe, const Reader &ir, const Handle &h, const Reader &deps_ir, const ReadProvider &deps_ip, PipelineResources &resources,
	const PipelineInfo &pipeline, bool queue_texture_loads, bool do_not_load_resources, bool silent) {
	std::string tex_name;

	Read(ir, h, tex_name);
	if (!tex_name.empty())
		probe.irradiance_map = SkipLoadOrQueueTextureLoad(deps_ir, deps_ip, tex_name.c_str(), resources, queue_texture_loads, do_not_load_resources, silent);
	Read(ir, h, tex_name);
	if (!tex_name.empty())
		probe.radiance_map = SkipLoadOrQueueTextureLoad(deps_ir, deps_ip, tex_name.c_str(), resources, queue_texture_loads, do_not_load_resources, silent);

	Read(ir, h, probe.type);
	Read(ir, h, probe.parallax);
	Read(ir, h, probe.trs);
}

static void SkipProbe(const Reader &ir, const Handle &h) {
	SkipString(ir, h);
	SkipString(ir, h);

	Skip<ProbeType>(ir, h);
	Skip<uint8_t>(ir, h);
	Skip<TransformTRS>(ir, h);
}

//
uint32_t GetSceneBinaryFormatVersion() { return 9; }

bool Scene::Save_binary(
	const Writer &iw, const Handle &h, const PipelineResources &resources, uint32_t save_flags, const std::vector<NodeRef> *nodes_to_save) const {
	if (!iw.is_valid(h))
		return false;

	Write(iw, h, HarfangMagic);
	Write(iw, h, SceneMarker);

	/*
		version 0: initial format
		version 1: add support for scene key/value storage
		version 2: add skinning support in Object component
		version 3: add support for arbitrary number of bones
		version 4: light intensity factors
		version 5: save rigid body properties
		version 6: remove obsolete rigid properties
		version 7: store animation chunk size so that it can be jumped over without parsing its content
		version 8: save collision properties
		version 9: save environment probe
	*/
	const auto version = GetSceneBinaryFormatVersion();
	Write<uint32_t>(iw, h, version);

	Write<uint32_t>(iw, h, save_flags); // so that we know what to expect when loading this data back

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

				if (save_flags & LSSF_Physics) {
					if (rigid_bodies.is_valid(node_->components[NCI_RigidBody]))
						used_component_refs[NCI_RigidBody].insert(node_->components[NCI_RigidBody]);

					{
						const auto &i = node_collisions.find(ref);
						if (i != std::end(node_collisions))
							for (const auto &ref : i->second)
								used_collision_refs.insert(ref);
					}
				}

				if (save_flags & LSSF_Scripts) {
					const auto &i = node_scripts.find(ref);
					if (i != std::end(node_scripts))
						for (const auto &ref : i->second)
							used_script_refs.insert(ref);
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
	Write(iw, h, numeric_cast<uint32_t>(used_component_refs[NCI_Transform].size()));
	for (const auto &ref : used_component_refs[NCI_Transform])
		if (transforms.is_valid(ref))
			SaveComponent(&transforms[ref.idx], iw, h);

	Write(iw, h, numeric_cast<uint32_t>(used_component_refs[NCI_Camera].size()));
	for (const auto &ref : used_component_refs[NCI_Camera])
		if (cameras.is_valid(ref))
			SaveComponent(&cameras[ref.idx], iw, h);

	Write(iw, h, numeric_cast<uint32_t>(used_component_refs[NCI_Object].size()));
	for (const auto &ref : used_component_refs[NCI_Object])
		if (objects.is_valid(ref))
			SaveComponent(&objects[ref.idx], iw, h, resources);

	Write(iw, h, numeric_cast<uint32_t>(used_component_refs[NCI_Light].size()));
	for (const auto &ref : used_component_refs[NCI_Light])
		if (lights.is_valid(ref))
			SaveComponent(&lights[ref.idx], iw, h);

	if (save_flags & LSSF_Physics) {
		Write(iw, h, numeric_cast<uint32_t>(used_component_refs[NCI_RigidBody].size()));
		for (const auto &ref : used_component_refs[NCI_RigidBody])
			if (rigid_bodies.is_valid(ref))
				SaveComponent(&rigid_bodies[ref.idx], iw, h);

		Write(iw, h, numeric_cast<uint32_t>(used_collision_refs.size()));
		for (const auto &ref : used_collision_refs)
			if (collisions.is_valid(ref))
				SaveComponent(&collisions[ref.idx], iw, h);
	}

	if (save_flags & LSSF_Scripts) {
		Write(iw, h, numeric_cast<uint32_t>(used_script_refs.size()));
		for (const auto &ref : used_script_refs)
			if (scripts.is_valid(ref))
				SaveComponent(&scripts[ref.idx], iw, h);
	}

	Write(iw, h, numeric_cast<uint32_t>(used_instance_refs.size()));
	for (const auto &ref : used_instance_refs)
		if (instances.is_valid(ref))
			SaveComponent(&instances[ref.idx], iw, h);

	//
	if (save_flags & LSSF_Nodes) {
		Write(iw, h, numeric_cast<uint32_t>(node_refs.size()));

		for (const auto &ref : node_refs)
			if (const auto *node_ = GetNode_(ref)) {
				Write(iw, h, ref.idx);
				Write(iw, h, node_->name);
				Write(iw, h, node_->flags & NF_SerializedMask);

				if (transforms.is_valid(node_->components[NCI_Transform])) {
					const auto &i = used_component_refs[NCI_Transform].find(node_->components[NCI_Transform]);
					Write(iw, h, numeric_cast<uint32_t>(std::distance(std::begin(used_component_refs[NCI_Transform]), i)));
				} else {
					Write<uint32_t>(iw, h, 0xffffffff);
				}

				if (cameras.is_valid(node_->components[NCI_Camera])) {
					const auto &i = used_component_refs[NCI_Camera].find(node_->components[NCI_Camera]);
					Write(iw, h, numeric_cast<uint32_t>(std::distance(std::begin(used_component_refs[NCI_Camera]), i)));
				} else {
					Write<uint32_t>(iw, h, 0xffffffff);
				}

				if (objects.is_valid(node_->components[NCI_Object])) {
					const auto &i = used_component_refs[NCI_Object].find(node_->components[NCI_Object]);
					Write(iw, h, numeric_cast<uint32_t>(std::distance(std::begin(used_component_refs[NCI_Object]), i)));
				} else {
					Write<uint32_t>(iw, h, 0xffffffff);
				}

				if (lights.is_valid(node_->components[NCI_Light])) {
					const auto &i = used_component_refs[NCI_Light].find(node_->components[NCI_Light]);
					Write(iw, h, numeric_cast<uint32_t>(std::distance(std::begin(used_component_refs[NCI_Light]), i)));
				} else {
					Write<uint32_t>(iw, h, 0xffffffff);
				}

				if (save_flags & LSSF_Physics) {
					if (rigid_bodies.is_valid(node_->components[NCI_RigidBody])) {
						const auto &i = used_component_refs[NCI_RigidBody].find(node_->components[NCI_RigidBody]);
						Write(iw, h, numeric_cast<uint32_t>(std::distance(std::begin(used_component_refs[NCI_RigidBody]), i)));
					} else {
						Write<uint32_t>(iw, h, 0xffffffff);
					}

					const auto &c = node_collisions.find(ref);
					if (c != std::end(node_collisions)) {
						Write(iw, h, numeric_cast<uint32_t>(c->second.size())); // collision count
						for (const auto &col_ref : c->second) {
							const auto &i = used_collision_refs.find(col_ref);
							Write(iw, h, numeric_cast<uint32_t>(std::distance(std::begin(used_collision_refs), i))); // idx
						}
					} else {
						Write<uint32_t>(iw, h, 0); // collision count
					}
				}

				if (save_flags & LSSF_Scripts) {
					const auto &c = node_scripts.find(ref);
					if (c != std::end(node_scripts)) {
						Write(iw, h, numeric_cast<uint32_t>(c->second.size())); // script count
						for (const auto &script_ref : c->second) {
							const auto &i = used_script_refs.find(script_ref);
							Write(iw, h, numeric_cast<uint32_t>(std::distance(std::begin(used_script_refs), i))); // idx
						}
					} else {
						Write<uint32_t>(iw, h, 0); // script count
					}
				}

				{
					const auto c = node_instance.find(ref);
					if (c != std::end(node_instance)) {
						const auto &i = used_instance_refs.find(c->second);
						Write(iw, h, numeric_cast<uint32_t>(std::distance(std::begin(used_instance_refs), i)));
					} else {
						Write(iw, h, InvalidComponentRef.idx);
					}
				}
			}
	}

	if (save_flags & LSSF_Scene) {
		if (save_flags & LSSF_Nodes)
			Write(iw, h, current_camera.idx);

		Write(iw, h, environment.ambient);
		Write(iw, h, environment.fog_near);
		Write(iw, h, environment.fog_far);
		Write(iw, h, environment.fog_color);

		SaveProbe(environment.probe, iw, h, resources);
		Write(iw, h, resources.textures.GetName(environment.brdf_map));

		Write(iw, h, canvas.clear_z);
		Write(iw, h, canvas.clear_color);
		Write(iw, h, canvas.color);
	}

	if (save_flags & LSSF_Anims) {
		DeferredWrite<uint32_t> chunk_size(iw, h);

		const auto anim_start_cursor = Tell(iw, h);

		{
			uint32_t count = 0;
			for (auto ref = anims.first_ref(); ref != InvalidAnimRef; ref = anims.next_ref(ref)) {
				const auto &anim = anims[ref.idx];
				if (!(anim.flags & AF_Instantiated))
					++count;
			}

			Write(iw, h, count);
			for (auto ref = anims.first_ref(); ref != InvalidAnimRef; ref = anims.next_ref(ref)) {
				const auto &anim = anims[ref.idx];
				if (anim.flags & AF_Instantiated)
					continue;

				Write(iw, h, ref.idx);
				SaveAnimToBinary(iw, h, anim);
			}
		}

		{
			uint32_t count = 0;
			for (auto ref = scene_anims.first_ref(); ref != InvalidAnimRef; ref = scene_anims.next_ref(ref)) {
				const auto &scene_anim = scene_anims[ref.idx];
				if (!(scene_anim.flags & SAF_Instantiated))
					++count;
			}

			Write(iw, h, count);
			for (auto ref = scene_anims.first_ref(); ref != InvalidAnimRef; ref = scene_anims.next_ref(ref)) {
				const auto &scene_anim = scene_anims[ref.idx];
				if (scene_anim.flags & SAF_Instantiated)
					continue;

				Write(iw, h, scene_anim.name);
				Write(iw, h, scene_anim.t_start);
				Write(iw, h, scene_anim.t_end);
				Write(iw, h, scene_anim.scene_anim.idx);
				Write(iw, h, scene_anim.frame_duration);

				{
					Write(iw, h, uint32_t(scene_anim.node_anims.size()));
					for (const auto &node_anim : scene_anim.node_anims) {
						Write(iw, h, node_anim.node.idx);
						Write(iw, h, node_anim.anim.idx);
					}
				}
			}
		}

		const auto anim_end_cursor = Tell(iw, h);
		chunk_size.Commit(numeric_cast<uint32_t>(anim_end_cursor - anim_start_cursor));
	}

	if (save_flags & LSSF_KeyValues) {
		Write(iw, h, uint32_t(key_values.size()));

		for (const auto &i : key_values) {
			Write(iw, h, i.first);
			Write(iw, h, i.second);
		}
	}

	return true;
}

bool Scene::Load_binary(const Reader &ir, const Handle &h, const char *name, const Reader &deps_ir, const ReadProvider &deps_ip, PipelineResources &resources,
	const PipelineInfo &pipeline, LoadSceneContext &ctx, uint32_t load_flags) {
	ProfilerPerfSection section("Scene::Load_binary");

	const auto t_start = time_now();
	const auto silent = load_flags & LSSF_Silent;

	if (!ir.is_valid(h)) {
		if (!silent)
			warn(format("Cannot load scene '%1', invalid read handle").arg(name));
		return false;
	}

	if (Read<uint32_t>(ir, h) != HarfangMagic) {
		if (!silent)
			warn(format("Cannot load scene '%1', invalid magic marker").arg(name));
		return false;
	}

	if (Read<uint8_t>(ir, h) != SceneMarker) {
		if (!silent)
			warn(format("Cannot load scene '%1', invalid scene marker").arg(name));
		return false;
	}

	const auto version = Read<uint32_t>(ir, h);
	if (version != GetSceneBinaryFormatVersion()) {
		if (!silent)
			warn(format("Cannot load scene '%1', unsupported binary version %2").arg(name).arg(version));
		return false;
	}

	// get what was actually saved to the file
	const auto file_flags = Read<uint32_t>(ir, h);

	//
	const auto load_component_section_index = BeginProfilerSection("Scene::Load_binary: Load Components");

	const auto transform_count = Read<uint32_t>(ir, h);
	std::vector<ComponentRef> transform_refs(transform_count);
	for (size_t i = 0; i < transform_count; ++i) {
		const auto ref = transform_refs[i] = CreateTransform().ref;
		LoadComponent(&transforms[ref.idx], ir, h);
	}

	const auto camera_count = Read<uint32_t>(ir, h);
	std::vector<ComponentRef> camera_refs(camera_count);
	for (size_t i = 0; i < camera_count; ++i) {
		const auto ref = camera_refs[i] = CreateCamera().ref;
		LoadComponent(&cameras[ref.idx], ir, h);
	}

	const auto object_count = Read<uint32_t>(ir, h);
	std::vector<ComponentRef> object_refs(object_count);
	for (size_t i = 0; i < object_count; ++i) {
		const auto ref = object_refs[i] = CreateObject().ref;
		LoadComponent(&objects[ref.idx], ir, h, deps_ir, deps_ip, resources, pipeline, load_flags & LSSF_QueueModelLoads, load_flags & LSSF_QueueTextureLoads,
			load_flags & LSSF_DoNotLoadResources, silent);
	}

	const auto light_count = Read<uint32_t>(ir, h);
	std::vector<ComponentRef> light_refs(light_count);
	for (size_t i = 0; i < light_count; ++i) {
		const auto ref = light_refs[i] = CreateLight().ref;
		LoadComponent(&lights[ref.idx], ir, h);
	}

	std::vector<ComponentRef> rigid_body_refs, collision_refs;
	if (file_flags & LSSF_Physics) {
		const auto rigid_body_count = Read<uint32_t>(ir, h);
		rigid_body_refs.resize(rigid_body_count);
		for (size_t i = 0; i < rigid_body_count; ++i) {
			const auto ref = rigid_body_refs[i] = CreateRigidBody().ref;
			LoadComponent(&rigid_bodies[ref.idx], ir, h);
		}

		const auto collision_count = Read<uint32_t>(ir, h);
		collision_refs.resize(collision_count);
		for (size_t i = 0; i < collision_count; ++i) {
			const auto ref = collision_refs[i] = CreateCollision().ref;
			LoadComponent(&collisions[ref.idx], ir, h);
		}
	}

	std::vector<ComponentRef> script_refs;
	if (file_flags & LSSF_Scripts) {
		const auto script_count = Read<uint32_t>(ir, h);
		script_refs.resize(script_count);
		for (size_t i = 0; i < script_count; ++i) {
			const auto ref = script_refs[i] = CreateScript().ref;
			LoadComponent(&scripts[ref.idx], ir, h);
		}
	}

	const auto instance_count = Read<uint32_t>(ir, h);
	std::vector<ComponentRef> instance_refs(instance_count);
	for (size_t i = 0; i < instance_count; ++i) {
		const auto ref = instance_refs[i] = CreateInstance().ref;
		LoadComponent(&instances[ref.idx], ir, h);
	}

	EndProfilerSection(load_component_section_index);

	//
	std::vector<NodeRef> nodes_to_disable;

	if (file_flags & LSSF_Nodes) {
		ProfilerPerfSection section("Scene::Load_binary: Load Nodes");

		std::vector<NodeRef> node_with_instance_to_setup;
		node_with_instance_to_setup.reserve(64);

		const auto node_count = Read<uint32_t>(ir, h);
		for (uint32_t i = 0; i < node_count; ++i) {
			auto node_ref = CreateNode().ref;
			ctx.node_refs[Read<uint32_t>(ir, h)] = node_ref;
			ctx.view.nodes.push_back(node_ref);

			auto &node_ = nodes[node_ref.idx];
			node_.name = Read<std::string>(ir, h);

			const auto node_flags = Read<uint32_t>(ir, h);
			if (node_flags & NF_Disabled)
				nodes_to_disable.push_back(node_ref);

			const auto transform_idx = Read<uint32_t>(ir, h);
			if (transform_idx != 0xffffffff)
				node_.components[NCI_Transform] = transform_refs[transform_idx];

			const auto camera_idx = Read<uint32_t>(ir, h);
			if (camera_idx != 0xffffffff)
				node_.components[NCI_Camera] = camera_refs[camera_idx];

			const auto object_idx = Read<uint32_t>(ir, h);
			if (object_idx != 0xffffffff)
				node_.components[NCI_Object] = object_refs[object_idx];

			const auto light_idx = Read<uint32_t>(ir, h);
			if (light_idx != 0xffffffff)
				node_.components[NCI_Light] = light_refs[light_idx];

			if (file_flags & LSSF_Physics) {
				const auto rigid_body_idx = Read<uint32_t>(ir, h);
				if (rigid_body_idx != 0xffffffff)
					node_.components[NCI_RigidBody] = rigid_body_refs[rigid_body_idx];

				const auto collision_count = Read<uint32_t>(ir, h);
				for (uint32_t j = 0; j < collision_count; ++j) {
					const auto col_idx = Read<uint32_t>(ir, h);
					node_collisions[node_ref].push_back(collision_refs[col_idx]);
				}
			}

			if (file_flags & LSSF_Scripts) {
				const auto node_script_count = Read<uint32_t>(ir, h);
				for (uint32_t j = 0; j < node_script_count; ++j) {
					const auto script_idx = Read<uint32_t>(ir, h);
					node_scripts[node_ref].push_back(script_refs[script_idx]);
				}
			}

			const auto instance_idx = Read<uint32_t>(ir, h);
			if (instance_idx != 0xffffffff) {
				node_instance[node_ref] = instance_refs[instance_idx];
				node_with_instance_to_setup.push_back(node_ref);
			}
		}

		// setup instances
		if (!(load_flags & LSSF_DoNotLoadResources)) {
			ProfilerPerfSection section("Scene::Load_binary: Setup Instances");

			for (const auto ref : node_with_instance_to_setup) {
				NodeSetupInstance(ref, deps_ir, deps_ip, resources, pipeline, LSSF_AllNodeFeatures | (load_flags & LSSF_OptionsMask), ctx.recursion_level + 1);
				NodeStartOnInstantiateAnim(ref);
			}
		}

		{
			ProfilerPerfSection section("Scene::Load_binary: Fix References");

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
	}

	if (file_flags & LSSF_Scene) {
		ProfilerPerfSection section("Scene::Load_binary: Load Scene");

		if (load_flags & LSSF_Scene) {
			if (file_flags & LSSF_Nodes) {
				if (load_flags & LSSF_Nodes) {
					const auto camera_idx = Read<uint32_t>(ir, h);
					if (camera_idx != generational_vector_list<Node_>::invalid_idx)
						current_camera = ctx.node_refs[camera_idx];
				} else {
					Skip<uint32_t>(ir, h);
				}
			}

			Read(ir, h, environment.ambient); // 16B
			Read(ir, h, environment.fog_near); // 4B
			Read(ir, h, environment.fog_far); // 4B
			Read(ir, h, environment.fog_color); // 16B

			{
				LoadProbe(environment.probe, ir, h, deps_ir, deps_ip, resources, pipeline, load_flags & LSSF_QueueTextureLoads, load_flags & LSSF_DoNotLoadResources, silent);

				std::string name;
				Read(ir, h, name);

				if (!name.empty())
					environment.brdf_map = SkipLoadOrQueueTextureLoad(
						deps_ir, deps_ip, name.c_str(), resources, load_flags & LSSF_QueueTextureLoads, load_flags & LSSF_DoNotLoadResources, silent);
			}

			Read(ir, h, canvas.clear_z);
			Read(ir, h, canvas.clear_color);
			Read(ir, h, canvas.color); // 16B
		} else {
			if (file_flags & LSSF_Nodes)
				Skip<uint32_t>(ir, h); // current camera

			Seek(ir, h, 40, SM_Current); // skip environment chunk

			SkipProbe(ir, h); // probe
			SkipString(ir, h); // brdf name

			Seek(ir, h, sizeof(bool) * 2 + 16, SM_Current);
		}
	}

	if (file_flags & LSSF_Anims) {
		ProfilerPerfSection section("Scene::Load_binary: Load Animations");

		const auto anim_chunk_size = Read<uint32_t>(ir, h);

		if (load_flags & LSSF_Anims) {
			std::map<uint32_t, AnimRef> anim_refs;

			{
				uint32_t count;
				Read(ir, h, count);

				for (uint32_t i = 0; i < count; ++i) {
					uint32_t idx;
					Read(ir, h, idx);

					Anim anim;
					LoadAnimFromBinary(ir, h, anim);

					const auto anim_ref = AddAnim(anim);
					ctx.view.anims.push_back(anim_ref);
					anim_refs[idx] = anim_ref;
				}
			}

			{
				uint32_t count;
				Read(ir, h, count);

				for (uint32_t i = 0; i < count; ++i) {
					SceneAnim scene_anim;

					uint32_t scene_anim_idx;
					Read(ir, h, scene_anim.name);
					Read(ir, h, scene_anim.t_start);
					Read(ir, h, scene_anim.t_end);
					Read(ir, h, scene_anim_idx);
					Read(ir, h, scene_anim.frame_duration);

					const auto &i_scene_anim = anim_refs.find(scene_anim_idx);
					if (i_scene_anim != std::end(anim_refs))
						scene_anim.scene_anim = i_scene_anim->second;

					uint32_t node_anim_count;
					Read(ir, h, node_anim_count);

					for (uint32_t j = 0; j < node_anim_count; ++j) {
						uint32_t node_idx, anim_idx;

						Read(ir, h, node_idx);
						Read(ir, h, anim_idx);

						const auto &i_node_ref = ctx.node_refs.find(node_idx); // remap node
						const auto &i_anim_ref = anim_refs.find(anim_idx); // remap anim

						if (i_node_ref != std::end(ctx.node_refs) && i_anim_ref != std::end(anim_refs))
							scene_anim.node_anims.push_back({i_node_ref->second, i_anim_ref->second});
					}

					const auto scene_anim_ref = AddSceneAnim(scene_anim);
					ctx.view.scene_anims.push_back(scene_anim_ref);
				}
			}
		} else {
			Seek(ir, h, anim_chunk_size, SM_Current);
		}
	}

	if (file_flags & LSSF_KeyValues) {
		ProfilerPerfSection section("Scene::Load_binary: Load Key/Values");

		uint32_t count;
		Read(ir, h, count);

		if (load_flags & LSSF_KeyValues) {
			std::string key, value;
			for (uint32_t i = 0; i < count; ++i) {
				Read(ir, h, key);
				Read(ir, h, value);
				key_values[key] = value;
			}
		} else {
			for (uint32_t i = 0; i < count; ++i) {
				SkipString(ir, h);
				SkipString(ir, h);
			}
		}
	}

	//
	{
		ProfilerPerfSection section("Scene::Load_binary: Disable Nodes to Disable");
		for (const auto &ref : nodes_to_disable)
			DisableNode(ref);
	}

	//
	{
		ProfilerPerfSection section("Scene::Load_binary: Ready Matrices");

		ReadyWorldMatrices(); // [EJ] clear transform flag for newly created transforms ONLY if it becomes a DEMONSTRATED bottleneck which it has never been yet

		// compute new transform matrices
		for (const auto &ref : transform_refs)
			ComputeTransformWorldMatrix(ref.idx);
	}

	//
	if (!silent)
		log(format("Load scene '%1' took %2 ms").arg(name).arg(time_to_ms(time_now() - t_start)));

	return true;
}

//
bool Scene::SaveNodes_binary(const Writer &iw, const Handle &h, const std::vector<NodeRef> &nodes_to_save, const PipelineResources &resources) const {
	return Save_binary(iw, h, resources, LSSF_Nodes | LSSF_Physics | LSSF_Scripts, &nodes_to_save);
}

bool Scene::LoadNodes_binary(const Reader &ir, const Handle &h, const char *name, const Reader &deps_ir, const ReadProvider &deps_ip,
	PipelineResources &resources, const PipelineInfo &pipeline, LoadSceneContext &ctx) {
	return Load_binary(ir, h, name, deps_ir, deps_ip, resources, pipeline, ctx);
}

} // namespace hg
