// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/scene_to_obj.h"
#include "engine/geometry.h"
#include "engine/scene.h"

#include "foundation/format.h"
#include "foundation/path_tools.h"
#include "foundation/matrix3.h"

namespace hg {

struct IndexState {
	size_t last_idx_vtx = 1;
	size_t last_idx_uv = 1;
	size_t last_idx_nrm = 1;
};

std::string ExportNodeToOBJ(const Node &node, const std::string &model_dir, const PipelineResources &resources, IndexState &idx) {
	std::string out = format("# Node '%1'\n").arg(node.GetName());

	auto trs = node.GetTransform();
	if (!trs)
		return {};

	const auto world = trs.GetWorld();

	Mat3 normal = Transpose(Mat3(InverseFast(world)));

	//
	auto mdl_ref = node.GetObject().GetModelRef();
	if (!resources.models.IsValidRef(mdl_ref))
		return {};

	auto mdl_name = resources.models.GetName_unsafe_(mdl_ref.ref.idx);

	//
	out += format("o %1\n").arg(node.GetName()).str();

	//
	const auto geo = LoadGeometryFromFile(PathJoin({model_dir, mdl_name}).c_str());

	out += format("# %1 vertices\n").arg(geo.vtx.size()).str();
	for (const auto &vtx : geo.vtx) {
		const auto tv = world * vtx;
		out += format("v %1 %2 %3\n").arg(-tv.x).arg(tv.y).arg(tv.z).str();
	}
	out += "\n";

	out += format("# %1 UVs\n").arg(geo.uv[0].size()).str();
	for (const auto &uv : geo.uv[0])
		out += format("vt %1 %2\n").arg(uv.x).arg(uv.y).str();
	out += "\n";

	out += format("# %1 normals\n").arg(geo.normal.size()).str();
	for (const auto &n : geo.normal) {
		const auto tn = normal * n;
		out += format("vn %1 %2 %3\n").arg(-tn.x).arg(tn.y).arg(tn.z).str();
	}
	out += "\n";

	const auto pol_idx = ComputePolygonIndex(geo);

	out += format("# %1 polygons\n").arg(geo.pol.size()).str();
	for (size_t j = 0; j < geo.pol.size(); ++j) {
		const auto k = pol_idx[j];

		out += "f ";
		const auto &pol = geo.pol[j];
		for (size_t i = 0; i < pol.vtx_count; ++i)
			out += format("%1/%2/%3 ").arg(geo.binding[k + i] + idx.last_idx_vtx).arg(k + i + idx.last_idx_uv).arg(k + i + idx.last_idx_nrm).str();
		out += "\n";
	}
	out += "\n";

	idx.last_idx_vtx += geo.vtx.size();
	idx.last_idx_uv += geo.uv[0].size();
	idx.last_idx_nrm += geo.normal.size();

	return out;
}

static std::vector<Node> ExpandInstanceNodes(const Scene &scene, const std::vector<Node> &nodes) {
	std::vector<Node> out = nodes;

	for (auto node : nodes)
		if (node.HasInstance()) {
			const auto instantiated_nodes = ExpandInstanceNodes(scene, node.GetInstanceSceneView().GetNodes(scene));
			std::copy(std::begin(instantiated_nodes), std::end(instantiated_nodes), std::back_inserter(out));
		}

	return out;
}

std::string ExportNodesToOBJ(const Scene &scene, const std::vector<Node> &nodes_, const std::string &model_dir, const PipelineResources &resources) {
	std::string out;

	const auto nodes = ExpandInstanceNodes(scene, nodes_);

	out = "# Harfang scene export to OBJ\n";
	out += format("# %1 node(s)\n").arg(nodes.size()).str();
	out += "\n";

	IndexState idx_state;
	for (const auto &node : nodes)
		out += ExportNodeToOBJ(node, model_dir, resources, idx_state);

	out += "# end of scene\n";
	return out;
}

std::string ExportSceneToOBJ(const Scene &scene, const std::string &model_dir, const PipelineResources &resources) {
	return ExportNodesToOBJ(scene, scene.GetAllNodesWithComponent(NCI_Object), model_dir, resources);
}

} // namespace hg
