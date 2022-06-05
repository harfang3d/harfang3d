// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "engine/animation.h"
#include "engine/load_save_scene_flags.h"
#include "engine/render_pipeline.h"
#include "engine/script_param.h"

#include "foundation/generational_vector_list.h"
#include "foundation/intrusive_shared_ptr_st.h"
#include "foundation/vector3.h"

#include <string>

namespace hg {

struct Node;
struct SceneRef;

using NodeRef = gen_ref;
extern const NodeRef InvalidNodeRef;

using ComponentRef = gen_ref;
extern const ComponentRef InvalidComponentRef;

using SceneAnimRef = gen_ref;
extern const SceneAnimRef InvalidSceneAnimRef;

using ScenePlayAnimRef = gen_ref;
extern const ScenePlayAnimRef InvalidScenePlayAnimRef;

struct SceneView;

//
struct TransformTRS {
	Vec3 pos{}, rot{}, scl{1, 1, 1};
};

struct Transform {
	bool IsValid() const;
	explicit operator bool() const { return IsValid(); }

	bool operator==(const Transform &o) const { return scene_ref == o.scene_ref && ref == o.ref; }

	Vec3 GetPos() const;
	void SetPos(const Vec3 &v);
	Vec3 GetRot() const;
	void SetRot(const Vec3 &v);
	Vec3 GetScale() const;
	void SetScale(const Vec3 &v);
	TransformTRS GetTRS() const;
	void SetTRS(const TransformTRS &v);
	NodeRef GetParent() const;
	void SetParent(NodeRef v);
	Node GetParentNode() const;
	void SetParentNode(const Node &n);

	void GetPosRot(Vec3 &pos, Vec3 &rot);
	void SetPosRot(const Vec3 &pos, const Vec3 &rot);

	Mat4 GetWorld() const;
	void SetWorld(const Mat4 &world);
	void SetLocal(const Mat4 &local);

	intrusive_shared_ptr_st<SceneRef> scene_ref;
	ComponentRef ref;
};

//
struct CameraZRange {
	float znear{0.01f}, zfar{1000.f};
};

/// Add this component to a Node to implement the camera aspect.
/// Create a camera component with Scene_CreateCamera, use CreateCamera to create a complete camera node.
struct Camera { // 16B on 64 bit
	bool IsValid() const;
	explicit operator bool() const { return IsValid(); }

	bool operator==(const Camera &o) const { return scene_ref == o.scene_ref && ref == o.ref; }

	float GetZNear() const;
	void SetZNear(float v);
	float GetZFar() const;
	void SetZFar(float v);
	CameraZRange GetZRange() const;
	void SetZRange(CameraZRange v);
	float GetFov() const;
	void SetFov(float v);
	bool GetIsOrthographic() const;
	void SetIsOrthographic(bool v);
	float GetSize() const;
	void SetSize(float v);

	intrusive_shared_ptr_st<SceneRef> scene_ref;
	ComponentRef ref;
};

//
struct Object { // 16B on 64 bit
	bool IsValid() const;
	explicit operator bool() const { return IsValid(); }

	bool operator==(const Object &o) const { return scene_ref == o.scene_ref && ref == o.ref; }

	ModelRef GetModelRef() const;
	void SetModelRef(ModelRef v);
	void ClearModelRef();
	Material &GetMaterial(size_t slot_idx) const;
	void SetMaterial(size_t slot_idx, Material material);
	size_t GetMaterialCount() const;
	void SetMaterialCount(size_t v);
	std::string GetMaterialName(size_t slot_idx) const;
	void SetMaterialName(size_t slot_idx, const std::string &name);

	Material *GetMaterial(const std::string &name) const;

	bool GetMinMax(const PipelineResources &resources, MinMax &minmax) const;

	size_t GetBoneCount() const;
	void SetBoneCount(size_t count);
	NodeRef GetBone(size_t idx) const;
	bool SetBone(size_t idx, NodeRef ref);
	Node GetBoneNode(size_t idx) const;
	bool SetBoneNode(size_t idx, const Node &node);

	intrusive_shared_ptr_st<SceneRef> scene_ref;
	ComponentRef ref;
};

//
enum LightType { LT_Point, LT_Spot, LT_Linear };
enum LightShadowType { LST_None, LST_Map };

struct Light { // 16B on 64 bit
	bool IsValid() const;
	explicit operator bool() const { return IsValid(); }

	bool operator==(const Light &o) const { return scene_ref == o.scene_ref && ref == o.ref; }

	LightType GetType() const;
	void SetType(LightType v);
	LightShadowType GetShadowType() const;
	void SetShadowType(LightShadowType v);
	Color GetDiffuseColor() const;
	void SetDiffuseColor(const Color &v);
	float GetDiffuseIntensity() const;
	void SetDiffuseIntensity(float v);
	Color GetSpecularColor() const;
	void SetSpecularColor(const Color &v);
	float GetSpecularIntensity() const;
	void SetSpecularIntensity(float v);
	float GetRadius() const;
	void SetRadius(float v);
	float GetInnerAngle() const;
	void SetInnerAngle(float v);
	float GetOuterAngle() const;
	void SetOuterAngle(float v);
	Vec4 GetPSSMSplit() const;
	void SetPSSMSplit(const Vec4 &v);
	float GetPriority() const;
	void SetPriority(float v);
	float GetShadowBias() const;
	void SetShadowBias(float v);

	intrusive_shared_ptr_st<SceneRef> scene_ref;
	ComponentRef ref;
};

//
enum RigidBodyType : uint8_t { RBT_Dynamic, RBT_Kinematic, RBT_Static, RBT_Last };

struct RigidBody { // 16B on 64 bit
	bool IsValid() const;
	explicit operator bool() const { return IsValid(); }

	bool operator==(const RigidBody &o) const { return scene_ref == o.scene_ref && ref == o.ref; }

	void SetType(RigidBodyType type);
	RigidBodyType GetType() const;

	float GetLinearDamping() const;
	void SetLinearDamping(float damping);
	float GetAngularDamping() const;
	void SetAngularDamping(float damping);

	float GetRestitution() const;
	void SetRestitution(float restitution);
	float GetFriction() const;
	void SetFriction(float friction);
	float GetRollingFriction() const;
	void SetRollingFriction(float rolling_friction);

	intrusive_shared_ptr_st<SceneRef> scene_ref;
	ComponentRef ref;
};

//
enum CollisionType : uint8_t { CT_Sphere, CT_Cube, CT_Cone, CT_Capsule, CT_Cylinder, CT_Mesh, CT_MeshConvex, CT_Last };

struct Collision { // 16B on 64 bit
	bool IsValid() const;
	explicit operator bool() const { return IsValid(); }

	bool operator==(const Collision &o) const { return scene_ref == o.scene_ref && ref == o.ref; }

	CollisionType GetType() const;
	void SetType(CollisionType type);

	Mat4 GetLocalTransform() const;
	void SetLocalTransform(const Mat4 &local);

	Vec3 GetPosition() const;
	void SetPosition(const Vec3 &pos);
	Vec3 GetRotation() const;
	void SetRotation(const Vec3 &rot);
	float GetMass() const;
	void SetMass(float mass);
	float GetRadius() const;
	void SetRadius(float radius);
	float GetHeight() const;
	void SetHeight(float height);
	Vec3 GetSize() const;
	void SetSize(const Vec3 &size);
	std::string GetCollisionResource() const;
	void SetCollisionResource(const std::string &name);

	float GetRestitution() const;
	void SetRestitution(float restitution);
	float GetFriction() const;
	void SetFriction(float friction);

	intrusive_shared_ptr_st<SceneRef> scene_ref;
	ComponentRef ref;
};

//
struct Instance { // 16B on 64 bit
	bool IsValid() const;
	explicit operator bool() const { return IsValid(); }

	bool operator==(const Instance &i) const { return scene_ref == i.scene_ref && ref == i.ref; }

	std::string GetPath() const;
	void SetPath(const std::string &path);

	void SetOnInstantiateAnim(const std::string &anim);
	void SetOnInstantiateAnimLoopMode(AnimLoopMode loop_mode);
	void ClearOnInstantiateAnim();

	std::string GetOnInstantiateAnim();
	AnimLoopMode GetOnInstantiateAnimLoopMode();

	ScenePlayAnimRef GetOnInstantiatePlayAnimRef();

	intrusive_shared_ptr_st<SceneRef> scene_ref;
	ComponentRef ref;
};

//
struct Script { // 16B on 64 bit
	bool IsValid() const;
	explicit operator bool() const { return IsValid(); }

	bool operator==(const Script &o) const { return scene_ref == o.scene_ref && ref == o.ref; }

	std::string GetPath() const;
	void SetPath(const std::string &path);

	bool HasParam(const std::string &name) const;
	bool SetParam(const std::string &name, ScriptParam param);
	ScriptParam GetParam(const std::string &name) const;

	intrusive_shared_ptr_st<SceneRef> scene_ref;
	ComponentRef ref;
};

//
struct Node { // 16B on 64 bit
	bool IsValid() const;
	explicit operator bool() const { return IsValid(); }

	bool operator==(const Node &o) const { return scene_ref == o.scene_ref && ref == o.ref; }

	//
	uint32_t GetUid() const { return ref.idx; }

	std::string GetName() const;
	void SetName(const std::string &v);

	uint32_t GetFlags() const;
	void SetFlags(uint32_t flags);

	//
	void Enable();
	void Disable();

	bool IsEnabled() const;
	bool IsItselfEnabled() const;

	//
	bool HasTransform() const;
	Transform GetTransform() const;
	void SetTransform(const Transform &c);
	void RemoveTransform();

	//
	bool HasCamera() const;
	Camera GetCamera() const;
	void SetCamera(const Camera &c);
	void RemoveCamera();

	ViewState ComputeCameraViewState(const Vec2 &aspect_ratio);

	//
	bool HasObject() const;
	Object GetObject() const;
	void SetObject(const Object &c);
	void RemoveObject();

	bool GetMinMax(const PipelineResources &resources, MinMax &minmax) const;

	//
	bool HasLight() const;
	Light GetLight() const;
	void SetLight(const Light &c);
	void RemoveLight();

	//
	bool HasRigidBody() const;
	RigidBody GetRigidBody() const;
	void SetRigidBody(const RigidBody &b);
	void RemoveRigidBody();

	//
	size_t GetCollisionCount() const;
	Collision GetCollision(size_t slot_idx) const;
	void SetCollision(size_t slot_idx, const Collision &col);
	void RemoveCollision(ComponentRef cref);
	void RemoveCollision(size_t slot_idx);
	void RemoveCollision(const Collision &c) { RemoveCollision(c.ref); }

	//
	bool HasInstance() const;
	Instance GetInstance() const;
	void SetInstance(const Instance &i);

	bool SetupInstance(
		const Reader &ir, const ReadProvider &ip, PipelineResources &resources, const PipelineInfo &pipeline, uint32_t flags = LSSF_AllNodeFeatures);
	bool SetupInstanceFromFile(PipelineResources &resources, const PipelineInfo &pipeline, uint32_t flags = LSSF_AllNodeFeatures);
	bool SetupInstanceFromAssets(PipelineResources &resources, const PipelineInfo &pipeline, uint32_t flags = LSSF_AllNodeFeatures);
	void DestroyInstance();

	Node IsInstantiatedBy() const;

	const SceneView &GetInstanceSceneView() const;
	SceneAnimRef GetInstanceSceneAnim(const std::string &path) const;

	void StartOnInstantiateAnim();
	void StopOnInstantiateAnim();

	//
	size_t GetScriptCount() const;
	Script GetScript(size_t slot_idx) const;
	void SetScript(size_t slot_idx, const Script &script);
	void RemoveScript(ComponentRef cref);
	void RemoveScript(size_t slot_idx);
	void RemoveScript(const Script &c) { RemoveScript(c.ref); }

	/// Get world matrix from the scene graph
	Mat4 GetWorld() const;
	/*!
		@short Set node world matrix.
		Set a node world matrix and flag it as updated so that it won't be computed by the next call to ComputeWorldMatrices().
		@note This function INTENTIONALLY does not decompose the provided matrix to the transfrom position/rotation/scale fields.
	*/
	void SetWorld(const Mat4 &world);
	/*!
		@short Compute node world matrix from scratch on-the-fly.
		This function is slow but useful when scene matrices are not yet up-to-date.
	*/
	Mat4 ComputeWorld() const;

	//
	intrusive_shared_ptr_st<SceneRef> scene_ref;
	NodeRef ref;
};

static const Node NullNode;

//
bool GetNodesMinMax(const std::vector<Node> &nodes, const PipelineResources &resources, MinMax &minmax);

} // namespace hg
