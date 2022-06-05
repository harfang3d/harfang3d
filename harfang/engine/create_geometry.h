// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/render_pipeline.h"
#include <bgfx/bgfx.h>

namespace hg {

/// Create a cube render model.
/// @see CreateCapsuleModel, CreateConeModel, CreateCylinderModel, CreatePlaneModel, CreateSphereModel and DrawModel.
Model CreateCubeModel(const bgfx::VertexLayout &decl, float x, float y, float z);
/// Create a sphere render model.
/// @see CreateCubeModel, CreateConeModel, CreateCylinderModel, CreatePlaneModel, CreateCapsuleModel and DrawModel.
Model CreateSphereModel(const bgfx::VertexLayout &decl, float radius, int subdiv_x, int subdiv_y);
/// Create a plane render model.
/// @see CreateCubeModel, CreateConeModel, CreateCylinderModel, CreatePlaneModel, CreateCapsuleModel and DrawModel.
Model CreatePlaneModel(const bgfx::VertexLayout &decl, float width, float length, int subdiv_x, int subdiv_z);
/// Create a cylinder render model.
/// @see CreateCubeModel, CreateConeModel, CreateCylinderModel, CreatePlaneModel, CreateCapsuleModel and DrawModel.
Model CreateCylinderModel(const bgfx::VertexLayout &decl, float radius, float height, int subdiv_x);
/// Create a cone render model.
/// @see CreateCubeModel, CreateConeModel, CreateCylinderModel, CreatePlaneModel, CreateCapsuleModel and DrawModel.
Model CreateConeModel(const bgfx::VertexLayout &decl, float radius, float height, int subdiv_x);
/// Create a capsule render model.
/// @see CreateCubeModel, CreateConeModel, CreateCylinderModel, CreatePlaneModel, CreateCapsuleModel and DrawModel.
Model CreateCapsuleModel(const bgfx::VertexLayout &decl, float radius, float height, int subdiv_x, int subdiv_y);

} // namespace hg
