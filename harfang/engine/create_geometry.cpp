// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/math.h"

#include "engine/create_geometry.h"
#include "engine/model_builder.h"

namespace hg {

Model CreateCubeModel(const bgfx::VertexLayout &decl, float x, float y, float z) {
	ModelBuilder builder;

	x /= 2.f;
	y /= 2.f;
	z /= 2.f;

	VtxIdxType a, b, c, d;

	a = builder.AddVertex({{-x, -y, -z}, {0.f, 0.f, -1.f}, {}, {}, {0, 0}}); // -Z
	b = builder.AddVertex({{-x, y, -z}, {0.f, 0.f, -1.f}, {}, {}, {0, 1}});
	c = builder.AddVertex({{x, y, -z}, {0.f, 0.f, -1.f}, {}, {}, {1, 1}});
	d = builder.AddVertex({{x, -y, -z}, {0.f, 0.f, -1.f}, {}, {}, {1, 0}});
	builder.AddPolygon({d, c, b, a});

	a = builder.AddVertex({{-x, -y, z}, {0.f, 0.f, 1.f}, {}, {}, {0, 0}}); // +Z
	b = builder.AddVertex({{-x, y, z}, {0.f, 0.f, 1.f}, {}, {}, {0, 1}});
	c = builder.AddVertex({{x, y, z}, {0.f, 0.f, 1.f}, {}, {}, {1, 1}});
	d = builder.AddVertex({{x, -y, z}, {0.f, 0.f, 1.f}, {}, {}, {1, 0}});
	builder.AddPolygon({a, b, c, d});

	a = builder.AddVertex({{-x, -y, -z}, {0.f, -1.f, 0.f}, {}, {}, {0, 0}}); // -Y
	b = builder.AddVertex({{-x, -y, z}, {0.f, -1.f, 0.f}, {}, {}, {0, 1}});
	c = builder.AddVertex({{x, -y, z}, {0.f, -1.f, 0.f}, {}, {}, {1, 1}});
	d = builder.AddVertex({{x, -y, -z}, {0.f, -1.f, 0.f}, {}, {}, {1, 0}});
	builder.AddPolygon({a, b, c, d});

	a = builder.AddVertex({{-x, y, -z}, {0.f, 1.f, 0.f}, {}, {}, {0, 0}}); // +Y
	b = builder.AddVertex({{-x, y, z}, {0.f, 1.f, 0.f}, {}, {}, {0, 1}});
	c = builder.AddVertex({{x, y, z}, {0.f, 1.f, 0.f}, {}, {}, {1, 1}});
	d = builder.AddVertex({{x, y, -z}, {0.f, 1.f, 0.f}, {}, {}, {1, 0}});
	builder.AddPolygon({d, c, b, a});

	a = builder.AddVertex({{-x, -y, -z}, {-1.f, 0.f, 0.f}, {}, {}, {0, 0}}); // -X
	b = builder.AddVertex({{-x, -y, z}, {-1.f, 0.f, 0.f}, {}, {}, {0, 1}});
	c = builder.AddVertex({{-x, y, z}, {-1.f, 0.f, 0.f}, {}, {}, {1, 1}});
	d = builder.AddVertex({{-x, y, -z}, {-1.f, 0.f, 0.f}, {}, {}, {1, 0}});
	builder.AddPolygon({d, c, b, a});

	a = builder.AddVertex({{x, -y, -z}, {1.f, 0.f, 0.f}, {}, {}, {0, 0}}); // +X
	b = builder.AddVertex({{x, -y, z}, {1.f, 0.f, 0.f}, {}, {}, {0, 1}});
	c = builder.AddVertex({{x, y, z}, {1.f, 0.f, 0.f}, {}, {}, {1, 1}});
	d = builder.AddVertex({{x, y, -z}, {1.f, 0.f, 0.f}, {}, {}, {1, 0}});
	builder.AddPolygon({a, b, c, d});

	builder.EndList(0);

	return builder.MakeModel(decl);
}

Model CreateSphereModel(const bgfx::VertexLayout &decl, float radius, int subdiv_x, int subdiv_y) {
	ModelBuilder builder;

	const auto i_top = builder.AddVertex({{0.f, radius, 0.f}, {0.f, 1.f, 0.f}});
	const auto i_bottom = builder.AddVertex({{0.f, -radius, 0.f}, {0.f, -1.f, 0.f}});

	std::vector<uint16_t> ref(subdiv_y + 1), old_ref;

	for (int s = 0; s < subdiv_x + 1; ++s) {
		const auto t = (s + 1.f) / (subdiv_x + 2.f);
		const auto a = t * Pi;

		const auto section_y = Cos(a) * radius;
		const auto section_radius = Sin(a) * radius;

		uint16_t i, j;

		const Vec3 v = {section_radius, section_y, 0.f};
		ref[0] = i = builder.AddVertex({v, Normalize(v)});

		for (int c = 1; c <= subdiv_y; ++c) {
			const auto c_a = float(c) * TwoPi / subdiv_y;

			const Vec3 v = {Cos(c_a) * section_radius, section_y, Sin(c_a) * section_radius};
			j = builder.AddVertex({v, Normalize(v)});

			if (s == 0) // top section
				builder.AddTriangle(i_top, i, j);
			else // middle sections
				builder.AddPolygon({old_ref[c], old_ref[c - 1], i, j});

			if (s == subdiv_x) // bottom section
				builder.AddTriangle(i, i_bottom, j);

			ref[c] = i = j;
		}

		old_ref = ref;
	}

	builder.EndList(0);

	return builder.MakeModel(decl);
}

Model CreatePlaneModel(const bgfx::VertexLayout &decl, float width, float length, int subdiv_x, int subdiv_z) {
	ModelBuilder builder;

	float dx = width / static_cast<float>(subdiv_x);
	float dz = length / static_cast<float>(subdiv_z);

	float z0 = -length / 2.f;

	for (int j = 0; j < subdiv_z; j++) {
		float z1 = z0 + dz;
		float x0 = -width / 2.f;

		for (int i = 0; i < subdiv_x; i++) {
			float x1 = x0 + dx;

			uint16_t a = builder.AddVertex({{x0, 0.f, z0}, {0.f, 1.f, 0.f}, {}, {}, {float(i) / subdiv_x, float(j) / subdiv_z}});
			uint16_t b = builder.AddVertex({{x0, 0.f, z1}, {0.f, 1.f, 0.f}, {}, {}, {float(i) / subdiv_x, float(j + 1) / subdiv_z}});
			uint16_t c = builder.AddVertex({{x1, 0.f, z1}, {0.f, 1.f, 0.f}, {}, {}, {float(i + 1) / subdiv_x, float(j + 1) / subdiv_z}});
			uint16_t d = builder.AddVertex({{x1, 0.f, z0}, {0.f, 1.f, 0.f}, {}, {}, {float(i + 1) / subdiv_x, float(j) / subdiv_z}});
			builder.AddPolygon({d, c, b, a});

			x0 = x1;
		}
		z0 = z1;
	}

	builder.EndList(0);

	return builder.MakeModel(decl);
}

Model CreateCylinderModel(const bgfx::VertexLayout &decl, float radius, float height, int subdiv_x) {
	ModelBuilder builder;

	if (subdiv_x < 3) {
		subdiv_x = 3;
	}

	std::vector<VtxIdxType> ref(subdiv_x * 2);
	const float z = height / 2.f;
	for (int i = 0; i < subdiv_x; i++) {
		float t = 2.f * Pi * i / static_cast<float>(subdiv_x);
		float cs = Cos(t);
		float sn = Sin(t);
		float x = radius * cs, y = radius * sn;
		ref[2 * i] = builder.AddVertex({{x, z, y}, {cs, 0.f, sn}});
		ref[2 * i + 1] = builder.AddVertex({{x, -z, y}, {cs, 0.f, sn}});
	}

	VtxIdxType a = ref[2 * subdiv_x - 2];
	VtxIdxType b = ref[2 * subdiv_x - 1];
	for (int i = 0; i < subdiv_x; i++) {
		const auto c = ref[2 * i + 1];
		const auto d = ref[2 * i];
		builder.AddPolygon({a, b, c, d});
		a = d;
		b = c;
	}

	std::vector<VtxIdxType> cap(subdiv_x + 2);
	cap[0] = builder.AddVertex({{0, z, 0}, {0, 1, 0}});
	for (int i = 0; i < subdiv_x; i++) {
		cap[i + 1] = ref[2 * i];
	}
	cap[subdiv_x + 1] = ref[0];
	builder.AddPolygon(cap);

	cap[0] = builder.AddVertex({{0, -z, 0}, {0, -1, 0}});
	for (int i = 0; i < subdiv_x; i++) {
		cap[i + 1] = ref[2 * (subdiv_x - i) - 1];
	}
	cap[subdiv_x + 1] = ref[2 * subdiv_x - 1];
	builder.AddPolygon(cap);

	builder.EndList(0);

	return builder.MakeModel(decl);
}

Model CreateConeModel(const bgfx::VertexLayout &decl, float radius, float height, int subdiv_x) {
	ModelBuilder builder;

	if (subdiv_x < 3) {
		subdiv_x = 3;
	}

	std::vector<VtxIdxType> ref(subdiv_x + 2);
	for (int i = 0; i < subdiv_x; i++) {
		float t = 2.f * Pi * i / static_cast<float>(subdiv_x);
		float cs = Cos(t);
		float sn = Sin(t);
		float x = radius * cs, y = radius * sn;
		ref[i + 1] = builder.AddVertex({{x, 0, y}, {cs, 0.f, sn}});
	}
	ref[subdiv_x + 1] = ref[0];

	ref[0] = builder.AddVertex({{0, height, 0}, {0, 1, 0}});
	builder.AddPolygon(ref);

	std::reverse(ref.begin() + 1, ref.end());

	ref[0] = builder.AddVertex({{0, 0, 0}, {0, -1, 0}});
	builder.AddPolygon(ref);

	builder.EndList(0);

	return builder.MakeModel(decl);
}

Model CreateCapsuleModel(const bgfx::VertexLayout &decl, float radius, float height, int subdiv_x, int subdiv_y) {
	if (subdiv_x < 3)
		subdiv_x = 3;
	if (subdiv_y < 1)
		subdiv_y = 1;

	if (height < (2.f * radius))
		return CreateSphereModel(decl, radius, subdiv_x, subdiv_y);

	ModelBuilder builder;

	// cylinder
	std::vector<VtxIdxType> ref(subdiv_x * 2);
	const float z = height / 2.f - radius;
	for (int i = 0; i < subdiv_x; i++) {
		float t = 2.f * Pi * i / static_cast<float>(subdiv_x);
		float cs = Cos(t);
		float sn = Sin(t);
		float x = radius * cs, y = radius * sn;
		ref[2 * i] = builder.AddVertex({{x, z, y}, {cs, 0.f, sn}});
		ref[2 * i + 1] = builder.AddVertex({{x, -z, y}, {cs, 0.f, sn}});
	}

	VtxIdxType a0, b0, a1, b1;
	a0 = ref[2 * subdiv_x - 2];
	b0 = ref[2 * subdiv_x - 1];
	for (int i = 0; i < subdiv_x; i++) {
		VtxIdxType c0 = ref[2 * i + 1];
		VtxIdxType d0 = ref[2 * i];
		builder.AddPolygon({a0, b0, c0, d0});
		a0 = d0;
		b0 = c0;
	}

	VtxIdxType top = builder.AddVertex({{0.f, height / 2.f, 0.f}, {0.f, 1.f, 0.f}});
	VtxIdxType bottom = builder.AddVertex({{0.f, -height / 2.f, 0.f}, {0.f, -1.f, 0.f}});

	std::vector<VtxIdxType> fan(2 + subdiv_x);

	if (subdiv_y == 1) {
		// top cap
		fan[0] = top;
		for (int i = 0; i < subdiv_x; i++) {
			fan[i + 1] = ref[2 * i];
		}
		fan[subdiv_x + 1] = fan[1];
		builder.AddPolygon(fan);

		// bottom cap
		fan[0] = bottom;
		for (int i = 0; i < subdiv_x; i++) {
			fan[i + 1] = ref[2 * (subdiv_x - 1 - i) + 1];
		}
		fan[subdiv_x + 1] = fan[1];
		builder.AddPolygon(fan);

		return builder.MakeModel(decl);
	}

	// hemispherical caps
	std::vector<VtxIdxType> cap(subdiv_x * 2 * (subdiv_y - 1));
	for (int i = 0, k = 0; i < subdiv_x; i++) {
		float t0 = 2.f * Pi * i / static_cast<float>(subdiv_x);
		float c0 = radius * Cos(t0);
		float s0 = radius * Sin(t0);
		for (int j = 1; j < subdiv_y; j++) {
			float t1 = Pi * (j / static_cast<float>(subdiv_y)) / 2.f;
			Vec3 v = {c0 * Sin(t1), radius * Cos(t1), s0 * Sin(t1)};
			cap[k++] = builder.AddVertex({{v.x, v.y + z, v.z}, Normalize(v)});
			v.y = -v.y;
			cap[k++] = builder.AddVertex({{v.x, v.y - z, v.z}, Normalize(v)});
		}
	}

	// top cap
	fan[0] = top;
	for (int i = 0; i < subdiv_x; i++)
		fan[i + 1] = cap[2 * (subdiv_y - 1) * i];

	fan[subdiv_x + 1] = fan[1];
	builder.AddPolygon(fan);

	// bottom cap
	fan[0] = bottom;
	for (int i = 0; i < subdiv_x; i++)
		fan[i + 1] = cap[2 * (subdiv_y - 1) * (subdiv_x - 1 - i) + 1];

	fan[subdiv_x + 1] = fan[1];
	builder.AddPolygon(fan);

	// inner
	for (int j = 0; j < (subdiv_y - 2); j++) {
		int k = 2 * ((subdiv_y - 1) * (subdiv_x - 1) + j);
		// top
		a0 = cap[k];
		b0 = cap[k + 2];
		// bottom
		a1 = cap[k + 3];
		b1 = cap[k + 1];
		for (int i = 0; i < subdiv_x; i++) {
			k = 2 * ((subdiv_y - 1) * i + j);
			VtxIdxType c0 = cap[k + 2];
			VtxIdxType d0 = cap[k];
			builder.AddPolygon({a0, b0, c0, d0});
			a0 = d0;
			b0 = c0;

			VtxIdxType c1 = cap[k + 1];
			VtxIdxType d1 = cap[k + 3];
			builder.AddPolygon({a1, b1, c1, d1});
			a1 = d1;
			b1 = c1;
		}
	}

	// junction
	int offset = 2 * ((subdiv_y - 1) * subdiv_x - 1);
	a0 = cap[offset];
	b1 = cap[offset + 1];
	offset = 2 * (subdiv_x - 1);
	b0 = ref[offset];
	a1 = ref[offset + 1];
	for (int i = 0; i < subdiv_x; i++) {
		offset = 2 * i;
		VtxIdxType c0 = ref[offset];
		VtxIdxType d1 = ref[offset + 1];
		offset = 2 * ((subdiv_y - 1) * (i + 1) - 1);
		VtxIdxType d0 = cap[offset];
		VtxIdxType c1 = cap[offset + 1];
		builder.AddPolygon({a0, b0, c0, d0});
		builder.AddPolygon({a1, b1, c1, d1});
		a0 = d0;
		b0 = c0;
		a1 = d1;
		b1 = c1;
	}

	builder.EndList(0);

	return builder.MakeModel(decl);
}

} // namespace hg

#if 0

std::shared_ptr<Geometry> CreateGeometryFromHeightmap(uint32_t width, uint32_t height, const float *heightmap, float scale, const std::string &_material_path, const std::string &_name) {
	auto geo = std::make_shared<Geometry>();

	std::string material_path = _material_path.empty() ? default_material_path : _material_path;
	std::string name = _name.empty() ? format("@gen/geo_heightmap_%1_%2_%3_%4").arg(width).arg(height).arg(scale).arg(material_path) : _name;

	geo->SetName(name.c_str());

	geo->AllocateMaterialTable(1);
	geo->materials[0] = material_path;

	// generate vertices
	auto heightmap_size = width * height;
	geo->AllocateVertex(heightmap_size);

	int count_vtx = 0;
	for (uint32_t i = 0; i < width; ++i)
		for (uint32_t j = 0; j < height; ++j)
			geo->SetVertex(count_vtx++, Vec3((float(i) / width) * scale, (float(j) / height) * scale, heightmap[i + j * width]));

	// build polygons
	int polygon_count = (width - 1) * (height - 1);
	geo->AllocatePolygon(polygon_count);
	for (int i = 0; i < polygon_count; ++i)
		geo->SetPolygon(i, 4, 0);

	geo->AllocateRgb(polygon_count * 4);
	geo->AllocateUVChannels(1, polygon_count * 4);
	geo->AllocatePolygonBinding();

	int count_poly = 0;
	for (uint32_t i = 0; i < width - 1; ++i) {
		for (uint32_t j = 0; j < height - 1; ++j) {

			int idx[] = {int(i + j * width), int((i + 1) + j * width), int((i + 1) + (j + 1) * width), int(i + (j + 1) * width)};
			geo->SetPolygonBinding(count_poly, 4, idx);

			Color col[] = {Color::One, Color::One, Color::One, Color::One};
			geo->SetRgb(count_poly, 4, col);

			Vec2 uvs[] = {
				{1.f - (float(i) / width), float(j) / height},
				{1.f - (float(i) / width), (j + 1.f) / height},
				{1.f - ((float(i) + 1.f) / width), (j + 1.f) / height},
				{1.f - ((i + 1.f) / width), float(j) / height}};
			geo->SetUV(0, count_poly, 4, uvs);
			++count_poly;
		}
	}

	geo->ComputeVertexNormal(Deg(40.f));
	geo->ComputeVertexTangent();

	return geo;
}

} // namespace hg

#endif
