// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "engine/animation.h"
#include "engine/load_save_scene_flags.h"
#include "engine/meta.h"
#include "engine/node.h"
#include "engine/render_pipeline.h"

#include "foundation/easing.h"
#include "foundation/frustum.h"
#include "foundation/generational_vector_list.h"
#include "foundation/matrix4.h"
#include "foundation/matrix44.h"
#include "foundation/pack_float.h"
#include "foundation/rw_interface.h"
#include "foundation/signal.h"
#include "foundation/time.h"
#include "foundation/unit.h"

#include <functional>
#include <limits>
#include <memory>
#include <vector>

namespace hg {

class Scene;

//
struct NodesChildren {
	std::vector<NodeRef> GetChildren(NodeRef node) const;

private:
	friend class Scene;

	struct NodeChildren {
		uint32_t count, offset;
	};

	std::map<NodeRef, NodeChildren> node_children;
	std::vector<NodeRef> all_children;
};

//
extern time_ns UnspecifiedAnimTime;

// low-level anim
using AnimRef = gen_ref;
extern const AnimRef InvalidAnimRef;

enum NodeBoolAnimTarget { NBAT_Enable, NBAT_Count };
enum NodeFloatAnimTarget { NFAT_LightDiffuseIntensity, NFAT_LightSpecularIntensity, NFAT_CameraFov, NFAT_Count };
enum NodeVec3AnimTarget { NV3AT_TransformPosition, NV3AT_TransformRotation, NV3AT_TransformScale, NV3AT_Count };
enum NodeVec4AnimTarget { NV4AT_Count };
enum NodeQuatAnimTarget { NQAT_TransformRotation, NQAT_Count };
enum NodeColorAnimTarget { NCAT_LightDiffuse, NCAT_LightSpecular, NCAT_Count };
enum NodeStringAnimTarget { NSAT_Count };

struct BoundToNodeMaterialAnim {
	int8_t track_idx; // anim track idx
	uint8_t slot_idx; // material slot idx
	std::string value; // material value name
};

struct SceneBoundAnim;

struct BoundToNodeInstanceAnim {
	int kf;
	std::shared_ptr<SceneBoundAnim> bound_anim; // circular definition mandates a heap allocation
};

struct BoundToNodeAnim {
	std::array<int8_t, NBAT_Count> bool_track;
	std::array<int8_t, NFAT_Count> float_track;
	std::array<int8_t, NV3AT_Count> vec3_track;
	std::array<int8_t, NV4AT_Count> vec4_track;
	std::array<int8_t, NQAT_Count> quat_track;
	std::array<int8_t, NCAT_Count> color_track;

	NodeRef node; // 8B
	AnimRef anim; // 8B

	std::vector<BoundToNodeMaterialAnim> vec4_mat_track;

	mutable BoundToNodeInstanceAnim bound_to_node_instance_anim;
};

enum SceneFloatAnimTarget { SFAT_FogNear, SFAT_FogFar, SFAT_Count };
enum SceneColorAnimTarget { SCAT_FogColor, SCAT_AmbientColor, SCAT_Count };

struct BoundToSceneAnim {
	std::array<int8_t, SFAT_Count> float_track;
	std::array<int8_t, SCAT_Count> color_track;

	AnimRef anim; // 8B
};

//
struct NodeAnim { // 16B
	NodeRef node;
	AnimRef anim;
};

// non-serialized scene anim flags
static const uint8_t SAF_Instantiated = 0x1;

struct SceneAnim {
	std::string name;
	time_ns t_start{}, t_end{};

	AnimRef scene_anim;
	std::vector<NodeAnim> node_anims;

	time_ns frame_duration{time_from_ms(1000 / 20)}; // default to 20 fps
	uint8_t flags{};
};

using SceneAnimRef = gen_ref;
extern const SceneAnimRef InvalidSceneAnimRef;

struct SceneBoundAnim {
	BoundToSceneAnim bound_scene_anim;
	std::vector<BoundToNodeAnim> bound_node_anims;
};

//
using ScenePlayAnimRef = gen_ref;
extern const ScenePlayAnimRef InvalidScenePlayAnimRef;

//
enum ProbeType : uint8_t { PT_Sphere, PT_Cube, PT_Count };

struct Probe {
	TextureRef irradiance_map;
	TextureRef radiance_map;

	ProbeType type{PT_Sphere};
	uint8_t parallax{0};
	TransformTRS trs;
};

//
struct SceneView {
	std::vector<NodeRef> nodes;

	std::vector<AnimRef> anims;
	std::vector<SceneAnimRef> scene_anims;

	std::vector<Node> GetNodes(const Scene &scene) const;
	Node GetNode(const Scene &scene, const std::string &name) const;

	SceneAnimRef GetSceneAnim(const Scene &scene, const std::string &name) const;
};

struct LoadSceneContext {
	int recursion_level{};
	SceneView view;
	std::map<uint32_t, NodeRef> node_refs;
};

enum NodeComponentIdx { NCI_Transform, NCI_Camera, NCI_Object, NCI_Light, NCI_RigidBody, NCI_Count };

// serialized node flags
static const uint32_t NF_SerializedMask = 0x0000ffff;
static const uint32_t NF_Disabled = 0x00000001; // node is disabled
// non-serialized node flags
static const uint32_t NF_Instantiated = 0x00010000; // node was instantiated
static const uint32_t NF_InstanceDisabled = 0x00020000; // node is disabled through the node that instantiated it

//
class Scene {
public:
	Scene();
	~Scene();

	/// Clear all scene content. All external references are invalidated.
	void Clear();

	/**
		@short Clear orphaned scene content.

		Call this method after removing nodes from the scene. Components with
		no reference will be removed from the scene. Try to batch node removal
		and perform a single call to this function to improve performance.
	**/
	size_t GarbageCollect();

	// node
	Node CreateNode(std::string name = {});
	void DestroyNode(NodeRef ref);
	void DestroyNode(const Node &node) { DestroyNode(node.ref); }

	Node GetNode(const std::string &name) const;
	Node GetNode(NodeRef ref) const { return {scene_ref, ref}; }
	Node GetNodeEx(const std::string &path) const;

	std::vector<NodeRef> GetNodeChildRefs(NodeRef ref) const;

	std::vector<Node> GetNodeChildren(NodeRef ref) const;
	std::vector<Node> GetNodeChildren(const Node &node) const { return GetNodeChildren(node.ref); }

	NodeRef GetNodeRef(uint32_t idx) const { return nodes.get_ref(idx); }

	size_t GetNodeCount() const;
	size_t GetAllNodeCount() const;

	std::vector<Node> GetNodes() const;
	std::vector<Node> GetAllNodes() const;
	std::vector<Node> GetNodesWithComponent(NodeComponentIdx idx) const;
	std::vector<Node> GetAllNodesWithComponent(NodeComponentIdx idx) const;

	std::string GetNodeName(NodeRef ref) const;
	void SetNodeName(NodeRef ref, const std::string &v);

	uint32_t GetNodeFlags(NodeRef ref) const;
	void SetNodeFlags(NodeRef ref, uint32_t flags);

	void ReserveNodes(size_t count);

	bool IsValidNodeRef(NodeRef ref) const { return nodes.is_valid(ref); }

	void EnableNode(NodeRef ref);
	void DisableNode(NodeRef ref);

	bool IsNodeEnabled(NodeRef ref) const { return nodes.is_valid(ref) ? !(nodes[ref.idx].flags & (NF_Disabled | NF_InstanceDisabled)) : false; }
	bool IsNodeItselfEnabled(NodeRef ref) const { return nodes.is_valid(ref) ? !(nodes[ref.idx].flags & NF_Disabled) : false; }

	bool IsChildOf(NodeRef ref, NodeRef parent) const;
	bool IsChildOf(const Node &node, const Node &parent) const { return IsChildOf(node.ref, parent.ref); }

	bool IsRoot(NodeRef ref) const { return IsChildOf(ref, InvalidNodeRef); }
	bool IsRoot(const Node &node) const { return IsChildOf(node.ref, InvalidNodeRef); }

	NodeRef IsInstantiatedBy(NodeRef ref) const;

	// Return a list of child node reference per node index. Valid until any modification is done on the scene graph.
	NodesChildren BuildNodesChildren() const;

	// transform component
	Transform CreateTransform();
	void DestroyTransform(ComponentRef ref);
	void DestroyTransform(const Transform &t) { DestroyTransform(t.ref); }
	Vec3 GetTransformPos(ComponentRef ref) const;
	void SetTransformPos(ComponentRef ref, const Vec3 &v);
	Vec3 GetTransformRot(ComponentRef ref) const;
	void SetTransformRot(ComponentRef ref, const Vec3 &v);
	Vec3 GetTransformScale(ComponentRef ref) const;
	void SetTransformScale(ComponentRef ref, const Vec3 &v);
	TransformTRS GetTransformTRS(ComponentRef ref) const;
	void SetTransformTRS(ComponentRef ref, const TransformTRS &v);
	NodeRef GetTransformParent(ComponentRef ref) const;
	void SetTransformParent(ComponentRef ref, const NodeRef &v);

	void SetTransformLocalMatrix(ComponentRef ref, const Mat4 &local);
	/*!
		Set and decompose transform world matrix.
		The provided matrix is decomposed over the position, rotation and scale members of the Transform.
		@see SetNodeWorldMatrix to set a node world matrix in the scene graph without affecting the Transform component.
	*/
	void SetTransformWorldMatrix(ComponentRef ref, const Mat4 &world);

	Transform CreateTransform(const Vec3 &pos, const Vec3 &rot = {0, 0, 0}, const Vec3 &scl = {1, 1, 1}, NodeRef parent = {});
	Transform CreateTransform(const Mat4 &mtx, NodeRef parent = {});

	void ReserveTransforms(size_t count);

	bool IsValidTransformRef(ComponentRef ref) const { return transforms.is_valid(ref); }

	ComponentRef GetNodeTransformRef(NodeRef ref) const;
	void SetNodeTransform(NodeRef ref, ComponentRef cref);
	Transform GetNodeTransform(NodeRef ref) const { return {scene_ref, GetNodeTransformRef(ref)}; }
	void SetNodeTransform(NodeRef ref, const Transform &v) { SetNodeTransform(ref, v.ref); }

	Mat4 GetNodeWorldMatrix(NodeRef ref) const;
	/// Set node world matrix.
	/// Set a node world matrix and flag it as updated so that it won't be computed by the next call to ComputeWorldMatrices().
	/// @note This function INTENTIONALLY does not decompose the provided matrix to the transfrom position/rotation/scale fields.
	void SetNodeWorldMatrix(NodeRef ref, const Mat4 &world);

	/// Compute node world matrix from scratch on-the-fly.
	/// This function is slow but useful when scene matrices are not yet up-to-date.
	Mat4 ComputeNodeWorldMatrix(NodeRef ref) const;

	//
	void StorePreviousWorldMatrices();
	void ReadyWorldMatrices();
	void ComputeWorldMatrices();
	void FixupPreviousWorldMatrices();

	void Update(time_ns dt);

	// camera component
	/// Create a new Node with a Transform and Camera components.
	Camera CreateCamera();
	void DestroyCamera(ComponentRef ref);
	void DestroyCamera(const Camera &c) { DestroyCamera(c.ref); }
	float GetCameraZNear(ComponentRef ref) const;
	void SetCameraZNear(ComponentRef ref, float v);
	float GetCameraZFar(ComponentRef ref) const;
	void SetCameraZFar(ComponentRef ref, float v);
	CameraZRange GetCameraZRange(ComponentRef ref) const;
	void SetCameraZRange(ComponentRef ref, CameraZRange v);
	float GetCameraFov(ComponentRef ref) const;
	void SetCameraFov(ComponentRef ref, float v);
	float GetCameraSize(ComponentRef ref) const;
	void SetCameraSize(ComponentRef ref, float v);
	bool GetCameraIsOrthographic(ComponentRef ref) const;
	void SetCameraIsOrthographic(ComponentRef ref, const bool &v);

	Camera CreateCamera(float znear, float zfar, float fov = Deg(45.f));
	Camera CreateOrthographicCamera(float znear, float zfar, float size = 1.f);

	void ReserveCameras(size_t count);

	bool IsValidCameraRef(ComponentRef ref) const { return cameras.is_valid(ref); }

	ComponentRef GetNodeCameraRef(NodeRef ref) const;
	void SetNodeCamera(NodeRef ref, ComponentRef cref);
	Camera GetNodeCamera(NodeRef ref) const { return {scene_ref, GetNodeCameraRef(ref)}; }
	void SetNodeCamera(NodeRef ref, const Camera &v) { SetNodeCamera(ref, v.ref); }

	// object component
	Object CreateObject();
	void DestroyObject(ComponentRef ref);
	void DestroyObject(const Object &o) { DestroyObject(o.ref); }
	ModelRef GetObjectModel(ComponentRef ref) const;
	void SetObjectModel(ComponentRef ref, const ModelRef &v);
	size_t GetObjectMaterialCount(ComponentRef ref) const;
	void SetObjectMaterialCount(ComponentRef ref, size_t v);

	bool GetObjectMinMax(ComponentRef ref, const PipelineResources &resources, MinMax &minmax) const;

	Material &GetObjectMaterial(ComponentRef ref, size_t slot_idx);
	void SetObjectMaterial(ComponentRef ref, size_t slot_idx, Material material);
	std::string GetObjectMaterialName(ComponentRef ref, size_t slot_idx) const;
	void SetObjectMaterialName(ComponentRef ref, size_t slot_idx, const std::string &name);

	Material *GetObjectMaterial(ComponentRef ref, const std::string &name);

	size_t GetObjectBoneCount(ComponentRef ref) const;
	void SetObjectBoneCount(ComponentRef ref, size_t count);
	bool SetObjectBone(ComponentRef ref, size_t idx, NodeRef bone_node);
	NodeRef GetObjectBone(ComponentRef ref, size_t idx) const;

	Object CreateObject(const ModelRef &model, std::vector<Material> materials = {});

	void ReserveObjects(size_t count);

	bool IsValidObjectRef(ComponentRef ref) const { return objects.is_valid(ref); }

	ComponentRef GetNodeObjectRef(NodeRef ref) const;
	void SetNodeObject(NodeRef ref, ComponentRef cref);
	Object GetNodeObject(NodeRef ref) const { return {scene_ref, GetNodeObjectRef(ref)}; }
	void SetNodeObject(NodeRef ref, const Object &v) { SetNodeObject(ref, v.ref); }

	// light component
	Light CreateLight();
	void DestroyLight(ComponentRef ref);
	void DestroyLight(const Light &l) { DestroyLight(l.ref); }
	LightType GetLightType(ComponentRef ref) const;
	void SetLightType(ComponentRef ref, LightType v);
	LightShadowType GetLightShadowType(ComponentRef ref) const;
	void SetLightShadowType(ComponentRef ref, LightShadowType v);
	Color GetLightDiffuseColor(ComponentRef ref) const;
	void SetLightDiffuseColor(ComponentRef ref, const Color &v);
	float GetLightDiffuseIntensity(ComponentRef ref) const;
	void SetLightDiffuseIntensity(ComponentRef ref, float v);
	Color GetLightSpecularColor(ComponentRef ref) const;
	void SetLightSpecularColor(ComponentRef ref, const Color &v);
	float GetLightSpecularIntensity(ComponentRef ref) const;
	void SetLightSpecularIntensity(ComponentRef ref, float v);
	float GetLightRadius(ComponentRef ref) const;
	void SetLightRadius(ComponentRef ref, float v);
	float GetLightInnerAngle(ComponentRef ref) const;
	void SetLightInnerAngle(ComponentRef ref, float v);
	float GetLightOuterAngle(ComponentRef ref) const;
	void SetLightOuterAngle(ComponentRef ref, float v);
	Vec4 GetLightPSSMSplit(ComponentRef ref) const;
	void SetLightPSSMSplit(ComponentRef ref, const Vec4 &v);
	float GetLightPriority(ComponentRef ref) const;
	void SetLightPriority(ComponentRef ref, float v);
	float GetLightShadowBias(ComponentRef ref) const;
	void SetLightShadowBias(ComponentRef ref, float v);

	/// Create a Node with a Transform and a point Light component.
	Light CreatePointLight(float radius, const Color &diffuse = {1, 1, 1}, float diffuse_intensity = 1, const Color &specular = {1, 1, 1},
		float specular_intensity = 1, float priority = 0, LightShadowType shadow_type = LST_None, float shadow_bias = default_shadow_bias);
	/// Create a Node with a Transform and a spot Light component.
	Light CreateSpotLight(float radius, float inner_angle, float outer_angle, const Color &diffuse = {1, 1, 1}, float diffuse_intensity = 1,
		const Color &specular = {1, 1, 1}, float specular_intensity = 1, float priority = 0, LightShadowType shadow_type = LST_None,
		float shadow_bias = default_shadow_bias);
	/// Create a Node with a Transform and a linear Light component.
	Light CreateLinearLight(const Color &diffuse = {1, 1, 1}, float diffuse_intensity = 1, const Color &specular = {1, 1, 1}, float specular_intensity = 1,
		float priority = 0, LightShadowType shadow_type = LST_None, float shadow_bias = default_shadow_bias, const Vec4 &pssm_split = default_pssm_split);

	Light CreatePointLight(float radius, const Color &diffuse, const Color &specular, float priority = 0, LightShadowType shadow_type = LST_None,
		float shadow_bias = default_shadow_bias) {
		return CreatePointLight(radius, diffuse, 1, specular, 1, priority, shadow_type, shadow_bias);
	}
	Light CreateSpotLight(float radius, float inner_angle, float outer_angle, const Color &diffuse, const Color &specular, float priority = 0,
		LightShadowType shadow_type = LST_None, float shadow_bias = default_shadow_bias) {
		return CreateSpotLight(radius, inner_angle, outer_angle, diffuse, 1, specular, 1, priority, shadow_type, shadow_bias);
	}
	Light CreateLinearLight(const Color &diffuse, const Color &specular, float priority = 0, LightShadowType shadow_type = LST_None,
		float shadow_bias = default_shadow_bias, const Vec4 &pssm_split = default_pssm_split) {
		return CreateLinearLight(diffuse, 1, specular, 1, priority, shadow_type, shadow_bias, pssm_split);
	}

	std::vector<Node> GetLights() const;

	void ReserveLights(size_t count);
	bool IsValidLightRef(ComponentRef ref) const { return lights.is_valid(ref); }

	ComponentRef GetNodeLightRef(NodeRef ref) const;
	void SetNodeLight(NodeRef ref, ComponentRef cref);
	Light GetNodeLight(NodeRef ref) const { return {scene_ref, GetNodeLightRef(ref)}; }
	void SetNodeLight(NodeRef ref, const Light &v) { SetNodeLight(ref, v.ref); }

	// rigid body component
	RigidBody CreateRigidBody();
	void DestroyRigidBody(ComponentRef ref);
	void DestroyRigidBody(const RigidBody &r) { DestroyRigidBody(r.ref); }

	RigidBodyType GetRigidBodyType(ComponentRef ref) const;
	void SetRigidBodyType(ComponentRef ref, RigidBodyType type);

	ComponentRef GetNodeRigidBodyRef(NodeRef ref) const;
	void SetNodeRigidBody(NodeRef ref, ComponentRef cref);
	RigidBody GetNodeRigidBody(NodeRef ref) const { return {scene_ref, GetNodeRigidBodyRef(ref)}; }
	void SetNodeRigidBody(NodeRef ref, const RigidBody &v) { SetNodeRigidBody(ref, v.ref); }

	bool IsValidRigidBodyRef(ComponentRef ref) const { return rigid_bodies.is_valid(ref); }

	float GetRigidBodyLinearDamping(ComponentRef ref) const;
	void SetRigidBodyLinearDamping(ComponentRef ref, float damping);
	float GetRigidBodyAngularDamping(ComponentRef ref) const;
	void SetRigidBodyAngularDamping(ComponentRef ref, float damping);

	float GetRigidBodyRestitution(ComponentRef ref) const;
	void SetRigidBodyRestitution(ComponentRef ref, float restitution);
	float GetRigidBodyFriction(ComponentRef ref) const;
	void SetRigidBodyFriction(ComponentRef ref, float friction);
	float GetRigidBodyRollingFriction(ComponentRef ref) const;
	void SetRigidBodyRollingFriction(ComponentRef ref, float rolling_friction);

	// collision component
	Collision CreateCollision();
	void DestroyCollision(ComponentRef ref);
	void DestroyCollision(const Collision &s) { DestroyCollision(s.ref); }

	Mat4 GetCollisionLocalTransform(ComponentRef ref) const;
	void SetCollisionLocalTransform(ComponentRef ref, const Mat4 &local);

	CollisionType GetCollisionType(ComponentRef ref) const;
	void SetCollisionType(ComponentRef ref, CollisionType type);
	Vec3 GetCollisionPosition(ComponentRef ref) const;
	void SetCollisionPosition(ComponentRef ref, const Vec3 &pos);
	Vec3 GetCollisionRotation(ComponentRef ref) const;
	void SetCollisionRotation(ComponentRef ref, const Vec3 &rot);
	float GetCollisionMass(ComponentRef ref) const;
	void SetCollisionMass(ComponentRef ref, float mass);
	Vec3 GetCollisionSize(ComponentRef ref) const;
	void SetCollisionSize(ComponentRef ref, const Vec3 &size);
	float GetCollisionRadius(ComponentRef ref) const;
	void SetCollisionRadius(ComponentRef ref, float radius);
	float GetCollisionHeight(ComponentRef ref) const;
	void SetCollisionHeight(ComponentRef ref, float radius);
	std::string GetCollisionResource(ComponentRef ref);
	void SetCollisionResource(ComponentRef ref, const std::string &path);

	Collision CreateSphereCollision(float radius, float mass = Kg(1.f));
	Collision CreateCubeCollision(float x, float y, float z, float mass = Kg(1.f));
	Collision CreateCapsuleCollision(float radius, float height, float mass = Kg(1.f));
	Collision CreateCylinderCollision(float radius, float height, float mass = Kg(1.f));
	Collision CreateConeCollision(float radius, float height, float mass = Kg(1.f));
	Collision CreateMeshCollision(const std::string &collision_path, float mass = Kg(1.f));
	Collision CreateMeshConvexCollision(const std::string &collision_path, float mass = Kg(1.f));

	size_t GetNodeCollisionCount(NodeRef ref) const;
	Collision GetNodeCollision(NodeRef ref, size_t idx) const;
	ComponentRef GetNodeCollisionRef(NodeRef ref, size_t idx) const;
	void SetNodeCollision(NodeRef ref, size_t idx, const Collision &col);
	void RemoveNodeCollision(NodeRef ref, ComponentRef cref);
	void RemoveNodeCollision(NodeRef ref, size_t idx);

	bool IsValidCollisionRef(ComponentRef ref) const { return collisions.is_valid(ref); }

	// instance component
	Instance CreateInstance();
	void DestroyInstance(ComponentRef ref);
	void DestroyInstance(const Instance &i) { DestroyInstance(i.ref); }

	bool IsValidInstanceRef(ComponentRef ref) const { return instances.is_valid(ref); }

	void SetInstancePath(ComponentRef ref, const std::string &path);
	std::string GetInstancePath(ComponentRef ref) const;

	void SetOnInstantiateAnim(ComponentRef ref, const std::string &anim);
	void SetOnInstantiateAnimLoopMode(ComponentRef ref, AnimLoopMode loop_mode);
	void ClearOnInstantiateAnim(ComponentRef ref);

	std::string GetOnInstantiateAnim(ComponentRef ref);
	AnimLoopMode GetOnInstantiateAnimLoopMode(ComponentRef ref);
	ScenePlayAnimRef GetOnInstantiatePlayAnimRef(ComponentRef ref);

	Instance GetNodeInstance(NodeRef ref) const;
	ComponentRef GetNodeInstanceRef(NodeRef ref) const;
	void SetNodeInstance(NodeRef ref, ComponentRef cref);
	void SetNodeInstance(NodeRef ref, const Instance &instance) { SetNodeInstance(ref, instance.ref); }

	bool NodeSetupInstance(NodeRef ref, const Reader &ir, const ReadProvider &ip, PipelineResources &resources, const PipelineInfo &pipeline,
		uint32_t flags = LSSF_AllNodeFeatures, int recursion_level = 1);
	bool NodeSetupInstanceFromFile(
		NodeRef ref, PipelineResources &resources, const PipelineInfo &pipeline, uint32_t flags = LSSF_AllNodeFeatures, int recursion_level = 1);
	bool NodeSetupInstanceFromAssets(
		NodeRef ref, PipelineResources &resources, const PipelineInfo &pipeline, uint32_t flags = LSSF_AllNodeFeatures, int recursion_level = 1);
	void NodeDestroyInstance(NodeRef ref);
	void NodeMoveInstance(NodeRef from, NodeRef to);

	ScenePlayAnimRef NodeStartOnInstantiateAnim(NodeRef ref);
	void NodeStopOnInstantiateAnim(NodeRef ref);

	const SceneView &GetNodeInstanceSceneView(NodeRef ref) const;

	Instance CreateInstance(const std::string &path);

	// scripts
	/// Helper function to create a Node with a Script component.
	Script CreateScript();
	Script CreateScript(const std::string &path);
	void DestroyScript(ComponentRef ref);
	void DestroyScript(const Script &s) { DestroyScript(s.ref); }
	Script GetScript(ComponentRef ref) const { return {scene_ref, ref}; }

	void SetScriptPath(ComponentRef ref, const std::string &path);
	std::string GetScriptPath(ComponentRef ref) const;

	bool ScriptHasParam(ComponentRef ref, const std::string &name) const;
	ScriptParam GetScriptParam(ComponentRef ref, const std::string &name) const;
	bool SetScriptParam(ComponentRef ref, const std::string &name, ScriptParam param);

	const std::map<std::string, ScriptParam> &GetScriptParams(ComponentRef ref) const;

	size_t GetNodeScriptCount(NodeRef ref) const;
	Script GetNodeScript(NodeRef ref, size_t idx) const;
	ComponentRef GetNodeScriptRef(NodeRef ref, size_t idx) const;
	void SetNodeScript(NodeRef ref, size_t idx, const Script &script);
	void RemoveNodeScript(NodeRef ref, ComponentRef cref);
	void RemoveNodeScript(NodeRef ref, size_t idx);

	void ReserveScripts(size_t count);
	bool IsValidScriptRef(ComponentRef ref) const { return scripts.is_valid(ref); }

	std::vector<Script> GetScripts() const;
	std::vector<ComponentRef> GetScriptRefs() const;

	size_t GetScriptCount() const;
	void SetScript(size_t slot_idx, const Script &script);
	Script GetScript(size_t slot_idx) const;

	const std::vector<ComponentRef> &GetSceneScripts() const { return scene_scripts; }
	const std::map<NodeRef, std::vector<ComponentRef>> &GetNodeScripts() const { return node_scripts; }

	/// Holds the canvas properties of a scene, see the `canvas` member of class Scene.
	struct Canvas {
		bool clear_z{true}, clear_color{true};
		Color color{0.f, 0.f, 0.f, 1.f};
	};

	Canvas canvas;

	/// Environment properties of a scene.
	/// @see Scene::environment member of the Scene class.
	struct Environment {
		Color ambient;
		Color fog_color;
		float fog_near, fog_far;

		Probe probe;
		TextureRef brdf_map;
	};

	Environment environment{};

	// scene state
	Node GetCurrentCamera() const { return {scene_ref, current_camera}; }
	void SetCurrentCamera(NodeRef ref) { current_camera = ref; }
	void SetCurrentCamera(const Node &n) { current_camera = n.ref; }

	ViewState ComputeCurrentCameraViewState(const Vec2 &aspect_ratio) const;
	ViewState ComputeCameraViewState(NodeRef ref, const Vec2 &aspect_ratio) const;

	const std::vector<Mat4> &GetTransformWorldMatrices() const { return transform_worlds; }
	void StorePreviousTransformWorldMatrices();
	const std::vector<Mat4> &GetPreviousTransformWorldMatrices() const { return previous_transform_worlds; }

	Mat4 GetTransformWorldMatrix(const uint32_t transform_idx) const;
	Mat4 GetPreviousTransformWorldMatrix(const uint32_t transform_idx) const;

	// view
	void DestroyViewContent(const SceneView &view);

	// rendering
	void GetModelDisplayLists(std::vector<ModelDisplayList> &out_opaque, std::vector<ModelDisplayList> &out_transparent,
		std::vector<SkinnedModelDisplayList> &out_opaque_skinned, std::vector<SkinnedModelDisplayList> &out_transparent_skinned,
		const PipelineResources &resources) const;

	//
	bool GetMinMax(const PipelineResources &resources, MinMax &minmax) const;

	// low-level animation
	AnimRef AddAnim(Anim anim);
	void DestroyAnim(AnimRef ref);
	bool IsValidAnim(AnimRef ref) const { return anims.is_valid(ref); }

	std::vector<AnimRef> GetAnims() const;
	Anim *GetAnim(AnimRef ref);
	const Anim *GetAnim(AnimRef ref) const;

	AnimRef GetAnimRef(uint32_t idx) const { return anims.get_ref(idx); }

	BoundToSceneAnim BindSceneAnim(AnimRef ref) const;
	void EvaluateBoundAnim(const BoundToSceneAnim &bound_anim, time_ns t);
	BoundToNodeAnim BindNodeAnim(NodeRef node_ref, AnimRef anim_ref) const;
	void EvaluateBoundAnim(const BoundToNodeAnim &bound_anim, time_ns t);

	// scene animation
	SceneAnimRef AddSceneAnim(SceneAnim anim);
	void DestroySceneAnim(SceneAnimRef ref);
	bool IsValidSceneAnim(SceneAnimRef ref) const { return scene_anims.is_valid(ref); }

	std::vector<SceneAnimRef> GetSceneAnims() const;

	SceneAnim *GetSceneAnim(SceneAnimRef ref);
	const SceneAnim *GetSceneAnim(SceneAnimRef ref) const;
	SceneAnimRef GetSceneAnim(const char *name) const;

	SceneBoundAnim BindAnim(const SceneAnim &scene_anim) const;
	SceneBoundAnim BindAnim(SceneAnimRef ref) const;
	void EvaluateBoundAnim(const SceneBoundAnim &bound_anim, time_ns t);

	ScenePlayAnimRef PlayAnim(SceneAnimRef ref, AnimLoopMode loop_mode = ALM_Once, Easing easing = E_Linear, time_ns t_start = UnspecifiedAnimTime,
		time_ns t_end = UnspecifiedAnimTime, bool paused = false, float t_scale = 1.f);
	bool IsPlaying(ScenePlayAnimRef ref) const;
	void StopAnim(ScenePlayAnimRef ref);
	void StopAllAnims();

	std::vector<std::string> GetPlayingAnimNames() const;
	std::vector<ScenePlayAnimRef> GetPlayingAnimRefs() const;

	void UpdatePlayingAnims(time_ns dt);

	SceneAnimRef DuplicateSceneAnim(SceneAnimRef ref);

	size_t GarbageCollectAnims();

	// scene meta
	bool HasKey(const std::string &key) const;
	std::vector<std::string> GetKeys() const;
	void RemoveKey(const std::string &key);

	std::string GetValue(const std::string &key) const;
	void SetValue(const std::string &key, const std::string &value);

	// serialization (member as we directly access low-level structures for better performances)
	bool Save_binary(const Writer &iw, const Handle &h, const PipelineResources &resources, uint32_t flags = LSSF_All,
		const std::vector<NodeRef> *nodes_to_save = nullptr) const;
	bool Load_binary(const Reader &ir, const Handle &h, const char *name, const Reader &deps_ir, const ReadProvider &deps_ip, PipelineResources &resources,
		const PipelineInfo &pipeline, LoadSceneContext &ctx, uint32_t flags = LSSF_All);

	bool SaveNodes_binary(const Writer &iw, const Handle &h, const std::vector<NodeRef> &nodes_to_save, const PipelineResources &resources) const;
	bool LoadNodes_binary(const Reader &ir, const Handle &h, const char *name, const Reader &deps_ir, const ReadProvider &deps_ip, PipelineResources &resources,
		const PipelineInfo &pipeline, LoadSceneContext &ctx);

	//
	bool Save_json(json &js, const PipelineResources &resources, uint32_t flags = LSSF_All, const std::vector<NodeRef> *nodes_to_save = nullptr) const;
	bool Load_json(const json &js, const char *name, const Reader &deps_ir, const ReadProvider &deps_ip, PipelineResources &resources,
		const PipelineInfo &pipeline, LoadSceneContext &ctx, uint32_t flags = LSSF_All);

private:
	std::map<std::string, std::string> key_values;

	friend void DumpSceneMemoryFootprint();

	intrusive_shared_ptr_st<SceneRef> scene_ref;

	size_t GarbageCollectPass();

	// nodes
	struct Node_ { // 52B
		std::string name; // 8B
		ComponentRef components[NCI_Count]; // 40B
		uint32_t flags; // 4B
	};

	generational_vector_list<Node_> nodes;

	inline Node_ *GetNode_(NodeRef ref) { return nodes.is_valid(ref) ? &nodes[ref.idx] : nullptr; }
	inline const Node_ *GetNode_(NodeRef ref) const { return nodes.is_valid(ref) ? &nodes[ref.idx] : nullptr; }

	NodeRef GetNodeEx_(const std::vector<NodeRef> &refs, const std::string &path) const;

	void EnableNode_(NodeRef ref, bool through_instance);
	void DisableNode_(NodeRef ref, bool through_instance);

	// component helpers
	template <int I> inline ComponentRef GetNodeComponentRef_(NodeRef ref) const {
		auto node_ = this->GetNode_(ref);
		return node_ ? node_->components[I] : ComponentRef{};
	}

	template <int I> inline void SetNodeComponentRef_(NodeRef nref, ComponentRef cref) {
		if (auto node_ = this->GetNode_(nref))
			node_->components[I] = cref;
	}

	template <typename T> inline auto GetComponent_(generational_vector_list<T> &l, ComponentRef ref) -> T * {
		return l.is_valid(ref) ? &l.value(ref.idx) : nullptr;
	}

	template <typename T> inline auto GetComponent_(const generational_vector_list<T> &l, ComponentRef ref) const -> const T * {
		return l.is_valid(ref) ? &l.value(ref.idx) : nullptr;
	}

	// components
	struct Transform_ {
		TransformTRS TRS;
		NodeRef parent;
	};

	struct Camera_ {
		CameraZRange zrange;
		float fov{Deg(40.f)};
		bool ortho{false};
		float size{1.f};
	};

	struct Object_ {
		ModelRef model;
		std::vector<Material> materials;

		struct MaterialInfo {
			std::string name;
		};
		std::vector<MaterialInfo> material_infos;

		std::vector<NodeRef> bones;
	};

	struct Light_ {
		LightType type{LT_Point};
		LightShadowType shadow_type{LST_None};

		Color diffuse{1.f, 1.f, 1.f, 1.f};
		float diffuse_intensity{1.f};
		Color specular{1.f, 1.f, 1.f, 1.f};
		float specular_intensity{1.f};
		float radius{0.f};
		float inner_angle{Deg(30.f)}, outer_angle{Deg(45.f)};

		Vec4 pssm_split{10.f, 50.f, 100.f, 200.f};
		float priority{0.f};

		float shadow_bias{default_shadow_bias};
	};

	struct RigidBody_ { // 6B
		RigidBodyType type{RBT_Dynamic};

		uint8_t linear_damping{pack_float<uint8_t>(0.f)};
		uint8_t angular_damping{pack_float<uint8_t>(0.f)};

		uint8_t restitution{pack_float<uint8_t>(0.f)};
		uint8_t friction{pack_float<uint8_t>(0.5f)};
		uint8_t rolling_friction{pack_float<uint8_t>(0.f)};
	};

	generational_vector_list<Transform_> transforms;
	generational_vector_list<Camera_> cameras;
	generational_vector_list<Object_> objects;
	generational_vector_list<Light_> lights;
	generational_vector_list<RigidBody_> rigid_bodies;

	//
	struct Collision_ {
		CollisionType type;
		float mass;
		std::string resource_path;
		TransformTRS trs;
	};

	generational_vector_list<Collision_> collisions;
	std::map<NodeRef, std::vector<ComponentRef>> node_collisions;

	//
	struct Script_ {
		std::string path;
		std::map<std::string, ScriptParam> params;
	};

	generational_vector_list<Script_> scripts;

	std::vector<ComponentRef> scene_scripts;
	std::map<NodeRef, std::vector<ComponentRef>> node_scripts;

	//
	struct Instance_ {
		std::string name;
		std::string anim; // animation to start upon instantiation
		AnimLoopMode loop_mode{ALM_Once};
		ScenePlayAnimRef play_anim_ref;
	};

	generational_vector_list<Instance_> instances; // create/destroy

	std::map<NodeRef, ComponentRef> node_instance; // node to instance component
	std::map<NodeRef, SceneView> node_instance_view; // node to instance scene view

	//
	friend void LoadComponent(Transform_ *data_, const Reader &ir, const Handle &h);
	friend void LoadComponent(Camera_ *data_, const Reader &ir, const Handle &h);
	friend void LoadComponent(Object_ *data_, const Reader &ir, const Handle &h, const Reader &deps_ir, const ReadProvider &deps_ip,
		PipelineResources &resources, const PipelineInfo &pipeline, bool queue_model_loads, bool queue_texture_loads, bool do_not_load_resources, bool silent);
	friend void LoadComponent(Light_ *data_, const Reader &ir, const Handle &h);
	friend void LoadComponent(RigidBody_ *data_, const Reader &ir, const Handle &h);
	friend void LoadComponent(Collision_ *data_, const Reader &ir, const Handle &h);
	friend void LoadComponent(Instance_ *data_, const Reader &ir, const Handle &h);
	friend void LoadComponent(Script_ *data_, const Reader &ir, const Handle &h);

	friend void SaveComponent(const Transform_ *data_, const Writer &iw, const Handle &h);
	friend void SaveComponent(const Camera_ *data_, const Writer &iw, const Handle &h);
	friend void SaveComponent(const Object_ *data_, const Writer &iw, const Handle &h, const PipelineResources &resources);
	friend void SaveComponent(const Light_ *data_, const Writer &iw, const Handle &h);
	friend void SaveComponent(const RigidBody_ *data_, const Writer &iw, const Handle &h);
	friend void SaveComponent(const Collision_ *data_, const Writer &iw, const Handle &h);
	friend void SaveComponent(const Instance_ *data_, const Writer &iw, const Handle &h);
	friend void SaveComponent(const Script_ *data_, const Writer &iw, const Handle &h);

	//
	friend void LoadComponent(Transform_ *data_, const json &js);
	friend void LoadComponent(Camera_ *data_, const json &js);
	friend void LoadComponent(Object_ *data_, const json &js, const Reader &deps_ir, const ReadProvider &deps_ip, PipelineResources &resources,
		const PipelineInfo &pipeline, bool queue_model_loads, bool queue_texture_loads, bool do_not_load_resources);
	friend void LoadComponent(Light_ *data_, const json &js);
	friend void LoadComponent(RigidBody_ *data_, const json &js);
	friend void LoadComponent(Collision_ *data_, const json &js);
	friend void LoadComponent(Instance_ *data_, const json &js);
	friend void LoadComponent(Script_ *data_, const json &js);

	friend void SaveComponent(const Transform_ *data_, json &js);
	friend void SaveComponent(const Camera_ *data_, json &js);
	friend void SaveComponent(const Object_ *data_, json &js, const PipelineResources &resources);
	friend void SaveComponent(const Light_ *data_, json &js);
	friend void SaveComponent(const RigidBody_ *data_, json &js);
	friend void SaveComponent(const Collision_ *data_, json &js);
	friend void SaveComponent(const Instance_ *data_, json &js);
	friend void SaveComponent(const Script_ *data_, json &js);

	//
	std::vector<Mat4> transform_worlds; // filled during Update()
	std::vector<bool> transform_worlds_updated;

	void ComputeTransformWorldMatrix(uint32_t idx);

	std::vector<Mat4> previous_transform_worlds;
	std::vector<bool> previous_transform_worlds_updated;

	//
	generational_vector_list<Anim> anims;
	generational_vector_list<SceneAnim> scene_anims;

	//
	static constexpr uint8_t SPAF_Paused = 0x1;

	struct ScenePlayAnim {
		std::string name;
		SceneBoundAnim bound_anim;

		time_ns t, t_start, t_end; // 16B
		int8_t t_scale; // <- 16=1.f, 4 bit precision on range [-7.f;7.f[

		uint8_t flags;
		AnimLoopMode loop_mode;

		Easing easing;
	};

	generational_vector_list<ScenePlayAnim> play_anims;

private:
	NodeRef current_camera{};
};

//
std::vector<NodeRef> NodesToNodeRefs(const std::vector<Node> &nodes);
std::vector<Node> NodeRefsToNodes(const Scene &scene, const std::vector<NodeRef> &refs);

//
struct SceneRef {
	Scene *scene{nullptr};
	uint32_t ref_count{};
};

/// Helper function to create a Node with a Transform component then parent all root nodes in the scene to it.
Node CreateSceneRootNode(Scene &scene, std::string name = {}, const Mat4 &mtx = Mat4::Identity);

Node CreateCamera(Scene &scene, const Mat4 &mtx, float znear, float zfar, float fov = Deg(45.f));
Node CreateOrthographicCamera(Scene &scene, const Mat4 &mtx, float znear, float zfar, float size = Deg(1.f));

Node CreatePointLight(Scene &scene, const Mat4 &mtx, float radius, const Color &diffuse = {1, 1, 1}, float diffuse_intensity = 1,
	const Color &specular = {1, 1, 1}, float specular_intensity = 1, float priority = 0.f, LightShadowType shadow_type = LST_None,
	float shadow_bias = default_shadow_bias);
Node CreateSpotLight(Scene &scene, const Mat4 &mtx, float radius, float inner_angle, float outer_angle, const Color &diffuse = {1, 1, 1},
	float diffuse_intensity = 1, const Color &specular = {1, 1, 1}, float specular_intensity = 1, float priority = 0, LightShadowType shadow_type = LST_None,
	float shadow_bias = default_shadow_bias);
Node CreateLinearLight(Scene &scene, const Mat4 &mtx, const Color &diffuse = {1, 1, 1}, float diffuse_intensity = 1, const Color &specular = {1, 1, 1},
	float specular_intensity = 1, float priority = 0, LightShadowType shadow_type = LST_None, float shadow_bias = default_shadow_bias,
	const Vec4 &pssm_split = default_pssm_split);

Node CreatePointLight(Scene &scene, const Mat4 &mtx, float radius, const Color &diffuse, const Color &specular, float priority = 0.f,
	LightShadowType shadow_type = LST_None, float shadow_bias = default_shadow_bias);
Node CreateSpotLight(Scene &scene, const Mat4 &mtx, float radius, float inner_angle, float outer_angle, const Color &diffuse, const Color &specular,
	float priority = 0, LightShadowType shadow_type = LST_None, float shadow_bias = default_shadow_bias);
Node CreateLinearLight(Scene &scene, const Mat4 &mtx, const Color &diffuse, const Color &specular, float priority = 0, LightShadowType shadow_type = LST_None,
	float shadow_bias = default_shadow_bias, const Vec4 &pssm_split = default_pssm_split);

Node CreateObject(Scene &scene, const Mat4 &mtx, const ModelRef &model, std::vector<Material> materials = {});

Node CreateInstance(Scene &scene, const Mat4 &mtx, const std::string &name, const Reader &ir, const ReadProvider &ip, PipelineResources &resources,
	const PipelineInfo &pipeline, bool &success, uint32_t flags = LSSF_Nodes | LSSF_Anims);
Node CreateInstanceFromFile(Scene &scene, const Mat4 &mtx, const std::string &name, PipelineResources &resources, const PipelineInfo &pipeline, bool &success,
	uint32_t flags = LSSF_Nodes | LSSF_Anims);
Node CreateInstanceFromAssets(Scene &scene, const Mat4 &mtx, const std::string &name, PipelineResources &resources, const PipelineInfo &pipeline, bool &success,
	uint32_t flags = LSSF_Nodes | LSSF_Anims);

Node CreateScript(Scene &scene);
Node CreateScript(Scene &scene, const std::string &path);

Node CreatePhysicSphere(Scene &scene, float radius, const Mat4 &mtx, const ModelRef &model_ref, std::vector<Material> materials, float mass = 1.f);
Node CreatePhysicCube(Scene &scene, const Vec3 &size, const Mat4 &mtx, const ModelRef &model_ref, std::vector<Material> materials, float mass = 1.f);

//
uint32_t GetSceneBinaryFormatVersion();

bool SaveSceneJsonToFile(const char *path, const Scene &scene, const PipelineResources &resources, uint32_t save_flags = LSSF_All);
bool SaveSceneJsonToData(Data &data, const Scene &scene, const PipelineResources &resources, uint32_t save_flags = LSSF_All);
bool SaveSceneBinaryToFile(const char *path, const Scene &scene, const PipelineResources &resources, uint32_t save_flags = LSSF_All, bool debug = false);
bool SaveSceneBinaryToData(Data &data, const Scene &scene, const PipelineResources &resources, uint32_t save_flags = LSSF_All, bool debug = false);

bool IsBinarySceneFile(const char *path);
bool IsBinarySceneAsset(const char *name);

bool LoadSceneJsonFromFile(
	const char *path, Scene &scene, PipelineResources &resources, const PipelineInfo &pipeline, LoadSceneContext &ctx, uint32_t flags = LSSF_All);
bool LoadSceneJsonFromAssets(
	const char *name, Scene &scene, PipelineResources &resources, const PipelineInfo &pipeline, LoadSceneContext &ctx, uint32_t flags = LSSF_All);
bool LoadSceneJsonFromData(const Data &data, const char *name, Scene &scene, const Reader &deps_ir, const ReadProvider &deps_ip, PipelineResources &resources,
	const PipelineInfo &pipeline, LoadSceneContext &ctx, uint32_t flags = LSSF_All);

bool LoadSceneBinaryFromFile(const char *path, Scene &scene, PipelineResources &resources, const PipelineInfo &pipeline, LoadSceneContext &ctx,
	uint32_t flags = LSSF_All, bool debug = false);
bool LoadSceneBinaryFromAssets(const char *name, Scene &scene, PipelineResources &resources, const PipelineInfo &pipeline, LoadSceneContext &ctx,
	uint32_t flags = LSSF_All, bool debug = false);
bool LoadSceneBinaryFromData(const Data &data, const char *name, Scene &scene, const Reader &deps_ir, const ReadProvider &deps_ip, PipelineResources &resources,
	const PipelineInfo &pipeline, LoadSceneContext &ctx, uint32_t flags = LSSF_All, bool debug = false);

bool LoadSceneFromFile(
	const char *path, Scene &scene, PipelineResources &resources, const PipelineInfo &pipeline, LoadSceneContext &ctx, uint32_t flags = LSSF_All);
bool LoadSceneFromAssets(
	const char *name, Scene &scene, PipelineResources &resources, const PipelineInfo &pipeline, LoadSceneContext &ctx, uint32_t flags = LSSF_All);

//
std::vector<NodeRef> DuplicateNodes(Scene &scene, const std::vector<NodeRef> &nodes, const Reader &deps_ir, const ReadProvider &deps_ip,
	PipelineResources &resources, const PipelineInfo &pipeline);

/// Duplicate each node of a list. Resources will be loaded from the local filesystem.
std::vector<Node> DuplicateNodesFromFile(Scene &scene, const std::vector<Node> &nodes, PipelineResources &resources, const PipelineInfo &pipeline);
/// Duplicate each node of a list. Resources will be loaded from the assets system.
std::vector<Node> DuplicateNodesFromAssets(Scene &scene, const std::vector<Node> &nodes, PipelineResources &resources, const PipelineInfo &pipeline);
/// Duplicate each node and children of a list. Resources will be loaded from the local filesystem.
std::vector<Node> DuplicateNodesAndChildrenFromFile(Scene &scene, const std::vector<Node> &nodes, PipelineResources &resources, const PipelineInfo &pipeline);
/// Duplicate each node and children of a list. Resources will be loaded from the assets system.
std::vector<Node> DuplicateNodesAndChildrenFromAssets(Scene &scene, const std::vector<Node> &nodes, PipelineResources &resources, const PipelineInfo &pipeline);

Node DuplicateNodeFromFile(Scene &scene, Node node, PipelineResources &resources, const PipelineInfo &pipeline);
Node DuplicateNodeFromAssets(Scene &scene, Node node, PipelineResources &resources, const PipelineInfo &pipeline);
std::vector<Node> DuplicateNodeAndChildrenFromFile(Scene &scene, Node node, PipelineResources &resources, const PipelineInfo &pipeline);
std::vector<Node> DuplicateNodeAndChildrenFromAssets(Scene &scene, Node node, PipelineResources &resources, const PipelineInfo &pipeline);

//
void DumpSceneMemoryFootprint();

//
bool GetAnimableNodePropertyBool(const Scene &scene, NodeRef ref, const std::string &name);
void SetAnimableNodePropertyBool(Scene &scene, NodeRef ref, const std::string &name, bool v);

float GetAnimableScenePropertyFloat(const Scene &scene, const std::string &name);
void SetAnimableScenePropertyFloat(Scene &scene, const std::string &name, float v);
float GetAnimableNodePropertyFloat(const Scene &scene, NodeRef ref, const std::string &name);
void SetAnimableNodePropertyFloat(Scene &scene, NodeRef ref, const std::string &name, float v);

Vec3 GetAnimableScenePropertyVec3(const Scene &scene, const std::string &name);
void SetAnimableScenePropertyVec3(Scene &scene, const std::string &name, const Vec3 &v);
Vec3 GetAnimableNodePropertyVec3(const Scene &scene, NodeRef ref, const std::string &name);
void SetAnimableNodePropertyVec3(Scene &scene, NodeRef ref, const std::string &name, const Vec3 &v);
Vec4 GetAnimableNodePropertyVec4(const Scene &scene, NodeRef ref, const std::string &name);
void SetAnimableNodePropertyVec4(Scene &scene, NodeRef ref, const std::string &name, const Vec4 &v);

std::string GetAnimableNodePropertyString(const Scene &scene, NodeRef ref, const std::string &name);
void SetAnimableNodePropertyString(Scene &scene, NodeRef ref, const std::string &name, const std::string &v);

Color GetAnimableScenePropertyColor(const Scene &scene, const std::string &name);
void SetAnimableScenePropertyColor(Scene &scene, const std::string &name, const Color &v);
Color GetAnimableNodePropertyColor(const Scene &scene, NodeRef ref, const std::string &name);
void SetAnimableNodePropertyColor(Scene &scene, NodeRef ref, const std::string &name, const Color &v);

void ReverseSceneAnim(Scene &scene, SceneAnim &scene_anim);
void QuantizeSceneAnim(Scene &scene, SceneAnim &scene_anim, time_ns t_step);
void DeleteEmptySceneAnims(Scene &scene, SceneAnim &scene_anim);

#ifdef NDEBUG
#define _HG_CHECK_DLL_SCENE_DATA_TYPES
#else
void _CheckDllSceneDataTypes(size_t s_gen_ref, size_t s_node, size_t s_scene, size_t s_scene_view, size_t s_scene_anim);
#define _HG_CHECK_DLL_SCENE_DATA_TYPES _CheckDllDataTypes(sizeof(gen_ref), sizeof(Node), sizeof(Scene), sizeof(SceneView), sizeof(SceneAnim));
#endif

} // namespace hg
