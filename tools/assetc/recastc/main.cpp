// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include <foundation/build_info.h>
#include <foundation/cmd_line.h>
#include <foundation/format.h>
#include <foundation/log.h>
#include <foundation/path_tools.h>
#include <foundation/string.h>

#include <engine/geometry.h>
#include <engine/meta.h>
#include <engine/recast_detour.h>
#include <engine/to_json.h>

#include <DetourNavMeshBuilder.h>
#include <DetourNavMeshQuery.h>
#include <Recast.h>

#include <json/json.hpp>

#include <iostream>
#include <string>

using namespace hg;

static std::string root_path;

std::string GetAbs(const std::string &path) { return IsPathAbsolute(path) ? path : PathJoin({root_path, path}); }

//
static bool SavePathfindingNavMesh(const dtNavMesh *nav_mesh, const char *path) {
	auto f = fopen(path, "wb");
	if (f == nullptr)
		return false;

	struct NavMeshSetHeader {
		int magic;
		int version;
		int numTiles;
		dtNavMeshParams params;
	};

	struct NavMeshTileHeader {
		dtTileRef tileRef;
		int dataSize;
	};

	// store header
	NavMeshSetHeader header;
	header.magic = NAVMESHSET_MAGIC;
	header.version = NAVMESHSET_VERSION;
	header.numTiles = 0;
	for (int i = 0; i < nav_mesh->getMaxTiles(); ++i) {
		const dtMeshTile *tile = nav_mesh->getTile(i);
		if (!tile || !tile->header || !tile->dataSize)
			continue;
		header.numTiles++;
	}
	memcpy(&header.params, nav_mesh->getParams(), sizeof(dtNavMeshParams));
	fwrite(&header, sizeof(NavMeshSetHeader), 1, f);

	// store tiles
	for (int i = 0; i < nav_mesh->getMaxTiles(); ++i) {
		const dtMeshTile *tile = nav_mesh->getTile(i);
		if (!tile || !tile->header || !tile->dataSize)
			continue;

		NavMeshTileHeader tileHeader;
		tileHeader.tileRef = nav_mesh->getTileRef(tile);
		tileHeader.dataSize = tile->dataSize;
		fwrite(&tileHeader, sizeof(tileHeader), 1, f);

		fwrite(tile->data, tile->dataSize, 1, f);
	}

	fclose(f);
	return true;
}

bool ProcessPathfinding(nlohmann::json &i, const std::string &in, const std::string &out) {
	auto i_in = i.find("input");
	if (i_in == std::end(i))
		return false;

	//
	float agent_radius = 1.f;
	float agent_height = 1.8f;
	float max_slope = 0.1f;
	float max_climb = 0.4f;

	for (auto j : i.items()) {
		if (j.key() == "agent-radius")
			agent_radius = j.value();
		else if (j.key() == "agent-height")
			agent_height = j.value();
		else if (j.key() == "max-slope")
			max_slope = j.value();
		else if (j.key() == "max-climb")
			max_climb = j.value();
	}

	debug(format("Recast: agent_radius=%1, agent_height=%2, max_slope=%3, max_climb=%4").arg(agent_radius).arg(agent_height).arg(max_slope).arg(max_climb));

	//
	NavMeshInput nav_mesh_input;

	for (auto j : *i_in) {
		Mat4 mtx = Mat4::Identity;

		auto i_mtx = j.find("matrix");
		if (i_mtx != std::end(j))
			mtx = i_mtx->get<Mat4>();

		//
		auto i_geo = j.find("geometry");

		if (i_geo != std::end(j)) {
			auto path = i_geo->get<std::string>();
			log(format("Loading geometry '%1'").arg(path));
			const auto geo = LoadGeometryFromFile(GetAbs(path).c_str());

			if (geo.vtx.empty() || geo.pol.empty()) {
				error(format("Failed to load geometry '%1'").arg(path));
				return false;
			}

			AddGeometryToNavMeshInput(nav_mesh_input, geo, mtx);
		} else {
			error("Invalid pathfinding source");
			return false;
		}
	}

	auto nav_mesh = CreateNavMesh(nav_mesh_input, agent_radius, agent_height, max_slope, max_climb);
	if (!nav_mesh)
		return false;

	auto res = SavePathfindingNavMesh(nav_mesh, out.c_str());
	dtFree(nav_mesh);

	return res;
}

//
static void OutputUsage(const CmdLineFormat &cmd_format) {
	std::cout << "Usage: recastc " << word_wrap(FormatCmdLineArgs(cmd_format), 120, 14) << std::endl << std::endl;
	std::cout << FormatCmdLineArgsDescription(cmd_format);
}

int main(int narg, const char **args) {
	std::cout << "recastc 1.0" << std::endl;

	// parse command line
	CmdLineFormat cmd_format = {
		{
			{"-quiet", "Disable all build information but errors"},
			{"-verbose", "Output additional information about the compilation process"},
		},
		{
			{"-root", "Path to the source directory"},
		},
		{
			{"input", "Input .pathfinding file"},
			{"output", "Output Newton collision tree", true},
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
	if (cmd_content.positionals.empty()) {
		std::cout << "Error: No input specified" << std::endl;
		OutputUsage(cmd_format);
		return -2;
	}

	std::string in = cmd_content.positionals[0];
	std::string out = cmd_content.positionals.size() > 1 ? cmd_content.positionals[1] : root_path;

	const bool debug = GetCmdLineFlagValue(cmd_content, "-debug");

	//
	auto js = LoadJsonFromFile(GetAbs(in).c_str());
	if (js.empty())
		return -3; // empty js

	//
	auto i = js.find("pathfinding");

	if (i != std::end(js)) {
		if (!ProcessPathfinding(*i, in, out))
			return -4; // pathfinding directive error
	} else {
		error("Invalid pathfinding file");
		return -5; // pathfinding file error
	}

	return 0;
}
