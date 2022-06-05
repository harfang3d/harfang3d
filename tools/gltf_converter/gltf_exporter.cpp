// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/assets.h"

#include <engine/vertex.h>
#include <engine/geometry.h>
#include <engine/model_builder.h>
#include <engine/node.h>
#include <engine/physics.h>
#include <engine/render_pipeline.h>
#include <engine/forward_pipeline.h>
#include <engine/scene.h>

#include <foundation/build_info.h>
#include <foundation/cext.h>
#include <foundation/cmd_line.h>
#include <foundation/dir.h>
#include <foundation/format.h>
#include <foundation/log.h>
#include <foundation/math.h>
#include <foundation/matrix3.h>
#include <foundation/matrix4.h>
#include <foundation/pack_float.h>
#include <foundation/path_tools.h>
#include <foundation/projection.h>
#include <foundation/string.h>
#include <foundation/time.h>
#include <foundation/vector3.h>

#include "stb_image.h"

#include "tiny_gltf.h"

#include <fstream>
#include <iostream>
#include <mutex>

#undef CopyFile
#undef GetObject

#include <foundation/file.h>

#include "platform/window_system.h"

#include "limits.h"
#include <vector>

using namespace tinygltf;

//
static inline const hg::Material::Value *GetMaterialValue(const hg::Material &mat, const std::string &name) {
	const auto &i = mat.values.find(name);
	return i != std::end(mat.values) ? &i->second : (hg::Material::Value *)nullptr;
}

std::map<hg::NodeRef, int> NodeRef_to_IdNode;

/// Adapts an array of bytes to an array of T. Will advace of byte_stride each
/// elements.
template <typename T> struct arrayAdapter {
	/// Pointer to the bytes
	const unsigned char *dataPtr;
	/// Number of elements in the array
	const size_t elemCount;
	/// Stride in bytes between two elements
	const size_t stride;

	/// Construct an array adapter.
	/// \param ptr Pointer to the start of the data, with offset applied
	/// \param count Number of elements in the array
	/// \param byte_stride Stride betweens elements in the array
	arrayAdapter(const unsigned char *ptr, size_t count, size_t byte_stride) : dataPtr(ptr), elemCount(count), stride(byte_stride) {}

	/// Returns a *copy* of a single element. Can't be used to modify it.
	T operator[](size_t pos) const {
		if (pos >= elemCount)
			throw std::out_of_range("Tried to access beyond the last element of an array adapter with "
									"count " +
									std::to_string(elemCount) + " while getting elemnet number " + std::to_string(pos));
		return *(reinterpret_cast<const T *>(dataPtr + pos * stride));
	}
};

/// Interface of any adapted array that returns integer data
struct intArrayBase {
	virtual ~intArrayBase() = default;
	virtual unsigned int operator[](size_t) const = 0;
	virtual size_t size() const = 0;
};

/// Interface of any adapted array that returns float data
struct floatArrayBase {
	virtual ~floatArrayBase() = default;
	virtual float operator[](size_t) const = 0;
	virtual size_t size() const = 0;
};

/// An array that loads interger types, returns them as int
template <class T> struct intArray : public intArrayBase {
	arrayAdapter<T> adapter;

	explicit intArray(const arrayAdapter<T> &a) : adapter(a) {}
	unsigned int operator[](size_t position) const override { return static_cast<unsigned int>(adapter[position]); }

	size_t size() const override { return adapter.elemCount; }
};

template <class T> struct floatArray : public floatArrayBase {
	arrayAdapter<T> adapter;

	explicit floatArray(const arrayAdapter<T> &a) : adapter(a) {}
	float operator[](size_t position) const override { return static_cast<float>(adapter[position]); }

	size_t size() const override { return adapter.elemCount; }
};

struct v2fArray {
	arrayAdapter<hg::Vec2> adapter;
	explicit v2fArray(const arrayAdapter<hg::Vec2> &a) : adapter(a) {}

	hg::Vec2 operator[](size_t position) const { return adapter[position]; }
	size_t size() const { return adapter.elemCount; }
};

struct v3fArray {
	arrayAdapter<hg::Vec3> adapter;
	explicit v3fArray(const arrayAdapter<hg::Vec3> &a) : adapter(a) {}

	hg::Vec3 operator[](size_t position) const { return adapter[position]; }
	size_t size() const { return adapter.elemCount; }
};

struct v4fArray {
	arrayAdapter<hg::Vec4> adapter;
	explicit v4fArray(const arrayAdapter<hg::Vec4> &a) : adapter(a) {}

	hg::Vec4 operator[](size_t position) const { return adapter[position]; }
	size_t size() const { return adapter.elemCount; }
};

struct Config {
	std::string input_path;
	std::string name; // output name (may be empty)
	std::string base_output_path{"./"};
	std::string compiled_output_path{"./"};
	std::string source_output_path{"./"};
	bool binary;
	bool filter_disabled_node;
};

static bool GetOutputPath(std::string &path, const std::string &base, const std::string &name, const std::string &prefix, const std::string &ext) {
	if (base.empty())
		return false;

	const auto filename = name.empty() ? prefix : (prefix.empty() ? name : prefix + "-" + name);
	path = hg::CleanPath(base + "/" + filename + "." + ext);

	return true;
}

#define for_with_index(V, ...)                                                                                                                                 \
	for (size_t V = 0, _brk_ = 0, _i_ = 1; _i_; _i_ = 0)                                                                                                       \
		for (__VA_ARGS__)                                                                                                                                      \
			if (_brk_)                                                                                                                                         \
				break;                                                                                                                                         \
			else                                                                                                                                               \
				for (_brk_ = 1; _brk_; _brk_ = 0, V++)

template <typename T> uint32_t serialize(std::vector<T> const &from, std::vector<unsigned char> &to, std::size_t offset) {
	uint32_t bytesToSerialize = sizeof(T) * static_cast<uint32_t>(from.size());

	to.resize(to.size() + bytesToSerialize);
	std::memcpy(&to[offset], &from[0], bytesToSerialize);

	return bytesToSerialize;
}

std::string GetTargetPathGltf(const std::string &hg_target) {
	// get target
	if (hg_target == "Position")
		return "translation";
	else if (hg_target == "Rotation")
		return "rotation";
	else if (hg_target == "Scale")
		return "scale";
	else if (hg_target == "Weights")
		return "weights";
	return "";
}

//
static void ExportMotions(Model &model, hg::Scene &scene) {
	for (const auto &scene_anime_ref : scene.GetSceneAnims()) {
		tinygltf::Animation animation;
		hg::SceneAnim *scene_anim = scene.GetSceneAnim(scene_anime_ref);

		for (const auto &node_anim : scene_anim->node_anims) {
			tinygltf::AnimationSampler sampler;
			tinygltf::AnimationChannel channel;

			auto *anim = scene.GetAnim(node_anim.anim);

			//check if anim track can be exported TODO maybe add some other type
			if (!anim->vec3_tracks.size() && !anim->quat_tracks.size())
				continue;

			Buffer InputBuffer, OutputBuffer;
			BufferView InputBufferView, OutputBufferView;
			Accessor InputAccessor, OutputAccessor;

			InputAccessor.byteOffset = 0;
			OutputAccessor.byteOffset = 0;

			InputAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
			InputAccessor.type = TINYGLTF_TYPE_SCALAR;

			if (anim->quat_tracks.size()) {
				OutputAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
				OutputAccessor.type = TINYGLTF_TYPE_VEC4;

				std::vector<float> input;
				float input_min = UINT_MAX;
				float input_max = 0;

				std::vector<float> output;
				hg::Vec4 output_min(std::numeric_limits<float>::max()), output_max(std::numeric_limits<float>::lowest());

				for (const auto &key : anim->quat_tracks[0].keys) {
					// t
					auto t = hg::time_to_sec_f(key.t);
					input.push_back(t);
					input_min = t < input_min ? t : input_min;
					input_max = t > input_max ? t : input_max;

					// add quat
					auto quat = key.v;
					auto r = hg::ToEuler(quat);
					quat = hg::QuaternionFromEuler(-r.x, -r.y, r.z);

					output.push_back(quat.x);
					output.push_back(quat.y);
					output.push_back(quat.z);
					output.push_back(quat.w);

					output_min.x = quat.x < output_min.x ? quat.x : output_min.x;
					output_min.y = quat.y < output_min.y ? quat.y : output_min.y;
					output_min.z = quat.z < output_min.z ? quat.z : output_min.z;
					output_min.w = quat.w < output_min.w ? quat.w : output_min.w;
					output_max.x = quat.x > output_max.x ? quat.x : output_max.x;
					output_max.y = quat.y > output_max.y ? quat.y : output_max.y;
					output_max.z = quat.z > output_max.z ? quat.z : output_max.z;
					output_max.w = quat.w > output_max.w ? quat.z : output_max.w;
				}

				// t
				uint32_t indexSize = serialize(input, InputBuffer.data, 0);

				InputAccessor.minValues.push_back(input_min);
				InputAccessor.maxValues.push_back(input_max);
				InputAccessor.count = input.size();
				InputBufferView.byteLength = InputBuffer.data.size();

				// save input buffer
				InputBufferView.buffer = model.buffers.size();
				model.buffers.push_back(InputBuffer);
				InputAccessor.bufferView = model.bufferViews.size();
				model.bufferViews.push_back(InputBufferView);
				sampler.input = model.accessors.size();
				model.accessors.push_back(InputAccessor);

				// quat
				serialize(output, OutputBuffer.data, 0);

				OutputAccessor.minValues.push_back(output_min.x);
				OutputAccessor.minValues.push_back(output_min.y);
				OutputAccessor.minValues.push_back(output_min.z);
				OutputAccessor.minValues.push_back(output_min.w);
				OutputAccessor.maxValues.push_back(output_max.x);
				OutputAccessor.maxValues.push_back(output_max.y);
				OutputAccessor.maxValues.push_back(output_max.z);
				OutputAccessor.maxValues.push_back(output_max.w);
				OutputAccessor.count = output.size() / 4;
				OutputBufferView.byteLength = OutputBuffer.data.size();

				// save output buffer
				OutputBufferView.buffer = model.buffers.size();
				model.buffers.push_back(OutputBuffer);
				OutputAccessor.bufferView = model.bufferViews.size();
				model.bufferViews.push_back(OutputBufferView);

				sampler.output = model.accessors.size();
				model.accessors.push_back(OutputAccessor);

				// get target
				channel.target_path = GetTargetPathGltf(anim->quat_tracks[0].target);
			} else if (anim->vec3_tracks.size()) {
				OutputAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
				OutputAccessor.type = TINYGLTF_TYPE_VEC3;

				std::vector<float> input;
				float input_min = UINT_MAX;
				float input_max = 0;

				std::vector<float> output;
				hg::Vec3 output_min(std::numeric_limits<float>::max()), output_max(std::numeric_limits<float>::lowest());

				for (const auto &key : anim->vec3_tracks[0].keys) {
					// t
					auto t = hg::time_to_sec_f(key.t);
					input.push_back(t);
					input_min = t < input_min ? t : input_min;
					input_max = t > input_max ? t : input_max;

					// add vec
					auto vec = key.v;
					vec.z = -vec.z;
					output.push_back(vec.x);
					output.push_back(vec.y);
					output.push_back(vec.z);

					output_min.x = vec.x < output_min.x ? vec.x : output_min.x;
					output_min.y = vec.y < output_min.y ? vec.y : output_min.y;
					output_min.z = vec.z < output_min.z ? vec.z : output_min.z;
					output_max.x = vec.x > output_max.x ? vec.x : output_max.x;
					output_max.y = vec.y > output_max.y ? vec.y : output_max.y;
					output_max.z = vec.z > output_max.z ? vec.z : output_max.z;
				}

				// t
				uint32_t indexSize = serialize(input, InputBuffer.data, 0);

				InputAccessor.minValues.push_back(input_min);
				InputAccessor.maxValues.push_back(input_max);
				InputAccessor.count = input.size();
				InputBufferView.byteLength = InputBuffer.data.size();

				// save input buffer
				InputBufferView.buffer = model.buffers.size();
				model.buffers.push_back(InputBuffer);
				InputAccessor.bufferView = model.bufferViews.size();
				model.bufferViews.push_back(InputBufferView);
				sampler.input = model.accessors.size();
				model.accessors.push_back(InputAccessor);

				// quat
				serialize(output, OutputBuffer.data, 0);

				OutputAccessor.minValues.push_back(output_min.x);
				OutputAccessor.minValues.push_back(output_min.y);
				OutputAccessor.minValues.push_back(output_min.z);
				OutputAccessor.maxValues.push_back(output_max.x);
				OutputAccessor.maxValues.push_back(output_max.y);
				OutputAccessor.maxValues.push_back(output_max.z);
				OutputAccessor.count = output.size() / 3;
				OutputBufferView.byteLength = OutputBuffer.data.size();

				// save output buffer
				OutputBufferView.buffer = model.buffers.size();
				model.buffers.push_back(OutputBuffer);
				OutputAccessor.bufferView = model.bufferViews.size();
				model.bufferViews.push_back(OutputBufferView);

				sampler.output = model.accessors.size();
				model.accessors.push_back(OutputAccessor);

				// get target
				channel.target_path = GetTargetPathGltf(anim->vec3_tracks[0].target);
			}

			channel.target_node = NodeRef_to_IdNode[node_anim.node];
			channel.sampler = animation.samplers.size();

			animation.samplers.push_back(sampler);
			animation.channels.push_back(channel);
			animation.name = scene_anim->name;
		}

		if(animation.channels.size())
			model.animations.push_back(animation);
	}
}

std::map<std::string, size_t> textures_cache;

static size_t ExportTexture(Model &model, const hg::TextureRef &tex_ref, const Config &config, hg::PipelineResources &resources) {
	auto texture_name = resources.textures.GetName(tex_ref);
	if (texture_name == "")
		return -1;

	std::string texture_path(hg::PathJoin({config.source_output_path, texture_name}));
	if (textures_cache.find(texture_path) == textures_cache.end()) {
		Image image;
		Texture texture;
		hg::Picture pic;

		// check extension is png or jpg, else convert to png
		auto file_ext = hg::GetFileExtension(texture_name);
		if (file_ext != "png" && file_ext != "jpg" && file_ext != "jpeg") {
			if (!hg::LoadPicture(pic, texture_path.c_str()))
				return -1;

			texture_name = hg::CutFileExtension(texture_name) + ".png";
			texture_path = hg::PathJoin({ config.source_output_path, texture_name });
			SavePNG(pic, texture_path.c_str());
		}

		if (!hg::LoadPicture(pic, texture_path.c_str()))
			return -1;

		image.name = hg::CutFileExtension(texture_name);

		image.mimeType = file_ext == "jpg" || file_ext == "jpeg" ? "image/jpeg" : "image/png";

		image.component = pic.GetFormat() == hg::PF_RGB24 ? 3 : 4;
		image.bits = 8;
		image.pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
		image.width = pic.GetWidth();
		image.height = pic.GetHeight();
	//	image.image = std::vector<unsigned char>(&pic.GetData()[0], &pic.GetData()[image.width * image.height * image.component]);

		// open the file:
		std::streampos fileSize;
		std::ifstream file(hg::PathJoin({config.source_output_path, texture_name}).c_str(), std::ios::binary);

		// get its size:
		file.seekg(0, std::ios::end);
		fileSize = file.tellg();
		file.seekg(0, std::ios::beg);

		// read the data:
		std::vector<unsigned char> fileData(fileSize);
		file.read((char *)&fileData[0], fileSize);

		// save data into buffer
		Buffer buffer;
		BufferView bufferView;
		serialize(fileData, buffer.data, 0);
		bufferView.byteLength = buffer.data.size();
		bufferView.buffer = model.buffers.size();
		model.buffers.push_back(buffer);
		image.bufferView = model.bufferViews.size();
		model.bufferViews.push_back(bufferView);

		texture.source = model.images.size();
		model.images.push_back(image);

		auto texture_id = model.textures.size();
		model.textures.push_back(texture);

		textures_cache.emplace(texture_path, texture_id);
	
		// remove the temporary file
		if (file_ext != "png" && file_ext != "jpg" && file_ext != "jpeg")
			remove(hg::PathJoin({config.source_output_path, texture_name}).c_str());

		return texture_id;
	} else {
		return textures_cache[texture_path];
	}
}

static void ExportMaterial(Model &model, Material &mat, hg::Material &m, const Config &config, hg::PipelineResources &resources) {
	auto t = hg::GetMaterialTexture(m, "uBaseOpacityMap");
	if (t != hg::InvalidTextureRef)
		mat.pbrMetallicRoughness.baseColorTexture.index = ExportTexture(model, t, config, resources);
	else {
		t = hg::GetMaterialTexture(m, "uDiffuseMap");
		if (t != hg::InvalidTextureRef)
			mat.pbrMetallicRoughness.baseColorTexture.index = ExportTexture(model, t, config, resources);
	}

	t = hg::GetMaterialTexture(m, "uOcclusionRoughnessMetalnessMap");
	if (t != hg::InvalidTextureRef)
		mat.pbrMetallicRoughness.metallicRoughnessTexture.index = ExportTexture(model, t, config, resources);

	t = hg::GetMaterialTexture(m, "uOcclusionMap");
	if (t != hg::InvalidTextureRef) {
		mat.occlusionTexture.index = ExportTexture(model, t, config, resources);
		mat.occlusionTexture.texCoord = 1;
	} else if (mat.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0)
		mat.occlusionTexture.index = mat.pbrMetallicRoughness.metallicRoughnessTexture.index;

	t = hg::GetMaterialTexture(m, "uNormalMap");
	if (t != hg::InvalidTextureRef)
		mat.normalTexture.index = ExportTexture(model, t, config, resources);

	t = hg::GetMaterialTexture(m, "uSelfMap");
	if (t != hg::InvalidTextureRef)
		mat.emissiveTexture.index = ExportTexture(model, t, config, resources);

	if (const auto &v = GetMaterialValue(m, "uBaseOpacityColor")) {
		mat.pbrMetallicRoughness.baseColorFactor[0] = v->value[0];
		mat.pbrMetallicRoughness.baseColorFactor[1] = v->value[1];
		mat.pbrMetallicRoughness.baseColorFactor[2] = v->value[2];
		mat.pbrMetallicRoughness.baseColorFactor[3] = v->value[3];
	}else if (const auto &v = GetMaterialValue(m, "uDiffuseColor")) {
		mat.pbrMetallicRoughness.baseColorFactor[0] = v->value[0];
		mat.pbrMetallicRoughness.baseColorFactor[1] = v->value[1];
		mat.pbrMetallicRoughness.baseColorFactor[2] = v->value[2];
		mat.pbrMetallicRoughness.baseColorFactor[3] = v->value[3];
	}

	if (const auto &v = GetMaterialValue(m, "uOcclusionRoughnessMetalnessColor")) {
		mat.pbrMetallicRoughness.roughnessFactor = v->value[1];
		mat.pbrMetallicRoughness.metallicFactor = v->value[2];
	}
	if (const auto &v = GetMaterialValue(m, "uSelfColor")) {
		mat.emissiveFactor.resize(3, 0);
		mat.emissiveFactor[0] = v->value[0];
		mat.emissiveFactor[1] = v->value[1];
		mat.emissiveFactor[2] = v->value[2];
	}

	if (GetMaterialBlendMode(m) == hg::BM_Alpha)
		mat.alphaMode = "BLEND";
	if (GetMaterialFaceCulling(m) == hg::FC_Disabled)
		mat.doubleSided = true;
}

//
static hg::Vertex PreparePolygonVertex(const hg::Geometry &geo, size_t i_bind, size_t i_vtp) {
	hg::Vertex vtx;

	vtx.pos = geo.vtx[geo.binding[i_bind + i_vtp]];

	if (!geo.normal.empty())
		vtx.normal = geo.normal[i_bind + i_vtp];

	if (!geo.tangent.empty()) {
		vtx.tangent = geo.tangent[i_bind + i_vtp].T;
		vtx.binormal = geo.tangent[i_bind + i_vtp].B;
	}

	if (!geo.color.empty())
		vtx.color0 = geo.color[i_bind + i_vtp];

	for (auto i = 0; i < geo.uv.size(); ++i)
		if (!geo.uv[i].empty())
			*(&vtx.uv0 + i) = geo.uv[i][i_bind + i_vtp];

	return vtx;
}

static void ExportGeometry(Model &model, Mesh &mesh, const int &nb_materials, const hg::Geometry &geo, const Config &config) {

	// prepare triangulation
	const uint8_t mat_count = GetMaterialCount(geo);

	std::map<int, std::vector<uint32_t>> geo_mat_indices;
	std::vector<hg::Vertex> geo_vtx;

	for (auto i_mat = 0; i_mat < mat_count; ++i_mat) {
		geo_mat_indices.emplace(i_mat, std::vector<uint32_t>());
		size_t i_bind = 0;

		for (auto &pol : geo.pol) {
			if (pol.material == i_mat) {
				for (auto i_vtp = 1; i_vtp < pol.vtx_count - 1; ++i_vtp) {
					geo_mat_indices[i_mat].push_back(geo_vtx.size());
					geo_vtx.push_back(PreparePolygonVertex(geo, i_bind, 0));
					geo_mat_indices[i_mat].push_back(geo_vtx.size());
					geo_vtx.push_back(PreparePolygonVertex(geo, i_bind, i_vtp + 1));
					geo_mat_indices[i_mat].push_back(geo_vtx.size());
					geo_vtx.push_back(PreparePolygonVertex(geo, i_bind, i_vtp));
				}
			}

			i_bind += pol.vtx_count;
		}
	}

	int accessor_id_position = -1, accessor_id_normal = -1, accessor_id_tangent = -1, accessor_id_texcoord0 = -1, accessor_id_texcoord1 = -1;
	// VERTEX BUFFER
	{
		Buffer buffer;
		BufferView bufferView;
		Accessor verticesAccessor;

		verticesAccessor.byteOffset = 0;
		verticesAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
		verticesAccessor.type = TINYGLTF_TYPE_VEC3;

		bufferView.byteOffset = 0;
		// bufferView.byteStride = 4; // not sur if needed yet

		std::vector<float> vertices;
		hg::Vec3 min(std::numeric_limits<float>::max()), max(std::numeric_limits<float>::lowest());

		for (const auto &vertex : geo_vtx) {
			const auto &vtx = vertex.pos;
			vertices.push_back(vtx.x);
			vertices.push_back(vtx.y);
			vertices.push_back(-vtx.z);

			min.x = vtx.x < min.x ? vtx.x : min.x;
			min.y = vtx.y < min.y ? vtx.y : min.y;
			min.z = -vtx.z < min.z ? -vtx.z : min.z;
			max.x = vtx.x > max.x ? vtx.x : max.x;
			max.y = vtx.y > max.y ? vtx.y : max.y;
			max.z = -vtx.z > max.z ? -vtx.z : max.z;
		}

		serialize(vertices, buffer.data, 0);

		verticesAccessor.minValues.push_back(min.x);
		verticesAccessor.minValues.push_back(min.y);
		verticesAccessor.minValues.push_back(min.z);
		verticesAccessor.maxValues.push_back(max.x);
		verticesAccessor.maxValues.push_back(max.y);
		verticesAccessor.maxValues.push_back(max.z);
		verticesAccessor.count = vertices.size() / 3;
		bufferView.byteLength = buffer.data.size();

		// save vertices buffer
		bufferView.buffer = model.buffers.size();
		model.buffers.push_back(buffer);
		verticesAccessor.bufferView = model.bufferViews.size();
		model.bufferViews.push_back(bufferView);

		accessor_id_position = model.accessors.size();
		model.accessors.push_back(verticesAccessor);
	}
	// NORMAL BUFFER
	{
		Buffer buffer;
		BufferView bufferView;
		Accessor normalAccessor;

		normalAccessor.byteOffset = 0;
		normalAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
		normalAccessor.type = TINYGLTF_TYPE_VEC3;

		bufferView.byteOffset = 0;
		// bufferView.byteStride = 4; // not sur if needed yet

		std::vector<float> normal;
		hg::Vec3 min(std::numeric_limits<float>::max()), max(std::numeric_limits<float>::lowest());
		for (const auto &vertex : geo_vtx) {
			const auto &nrm = vertex.normal;
			normal.push_back(nrm.x);
			normal.push_back(nrm.y);
			normal.push_back(-nrm.z);

			min.x = nrm.x < min.x ? nrm.x : min.x;
			min.y = nrm.y < min.y ? nrm.y : min.y;
			min.z = -nrm.z < min.z ? -nrm.z : min.z;
			max.x = nrm.x > max.x ? nrm.x : max.x;
			max.y = nrm.y > max.y ? nrm.y : max.y;
			max.z = -nrm.z > max.z ? -nrm.z : max.z;
		}

		serialize(normal, buffer.data, 0);

		normalAccessor.minValues.push_back(min.x);
		normalAccessor.minValues.push_back(min.y);
		normalAccessor.minValues.push_back(min.z);
		normalAccessor.maxValues.push_back(max.x);
		normalAccessor.maxValues.push_back(max.y);
		normalAccessor.maxValues.push_back(max.z);
		normalAccessor.count = normal.size() / 3;
		bufferView.byteLength = buffer.data.size();

		// save normal buffer
		bufferView.buffer = model.buffers.size();
		model.buffers.push_back(buffer);
		normalAccessor.bufferView = model.bufferViews.size();
		model.bufferViews.push_back(bufferView);

		accessor_id_normal = model.accessors.size();
		model.accessors.push_back(normalAccessor);
	}
	/*	// TANGENT BUFFER
	if(!geo.tangent.empty())
	{
	Buffer buffer;
	BufferView bufferView;
	Accessor tangentAccessor;

	tangentAccessor.byteOffset = 0;
	tangentAccessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
	tangentAccessor.type = TINYGLTF_TYPE_VEC4;

	bufferView.byteOffset = 0;
	// bufferView.byteStride = 4; // not sur if needed yet

	std::vector<float> tangent;
	hg::Vec3 min(std::numeric_limits<float>::max()), max(std::numeric_limits<float>::lowest()/*);
	for (const auto &tgt : geo.tangent) {
	tangent.push_back(tgt.T.x);
	tangent.push_back(tgt.T.y);
	tangent.push_back(tgt.T.z);
	tangent.push_back(1.0);

	min.x = tgt.T.x < min.x ? tgt.T.x : min.x;
	min.y = tgt.T.y < min.y ? tgt.T.y : min.y;
	min.z = tgt.T.z < min.z ? tgt.T.z : min.z;
	max.x = tgt.T.x > max.x ? tgt.T.x : max.x;
	max.y = tgt.T.y > max.y ? tgt.T.y : max.y;
	max.z = tgt.T.z > max.z ? tgt.T.z : max.z;
	}

	serialize(tangent, buffer.data, 0);

	tangentAccessor.minValues.push_back(min.x);
	tangentAccessor.minValues.push_back(min.y);
	tangentAccessor.minValues.push_back(min.z);
	tangentAccessor.minValues.push_back(1);
	tangentAccessor.maxValues.push_back(max.x);
	tangentAccessor.maxValues.push_back(max.y);
	tangentAccessor.maxValues.push_back(max.z);
	tangentAccessor.maxValues.push_back(1);

	tangentAccessor.count = tangent.size() / 4;
	bufferView.byteLength = buffer.data.size();

	// save tangent buffer
	bufferView.buffer = model.buffers.size();
	model.buffers.push_back(buffer);
	tangentAccessor.bufferView = model.bufferViews.size();
	model.bufferViews.push_back(bufferView);

	accessor_id_tangent = model.accessors.size();
	model.accessors.push_back(tangentAccessor);
	}*/

	// UV0 BUFFER
	if (!geo.uv[0].empty()) {
		Buffer buffer;
		BufferView bufferView;
		Accessor uv0Accessor;

		uv0Accessor.byteOffset = 0;
		uv0Accessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
		uv0Accessor.type = TINYGLTF_TYPE_VEC2;

		bufferView.byteOffset = 0;
		// bufferView.byteStride = 4; // not sur if needed yet

		std::vector<float> uv0;
		hg::Vec3 min(std::numeric_limits<float>::max()), max(std::numeric_limits<float>::lowest());
		for (const auto &vertex : geo_vtx) {
			const auto &uv = vertex.uv0;
			uv0.push_back(uv.x);
			uv0.push_back(uv.y);

			min.x = uv.x < min.x ? uv.x : min.x;
			min.y = uv.y < min.y ? uv.y : min.y;
			max.x = uv.x > max.x ? uv.x : max.x;
			max.y = uv.y > max.y ? uv.y : max.y;
		}
		serialize(uv0, buffer.data, 0);

		uv0Accessor.minValues.push_back(min.x);
		uv0Accessor.minValues.push_back(min.y);
		uv0Accessor.maxValues.push_back(max.x);
		uv0Accessor.maxValues.push_back(max.y);
		uv0Accessor.count = uv0.size() / 2;
		bufferView.byteLength = buffer.data.size();

		// save uv0 buffer
		bufferView.buffer = model.buffers.size();
		model.buffers.push_back(buffer);
		uv0Accessor.bufferView = model.bufferViews.size();
		model.bufferViews.push_back(bufferView);

		accessor_id_texcoord0 = model.accessors.size();
		model.accessors.push_back(uv0Accessor);
	}
	// UV1 BUFFER
	if (!geo.uv[1].empty()) {
		Buffer buffer;
		BufferView bufferView;
		Accessor uv1Accessor;

		uv1Accessor.byteOffset = 0;
		uv1Accessor.componentType = TINYGLTF_COMPONENT_TYPE_FLOAT;
		uv1Accessor.type = TINYGLTF_TYPE_VEC2;

		bufferView.byteOffset = 0;
		// bufferView.byteStride = 4; // not sur if needed yet

		std::vector<float> uv1;
		hg::Vec3 min(std::numeric_limits<float>::max()), max(std::numeric_limits<float>::lowest());
		for (const auto &vertex : geo_vtx) {
			const auto &uv = vertex.uv1;
			uv1.push_back(uv.x);
			uv1.push_back(uv.y);

			min.x = uv.x < min.x ? uv.x : min.x;
			min.y = uv.y < min.y ? uv.y : min.y;
			max.x = uv.x > max.x ? uv.x : max.x;
			max.y = uv.y > max.y ? uv.y : max.y;
		}
		serialize(uv1, buffer.data, 0);

		uv1Accessor.minValues.push_back(min.x);
		uv1Accessor.minValues.push_back(min.y);
		uv1Accessor.maxValues.push_back(max.x);
		uv1Accessor.maxValues.push_back(max.y);
		uv1Accessor.count = uv1.size() / 2;
		bufferView.byteLength = buffer.data.size();

		// save uv1 buffer
		bufferView.buffer = model.buffers.size();
		model.buffers.push_back(buffer);
		uv1Accessor.bufferView = model.bufferViews.size();
		model.bufferViews.push_back(bufferView);

		accessor_id_texcoord1 = model.accessors.size();
		model.accessors.push_back(uv1Accessor);
	}

	for (int i_mat = 0; i_mat < nb_materials; ++i_mat) {
		Primitive primitive;
		primitive.mode = TINYGLTF_MODE_TRIANGLES;

		// one primitive per material using the same buffer (not really optimised for now!!!)

		if (accessor_id_position != -1)
			primitive.attributes["POSITION"] = accessor_id_position;
		if (accessor_id_normal != -1)
			primitive.attributes["NORMAL"] = accessor_id_normal;
		if (accessor_id_tangent != -1)
			primitive.attributes["TANGENT"] = accessor_id_tangent;
		if (accessor_id_texcoord0 != -1)
			primitive.attributes["TEXCOORD_0"] = accessor_id_texcoord0;
		if (accessor_id_texcoord1 != -1)
			primitive.attributes["TEXCOORD_1"] = accessor_id_texcoord1;

		// INDICES BUFFER
		{
			Buffer buffer;
			BufferView bufferView;
			Accessor indicesAccessor;

			indicesAccessor.byteOffset = 0;
			indicesAccessor.componentType = TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT;
			indicesAccessor.type = TINYGLTF_TYPE_SCALAR;

			bufferView.byteOffset = 0;
			// bufferView.byteStride = 4; // not sur if needed yet

			const auto pol_idx = ComputePolygonIndex(geo);
			std::vector<uint32_t> indices;
			uint32_t min = UINT_MAX;
			uint32_t max = 0;

			for (const auto &i : geo_mat_indices[i_mat]) {
				indices.push_back(i);
				min = i < min ? i : min;
				max = i > max ? i : max;
			}

			uint32_t indexSize = serialize(indices, buffer.data, 0);

			indicesAccessor.minValues.push_back(min);
			indicesAccessor.maxValues.push_back(max);
			indicesAccessor.count = indices.size();
			bufferView.byteLength = buffer.data.size();

			// save indices buffer
			bufferView.buffer = model.buffers.size();
			model.buffers.push_back(buffer);
			indicesAccessor.bufferView = model.bufferViews.size();
			model.bufferViews.push_back(bufferView);
			primitive.indices = model.accessors.size();
			model.accessors.push_back(indicesAccessor);
		}
		mesh.primitives.push_back(primitive);
	}
}

std::map<std::string, std::vector<Primitive>> geo_to_primitives;
static const size_t EmptyMeshID = static_cast<size_t>(-1);

static const size_t ExportObject(Model &model, const hg::Object &object, const Config &config, hg::PipelineResources &resources) {
	Mesh mesh;

	// export geo
	auto mdl_ref = object.GetModelRef();
	if (resources.models.IsValidRef(mdl_ref)) {
		auto mdl_name = resources.models.GetName_unsafe_(mdl_ref.ref.idx);

		mesh.name = mdl_name;
		std::string geo_name = hg::PathJoin({config.source_output_path, mdl_name});

		if (geo_to_primitives.find(geo_name) == geo_to_primitives.end()) {
			// this geo was not exported
			const auto geo = hg::LoadGeometryFromFile(geo_name.c_str());

			// add primitive to mesh or multiple primitive (one per material)
			if (geo.vtx.size()) {
				ExportGeometry(model, mesh, object.GetMaterialCount(), geo, config);
				geo_to_primitives[geo_name] = mesh.primitives;
			} else
				return EmptyMeshID; // no geo, no mesh
		} else {
			mesh.primitives = geo_to_primitives[geo_name];
		}
	} else
		return EmptyMeshID; // no geo, no mesh

	// export materials
	for (int i = 0; i < object.GetMaterialCount(); ++i) {
		Material mat;
		auto m = object.GetMaterial(i);
		mat.name = object.GetMaterialName(i);

		ExportMaterial(model, mat, m, config, resources);

		mesh.primitives[i].material = model.materials.size();
		model.materials.push_back(mat);
	}

	size_t id_mesh = model.meshes.size();
	model.meshes.push_back(mesh);
	return id_mesh;
}

//
static const int ExportNode(Model &model, const hg::NodeRef nodeRef, const std::vector<hg::NodeRef> &children, const hg::NodesChildren &nodes_children,
	const hg::Scene &scene, const Config &config, hg::PipelineResources &resources) {
	// [EJ] this code is confusing/confused...
	// is nodeRef.gen known to be outdated? if so, why pass a ref and not an index?
	// why is the returned Node object stored as a reference to a temporary?
	const auto &node = scene.GetNode(scene.GetNodeRef(nodeRef.idx));
	Node n;
	n.name = node.GetName();

	// if disable
	if (!node.IsEnabled()) {
		if (config.filter_disabled_node)
			return -1;

		if (std::find(model.extensionsUsed.begin(), model.extensionsUsed.end(), "KHR_nodes_disable") == model.extensionsUsed.end())
			model.extensionsUsed.push_back("KHR_nodes_disable");
		n.extensions["KHR_nodes_disable"] = Value({ { "visible", Value(false) } });
	}

	if (node.GetTransform()) {
		auto p = node.GetTransform().GetPos();
		n.translation.push_back(p.x);
		n.translation.push_back(p.y);
		n.translation.push_back(-p.z);

		auto r = node.GetTransform().GetRot();
		r.x = -r.x;
		r.y = -r.y;
		auto q = hg::QuaternionFromEuler(r);
		n.rotation.push_back(q.x);
		n.rotation.push_back(q.y);
		n.rotation.push_back(q.z);
		n.rotation.push_back(q.w);

		auto s = node.GetTransform().GetScale();
		n.scale.push_back(s.x);
		n.scale.push_back(s.y);
		n.scale.push_back(s.z);
	}

	/*
	// is it a camera
	if (gltf_node.camera >= 0)
	ExportCamera(model, gltf_node, node, scene, config, resources);
	*/
	//
	if (node.HasObject()) {
		auto id_mesh = ExportObject(model, node.GetObject(), config, resources);
		if (id_mesh != EmptyMeshID)
			n.mesh = id_mesh;
	}

	// save children
	for (const auto &child : children) {
		auto id_node = ExportNode(model, child, nodes_children.GetChildren(child), nodes_children, scene, config, resources);
		if (id_node >= 0)
			n.children.push_back(id_node);
	}

	auto id_node = model.nodes.size();
	NodeRef_to_IdNode[node.ref] = id_node;
	model.nodes.push_back(n);
	return id_node;
}

static bool ExportGltfScene(const std::string &path, const Config &config) {
	const auto t_start = hg::time_now();

	// init bgfx
	hg::WindowSystemInit();
	//
	auto win = hg::RenderInit(1, 1, BGFX_RESET_MSAA_X4, bgfx::TextureFormat::Count); // , BGFX_DEBUG_STATS);
	hg::HideWindow(win);

	// create output directory if missing
	if (hg::Exists(config.base_output_path.c_str())) {
		if (!hg::IsDir(config.base_output_path.c_str()))
			return false; // can't output to this path
	} else {
		if (!hg::MkDir(config.base_output_path.c_str()))
			return false;
	}

	// add assets path
	hg::AddAssetsFolder(config.compiled_output_path.c_str());

	// load scene
	hg::PipelineResources pl_resources;
	hg::Scene scene;
	hg::LoadSceneContext ctx;
	if (!hg::LoadSceneFromAssets(hg::PathStripPrefix(path, config.compiled_output_path).c_str(), scene, pl_resources, hg::GetForwardPipelineInfo(), ctx, hg::LSSF_All))
		return false;

	// get root nodes to export (then proceed with children)
	std::vector<hg::NodeRef> root_nodes;

	const auto nodes = scene.GetNodes();
	for (auto &node : nodes)
		if (!scene.IsValidNodeRef(node.GetTransform().GetParent())) // skip nodes with parent
			root_nodes.push_back(node.ref);

	const auto nodes_children = scene.BuildNodesChildren();

	//
	Model model;
	TinyGLTF saver;
	std::string err;
	std::string warn;

	model.asset.generator = "Harfang Exporter";
	model.asset.version = "2.0";

	Scene scn;
	// export root node (following by the children)
	for (const auto &node : root_nodes) {
		auto id_node = ExportNode(model, node, nodes_children.GetChildren(node), nodes_children, scene, config, pl_resources);
		if (id_node >= 0)
			scn.nodes.push_back(id_node);
	}

	ExportMotions(model, scene);

	model.scenes.push_back(scn);

	std::string export_path = config.name.empty() ? hg::GetFileName(path) : config.name;
	// if no extension, set to binary
	if (hg::GetFileExtension(export_path) == "")
		export_path += ".glb";

	std::string out_path;
	bool embedImages = true;
	bool embedBuffers = true;
	bool prettyPrint = true;
	bool writeBinary = config.binary || (hg::GetFileExtension(export_path) == "glb");
	if (!GetOutputPath(out_path, config.base_output_path,  hg::GetFileName(export_path), {}, hg::GetFileExtension(export_path))) {
		hg::error("failed to compute output path");
		return false;
	}

		bool ret = saver.WriteGltfSceneToFile(&model, out_path, embedImages, embedBuffers, prettyPrint, writeBinary);
	if (!ret) {
		hg::error(hg::format("failed to write scene to %1").arg(out_path.c_str()));
		return false;
	}

	hg::log(hg::format("Export complete, took %1 ms").arg(hg::time_to_ms(hg::time_now() - t_start)));
	return true;
}

static void OutputUsage(const hg::CmdLineFormat &cmd_format) {
	hg::debug((std::string("Usage: gltf_exporter ") + hg::word_wrap(hg::FormatCmdLineArgs(cmd_format), 80, 21) + "\n").c_str());
	hg::debug((hg::FormatCmdLineArgsDescription(cmd_format)).c_str());
	hg::debug((std::string("Header-only tiny glTF 2.0 loader and serializer.The MIT License (MIT)") +
			   "Copyright (c) 2015 - 2019 Syoyo Fujita, Aur�lien Chatelain and manycontributors." +
			   "Permission is hereby granted, free of charge, to any person obtaining a copy" +
			   "of this software and associated documentation files (the \" Software \"), to deal" +
			   "in the Software without restriction, including without limitation the rights" +
			   "to use, copy, modify, merge, publish, distribute, sublicense, and/or sell" +
			   "copies of the Software, and to permit persons to whom the Software is" +
			   "furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in" +
			   "all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED \" AS IS \", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR" +
			   "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY," +
			   "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE" +
			   "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER" +
			   "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM," +
			   "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.")
				  .c_str());
}

//
static std::mutex log_mutex;
static bool quiet = false;

int main(int argc, const char **argv) {
	hg::set_log_hook(
		[](const char *msg, int mask, const char *details, void *user) {
			if (quiet && !(mask & hg::LL_Error))
				return; // skip masked entries

			std::lock_guard<std::mutex> guard(log_mutex);
			std::cout << msg << std::endl;
		},
		nullptr);
	hg::set_log_level(hg::LL_All);

	hg::debug(hg::format("Harfang->GLTF Converter %1 (%2)").arg(hg::get_version_string()).arg(hg::get_build_sha()).c_str());

	hg::CmdLineFormat cmd_format = {
		{
			{"-quiet", "Quiet log, only log errors"},
			{"-binary", "Export binary file"},
			{"-filter_disabled_node", "Don't export disabled node"},
		},
		{{"-out", "Output directory", true}, {"-compiled-resource-path", "Where the scene compiled resources are", true},
			{"-source-resource-path", "Where the scene not compiled resources are", true}, {"-name", "Specify the output scene name", true}},
		{
			{"input", "Input harfang scene file to convert"},
		},
		{{"-o", "-out"}, {"-h", "-help"}, {"-q", "-quiet"}},
	};

	hg::CmdLineContent cmd_content;
	if (!hg::ParseCmdLine({argv + 1, argv + argc}, cmd_format, cmd_content)) {
		OutputUsage(cmd_format);
		return -1;
	}

	//
	Config config;
	config.base_output_path = hg::CleanPath(hg::GetCmdLineSingleValue(cmd_content, "-out", "./"));
	config.compiled_output_path = hg::CleanPath(hg::GetCmdLineSingleValue(cmd_content, "-compiled-resource-path", ""));
	config.source_output_path = hg::CleanPath(hg::GetCmdLineSingleValue(cmd_content, "-source-resource-path", ""));
	config.name = hg::CleanPath(hg::GetCmdLineSingleValue(cmd_content, "-name", ""));

	quiet = hg::GetCmdLineFlagValue(cmd_content, "-quiet");
	config.binary = hg::GetCmdLineFlagValue(cmd_content, "-binary");
	config.filter_disabled_node = hg::GetCmdLineFlagValue(cmd_content, "-filter_disabled_node");

	//
	if (cmd_content.positionals.size() != 1) {
		hg::debug("No input file");
		OutputUsage(cmd_format);
		return -2;
	}

	//
	config.input_path = cmd_content.positionals[0];
	auto res = ExportGltfScene(cmd_content.positionals[0], config);

	const auto msg = std::string("[ExportScene") + std::string(res ? ": OK]" : ": KO]");
	hg::log(msg.c_str());

	return res ? 0 : 1;
}
