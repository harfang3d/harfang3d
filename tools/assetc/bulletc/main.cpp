// HARFANG(R) Copyright (C) 2022 Thomas Simonnet, Movida Production. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include <foundation/build_info.h>
#include <foundation/cmd_line.h>
#include <foundation/format.h>
#include <foundation/log.h>
#include <foundation/path_tools.h>
#include <foundation/string.h>

#include <engine/geometry.h>
#include <engine/meta.h>
#include <engine/to_json.h>

#include <btBulletDynamicsCommon.h>

#include <Serialize/BulletWorldImporter/btBulletWorldImporter.h>

#include <json/json.hpp>

#include <iostream>
#include <string>

using namespace hg;

static std::string root_path;

std::string GetAbs(const std::string &path) { return IsPathAbsolute(path) ? path : PathJoin({root_path, path}); }

btVector3 to_btVector3(const Vec3 &v) { return btVector3(v.x, v.y, v.z); }
//
void SerializeCollisionMesh(void *serializeHandle, const void *const buffer, int size) { fwrite(buffer, size, 1, (FILE *)serializeHandle); }

bool AddGeometryToCollisionTree(btTriangleMesh *collision, const std::string &path, const Mat4 &mtx) {
	log(format("Loading geometry '%1'").arg(path));

	const auto geo = LoadGeometryFromFile(GetAbs(path).c_str());

	if (geo.vtx.empty() || geo.pol.empty()) {
		error(format("Failed to load geometry '%1'").arg(path));
		return false;
	}

	std::array<Vec3, 256> face;

	size_t tt = 0;
	for (auto &pol : geo.pol) {
		for (size_t i = 0; i < pol.vtx_count; ++i)
			face[i] = mtx * geo.vtx[geo.binding[tt + i]];
		
		for (auto i = 1; i < pol.vtx_count - 1; ++i)
			collision->addTriangle(to_btVector3(face[0]), to_btVector3(face[i + 1]), to_btVector3(face[i]));

		tt += pol.vtx_count;
	}

	return true;
}

bool AddGeometryToCollisionConvexTree(btConvexHullShape *collision, const std::string &path, const Mat4 &mtx) {
	log(format("Loading geometry '%1'").arg(path));

	const auto geo = LoadGeometryFromFile(GetAbs(path).c_str());

	if (geo.vtx.empty() || geo.pol.empty()) {
		error(format("Failed to load geometry '%1'").arg(path));
		return false;
	}

	size_t tt = 0;
	for (auto &pol : geo.pol) {
		for (size_t i = 0; i < pol.vtx_count; ++i) {
			Vec3 v = mtx * geo.vtx[geo.binding[tt + i]];
			collision->addPoint(to_btVector3(v), false);
		}
		tt += pol.vtx_count;
	}
	collision->recalcLocalAabb();
	collision->optimizeConvexHull();
	collision->initializePolyhedralFeatures();

	return true;
}

bool ProcessCollision(btDiscreteDynamicsWorld *world, nlohmann::json &i, const std::string &in, const std::string &out, bool debug) {
	bool rvalue = true;

	//
	std::string type = "tree";

	auto i_type = i.find("type");
	if (i_type != std::end(i))
		type = i_type->get<std::string>();

	//
	auto i_src = i.find("input");

	if (i_src != std::end(i)) {
		if (type == "tree") {
			btCollisionShape *collisionShape = nullptr;
			btTriangleMesh *triangleMesh = nullptr;

			for (auto j : *i_src) {
				Mat4 mtx = Mat4::Identity;

				auto i_mtx = j.find("matrix");
				if (i_mtx != std::end(j))
					mtx = i_mtx->get<Mat4>();

				auto i_type = j.find("type");
				std::string type_mesh;
				if (i_type != std::end(j))
					type_mesh = i_type->get<std::string>();

				auto i_geo = j.find("geometry"), i_scn = j.find("scene");

				if (i_geo != std::end(j)) {
					if (type_mesh == "triangle") {
						triangleMesh = new btTriangleMesh();
						if (!AddGeometryToCollisionTree(triangleMesh, i_geo->get<std::string>(), mtx))
							rvalue = false;
					} else if (type_mesh == "convex") {
						collisionShape = new btConvexHullShape();
						if (!AddGeometryToCollisionConvexTree(static_cast<btConvexHullShape *>(collisionShape), i_geo->get<std::string>(), mtx))
							rvalue = false;
					}

				} else {
					error("Invalid collision source");
					rvalue = false;
				}

				if (!rvalue)
					break;
			}

			if (triangleMesh) // create collision shape from triangle
				collisionShape = new btBvhTriangleMeshShape(triangleMesh, true);

			if (rvalue && collisionShape) {
				auto f = fopen(out.c_str(), "wb");

				if (f) {
					btDefaultSerializer serializer;
					serializer.registerNameForPointer(collisionShape, out.c_str());

					serializer.startSerialization();
					collisionShape->serializeSingleShape(&serializer);
					serializer.finishSerialization();

					fwrite(serializer.getBufferPointer(), serializer.getCurrentBufferSize(), 1, f);
					fclose(f);
				} else {
					rvalue = false;
				}
			}

			delete triangleMesh;
			delete collisionShape;
		} else {
			error(format("Unsupported collision output type '%1'").arg(type));
			rvalue = false;
		}
	}

	return rvalue;
}

//
static void OutputUsage(const CmdLineFormat &cmd_format) {
	std::cout << "Usage: bulletc " << word_wrap(FormatCmdLineArgs(cmd_format), 120, 14) << std::endl << std::endl;
	std::cout << FormatCmdLineArgsDescription(cmd_format);
}

int main(int narg, const char **args) {
	std::cout << "bulletc 1.0" << std::endl;

	// parse command line
	CmdLineFormat cmd_format = {
		{
			{"-debug", "Compile in debug mode (eg. do not perform any optimisation)"},
			{"-quiet", "Disable all build information but errors"},
			{"-verbose", "Output additional information about the compilation process"},
		},
		{
			{"-root", "Path to the source directory"},
		},
		{
			{"input", "Input .physic file"},
			{"output", "Output directory", true},
		},
		{
			{"-q", "-quiet"},
			{"-v", "-verbose"},
		},
	};

	CmdLineContent cmd_content;
	if (!ParseCmdLine({args + 1, args + narg}, cmd_format, cmd_content)) {
		OutputUsage(cmd_format);
		return -1;
	}

	root_path = GetCmdLineSingleValue(cmd_content, "-root", hg::GetCurrentWorkingDirectory());

	//
	int log_level = LL_Normal | LL_Warning | LL_Error;

	if (GetCmdLineFlagValue(cmd_content, "-quiet"))
		log_level = LL_Warning | LL_Error;

	if (GetCmdLineFlagValue(cmd_content, "-verbose"))
		log_level = LL_All; // verbose logs all

	set_log_level(log_level);

	// input folder compilation mode
	if (cmd_content.positionals.size() < 2) {
		std::cout << "Error: No input/output specified" << std::endl;
		OutputUsage(cmd_format);
		return -2;
	}

	const auto in = cmd_content.positionals[0];
	const auto out = cmd_content.positionals[1];

	const bool debug = GetCmdLineFlagValue(cmd_content, "-debug");

	//
	auto js = LoadJsonFromFile(in.c_str());
	if (js.empty())
		return -3; // empty js

	//
	auto collision_cfg = new btDefaultCollisionConfiguration;

	auto dispatcher = new btCollisionDispatcher(collision_cfg);
	auto broadphase = new btDbvtBroadphase;

	auto solver = new btSequentialImpulseConstraintSolver;

	auto world = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collision_cfg);

	int rvalue = 0;

	auto i = js.find("collision");

	if (i != std::end(js)) {
		if (!ProcessCollision(world, *i, in, out, debug))
			rvalue = -4; // collision directive error
	} else {
		error("Invalid physic file");
		rvalue = -5; // physic file error
	}

	delete world;
	return rvalue;
}
