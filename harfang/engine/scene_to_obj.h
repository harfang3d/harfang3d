// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include <string>
#include <vector>

namespace hg {

class Scene;

struct Node;
struct PipelineResources;

std::string ExportNodesToOBJ(const Scene &scene, const std::vector<Node> &nodes, const std::string &model_dir, const PipelineResources &resources);
std::string ExportSceneToOBJ(const Scene &scene, const std::string &model_dir, const PipelineResources &resources);

} // namespace hg
