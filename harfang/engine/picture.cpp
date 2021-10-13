// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "engine/picture.h"
#include "foundation/cext.h"
#include "foundation/file.h"
#include "foundation/math.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "engine/stb_image.h"
#include "engine/stb_image_write.h"

namespace hg {

int size_of(PictureFormat format) {
	static int size_of_format[PF_Last] = {0, 3, 4, 16};
	return size_of_format[format];
}

size_t GetChannelCount(PictureFormat format) {
	static size_t channel_count_format[PF_Last] = {0, 3, 4, 4};
	return channel_count_format[format];
}

//
Picture::Picture(uint16_t width, uint16_t height, PictureFormat format)
	: w(width), h(height), f(format), has_ownership(1), d(new uint8_t[w * h * size_of(f)]) {}

Picture::Picture(void *data, uint16_t width, uint16_t height, PictureFormat format) : w(width), h(height), f(format), d(reinterpret_cast<uint8_t *>(data)) {}

Picture::Picture(const Picture &pic)
	: w(pic.w), h(pic.h), f(pic.f), has_ownership(pic.has_ownership), d(pic.has_ownership ? new uint8_t[w * h * size_of(f)] : pic.d) {}

Picture::Picture(Picture &&pic) noexcept : w(pic.w), h(pic.h), f(pic.f), has_ownership(pic.has_ownership), d(pic.d) {
	pic.w = 0;
	pic.h = 0;
	pic.f = PF_RGBA32;
	pic.has_ownership = 0;
	pic.d = nullptr;
}

Picture::~Picture() { Clear(); }

//
void Picture::SetData(void *data, uint16_t width, uint16_t height, PictureFormat format) {
	Clear();

	w = width;
	h = height;
	f = format;

	has_ownership = 0;
	d = reinterpret_cast<uint8_t *>(data);
}

void Picture::CopyData(const void *data, uint16_t width, uint16_t height, PictureFormat format) {
	Clear();

	w = width;
	h = height;
	f = format;

	has_ownership = 1;
	d = new uint8_t[w * h * size_of(f)];
	std::copy(reinterpret_cast<const uint8_t *>(data), reinterpret_cast<const uint8_t *>(data) + w * h * size_of(f), d);
}

void Picture::TakeDataOwnership() {
	if (has_ownership || d == nullptr)
		return;

	auto *d_ = new uint8_t[w * h * size_of(f)];
	std::copy(d, d + w * h * size_of(f), d_);

	has_ownership = 1;
	d = d_;
}

//
Picture &Picture::operator=(const Picture &pic) {
	if (pic.has_ownership)
		CopyData(pic.d, pic.w, pic.h, pic.f);
	else
		SetData(pic.d, pic.w, pic.h, pic.f);
	return *this;
}

Picture &Picture::operator=(Picture &&pic) noexcept {
	Clear();

	w = pic.w;
	h = pic.h;
	f = pic.f;

	d = pic.d;
	has_ownership = pic.has_ownership;

	pic.w = 0;
	pic.h = 0;
	pic.f = PF_RGBA32;

	pic.has_ownership = 0;
	pic.d = nullptr;

	return *this;
}

//
Picture MakePictureView(void *data, uint16_t width, uint16_t height, PictureFormat format) {
	Picture pic;
	pic.SetData(data, width, height, format);
	return pic;
}

Picture MakePicture(const void *data, uint16_t width, uint16_t height, PictureFormat format) {
	Picture pic;
	pic.CopyData(data, width, height, format);
	return pic;
}

//
void Picture::Clear() {
	w = h = 0;
	f = PF_RGBA32;

	if (has_ownership)
		delete[](reinterpret_cast<uint8_t *>(d));
	has_ownership = 0;
	d = nullptr;
}

// TODO EJ implement these
Picture Crop(const Picture &picture, uint16_t width, uint16_t height) { return picture; }
Picture Resize(const Picture &picture, uint16_t width, uint16_t height) { return picture; }

//
Color GetPixelRGBA(const Picture &pic, uint16_t x, uint16_t y) {
	if (x >= pic.GetWidth() || y >= pic.GetHeight())
		return Color::Zero;

	const int size = size_of(pic.GetFormat());
	int offset = numeric_cast<int>((x + (size_t)pic.GetWidth() * y) * size);
	Color out(Color::Zero);
	const uint8_t *data = pic.GetData();

	for (int i = 0; i < size;)
		out[i++] = data[offset++] / 255.f;

	return out;
}

void SetPixelRGBA(Picture &pic, uint16_t x, uint16_t y, const Color &col) {
	if ((x >= pic.GetWidth()) || (y >= pic.GetHeight()))
		return;

	const int size = size_of(pic.GetFormat());
	int offset = numeric_cast<int>((x + (size_t)pic.GetWidth() * y) * size);
	uint8_t *data = pic.GetData();

	for (int i = 0; i < size;)
		data[offset++] = uint8_t(Clamp(col[i++], 0.f, 1.f) * 255.f);
}

//
struct STB_callbacks {
	ScopedFile file;
	stbi_io_callbacks clbk;
};

STB_callbacks open_STB_file(const char *path) {
	return {Open(path), {
							[](void *user, char *data, int size) -> int { return int(Read(*reinterpret_cast<ScopedFile *>(user), data, size)); },
							[](void *user, int n) -> void { Seek(*reinterpret_cast<ScopedFile *>(user), n, SM_Current); },
							[](void *user) -> int { return IsEOF(*reinterpret_cast<ScopedFile *>(user)) ? 0 : 1; },
						}};
}

static bool load_STB_picture(Picture &pic, const char *path) {
	auto cb = open_STB_file(path);
	if (!cb.file)
		return false;

	int x, y, n;
	const auto data = stbi_load_from_callbacks(&cb.clbk, &cb.file, &x, &y, &n, 4);
	if (!data)
		return false;

	pic.CopyData(data, x, y, PF_RGBA32);

	stbi_image_free(data);
	return true;
}

bool LoadJPG(Picture &pic, const char *path) { return load_STB_picture(pic, path); }
bool LoadPNG(Picture &pic, const char *path) { return load_STB_picture(pic, path); }
bool LoadGIF(Picture &pic, const char *path) { return load_STB_picture(pic, path); }
bool LoadPSD(Picture &pic, const char *path) { return load_STB_picture(pic, path); }
bool LoadTGA(Picture &pic, const char *path) { return load_STB_picture(pic, path); }
bool LoadBMP(Picture &pic, const char *path) { return load_STB_picture(pic, path); }

bool LoadPicture(Picture &pic, const char *path) { return load_STB_picture(pic, path); }

//
static void STB_write(void *user, void *data, int size) {
	ScopedFile *file = reinterpret_cast<ScopedFile*>(user);
	if(file) {
		Write(file->f, data, size);
	}
}

bool SavePNG(const Picture &pic, const char *path) {
	if (!pic.GetHeight() || !pic.GetWidth())
		return false;

	ScopedFile file(OpenWrite(path));
	if (!file)
		return false;

	return stbi_write_png_to_func(
				STB_write, &file, pic.GetWidth(), pic.GetHeight(), size_of(pic.GetFormat()), pic.GetData(), pic.GetWidth() * size_of(pic.GetFormat())) != 0;
}

bool SaveBMP(const Picture &pic, const char *path) {
	if (!pic.GetHeight() || !pic.GetWidth())
		return false;

	ScopedFile file(OpenWrite(path));
	if (!file)
		return false;

	return stbi_write_bmp_to_func(STB_write, &file, pic.GetWidth(), pic.GetHeight(), size_of(pic.GetFormat()), pic.GetData()) != 0;
}

bool SaveTGA(const Picture &pic, const char *path) {
	if (!pic.GetHeight() || !pic.GetWidth())
		return false;

	ScopedFile file(OpenWrite(path));
	if (!file)
		return false;

	return stbi_write_tga_to_func(STB_write, &file, pic.GetWidth(), pic.GetHeight(), size_of(pic.GetFormat()), pic.GetData()) != 0;
}

} // namespace hg
