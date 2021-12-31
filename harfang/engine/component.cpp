// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/file.h"
#include "foundation/format.h"
#include "foundation/log.h"
#include "foundation/math.h"

#include "engine/assets.h"
#include "engine/scene.h"

namespace hg {

void Scene::ReserveTransforms(const size_t count) { transforms.reserve(transforms.size() + count); }
void Scene::ReserveCameras(const size_t count) { cameras.reserve(cameras.size() + count); }
void Scene::ReserveObjects(const size_t count) { objects.reserve(objects.size() + count); }
void Scene::ReserveLights(const size_t count) { lights.reserve(lights.size() + count); }
void Scene::ReserveScripts(const size_t count) { scripts.reserve(scripts.size() + count); }

//
static bool IsChildOf(Scene &scene, NodeRef node, ComponentRef transform) {
	if (scene.GetNode(node).GetTransform().ref == transform)
		return true;

	while (node != InvalidNodeRef) {
		const auto parent = scene.GetNode(node).GetTransform().GetParent();
		if (scene.GetNode(parent).GetTransform().ref == transform)
			return true;
		node = parent;
	}
	return false;
}

//
Transform Scene::CreateTransform() {
	const auto ref = transforms.add_ref({});
	if (ref.idx >= transform_worlds.size())
		transform_worlds.resize(ref.idx + 64, Mat4::Identity); // so that GetWorld works straight away
	return {scene_ref, ref};
}

void Scene::DestroyTransform(ComponentRef ref) { transforms.remove_ref(ref); }

Vec3 Scene::GetTransformPos(ComponentRef ref) const {
	if (const auto *c = GetComponent_(transforms, ref))
		return c->TRS.pos;
	return {};
}

void Scene::SetTransformPos(ComponentRef ref, const Vec3 &v) {
	if (auto *c = GetComponent_(transforms, ref))
		c->TRS.pos = v;
}

bool Transform::IsValid() const { return scene_ref && scene_ref->scene ? scene_ref->scene->IsValidTransformRef(ref) : false; }

Vec3 Transform::GetPos() const {
	if (scene_ref && scene_ref->scene)
		return scene_ref->scene->GetTransformPos(ref);
	return {};
}

void Transform::SetPos(const Vec3 &v) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetTransformPos(ref, v);
}

void Transform::GetPosRot(Vec3 &p, Vec3 &r) {
	p = GetPos();
	r = GetRot();
}

void Transform::SetPosRot(const Vec3 &p, const Vec3 &r) {
	SetPos(p);
	SetRot(r);
}

Vec3 Scene::GetTransformRot(ComponentRef ref) const {
	if (const auto *c = GetComponent_(transforms, ref))
		return c->TRS.rot;
	return {};
}

void Scene::SetTransformRot(ComponentRef ref, const Vec3 &v) {
	if (auto *c = GetComponent_(transforms, ref))
		c->TRS.rot = v;
}

Vec3 Transform::GetRot() const {
	if (scene_ref && scene_ref->scene)
		return scene_ref->scene->GetTransformRot(ref);
	return {};
}

void Transform::SetRot(const Vec3 &v) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetTransformRot(ref, v);
}

Vec3 Scene::GetTransformScale(ComponentRef ref) const {
	if (const auto *c = GetComponent_(transforms, ref))
		return c->TRS.scl;
	return {};
}
void Scene::SetTransformScale(ComponentRef ref, const Vec3 &v) {
	if (auto *c = GetComponent_(transforms, ref))
		c->TRS.scl = v;
}

Vec3 Transform::GetScale() const {
	if (scene_ref && scene_ref->scene)
		return scene_ref->scene->GetTransformScale(ref);
	return {};
}

void Transform::SetScale(const Vec3 &v) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetTransformScale(ref, v);
}

TransformTRS Scene::GetTransformTRS(ComponentRef ref) const {
	if (const auto *c = GetComponent_(transforms, ref))
		return c->TRS;
	return {};
}

void Scene::SetTransformTRS(ComponentRef ref, const TransformTRS &v) {
	if (auto *c = GetComponent_(transforms, ref))
		c->TRS = v;
}

TransformTRS Transform::GetTRS() const {
	if (scene_ref && scene_ref->scene)
		return scene_ref->scene->GetTransformTRS(ref);
	return {};
}

void Transform::SetTRS(const TransformTRS &v) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetTransformTRS(ref, v);
}

NodeRef Scene::GetTransformParent(ComponentRef ref) const {
	if (const auto *c = GetComponent_(transforms, ref))
		return c->parent;
	return {};
}

void Scene::SetTransformParent(ComponentRef ref, const NodeRef &v) {
	if (auto *c = GetComponent_(transforms, ref))
		if (!hg::IsChildOf(*scene_ref->scene, v, ref))
			c->parent = v;
}

NodeRef Transform::GetParent() const {
	if (scene_ref && scene_ref->scene)
		return scene_ref->scene->GetTransformParent(ref);
	return {};
}

void Transform::SetParent(NodeRef v) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetTransformParent(ref, v);
}

Node Transform::GetParentNode() const {
	if (scene_ref && scene_ref->scene)
		return {scene_ref, scene_ref->scene->GetTransformParent(ref)};
	return {};
}

void Transform::SetParentNode(const Node &n) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetTransformParent(ref, n.ref);
}

Transform Scene::CreateTransform(const Vec3 &pos, const Vec3 &rot, const Vec3 &scl, NodeRef parent) {
	return {scene_ref, transforms.add_ref({{pos, rot, scl}, parent})};
}

Transform Scene::CreateTransform(const hg::Mat4 &mtx, NodeRef parent) {
	Vec3 pos, rot, scl;
	Decompose(mtx, &pos, &rot, &scl);
	return CreateTransform(pos, rot, scl);
}

//
void Scene::SetTransformLocalMatrix(ComponentRef ref, const Mat4 &local) {
	if (auto trs = GetComponent_(transforms, ref)) {
		Decompose(local, &trs->TRS.pos, &trs->TRS.rot, &trs->TRS.scl);

		const auto parent_trs_ref = GetNodeComponentRef_<NCI_Transform>(trs->parent);
		const auto world = IsValidTransformRef(parent_trs_ref) ? transform_worlds[parent_trs_ref.idx] * local : local;

		transform_worlds[ref.idx] = world;
	}
}

Mat4 Scene::GetTransformWorldMatrix(const uint32_t transform_idx) const {
	return transform_idx < transform_worlds.size() ? transform_worlds[transform_idx] : Mat4::Identity;
}

Mat4 Scene::GetPreviousTransformWorldMatrix(const uint32_t transform_idx) const {
	return transform_idx < previous_transform_worlds.size() ? previous_transform_worlds[transform_idx] : transform_worlds[transform_idx];
}

void Scene::SetTransformWorldMatrix(ComponentRef ref, const Mat4 &world) {
	if (auto trs = GetComponent_(transforms, ref))
		if (ref.idx < transform_worlds.size()) {
			transform_worlds[ref.idx] = world;
			const auto parent_trs_ref = GetNodeComponentRef_<NCI_Transform>(trs->parent);
			const auto local = IsValidTransformRef(parent_trs_ref) ? InverseFast(transform_worlds[parent_trs_ref.idx]) * world : world;
			Decompose(local, &trs->TRS.pos, &trs->TRS.rot, &trs->TRS.scl);
		}
}

//
Mat4 Transform::GetWorld() const { return scene_ref && scene_ref->scene ? scene_ref->scene->GetTransformWorldMatrix(ref.idx) : Mat4::Identity; }

void Transform::SetWorld(const Mat4 &world) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetTransformWorldMatrix(ref, world);
}

void Transform::SetLocal(const Mat4 &local) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetTransformLocalMatrix(ref, local);
}

//
Camera Scene::CreateCamera() { return {scene_ref, cameras.add_ref({})}; }
void Scene::DestroyCamera(ComponentRef ref) { cameras.remove_ref(ref); }

float Scene::GetCameraZNear(ComponentRef ref) const {
	if (const auto *c = GetComponent_(cameras, ref))
		return c->zrange.znear;
	return {};
}

void Scene::SetCameraZNear(ComponentRef ref, float v) {
	if (auto *c = GetComponent_(cameras, ref))
		c->zrange.znear = Clamp(v, 0.0001f, c->zrange.zfar - 0.0001f);
}

bool Camera::IsValid() const { return scene_ref && scene_ref->scene ? scene_ref->scene->IsValidCameraRef(ref) : false; }

float Camera::GetZNear() const {
	if (scene_ref && scene_ref->scene)
		return scene_ref->scene->GetCameraZNear(ref);
	return {};
}

void Camera::SetZNear(float v) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetCameraZNear(ref, v);
}

float Scene::GetCameraZFar(ComponentRef ref) const {
	if (const auto *c = GetComponent_(cameras, ref))
		return c->zrange.zfar;
	return {};
}

void Scene::SetCameraZFar(ComponentRef ref, float v) {
	if (auto *c = GetComponent_(cameras, ref))
		c->zrange.zfar = Max(v, c->zrange.znear + 0.0001f);
}

float Camera::GetZFar() const {
	if (scene_ref && scene_ref->scene)
		return scene_ref->scene->GetCameraZFar(ref);
	return {};
}

void Camera::SetZFar(float v) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetCameraZFar(ref, v);
}

CameraZRange Scene::GetCameraZRange(ComponentRef ref) const {
	if (const auto *c = GetComponent_(cameras, ref))
		return c->zrange;
	return {};
}

void Scene::SetCameraZRange(ComponentRef ref, CameraZRange v) {
	if (auto *c = GetComponent_(cameras, ref)) {
		const auto znear = Clamp(v.znear, 0.0001f, v.zfar - 0.0001f), zfar = Max(v.zfar, znear);
		c->zrange = {znear, zfar};
	}
}

CameraZRange Camera::GetZRange() const {
	if (scene_ref && scene_ref->scene)
		return scene_ref->scene->GetCameraZRange(ref);
	return {};
}

void Camera::SetZRange(CameraZRange v) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetCameraZRange(ref, v);
}

float Scene::GetCameraFov(ComponentRef ref) const {
	if (const auto *c = GetComponent_(cameras, ref))
		return c->fov;
	return {};
}

void Scene::SetCameraFov(ComponentRef ref, float v) {
	if (auto *c = GetComponent_(cameras, ref))
		c->fov = Clamp(v, Deg(0.0001f), Deg(180.f - 0.0001f));
}

float Camera::GetFov() const {
	if (scene_ref && scene_ref->scene)
		return scene_ref->scene->GetCameraFov(ref);
	return {};
}

void Camera::SetFov(float v) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetCameraFov(ref, v);
}

bool Scene::GetCameraIsOrthographic(ComponentRef ref) const {
	if (const auto *c = GetComponent_(cameras, ref))
		return c->ortho;
	return {};
}

void Scene::SetCameraIsOrthographic(ComponentRef ref, const bool &v) {
	if (auto *c = GetComponent_(cameras, ref))
		c->ortho = v;
}

bool Camera::GetIsOrthographic() const {
	if (scene_ref && scene_ref->scene)
		return scene_ref->scene->GetCameraIsOrthographic(ref);
	return {};
}

void Camera::SetIsOrthographic(bool v) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetCameraIsOrthographic(ref, v);
}

float Scene::GetCameraSize(ComponentRef ref) const {
	if (const auto *c = GetComponent_(cameras, ref))
		return c->size;
	return {};
}

void Scene::SetCameraSize(ComponentRef ref, float v) {
	if (auto *c = GetComponent_(cameras, ref))
		c->size = v != 0.f ? v : 0.0001f;
}

float Camera::GetSize() const {
	if (scene_ref && scene_ref->scene)
		return scene_ref->scene->GetCameraSize(ref);
	return {};
}

void Camera::SetSize(float v) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetCameraSize(ref, v);
}

Camera Scene::CreateCamera(const float znear, const float zfar, const float fov) { return {scene_ref, cameras.add_ref({{znear, zfar}, fov, false})}; }

Camera Scene::CreateOrthographicCamera(const float znear, const float zfar, const float size) {
	return {scene_ref, cameras.add_ref({{znear, zfar}, Deg(45.f), true, size})};
}

//
Object Scene::CreateObject() { return {scene_ref, objects.add_ref({})}; }
void Scene::DestroyObject(ComponentRef ref) { objects.remove_ref(ref); }

ModelRef Scene::GetObjectModel(ComponentRef ref) const {
	if (const auto *c = GetComponent_(objects, ref))
		return c->model;
	return {};
}

void Scene::SetObjectModel(ComponentRef ref, const ModelRef &v) {
	if (auto *c = GetComponent_(objects, ref))
		c->model = v;
}

bool Object::IsValid() const { return scene_ref && scene_ref->scene ? scene_ref->scene->IsValidObjectRef(ref) : false; }

ModelRef Object::GetModelRef() const {
	if (scene_ref && scene_ref->scene)
		return scene_ref->scene->GetObjectModel(ref);
	return {};
}

void Object::SetModelRef(ModelRef v) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetObjectModel(ref, v);
};

void Object::ClearModelRef() {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetObjectModel(ref, InvalidModelRef);
};

//
static Material null_bound_material;

Material &Scene::GetObjectMaterial(ComponentRef ref, size_t slot_idx) {
	if (auto *c = GetComponent_(objects, ref))
		return slot_idx < c->materials.size() ? c->materials[slot_idx] : null_bound_material;
	return null_bound_material;
}

void Scene::SetObjectMaterial(ComponentRef ref, size_t slot_idx, Material material) {
	if (auto *c = GetComponent_(objects, ref)) {
		if (c->materials.size() <= slot_idx)
			c->materials.resize(slot_idx + 1);
		c->materials[slot_idx] = std::move(material);
	};
}

Material *Scene::GetObjectMaterial(ComponentRef ref, const std::string &name) {
	if (auto *c = GetComponent_(objects, ref)) {
		size_t i = 0;
		for (; i < c->material_infos.size(); ++i)
			if (c->material_infos[i].name == name)
				break;

		if (i == c->material_infos.size() || i >= c->materials.size())
			return nullptr;

		return &c->materials[i];
	}
	return nullptr;
}

Material &Object::GetMaterial(const size_t slot_idx) const {
	return scene_ref && scene_ref->scene ? scene_ref->scene->GetObjectMaterial(ref, slot_idx) : null_bound_material;
}

void Object::SetMaterial(const size_t slot_idx, Material material) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetObjectMaterial(ref, slot_idx, material);
}

std::string Scene::GetObjectMaterialName(ComponentRef ref, size_t slot_idx) const {
	if (auto *c = GetComponent_(objects, ref))
		return slot_idx < c->material_infos.size() ? c->material_infos[slot_idx].name : std::string{};
	return {};
}

void Scene::SetObjectMaterialName(ComponentRef ref, size_t slot_idx, const std::string &name) {
	if (auto *c = GetComponent_(objects, ref)) {
		c->material_infos.resize(slot_idx + 1);
		c->material_infos[slot_idx].name = name;
	};
}

//
size_t Scene::GetObjectBoneCount(ComponentRef ref) const {
	if (auto *c = GetComponent_(objects, ref))
		return c->bones.size();
	return 0;
}

size_t Object::GetBoneCount() const { return scene_ref && scene_ref->scene ? scene_ref->scene->GetObjectBoneCount(ref) : 0; }

void Scene::SetObjectBoneCount(ComponentRef ref, size_t count) {
	if (auto *c = GetComponent_(objects, ref))
		c->bones.resize(count);
}

void Object::SetBoneCount(size_t count) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetObjectBoneCount(ref, count);
}

bool Scene::SetObjectBone(ComponentRef ref, size_t idx, NodeRef bone_node) {
	if (auto *c = GetComponent_(objects, ref))
		if (idx < c->bones.size()) {
			c->bones[idx] = bone_node;
			return true;
		}

	return false;
}

bool Object::SetBone(size_t idx, NodeRef bone) { return scene_ref && scene_ref->scene ? scene_ref->scene->SetObjectBone(ref, idx, bone) : false; }

NodeRef Scene::GetObjectBone(ComponentRef ref, size_t idx) const {
	if (auto *c = GetComponent_(objects, ref))
		if (idx < c->bones.size())
			return c->bones[idx];
	return InvalidNodeRef;
}

NodeRef Object::GetBone(size_t idx) const { return scene_ref && scene_ref->scene ? scene_ref->scene->GetObjectBone(ref, idx) : InvalidNodeRef; }

Node Object::GetBoneNode(size_t idx) const { return {scene_ref, GetBone(idx)}; }
bool Object::SetBoneNode(size_t idx, const Node &node) { return SetBone(idx, node.ref); }

//
std::string Object::GetMaterialName(size_t slot_idx) const {
	return scene_ref && scene_ref->scene ? scene_ref->scene->GetObjectMaterialName(ref, slot_idx) : std::string{};
}

void Object::SetMaterialName(size_t slot_idx, const std::string &name) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetObjectMaterialName(ref, slot_idx, name);
}

size_t Scene::GetObjectMaterialCount(ComponentRef ref) const {
	if (const auto *c = GetComponent_(objects, ref))
		return c->materials.size();
	return 0;
}

void Scene::SetObjectMaterialCount(ComponentRef ref, size_t v) {
	if (auto *c = GetComponent_(objects, ref)) {
		c->material_infos.resize(v);
		c->materials.resize(v);
	}
}

size_t Object::GetMaterialCount() const { return scene_ref && scene_ref->scene ? scene_ref->scene->GetObjectMaterialCount(ref) : 0; }

void Object::SetMaterialCount(size_t v) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetObjectMaterialCount(ref, v);
};

Material *Object::GetMaterial(const std::string &name) const {
	return scene_ref && scene_ref->scene ? scene_ref->scene->GetObjectMaterial(ref, name) : nullptr;
}

bool Scene::GetObjectMinMax(ComponentRef ref, const PipelineResources &resources, MinMax &minmax) const {
	const auto *c = GetComponent_(objects, ref);
	if (!c)
		return false;

	if (!resources.models.IsValidRef(c->model))
		return false;

	const auto &mdl = resources.models.Get_unsafe_(c->model.ref.idx);
	if (mdl.bounds.empty())
		return false;

	minmax = mdl.bounds[0];
	for (size_t i = 1; i < mdl.bounds.size(); ++i)
		minmax = Union(minmax, mdl.bounds[i]);

	return true;
}

bool Object::GetMinMax(const PipelineResources &resources, MinMax &minmax) const {
	return scene_ref && scene_ref->scene ? scene_ref->scene->GetObjectMinMax(ref, resources, minmax) : false;
}

Object Scene::CreateObject(const ModelRef &model, std::vector<Material> materials) { return {scene_ref, objects.add_ref({model, std::move(materials)})}; }

//
Light Scene::CreateLight() { return {scene_ref, lights.add_ref({})}; }
void Scene::DestroyLight(ComponentRef ref) { lights.remove_ref(ref); }

LightType Scene::GetLightType(ComponentRef ref) const {
	if (const auto *c = GetComponent_(lights, ref))
		return c->type;
	return {};
}

void Scene::SetLightType(ComponentRef ref, LightType v) {
	if (auto *c = GetComponent_(lights, ref))
		c->type = v;
}

LightShadowType Scene::GetLightShadowType(ComponentRef ref) const {
	if (const auto *c = GetComponent_(lights, ref))
		return c->shadow_type;
	return {};
}

void Scene::SetLightShadowType(ComponentRef ref, LightShadowType v) {
	if (auto *c = GetComponent_(lights, ref))
		c->shadow_type = v;
}

bool Light::IsValid() const { return scene_ref && scene_ref->scene ? scene_ref->scene->IsValidLightRef(ref) : false; }

LightType Light::GetType() const {
	if (scene_ref && scene_ref->scene)
		return scene_ref->scene->GetLightType(ref);
	return {};
}

void Light::SetType(LightType v) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetLightType(ref, v);
}

LightShadowType Light::GetShadowType() const {
	if (scene_ref && scene_ref->scene)
		return scene_ref->scene->GetLightShadowType(ref);
	return {};
}

void Light::SetShadowType(LightShadowType v) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetLightShadowType(ref, v);
}

Color Scene::GetLightDiffuseColor(ComponentRef ref) const {
	if (const auto *c = GetComponent_(lights, ref))
		return c->diffuse;
	return {};
}

void Scene::SetLightDiffuseColor(ComponentRef ref, const Color &v) {
	if (auto *c = GetComponent_(lights, ref))
		c->diffuse = v;
}

float Scene::GetLightDiffuseIntensity(ComponentRef ref) const {
	if (const auto *c = GetComponent_(lights, ref))
		return c->diffuse_intensity;
	return {};
}

void Scene::SetLightDiffuseIntensity(ComponentRef ref, float v) {
	if (auto *c = GetComponent_(lights, ref))
		c->diffuse_intensity = v;
}

Color Light::GetDiffuseColor() const {
	if (scene_ref && scene_ref->scene)
		return scene_ref->scene->GetLightDiffuseColor(ref);
	return {};
}

void Light::SetDiffuseColor(const Color &v) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetLightDiffuseColor(ref, v);
}

float Light::GetDiffuseIntensity() const {
	if (scene_ref && scene_ref->scene)
		return scene_ref->scene->GetLightDiffuseIntensity(ref);
	return {};
}

void Light::SetDiffuseIntensity(float v) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetLightDiffuseIntensity(ref, v);
}

Color Scene::GetLightSpecularColor(ComponentRef ref) const {
	if (const auto *c = GetComponent_(lights, ref))
		return c->specular;
	return {};
}

void Scene::SetLightSpecularColor(ComponentRef ref, const Color &v) {
	if (auto *c = GetComponent_(lights, ref))
		c->specular = v;
}

float Scene::GetLightSpecularIntensity(ComponentRef ref) const {
	if (const auto *c = GetComponent_(lights, ref))
		return c->specular_intensity;
	return {};
}

void Scene::SetLightSpecularIntensity(ComponentRef ref, float v) {
	if (auto *c = GetComponent_(lights, ref))
		c->specular_intensity = v;
}

Color Light::GetSpecularColor() const {
	if (scene_ref && scene_ref->scene)
		return scene_ref->scene->GetLightSpecularColor(ref);
	return {};
}

void Light::SetSpecularColor(const Color &v) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetLightSpecularColor(ref, v);
}

float Light::GetSpecularIntensity() const {
	if (scene_ref && scene_ref->scene)
		return scene_ref->scene->GetLightSpecularIntensity(ref);
	return {};
}

void Light::SetSpecularIntensity(float v) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetLightSpecularIntensity(ref, v);
}

float Scene::GetLightRadius(ComponentRef ref) const {
	if (const auto *c = GetComponent_(lights, ref))
		return c->radius;
	return {};
}

void Scene::SetLightRadius(ComponentRef ref, float v) {
	if (auto *c = GetComponent_(lights, ref))
		c->radius = Max(v, 0.f);
}

float Light::GetRadius() const {
	if (scene_ref && scene_ref->scene)
		return scene_ref->scene->GetLightRadius(ref);
	return {};
}

void Light::SetRadius(float v) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetLightRadius(ref, v);
}

float Scene::GetLightInnerAngle(ComponentRef ref) const {
	if (const auto *c = GetComponent_(lights, ref))
		return c->inner_angle;
	return {};
}

void Scene::SetLightInnerAngle(ComponentRef ref, float v) {
	if (auto *c = GetComponent_(lights, ref))
		c->inner_angle = Clamp(v, 0.0001f, c->outer_angle - 0.0001f);
}

float Light::GetInnerAngle() const {
	if (scene_ref && scene_ref->scene)
		return scene_ref->scene->GetLightInnerAngle(ref);
	return {};
}

void Light::SetInnerAngle(float v) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetLightInnerAngle(ref, v);
}

float Scene::GetLightOuterAngle(ComponentRef ref) const {
	if (const auto *c = GetComponent_(lights, ref))
		return c->outer_angle;
	return {};
}

void Scene::SetLightOuterAngle(ComponentRef ref, float v) {
	if (auto *c = GetComponent_(lights, ref))
		c->outer_angle = Max(v, c->inner_angle + 0.0001f);
}

float Light::GetOuterAngle() const {
	if (scene_ref && scene_ref->scene)
		return scene_ref->scene->GetLightOuterAngle(ref);
	return {};
}

void Light::SetOuterAngle(float v) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetLightOuterAngle(ref, v);
}

Vec4 Scene::GetLightPSSMSplit(ComponentRef ref) const {
	if (const auto *c = GetComponent_(lights, ref))
		return c->pssm_split;
	return {};
}

void Scene::SetLightPSSMSplit(ComponentRef ref, const Vec4 &v) {
	if (auto *c = GetComponent_(lights, ref))
		c->pssm_split = v;
}

Vec4 Light::GetPSSMSplit() const {
	if (scene_ref && scene_ref->scene)
		return scene_ref->scene->GetLightPSSMSplit(ref);
	return {};
}

void Light::SetPSSMSplit(const Vec4 &v) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetLightPSSMSplit(ref, v);
}

float Scene::GetLightPriority(ComponentRef ref) const {
	if (const auto *c = GetComponent_(lights, ref))
		return c->priority;
	return {};
}

void Scene::SetLightPriority(ComponentRef ref, float v) {
	if (auto *c = GetComponent_(lights, ref))
		c->priority = v;
}

float Light::GetPriority() const {
	if (scene_ref && scene_ref->scene)
		return scene_ref->scene->GetLightPriority(ref);
	return 0;
}

void Light::SetPriority(float v) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetLightPriority(ref, v);
}

float Scene::GetLightShadowBias(ComponentRef ref) const {
	if (const auto *c = GetComponent_(lights, ref))
		return c->shadow_bias;
	return {};
}

void Scene::SetLightShadowBias(ComponentRef ref, float v) {
	if (auto *c = GetComponent_(lights, ref))
		c->shadow_bias = v;
}

float Light::GetShadowBias() const {
	if (scene_ref && scene_ref->scene)
		return scene_ref->scene->GetLightShadowBias(ref);
	return {};
}

void Light::SetShadowBias(float v) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetLightShadowBias(ref, v);
}

//
Light Scene::CreateLinearLight(const Color &diffuse, float diffuse_intensity, const Color &specular, float specular_intensity, float priority,
	LightShadowType shadow_type, float shadow_bias, const Vec4 &pssm_split) {
	return {scene_ref,
		lights.add_ref({LT_Linear, shadow_type, diffuse, diffuse_intensity, specular, specular_intensity, 0, 0, 0, pssm_split, priority, shadow_bias})};
}

Light Scene::CreatePointLight(const float radius, const Color &diffuse, float diffuse_intensity, const Color &specular, float specular_intensity,
	float priority, LightShadowType shadow_type, float shadow_bias) {
	return {scene_ref,
		lights.add_ref({LT_Point, shadow_type, diffuse, diffuse_intensity, specular, specular_intensity, radius, 0, 0, Vec4::Zero, priority, shadow_bias})};
}

Light Scene::CreateSpotLight(const float radius, const float inner_angle, const float outer_angle, const Color &diffuse, float diffuse_intensity,
	const Color &specular, float specular_intensity, float priority, LightShadowType shadow_type, float shadow_bias) {
	return {scene_ref, lights.add_ref({LT_Spot, shadow_type, diffuse, diffuse_intensity, specular, specular_intensity, radius, inner_angle, outer_angle,
						   Vec4::Zero, priority, shadow_bias})};
}

//
bool RigidBody::IsValid() const { return scene_ref && scene_ref->scene ? scene_ref->scene->IsValidRigidBodyRef(ref) : false; }

RigidBodyType RigidBody::GetType() const { return scene_ref && scene_ref->scene ? scene_ref->scene->GetRigidBodyType(ref) : RBT_Dynamic; }

void RigidBody::SetType(RigidBodyType type) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetRigidBodyType(ref, type);
}

float RigidBody::GetLinearDamping() const { return scene_ref && scene_ref->scene ? scene_ref->scene->GetRigidBodyLinearDamping(ref) : 0.f; }

void RigidBody::SetLinearDamping(float damping) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetRigidBodyLinearDamping(ref, damping);
}

float RigidBody::GetAngularDamping() const { return scene_ref && scene_ref->scene ? scene_ref->scene->GetRigidBodyAngularDamping(ref) : 0.f; }

void RigidBody::SetAngularDamping(float damping) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetRigidBodyAngularDamping(ref, damping);
}

float RigidBody::GetRestitution() const { return scene_ref && scene_ref->scene ? scene_ref->scene->GetRigidBodyRestitution(ref) : 0.f; }

void RigidBody::SetRestitution(float restitution) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetRigidBodyRestitution(ref, restitution);
}

float RigidBody::GetFriction() const { return scene_ref && scene_ref->scene ? scene_ref->scene->GetRigidBodyFriction(ref) : 0.5f; }

void RigidBody::SetFriction(float friction) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetRigidBodyFriction(ref, friction);
}
float RigidBody::GetRollingFriction() const { return scene_ref && scene_ref->scene ? scene_ref->scene->GetRigidBodyRollingFriction(ref) : 0.f; }

void RigidBody::SetRollingFriction(float rolling_friction) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetRigidBodyRollingFriction(ref, rolling_friction);
}

//
bool Collision::IsValid() const { return scene_ref && scene_ref->scene ? scene_ref->scene->IsValidCollisionRef(ref) : false; }

CollisionType Collision::GetType() const { return scene_ref && scene_ref->scene ? scene_ref->scene->GetCollisionType(ref) : CT_Sphere; }

void Collision::SetType(CollisionType type) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetCollisionType(ref, type);
}

Mat4 Collision::GetLocalTransform() const { return scene_ref && scene_ref->scene ? scene_ref->scene->GetCollisionLocalTransform(ref) : Mat4::Identity; }

void Collision::SetLocalTransform(Mat4 m) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetCollisionLocalTransform(ref, m);
}

float Collision::GetMass() const { return scene_ref && scene_ref->scene ? scene_ref->scene->GetCollisionMass(ref) : 0.f; }

void Collision::SetMass(float mass) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetCollisionMass(ref, mass);
}

float Collision::GetRadius() const { return scene_ref && scene_ref->scene ? scene_ref->scene->GetCollisionRadius(ref) : 0.f; }

void Collision::SetRadius(float radius) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetCollisionRadius(ref, radius);
}

float Collision::GetHeight() const { return scene_ref && scene_ref->scene ? scene_ref->scene->GetCollisionHeight(ref) : 0.f; }

void Collision::SetHeight(float height) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetCollisionHeight(ref, height);
}

Vec3 Collision::GetSize() const { return scene_ref && scene_ref->scene ? scene_ref->scene->GetCollisionSize(ref) : Vec3{}; }

void Collision::SetSize(const Vec3 &size) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetCollisionSize(ref, size);
}

std::string Collision::GetCollisionResource() const { return scene_ref && scene_ref->scene ? scene_ref->scene->GetCollisionResource(ref) : std::string{}; }

void Collision::SetCollisionResource(const std::string &path) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetCollisionResource(ref, path);
}

//
bool Instance::IsValid() const { return scene_ref && scene_ref->scene ? scene_ref->scene->IsValidInstanceRef(ref) : false; }

std::string Instance::GetPath() const { return scene_ref && scene_ref->scene ? scene_ref->scene->GetInstancePath(ref) : std::string{}; }

void Instance::SetPath(const std::string &path) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetInstancePath(ref, path);
}

void Instance::SetOnInstantiateAnim(const std::string &anim) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetOnInstantiateAnim(ref, anim);
}

void Instance::SetOnInstantiateAnimLoopMode(AnimLoopMode loop_mode) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetOnInstantiateAnimLoopMode(ref, loop_mode);
}

void Instance::ClearOnInstantiateAnim() {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->ClearOnInstantiateAnim(ref);
}

std::string Instance::GetOnInstantiateAnim() { return scene_ref && scene_ref->scene ? scene_ref->scene->GetOnInstantiateAnim(ref) : std::string(); }
AnimLoopMode Instance::GetOnInstantiateAnimLoopMode() { return scene_ref && scene_ref->scene ? scene_ref->scene->GetOnInstantiateAnimLoopMode(ref) : ALM_Once; }

ScenePlayAnimRef Instance::GetOnInstantiatePlayAnimRef() {
	return scene_ref && scene_ref->scene ? scene_ref->scene->GetOnInstantiatePlayAnimRef(ref) : InvalidScenePlayAnimRef;
}

//
bool Script::IsValid() const { return scene_ref && scene_ref->scene ? scene_ref->scene->IsValidScriptRef(ref) : false; }

std::string Script::GetPath() const { return scene_ref && scene_ref->scene ? scene_ref->scene->GetScriptPath(ref) : std::string{}; }

void Script::SetPath(const std::string &path) {
	if (scene_ref && scene_ref->scene)
		scene_ref->scene->SetScriptPath(ref, path);
}

//
bool Script::HasParam(const std::string &name) const { return scene_ref && scene_ref->scene ? scene_ref->scene->ScriptHasParam(ref, name) : false; }

bool Script::SetParam(const std::string &name, ScriptParam param) {
	if (scene_ref && scene_ref->scene)
		return scene_ref->scene->SetScriptParam(ref, name, param);
	return false;
}

ScriptParam Script::GetParam(const std::string &name) const {
	return scene_ref && scene_ref->scene ? scene_ref->scene->GetScriptParam(ref, name) : ScriptParam{};
}

} // namespace hg
