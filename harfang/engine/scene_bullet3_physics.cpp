// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/scene_bullet3_physics.h"
#include "engine/assets_rw_interface.h"
#include "engine/render_pipeline.h"
#include "engine/scene.h"

#include "foundation/file_rw_interface.h"
#include "foundation/format.h"
#include "foundation/log.h"
#include "foundation/rw_interface.h"
#include "foundation/vector3.h"

#include <btBulletDynamicsCommon.h>

#include <BulletCollision/CollisionDispatch/btCollisionDispatcherMt.h>
#include <BulletDynamics/ConstraintSolver/btNNCGConstraintSolver.h>
#include <BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolverMt.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorldMt.h>

#include <BulletCollision/BroadphaseCollision/btAxisSweep3.h>

#include <memory>

namespace hg {

btRigidBody *SceneBullet3Physics::GetNodeBody(NodeRef ref, const char *func) const {
	const auto i = nodes.find(ref);

	if (i != std::end(nodes))
		return i->second.body;

	if (func)
		warn(format("Node physics missing when calling %1 for NodeRef %2:%3").arg(func).arg(ref.gen).arg(ref.idx));

	return nullptr;
}

//
struct DebugDrawContext {
	bgfx::ViewId view_id;
	bgfx::VertexLayout vtx_decl;
	bgfx::ProgramHandle program;
	RenderState state;
	uint32_t depth;
};

struct Bullet3DebugDraw : btIDebugDraw {
	void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color) override;
	void drawContactPoint(const btVector3 &point_on_b, const btVector3 &normal_on_b, btScalar distance, int life_time, const btVector3 &color) override {}

	void reportErrorWarning(const char *warning_string) override {}
	void draw3dText(const btVector3 &location, const char *text_string) override {}

	int debug_mode = 0;

	void setDebugMode(int mode) override { debug_mode = mode; }
	int getDebugMode() const override { return debug_mode; }

	DebugDrawContext context;

	void SetDrawContext(const DebugDrawContext &context_) {
		context = context_;

		lines.reset(new Vertices(context.vtx_decl, 512));
	}

	std::unique_ptr<Vertices> lines;
	uint32_t line_count = 0;

	void clearLines() override;
	void flushLines() override;
};

void Bullet3DebugDraw::drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color) {
	lines->Begin(line_count++);
	lines->SetPos({from.x(), from.y(), from.z()});
	lines->SetColor0({color.x(), color.y(), color.z(), color.w()});
	lines->End();

	lines->Begin(line_count++);
	lines->SetPos({to.x(), to.y(), to.z()});
	lines->SetColor0({color.x(), color.y(), color.z(), color.w()});
	lines->End();
}

void Bullet3DebugDraw::clearLines() {
	lines->Clear();
	line_count = 0;
}

void Bullet3DebugDraw::flushLines() { DrawLines(context.view_id, *lines, context.program, context.state, context.depth); }

//
SceneBullet3Physics::SceneBullet3Physics(int threads) {
#if 1
	auto collision_cfg = new btDefaultCollisionConfiguration;

	auto dispatcher = new btCollisionDispatcher(collision_cfg);
	auto broadphase = new btDbvtBroadphase;

	auto solver = new btSequentialImpulseConstraintSolver;

	world.reset(new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collision_cfg));
#else
	btSetTaskScheduler(btCreateDefaultTaskScheduler());

	btDefaultCollisionConstructionInfo cci;
	cci.m_defaultMaxPersistentManifoldPoolSize = 80000;
	cci.m_defaultMaxCollisionAlgorithmPoolSize = 80000;
	auto collision_cfg = new btDefaultCollisionConfiguration(cci);

	auto dispatcher = new btCollisionDispatcherMt(collision_cfg, 40);
	auto broadphase = new btDbvtBroadphase;

	btConstraintSolver *solvers[BT_MAX_THREAD_COUNT];
	for (int i = 0; i < BT_MAX_THREAD_COUNT; ++i)
		solvers[i] = new btSequentialImpulseConstraintSolver;

	auto solver_pool = new btConstraintSolverPoolMt(solvers, BT_MAX_THREAD_COUNT);
	auto solver = new btSequentialImpulseConstraintSolverMt;

	world.reset(new btDiscreteDynamicsWorldMt(dispatcher, broadphase, solver_pool, solver, collision_cfg));
#endif

	world->setGravity(btVector3(0.f, -9.81f, 0.f));

	auto debug_draw = new Bullet3DebugDraw;
	debug_draw->setDebugMode(btIDebugDraw::DBG_DrawWireframe | btIDebugDraw::DBG_DrawContactPoints | btIDebugDraw::DBG_DrawConstraints |
							 btIDebugDraw::DBG_DrawConstraintLimits | btIDebugDraw::DBG_DrawFeaturesText);
	world->setDebugDrawer(debug_draw);
}

SceneBullet3Physics::~SceneBullet3Physics() { Clear(); }

//
void SceneBullet3Physics::SceneCreatePhysics(const Scene &scene, const Reader &ir, const ReadProvider &ip) {
	const auto nodes = scene.GetAllNodes();

	for (auto &node : nodes)
		if (!NodeHasBody(node.ref))
			NodeCreatePhysics(node, ir, ip);
}

void SceneBullet3Physics::SceneCreatePhysicsFromFile(const Scene &scene) { SceneCreatePhysics(scene, g_file_reader, g_file_read_provider); }
void SceneBullet3Physics::SceneCreatePhysicsFromAssets(const Scene &scene) { SceneCreatePhysics(scene, g_assets_reader, g_assets_read_provider); }

//
btTransform to_btTransform(const Mat4 &m) {
	btScalar scalar[15];
	scalar[0] = m.m[0][0];
	scalar[1] = m.m[1][0];
	scalar[2] = m.m[2][0];
	scalar[3] = 1;
	scalar[4] = m.m[0][1];
	scalar[5] = m.m[1][1];
	scalar[6] = m.m[2][1];
	scalar[7] = 1;
	scalar[8] = m.m[0][2];
	scalar[9] = m.m[1][2];
	scalar[10] = m.m[2][2];
	scalar[11] = 1;
	scalar[12] = m.m[0][3];
	scalar[13] = m.m[1][3];
	scalar[14] = m.m[2][3];

	btTransform t;
	t.setFromOpenGLMatrix(scalar);
	return t;
}

Mat4 from_btTransform(const btTransform &t) {
	btScalar scalar[16];
	t.getOpenGLMatrix(scalar);

	Mat4 m;
	m.m[0][0] = scalar[0];
	m.m[1][0] = scalar[1];
	m.m[2][0] = scalar[2];
	m.m[0][1] = scalar[4];
	m.m[1][1] = scalar[5];
	m.m[2][1] = scalar[6];
	m.m[0][2] = scalar[8];
	m.m[1][2] = scalar[9];
	m.m[2][2] = scalar[10];
	m.m[0][3] = scalar[12];
	m.m[1][3] = scalar[13];
	m.m[2][3] = scalar[14];
	return m;
}

Vec3 from_btVector3(const btVector3 &v) { return Vec3(v.x(), v.y(), v.z()); }
btVector3 to_btVector3(const Vec3 &v) { return btVector3(v.x, v.y, v.z); }

//
#if 0
struct PhysicSystemMotionState : public btMotionState {
	PhysicSystemMotionState(Node node_, const btTransform &com_) : com(com_), node(node_) {}

	void getWorldTransform(btTransform &com_world_trs) const override {
		if (auto trs = node.GetTransform()) {
			auto w = trs.GetWorld();
			const auto world = TransformationMat4(GetT(w), GetR(w), Vec3::One);
			com_world_trs = to_btTransform(world) * com.inverse();
		}
	}

	void setWorldTransform(const btTransform &com_world_trs) override {
		if (auto trs = node.GetTransform())
			trs.SetWorld(from_btTransform(com_world_trs * com));
	}

	btTransform com;
	Node node;
};
#endif

//
static void __DeleteRigidBody(const btRigidBody *body) {
	delete body->getMotionState();
	delete body->getCollisionShape();
	delete body;
}

void SceneBullet3Physics::NodeCreatePhysics(const Node &node, const Reader &ir, const ReadProvider &ip) {
	auto rb = node.GetRigidBody();
	if (!rb)
		return; // no rigid body component

	auto &_node = nodes[node.ref];

	if (_node.body) {
		__DeleteRigidBody(_node.body);
		_node.body = nullptr;
	}

	if (!node.GetCollisionCount())
		return; // no collision shape, nothing to simulate

	std::vector<btCollisionShape *> shapes;
	shapes.reserve(4);

	float total_mass = 0.f;

	for (size_t idx = 0; idx < node.GetCollisionCount(); ++idx) {
		const auto col = node.GetCollision(idx);

		const auto type = col.GetType();
		const auto size = col.GetSize();

		if (type == CT_Sphere) {
			shapes.push_back(new btSphereShape(size.x));
		} else if (type == CT_Cube) {
			shapes.push_back(new btBoxShape(btVector3(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f)));
		} else if (type == CT_Cone) {
			shapes.push_back(new btConeShape(size.x, size.y));
		} else if (type == CT_Capsule) {
			shapes.push_back(new btCapsuleShape(size.x, size.y));
		} else if (type == CT_Cylinder) {
			shapes.push_back(new btCylinderShape(btVector3(size.x, size.y, size.z)));
		} else if (type == CT_Mesh) {
			if (auto tree = LoadCollisionTree(ir, ip, col.GetCollisionResource().c_str()))
				shapes.push_back(tree);
		}

		shapes.back()->setUserIndex(node.ref.idx); // ref back to node
		total_mass += col.GetMass();
	}

	auto trs = node.GetTransform();
	btTransform bt_trs = btTransform::getIdentity();
	if (trs)
		bt_trs = to_btTransform(node.ComputeWorld()); // require most up-to-date world matrix

	if (!shapes.empty()) {
		auto root_shape = new btCompoundShape;
		for (size_t idx = 0; idx < node.GetCollisionCount(); ++idx) {
			const auto col = node.GetCollision(idx);
			root_shape->addChildShape(to_btTransform(col.GetLocalTransform()), shapes[idx]);
		}

		root_shape->setUserIndex(node.ref.idx); // ref back to node

		// create body
		btVector3 local_inertia;
		root_shape->calculateLocalInertia(total_mass, local_inertia);

		btRigidBody::btRigidBodyConstructionInfo rb_info(total_mass, nullptr, root_shape, local_inertia);

		rb_info.m_restitution = rb.GetRestitution();
		rb_info.m_friction = rb.GetFriction();
		rb_info.m_rollingFriction = rb.GetRollingFriction();
		rb_info.m_startWorldTransform = bt_trs;
		rb_info.m_linearDamping = rb.GetLinearDamping();
		rb_info.m_angularDamping = rb.GetAngularDamping();

		_node.body = new btRigidBody(rb_info);

		_node.body->setCollisionShape(root_shape);
		_node.body->setUserIndex(node.ref.idx); // ref back to node

		// configure
		const auto type = rb.GetType();
		const auto flags = _node.body->getCollisionFlags();

		if (type == RBT_Dynamic)
			_node.body->setCollisionFlags(flags & ~(btRigidBody::CF_KINEMATIC_OBJECT | btCollisionObject::CF_STATIC_OBJECT));
		else if (type == RBT_Kinematic)
			_node.body->setCollisionFlags((flags | btRigidBody::CF_KINEMATIC_OBJECT) & ~btCollisionObject::CF_STATIC_OBJECT);
		else
			_node.body->setCollisionFlags((flags & ~btRigidBody::CF_KINEMATIC_OBJECT) | btCollisionObject::CF_STATIC_OBJECT);

		// add to world
		world->addRigidBody(_node.body);
	}
}

void SceneBullet3Physics::NodeCreatePhysicsFromFile(const Node &node) { NodeCreatePhysics(node, g_file_reader, g_file_read_provider); }
void SceneBullet3Physics::NodeCreatePhysicsFromAssets(const Node &node) { NodeCreatePhysics(node, g_assets_reader, g_assets_read_provider); }

//
void SceneBullet3Physics::NodeDestroyPhysics(const Node &node) {
	auto i = nodes.find(node.ref);

	if (i != std::end(nodes)) {
		world->removeRigidBody(i->second.body);
		__DeleteRigidBody(i->second.body);
		nodes.erase(i);
	}
}

//
void SceneBullet3Physics::ClearNodes() {
	for (auto i : nodes) // EJ maximize code cache hit by first removing all then deleting all
		world->removeRigidBody(i.second.body);

	for (auto i : nodes)
		__DeleteRigidBody(i.second.body);

	nodes.clear();
}

void SceneBullet3Physics::Clear() {
	ClearNodes();

	for (auto i : collision_trees)
		;

	collision_trees.clear();
}

//
size_t SceneBullet3Physics::GarbageCollect(const Scene &scene) {
	size_t erased = 0;
	for (auto i = std::begin(nodes); i != std::end(nodes);)
		if (!scene.IsValidNodeRef(i->first)) {
			world->removeRigidBody(i->second.body);
			__DeleteRigidBody(i->second.body);
			i = nodes.erase(i);

			++erased;
		} else {
			++i;
		}
	return erased;
}

size_t SceneBullet3Physics::GarbageCollectResources() {
	// TODO run through existing bodies and destroy all orphaned collision trees
	return 0;
}

//
void SceneBullet3Physics::StepSimulation(time_ns dt, time_ns step, int max_step) {
	// store current matrices to use as previous matrices should a physics sub-step be taken
	std::map<NodeRef, Mat4> prv_mtx;

	for (const auto &i : nodes) {
		const auto body = i.second.body;
		if (body->getCollisionFlags() == btRigidBody::CF_DYNAMIC_OBJECT)
			prv_mtx[i.first] = from_btTransform(body->getWorldTransform());
	}

	// remove previous matrix for teleported nodes ([EJ] two step to avoid n2 queries)
	for (auto ref : was_teleported)
		prv_mtx.erase(ref);

	was_teleported.clear();

	//
	const auto substep_count = world->stepSimulation(time_to_sec_f(dt), max_step, time_to_sec_f(step));

	// if substep was taken commit to prv_world_mtx
	if (substep_count > 0)
		prv_world_mtx = std::move(prv_mtx); // [EJ] would be much better to know beforehand that substep count will be >0 and work on prv_world_mtx directly

	//
	physics_motion_clock += dt - substep_count * step;
	physics_motion_k = time_to_sec_f(physics_motion_clock) / time_to_sec_f(step);
}

//
void SceneBullet3Physics::SyncTransformsFromScene(const Scene &scene) {
	for (const auto &i : nodes) {
		const auto body = i.second.body;
		const auto flags = body->getCollisionFlags();

		if (flags == btRigidBody::CF_KINEMATIC_OBJECT) {
			/*
				[EJ] ComputeNodeWorldMatrix is wasteful and only required so that the first frame
				synchronization is correct as the scene matrices have not been computed yet.

				We count on the fact that kinematic objects are a minority in the scene graph for
				this change to not impact performances too much.
			*/
			const auto world_no_scale = Normalize(scene.ComputeNodeWorldMatrix(i.first));
			body->setWorldTransform(to_btTransform(world_no_scale));
		}
	}
}

void SceneBullet3Physics::SyncTransformsToScene(Scene &scene) {
	Mat4 world;

	for (const auto &i : nodes) {
		const auto body = i.second.body;
		const auto flags = body->getCollisionFlags();

		if (flags == btRigidBody::CF_DYNAMIC_OBJECT) {
			const auto j = prv_world_mtx.find(i.first);

			if (j != std::end(prv_world_mtx)) {
				const auto prv_world = j->second;
				const auto cur_world = from_btTransform(body->getWorldTransform());

				world = LerpAsOrthonormalBase(prv_world, cur_world, physics_motion_k);
			} else {
				world = from_btTransform(body->getWorldTransform());
			}

			scene.SetNodeWorldMatrix(i.first, world); // EJ20222101 do not use Transform.SetWorld which is much slower
		}
	}
}

//
void SceneBullet3Physics::NodeStartTrackingCollisionEvents(NodeRef ref, CollisionEventTrackingMode mode) { node_collision_event_tracking_modes[ref] = mode; }
void SceneBullet3Physics::NodeStopTrackingCollisionEvents(NodeRef ref) { node_collision_event_tracking_modes.erase(ref); }

void SceneBullet3Physics::CollectCollisionEvents(const Scene &scene, NodePairContacts &node_node_contacts) {
	node_node_contacts.clear();

	const int manifold_count = world->getDispatcher()->getNumManifolds();
	const auto manifolds = world->getDispatcher()->getInternalManifoldPointer();

	for (int n = 0; n < manifold_count; ++n) {
		const auto manifold = manifolds[n];
		if (!manifold) // manifolds are valid as long as the bodies overlap in the broadphase
			continue;

		const auto manifold_contact_count = manifold->getNumContacts();
		if (!manifold_contact_count)
			continue;

		const auto node_a_ref = scene.GetNodeRef(static_cast<const btRigidBody *>(manifold->getBody0())->getUserIndex());
		if (!scene.IsValidNodeRef(node_a_ref))
			continue;

		const auto node_b_ref = scene.GetNodeRef(static_cast<const btRigidBody *>(manifold->getBody1())->getUserIndex());
		if (!scene.IsValidNodeRef(node_a_ref))
			continue;

		{
			auto i = node_collision_event_tracking_modes.find(node_a_ref);
			if (i != std::end(node_collision_event_tracking_modes)) {
				auto &contacts = node_node_contacts[node_a_ref][node_b_ref];
				contacts.reserve(manifold_contact_count);

				for (int j = 0; j < manifold_contact_count; ++j) {
					const auto &contact = manifold->getContactPoint(j);
					contacts.push_back({from_btVector3(contact.m_positionWorldOnB), from_btVector3(contact.m_normalWorldOnB), contact.getDistance()});
				}
			}
		}
	}
}

//
void SceneBullet3Physics::NodeWake(NodeRef ref) const {
	if (auto body = GetNodeBody(ref, __func__))
		body->activate();
}

//
void SceneBullet3Physics::NodeSetDeactivation(NodeRef ref, bool enable) const {
	if (auto body = GetNodeBody(ref, __func__))
		body->setActivationState(enable ? ACTIVE_TAG : DISABLE_DEACTIVATION);
}

bool SceneBullet3Physics::NodeGetDeactivation(NodeRef ref) const {
	if (auto body = GetNodeBody(ref, __func__))
		return body->getActivationState() == ACTIVE_TAG ? true : false;
	return true;
}

//
void SceneBullet3Physics::NodeResetWorld(NodeRef ref, const Mat4 &world) const {
	if (auto body = GetNodeBody(ref, __func__)) {
		body->setWorldTransform(to_btTransform(world));
		body->setLinearVelocity(btVector3(0, 0, 0));
		body->setAngularVelocity(btVector3(0, 0, 0));
		body->clearForces();
		body->activate();
	}
}

void SceneBullet3Physics::NodeTeleport(NodeRef ref, const Mat4 &world) {
	if (auto body = GetNodeBody(ref, __func__)) {
		body->setWorldTransform(to_btTransform(world));
		was_teleported.push_back(ref);
	}
}

//
static btVector3 __bt_WorldToLocalPos(btRigidBody *body, const btVector3 &pos) {
	btTransform world_transform;
	body->getMotionState()->getWorldTransform(world_transform);
	return world_transform.inverse() * pos;
}

void SceneBullet3Physics::NodeAddForce(NodeRef ref, const Vec3 &F) {
	if (auto body = GetNodeBody(ref, __func__))
		body->applyCentralForce(to_btVector3(F));
}

void SceneBullet3Physics::NodeAddForce(NodeRef ref, const Vec3 &F, const Vec3 &world_pos) {
	if (auto body = GetNodeBody(ref, __func__))
		body->applyForce(to_btVector3(F), to_btVector3(world_pos) - body->getCenterOfMassPosition());
}

void SceneBullet3Physics::NodeAddImpulse(NodeRef ref, const Vec3 &I) {
	if (auto body = GetNodeBody(ref, __func__))
		body->applyCentralImpulse(to_btVector3(I));
}

void SceneBullet3Physics::NodeAddImpulse(NodeRef ref, const Vec3 &I, const Vec3 &world_pos) {
	if (auto body = GetNodeBody(ref, __func__))
		body->applyImpulse(to_btVector3(I), to_btVector3(world_pos) - body->getCenterOfMassPosition());
}

void SceneBullet3Physics::NodeAddTorque(NodeRef ref, const Vec3 &T) {
	if (auto body = GetNodeBody(ref, __func__))
		body->applyTorque(to_btVector3(T));
}

void SceneBullet3Physics::NodeAddTorqueImpulse(NodeRef ref, const Vec3 &T) {
	if (auto body = GetNodeBody(ref, __func__))
		body->applyTorqueImpulse(to_btVector3(T));
}

Vec3 SceneBullet3Physics::NodeGetPointVelocity(NodeRef ref, const Vec3 &world_pos) const {
	if (auto body = GetNodeBody(ref, __func__))
		return from_btVector3(body->getVelocityInLocalPoint(to_btVector3(world_pos) - body->getCenterOfMassPosition()));
	return {};
}

//
Vec3 SceneBullet3Physics::NodeGetLinearVelocity(NodeRef ref) const {
	if (auto body = GetNodeBody(ref, __func__))
		return from_btVector3(body->getLinearVelocity());
	return {};
}

void SceneBullet3Physics::NodeSetLinearVelocity(NodeRef ref, const Vec3 &V) {
	if (auto body = GetNodeBody(ref, __func__))
		body->applyCentralImpulse(to_btVector3(V - from_btVector3(body->getLinearVelocity())));
}

Vec3 SceneBullet3Physics::NodeGetAngularVelocity(NodeRef ref) const {
	if (auto body = GetNodeBody(ref, __func__))
		return from_btVector3(body->getAngularVelocity());
	return {};
}

void SceneBullet3Physics::NodeSetAngularVelocity(NodeRef ref, const Vec3 &W) {
	if (auto body = GetNodeBody(ref, __func__))
		body->setAngularVelocity(to_btVector3(W));
}

//
Vec3 SceneBullet3Physics::NodeGetLinearFactor(NodeRef ref) const {
	if (auto body = GetNodeBody(ref, __func__))
		return from_btVector3(body->getLinearFactor());
	return {};
}

void SceneBullet3Physics::NodeSetLinearFactor(NodeRef ref, const Vec3 &k) {
	if (auto body = GetNodeBody(ref, __func__))
		body->setLinearFactor(to_btVector3(k));
}

Vec3 SceneBullet3Physics::NodeGetAngularFactor(NodeRef ref) const {
	if (auto body = GetNodeBody(ref, __func__))
		return from_btVector3(body->getAngularFactor());
	return {};
}

void SceneBullet3Physics::NodeSetAngularFactor(NodeRef ref, const Vec3 &k) {
	if (auto body = GetNodeBody(ref, __func__))
		body->setAngularFactor(to_btVector3(k));
}

//
struct NodeCollideWorldCallback : btCollisionWorld::ContactResultCallback {
	NodeCollideWorldCallback(const Scene &_scene, NodeRef _node_ref) : scene(_scene), node_ref(_node_ref) {}

	btScalar addSingleResult(btManifoldPoint &cp, const btCollisionObjectWrapper *col_obj_0_wrap, int part_id_0, int index_0,
		const btCollisionObjectWrapper *col_obj_1_wrap, int part_id_1, int index_1) override {
		const auto node_0_ref = scene.GetNodeRef(uint32_t(col_obj_0_wrap->getCollisionObject()->getUserIndex()));
		const auto node_1_ref = scene.GetNodeRef(uint32_t(col_obj_1_wrap->getCollisionObject()->getUserIndex()));

		if (scene.IsValidNodeRef(node_0_ref) && scene.IsValidNodeRef(node_1_ref)) {
			contacts[node_ref == node_0_ref ? node_1_ref : node_0_ref].push_back(
				{from_btVector3(cp.m_positionWorldOnB), from_btVector3(cp.m_normalWorldOnB), cp.getDistance()});
		}
		return 0.f;
	}

	const Scene &scene;

	NodeRef node_ref;
	NodeContacts contacts;
};

NodePairContacts SceneBullet3Physics::NodeCollideWorld(const Scene &scene, NodeRef ref, const Mat4 &mtx, int max_contact) const {
	const auto body = GetNodeBody(ref, __func__);
	if (!body)
		return {};

	const auto trs = body->getWorldTransform(); // save body transform
	body->setWorldTransform(to_btTransform(mtx));

	NodeCollideWorldCallback callback(scene, ref);
	world->contactTest(body, callback);

	body->setWorldTransform(trs); // restore body transform
	return {{ref, callback.contacts}};
}

NodePairContacts SceneBullet3Physics::NodeCollideWorld(const Node &node, const Mat4 &world, int max_contact) const {
	return node.scene_ref ? NodeCollideWorld(*node.scene_ref->scene, node.ref, world, max_contact) : NodePairContacts{};
}

//
RaycastOut SceneBullet3Physics::RaycastFirstHit(const Scene &scene, const Vec3 &world_p0, const Vec3 &world_p1) const {
	struct ClosestRayResultWithTriangleIndexCallback : public btCollisionWorld::ClosestRayResultCallback {
		ClosestRayResultWithTriangleIndexCallback(const btVector3 &rayFromWorld, const btVector3 &rayToWorld)
			: ClosestRayResultCallback(rayFromWorld, rayToWorld), m_TriangleIndex(-1), m_shapePart(-1) {}

		int m_TriangleIndex;
		int m_shapePart;

		virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult &rayResult, bool normalInWorldSpace) {
			if (rayResult.m_localShapeInfo) {
				m_TriangleIndex = rayResult.m_localShapeInfo->m_triangleIndex;
				m_shapePart = rayResult.m_localShapeInfo->m_shapePart;
			} else {
				m_TriangleIndex = -1;
				m_shapePart = -1;
			}
			return ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
		}
	};

	btVector3 from = to_btVector3(world_p0), to = to_btVector3(world_p1);

	ClosestRayResultWithTriangleIndexCallback trace(from, to);
	trace.m_collisionFilterGroup = btBroadphaseProxy::AllFilter;
	trace.m_collisionFilterMask = ~0; // FIXME

	world->rayTest(from, to, trace);

	if (!trace.hasHit())
		return {};

	RaycastOut out;

	out.N = from_btVector3(trace.m_hitNormalWorld);
	out.node = scene.GetNode(scene.GetNodeRef(trace.m_collisionObject->getUserIndex()));
	out.P = from_btVector3(trace.m_hitPointWorld);
	out.t = Dot(out.P - world_p0, Normalize(world_p1 - world_p0));

	/* FIXME
		if (RigidBody *rb = (RigidBody *)hit.i->rigid_body)
			if ((trace.m_shapePart >= 0) && ((uint)trace.m_shapePart < rb->shapes.size()))
				if (Mesh *mesh = rb->shapes[trace.m_shapePart].mesh.c_ptr())
					if (uint(trace.m_TriangleIndex) < mesh->bt_id_mat.size())
						hit.m = mesh->bt_mat[mesh->bt_id_mat[trace.m_TriangleIndex]];
	*/
	return out;
}

std::vector<RaycastOut> SceneBullet3Physics::RaycastAllHits(const Scene &scene, const Vec3 &world_p0, const Vec3 &world_p1) const {
	struct AllRayResultWithTriangleIndexCallback : public btCollisionWorld::AllHitsRayResultCallback {
		AllRayResultWithTriangleIndexCallback(const btVector3 &rayFromWorld, const btVector3 &rayToWorld)
			: AllHitsRayResultCallback(rayFromWorld, rayToWorld), m_TriangleIndex(-1), m_shapePart(-1) {}

		int m_TriangleIndex;
		int m_shapePart;

		virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult &rayResult, bool normalInWorldSpace) {
			if (rayResult.m_localShapeInfo) {
				m_TriangleIndex = rayResult.m_localShapeInfo->m_triangleIndex;
				m_shapePart = rayResult.m_localShapeInfo->m_shapePart;
			} else {
				m_TriangleIndex = -1;
				m_shapePart = -1;
			}
			return AllHitsRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
		}
	};

	btVector3 from = to_btVector3(world_p0), to = to_btVector3(world_p1);

	AllRayResultWithTriangleIndexCallback trace(from, to);
	trace.m_collisionFilterGroup = btBroadphaseProxy::AllFilter;
	trace.m_collisionFilterMask = ~0; // FIXME

	world->rayTest(from, to, trace);

	if (!trace.hasHit())
		return {};

	std::vector<RaycastOut> outs;
	outs.resize(trace.m_hitNormalWorld.size());

	for (int i = 0; i < trace.m_hitNormalWorld.size(); ++i) {
		outs[i].N = from_btVector3(trace.m_hitNormalWorld[i]);
		outs[i].node = scene.GetNode(scene.GetNodeRef(trace.m_collisionObjects[i]->getUserIndex()));
		outs[i].P = from_btVector3(trace.m_hitPointWorld[i]);
		outs[i].t = Dot(outs[i].P - world_p0, Normalize(world_p1 - world_p0));
	}

	/* FIXME
		if (RigidBody *rb = (RigidBody *)hit.i->rigid_body)
			if ((trace.m_shapePart >= 0) && ((uint)trace.m_shapePart < rb->shapes.size()))
				if (Mesh *mesh = rb->shapes[trace.m_shapePart].mesh.c_ptr())
					if (uint(trace.m_TriangleIndex) < mesh->bt_id_mat.size())
						hit.m = mesh->bt_mat[mesh->bt_id_mat[trace.m_TriangleIndex]];
	*/
	return outs;
}

//
struct DeserializeHandle {
	Reader ir;
	Handle h;
};

static void _ReadCollisionMesh(void *const deserializeHandle, void *const buffer, int size) {
	auto hnd = reinterpret_cast<const DeserializeHandle *>(deserializeHandle);
	hnd->ir.read(hnd->h, buffer, size);
}

btCollisionShape *SceneBullet3Physics::LoadCollisionTree(const Reader &ir, const ReadProvider &ip, const char *name, int id) {
	auto i = collision_trees.find(name);
	if (i != std::end(collision_trees))
		return i->second;

	btCollisionShape *collision = nullptr;

	ScopedReadHandle h(ip, name);
	DeserializeHandle hnd = {ir, h};

	if (ir.is_valid(hnd.h))
		collision = nullptr; // NewtonCreateCollisionFromSerialization(world, _ReadCollisionMesh, &hnd);

	collision_trees[name] = collision;
	if (!collision)
		warn(format("Failed to load Bullet3 collision tree '%1'").arg(name));

	return collision;
}

btCollisionShape *SceneBullet3Physics::LoadCollisionTreeFromFile(const char *path, int id) {
	return LoadCollisionTree(g_file_reader, g_file_read_provider, path, id);
}

btCollisionShape *SceneBullet3Physics::LoadCollisionTreeFromAssets(const char *name, int id) {
	return LoadCollisionTree(g_assets_reader, g_assets_read_provider, name, id);
}

//
struct DebugRenderContext {
	bgfx::ViewId view_id;
	const bgfx::VertexLayout &vtx_decl;

	bgfx::ProgramHandle program;
	RenderState state;

	uint32_t depth;
};

void SceneBullet3Physics::RenderCollision(
	bgfx::ViewId view_id, const bgfx::VertexLayout &vtx_decl, bgfx::ProgramHandle program, RenderState state, uint32_t depth) {
	auto debug_draw = reinterpret_cast<Bullet3DebugDraw *>(world->getDebugDrawer());
	debug_draw->SetDrawContext({view_id, vtx_decl, program, state, depth});

	world->debugDrawWorld();
}

} // namespace hg
