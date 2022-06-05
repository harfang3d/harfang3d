// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "fabgen.h"

#include "engine/assets_rw_interface.h"
#include "engine/file_format.h"
#include "engine/meta.h"
#include "engine/render_pipeline.h"
#include "engine/scene.h"

#include "foundation/data_rw_interface.h"
#include "foundation/file_rw_interface.h"
#include "foundation/format.h"
#include "foundation/log.h"
#include "foundation/pack_float.h"
#include "foundation/string.h"

#include "json/json.hpp"

#include <numeric>
#include <set>

namespace hg {

Scene::Scene() : scene_ref(new SceneRef{this}) {}

Scene::~Scene() {
	Clear();

	scene_ref->scene = nullptr;
}

//
static void _ResizeComponents(std::vector<ComponentRef> &cs) {
	ptrdiff_t i;
	for (i = cs.size() - 1; i >= 0; --i)
		if (cs[i] != invalid_gen_ref)
			break;
	cs.resize(i + 1);
}

//
void Scene::Clear() {
	// physic world
	collisions.clear();
	rigid_bodies.clear();

	node_collisions.clear();

	// instances
	node_instance.clear();
	node_instance_view.clear();

	// scene graph
	nodes.clear();

	transforms.clear();
	cameras.clear();
	objects.clear();
	lights.clear();

	// animations
	anims.clear();
	scene_anims.clear();

	play_anims.clear();

	// scripts
	scripts.clear();

	scene_scripts.clear();
	node_scripts.clear();

	//
	current_camera = {};
	transform_worlds.clear();

	environment = {};

	//
	key_values.clear();
}

size_t Scene::GarbageCollectPass() {
	size_t removed_count = 0;

#define GC_Components(LIST, IDX, ON_REMOVE)                                                                                                                    \
	{                                                                                                                                                          \
		std::vector<bool> is_refd(LIST.capacity(), false);                                                                                                     \
                                                                                                                                                               \
		for (const auto &n : nodes) {                                                                                                                          \
			const auto ref = n.components[IDX];                                                                                                                \
			if (LIST.is_valid(ref))                                                                                                                            \
				is_refd[ref.idx] = true;                                                                                                                       \
		}                                                                                                                                                      \
                                                                                                                                                               \
		for (size_t i = 0; i < is_refd.size(); ++i)                                                                                                            \
			if (!is_refd[i] && LIST.is_used(uint32_t(i))) {                                                                                                    \
				ON_REMOVE(LIST.get_ref(uint32_t(i)));                                                                                                          \
				++removed_count;                                                                                                                               \
			}                                                                                                                                                  \
	}

	GC_Components(transforms, NCI_Transform, DestroyTransform);
	GC_Components(cameras, NCI_Camera, DestroyCamera);
	GC_Components(lights, NCI_Light, DestroyLight);
	GC_Components(objects, NCI_Object, DestroyObject);
	GC_Components(rigid_bodies, NCI_RigidBody, DestroyRigidBody);

	// cleanup collisions
	{
		std::vector<bool> is_refd(collisions.capacity(), false);

		for (auto i = std::begin(node_collisions); i != std::end(node_collisions);) {
			if (nodes.is_valid(i->first)) {
				auto &node_collisions = i->second;

				for (auto &ref : node_collisions)
					if (collisions.is_valid(ref))
						is_refd[ref.idx] = true;
					else
						ref = InvalidComponentRef;

				++i;
			} else {
				i = node_collisions.erase(i);
			}
		}

		for (size_t i = 0; i < is_refd.size(); ++i)
			if (!is_refd[i] && collisions.is_used(uint32_t(i))) {
				DestroyCollision(collisions.get_ref(uint32_t(i)));
				++removed_count;
			}
	}

	// cleanup instances
	{
		std::vector<bool> is_refd(instances.capacity(), false);

		for (auto i = std::begin(node_instance); i != std::end(node_instance);) {
			if (nodes.is_valid(i->first) && instances.is_valid(i->second)) {
				is_refd[i->second.idx] = true;
				++i;
			} else {
				i = node_instance.erase(i);
			}
		}

		for (size_t i = 0; i < is_refd.size(); ++i)
			if (!is_refd[i] && instances.is_used(uint32_t(i))) {
				DestroyInstance(instances.get_ref(uint32_t(i)));
				++removed_count;
			}
	}

	// cleanup instance views
	{
		for (auto i = std::begin(node_instance_view); i != std::end(node_instance_view);) {
			bool is_node_instance_view_still_valid = true;

			const auto j = node_instance.find(i->first);

			if (j == std::end(node_instance)) {
				is_node_instance_view_still_valid = false; // node not found
			} else {
				if (!instances.is_valid(j->second))
					is_node_instance_view_still_valid = false; // node references an invalid component
			}

			if (is_node_instance_view_still_valid) {
				++i;
			} else {
				DestroyViewContent(i->second);
				i = node_instance_view.erase(i);
			}
		}
	}

	// cleanup scripts
	{
		std::vector<bool> is_refd(scripts.capacity(), false);

		for (auto i : scene_scripts)
			if (scripts.is_valid(i))
				is_refd[i.idx] = true;

		for (auto i = std::begin(node_scripts); i != std::end(node_scripts);) {
			if (nodes.is_valid(i->first)) {
				for (auto j : i->second)
					if (scripts.is_valid(j))
						is_refd[j.idx] = true;
				++i;
			} else {
				i = node_scripts.erase(i);
			}
		}

		for (size_t i = 0; i < is_refd.size(); ++i)
			if (!is_refd[i] && scripts.is_used(uint32_t(i))) {
				DestroyScript(scripts.get_ref(uint32_t(i)));
				++removed_count;
			}
	}

	// cleanup anims
	{ removed_count += GarbageCollectAnims(); }

	return removed_count;
}

size_t Scene::GarbageCollect() {
	const auto t_start = time_now();

	// n-pass to cleanup node/component
	size_t total_removed = 0, pass_count = 0;

	for (;; ++pass_count) {
		const size_t pass_removed = GarbageCollectPass();
		if (pass_removed == 0)
			break;
		total_removed += pass_removed;
	}

	// stats
	const auto t_total = time_now() - t_start;

	return total_removed;
}

//
ViewState Scene::ComputeCameraViewState(NodeRef ref, const Vec2 &aspect_ratio) const {
	if (const auto node_ = GetNode_(ref)) {
		const auto trs_ref = node_->components[NCI_Transform];

		if (IsValidTransformRef(trs_ref)) {
			if (auto cam_ = GetComponent_(cameras, node_->components[NCI_Camera])) {
				const auto &world = transform_worlds[trs_ref.idx];
				return cam_->ortho ? ComputeOrthographicViewState(world, cam_->size, cam_->zrange.znear, cam_->zrange.zfar, aspect_ratio)
								   : ComputePerspectiveViewState(world, cam_->fov, cam_->zrange.znear, cam_->zrange.zfar, aspect_ratio);
			} else {
				warn("Invalid node camera");
			}
		} else {
			warn("Invalid node transform");
		}
	} else {
		warn("Invalid node");
	}
	return {};
}

ViewState Scene::ComputeCurrentCameraViewState(const Vec2 &aspect_ratio) const { return ComputeCameraViewState(current_camera, aspect_ratio); }

void Scene::ComputeTransformWorldMatrix(uint32_t idx) {
	if (!transform_worlds_updated[idx]) {
		const auto &trs = transforms[idx];
		auto world = TransformationMat4(trs.TRS.pos, trs.TRS.rot, trs.TRS.scl);

		const auto parent_ref = GetNodeComponentRef_<NCI_Transform>(trs.parent);
		if (transforms.is_valid(parent_ref)) {
			ComputeTransformWorldMatrix(parent_ref.idx);
			world = transform_worlds[parent_ref.idx] * world;
		}

		transform_worlds_updated[idx] = true;
		transform_worlds[idx] = world;
	}
}

//
void Scene::ReadyWorldMatrices() {
	transform_worlds.resize(transforms.capacity()); // EJ vector_list can have holes, so size() does not necessarily includes the highest index in use
	transform_worlds_updated.resize(transforms.capacity());
	std::fill(std::begin(transform_worlds_updated), std::end(transform_worlds_updated), false);
}

void Scene::ComputeWorldMatrices() {
	for (auto i = transforms.first(); i != generational_vector_list<Transform_>::invalid_idx; i = transforms.next(i))
		ComputeTransformWorldMatrix(i);
}

void Scene::StorePreviousWorldMatrices() {
	std::swap(transform_worlds, previous_transform_worlds);
	std::swap(transform_worlds_updated, previous_transform_worlds_updated);
}

void Scene::FixupPreviousWorldMatrices() {
	previous_transform_worlds.resize(transform_worlds.size());
	previous_transform_worlds_updated.resize(transform_worlds_updated.size(), false);

	const size_t count = transform_worlds.size();
	for (size_t i = 0; i < count; ++i)
		if (!previous_transform_worlds_updated[i])
			previous_transform_worlds[i] == transform_worlds[i]; // ensure coherency of previous transform
}

//
void Scene::Update(time_ns dt) {
	StorePreviousWorldMatrices();
	ReadyWorldMatrices();

	UpdatePlayingAnims(dt);

	ComputeWorldMatrices();
	FixupPreviousWorldMatrices();
}

//
void Scene::GetModelDisplayLists(std::vector<ModelDisplayList> &out_opaque, std::vector<ModelDisplayList> &out_transparent,
	std::vector<SkinnedModelDisplayList> &out_opaque_skinned, std::vector<SkinnedModelDisplayList> &out_transparent_skinned,
	const PipelineResources &resources) const {
	out_opaque.clear();
	out_opaque.reserve(nodes.size());

	out_transparent.clear();
	out_transparent.reserve(nodes.size());

	out_opaque_skinned.clear();
	out_transparent_skinned.clear();

	for (const auto &node : nodes) {
		if (node.flags & (NF_Disabled | NF_InstanceDisabled))
			continue;

		const ComponentRef trs_ref = node.components[NCI_Transform];
		if (!transforms.is_valid(trs_ref))
			continue; // [EJ12102020] FIXME this is not required for a skinned object

		const Object_ *obj_ = GetComponent_(objects, node.components[NCI_Object]);
		if (!obj_)
			continue;

		const uint16_t mdl_idx = resources.models.GetValidatedRefIndex(obj_->model);
		const Model &mdl = resources.models.Get_unsafe_(mdl_idx);

		const auto total_bone_count = obj_->bones.size();

		const bool obj_has_valid_skin = total_bone_count > 0 && total_bone_count == mdl.bind_pose.size();

		for (size_t i = 0; i < mdl.lists.size(); ++i) {
			const auto mat_idx = mdl.mats[i];

			if (mat_idx < obj_->materials.size()) { // FIXME fall back to error material
				const auto mat = &obj_->materials[mat_idx];
				const auto &bones_table = mdl.lists[i].bones_table;
				__ASSERT__(bones_table.size() <= max_skinned_model_matrix_count);

				const bool is_transparent = GetMaterialBlendMode(*mat) != BM_Opaque;

				if (!obj_has_valid_skin) {
					if (is_transparent)
						out_transparent.push_back({mat, trs_ref.idx, mdl_idx, uint16_t(i)}); // worlds vector entries map 1:1 to the transform_ vector_list
					else
						out_opaque.push_back({mat, trs_ref.idx, mdl_idx, uint16_t(i)}); // worlds vector entries map 1:1 to the transform_ vector_list
				} else {
					SkinnedModelDisplayList dl;
					dl.mat = mat;

					for (int j = 0; j < bones_table.size(); ++j) {
						auto bone_idx = bones_table[j];
						__ASSERT__(bone_idx < total_bone_count);

						uint32_t mtx_idx = trs_ref.idx; // default to the node matrix in case a bone reference is invalid

						if (bone_idx < total_bone_count) {
							const NodeRef bone_ref = obj_->bones[bone_idx];

							if (nodes.is_valid(bone_ref)) {
								const auto &bone_node_ = nodes[bone_ref.idx];

								const ComponentRef bone_trs_ref = bone_node_.components[NCI_Transform];
								if (transforms.is_valid(bone_trs_ref))
									mtx_idx = bone_trs_ref.idx; // worlds vector entries map 1:1 to the transform_ vector_list
							}
						} else {
							bone_idx = 0;
						}

						dl.mtx_idxs[j] = mtx_idx;
						dl.bones_idxs[j] = bone_idx;
					}

					dl.bone_count = bones_table.size();
					dl.mdl_idx = mdl_idx;
					dl.lst_idx = uint16_t(i);

					if (is_transparent)
						out_transparent_skinned.push_back(dl);
					else
						out_opaque_skinned.push_back(dl);
				}
			}
		}
	}
}

//
std::vector<Node> Scene::GetLights() const {
	std::vector<Node> lights;
	lights.reserve(32);

	for (auto i = nodes.first(); i != generational_vector_list<Node_>::invalid_idx; i = nodes.next(i)) {
		const auto &node = nodes[i];
		if (IsValidLightRef(node.components[NCI_Light]) && IsValidTransformRef(node.components[NCI_Transform]))
			lights.push_back(GetNode(GetNodeRef(i)));
	}

	return lights;
}

//
NodesChildren Scene::BuildNodesChildren() const {
	NodesChildren nodes_children;

	for (auto i = nodes.first(); i != generational_vector_list<Node_>::invalid_idx; i = nodes.next(i))
		if (const auto trs = GetComponent_(transforms, nodes[i].components[NCI_Transform]))
			if (IsValidNodeRef(trs->parent))
				++nodes_children.node_children[trs->parent].count;

	uint32_t total_children_count = 0;
	for (const auto &e : nodes_children.node_children)
		total_children_count += e.second.count;

	nodes_children.all_children.resize(total_children_count);

	uint32_t offset = 0;
	for (auto &e : nodes_children.node_children) {
		e.second.offset = offset;
		offset += e.second.count;
		e.second.count = 0; // will be set again by the coming last step
	}

	for (auto i = nodes.first(); i != generational_vector_list<Node_>::invalid_idx; i = nodes.next(i))
		if (const auto trs = GetComponent_(transforms, nodes[i].components[NCI_Transform]))
			if (IsValidNodeRef(trs->parent)) {
				auto &e = nodes_children.node_children[trs->parent];
				nodes_children.all_children[size_t(e.offset) + e.count++] = nodes.get_ref(i);
			}

	return nodes_children;
}

std::vector<NodeRef> NodesChildren::GetChildren(NodeRef node) const {
	const auto i = node_children.find(node);
	if (i == std::end(node_children))
		return {};

	const auto &e = i->second;

	std::vector<NodeRef> children;
	children.reserve(e.count);
	for (uint32_t i = 0; i < e.count; ++i)
		children.push_back(all_children[size_t(e.offset) + i]);

	return children;
}

//
Node Scene::CreateNode(std::string name) { return {scene_ref, nodes.add_ref({std::move(name)})}; }
void Scene::DestroyNode(NodeRef ref) { nodes.remove_ref(ref); }

//
void Scene::EnableNode_(NodeRef ref, bool through_instance) {
	if (!nodes.is_valid(ref)) {
		warn("Invalid node reference");
		return;
	}

	nodes[ref.idx].flags &= through_instance ? ~NF_InstanceDisabled : ~NF_Disabled;

	// enable instance content
	if (nodes[ref.idx].flags & (NF_Disabled | NF_InstanceDisabled)) // [EJ11262019] only if fully enabled
		return;

	const auto i = node_instance_view.find(ref);
	if (i != std::end(node_instance_view))
		for (auto &instantiated_node_ref : i->second.nodes)
			EnableNode_(instantiated_node_ref, true);
}

void Scene::DisableNode_(NodeRef ref, bool through_instance) {
	if (!nodes.is_valid(ref)) {
		warn("Invalid node reference");
		return;
	}

	nodes[ref.idx].flags |= through_instance ? NF_InstanceDisabled : NF_Disabled;

	// disable instance content
	const auto i = node_instance_view.find(ref);
	if (i != std::end(node_instance_view))
		for (auto &instantiated_node_ref : i->second.nodes)
			DisableNode_(instantiated_node_ref, true);
}

void Scene::EnableNode(NodeRef ref) { EnableNode_(ref, false); }
void Scene::DisableNode(NodeRef ref) { DisableNode_(ref, false); }

//
NodeRef Scene::IsInstantiatedBy(NodeRef ref) const {
	for (const auto &i : node_instance_view)
		for (const auto &r : i.second.nodes)
			if (r == ref)
				return i.first;

	return InvalidNodeRef;
}

//
void Scene::ReserveNodes(const size_t count) { nodes.reserve(nodes.size() + count); }

//
Node Scene::GetNode(const std::string &name) const {
	for (auto i = nodes.first_ref(); i != InvalidNodeRef; i = nodes.next_ref(i))
		if (nodes[i.idx].name == name)
			return {scene_ref, i};
	return {scene_ref};
}

//
NodeRef Scene::GetNodeEx_(const std::vector<NodeRef> &refs, const std::string &path) const {
	int mode = 0;

	size_t s = 0;
	for (; s < path.size(); ++s)
		if (path[s] == ':') {
			mode = 1;
			break;
		} else if (path[s] == '/') {
			mode = 2;
			break;
		}

	const auto name = left(path, s), remainder = slice(path, s + 1);

	for (const auto &ref : refs)
		if (nodes[ref.idx].name == name) {
			if (mode == 0) {
				return ref; // look no further
			} else if (mode == 1) {
				const auto &i = node_instance_view.find(ref); // look in instance
				if (i == std::end(node_instance_view))
					return InvalidNodeRef; // not an instance

				const auto &scene_view = i->second;

				std::vector<NodeRef> roots;
				for (const auto &i : scene_view.nodes)
					if (IsChildOf(i, ref))
						roots.push_back(i);

				return GetNodeEx_(roots, remainder);
			} else if (mode == 2) {
				const auto child_refs = GetNodeChildRefs(ref); // look in children

				return GetNodeEx_(child_refs, remainder);
			}
		}

	return InvalidNodeRef;
}

Node Scene::GetNodeEx(const std::string &path) const {
	std::vector<NodeRef> roots; // root nodes = no transform or no parent
	for (auto i = nodes.first_ref(); i != InvalidNodeRef; i = nodes.next_ref(i))
		if (IsRoot(i))
			roots.push_back(i);

	return GetNode(GetNodeEx_(roots, path));
}

//
std::vector<Node> Scene::GetNodes() const {
	std::vector<Node> nodes_;
	nodes_.reserve(nodes.size());
	for (auto i = nodes.first_ref(); i != InvalidNodeRef; i = nodes.next_ref(i)) {
		const auto node_ = GetNode_(i);

		if (node_->flags & NF_Instantiated)
			continue; // do not return instantiated nodes

		nodes_.push_back({scene_ref, i});
	}
	return nodes_;
}

std::vector<Node> Scene::GetAllNodes() const {
	std::vector<Node> nodes_;
	nodes_.reserve(nodes.size());
	for (auto i = nodes.first_ref(); i != InvalidNodeRef; i = nodes.next_ref(i))
		nodes_.push_back({scene_ref, i});
	return nodes_;
}

std::vector<Node> Scene::GetNodesWithComponent(NodeComponentIdx idx) const {
	std::vector<Node> nodes_;
	for (auto i = nodes.first_ref(); i != InvalidNodeRef; i = nodes.next_ref(i)) {
		const auto node_ = &nodes[i.idx];

		if (node_->flags & NF_Instantiated)
			continue; // do not return instantiated nodes

		const auto ref = node_->components[idx];

		bool has_component{};

		switch (idx) {
			case NCI_Transform:
				has_component = transforms.is_valid(ref);
				break;
			case NCI_Object:
				has_component = objects.is_valid(ref);
				break;
			case NCI_Camera:
				has_component = cameras.is_valid(ref);
				break;
			case NCI_Light:
				has_component = lights.is_valid(ref);
				break;
		}

		if (has_component)
			nodes_.push_back({scene_ref, i});
	}
	return nodes_;
}

std::vector<Node> Scene::GetAllNodesWithComponent(NodeComponentIdx idx) const {
	std::vector<Node> nodes_;
	for (auto i = nodes.first_ref(); i != InvalidNodeRef; i = nodes.next_ref(i)) {
		const auto node_ = &nodes[i.idx];

		const auto ref = node_->components[idx];

		bool has_component{};

		switch (idx) {
			case NCI_Transform:
				has_component = transforms.is_valid(ref);
				break;
			case NCI_Object:
				has_component = objects.is_valid(ref);
				break;
			case NCI_Camera:
				has_component = cameras.is_valid(ref);
				break;
			case NCI_Light:
				has_component = lights.is_valid(ref);
				break;
		}

		if (has_component)
			nodes_.push_back({scene_ref, i});
	}
	return nodes_;
}

size_t Scene::GetNodeCount() const {
	size_t count = 0;
	for (auto i = nodes.first_ref(); i != InvalidNodeRef; i = nodes.next_ref(i)) {
		const auto node_ = GetNode_(i);

		if (node_->flags & NF_Instantiated)
			continue; // do not count instantiated nodes

		++count;
	}
	return count;
}

size_t Scene::GetAllNodeCount() const { return nodes.size(); }

//
bool Scene::IsChildOf(NodeRef ref, NodeRef parent) const {
	const auto t_ref = GetNodeComponentRef_<NCI_Transform>(ref);
	if (transforms.is_valid(t_ref))
		if (transforms[t_ref.idx].parent != parent)
			return false;
	return true;
}

//
std::vector<NodeRef> Scene::GetNodeChildRefs(NodeRef ref) const {
	std::vector<NodeRef> child_refs;
	child_refs.reserve(16);
	for (auto i = nodes.first(); i != generational_vector_list<Node_>::invalid_idx; i = nodes.next(i))
		if (const auto trs = GetComponent_(transforms, nodes[i].components[NCI_Transform]))
			if (trs->parent == ref)
				child_refs.push_back(GetNodeRef(i));
	return child_refs;
}

std::vector<Node> Scene::GetNodeChildren(NodeRef ref) const {
	const auto child_refs = GetNodeChildRefs(ref);
	return NodeRefsToNodes(*this, child_refs);
}

//
std::string Scene::GetNodeName(NodeRef ref) const {
	if (auto node_ = GetNode_(ref))
		return node_->name;

	warn("Invalid node");
	return {};
}

void Scene::SetNodeName(NodeRef ref, const std::string &v) {
	if (auto node_ = GetNode_(ref))
		node_->name = v;
	else
		warn("Invalid node");
}

//
uint32_t Scene::GetNodeFlags(NodeRef ref) const {
	if (auto node_ = GetNode_(ref))
		return node_->flags;

	warn("Invalid node");
	return 0;
}

void Scene::SetNodeFlags(NodeRef ref, uint32_t flags) {
	if (auto node_ = GetNode_(ref))
		node_->flags = flags;
	else
		warn("Invalid node");
}

//
ComponentRef Scene::GetNodeTransformRef(NodeRef ref) const { return GetNodeComponentRef_<NCI_Transform>(ref); }

void Scene::SetNodeTransform(NodeRef ref, ComponentRef cref) {
	if (auto node_ = this->GetNode_(ref))
		node_->components[NCI_Transform] = cref;
	else
		warn("Invalid node");
}

//
Mat4 Scene::GetNodeWorldMatrix(NodeRef ref) const {
	if (auto node_ = this->GetNode_(ref)) {
		const auto trs_ref = node_->components[NCI_Transform];

		if (transforms.is_valid(trs_ref)) {
			if (trs_ref.idx < transform_worlds.size())
				return transform_worlds[trs_ref.idx];

			warn("Invalid node transform index");
		} else {
			warn("Invalid node transform");
		}
	} else {
		warn("Invalid node");
	}
	return Mat4::Identity;
}

void Scene::SetNodeWorldMatrix(NodeRef ref, const Mat4 &world) {
	if (auto node_ = this->GetNode_(ref)) {
		const auto trs_ref = node_->components[NCI_Transform];

		if (transforms.is_valid(trs_ref)) {
			if (trs_ref.idx < transform_worlds.size()) {
				transform_worlds[trs_ref.idx] = world;
				transform_worlds_updated[trs_ref.idx] = true;
			} else {
				warn("Invalid node transform index");
			}
		} else {
			warn("Invalid node transform");
		}
	} else {
		warn("Invalid node");
	}
}

//
Mat4 Scene::ComputeNodeWorldMatrix(NodeRef ref) const {
	if (auto node_ = this->GetNode_(ref)) {
		const auto trs_ref = node_->components[NCI_Transform];

		if (transforms.is_valid(trs_ref)) {
			const auto &trs = transforms[trs_ref.idx];

			auto mtx = TransformationMat4(trs.TRS.pos, trs.TRS.rot, trs.TRS.scl);
			if (trs.parent != InvalidNodeRef)
				mtx = ComputeNodeWorldMatrix(trs.parent) * mtx;

			return mtx;
		} else {
			warn("Invalid node transform");
		}
	} else {
		warn("Invalid node");
	}
	return Mat4::Identity;
}

//
ComponentRef Scene::GetNodeCameraRef(NodeRef ref) const { return GetNodeComponentRef_<NCI_Camera>(ref); }

void Scene::SetNodeCamera(NodeRef ref, ComponentRef cref) {
	if (auto node_ = this->GetNode_(ref))
		node_->components[NCI_Camera] = cref;
	else
		warn("Invalid node");
}

//
ComponentRef Scene::GetNodeObjectRef(NodeRef ref) const { return GetNodeComponentRef_<NCI_Object>(ref); }

void Scene::SetNodeObject(NodeRef ref, ComponentRef cref) {
	if (auto node_ = this->GetNode_(ref))
		node_->components[NCI_Object] = cref;
	else
		warn("Invalid node");
}

//
ComponentRef Scene::GetNodeLightRef(NodeRef ref) const { return GetNodeComponentRef_<NCI_Light>(ref); }

void Scene::SetNodeLight(NodeRef ref, ComponentRef cref) {
	if (auto node_ = this->GetNode_(ref))
		node_->components[NCI_Light] = cref;
	else
		warn("Invalid node");
}

//
ComponentRef Scene::GetNodeRigidBodyRef(NodeRef ref) const { return GetNodeComponentRef_<NCI_RigidBody>(ref); }

void Scene::SetNodeRigidBody(NodeRef ref, ComponentRef cref) {
	if (auto node_ = this->GetNode_(ref))
		node_->components[NCI_RigidBody] = cref;
	else
		warn("Invalid node");
}

ComponentRef Scene::GetNodeCollisionRef(NodeRef ref, size_t idx) const {
	auto i = nodes.is_valid(ref) ? node_collisions.find(ref) : std::end(node_collisions);
	return i != std::end(node_collisions) && idx < i->second.size() ? i->second[idx] : InvalidComponentRef;
}

Collision Scene::GetNodeCollision(NodeRef ref, size_t idx) const {
	const auto cref = GetNodeCollisionRef(ref, idx);
	return cref == InvalidComponentRef ? Collision{} : Collision{scene_ref, cref};
}

void Scene::SetNodeCollision(NodeRef ref, size_t idx, const Collision &collision) {
	if (nodes.is_valid(ref)) {
		node_collisions[ref].resize(idx + 1);
		node_collisions[ref][idx] = collision.ref;
	} else {
		warn("Invalid node");
	}
}

void Scene::RemoveNodeCollision(NodeRef ref, ComponentRef cref) {
	if (nodes.is_valid(ref)) {
		auto &collisions = node_collisions[ref];

		for (size_t slot_idx = 0; slot_idx < collisions.size(); ++slot_idx)
			if (collisions[slot_idx] == cref)
				collisions[slot_idx] = invalid_gen_ref;

		_ResizeComponents(collisions);
	} else {
		warn("Invalid node");
	}
}

void Scene::RemoveNodeCollision(NodeRef ref, size_t slot_idx) {
	if (nodes.is_valid(ref)) {
		auto &collisions = node_collisions[ref];

		if (slot_idx < collisions.size())
			if (collisions[slot_idx] != invalid_gen_ref)
				collisions[slot_idx] = invalid_gen_ref;

		_ResizeComponents(collisions);
	} else {
		warn("Invalid node");
	}
}

//
RigidBody Scene::CreateRigidBody() { return {scene_ref, rigid_bodies.add_ref({})}; }

void Scene::DestroyRigidBody(ComponentRef ref) { rigid_bodies.remove_ref(ref); }

void Scene::SetRigidBodyType(ComponentRef ref, RigidBodyType type) {
	if (auto rb = GetComponent_(rigid_bodies, ref))
		rb->type = type;
	else
		warn("Invalid rigid body");
}

RigidBodyType Scene::GetRigidBodyType(ComponentRef ref) const {
	if (auto rb = GetComponent_(rigid_bodies, ref))
		return rb->type;

	warn("Invalid rigid body");
	return RBT_Dynamic;
}

float Scene::GetRigidBodyLinearDamping(ComponentRef ref) const {
	if (auto rb = GetComponent_(rigid_bodies, ref))
		return unpack_float<uint8_t>(rb->linear_damping);

	warn("Invalid rigid body");
	return 0.f;
}

void Scene::SetRigidBodyLinearDamping(ComponentRef ref, float damping) {
	if (auto rb = GetComponent_(rigid_bodies, ref))
		rb->linear_damping = pack_float<uint8_t>(damping);
	else
		warn("Invalid rigid body");
}

float Scene::GetRigidBodyAngularDamping(ComponentRef ref) const {
	if (auto rb = GetComponent_(rigid_bodies, ref))
		return unpack_float<uint8_t>(rb->angular_damping);

	warn("Invalid rigid body");
	return {};
}

void Scene::SetRigidBodyAngularDamping(ComponentRef ref, float damping) {
	if (auto rb = GetComponent_(rigid_bodies, ref))
		rb->angular_damping = pack_float<uint8_t>(damping);
	else
		warn("Invalid rigid body");
}

float Scene::GetRigidBodyRestitution(ComponentRef ref) const {
	if (const auto rb = GetComponent_(rigid_bodies, ref))
		return unpack_float<uint8_t>(rb->restitution);

	warn("Invalid rigid body");
	return 0.f;
}

void Scene::SetRigidBodyRestitution(ComponentRef ref, float restitution) {
	if (auto rb = GetComponent_(rigid_bodies, ref))
		rb->restitution = pack_float<uint8_t>(restitution);
	else
		warn("Invalid rigid body");
}

float Scene::GetRigidBodyFriction(ComponentRef ref) const {
	if (const auto rb = GetComponent_(rigid_bodies, ref))
		return unpack_float<uint8_t>(rb->friction);

	warn("Invalid rigid body");
	return 0.5f;
}

void Scene::SetRigidBodyFriction(ComponentRef ref, float friction) {
	if (auto rb = GetComponent_(rigid_bodies, ref))
		rb->friction = pack_float<uint8_t>(friction);
	else
		warn("Invalid rigid body");
}

float Scene::GetRigidBodyRollingFriction(ComponentRef ref) const {
	if (const auto rb = GetComponent_(rigid_bodies, ref))
		return unpack_float<uint8_t>(rb->rolling_friction);

	warn("Invalid rigid body");
	return 0.f;
}

void Scene::SetRigidBodyRollingFriction(ComponentRef ref, float rolling_friction) {
	if (auto rb = GetComponent_(rigid_bodies, ref))
		rb->rolling_friction = pack_float<uint8_t>(rolling_friction);
	else
		warn("Invalid rigid body");
}

//
Collision Scene::CreateCollision() { return {scene_ref, collisions.add_ref({})}; }
void Scene::DestroyCollision(ComponentRef ref) { collisions.remove_ref(ref); }

void Scene::SetCollisionType(ComponentRef ref, CollisionType type) {
	if (auto col = GetComponent_(collisions, ref))
		col->type = type;
	else
		warn("Invalid collision");
}

CollisionType Scene::GetCollisionType(ComponentRef ref) const {
	if (const auto col = GetComponent_(collisions, ref))
		return col->type;

	warn("Invalid collision");
	return CT_Sphere;
}

void Scene::SetCollisionLocalTransform(ComponentRef ref, const Mat4 &m) {
	if (auto col = GetComponent_(collisions, ref))
		Decompose(m, &col->trs.pos, &col->trs.rot, nullptr); // scale is the shape size, scaling collision shapes is NOT supported
	else
		warn("Invalid collision");
}

Mat4 Scene::GetCollisionLocalTransform(ComponentRef ref) const {
	if (const auto col = GetComponent_(collisions, ref))
		return TransformationMat4(col->trs.pos, col->trs.rot, Vec3::One); // scale is shape size, scaling collision shapes is NOT supported

	warn("Invalid collision");
	return Mat4::Identity;
}

Vec3 Scene::GetCollisionPosition(ComponentRef ref) const {
	if (const auto col = GetComponent_(collisions, ref))
		return col->trs.pos;

	warn("Invalid collision");
	return {};
}

void Scene::SetCollisionPosition(ComponentRef ref, const Vec3 &pos) {
	if (auto col = GetComponent_(collisions, ref))
		col->trs.pos = pos;
	else
		warn("Invalid collision");
}

Vec3 Scene::GetCollisionRotation(ComponentRef ref) const {
	if (const auto col = GetComponent_(collisions, ref))
		return col->trs.rot;

	warn("Invalid collision");
	return {};
}

void Scene::SetCollisionRotation(ComponentRef ref, const Vec3 &rot) {
	if (auto col = GetComponent_(collisions, ref))
		col->trs.rot = rot;
	else
		warn("Invalid collision");
}

void Scene::SetCollisionMass(ComponentRef ref, float mass) {
	if (auto col = GetComponent_(collisions, ref))
		col->mass = mass;
	else
		warn("Invalid collision");
}

float Scene::GetCollisionMass(ComponentRef ref) const {
	if (const auto col = GetComponent_(collisions, ref))
		return col->mass;

	warn("Invalid collision");
	return 0.f;
}

void Scene::SetCollisionSize(ComponentRef ref, const Vec3 &size) {
	if (auto col = GetComponent_(collisions, ref))
		col->trs.scl = size;
	else
		warn("Invalid collision");
}

Vec3 Scene::GetCollisionSize(ComponentRef ref) const {
	if (const auto col = GetComponent_(collisions, ref))
		return col->trs.scl;

	warn("Invalid collision");
	return {-1.f, -1.f, -1.f};
}

void Scene::SetCollisionRadius(ComponentRef ref, float radius) {
	if (auto col = GetComponent_(collisions, ref))
		col->trs.scl.x = radius;
	else
		warn("Invalid collision");
}

float Scene::GetCollisionRadius(ComponentRef ref) const {
	if (const auto col = GetComponent_(collisions, ref))
		return col->trs.scl.x;

	warn("Invalid collision");
	return -1.f;
}

void Scene::SetCollisionHeight(ComponentRef ref, float height) {
	if (auto col = GetComponent_(collisions, ref))
		col->trs.scl.y = height;
	else
		warn("Invalid collision");
}

float Scene::GetCollisionHeight(ComponentRef ref) const {
	if (const auto col = GetComponent_(collisions, ref))
		return col->trs.scl.y;

	warn("Invalid collision");
	return -1.f;
}

void Scene::SetCollisionResource(ComponentRef ref, const std::string &path) {
	if (auto col = GetComponent_(collisions, ref))
		col->resource_path = path;
	else
		warn("Invalid collision");
}

std::string Scene::GetCollisionResource(ComponentRef ref) {
	if (auto col = GetComponent_(collisions, ref))
		return col->resource_path;

	warn("Invalid collision");
	return {};
}

//
size_t Scene::GetNodeCollisionCount(NodeRef ref) const {
	auto i = nodes.is_valid(ref) ? node_collisions.find(ref) : std::end(node_collisions);
	return i != std::end(node_collisions) ? i->second.size() : 0;
}

//
Collision Scene::CreateSphereCollision(float radius, float mass) {
	auto col = CreateCollision();
	SetCollisionType(col.ref, CT_Sphere);
	SetCollisionRadius(col.ref, radius);
	SetCollisionMass(col.ref, mass);
	return col;
}

Collision Scene::CreateCubeCollision(float x, float y, float z, float mass) {
	auto col = CreateCollision();
	SetCollisionType(col.ref, CT_Cube);
	SetCollisionSize(col.ref, {x, y, z});
	SetCollisionMass(col.ref, mass);
	return col;
}

Collision Scene::CreateCapsuleCollision(float radius, float height, float mass) {
	auto col = CreateCollision();
	SetCollisionType(col.ref, CT_Capsule);
	SetCollisionRadius(col.ref, radius);
	SetCollisionHeight(col.ref, height);
	SetCollisionMass(col.ref, mass);
	return col;
}

Collision Scene::CreateCylinderCollision(float radius, float height, float mass) {
	auto col = CreateCollision();
	SetCollisionType(col.ref, CT_Cylinder);
	SetCollisionRadius(col.ref, radius);
	SetCollisionHeight(col.ref, height);
	SetCollisionMass(col.ref, mass);
	return col;
}

Collision Scene::CreateConeCollision(float radius, float height, float mass) {
	auto col = CreateCollision();
	SetCollisionType(col.ref, CT_Cone);
	SetCollisionRadius(col.ref, radius);
	SetCollisionHeight(col.ref, height);
	SetCollisionMass(col.ref, mass);
	return col;
}

Collision Scene::CreateMeshCollision(const std::string &path, float mass) {
	auto col = CreateCollision();
	SetCollisionType(col.ref, CT_Mesh);
	SetCollisionResource(col.ref, path);
	SetCollisionMass(col.ref, mass);
	return col;
}

Collision Scene::CreateMeshConvexCollision(const std::string &path, float mass) {
	auto col = CreateCollision();
	SetCollisionType(col.ref, CT_MeshConvex);
	SetCollisionResource(col.ref, path);
	SetCollisionMass(col.ref, mass);
	return col;
}

//
Instance Scene::CreateInstance() { return {scene_ref, instances.add_ref({})}; }
void Scene::DestroyInstance(ComponentRef ref) { instances.remove_ref(ref); }

void Scene::SetInstancePath(ComponentRef ref, const std::string &path) {
	if (instances.is_valid(ref))
		instances[ref.idx].name = path;
	else
		warn("Invalid instance");
}

std::string Scene::GetInstancePath(ComponentRef ref) const {
	if (instances.is_valid(ref))
		return instances[ref.idx].name;

	warn("Invalid instance");
	return {};
}

void Scene::SetOnInstantiateAnim(ComponentRef ref, const std::string &anim) {
	if (instances.is_valid(ref))
		instances[ref.idx].anim = anim;
	else
		warn("Invalid instance");
}

void Scene::SetOnInstantiateAnimLoopMode(ComponentRef ref, AnimLoopMode loop_mode) {
	if (instances.is_valid(ref))
		instances[ref.idx].loop_mode = loop_mode;
	else
		warn("Invalid instance");
}

void Scene::ClearOnInstantiateAnim(ComponentRef ref) {
	if (instances.is_valid(ref))
		instances[ref.idx].anim.clear();
	else
		warn("Invalid instance");
}

std::string Scene::GetOnInstantiateAnim(ComponentRef ref) {
	if (instances.is_valid(ref))
		return instances[ref.idx].anim;

	warn("Invalid instance");
	return {};
}

AnimLoopMode Scene::GetOnInstantiateAnimLoopMode(ComponentRef ref) {
	if (instances.is_valid(ref))
		return instances[ref.idx].loop_mode;

	warn("Invalid instance");
	return {};
}

ScenePlayAnimRef Scene::GetOnInstantiatePlayAnimRef(ComponentRef ref) {
	if (instances.is_valid(ref))
		return instances[ref.idx].play_anim_ref;

	warn("Invalid instance");
	return InvalidScenePlayAnimRef;
}

Instance Scene::GetNodeInstance(NodeRef ref) const {
	const auto cref = GetNodeInstanceRef(ref);
	if (cref != InvalidComponentRef)
		return {scene_ref, cref};

	warn("Invalid node instance");
	return {};
}

ComponentRef Scene::GetNodeInstanceRef(NodeRef ref) const {
	const auto i = node_instance.find(ref);
	if (i != std::end(node_instance))
		return i->second;
	return InvalidComponentRef;
}

void Scene::SetNodeInstance(NodeRef ref, ComponentRef cref) {
	if (cref == InvalidComponentRef)
		node_instance.erase(ref);
	else
		node_instance[ref] = cref;
}

void Scene::DestroyViewContent(const SceneView &view) {
	for (const auto &node : view.nodes)
		DestroyNode(node);
	for (const auto &anim : view.anims)
		DestroyAnim(anim);
	for (const auto &scene_anim : view.scene_anims)
		DestroySceneAnim(scene_anim);
}

void Scene::NodeDestroyInstance(NodeRef ref) {
	NodeStopOnInstantiateAnim(ref);

	const auto i = node_instance_view.find(ref);

	if (i != std::end(node_instance_view)) {
		DestroyViewContent(i->second);
		node_instance_view.erase(i);
	} else {
		warn("Invalid node instance view");
	}
}

static bool LoadScene(const Reader &ir, const Handle &h, const char *name, Scene &scene, const Reader &deps_ir, const ReadProvider &deps_ip,
	PipelineResources &resources, const PipelineInfo &pipeline, LoadSceneContext &ctx, uint32_t flags);

bool Scene::NodeSetupInstance(
	NodeRef ref, const Reader &ir, const ReadProvider &ip, PipelineResources &resources, const PipelineInfo &pipeline, uint32_t flags, int recursion_level) {
	if (recursion_level > 4)
		return true;

	const auto i = node_instance.find(ref);
	if (i == std::end(node_instance))
		return false;

	const auto host_is_enabled = IsNodeEnabled(ref);

	if (instances.is_valid(i->second)) {
		LoadSceneContext ctx = {recursion_level};

		{
			const auto &i_ = instances[i->second.idx];
			if (!LoadScene(ir, ScopedReadHandle(ip, i_.name.c_str(), flags & LSSF_Silent), i_.name.c_str(), *this, ir, ip, resources, pipeline, ctx, flags))
				return false;
		}

		auto &i_ = instances[i->second.idx]; // [EJ12102019] LoadScene might reallocate instances buffer so fetch i_ anew

		for (auto node : ctx.view.nodes) {
			auto &n = nodes[node.idx];

			n.flags |= NF_Instantiated; // flag as instantiated

			if (!host_is_enabled)
				n.flags |= NF_InstanceDisabled; // flag as disabled through host

			if (auto trs = GetComponent_(transforms, n.components[NCI_Transform]))
				if (trs->parent == InvalidNodeRef)
					trs->parent = ref; // parent node to the instance node
		}

		for (auto &anim : ctx.view.anims)
			anims[anim.idx].flags |= AF_Instantiated; // flag as instantiated

		for (auto scene_anim : ctx.view.scene_anims)
			scene_anims[scene_anim.idx].flags |= SAF_Instantiated; // flag as instantiated

		node_instance_view[ref] = std::move(ctx.view);
	}
	return true;
}

ScenePlayAnimRef Scene::NodeStartOnInstantiateAnim(NodeRef ref) {
	NodeStopOnInstantiateAnim(ref);

	const auto i = node_instance.find(ref);

	if (i == std::end(node_instance))
		return InvalidScenePlayAnimRef; // no instance on node

	if (!instances.is_valid(i->second))
		return InvalidScenePlayAnimRef; // invalid instance ref

	auto &i_ = instances[i->second.idx];

	if (i_.anim.empty())
		return InvalidScenePlayAnimRef; // no anim to play on instantiation

	const auto i_view = node_instance_view.find(ref);

	if (i_view == std::end(node_instance_view))
		return InvalidScenePlayAnimRef; // no instance view

	const auto anim_ref = i_view->second.GetSceneAnim(*this, i_.anim);
	return i_.play_anim_ref = PlayAnim(anim_ref, i_.loop_mode);
}

void Scene::NodeStopOnInstantiateAnim(NodeRef ref) {
	const auto i = node_instance.find(ref);

	if (i == std::end(node_instance))
		return; // no instance on node

	if (!instances.is_valid(i->second))
		return; // invalid instance ref

	auto &i_ = instances[i->second.idx];

	StopAnim(i_.play_anim_ref);
	i_.play_anim_ref = InvalidScenePlayAnimRef;
}

bool Scene::NodeSetupInstanceFromFile(NodeRef ref, PipelineResources &resources, const PipelineInfo &pipeline, uint32_t flags, int recursion_level) {
	return NodeSetupInstance(ref, g_file_reader, g_file_read_provider, resources, pipeline, flags, recursion_level);
}

bool Scene::NodeSetupInstanceFromAssets(NodeRef ref, PipelineResources &resources, const PipelineInfo &pipeline, uint32_t flags, int recursion_level) {
	return NodeSetupInstance(ref, g_assets_reader, g_assets_read_provider, resources, pipeline, flags, recursion_level);
}

const SceneView &Scene::GetNodeInstanceSceneView(NodeRef ref) const {
	static SceneView dummy_view;
	const auto i = node_instance_view.find(ref);
	if (i == std::end(node_instance_view)) {
		warn(format("No instance scene view on node (%1:%2)").arg(ref.idx).arg(ref.gen).c_str());
		return dummy_view;
	}
	return i->second;
}

Instance Scene::CreateInstance(const std::string &path) {
	auto instance = CreateInstance();
	instance.SetPath(path);
	return instance;
}

void Scene::NodeMoveInstance(NodeRef from, NodeRef to) {
	if (!nodes.is_valid(from) || !nodes.is_valid(to))
		return;

	NodeDestroyInstance(to); // drop current target instance scene view if any
	node_instance.erase(to); // drop current target instance component if any

	const bool tgt_disabled = nodes[to.idx].flags & NF_Disabled;

	const auto &i = node_instance_view.find(from);

	if (i != std::end(node_instance_view)) {
		for (auto n : i->second.nodes) {
			// re-parent instantiated nodes to the target node
			const auto trsf_ref = GetNodeComponentRef_<NCI_Transform>(n);
			if (trsf_ref != InvalidComponentRef)
				if (transforms[trsf_ref.idx].parent == from)
					transforms[trsf_ref.idx].parent = to;

			// update disable flag
			tgt_disabled ? DisableNode_(n, true) : EnableNode_(n, true);
		}
		node_instance_view[to] = std::move(i->second); // transfer instance view
	}
	node_instance_view.erase(i); // drop from source

	if (node_instance.find(from) != std::end(node_instance))
		node_instance[to] = node_instance[from];
	node_instance.erase(from); // drop from source
}

//
Node CreateCamera(Scene &scene, const Mat4 &mtx, float znear, float zfar, float fov) {
	Node node = scene.CreateNode();
	node.SetName("Camera");
	node.SetTransform(scene.CreateTransform(mtx));
	node.SetCamera(scene.CreateCamera(znear, zfar, fov));
	return node;
}

Node CreateOrthographicCamera(Scene &scene, const Mat4 &mtx, float znear, float zfar, float size) {
	Node node = scene.CreateNode();
	node.SetName("Camera");
	node.SetTransform(scene.CreateTransform(mtx));
	node.SetCamera(scene.CreateOrthographicCamera(znear, zfar, size));
	return node;
}

//
Node CreatePointLight(Scene &scene, const Mat4 &mtx, float radius, const Color &diffuse, float diffuse_intensity, const Color &specular,
	float specular_intensity, float priority, LightShadowType shadow_type, float shadow_bias) {
	Node node = scene.CreateNode();
	node.SetName("Point Light");
	node.SetTransform(scene.CreateTransform(mtx));
	node.SetLight(scene.CreatePointLight(radius, diffuse, diffuse_intensity, specular, specular_intensity, priority, shadow_type, shadow_bias));
	return node;
}

Node CreateSpotLight(Scene &scene, const Mat4 &mtx, float radius, float inner_angle, float outer_angle, const Color &diffuse, float diffuse_intensity,
	const Color &specular, float specular_intensity, float priority, LightShadowType shadow_type, float shadow_bias) {
	Node node = scene.CreateNode();
	node.SetName("Spot Light");
	node.SetTransform(scene.CreateTransform(mtx));
	node.SetLight(
		scene.CreateSpotLight(radius, inner_angle, outer_angle, diffuse, diffuse_intensity, specular, specular_intensity, priority, shadow_type, shadow_bias));
	return node;
}

Node CreateLinearLight(Scene &scene, const Mat4 &mtx, const Color &diffuse, float diffuse_intensity, const Color &specular, float specular_intensity,
	float priority, LightShadowType shadow_type, float shadow_bias, const Vec4 &pssm_split) {
	Node node = scene.CreateNode();
	node.SetName("Linear Light");
	node.SetTransform(scene.CreateTransform(mtx));
	node.SetLight(scene.CreateLinearLight(diffuse, diffuse_intensity, specular, specular_intensity, priority, shadow_type, shadow_bias, pssm_split));
	return node;
}

Node CreateInstance(Scene &scene, const Mat4 &mtx, const std::string &name, const Reader &ir, const ReadProvider &ip, PipelineResources &resources,
	const PipelineInfo &pipeline, bool &success, uint32_t flags) {
	Node node = scene.CreateNode();
	node.SetName(name);
	node.SetTransform(scene.CreateTransform(mtx));
	node.SetInstance(scene.CreateInstance(name));
	success = node.SetupInstance(ir, ip, resources, pipeline, flags);
	return node;
}

Node CreateInstanceFromFile(
	Scene &scene, const Mat4 &mtx, const std::string &name, PipelineResources &resources, const PipelineInfo &pipeline, bool &success, uint32_t flags) {
	return CreateInstance(scene, mtx, name, g_file_reader, g_file_read_provider, resources, pipeline, success, flags);
}

Node CreateInstanceFromAssets(
	Scene &scene, const Mat4 &mtx, const std::string &name, PipelineResources &resources, const PipelineInfo &pipeline, bool &success, uint32_t flags) {
	return CreateInstance(scene, mtx, name, g_assets_reader, g_assets_read_provider, resources, pipeline, success, flags);
}

//
Script Scene::CreateScript() { return {scene_ref, scripts.add_ref({})}; }
Script Scene::CreateScript(const std::string &path) { return {scene_ref, scripts.add_ref({path})}; }

void Scene::DestroyScript(ComponentRef ref) { scripts.remove_ref(ref); }

void Scene::SetScriptPath(ComponentRef ref, const std::string &path) {
	if (auto s = GetComponent_(scripts, ref))
		s->path = path;
	else
		warn("Invalid script");
}

std::string Scene::GetScriptPath(ComponentRef ref) const {
	if (const auto s = GetComponent_(scripts, ref))
		return s->path;

	warn("Invalid script");
	return {};
}

//
bool Scene::ScriptHasParam(ComponentRef ref, const std::string &name) const {
	if (const auto s = GetComponent_(scripts, ref))
		return s->params.find(name) != std::end(s->params);

	warn("Invalid script");
	return false;
}

bool Scene::SetScriptParam(ComponentRef ref, const std::string &name, ScriptParam param) {
	if (const auto s = GetComponent_(scripts, ref)) {
		s->params[name] = std::move(param);
		return true;
	}

	warn("Invalid script");
	return false;
}

ScriptParam Scene::GetScriptParam(ComponentRef ref, const std::string &name) const {
	if (const auto s = GetComponent_(scripts, ref)) {
		const auto i = s->params.find(name);
		if (i != std::end(s->params))
			return i->second;
		else
			warn(format("Invalid script parameter '%1'").arg(name).c_str());
	} else {
		warn("Invalid script");
	}
	return {SPT_Null};
}

static std::map<std::string, ScriptParam> _empty_script_params;

const std::map<std::string, ScriptParam> &Scene::GetScriptParams(ComponentRef ref) const {
	if (const auto s = GetComponent_(scripts, ref))
		return s->params;

	warn("Invalid script");
	return _empty_script_params;
}

//
size_t Scene::GetNodeScriptCount(NodeRef ref) const {
	if (!nodes.is_valid(ref)) {
		warn("Invalid node");
		return 0;
	}

	const auto i = node_scripts.find(ref);
	return i != std::end(node_scripts) ? i->second.size() : 0;
}

Script Scene::GetNodeScript(NodeRef ref, size_t idx) const {
	const auto cref = GetNodeScriptRef(ref, idx);
	return cref == InvalidComponentRef ? Script{} : Script{scene_ref, cref};
}

ComponentRef Scene::GetNodeScriptRef(NodeRef ref, size_t idx) const {
	auto i = nodes.is_valid(ref) ? node_scripts.find(ref) : std::end(node_scripts);
	return i != std::end(node_scripts) && idx < i->second.size() ? i->second[idx] : InvalidComponentRef;
}

void Scene::SetNodeScript(NodeRef ref, size_t idx, const Script &script) {
	if (nodes.is_valid(ref)) {
		node_scripts[ref].resize(idx + 1);
		node_scripts[ref][idx] = script.ref;
	} else {
		warn("Invalid node");
	}
}

void Scene::RemoveNodeScript(NodeRef ref, ComponentRef cref) {
	if (nodes.is_valid(ref)) {
		auto &scripts = node_scripts[ref];

		for (size_t slot_idx = 0; slot_idx < scripts.size(); ++slot_idx)
			if (scripts[slot_idx] == cref)
				scripts[slot_idx] = invalid_gen_ref;

		_ResizeComponents(scripts);
	} else {
		warn("Invalid node");
	}
}

void Scene::RemoveNodeScript(NodeRef ref, size_t slot_idx) {
	if (nodes.is_valid(ref)) {
		auto &scripts = node_scripts[ref];

		if (slot_idx < scripts.size())
			if (scripts[slot_idx] != invalid_gen_ref)
				scripts[slot_idx] = invalid_gen_ref;

		_ResizeComponents(scripts);
	} else {
		warn("Invalid node");
	}
}

std::vector<Script> Scene::GetScripts() const {
	std::vector<Script> out;
	out.reserve(scripts.size());
	for (auto ref = scripts.first_ref(); scripts.is_valid(ref); ref = scripts.next_ref(ref))
		out.push_back({scene_ref, ref});
	return out;
}

std::vector<ComponentRef> Scene::GetScriptRefs() const {
	std::vector<ComponentRef> out;
	out.reserve(scripts.size());
	for (auto ref = scripts.first_ref(); scripts.is_valid(ref); ref = scripts.next_ref(ref))
		out.push_back(ref);
	return out;
}

size_t Scene::GetScriptCount() const { return scene_scripts.size(); }

void Scene::SetScript(size_t slot_idx, const Script &script) {
	scene_scripts.resize(slot_idx + 1);
	scene_scripts[slot_idx] = script.ref;
}

Script Scene::GetScript(size_t slot_idx) const { return slot_idx < scene_scripts.size() ? Script{scene_ref, scene_scripts[slot_idx]} : Script{}; }

//
Node CreateObject(Scene &scene, const Mat4 &mtx, const ModelRef &model_ref, std::vector<Material> materials) {
	Node node = scene.CreateNode();
	node.SetName("Object");
	node.SetTransform(scene.CreateTransform(mtx));
	node.SetObject(scene.CreateObject(model_ref, materials));
	return node;
}

//
Node CreatePhysicSphere(Scene &scene, float radius, const Mat4 &mtx, const ModelRef &model_ref, std::vector<Material> materials, float mass) {
	auto node = CreateObject(scene, mtx, model_ref, materials);
	node.SetName("Physic Sphere");
	node.SetRigidBody(scene.CreateRigidBody());
	node.SetCollision(0, scene.CreateSphereCollision(radius, mass));
	return node;
}

Node CreatePhysicCube(Scene &scene, const Vec3 &size, const Mat4 &mtx, const ModelRef &model_ref, std::vector<Material> materials, float mass) {
	auto node = CreateObject(scene, mtx, model_ref, materials);
	node.SetName("Physic Cube");
	node.SetRigidBody(scene.CreateRigidBody());
	node.SetCollision(0, scene.CreateCubeCollision(size.x, size.y, size.z, mass));
	return node;
}

//
Node CreateScript(Scene &scene) {
	Node node = scene.CreateNode();
	node.SetName("Script");
	node.SetScript(0, scene.CreateScript());
	return node;
}

Node CreateScript(Scene &scene, const std::string &path) {
	Node node = scene.CreateNode();
	node.SetName("Script");
	node.SetScript(0, scene.CreateScript(path));
	return node;
}

//
void DumpSceneMemoryFootprint() {
	log(format("sizeof(Scene): %1").arg(sizeof(Scene)).c_str());

	log("");
	log(format("sizeof(Scene::Node_): %1").arg(sizeof(Scene::Node_)).c_str());
	log(format("sizeof(Scene::Transform_): %1").arg(sizeof(Scene::Transform_)).c_str());
	log(format("sizeof(Scene::Camera_): %1").arg(sizeof(Scene::Camera_)).c_str());
	log(format("sizeof(Scene::Object_): %1").arg(sizeof(Scene::Object_)).c_str());
	log(format("sizeof(Scene::Light_): %1").arg(sizeof(Scene::Light_)).c_str());
	log(format("sizeof(Scene::RigidBody_): %1").arg(sizeof(Scene::RigidBody_)).c_str());
	log(format("sizeof(Scene::Collision_): %1").arg(sizeof(Scene::Collision_)).c_str());
	log(format("sizeof(Scene::Instance_): %1").arg(sizeof(Scene::Instance_)).c_str());

	log("");
	log(format("sizeof(BoundToNodeAnim): %1").arg(sizeof(BoundToNodeAnim)).c_str());
	log(format("sizeof(BoundToSceneAnim): %1").arg(sizeof(BoundToSceneAnim)).c_str());
	log(format("sizeof(SceneBoundAnim): %1").arg(sizeof(SceneBoundAnim)).c_str());

	log("");
	log(format("sizeof(Node): %1").arg(sizeof(Node)).c_str());
	log(format("sizeof(Transform): %1").arg(sizeof(Transform)).c_str());
	log(format("sizeof(Camera): %1").arg(sizeof(Camera)).c_str());
	log(format("sizeof(Object): %1").arg(sizeof(Object)).c_str());
	log(format("sizeof(Light): %1").arg(sizeof(Light)).c_str());
	log(format("sizeof(RigidBody): %1").arg(sizeof(RigidBody)).c_str());
	log(format("sizeof(Collision): %1").arg(sizeof(Collision)).c_str());
	log(format("sizeof(Instance): %1").arg(sizeof(Instance)).c_str());
}

//
bool SaveSceneBinaryToFile(const char *path, const Scene &scene, const PipelineResources &resources, uint32_t flags, bool debug) {
#ifdef ENABLE_BINARY_DEBUG_HANDLE
	ScopedWriteHandle handle(g_file_write_provider, path, debug);
#else
	ScopedWriteHandle handle(g_file_write_provider, path);
#endif
	return scene.Save_binary(g_file_writer, handle, resources, flags);
}

bool SaveSceneBinaryToData(Data &data, const Scene &scene, const PipelineResources &resources, uint32_t flags, bool debug) {
#ifdef ENABLE_BINARY_DEBUG_HANDLE
	DataWriteHandle handle(data, debug);
#else
	DataWriteHandle handle(data);
#endif
	return scene.Save_binary(g_data_writer, handle, resources, flags);
}

//
bool SaveSceneJsonToFile(const char *path, const Scene &scene, const PipelineResources &resources, uint32_t flags) {
	json js;
	if (!scene.Save_json(js, resources, flags))
		return false;
	return SaveJsonToFile(js, path);
}

bool SaveSceneJsonToData(Data &data, const Scene &scene, const PipelineResources &resources, uint32_t flags) {
	json js;
	if (!scene.Save_json(js, resources, flags))
		return false;
	const auto mpack = json::to_msgpack(js);
	const auto size = mpack.size();
	return data.Write(mpack.data(), size) == size;
}

//
static bool IsBinaryScene(const Reader &ir, const Handle &h) {
	if (ir.size(h) < 4)
		return false;
	const auto p = ir.tell(h);
	ir.seek(h, 0, SM_Start);
	const bool is_hg_binary = Read<uint32_t>(ir, h) == HarfangMagic;
	ir.seek(h, p, SM_Start);
	return is_hg_binary;
}

bool IsBinarySceneFile(const char *path) { return IsBinaryScene(g_file_reader, ScopedReadHandle(g_file_read_provider, path)); }
bool IsBinarySceneAsset(const char *name) { return IsBinaryScene(g_assets_reader, ScopedReadHandle(g_assets_read_provider, name)); }

static bool LoadSceneJson(const Reader &ir, const Handle &h, const char *name, Scene &scene, const Reader &deps_ir, const ReadProvider &deps_ip,
	PipelineResources &resources, const PipelineInfo &pipeline, LoadSceneContext &ctx, uint32_t flags) {
	const auto js = LoadJson(ir, h);
	return scene.Load_json(js, name, deps_ir, deps_ip, resources, pipeline, ctx, flags);
}

//
bool LoadSceneJsonFromFile(const char *path, Scene &scene, PipelineResources &resources, const PipelineInfo &pipeline, LoadSceneContext &ctx, uint32_t flags) {
	return LoadSceneJson(
		g_file_reader, ScopedReadHandle(g_file_read_provider, path), path, scene, g_file_reader, g_file_read_provider, resources, pipeline, ctx, flags);
}

bool LoadSceneJsonFromAssets(
	const char *name, Scene &scene, PipelineResources &resources, const PipelineInfo &pipeline, LoadSceneContext &ctx, uint32_t flags) {
	return LoadSceneJson(
		g_assets_reader, ScopedReadHandle(g_assets_read_provider, name), name, scene, g_assets_reader, g_assets_read_provider, resources, pipeline, ctx, flags);
}

bool LoadSceneJsonFromData(const Data &data, const char *name, Scene &scene, const Reader &deps_ir, const ReadProvider &deps_ip, PipelineResources &resources,
	const PipelineInfo &pipeline, LoadSceneContext &ctx, uint32_t flags) {
	const auto js = json::from_msgpack(data.GetData(), data.GetData() + data.GetSize());
	return scene.Load_json(js, name, deps_ir, deps_ip, resources, pipeline, ctx, flags);
}

//
bool LoadSceneBinaryFromFile(
	const char *path, Scene &scene, PipelineResources &resources, const PipelineInfo &pipeline, LoadSceneContext &ctx, uint32_t flags, bool debug) {
#ifdef ENABLE_BINARY_DEBUG_HANDLE
	ScopedReadHandle handle(g_file_read_provider, path, debug);
#else
	ScopedReadHandle handle(g_file_read_provider, path);
#endif
	return scene.Load_binary(g_file_reader, handle, path, g_file_reader, g_file_read_provider, resources, pipeline, ctx, flags);
}

bool LoadSceneBinaryFromAssets(
	const char *name, Scene &scene, PipelineResources &resources, const PipelineInfo &pipeline, LoadSceneContext &ctx, uint32_t flags, bool debug) {
#ifdef ENABLE_BINARY_DEBUG_HANDLE
	ScopedReadHandle handle(g_assets_read_provider, name, debug);
#else
	ScopedReadHandle handle(g_assets_read_provider, name);
#endif
	return scene.Load_binary(g_assets_reader, handle, name, g_assets_reader, g_assets_read_provider, resources, pipeline, ctx, flags);
}

bool LoadSceneBinaryFromData(const Data &data, const char *name, Scene &scene, const Reader &deps_ir, const ReadProvider &deps_ip, PipelineResources &resources,
	const PipelineInfo &pipeline, LoadSceneContext &ctx, uint32_t flags, bool debug) {
#ifdef ENABLE_BINARY_DEBUG_HANDLE
	DataReadHandle handle(data, debug);
#else
	DataReadHandle handle(data);
#endif
	return scene.Load_binary(g_data_reader, handle, name, deps_ir, deps_ip, resources, pipeline, ctx, flags);
}

//
static bool LoadScene(const Reader &ir, const Handle &h, const char *name, Scene &scene, const Reader &deps_ir, const ReadProvider &deps_ip,
	PipelineResources &resources, const PipelineInfo &pipeline, LoadSceneContext &ctx, uint32_t flags) {

	if (!ir.is_valid(h))
		return false;

	return IsBinaryScene(ir, h) ? scene.Load_binary(ir, h, name, deps_ir, deps_ip, resources, pipeline, ctx, flags)
								: LoadSceneJson(ir, h, name, scene, deps_ir, deps_ip, resources, pipeline, ctx, flags);
}

bool LoadSceneFromFile(const char *path, Scene &scene, PipelineResources &resources, const PipelineInfo &pipeline, LoadSceneContext &ctx, uint32_t flags) {
	return LoadScene(g_file_reader, ScopedReadHandle(g_file_read_provider, path, flags & LSSF_Silent), path, scene, g_file_reader, g_file_read_provider,
		resources, pipeline, ctx, flags);
}

bool LoadSceneFromAssets(const char *name, Scene &scene, PipelineResources &resources, const PipelineInfo &pipeline, LoadSceneContext &ctx, uint32_t flags) {
	return LoadScene(g_assets_reader, ScopedReadHandle(g_assets_read_provider, name, flags & LSSF_Silent), name, scene, g_assets_reader, g_assets_read_provider,
		resources, pipeline, ctx, flags);
}

//
std::vector<NodeRef> NodesToNodeRefs(const std::vector<Node> &nodes) {
	std::vector<NodeRef> refs(nodes.size());
	for (size_t i = 0; i < nodes.size(); ++i)
		refs[i] = nodes[i].ref;
	return refs;
}

std::vector<Node> NodeRefsToNodes(const Scene &scene, const std::vector<NodeRef> &refs) {
	std::vector<Node> nodes(refs.size());
	for (size_t i = 0; i < refs.size(); ++i)
		nodes[i] = scene.GetNode(refs[i]);
	return nodes;
}

//
std::vector<NodeRef> DuplicateNodes(Scene &scene, const std::vector<NodeRef> &nodes, const Reader &deps_ir, const ReadProvider &deps_ip,
	PipelineResources &resources, const PipelineInfo &pipeline) {
	Data data;

	if (!scene.SaveNodes_binary(g_data_writer, DataWriteHandle(data), nodes, resources)) {
		warn("Failed to duplicate nodes, an error occurred while saving the node selection");
		return {};
	}

	data.Rewind();

	LoadSceneContext ctx;
	if (!scene.LoadNodes_binary(g_data_reader, DataReadHandle(data), "DuplicateNodes", deps_ir, deps_ip, resources, pipeline, ctx)) {
		warn("Failed to duplicate nodes, an error occurred while loading the node selection");
		return {};
	}

	return ctx.view.nodes;
}

static std::vector<NodeRef> GetNodeAndchildren(const Scene &scene, const std::vector<NodeRef> &refs) {
	std::set<NodeRef> out;

	for (auto ref : refs) {
		out.insert(ref);
		const auto child_refs = scene.GetNodeChildRefs(ref);
		for (auto child_ref : child_refs)
			out.insert(child_ref);
	}

	return {std::begin(out), std::end(out)};
}

//
std::vector<Node> DuplicateNodesFromFile(Scene &scene, const std::vector<Node> &nodes, PipelineResources &resources, const PipelineInfo &pipeline) {
	return NodeRefsToNodes(scene, DuplicateNodes(scene, NodesToNodeRefs(nodes), g_file_reader, g_file_read_provider, resources, pipeline));
}

std::vector<Node> DuplicateNodesFromAssets(Scene &scene, const std::vector<Node> &nodes, PipelineResources &resources, const PipelineInfo &pipeline) {
	return NodeRefsToNodes(scene, DuplicateNodes(scene, NodesToNodeRefs(nodes), g_assets_reader, g_assets_read_provider, resources, pipeline));
}

std::vector<Node> DuplicateNodesAndChildrenFromFile(Scene &scene, const std::vector<Node> &nodes, PipelineResources &resources, const PipelineInfo &pipeline) {
	return NodeRefsToNodes(
		scene, DuplicateNodes(scene, GetNodeAndchildren(scene, NodesToNodeRefs(nodes)), g_file_reader, g_file_read_provider, resources, pipeline));
}

std::vector<Node> DuplicateNodesAndChildrenFromAssets(
	Scene &scene, const std::vector<Node> &nodes, PipelineResources &resources, const PipelineInfo &pipeline) {
	return NodeRefsToNodes(
		scene, DuplicateNodes(scene, GetNodeAndchildren(scene, NodesToNodeRefs(nodes)), g_assets_reader, g_assets_read_provider, resources, pipeline));
}

Node DuplicateNodeFromFile(Scene &scene, Node node, PipelineResources &resources, const PipelineInfo &pipeline) {
	const auto out = DuplicateNodesFromFile(scene, {node}, resources, pipeline);
	return out.empty() ? Node() : out[0];
}

Node DuplicateNodeFromAssets(Scene &scene, Node node, PipelineResources &resources, const PipelineInfo &pipeline) {
	const auto out = DuplicateNodesFromAssets(scene, {node}, resources, pipeline);
	return out.empty() ? Node() : out[0];
}

std::vector<Node> DuplicateNodeAndChildrenFromFile(Scene &scene, Node node, PipelineResources &resources, const PipelineInfo &pipeline) {
	return DuplicateNodesAndChildrenFromFile(scene, {node}, resources, pipeline);
}

std::vector<Node> DuplicateNodeAndChildrenFromAssets(Scene &scene, Node node, PipelineResources &resources, const PipelineInfo &pipeline) {
	return DuplicateNodesAndChildrenFromAssets(scene, {node}, resources, pipeline);
}

//
std::vector<Node> SceneView::GetNodes(const Scene &scene) const {
	std::vector<Node> _nodes;
	_nodes.reserve(nodes.size());
	for (const auto &ref : nodes)
		_nodes.push_back(scene.GetNode(ref));
	return _nodes;
}

Node SceneView::GetNode(const Scene &scene, const std::string &name) const {
	for (const auto &ref : nodes)
		if (scene.GetNodeName(ref) == name)
			return scene.GetNode(ref);
	return {};
}

SceneAnimRef SceneView::GetSceneAnim(const Scene &scene, const std::string &name) const {
	for (const auto &ref : scene_anims)
		if (const auto anim = scene.GetSceneAnim(ref))
			if (anim->name == name)
				return ref;
	return InvalidSceneAnimRef;
}

//
std::vector<std::string> Scene::GetPlayingAnimNames() const {
	std::vector<std::string> names;
	for (auto i : play_anims)
		names.push_back(i.name);
	return names;
}

std::vector<ScenePlayAnimRef> Scene::GetPlayingAnimRefs() const {
	std::vector<ScenePlayAnimRef> refs;
	for (auto ref = play_anims.first_ref(); play_anims.is_valid(ref); ref = play_anims.next_ref(ref))
		refs.push_back(ref);
	return refs;
}

//
void Scene::StopAllAnims() { play_anims.clear(); }
bool Scene::GetMinMax(const PipelineResources &resources, MinMax &minmax) const { return GetNodesMinMax(GetNodesWithComponent(NCI_Object), resources, minmax); }

//
const AnimRef InvalidAnimRef;

time_ns UnspecifiedAnimTime = std::numeric_limits<time_ns>::max();

AnimRef Scene::AddAnim(Anim anim) { return anims.add_ref(std::move(anim)); }

std::vector<AnimRef> Scene::GetAnims() const {
	std::vector<AnimRef> refs;
	refs.reserve(anims.size());
	for (auto ref = anims.first_ref(); ref != InvalidAnimRef; ref = anims.next_ref(ref))
		refs.push_back(ref);
	return refs;
}

Anim *Scene::GetAnim(AnimRef ref) { return anims.is_valid(ref) ? &anims[ref.idx] : nullptr; }
const Anim *Scene::GetAnim(AnimRef ref) const { return anims.is_valid(ref) ? &anims[ref.idx] : nullptr; }
void Scene::DestroyAnim(AnimRef anim) { anims.remove_ref(anim); }

BoundToSceneAnim Scene::BindSceneAnim(AnimRef anim_ref) const {
	if (!anims.is_valid(anim_ref)) {
		warn("Invalid animation");
		return {};
	}

	const auto &anim = anims[anim_ref.idx];

	BoundToSceneAnim bound_anim;

	bound_anim.anim = anim_ref;

	std::fill(std::begin(bound_anim.float_track), std::end(bound_anim.float_track), -1);
	for (size_t i = 0; i < anim.float_tracks.size(); ++i) {
		__ASSERT__(i < 128);
		const auto &t = anim.float_tracks[i];
		if (t.target == "FogNear")
			bound_anim.float_track[SFAT_FogNear] = int8_t(i);
		else if (t.target == "FogFar")
			bound_anim.float_track[SFAT_FogFar] = int8_t(i);
	}

	std::fill(std::begin(bound_anim.color_track), std::end(bound_anim.color_track), -1);
	for (size_t i = 0; i < anim.color_tracks.size(); ++i) {
		__ASSERT__(i < 128);
		const auto &t = anim.color_tracks[i];
		if (t.target == "AmbientColor")
			bound_anim.color_track[SCAT_AmbientColor] = int8_t(i);
		else if (t.target == "FogColor")
			bound_anim.color_track[SCAT_FogColor] = int8_t(i);
	}

	return bound_anim;
}

void Scene::EvaluateBoundAnim(const BoundToSceneAnim &bound_anim, time_ns t) {
	if (anims.is_valid(bound_anim.anim)) {
		const auto &anim = anims[bound_anim.anim.idx];

		if (bound_anim.float_track[SFAT_FogNear] != -1)
			Evaluate(anim.float_tracks[bound_anim.float_track[SFAT_FogNear]], t, environment.fog_near);

		if (bound_anim.float_track[SFAT_FogFar] != -1)
			Evaluate(anim.float_tracks[bound_anim.float_track[SFAT_FogFar]], t, environment.fog_far);

		if (bound_anim.color_track[SCAT_FogColor] != -1)
			Evaluate(anim.color_tracks[bound_anim.color_track[SCAT_FogColor]], t, environment.fog_color);

		if (bound_anim.color_track[SCAT_AmbientColor] != -1)
			Evaluate(anim.color_tracks[bound_anim.color_track[SCAT_AmbientColor]], t, environment.ambient);
	}
}

static bool SplitMaterialPropertyName(const std::string &name, size_t &slot_idx, std::string &value) {
	if (!starts_with(name, "Material."))
		return false;

	const auto es = split(slice(name, 9), ".");

	if (es.size() != 2)
		return false;

	slot_idx = atoi(es[0].c_str());
	value = es[1];
	return true;
}

BoundToNodeAnim Scene::BindNodeAnim(NodeRef ref, AnimRef anim_ref) const {
	if (!anims.is_valid(anim_ref)) {
		warn("Invalid animation");
		return {};
	}

	const auto &anim = anims[anim_ref.idx];

	BoundToNodeAnim bound_anim;

	bound_anim.node = ref;
	bound_anim.anim = anim_ref;

	std::fill(std::begin(bound_anim.bool_track), std::end(bound_anim.bool_track), -1);
	for (size_t i = 0; i < anim.bool_tracks.size(); ++i) {
		__ASSERT__(i < 128);
		const auto &t = anim.bool_tracks[i];
		if (t.target == "Enable")
			bound_anim.bool_track[NBAT_Enable] = int8_t(i);
	}

	std::fill(std::begin(bound_anim.float_track), std::end(bound_anim.float_track), -1);
	for (size_t i = 0; i < anim.float_tracks.size(); ++i) {
		__ASSERT__(i < 128);
		const auto &t = anim.float_tracks[i];
		if (t.target == "Light.DiffuseIntensity")
			bound_anim.float_track[NFAT_LightDiffuseIntensity] = int8_t(i);
		else if (t.target == "Light.SpecularIntensity")
			bound_anim.float_track[NFAT_LightSpecularIntensity] = int8_t(i);
		else if (t.target == "Camera.Fov")
			bound_anim.float_track[NFAT_CameraFov] = int8_t(i);
	}

	std::fill(std::begin(bound_anim.vec3_track), std::end(bound_anim.vec3_track), -1);
	for (size_t i = 0; i < anim.vec3_tracks.size(); ++i) {
		__ASSERT__(i < 128);
		const auto &t = anim.vec3_tracks[i];
		if (t.target == "Position")
			bound_anim.vec3_track[NV3AT_TransformPosition] = int8_t(i);
		else if (t.target == "Rotation")
			bound_anim.vec3_track[NV3AT_TransformRotation] = int8_t(i);
		else if (t.target == "Scale")
			bound_anim.vec3_track[NV3AT_TransformScale] = int8_t(i);
	}

	std::fill(std::begin(bound_anim.vec4_track), std::end(bound_anim.vec4_track), -1);
	for (size_t i = 0; i < anim.vec4_tracks.size(); ++i) {
		__ASSERT__(i < 128);
		const auto &t = anim.vec4_tracks[i];

		// material value vec4
		size_t slot_idx;
		std::string value;
		if (SplitMaterialPropertyName(t.target, slot_idx, value)) {
			__ASSERT__(slot_idx < 256);
			bound_anim.vec4_mat_track.push_back({int8_t(i), uint8_t(slot_idx), value});
		}
	}

	std::fill(std::begin(bound_anim.quat_track), std::end(bound_anim.quat_track), -1);
	for (size_t i = 0; i < anim.quat_tracks.size(); ++i) {
		__ASSERT__(i < 128);
		const auto &t = anim.quat_tracks[i];
		if (t.target == "Rotation")
			bound_anim.quat_track[NQAT_TransformRotation] = int8_t(i);
	}

	std::fill(std::begin(bound_anim.color_track), std::end(bound_anim.color_track), -1);
	for (size_t i = 0; i < anim.color_tracks.size(); ++i) {
		__ASSERT__(i < 128);
		const auto &t = anim.color_tracks[i];
		if (t.target == "Light.Diffuse")
			bound_anim.color_track[NCAT_LightDiffuse] = int8_t(i);
		else if (t.target == "Light.Specular")
			bound_anim.color_track[NCAT_LightSpecular] = int8_t(i);
	}

	bound_anim.bound_to_node_instance_anim = {0, nullptr};
	if (!anim.instance_anim_track.keys.empty())
		bound_anim.bound_to_node_instance_anim.kf = -1;

	return bound_anim;
}

void Scene::EvaluateBoundAnim(const BoundToNodeAnim &bound_anim, time_ns t) {
	if (anims.is_valid(bound_anim.anim) && nodes.is_valid(bound_anim.node)) {
		const auto &anim = anims[bound_anim.anim.idx];

		if (bound_anim.bool_track[NBAT_Enable] != -1) {
			bool enable = IsNodeItselfEnabled(bound_anim.node);
			if (Evaluate(anim.bool_tracks[bound_anim.bool_track[NBAT_Enable]], t, enable))
				enable ? EnableNode(bound_anim.node) : DisableNode(bound_anim.node);
		}

		if (auto trs = GetComponent_(transforms, GetNodeComponentRef_<NCI_Transform>(bound_anim.node))) {
			if (bound_anim.vec3_track[NV3AT_TransformPosition] != -1)
				Evaluate(anim.vec3_tracks[bound_anim.vec3_track[NV3AT_TransformPosition]], t, trs->TRS.pos);

			if (anim.flags & AF_UseQuaternionForRotation) {
				if (bound_anim.quat_track[NQAT_TransformRotation] != -1) {
					Quaternion rot;
					if (Evaluate(anim.quat_tracks[bound_anim.quat_track[NQAT_TransformRotation]], t, rot))
						trs->TRS.rot = ToEuler(Normalize(rot)); // EvaluateLinear doesn't normalize quaternions, so we're doing it here
				}
			} else {
				if (bound_anim.vec3_track[NV3AT_TransformRotation] != -1)
					Evaluate(anim.vec3_tracks[bound_anim.vec3_track[NV3AT_TransformRotation]], t, trs->TRS.rot);
			}

			if (bound_anim.vec3_track[NV3AT_TransformScale] != -1)
				Evaluate(anim.vec3_tracks[bound_anim.vec3_track[NV3AT_TransformScale]], t, trs->TRS.scl);
		}

		if (auto lgt = GetComponent_(lights, GetNodeComponentRef_<NCI_Light>(bound_anim.node))) {
			if (bound_anim.color_track[NCAT_LightDiffuse] != -1)
				Evaluate(anim.color_tracks[bound_anim.color_track[NCAT_LightDiffuse]], t, lgt->diffuse);
			if (bound_anim.color_track[NCAT_LightSpecular] != -1)
				Evaluate(anim.color_tracks[bound_anim.color_track[NCAT_LightSpecular]], t, lgt->specular);
			if (bound_anim.float_track[NFAT_LightDiffuseIntensity] != -1)
				Evaluate(anim.float_tracks[bound_anim.float_track[NFAT_LightDiffuseIntensity]], t, lgt->diffuse_intensity);
			if (bound_anim.float_track[NFAT_LightSpecularIntensity] != -1)
				Evaluate(anim.float_tracks[bound_anim.float_track[NFAT_LightSpecularIntensity]], t, lgt->specular_intensity);
		}

		if (auto cam = GetComponent_(cameras, GetNodeComponentRef_<NCI_Camera>(bound_anim.node))) {
			if (bound_anim.float_track[NFAT_CameraFov] != -1)
				Evaluate(anim.float_tracks[bound_anim.float_track[NFAT_CameraFov]], t, cam->fov);
		}

		if (auto obj = GetComponent_(objects, GetNodeComponentRef_<NCI_Object>(bound_anim.node))) {
			// material value tracks
			for (auto &mt : bound_anim.vec4_mat_track) {
				if (mt.slot_idx >= obj->materials.size())
					continue; // invalid material slot

				auto &mat = obj->materials[mt.slot_idx];

				auto i = mat.values.find(mt.value);
				if (i == std::end(mat.values))
					continue; // invalid material value name

				Vec4 v;
				if (Evaluate(anim.vec4_tracks[mt.track_idx], t, v))
					i->second.value = {v.x, v.y, v.z, v.w};
			}
		}

		if (!anim.instance_anim_track.keys.empty()) {
			auto i = node_instance_view.find(bound_anim.node);

			if (i != std::end(node_instance_view)) {
				int kf = numeric_cast<int>(anim.instance_anim_track.keys.size()) - 1;
				for (; kf >= 0; --kf) // grab closest keys to the evaluation time
					if (t >= anim.instance_anim_track.keys[kf].t)
						break;

				if (kf != bound_anim.bound_to_node_instance_anim.kf) {
					const auto ref =
						kf >= 0 ? i->second.GetSceneAnim(*this, anim.instance_anim_track.keys[kf].v.anim_name) : InvalidSceneAnimRef; // get new kf anim

					if (ref != InvalidSceneAnimRef)
						bound_anim.bound_to_node_instance_anim.bound_anim = std::make_shared<SceneBoundAnim>(BindAnim(ref));
					else
						bound_anim.bound_to_node_instance_anim.bound_anim.reset();
				}

				bound_anim.bound_to_node_instance_anim.kf = kf;

				if (bound_anim.bound_to_node_instance_anim.bound_anim) {
					const auto &key = anim.instance_anim_track.keys[kf];

					time_ns sub_t = ((t - key.t) * time_ns(key.v.t_scale * 256.f)) / 256;

					// handle negative time scale and loop mode (both require anim time range)
					if (key.v.t_scale < 0.f || key.v.loop_mode == ALM_Loop) {
						const auto ref = i->second.GetSceneAnim(*this, anim.instance_anim_track.keys[kf].v.anim_name);

						if (const auto anim = GetSceneAnim(ref)) {
							// handle negative t scale
							if (key.v.t_scale < 0.f)
								sub_t += anim->t_end;

							// handle loop mode
							if (key.v.loop_mode == ALM_Loop) {
								if (key.v.t_scale >= 0) {
									while (sub_t >= anim->t_end)
										sub_t -= anim->t_end - anim->t_start;
								} else {
									while (sub_t <= anim->t_start)
										sub_t += anim->t_end - anim->t_start;
								}
							}
						}
					}

					EvaluateBoundAnim(*bound_anim.bound_to_node_instance_anim.bound_anim, sub_t); // evaluate instance bound anim
				}
			}
		}
	}
}

//
const SceneAnimRef InvalidSceneAnimRef;

SceneAnimRef Scene::AddSceneAnim(SceneAnim anim) { return scene_anims.add_ref(std::move(anim)); }
void Scene::DestroySceneAnim(SceneAnimRef ref) { scene_anims.remove_ref(ref); }

size_t Scene::GarbageCollectAnims() {
	std::vector<bool> is_refd(anims.capacity(), false);

	for (const auto &scene_anim : scene_anims) {
		if (scene_anim.scene_anim != InvalidAnimRef)
			is_refd[scene_anim.scene_anim.idx] = true;

		for (const auto &node_anim : scene_anim.node_anims)
			if (node_anim.anim != InvalidAnimRef)
				is_refd[node_anim.anim.idx] = true;
	}

	size_t removed_count = 0;

	for (size_t i = 0; i < is_refd.size(); ++i)
		if (!is_refd[i] && anims.is_used(uint32_t(i))) {
			anims.remove(uint32_t(i));
			++removed_count;
		}

	return removed_count;
}

std::vector<SceneAnimRef> Scene::GetSceneAnims() const {
	std::vector<SceneAnimRef> refs;
	refs.reserve(scene_anims.size());
	for (auto ref = scene_anims.first_ref(); ref != InvalidSceneAnimRef; ref = scene_anims.next_ref(ref))
		if (!(scene_anims[ref.idx].flags & SAF_Instantiated))
			refs.push_back(ref);
	return refs;
}

SceneAnim *Scene::GetSceneAnim(SceneAnimRef ref) { return scene_anims.is_valid(ref) ? &scene_anims[ref.idx] : nullptr; }
const SceneAnim *Scene::GetSceneAnim(SceneAnimRef ref) const { return scene_anims.is_valid(ref) ? &scene_anims[ref.idx] : nullptr; }

SceneAnimRef Scene::GetSceneAnim(const char *name) const {
	for (auto ref = scene_anims.first_ref(); ref != InvalidSceneAnimRef; ref = scene_anims.next_ref(ref))
		if (!(scene_anims[ref.idx].flags & SAF_Instantiated))
			if (scene_anims[ref.idx].name == name)
				return ref;
	return InvalidSceneAnimRef;
}

SceneBoundAnim Scene::BindAnim(const SceneAnim &scene_anim) const {
	SceneBoundAnim bound_anim;

	if (anims.is_valid(scene_anim.scene_anim))
		bound_anim.bound_scene_anim = BindSceneAnim(scene_anim.scene_anim);
	else
		warn("Invalid scene animation");

	for (const auto &i : scene_anim.node_anims)
		bound_anim.bound_node_anims.push_back(BindNodeAnim(i.node, i.anim));

	return bound_anim;
}

SceneBoundAnim Scene::BindAnim(SceneAnimRef ref) const {
	if (!scene_anims.is_valid(ref)) {
		warn("Invalid scene animation reference");
		return {};
	}
	return BindAnim(scene_anims[ref.idx]);
}

void Scene::EvaluateBoundAnim(const SceneBoundAnim &anim, time_ns t) {
	EvaluateBoundAnim(anim.bound_scene_anim, t);

	for (const auto &i : anim.bound_node_anims)
		EvaluateBoundAnim(i, t);
}

//
const ScenePlayAnimRef InvalidScenePlayAnimRef;

ScenePlayAnimRef Scene::PlayAnim(SceneAnimRef ref, AnimLoopMode loop_mode, Easing easing, time_ns t_start, time_ns t_end, bool paused, float t_scale) {
	if (!scene_anims.is_valid(ref)) {
		warn("Invalid scene animation reference");
		return InvalidScenePlayAnimRef;
	}

	ScenePlayAnim play_anim;

	play_anim.name = scene_anims[ref.idx].name;
	play_anim.bound_anim = BindAnim(ref);

	play_anim.flags = paused ? SPAF_Paused : 0;
	play_anim.loop_mode = loop_mode;

	play_anim.t_start = t_start != UnspecifiedAnimTime ? t_start : scene_anims[ref.idx].t_start;
	play_anim.t_end = t_end != UnspecifiedAnimTime ? t_end : scene_anims[ref.idx].t_end;
	play_anim.t = play_anim.t_start;

	play_anim.t_scale = int8_t(t_scale * 16.f);
	play_anim.easing = easing;

	return play_anims.add_ref(std::move(play_anim));
}

bool Scene::IsPlaying(ScenePlayAnimRef ref) const { return play_anims.is_valid(ref); }
void Scene::StopAnim(ScenePlayAnimRef ref) { play_anims.remove_ref(ref); }

void Scene::UpdatePlayingAnims(time_ns dt) {
	std::vector<ScenePlayAnimRef> clean_list;

	for (auto i = play_anims.first_ref(); i != InvalidScenePlayAnimRef; i = play_anims.next_ref(i)) {
		auto &play_anim = play_anims[i.idx];

		// step clock
		if (!(play_anim.flags & SPAF_Paused))
			play_anim.t += (dt * play_anim.t_scale) >> 4;

		// handle end of playback
		if (play_anim.loop_mode == ALM_Infinite) {
			; // let it run indefinitely
		} else if (play_anim.loop_mode == ALM_Loop) {
			if (play_anim.t_scale >= 0) {
				while (play_anim.t >= play_anim.t_end)
					play_anim.t -= play_anim.t_end - play_anim.t_start;
			} else {
				while (play_anim.t <= play_anim.t_start)
					play_anim.t += play_anim.t_end - play_anim.t_start;
			}
		} else { // ALM_Once
			if (play_anim.t_scale >= 0) {
				if (play_anim.t >= play_anim.t_end) {
					play_anim.t = play_anim.t_end;
					clean_list.push_back(i); // let the last evaluation run
				}
			} else {
				if (play_anim.t <= play_anim.t_start) {
					play_anim.t = play_anim.t_start;
					clean_list.push_back(i); // let the last evaluation run
				}
			}
		}

		// easing
		time_ns t = play_anim.t;

		if (play_anim.easing != E_Linear)
			if (t > 0.f && t < 1.f) { // handle ALM_Infinite putting t outside of the easing range
				const auto t_normd = time_to_sec_f(t - play_anim.t_start) / time_to_sec_f(play_anim.t_end - play_anim.t_start);
				const auto t_eased = GetEaseFunc(play_anim.easing)(t_normd);
				t = time_from_sec_f(t_eased * time_to_sec_f(play_anim.t_end - play_anim.t_start) + time_to_sec_f(play_anim.t_start));
			}

		// evaluate
		EvaluateBoundAnim(play_anim.bound_anim, t);
	}

	for (auto i : clean_list)
		play_anims.remove(i.idx); // no point in going through the gen_ref check
}

//
SceneAnimRef Scene::DuplicateSceneAnim(SceneAnimRef ref) {
	const auto anim = GetSceneAnim(ref);

	if (!anim) {
		warn("Invalid scene animation reference");
		return InvalidSceneAnimRef;
	}

	SceneAnim out;

	out.name = anim->name;
	out.t_start = anim->t_start;
	out.t_end = anim->t_end;

	// duplicate animations
	if (const auto in_scene_anim = GetAnim(anim->scene_anim))
		out.scene_anim = AddAnim(*in_scene_anim);
	else
		out.scene_anim = InvalidAnimRef;

	for (const auto &node_anim : anim->node_anims)
		if (const auto in_node_anim = GetAnim(node_anim.anim))
			out.node_anims.push_back({node_anim.node, AddAnim(*in_node_anim)});

	//
	out.frame_duration = anim->frame_duration;
	out.flags = anim->flags;

	return AddSceneAnim(out);
}

//
bool Scene::HasKey(const std::string &key) const {
	auto i = key_values.find(key);
	return i != std::end(key_values);
}

std::vector<std::string> Scene::GetKeys() const {
	std::vector<std::string> keys;
	keys.reserve(key_values.size());

	for (auto i : key_values)
		keys.push_back(i.first);

	return keys;
}

void Scene::RemoveKey(const std::string &key) { key_values.erase(key); }

std::string Scene::GetValue(const std::string &key) const {
	auto i = key_values.find(key);
	return i == std::end(key_values) ? std::string{} : i->second;
}

void Scene::SetValue(const std::string &key, const std::string &value) { key_values[key] = value; }

//
bool GetAnimableNodePropertyBool(const Scene &scene, NodeRef ref, const std::string &name) {
	if (const auto node = scene.GetNode(ref))
		if (name == "Enable")
			return node.IsEnabled();

	return false;
}

void SetAnimableNodePropertyBool(Scene &scene, NodeRef ref, const std::string &name, bool v) {
	if (auto node = scene.GetNode(ref)) {
		if (name == "Enable")
			v ? node.Enable() : node.Disable();
	}
}

//
float GetAnimableScenePropertyFloat(const Scene &scene, const std::string &name) {
	if (name == "FogNear")
		return scene.environment.fog_near;
	if (name == "FogFar")
		return scene.environment.fog_far;
	return 1.f;
}

void SetAnimableScenePropertyFloat(Scene &scene, const std::string &name, float v) {
	if (name == "FogNear")
		scene.environment.fog_near = v;
	else if (name == "FogFar")
		scene.environment.fog_far = v;
}

float GetAnimableNodePropertyFloat(const Scene &scene, NodeRef ref, const std::string &name) {
	if (const auto node = scene.GetNode(ref)) {
		if (name == "Light.DiffuseIntensity")
			return node.GetLight().GetDiffuseIntensity();
		else if (name == "Light.SpecularIntensity")
			return node.GetLight().GetSpecularIntensity();
	}
	return 1.f;
}

void SetAnimableNodePropertyFloat(Scene &scene, NodeRef ref, const std::string &name, float v) {
	if (const auto node = scene.GetNode(ref)) {
		if (name == "Light.DiffuseIntensity")
			node.GetLight().SetDiffuseIntensity(v);
		else if (name == "Light.SpecularIntensity")
			node.GetLight().SetSpecularIntensity(v);
	}
}

//
Vec3 GetAnimableNodePropertyVec3(const Scene &scene, NodeRef ref, const std::string &name) {
	if (const auto node = scene.GetNode(ref)) {
		if (name == "Position")
			return node.GetTransform().GetPos();
		else if (name == "Rotation")
			return node.GetTransform().GetRot();
		else if (name == "Scale")
			return node.GetTransform().GetScale();
	}
	return {};
}

void SetAnimableNodePropertyVec3(Scene &scene, NodeRef ref, const std::string &name, const Vec3 &v) {
	if (const auto node = scene.GetNode(ref)) {
		if (name == "Position")
			node.GetTransform().SetPos(v);
		else if (name == "Rotation")
			node.GetTransform().SetRot(v);
		else if (name == "Scale")
			node.GetTransform().SetScale(v);
	}
}

//
Vec4 GetAnimableNodePropertyVec4(const Scene &scene, NodeRef ref, const std::string &name) {
	if (const auto node = scene.GetNode(ref)) {
		size_t slot_idx;
		std::string value;
		if (SplitMaterialPropertyName(name, slot_idx, value)) {
			if (auto obj = node.GetObject()) {
				if (slot_idx < obj.GetMaterialCount()) {
					const auto &mat = obj.GetMaterial(slot_idx);

					auto i = mat.values.find(value);
					if (i != std::end(mat.values))
						return {i->second.value[0], i->second.value[1], i->second.value[2], i->second.value[3]};
				}
			}
		}
	}
	return {};
}

void SetAnimableNodePropertyVec4(Scene &scene, NodeRef ref, const std::string &name, const Vec4 &v) {
	if (const auto node = scene.GetNode(ref)) {
		size_t slot_idx;
		std::string value;
		if (SplitMaterialPropertyName(name, slot_idx, value)) {
			if (auto obj = node.GetObject()) {
				if (slot_idx < obj.GetMaterialCount()) {
					auto &mat = obj.GetMaterial(slot_idx);

					auto i = mat.values.find(value);
					if (i != std::end(mat.values))
						i->second.value = {v.x, v.y, v.z, v.w};
				}
			}
		}
	}
}

//
Color GetAnimableScenePropertyColor(const Scene &scene, const std::string &name) {
	if (name == "FogColor")
		return scene.environment.fog_color;
	if (name == "AmbientColor")
		return scene.environment.ambient;
	return {};
}

void SetAnimableScenePropertyColor(Scene &scene, const std::string &name, const Color &v) {
	if (name == "FogColor")
		scene.environment.fog_color = v;
	else if (name == "AmbientColor")
		scene.environment.ambient = v;
}

Color GetAnimableNodePropertyColor(const Scene &scene, NodeRef ref, const std::string &name) {
	if (const auto node = scene.GetNode(ref)) {
		if (name == "Light.Diffuse")
			return node.GetLight().GetDiffuseColor();
		else if (name == "Light.Specular")
			return node.GetLight().GetSpecularColor();
	}
	return {};
}

void SetAnimableNodePropertyColor(Scene &scene, NodeRef ref, const std::string &name, const Color &v) {
	if (const auto node = scene.GetNode(ref)) {
		if (name == "Light.Diffuse")
			node.GetLight().SetDiffuseColor(v);
		else if (name == "Light.Specular")
			node.GetLight().SetSpecularColor(v);
	}
}

//
std::string GetAnimableNodePropertyString(const Scene &scene, NodeRef ref, const std::string &name) {
	if (const auto node = scene.GetNode(ref)) {
		if (name == "Instance.Anim") {
			if (auto instance = node.GetInstance())
				return instance.GetOnInstantiateAnim();
		}
	}
	return {};
}

void SetAnimableNodePropertyString(Scene &scene, NodeRef ref, const std::string &name, const std::string &v) {
	if (auto node = scene.GetNode(ref)) {
		if (name == "Instance.Anim") {
			if (auto instance = node.GetInstance())
				if (instance.GetOnInstantiateAnim() != v) {
					instance.SetOnInstantiateAnim(v);
					node.StartOnInstantiateAnim();
				}
		}
	}
}

//
void ReverseSceneAnim(Scene &scene, SceneAnim &scene_anim) {
	if (auto anim = scene.GetAnim(scene_anim.scene_anim))
		ReverseAnim(*anim, scene_anim.t_start, scene_anim.t_end);

	for (auto node_anim : scene_anim.node_anims)
		if (auto anim = scene.GetAnim(node_anim.anim))
			ReverseAnim(*anim, scene_anim.t_start, scene_anim.t_end);
}

void QuantizeSceneAnim(Scene &scene, SceneAnim &scene_anim, time_ns t_step) {
	scene_anim.t_start = (scene_anim.t_start / t_step) * t_step;
	scene_anim.t_end = (scene_anim.t_end / t_step) * t_step;

	if (auto anim = scene.GetAnim(scene_anim.scene_anim))
		QuantizeAnim(*anim, t_step);

	for (auto node_anim : scene_anim.node_anims)
		if (auto anim = scene.GetAnim(node_anim.anim))
			QuantizeAnim(*anim, t_step);
}

void DeleteEmptySceneAnims(Scene &scene, SceneAnim &scene_anim) {
	if (auto anim = scene.GetAnim(scene_anim.scene_anim))
		if (!AnimHasKeys(*anim))
			scene_anim.scene_anim = InvalidAnimRef;

	for (const auto &node_anim : scene_anim.node_anims)
		scene_anim.node_anims.erase(std::remove_if(std::begin(scene_anim.node_anims), std::end(scene_anim.node_anims),
										[&](const NodeAnim &node_anim) {
											auto anim = scene.GetAnim(node_anim.anim);
											if (!anim)
												return true; // remove invalid anim ref
											return AnimHasKeys(*anim) == false; // remove anim with no keys
										}),
			std::end(scene_anim.node_anims));
}

//
Node CreateSceneRootNode(Scene &scene, std::string name, const Mat4 &mtx) {
	Node root = scene.CreateNode(name);
	root.SetTransform(scene.CreateTransform(mtx));

	for (const auto &node : scene.GetNodesWithComponent(NCI_Transform)) {
		auto transform = node.GetTransform();
		if (transform.GetParent() == InvalidNodeRef)
			transform.SetParent(root.ref);
	}

	return root;
}

//
Node CreatePointLight(
	Scene &scene, const Mat4 &mtx, float radius, const Color &diffuse, const Color &specular, float priority, LightShadowType shadow_type, float shadow_bias) {
	return CreatePointLight(scene, mtx, radius, diffuse, 1.f, specular, 1.f, priority, shadow_type, shadow_bias);
}

Node CreateSpotLight(Scene &scene, const Mat4 &mtx, float radius, float inner_angle, float outer_angle, const Color &diffuse, const Color &specular,
	float priority, LightShadowType shadow_type, float shadow_bias) {
	return CreateSpotLight(scene, mtx, radius, inner_angle, outer_angle, diffuse, 1.f, specular, 1.f, priority, shadow_type, shadow_bias);
}

Node CreateLinearLight(Scene &scene, const Mat4 &mtx, const Color &diffuse, const Color &specular, float priority, LightShadowType shadow_type,
	float shadow_bias, const Vec4 &pssm_split) {
	return CreateLinearLight(scene, mtx, diffuse, 1.f, specular, 1.f, priority, shadow_type, shadow_bias, pssm_split);
}

//
#ifndef NDEBUG
void _CheckDllSceneDataTypes(size_t s_gen_ref, size_t s_node, size_t s_scene, size_t s_scene_view, size_t s_scene_anim) {
	assert(s_gen_ref == sizeof(gen_ref));
	assert(s_node == sizeof(Node));
	assert(s_scene == sizeof(Scene));
	assert(s_scene_view == sizeof(SceneView));
	assert(s_scene_anim == sizeof(SceneAnim));
}
#endif

} // namespace hg
