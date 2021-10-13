// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include <cstdint>
#include <vector>

namespace hg {

using IsoSurface = std::vector<float>;

/// Create a new iso surface of the provided dimensions.
IsoSurface NewIsoSurface(int width, int height, int depth);

/// Render a sphere to an isosurface.
void IsoSurfaceSphere(IsoSurface &surface, int width, int height, int depth, float x, float y, float z, float radius, float value = 1.f, float exponent = 1.f);

/// Apply a 3x3x3 convolution kernel to an isosurface.
IsoSurface ConvoluteIsoSurface(const IsoSurface &surface, int width, int height, int depth, const float *kernel_3x3x3);
/// Apply a 3x3x3 gaussian blur to an isosurface.
IsoSurface GaussianBlurIsoSurface(const IsoSurface &surface, int width, int height, int depth);

//
struct ModelBuilder;

/// Convert an iso surface to a render model.
bool IsoSurfaceToModel(ModelBuilder &builder, const IsoSurface &surface, int width, int height, int depth, uint16_t material = 0, float isolevel = 0.5f,
	float scale_x = 1.f, float scale_y = 1.f, float scale_z = 1.f);

} // namespace hg
