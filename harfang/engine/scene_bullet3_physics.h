// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "engine/physics.h"

#include <array>
#include <limits>
#include <memory>

class btRigidBody;
class btDiscreteDynamicsWorld;
class btCollisionShape;

namespace hg {

class Scene;

//
struct Bullet3Node {
	btRigidBody *body{};
	Vec3 scl{Vec3::One};
	Mat4 ref_mtx{Mat4::Identity};
};

//
class SceneBullet3Physics {
public:
	SceneBullet3Physics(int thread_count = 1);
	~SceneBullet3Physics();

	//
	void SceneCreatePhysics(const Scene &scene, const Reader &ir, const ReadProvider &ip);
	void SceneCreatePhysicsFromFile(const Scene &scene);
	void SceneCreatePhysicsFromAssets(const Scene &scene);

	void NodeCreatePhysics(const Node &node, const Reader &ir, const ReadProvider &ip);
	void NodeCreatePhysicsFromFile(const Node &node);
	void NodeCreatePhysicsFromAssets(const Node &node);

	void NodeStartTrackingCollisionEvents(NodeRef ref, CollisionEventTrackingMode mode = CETM_EventOnly);
	void NodeStopTrackingCollisionEvents(NodeRef ref);

	void NodeStartTrackingCollisionEvents(const Node &node, CollisionEventTrackingMode mode = CETM_EventOnly) {
		NodeStartTrackingCollisionEvents(node.ref, mode);
	}
	void NodeStopTrackingCollisionEvents(const Node &node) { NodeStopTrackingCollisionEvents(node.ref); }

	void NodeDestroyPhysics(const Node &node);

	bool NodeHasBody(NodeRef ref) const { return nodes.find(ref) != std::end(nodes); }
	bool NodeHasBody(const Node &node) const { return NodeHasBody(node.ref); }

	/// Step physics world
	void StepSimulation(time_ns dt, time_ns step = time_from_ms(16), int max_step = 8);

	void CollectCollisionEvents(const Scene &scene, NodePairContacts &contacts);

	void SyncTransformsFromScene(const Scene &scene);
	void SyncTransformsToScene(Scene &scene);

	//
	size_t GarbageCollect(const Scene &scene);
	size_t GarbageCollectResources();

	void ClearNodes();
	void Clear();

	//
	void NodeWake(NodeRef ref) const;
	void NodeWake(const Node &node) const { NodeWake(node.ref); }

	void NodeSetDeactivation(NodeRef ref, bool enable) const;
	void NodeSetDeactivation(const Node &node, bool enable) const { NodeSetDeactivation(node.ref, enable); }
	bool NodeGetDeactivation(NodeRef ref) const;
	bool NodeGetDeactivation(const Node &node) const { return NodeGetDeactivation(node.ref); }

	void NodeResetWorld(NodeRef ref, const Mat4 &world) const;
	void NodeResetWorld(const Node &node, const Mat4 &world) const { NodeResetWorld(node.ref, world); }
	void NodeTeleport(NodeRef ref, const Mat4 &world);
	void NodeTeleport(const Node &node, const Mat4 &world) { NodeTeleport(node.ref, world); }

	void NodeAddForce(NodeRef ref, const Vec3 &F);
	void NodeAddForce(NodeRef ref, const Vec3 &F, const Vec3 &world_pos);
	void NodeAddImpulse(NodeRef ref, const Vec3 &dt_velocity);
	void NodeAddImpulse(NodeRef ref, const Vec3 &dt_velocity, const Vec3 &world_pos);
	void NodeAddTorque(NodeRef ref, const Vec3 &T);
	void NodeAddTorqueImpulse(NodeRef ref, const Vec3 &T);

	void NodeAddForce(const Node &node, const Vec3 &F) { NodeAddForce(node.ref, F); }
	void NodeAddForce(const Node &node, const Vec3 &F, const Vec3 &world_pos) { NodeAddForce(node.ref, F, world_pos); }
	void NodeAddImpulse(const Node &node, const Vec3 &dt_velocity) { NodeAddImpulse(node.ref, dt_velocity); }
	void NodeAddImpulse(const Node &node, const Vec3 &dt_velocity, const Vec3 &world_pos) { NodeAddImpulse(node.ref, dt_velocity, world_pos); }
	void NodeAddTorque(const Node &node, const Vec3 &T) { NodeAddTorque(node.ref, T); }
	void NodeAddTorqueImpulse(const Node &node, const Vec3 &T) { NodeAddTorqueImpulse(node.ref, T); }

	Vec3 NodeGetPointVelocity(const Node &node, const Vec3 &world_pos) const { return NodeGetPointVelocity(node.ref, world_pos); }
	Vec3 NodeGetPointVelocity(NodeRef ref, const Vec3 &world_pos) const;

	Vec3 NodeGetLinearVelocity(NodeRef ref) const;
	void NodeSetLinearVelocity(NodeRef ref, const Vec3 &V);
	Vec3 NodeGetAngularVelocity(NodeRef ref) const;
	void NodeSetAngularVelocity(NodeRef ref, const Vec3 &W);

	Vec3 NodeGetLinearVelocity(const Node &node) const { return NodeGetLinearVelocity(node.ref); }
	void NodeSetLinearVelocity(const Node &node, const Vec3 &V) { NodeSetLinearVelocity(node.ref, V); }
	Vec3 NodeGetAngularVelocity(const Node &node) const { return NodeGetAngularVelocity(node.ref); }
	void NodeSetAngularVelocity(const Node &node, const Vec3 &W) { NodeSetAngularVelocity(node.ref, W); }

	Vec3 NodeGetLinearFactor(NodeRef ref) const;
	void NodeSetLinearFactor(NodeRef ref, const Vec3 &k);
	Vec3 NodeGetAngularFactor(NodeRef ref) const;
	void NodeSetAngularFactor(NodeRef ref, const Vec3 &k);

	Vec3 NodeGetLinearFactor(const Node &node) const { return NodeGetLinearFactor(node.ref); }
	void NodeSetLinearFactor(const Node &node, const Vec3 &k) { NodeSetLinearFactor(node.ref, k); }
	Vec3 NodeGetAngularFactor(const Node &node) const { return NodeGetAngularFactor(node.ref); }
	void NodeSetAngularFactor(const Node &node, const Vec3 &k) { NodeSetAngularFactor(node.ref, k); }

	//
	btCollisionShape *LoadCollisionTree(const Reader &ir, const ReadProvider &ip, const char *name, int shape_id = 0);

	btCollisionShape *LoadCollisionTreeFromFile(const char *path, int shape_id = 0);
	btCollisionShape *LoadCollisionTreeFromAssets(const char *name, int shape_id = 0);

	//
	NodePairContacts NodeCollideWorld(const Scene &scene, NodeRef ref, const Mat4 &world, int max_contact = 1) const;
	NodePairContacts NodeCollideWorld(const Node &node, const Mat4 &world, int max_contact = 1) const;

	RaycastOut RaycastFirstHit(const Scene &scene, const Vec3 &world_p0, const Vec3 &world_p1) const;
	std::vector<RaycastOut> RaycastAllHits(const Scene &scene, const Vec3 &world_p0, const Vec3 &world_p1) const;

	//
	void RenderCollision(bgfx::ViewId view_id, const bgfx::VertexLayout &vtx_decl, bgfx::ProgramHandle program, RenderState state, uint32_t depth);

private:
	std::unique_ptr<btDiscreteDynamicsWorld> world;

	std::map<NodeRef, Bullet3Node> nodes;
	std::map<std::string, btCollisionShape *> collision_trees;

	btRigidBody *GetNodeBody(NodeRef ref, const char *func) const;

	std::map<NodeRef, CollisionEventTrackingMode> node_collision_event_tracking_modes;

	time_ns physics_motion_clock = 0;
	float physics_motion_k = 0.f; // motion interpolation coefficient

	std::map<NodeRef, Mat4> prv_world_mtx; // [EJ] this will probably quickly become a bottleneck and just be replaced by a faster structure
	std::vector<NodeRef> was_teleported;
};

} // namespace hg
