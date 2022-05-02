// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/node.h"
#include "engine/scene.h"

namespace hg {

const NodeRef InvalidNodeRef;
const ComponentRef InvalidComponentRef;

//
bool Node::IsValid() const { return scene_ref && scene_ref->scene ? scene_ref->scene->IsValidNodeRef(ref) : false; }

//
std::string Node::GetName() const {
	if (scene_ref && scene_ref->scene)
		return scene_ref->scene->GetNodeName(ref);
	return {};
}

void Node::SetName(const std::string &v) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetNodeName(ref, v);
}

//
uint32_t Node::GetFlags() const {
	if (scene_ref && scene_ref->scene)
		return scene_ref->scene->GetNodeFlags(ref);
	return {};
}

void Node::SetFlags(uint32_t v) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetNodeFlags(ref, v);
}

//
void Node::Enable() {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->EnableNode(ref);
}

void Node::Disable() {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->DisableNode(ref);
}

bool Node::IsEnabled() const { return scene_ref && scene_ref->scene ? scene_ref->scene->IsNodeEnabled(ref) : false; }
bool Node::IsItselfEnabled() const { return scene_ref && scene_ref->scene ? scene_ref->scene->IsNodeItselfEnabled(ref) : false; }

//
bool Node::HasTransform() const {
	return scene_ref && scene_ref->scene ? scene_ref->scene->IsValidTransformRef(scene_ref->scene->GetNodeTransformRef(ref)) : false;
}

Transform Node::GetTransform() const { return scene_ref && scene_ref->scene ? scene_ref->scene->GetNodeTransform(ref) : Transform{nullptr}; }

void Node::SetTransform(const Transform &c) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetNodeTransform(ref, c);
}

void Node::RemoveTransform() {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetNodeTransform(ref, InvalidComponentRef);
}

//
bool Node::HasCamera() const { return scene_ref && scene_ref->scene ? scene_ref->scene->IsValidCameraRef(scene_ref->scene->GetNodeCameraRef(ref)) : false; }

Camera Node::GetCamera() const { return scene_ref && scene_ref->scene ? scene_ref->scene->GetNodeCamera(ref) : Camera{nullptr}; }

void Node::SetCamera(const Camera &c) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetNodeCamera(ref, c);
}

void Node::RemoveCamera() {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetNodeCamera(ref, InvalidComponentRef);
}

//
ViewState Node::ComputeCameraViewState(const Vec2 &aspect_ratio) {
	if (scene_ref && scene_ref->scene)
		return scene_ref->scene->ComputeCameraViewState(ref, aspect_ratio);
	return {};
}

//
bool Node::HasObject() const { return scene_ref && scene_ref->scene ? scene_ref->scene->IsValidObjectRef(scene_ref->scene->GetNodeObjectRef(ref)) : false; }

Object Node::GetObject() const { return scene_ref && scene_ref->scene ? scene_ref->scene->GetNodeObject(ref) : Object{nullptr}; }

void Node::SetObject(const Object &c) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetNodeObject(ref, c);
}

void Node::RemoveObject() {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetNodeObject(ref, InvalidComponentRef);
}

bool Node::GetMinMax(const PipelineResources &resources, MinMax &minmax) const {
	if (HasObject()) {
		if (!GetObject().GetMinMax(resources, minmax))
			return false;
	} else if (HasInstance()) {
		return scene_ref && scene_ref->scene ? GetNodesMinMax(GetInstanceSceneView().GetNodes(*scene_ref->scene), resources, minmax) : false;
	} else {
		return false;
	}

	const auto world = GetTransform().GetWorld();
	minmax = minmax * world;
	return true;
}

//
bool Node::HasLight() const { return scene_ref && scene_ref->scene ? scene_ref->scene->IsValidLightRef(scene_ref->scene->GetNodeLightRef(ref)) : false; }

Light Node::GetLight() const { return scene_ref && scene_ref->scene ? scene_ref->scene->GetNodeLight(ref) : Light{nullptr}; }

void Node::SetLight(const Light &c) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetNodeLight(ref, c);
}

void Node::RemoveLight() {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetNodeLight(ref, InvalidComponentRef);
}

//
bool Node::HasRigidBody() const {
	return scene_ref && scene_ref->scene ? scene_ref->scene->IsValidRigidBodyRef(scene_ref->scene->GetNodeRigidBodyRef(ref)) : false;
}

RigidBody Node::GetRigidBody() const { return scene_ref && scene_ref->scene ? scene_ref->scene->GetNodeRigidBody(ref) : RigidBody{nullptr}; }

void Node::SetRigidBody(const RigidBody &rb) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetNodeRigidBody(ref, rb);
}

void Node::RemoveRigidBody() {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetNodeRigidBody(ref, InvalidComponentRef);
}

//
size_t Node::GetCollisionCount() const { return scene_ref && scene_ref->scene ? scene_ref->scene->GetNodeCollisionCount(ref) : 0; }

Collision Node::GetCollision(size_t idx) const { return scene_ref && scene_ref->scene ? scene_ref->scene->GetNodeCollision(ref, idx) : Collision{nullptr}; }

void Node::SetCollision(size_t idx, const Collision &collision) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetNodeCollision(ref, idx, collision);
}

void Node::RemoveCollision(ComponentRef cref) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->RemoveNodeCollision(ref, cref);
}

void Node::RemoveCollision(size_t slot_idx) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->RemoveNodeCollision(ref, slot_idx);
}

//
bool Node::HasInstance() const {
	return scene_ref && scene_ref->scene ? scene_ref->scene->IsValidInstanceRef(scene_ref->scene->GetNodeInstanceRef(ref)) : false;
}

Instance Node::GetInstance() const { return scene_ref && scene_ref->scene ? scene_ref->scene->GetNodeInstance(ref) : Instance{nullptr}; }

void Node::SetInstance(const Instance &i) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetNodeInstance(ref, i);
}

bool Node::SetupInstance(const Reader &ir, const ReadProvider &ip, PipelineResources &resources, const PipelineInfo &pipeline, uint32_t flags) {
	if (scene_ref && scene_ref->scene)
		return scene_ref->scene->NodeSetupInstance(ref, ir, ip, resources, pipeline, flags);
	return true;
}

bool Node::SetupInstanceFromFile(PipelineResources &resources, const PipelineInfo &pipeline, uint32_t flags) {
	if (scene_ref && scene_ref->scene)
		return scene_ref->scene->NodeSetupInstanceFromFile(ref, resources, pipeline, flags);
	return true;
}

bool Node::SetupInstanceFromAssets(PipelineResources &resources, const PipelineInfo &pipeline, uint32_t flags) {
	if (scene_ref && scene_ref->scene)
		return scene_ref->scene->NodeSetupInstanceFromAssets(ref, resources, pipeline);
	return true;
}

void Node::DestroyInstance() {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->NodeDestroyInstance(ref);
}

const SceneView &Node::GetInstanceSceneView() const {
	static SceneView empty_view;
	if (scene_ref && scene_ref->scene)
		return scene_ref->scene->GetNodeInstanceSceneView(ref);
	return empty_view;
}

SceneAnimRef Node::GetInstanceSceneAnim(const std::string &name) const {
	if (scene_ref && scene_ref->scene)
		return scene_ref->scene->GetNodeInstanceSceneView(ref).GetSceneAnim(*scene_ref->scene, name);
	return InvalidSceneAnimRef;
}

void Node::StartOnInstantiateAnim() {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->NodeStartOnInstantiateAnim(ref);
}

void Node::StopOnInstantiateAnim() {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->NodeStopOnInstantiateAnim(ref);
}

Node Node::IsInstantiatedBy() const { return scene_ref && scene_ref->scene ? scene_ref->scene->GetNode(scene_ref->scene->IsInstantiatedBy(ref)) : Node{}; }

//
size_t Node::GetScriptCount() const { return scene_ref && scene_ref->scene ? scene_ref->scene->GetNodeScriptCount(ref) : 0; }

Script Node::GetScript(size_t idx) const { return scene_ref && scene_ref->scene ? scene_ref->scene->GetNodeScript(ref, idx) : Script{}; }

void Node::SetScript(size_t idx, const Script &script) {
	if (scene_ref && scene_ref->scene)
		if (scene_ref->scene->IsValidScriptRef(script.ref))
			scene_ref->scene->SetNodeScript(ref, idx, script);
}

void Node::RemoveScript(ComponentRef cref) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->RemoveNodeScript(ref, cref);
}

void Node::RemoveScript(size_t slot_idx) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->RemoveNodeScript(ref, slot_idx);
}

//
Mat4 Node::GetWorld() const { return scene_ref && scene_ref->scene ? scene_ref->scene->GetNodeWorldMatrix(ref) : Mat4::Identity; }

void Node::SetWorld(const Mat4 &world) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetNodeWorldMatrix(ref, world);
}

Mat4 Node::ComputeWorld() const { return scene_ref && scene_ref->scene ? scene_ref->scene->ComputeNodeWorldMatrix(ref) : Mat4::Identity; }

//
bool GetNodesMinMax(const std::vector<Node> &nodes, const PipelineResources &resources, MinMax &minmax) {
	if (nodes.empty())
		return false;

	size_t minmax_count = 0;

	for (const auto &node : nodes) {
		MinMax node_minmax;
		if (!node.GetMinMax(resources, node_minmax))
			continue;

		if (minmax_count == 0)
			minmax = node_minmax;
		else
			minmax = Union(minmax, node_minmax);

		++minmax_count;
	}

	return minmax_count > 0;
}

} // namespace hg
