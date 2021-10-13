// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/render_pipeline.h"
#include <bgfx/bgfx.h>

namespace hg {

Model CreateCubeModel(const bgfx::VertexLayout &decl, float x, float y, float z);
Model CreateSphereModel(const bgfx::VertexLayout &decl, float radius, int subdiv_x, int subdiv_y);
Model CreatePlaneModel(const bgfx::VertexLayout &decl, float width, float length, int subdiv_x, int subdiv_z);
Model CreateCylinderModel(const bgfx::VertexLayout &decl, float radius, float height, int subdiv_x);
Model CreateConeModel(const bgfx::VertexLayout &decl, float radius, float height, int subdiv_x);
Model CreateCapsuleModel(const bgfx::VertexLayout &decl, float radius, float height, int subdiv_x, int subdiv_y);

} // namespace hg
