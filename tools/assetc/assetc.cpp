// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include <algorithm>
#include <deque>
#include <future>
#include <iostream>
#include <map>
#include <mutex>
#include <regex>
#include <set>
#include <string>
#include <thread>

#include <json/json.hpp>
#include <process.hpp>

#include <foundation/build_info.h>
#include <foundation/cmd_line.h>
#include <foundation/data.h>
#include <foundation/dir.h>
#include <foundation/file.h>
#include <foundation/format.h>
#include <foundation/log.h>
#include <foundation/path_tools.h>
#include <foundation/profiler.h>
#include <foundation/string.h>
#include <foundation/time.h>
#include <foundation/time_chrono.h>
#include <foundation/xxhash.h>

#include <engine/forward_pipeline.h>
#include <engine/geometry.h>
#include <engine/meta.h>
#include <engine/render_pipeline.h>
#include <engine/scene.h>

#include <platform/filesystem_watcher.h>
#include <platform/process.h>

#define HASH_METHOD 2 // 0:SHA1 1:Murmurv3 2:xxHash

#if USE_SHA1
#include <foundation/sha1.h>
#else
#include <foundation/murmur3.h>
#endif

#if WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>
#undef CopyFile
#endif

using namespace hg;

namespace assetc {

static const int classify_progress_weight = 30;
static const int compile_progress_weight = 20;
static const int run_queue_progress_weight = 50;

//
static std::string cwd;

//
static std::atomic<int> error_count;
static bool log_errors_to_stderr = false;

static bool progress = false;

static std::mutex failed_inputs_mutex;
static std::set<std::string> failed_inputs;

static void ReportFailedInput(std::string name) {
	std::lock_guard<std::mutex> lock(failed_inputs_mutex);
	failed_inputs.insert(std::move(name));
}

void log_error(const json &js) {
	std::stringstream ss;

	if (log_errors_to_stderr) {
		ss << js.dump(4) << std::endl;
		std::cerr << ss.str();
	} else {
		const auto type = js["type"].get<std::string>();

		ss << "Error " << type << std::endl;

		if (type == "CompileProcessReturnedNonZero") {
			ss << " -> CWD: " << js["cwd"].get<std::string>() << std::endl;
			ss << " -> CMD: " << js["cmd"].get<std::string>() << std::endl;

			const auto out = js["out"].get<std::string>();
			if (!out.empty())
				ss << " -> STDOUT:" << std::endl << out << std::endl;

			const auto err = js["err"].get<std::string>();
			if (!err.empty())
				ss << " -> STDERR:" << std::endl << err << std::endl;
		}

		std::cout << ss.str();
	}

	++error_count;
}

//
static bool compile_with_debug_info = false;

//
static std::string global_shader_defines;

//
struct Toolchain {
	std::string shaderc, texturec, luac, cmft, recastc, texconv;
};

static Toolchain toolchain;

static void ToolchainExists() {
	if (!IsFile(toolchain.shaderc.c_str()))
		toolchain.shaderc.clear();
	if (!IsFile(toolchain.texturec.c_str()))
		toolchain.texturec.clear();
	if (!IsFile(toolchain.luac.c_str()))
		toolchain.luac.clear();
	if (!IsFile(toolchain.cmft.c_str()))
		toolchain.cmft.clear();
	if (!IsFile(toolchain.recastc.c_str()))
		toolchain.recastc.clear();
	if (!IsFile(toolchain.texconv.c_str()))
		toolchain.texconv.clear();
}

static void SetToolchain(const std::string &path) {
#ifdef WIN32
	toolchain = {PathJoin({path, "shaderc.exe"}), PathJoin({path, "texturec.exe"}), PathJoin({path, "luac.exe"}), PathJoin({path, "cmft.exe"}),
		PathJoin({path, "recastc.exe"}), PathJoin({path, "texconv.exe"})};
#else
	toolchain = {PathJoin({path, "shaderc"}), PathJoin({path, "texturec"}), PathJoin({path, "luac"}), PathJoin({path, "cmft"}), PathJoin({path, "texconv"})};
#endif
}

//
using process_id = unsigned long;
static process_id poll_process_id = 0;

static bool IsPollProcessUp() {
	if (poll_process_id == 0)
		return true;

	bool running = true;
	is_process_running(poll_process_id, running);

	return running;
}

//
static std::string api = HG_GRAPHIC_API; // DX11/DX12/GL

#if defined(WIN32)
#define ASSSETC_DEFAULT_PLATFORM "windows"
#elif defined(__linux__)
#define ASSSETC_DEFAULT_PLATFORM "linux"
#elif defined(__APPLE__)
#define ASSSETC_DEFAULT_PLATFORM "osx"
#else
#define ASSSETC_DEFAULT_PLATFORM ""
#endif
// remaining shaderc platforms
//		android
//		asm.js
//		ios
//		orbis

static std::string platform = ASSSETC_DEFAULT_PLATFORM;

#if 0
// CWE 561: The function 'SetAPI' is never used.
void SetAPI(const char *api_) { api = api_; }
#endif
//
static std::string input_dir;
static std::string output_dir;

static std::string FullInputPath(const std::string &path) { return PathJoin({input_dir, path}); }
static std::string FullOutputPath(const std::string &path) { return PathJoin({output_dir, path}); }

static std::string FullOutputDir(const std::string &path) { return CutFileName(FullOutputPath(path)); }
static bool MkOutputTree(const std::string &path) {
	const auto res = MkTree(FullOutputDir(path).c_str());
	if (!res) {
		const json json_err = {{"type", "FailedToMkOutputTree"}, {"path", path}};
		log_error(json_err);
	}
	return res;
}

#if HASH_METHOD == 0
using Hash = SHA1Hash;
#else
using Hash = std::array<char, 16>; // 128 bit
#endif

void ComputeHash(const void *data, size_t len, Hash &out) {
	ProfilerPerfSection perf("Manage/ComputeHash");

#if HASH_METHOD == 0
	ComputeSHA1(data, len, out);
#elif HASH_METHOD == 1
#if defined(_M_X64) || defined(__amd64__)
	MurmurHash3_x64_128(data, len, 0, &out);
#else
	MurmurHash3_x86_128(data, len, 0, &out);
#endif
#else
	XXH128(data, len, 0, &out);
#endif
}

//
struct CompilationDB {
	std::map<std::string, std::set<std::string>> output_to_inputs;
	std::map<std::string, Hash> source_hashes;
	std::map<std::string, Hash> output_build_params;
};

static CompilationDB compilation_db;

static size_t processed_count = 0;
static bool fast_check = false;

static bool TestInputs(std::map<std::string, Hash> &hashes, const std::set<std::string> &inputs) {
	bool need_refresh = false;

	for (const auto &i : inputs) {
		const auto input_path = FullInputPath(i);

		if (!IsFile(input_path.c_str()))
			continue;

		Data data;
		if (fast_check) {
			const auto info = GetFileInfo(input_path.c_str());
			if (info.size == 0)
				error(format("Failed to stat input '%1'").arg(input_path));

			Write(data, info.size);
			Write(data, info.modified);
		} else {
			if (!LoadDataFromFile(input_path.c_str(), data))
				error(format("Failed to load '%1' data").arg(input_path));
		}

		Hash hash;
		ComputeHash(data.GetData(), data.GetSize(), hash);

		const auto &j = compilation_db.source_hashes.find(i);

		if (j == std::end(compilation_db.source_hashes)) {
			debug(format("    [!] Source file '%1' hash missing, triggering refresh").arg(i));
			need_refresh = true;
		} else {
			if (j->second != hash) {
				debug(format("    [!] Source file '%1' hash mismatch, triggering refresh").arg(i));
				need_refresh = true; // modified source
			}
		}

		hashes[i] = hash; // update source file hash
	}

	return need_refresh;
}

static bool TestOutputs(const std::set<std::string> &outputs, const Data &build_context) {
	bool need_refresh = false;

	Hash build_params_hash{};
	ComputeHash(build_context.GetData(), build_context.GetSize(), build_params_hash);

	for (const auto &output : outputs) {
		if (!IsFile(FullOutputPath(output).c_str())) {
			debug(format("    [!] Output file '%1' missing, triggering refresh").arg(output));
			need_refresh = true; // missing output asset
		}

		auto i = compilation_db.output_build_params.find(output);
		if (i == std::end(compilation_db.output_build_params)) {
			debug(format("    [!] Output file '%1' build context missing, triggering refresh").arg(output));
			need_refresh = true;
		} else {
			if (i->second != build_params_hash) {
				debug(format("    [!] Output file '%1' build context changed, triggering refresh").arg(output));
				need_refresh = true; // build params modified
			}
		}

		compilation_db.output_build_params[output] = build_params_hash; // update build params hash
	}

	return need_refresh;
}

static bool NeedsCompilation(
	std::map<std::string, Hash> &hashes, const std::set<std::string> &inputs, const std::set<std::string> &outputs, const Data &build_context) {
	ProfilerPerfSection perf("Manage/NeedsCompilation");

	bool need_refresh = false;

	if (TestOutputs(outputs, build_context))
		need_refresh = true;

	if (TestInputs(hashes, inputs))
		need_refresh = true;

	if (need_refresh)
		processed_count += outputs.size();

	for (const auto &output : outputs)
		compilation_db.output_to_inputs[output] = inputs;

	return need_refresh;
}

static void CleanOutputs(const std::set<std::string> &outputs) {
	ProfilerPerfSection perf("Manage/CleanOutputs");

	for (const auto &output : outputs)
		Unlink(FullOutputPath(output).c_str());
}

//
void Reset() {
	compilation_db.source_hashes.clear();
	compilation_db.output_to_inputs.clear();
	compilation_db.output_build_params.clear();
}

//
void SetInputDir(const std::string &path) { input_dir = CleanPath(path); }

bool SetOutputDir(const std::string &path) {
	output_dir = CleanPath(path);
	const auto res = MkTree(output_dir.c_str());
	if (!res) {
		const json json_err = {{"type", "SetOutputDirFailedToMkTree"}, {"path", output_dir}};
		log_error(json_err);
	}
	return res;
}

//
static json LoadMeta(const std::string &path) {
	ProfilerPerfSection perf("Manage/LoadMeta");
	return LoadResourceMetaFromFile(FullInputPath(path));
}

//
static std::mutex default_log_mutex;
static std::map<std::string, std::string> default_log_output;
static std::map<std::string, std::string> default_error_log_output;

static void RunProcess(const std::string &name, const std::string &cmd, const std::string &cwd) {
	//
	const auto cmd_elms = split(cmd, " ");
	ProfilerPerfSection perf(format("Command/RunProcess/%1").arg(cmd_elms[0]));

	log(format("    Spawning compile process for %1").arg(name));

	TinyProcessLib::Process process(
		cmd, cwd,
		[&](const char *bytes, size_t n) {
			std::lock_guard<std::mutex> lock(default_log_mutex);
			default_log_output[name] += {bytes, n};
		},
		[&](const char *bytes, size_t n) {
			std::lock_guard<std::mutex> lock(default_log_mutex);
			default_error_log_output[name] += {bytes, n};
		},
		false);

	if (process.get_id() == 0) {
		ReportFailedInput(name);
		const json json_err = {{"type", "FailedToSpawnCompileProcess"}, {"name", name}};
		log_error(json_err);
	} else {
		const auto res = process.get_exit_status();

		std::string out, err;
		{
			std::lock_guard<std::mutex> lock(default_log_mutex);
			{
				auto i = default_log_output.find(name);
				if (i != std::end(default_log_output)) {
					out = std::move(i->second);
					default_log_output.erase(i);
				}
			}
			{
				auto i = default_error_log_output.find(name);
				if (i != std::end(default_error_log_output)) {
					err = std::move(i->second);
					default_error_log_output.erase(i);
				}
			}
		}

		if (res != 0) {
			ReportFailedInput(name);
			const json json_err = {{"type", "CompileProcessReturnedNonZero"}, {"res", res}, {"cmd", cmd}, {"cwd", cwd}, {"out", out}, {"err", err}};
			log_error(json_err);
		}
	}
}

//
struct task {
	std::string name;
	std::function<std::future<void>()> func;
};

static std::deque<task> task_queue;

static void PushAsyncProcessTask(const std::string &name, const std::string &cmd, const std::string &cwd) {
	task_queue.emplace_back(task{name, [=]() { return std::async(std::launch::async, RunProcess, name, cmd, cwd); }});
}

static int max_async_jobs = 0;

static void RunTaskQueue() {
	const auto task_count = task_queue.size();

	std::deque<std::future<void>> task_running;
	std::string current_task_name;

	while (!task_queue.empty() || !task_running.empty()) {
		while (!task_queue.empty() && task_running.size() < max_async_jobs) {
			current_task_name = task_queue.front().name;
			task_running.emplace_back(task_queue.front().func()); // start task
			task_queue.pop_front();
		}

		const auto task_done = task_count - (task_queue.size() + task_running.size());
		if (progress)
			std::cout << "-> Progress: " << task_done * run_queue_progress_weight / task_count + classify_progress_weight + compile_progress_weight << "% ("
					  << current_task_name << ")" << std::endl;

		time_ns t_ref = time_now();

		while (!task_running.empty()) {
			bool break_ = false;

			for (auto &t : task_running)
				if (t.wait_for(time_to_chrono(time_from_ms(1))) == std::future_status::ready) {
					break_ = true;
					break; // break asap to start a new task
				}

			if (break_)
				break;

			const auto t_now = time_now();

			if (t_now - t_ref >= time_from_sec(5)) {
				log(format("  %1 tasks running, %2 queued...").arg(task_running.size()).arg(task_queue.size()));
				t_ref = t_now;
			}
		}

		task_running.erase(std::remove_if(std::begin(task_running), std::end(task_running),
							   [](const std::future<void> &f) { return f.wait_for(time_to_chrono(0)) == std::future_status::ready; }),
			std::end(task_running));
	}
}

//
static const uint16_t CAB1 = 0xCAB1;

bool LoadCompilationDB(const char *filename) {
	ProfilerPerfSection perf("Manage/LoadCompilationDB");

	Reset();

	if (!Exists(filename))
		return false;

	ScopedFile file(Open(filename));
	if (!file)
		return false;

	//
	if (Read<uint16_t>(file) != CAB1)
		return false;

	const auto version = Read<uint16_t>(file);
	if (version > 2)
		return false;

	//
	if (version >= 2) {
		(void)ReadString(file); // build_sha
		(void)ReadString(file); // version_string
	}

	//
	const auto src_hashes_count = Read<uint32_t>(file);

	for (uint32_t i = 0; i < src_hashes_count; ++i) {
		const auto path = ReadString(file);
		const auto hash = Read<Hash>(file);
		compilation_db.source_hashes[path] = hash;
	}

	//
	const auto asset_to_source_count = Read<uint32_t>(file);

	for (uint32_t i = 0; i < asset_to_source_count; ++i) {
		const auto name = ReadString(file);
		const auto src_count = Read<uint32_t>(file);

		for (uint32_t j = 0; j < src_count; ++j) {
			const auto path = ReadString(file);
			compilation_db.output_to_inputs[name].insert(path);
		}
	}

	//
	if (Tell(file) < GetSize(file)) {
		const auto output_build_hash_count = Read<uint32_t>(file);

		for (uint32_t i = 0; i < output_build_hash_count; ++i) {
			const auto name = ReadString(file);
			const auto hash = Read<Hash>(file);
			compilation_db.output_build_params[name] = hash;
		}
	}

	return true;
}

bool LoadCompilationDB() { return LoadCompilationDB(FullOutputPath("assetc.cab").c_str()); }

bool SaveCompilationDB(const char *path) {
	ProfilerPerfSection perf("Manage/SaveCompilationDB");

	std::cout << format("    %1 input files").arg(compilation_db.source_hashes.size()) << std::endl;
	std::cout << format("    %1 output files").arg(compilation_db.output_to_inputs.size()) << std::endl;
	std::cout << format("    %1 processed").arg(processed_count) << std::endl;
	std::cout << format("    %1 failed").arg(error_count.load()) << std::endl;

	for (const auto &i : failed_inputs)
		std::cout << format("        - FAILED: %1").arg(i) << std::endl;
	failed_inputs.clear();

	std::cout << std::endl;

	processed_count = 0;
	error_count = 0;

	std::cout << format("  Saving compilation DB '%1'").arg(path) << std::endl;

	if (compilation_db.source_hashes.empty()) {
		std::cout << "    Empty DB, nothing to save" << std::endl;
		return false; // nothing to save
	}

	//
	ScopedFile file(OpenWrite(path));

	if (!file) {
		const json json_err = {{"type", "FailedToSaveCompilationDB"}, {"path", path}};
		log_error(json_err);
		return false;
	}

	/*
		version 0: initial version
		version 1: switch to murmurv3 hash
		version 2: add build sha and version string
	*/
	Write(file, CAB1);
	Write(file, uint16_t(2)); // version

	WriteString(file, get_build_sha());
	WriteString(file, get_version_string());

	//
	Write(file, uint32_t(compilation_db.source_hashes.size()));

	for (const auto &i : compilation_db.source_hashes) {
		WriteString(file, i.first);
		Write(file, i.second);
	}

	//
	Write(file, uint32_t(compilation_db.output_to_inputs.size()));

	for (const auto &i : compilation_db.output_to_inputs) {
		const auto &name = i.first;
		WriteString(file, name);

		const auto &sources = i.second;
		Write(file, uint32_t(sources.size()));
		for (auto &s : sources)
			WriteString(file, s);
	}

	//
	Write(file, uint32_t(compilation_db.output_build_params.size()));

	for (const auto &i : compilation_db.output_build_params) {
		WriteString(file, i.first);
		Write(file, i.second);
	}

	return true;
}

bool SaveCompilationDB() { return SaveCompilationDB(FullOutputPath("assetc.cab").c_str()); }

//
static PipelineInfo pipeline;

static void SetRenderPipeline(const PipelineInfo &_pipeline) { pipeline = _pipeline; }

//
static void Copy(std::map<std::string, Hash> &hashes, const std::string &path) {
	ProfilerPerfSection perf("Command/Copy");

	__ASSERT__(!IsDir(path.c_str()));

	log(format("  Copy '%1'").arg(path));

	if (NeedsCompilation(hashes, {path}, {path}, {})) {
		const auto src = FullInputPath(path), dst = FullOutputPath(path);

		MkOutputTree(path);
		CleanOutputs({path});

		if (!CopyFile(src.c_str(), dst.c_str())) {
			ReportFailedInput(src);
			const json json_err = {{"type", "FailedToCopyInput"}, {"src", src}, {"dst", dst}};
			log_error(json_err);
		}
	} else {
		debug("    [O] Copy up to date");
	}
}

//
static std::string profile = "default";

//
static void ProcessScene(const std::string &src, const std::string &dst) {
	Scene scene;
	PipelineResources res;
	LoadSceneContext ctx;

	bool load_res = false;
	try {
		load_res = LoadSceneFromFile(src.c_str(), scene, res, GetForwardPipelineInfo(), ctx, LSSF_All | LSSF_DoNotLoadResources);
	} catch (...) {}

	if (!load_res) {
		ReportFailedInput(src);
		const json json_err = {{"type", "FailedToLoadScene"}, {"src", src}};
		log_error(json_err);
	} else {
		if (!SaveSceneBinaryToFile(dst.c_str(), scene, res)) {
			const json json_err = {{"type", "FailedToSaveScene"}, {"dst", dst}};
			log_error(json_err);
		}
	}
}

void Scene(std::map<std::string, Hash> &hashes, const std::string &path) {
	ProfilerPerfSection perf("Command/Scene");

	log(format("  Scene '%1'").arg(path));

	Data build_ctx;
	Write(build_ctx, std::string(get_version_string()));
	Write(build_ctx, GetSceneBinaryFormatVersion());

	if (NeedsCompilation(hashes, {path}, {path}, build_ctx)) {
		const auto src = FullInputPath(path), dst = FullOutputPath(path);

		MkOutputTree(path);
		CleanOutputs({path});

		task_queue.emplace_back(task{path, [=]() { return std::async(std::launch::async, ProcessScene, src, dst); }});
	} else {
		debug("    [O] Scene up to date");
	}
}

//
void Copy(std::map<std::string, Hash> &hashes, const std::string &path);

//
static std::string Get_texconv_Format(const std::string &f) {
	if (f == "BC1")
		return "BC1_UNORM";
	if (f == "BC2")
		return "BC2_UNORM";
	if (f == "BC3")
		return "BC3_UNORM";
	if (f == "BC4")
		return "BC4_UNORM";
	if (f == "BC5")
		return "BC5_UNORM";
	if (f == "BC6H")
		return "BC6H_UF16";
	if (f == "BC7")
		return "BC7_UNORM";
	return "";
}

static bool PreprocessTexture(const json &i_preprocess_texture, std::map<std::string, Hash> &hashes, std::string &path) {
	// gather dependencies
	std::set<std::string> dependencies;
	for (const auto &i_directive : i_preprocess_texture.items())
		if (i_directive.key() == "construct") {
			if (i_directive.value().is_array()) {
				const auto channel_count = i_directive.value().size();

				if (channel_count < 3 || channel_count > 4) {
					error(format("Cannot construct texture '%1' due to incorrect number of input channel (3 or 4 supported)").arg(path));
					return false;
				}

				for (const auto &v : i_directive.value()) {
					if (v.is_number_integer()) { // constant
						;
					} else if (v.is_string()) { // channel or texture path
						const auto channel_v = v.get<std::string>();

						if (channel_v == "R" || channel_v == "G" || channel_v == "B" || channel_v == "A")
							dependencies.insert(path); // swizzle from input texture
						else
							dependencies.insert(channel_v); // swizzle from foreign texture
					}
				}
			} else {
				error(format("Invalid texture construct tag for '%1', expected an array of channel source").arg(path));
				return false;
			}
		}

	const auto has_construct_directive = i_preprocess_texture.find("construct") != std::end(i_preprocess_texture);

	if (!has_construct_directive)
		dependencies.insert(path); // no construct directive implicitely means we need the input texture

	// build context
	Data build_ctx;
	Write(build_ctx, i_preprocess_texture.dump()); // the whole preprocess block is our build context

	// preprocess if input changed
	const auto output_path = CutFileExtension(path) + ".tmp"; // preprocess into the input folder, output folder is *write-only*

	if (!TestInputs(hashes, dependencies) && IsFile(FullInputPath(output_path).c_str()))
		return true; // nothing to be done

	// load all dependencies
	std::map<std::string, Picture> picture_dependencies;

	for (const auto &dep : dependencies)
		if (!LoadPicture(picture_dependencies[dep], FullInputPath(dep).c_str())) {
			error(format("Failed to load '%1' to construct '%2'").arg(dep).arg(path));
			return false;
		}

	// determine output resolution
	uint16_t out_width = 0, out_height = 0;
	for (const auto &i : picture_dependencies) {
		out_width = Max(out_width, i.second.GetWidth());
		out_height = Max(out_height, i.second.GetHeight());
	}

	// conform dependencies
	for (auto i : picture_dependencies)
		if (i.second.GetWidth() != out_width && i.second.GetHeight() != out_height) {
			error(format("Cannot construct '%1' due to dependency '%2' with incompatible resolution").arg(path).arg(i.first));
			return false;
		}

	//
	Picture out_pic;

	if (!has_construct_directive)
		out_pic = picture_dependencies[path]; // initialize with input if not constructed

	for (const auto &i_directive : i_preprocess_texture.items())
		if (i_directive.key() == "construct") {
			out_pic = Picture(out_width, out_height, i_directive.value().size() == 3 ? PF_RGB24 : PF_RGBA32);

			const auto out_stride = size_of(out_pic.GetFormat());

			size_t channel_count = 0;
			for (const auto &v : i_directive.value()) {
				auto out = out_pic.GetData() + channel_count;

				if (v.is_number_integer()) { // constant
					const auto constant = v.get<int>();

					for (uint16_t x = 0; x < out_width; ++x)
						for (uint16_t y = 0; y < out_height; ++y) {
							*out = constant;
							out += out_stride;
						}
				} else if (v.is_string()) { // channel or texture path
					const auto channel_v = v.get<std::string>();

					uint8_t *in = nullptr;
					size_t in_stride = 0;

					if (channel_v == "R") {
						auto &in_pic = picture_dependencies[path];

						if (GetChannelCount(in_pic.GetFormat()) < 1) {
							error(format("Cannot construct '%1' due to missing R channel").arg(path));
							return false;
						}

						in = in_pic.GetData() + 0;
						in_stride = size_of(in_pic.GetFormat());
					} else if (channel_v == "G") {
						auto &in_pic = picture_dependencies[path];

						if (GetChannelCount(in_pic.GetFormat()) < 2) {
							error(format("Cannot construct '%1' due to missing G channel").arg(path));
							return false;
						}

						in = in_pic.GetData() + 1;
						in_stride = size_of(in_pic.GetFormat());
					} else if (channel_v == "B") {
						auto &in_pic = picture_dependencies[path];

						if (GetChannelCount(in_pic.GetFormat()) < 3) {
							error(format("Cannot construct '%1' due to missing B channel").arg(path));
							return false;
						}

						in = in_pic.GetData() + 2;
						in_stride = size_of(in_pic.GetFormat());
					} else if (channel_v == "A") {
						auto &in_pic = picture_dependencies[path];

						if (GetChannelCount(in_pic.GetFormat()) < 4) {
							error(format("Cannot construct '%1' due to missing A channel").arg(path));
							return false;
						}

						in = in_pic.GetData() + 3;
						in_stride = size_of(in_pic.GetFormat());
					} else {
						auto &in_pic = picture_dependencies[channel_v];

						if (GetChannelCount(in_pic.GetFormat()) <= channel_count) {
							const char *channel_names[] = {"R", "G", "B", "A"};
							error(format("Cannot construct '%1' due to '%2' missing %3 channel").arg(path).arg(channel_v).arg(channel_names[channel_count]));
							return false;
						}

						in = in_pic.GetData() + channel_count;
						in_stride = size_of(in_pic.GetFormat());
					}

					for (uint16_t y = 0; y < out_height; ++y)
						for (uint16_t x = 0; x < out_width; ++x) {
							*out = *in;
							in += in_stride;
							out += out_stride;
						}
				}
				++channel_count;
			}

			if (!SaveTGA(out_pic, FullInputPath(output_path).c_str())) {
				error(format("Failed to save constructed picture as '%1'").arg(output_path));
				return false;
			}
		}

	path = output_path;
	return true;
}

void Texture(std::map<std::string, Hash> &hashes, std::string path) {
	ProfilerPerfSection perf("Command/Texture");

	log(format("  Texture '%1'").arg(path));

	std::string in_path = path; // may be changed by a construct directive

	//
	const auto meta_db = LoadMeta(path);

	std::string type = "Standard";
	GetMetaValue(meta_db, "type", type, profile);
	int max_size = 16384;
	GetMetaValue(meta_db, "max-size", max_size, profile);
	std::string compression = "RAW";
	GetMetaValue(meta_db, "compression", compression, profile);
	bool generate_mips = true;
	GetMetaValue(meta_db, "generate-mips", generate_mips, profile);
	bool generate_probe = false;
	GetMetaValue(meta_db, "generate-probe", generate_probe, profile);
	int max_probe_size = 512;
	GetMetaValue(meta_db, "max-probe-size", max_probe_size, profile);
	bool radiance_edge_fixup = false;
	GetMetaValue(meta_db, "radiance-edge-fixup", radiance_edge_fixup, profile);

	// preprocessing
	const json *i_preprocess;
	if (GetMetaTag(meta_db, "preprocess", i_preprocess, profile)) {
		if (PreprocessTexture(*i_preprocess, hashes, in_path)) {
			;
		} else {
			const json json_err = {{"type", "FailedTexturePreprocessing"}};
			log_error(json_err);
		}
	}

	//
	if (api == "DX12" || api == "DX11" || api == "GL" || api == "GLES" || api == "VK") {
		const auto src = FullInputPath(in_path);

		if (type == "Copy") {
			Copy(hashes, in_path);
		} else if (type != "Ignore") {
			const auto dst = FullOutputPath(path);

			Data build_ctx;
			Write(build_ctx, max_size);
			Write(build_ctx, generate_mips);
			Write(build_ctx, compression);

			const auto texconv_fmt = Get_texconv_Format(compression);
			const auto use_texconv = !toolchain.texconv.empty() && !texconv_fmt.empty();

			if (use_texconv)
				Write(build_ctx, "texconv");

			if (toolchain.texturec.empty()) {
				warn("    Skipping, no compiler found for texture resource");
			} else {
				if (NeedsCompilation(hashes, {in_path}, {path}, build_ctx)) {
					MkOutputTree(path);
					CleanOutputs({path});

					std::string cmd;

					if (use_texconv) { // favor the much faster texconv over texturec
						cmd = format("%1 -y -nologo -maxsize %2 -f %3 -bc x").arg(toolchain.texconv).arg(max_size).arg(texconv_fmt).str();
						if (!generate_mips)
							cmd += " -m 1";
						cmd += format(" \"%1\" \"%2\"").arg(src).arg(dst).str();
					}

					if (cmd.empty()) { // fallback to texturec
						cmd = format("%1 -f \"%2\" -o \"%3\" --as dds --max %4").arg(toolchain.texturec).arg(src).arg(dst).arg(max_size).str();

						if (compression != "RAW")
							cmd += format(" -t %1").arg(compression).str();
						else if (assetc::api == "DX11")
							cmd += " -t BGRA8"; // prevent useless swizzle at runtime
						else
							cmd += " -t RGBA8"; // prevent useless swizzle at runtime

						if (generate_mips)
							cmd += " -m";
					}

					PushAsyncProcessTask(path + " (Texture)", cmd, cwd);
				} else {
					debug("    [O] Texture up to date");
				}
			}
		}

		if (generate_probe) {
			const auto dst = FullOutputPath(path + ".radiance");

			Data build_ctx;
			Write(build_ctx, max_probe_size);
			Write(build_ctx, radiance_edge_fixup);
			Write(build_ctx, 20); // gloss scale
			Write(build_ctx, 0); // gloss bias

			if (toolchain.cmft.empty()) {
				warn("    Skipping, no compiler found for radiance probe resource");
			} else {
				if (NeedsCompilation(hashes, {in_path}, {path + ".radiance"}, build_ctx)) {
					MkOutputTree(path + ".radiance");
					CleanOutputs({path + ".radiance"});

					const auto cmd = format("%1 --input \"%2\" --output0 \"%3\" --output0params dds,rgba16f,cubemap --useOpenCL true --filter radiance "
											"--srcFaceSize %4 --edgeFixup %5 --glossScale 20 --glossBias 0")
										 .arg(toolchain.cmft)
										 .arg(src)
										 .arg(dst)
										 .arg(max_probe_size)
										 .arg(radiance_edge_fixup ? "warp" : "none")
										 .str();

					PushAsyncProcessTask(path + " (Radiance Probe)", cmd, cwd);
				} else {
					debug("    [O] Texture radiance up to date");
				}
			}
		}

		if (generate_probe) {
			const auto dst = FullOutputPath(path + ".irradiance");

			Data build_ctx;
			Write(build_ctx, max_probe_size);

			if (toolchain.cmft.empty()) {
				warn("    Skipping, no compiler found for irradiance probe resource");
			} else {
				if (NeedsCompilation(hashes, {in_path}, {path + ".irradiance"}, build_ctx)) {
					MkOutputTree(path + ".irradiance");
					CleanOutputs({path + ".irradiance"});

					const auto cmd =
						format("%1 --input \"%2\" --output0 \"%3\" --output0params dds,rgba16f,cubemap --useOpenCL false --filter irradiance --srcFaceSize %4")
							.arg(toolchain.cmft)
							.arg(src)
							.arg(dst)
							.arg(max_probe_size)
							.str();

					PushAsyncProcessTask(path + " (Irradiance Probe)", cmd, cwd);
				} else {
					debug("    [O] Texture irradiance up to date");
				}
			}
		}

	} else {
		const json json_err = {{"type", "UnsupportedTextureAPI"}, {"api", api}};
		log_error(json_err);
	}
}

//
static void ProcessGeometry(const std::string &src, const std::string &dst, ModelOptimisationLevel optimisation_level) {
	const auto geo = LoadGeometryFromFile(src.c_str());

	if (!Validate(geo)) {
		const json json_err = {{"type", "InvalidGeometry"}, {"dst", dst}};
		log_error(json_err);
	} else {
		if (!SaveGeometryModelToFile(dst.c_str(), geo, optimisation_level)) {
			const json json_err = {{"type", "FailedToSaveModel"}, {"dst", dst}};
			log_error(json_err);
		}
	}
}

void Geometry(std::map<std::string, Hash> &hashes, const std::string &path) {
	ProfilerPerfSection perf("Command/Geometry");

	log(format("  Geometry '%1'").arg(path));

	const auto meta_db = LoadMeta(path);

	bool cook_model = true;
	GetMetaValue(meta_db, "cook-model", cook_model, profile);

	ModelOptimisationLevel optimisation_level = MOL_Full;

	Data build_ctx;
	Write(build_ctx, std::string(get_version_string()));
	Write(build_ctx, GetModelBinaryFormatVersion());
	Write(build_ctx, cook_model);
	Write(build_ctx, optimisation_level);

	if (NeedsCompilation(hashes, {path}, {path}, build_ctx)) {
		const auto src = FullInputPath(path), dst = FullOutputPath(path);

		MkOutputTree(path);
		CleanOutputs({path});

		task_queue.emplace_back(task{path, [=]() { return std::async(std::launch::async, ProcessGeometry, src, dst, optimisation_level); }});
	} else {
		debug("    [O] Geometry up to date");
	}
}

//
static void BuildComputeShader(std::map<std::string, Hash> &hashes, const std::string &cs_path, const std::string &defines) {
	ProfilerPerfSection perf("Command/ComputeShader");

	std::string cs_profile;
	if (api == "DX11" || api == "DX12") {
		cs_profile = "cs_5_0";
	} else if (api == "GL") {
		cs_profile = "130";
	} else if (api == "GLES") {
		// no profile => essl
	} else if (api == "VK") {
		cs_profile = "spirv";
	} else {
		const json json_err = {{"type", "UnsupportedComputeAPI"}, {"api", api}};
		log_error(json_err);
		return;
	}

	const std::string optim_flags = compile_with_debug_info ? "-O 0 --debug" : "-O 3";

	//
	const auto vs_defines = std::string("IS_COMPUTE_SHADER=1;") + defines;

	Data cs_build_ctx;
	Write(cs_build_ctx, cs_profile);
	Write(cs_build_ctx, optim_flags);
	Write(cs_build_ctx, vs_defines);

	if (toolchain.shaderc.empty()) {
		warn("    Skipping, no compiler found for compute resource");
	} else {
		if (NeedsCompilation(hashes, {cs_path}, {cs_path}, cs_build_ctx)) {
			if (!cs_profile.empty()) // GLES profile must be empty...
				cs_profile = "-p " + cs_profile;

			const auto cs_out = FullOutputPath(cs_path);

			MkOutputTree(cs_path);
			CleanOutputs({cs_path});

			const auto cs_cmd = format("%1 -f \"%2\" -o \"%3\" --platform %4 %5 %6 --type compute --define \"%7\"")
									.arg(toolchain.shaderc)
									.arg(FullInputPath(cs_path))
									.arg(cs_out)
									.arg(assetc::platform)
									.arg(cs_profile)
									.arg(optim_flags)
									.arg(vs_defines);

			PushAsyncProcessTask(cs_path + " (Compute Shader)", cs_cmd, cwd);
		} else {
			debug("    [O] Compute shader up to date");
		}
	}
}

static void ComputeShader(std::map<std::string, Hash> &hashes, const std::string &cs_path) {
	log(format("  Compute Shader '%1'").arg(cs_path));
	BuildComputeShader(hashes, cs_path, global_shader_defines);
}

//
static void BuildShader(std::map<std::string, Hash> &hashes, const std::string &name, const std::string &vs_path, const std::string &fs_path,
	const std::string &varying_path, const std::string &defines) {
	ProfilerPerfSection perf("Command/Shader");

	std::string vs_profile, fs_profile;
	if (api == "DX11" || api == "DX12") {
		vs_profile = "vs_5_0";
		fs_profile = "ps_5_0";
	} else if (api == "GL") {
		vs_profile = fs_profile = "130";
	} else if (api == "GLES") {
		// no profile => essl
	} else if (api == "VK") {
		vs_profile = fs_profile = "spirv";
	} else {
		const json json_err = {{"type", "UnsupportedShaderAPI"}, {"api", api}};
		log_error(json_err);
		return;
	}

	const std::string optim_flags = compile_with_debug_info ? "--debug" : "-O 3";

	//
	const auto vs_defines = std::string("IS_VERTEX_SHADER=1;") + defines;

	Data vs_build_ctx;
	Write(vs_build_ctx, vs_profile);
	Write(vs_build_ctx, optim_flags);
	Write(vs_build_ctx, vs_defines);

	const auto vs_name = format("%1.vsb").arg(name);

	if (toolchain.shaderc.empty()) {
		warn("    Skipping, no compiler found for shader resource");
	} else {
		if (NeedsCompilation(hashes, {vs_path, varying_path}, {vs_name}, vs_build_ctx)) {
			if (!vs_profile.empty()) // GLES profile must be empty...
				vs_profile = "-p " + vs_profile;

			const auto vs_out = FullOutputPath(vs_name);

			MkOutputTree(vs_name);
			CleanOutputs({vs_name});

			const auto vs_cmd = format("%1 -f \"%2\" -o \"%3\" --varyingdef \"%4\" --type v --platform %5 %6 %7 --define \"%8\"")
									.arg(toolchain.shaderc)
									.arg(FullInputPath(vs_path))
									.arg(vs_out)
									.arg(FullInputPath(varying_path))
									.arg(assetc::platform)
									.arg(vs_profile)
									.arg(optim_flags)
									.arg(vs_defines);

			PushAsyncProcessTask(name + " (Vertex Shader)", vs_cmd, cwd);
		} else {
			debug("    [O] Vertex shader up to date");
		}
	}

	//
	const auto fs_defines = std::string("IS_FRAGMENT_SHADER=1;") + defines;

	Data fs_build_ctx;
	Write(fs_build_ctx, fs_profile);
	Write(fs_build_ctx, optim_flags);
	Write(fs_build_ctx, fs_defines);

	const auto fs_name = format("%1.fsb").arg(name);

	if (toolchain.shaderc.empty()) {
		warn("    Skipping, no compiler found for shader resource");
	} else {
		if (NeedsCompilation(hashes, {fs_path, varying_path}, {fs_name}, fs_build_ctx)) {
			if (!fs_profile.empty())
				fs_profile = "-p " + fs_profile;

			const auto fs_out = FullOutputPath(fs_name);

			MkOutputTree(fs_name);
			CleanOutputs({fs_name});

			const auto fs_cmd = format("%1 -f \"%2\" -o \"%3\" --varyingdef \"%4\" --type f --platform %5 %6 %7 --define \"%8\"")
									.arg(toolchain.shaderc)
									.arg(FullInputPath(fs_path))
									.arg(fs_out)
									.arg(FullInputPath(varying_path))
									.arg(assetc::platform)
									.arg(fs_profile)
									.arg(optim_flags)
									.arg(fs_defines);

			PushAsyncProcessTask(name + " (Fragment Shader)", fs_cmd, cwd);
		} else {
			debug("    [O] Pixel shader up to date");
		}
	}
}

static void Shader(std::map<std::string, Hash> &hashes, const std::string &vs_path, const std::string &fs_path, const std::string &varying_path) {
	const auto name = slice(vs_path, 0, -6);
	log(format("  Shader '%1'").arg(name));
	BuildShader(hashes, name, vs_path, fs_path, varying_path, global_shader_defines);
}

//
static void BuildPipelineShaderVariant(std::map<std::string, Hash> &hashes, const std::string &name, const std::string &feats_path, const std::string &vs_path,
	const std::string &fs_path, const std::string &varying_path, const std::string &defines) {
	ProfilerPerfSection perf("Manage/BuildPipelineShaderVariant");

	size_t config = 0;
	for (auto &variant : pipeline.configs) {
		log(format("    Pipeline shader variant '%1' for pipeline config %2").arg(name).arg(config));
		const auto variant_name = format("%1_pipe-%2-cfg-%3").arg(name).arg(pipeline.name).arg(config++);
		const auto variant_defines = join(std::begin(variant), std::end(variant), ";") + ";" + defines;
		BuildShader(hashes, variant_name, vs_path, fs_path, varying_path, global_shader_defines + variant_defines);
	}
}

//
static bool IterateProgramFeatureStates(const std::vector<PipelineProgramFeature> &feats, std::vector<int> &states,
	const std::function<void(const std::vector<PipelineProgramFeature> &feats, const std::vector<int> &states)> &process_states, int i = 0) {
	if (states.empty()) {
		process_states(feats, states); // single program for no feature pipeline program
		return false;
	}

	if (i == states.size())
		return false;

	for (int j = 0; j < GetPipelineProgramFeatureStateCount(feats[i]); ++j) {
		states[i] = j;
		if (!IterateProgramFeatureStates(feats, states, process_states, i + 1))
			process_states(feats, states);
	}
	return true;
}

static std::string GetDefines(const std::vector<PipelineProgramFeature> &feats, const std::vector<int> &states) {
	std::vector<std::string> defines;

	bool has_ambient_uv_feature = false;

	for (size_t i = 0; i < states.size(); ++i) {
		const auto state = states[i];

		if (feats[i] == OptionalBaseColorOpacityMap) {
			defines.push_back(format("USE_BASE_COLOR_OPACITY_MAP=%1").arg(state ? "1" : "0"));
		} else if (feats[i] == OptionalOcclusionRoughnessMetalnessMap) {
			defines.push_back(format("USE_OCCLUSION_ROUGHNESS_METALNESS_MAP=%1").arg(state ? "1" : "0"));
		} else if (feats[i] == OptionalDiffuseMap) {
			defines.push_back(format("USE_DIFFUSE_MAP=%1").arg(state ? "1" : "0"));
		} else if (feats[i] == OptionalSpecularMap) {
			defines.push_back(format("USE_SPECULAR_MAP=%1").arg(state ? "1" : "0"));
		} else if (feats[i] == OptionalLightMap) {
			defines.push_back(format("USE_LIGHT_MAP=%1").arg(state ? "1" : "0"));
		} else if (feats[i] == OptionalSelfMap) {
			defines.push_back(format("USE_SELF_MAP=%1").arg(state ? "1" : "0"));
		} else if (feats[i] == OptionalOpacityMap) {
			defines.push_back(format("USE_OPACITY_MAP=%1").arg(state ? "1" : "0"));
		} else if (feats[i] == OptionalAmbientMap) {
			defines.push_back(format("USE_AMBIENT_MAP=%1").arg(state ? "1" : "0"));
		} else if (feats[i] == OptionalReflectionMap) {
			defines.push_back(format("USE_REFLECTION_MAP=%1").arg(state ? "1" : "0"));
		} else if (feats[i] == OptionalNormalMap) {
			defines.push_back(format("USE_NORMAL_MAP=%1").arg(state ? "1" : "0"));
		} else if (feats[i] == NormalMapInWorldSpace) {
			defines.push_back(format("NORMAL_MAP_IN_WORLD_SPACE=%1").arg(state ? "1" : "0"));
		} else if (feats[i] == DiffuseUV1) {
			defines.push_back(format("DIFFUSE_UV_CHANNEL=%1").arg(state));
		} else if (feats[i] == SpecularUV1) {
			defines.push_back(format("SPECULAR_UV_CHANNEL=%1").arg(state));
		} else if (feats[i] == AmbientUV1) {
			defines.push_back(format("AMBIENT_UV_CHANNEL=%1").arg(state));
			has_ambient_uv_feature = true;
		} else if (feats[i] == OptionalSkinning) {
			defines.push_back(format("ENABLE_SKINNING=%1").arg(state ? "1" : "0"));
		} else if (feats[i] == OptionalAlphaCut) {
			defines.push_back(format("ENABLE_ALPHA_CUT=%1").arg(state ? "1" : "0"));
		}
	}

	if (!has_ambient_uv_feature)
		defines.push_back("AMBIENT_UV_CHANNEL=1"); // [EJ02042022] default to legacy behavior

	return join(std::begin(defines), std::end(defines), ";");
}

//
static void PipelineShader(
	std::map<std::string, Hash> &hashes, const std::string &hps_path, const std::string &vs_path, const std::string &fs_path, const std::string &varying_path) {
	ProfilerPerfSection perf("Command/PipelineShader");

	log(format("  Pipeline shader '%1' for pipeline '%2'").arg(hps_path).arg(pipeline.name));

	Copy(hashes, hps_path);

	const auto name = CutFileExtension(hps_path);
	bool success;
	const auto feats = LoadPipelineProgramFeaturesFromFile(PathJoin({input_dir, hps_path}).c_str(), success);
	if (!success) {
		const json json_err = {{"type", "FailedToLoadPipelineProgramFeatures"}, {"path", hps_path}};
		log_error(json_err);
	}

	std::vector<int> states(feats.size(), 0);
	IterateProgramFeatureStates(feats, states, [&](const std::vector<PipelineProgramFeature> &feats, const std::vector<int> &states) {
		const auto variant_defines = GetDefines(feats, states);
		const auto variant_name = GetPipelineProgramVariantName(name.c_str(), feats, states);
		BuildPipelineShaderVariant(hashes, variant_name, hps_path, vs_path, fs_path, varying_path, variant_defines);
	});
}

//
static void LuaScript(std::map<std::string, Hash> &hashes, const std::string &path) {
	ProfilerPerfSection perf("Command/LuaScript");

	log(format("  Lua script '%1'").arg(path));

	if (toolchain.luac.empty()) {
		warn("    Skipping, no compiler found for Lua script resource");
	} else {
		if (NeedsCompilation(hashes, {path}, {path}, {})) {
			const auto src = FullInputPath(path), dst = FullOutputPath(path);

			MkOutputTree(path);
			CleanOutputs({path});

			const auto cmd = format("%1 -o %3 -s %2").arg(toolchain.luac).arg(src).arg(dst);
			PushAsyncProcessTask(path, cmd, cwd);
		} else {
			debug("  [O] Lua script up to date");
		}
	}
}

//
static void Physics(std::map<std::string, Hash> &hashes, const std::string &path) {
	ProfilerPerfSection perf("Command/Physics");

	log(format("  Physics resource '%1'").arg(path));
	// [todo]
	debug("    Skipping, no compiler found for physics resource");
}

static void PathFinding(std::map<std::string, Hash> &hashes, const std::string &path) {
	ProfilerPerfSection perf("Command/PathFinding");

	log(format("  Pathfinding resource '%1'").arg(path));

	if (toolchain.recastc.empty()) {
		debug("    Skipping, no compiler found for pathfinding resource");
	} else {
		if (NeedsCompilation(hashes, {path}, {path}, {})) {
			const auto src = FullInputPath(path), dst = FullOutputPath(path);

			MkOutputTree(path);
			CleanOutputs({path});

			const auto cmd = format("%1 %2 %3 -root %4").arg(toolchain.recastc).arg(src).arg(dst).arg(input_dir);
			PushAsyncProcessTask(path, cmd, GetCurrentWorkingDirectory());
		} else {
			debug("  [O] Pathfinding resource up to date");
		}
	}
}

//
enum class AssetType { Unprocessed, Ignore, Broken, Scene, Texture, Geometry, Lua, Shader, PipelineShader, Physics, PathFinding, ComputeShader, Count };

#if 0
// CWE 561: The function 'AssetTypeToString' is never used.
static const std::string& AssetTypeToString(AssetType type) {
	static const std::string types[static_cast<int>(AssetType::Count) + 1] = {
		"Unprocessed", "Ignore", "Broken", "Scene", "Texture", "Geometry", "Lua", "Shader", "PipelineShader", "Physics", "PathFinding", "ComputeShader", "Undefined"};
	return types[int(type)];
}
#endif

struct AssetConfig {
	AssetType type;
};

static AssetType GetAssetFileType(std::string path, const std::vector<std::string> &all_files, std::vector<std::string> &out_files) {
	static const std::set<std::string> texture_exts = {"bmp", "exr", "gif", "jpg", "hdr", "png", "psd", "tga"};
	static const std::set<std::string> ignored_exts = {"tmp"};

	const auto ext = tolower(GetFileExtension(path));

	if (ext == "scn") {
		out_files.push_back(path);
		return AssetType::Scene;
	} else if (texture_exts.find(ext) != std::end(texture_exts)) {
		out_files.push_back(path);
		return AssetType::Texture;
	} else if (ignored_exts.find(ext) != std::end(ignored_exts)) {
		return AssetType::Ignore;
	} else if (ext == "geo") {
		out_files.push_back(path);
		return AssetType::Geometry;
	} else if (ext == "lua") {
		out_files.push_back(path);
		return AssetType::Lua;
	} else if (ext == "sc") {
		std::string name;
		if (ends_with(path, "_vs.sc") || ends_with(path, "_fs.sc"))
			name = slice(path, 0, -6);
		else if (ends_with(path, "_varying.def"))
			name = slice(path, 0, -11);
		else
			name = slice(path, 0, -3);

		const auto hps_name = name + ".hps", vs_name = name + "_vs.sc", fs_name = name + "_fs.sc", varying_name = name + "_varying.def";

		const auto hps_i = std::find(std::begin(all_files), std::end(all_files), hps_name);
		const auto vs_i = std::find(std::begin(all_files), std::end(all_files), vs_name);
		const auto fs_i = std::find(std::begin(all_files), std::end(all_files), fs_name);
		const auto varying_i = std::find(std::begin(all_files), std::end(all_files), varying_name);

		const bool has_hps = hps_i != std::end(all_files), has_vs = vs_i != std::end(all_files), has_fs = fs_i != std::end(all_files),
				   has_varying = varying_i != std::end(all_files);

		if (has_hps) { // that's a pipeline shader
			if (has_vs && has_fs && has_varying) {
				out_files.push_back(hps_name);
				out_files.push_back(vs_name);
				out_files.push_back(fs_name);
				out_files.push_back(varying_name);
				return AssetType::PipelineShader;
			}
			out_files.push_back(hps_name);
		} else {
			if (has_vs && has_fs && has_varying) {
				out_files.push_back(vs_name);
				out_files.push_back(fs_name);
				out_files.push_back(varying_name);
				return AssetType::Shader;
			} else {
				out_files.push_back(path);
				return AssetType::ComputeShader;
			}
		}

		if (has_vs)
			out_files.push_back(vs_name);
		if (has_fs)
			out_files.push_back(fs_name);
		if (has_varying)
			out_files.push_back(varying_name);

		return AssetType::Broken;
	} else if (ext == "hps") {
		const auto name = slice(path, 0, -4);
		const auto vs_name = name + "_vs.sc", fs_name = name + "_fs.sc", varying_name = name + "_varying.def";

		const auto vs_i = std::find(std::begin(all_files), std::end(all_files), vs_name);
		const auto fs_i = std::find(std::begin(all_files), std::end(all_files), fs_name);
		const auto varying_i = std::find(std::begin(all_files), std::end(all_files), varying_name);

		const bool has_vs = vs_i != std::end(all_files), has_fs = fs_i != std::end(all_files), has_varying = varying_i != std::end(all_files);

		out_files.push_back(path);

		if (has_vs)
			out_files.push_back(vs_name);
		if (has_fs)
			out_files.push_back(fs_name);
		if (has_varying)
			out_files.push_back(varying_name);

		if (has_vs && has_fs && has_varying)
			return AssetType::PipelineShader;

		return AssetType::Broken;
	} else if (ext == "physics") {
		out_files.push_back(path);
		return AssetType::Physics;
	} else if (ext == "pathfinding") {
		out_files.push_back(path);
		return AssetType::PathFinding;
	}

	out_files.push_back(path);
	return AssetType::Unprocessed;
}

//
static std::vector<std::string> DirEntriesToPaths(const std::vector<DirEntry> &entries) {
	std::vector<std::string> paths;
	std::transform(std::begin(entries), std::end(entries), std::back_inserter(paths), [](const DirEntry &e) { return e.name; });
	return paths;
}

static std::vector<std::string> blacklist = {"_meta", "_prod", ".git", ".svn"};

static std::map<std::vector<std::string>, AssetConfig> ClassifyInputs(
	const std::vector<std::string> &all_files, const std::vector<std::string> &files_to_classify) {
	ProfilerPerfSection perf("Manage/ClassifyInputs");

	std::map<std::vector<std::string>, AssetConfig> asset_configs;

	size_t i = 0;
	const size_t file_count = files_to_classify.size();

	for (auto &path : files_to_classify) {
		if (progress)
			std::cout << "-> Progress: " << i * classify_progress_weight / file_count << "% (" << path << ")" << std::endl;
		++i;

		bool is_blacklisted = false;
		for (auto &prefix : blacklist)
			if (starts_with(path, prefix)) {
				is_blacklisted = true;
				break;
			}

		if (is_blacklisted)
			continue;

		bool already_classified = false;
		for (auto &cfg : asset_configs)
			if (std::find(std::begin(cfg.first), std::end(cfg.first), path) != std::end(cfg.first)) {
				already_classified = true;
				break;
			}

		if (already_classified)
			continue; // file already classified

		std::vector<std::string> out_files;
		const auto type = GetAssetFileType(path, all_files, out_files);

		asset_configs[std::move(out_files)] = {type};
	}
	return asset_configs;
}

static std::vector<std::string> GetInputDirFiles() { return DirEntriesToPaths(ListDirRecursive(input_dir.c_str(), DE_File)); }

static std::map<std::vector<std::string>, AssetConfig> ClassifyInputDir() {
	const auto all_files = GetInputDirFiles();
	return ClassifyInputs(all_files, all_files);
}

//
static bool CompileClassifiedInputs(const std::map<std::vector<std::string>, AssetConfig> &inputs) {
	ProfilerPerfSection perf("Manage/CompileClassifiedInputs");

	const auto t_start = time_now();
	std::cout << "Compilation start" << std::endl;

	std::map<std::string, Hash> updated_hashes;

	const auto input_count = inputs.size();
	size_t j = 0;

	for (auto &i : inputs) {
		const auto &names = i.first;

		if (!names.empty()) {
			const auto &config = i.second;

			if (progress)
				std::cout << "-> Progress: " << j * compile_progress_weight / input_count + classify_progress_weight << "% (" << names[0] << ")" << std::endl;
			++j;

			if (config.type == AssetType::Unprocessed)
				Copy(updated_hashes, names[0]);
			else if (config.type == AssetType::Scene)
				Scene(updated_hashes, names[0]);
			else if (config.type == AssetType::Texture)
				Texture(updated_hashes, names[0]);
			else if (config.type == AssetType::Geometry)
				Geometry(updated_hashes, names[0]);
			else if (config.type == AssetType::Lua)
				LuaScript(updated_hashes, names[0]);
			else if (config.type == AssetType::ComputeShader)
				ComputeShader(updated_hashes, names[0]);
			else if (config.type == AssetType::Shader)
				Shader(updated_hashes, names[0], names[1], names[2]);
			else if (config.type == AssetType::PipelineShader)
				PipelineShader(updated_hashes, names[0], names[1], names[2], names[3]);
			else if (config.type == AssetType::Physics)
				Physics(updated_hashes, names[0]);
			else if (config.type == AssetType::PathFinding)
				PathFinding(updated_hashes, names[0]);
			else if (config.type == AssetType::Broken)
				((void)0);
		}

		if (!IsPollProcessUp()) {
			std::cout << "Compilation aborted after " << time_to_ms(time_now() - t_start) << "ms, poll process down" << std::endl;
			return false;
		}
	}

	for (const auto &h : updated_hashes)
		compilation_db.source_hashes[h.first] = h.second; // commit updated hashes

	assetc::RunTaskQueue();
	assetc::SaveCompilationDB();

	std::cout << "Compilation done, took " << time_to_ms(time_now() - t_start) << "ms" << std::endl;
	return true;
}

//
static bool clean_outputs_for_removed_inputs = true;

static void CleanOutputsForRemovedInputs() {
	ProfilerPerfSection perf("Manage/CleanOutputsForRemovedInputs");

	std::map<std::string, std::set<std::string>> db_input_to_outputs;
	for (const auto &i : compilation_db.output_to_inputs)
		for (const auto &input : i.second)
			db_input_to_outputs[input].insert(i.first);

	const auto input_files = GetInputDirFiles(); // available inputs

	size_t removed = 0;
	for (const auto &i : db_input_to_outputs)
		if (std::find(std::begin(input_files), std::end(input_files), i.first) == std::end(input_files)) {
			for (const auto &output : i.second) // DB input is missing from input files, remove its associated outputs
				if (Unlink(FullOutputPath(output).c_str()))
					++removed;
		}

	std::cout << "Removed " << removed << " outputs due to missing input" << std::endl;
}

//
static void DaemonMode() {
	log("Entering daemon mode, press Ctrl+C to close\n");

	WatchDirectory(input_dir, true);

	while (true) {
		std::this_thread::sleep_for(std::chrono::seconds(1));

		const auto events = GetDirectoryWatchEvents(input_dir);

		std::set<std::string> modified_files;
		for (const auto &ev : events)
			if ((ev.type == WatchEvent::FileAdded) || (ev.type == WatchEvent::FileModified))
				if (IsFile(PathJoin({input_dir, ev.path}).c_str()))
					modified_files.insert(ev.path);

		// change in .meta must trickle down to the file it refers to
		std::set<std::string> modified_files_;
		for (const auto &i : modified_files) {
			modified_files_.insert(i);
			if (ends_with(i, ".meta", insensitive))
				modified_files_.insert(left(i, -5));
		}

		if (!modified_files_.empty()) {
			log(format("File system changes detected (%1):").arg(modified_files_.size()));
			for (const auto &f : modified_files_)
				log((std::string("    - ") + f).c_str());

			if (assetc::clean_outputs_for_removed_inputs)
				assetc::CleanOutputsForRemovedInputs();

			if (!CompileClassifiedInputs(ClassifyInputs(GetInputDirFiles(), {std::begin(modified_files_), std::end(modified_files_)})))
				break;
			log("Press Ctrl+C to close\n");
		}
	}

	UnwatchDirectory(input_dir);
}

//
static void OutputPerfReport() {
	const auto profile = EndProfilerFrame();

	time_ns total = 0;
	for (auto &task : profile.tasks)
		if (task.name == "Total")
			total = task.duration;

	if (total) {
		std::cout << std::endl << "Performance report" << std::endl;
		for (auto &task : profile.tasks)
			std::cout << "  - " << task.name << ": "
					  << "(" << (task.duration * 100 / total) << "%) " << FormatTime(task.duration) << " (" << task.section_indexes.size() << " call)"
					  << std::endl;

		std::cout << std::endl;
	}
}

} // namespace assetc

//
static void OutputUsage(const CmdLineFormat &cmd_format) {
	std::cout << "Usage: assetc " << word_wrap(FormatCmdLineArgs(cmd_format), 120, 14) << std::endl << std::endl;
	std::cout << FormatCmdLineArgsDescription(cmd_format);
}

#if WIN32
int wmain(int narg, wchar_t **argv) {
	std::vector<std::string> args_utf8(narg + 1);
	for (int i = 0; i < narg; ++i)
		args_utf8[i] = hg::wchar_to_utf8(argv[i]);

	std::vector<const char *> _args(narg + 1);
	for (int i = 0; i < narg; ++i)
		_args[i] = args_utf8[i].c_str();

	const char **args = _args.data();
#else
int main(int narg, const char **args) {
#endif
	const auto exe_path = GetFilePath(args[0]);

	std::cout << "Harfang ASSETC 1.1" << std::endl;
	assetc::cwd = GetCurrentWorkingDirectory();

	const auto default_toolchain_path = format("toolchains/host-%1-target-%2").arg(get_host_string()).arg(get_target_string()).str();

	// parse command line
	CmdLineFormat cmd_format = {
		{
			{"-daemon", "Run in the background and watch for modifications to the input folder"},
			{"-progress", "Output progress to the standard C output stream"},
			{"-log_errors_to_stderr", "Log errors as JSON to the standard C error output stream"},
			{"-debug", "Compile in debug mode (eg. output debug informations in shader)"},
			{"-quiet", "Disable all build information but errors"},
			{"-verbose", "Output additional information about the compilation process"},
			{"-fast_check", "Perform modification detection using input file timestamp"},
			{"-no_clean_removed_inputs", "Do not remove outputs for removed input files"},
		},
		{
			{"-job", "Maximum number of parallel job (0 - automatic)", true},
			{"-toolchain", format("Path to the toolchain folder (default: %1)").arg(default_toolchain_path), true},
			{"-platform", "Select the target platform to compile for (defaults to current platform)", true},
			{"-api", "Select the platform graphic API to compile for", true},
			{"-defines", "Semicolon separated defines to pass to shaderc (eg. FLAG;VALUE=2)", true},
			{"-poll_pid", "Poll the provided process and exit assetc if down", true},
		},
		{
			{"input", "Input folder to compile sources from"},
			{"output", "Output folder for compiled assets", true},
		},
		{
			{"-d", "-daemon"},
			{"-l", "-log_errors_to_stderr"},
			{"-j", "-job"},
			{"-t", "-toolchain"},
			{"-p", "-platform"},
			{"-q", "-quiet"},
			{"-v", "-verbose"},
			{"-D", "-defines"},
			{"-f", "-fast_check"},
			{"-n", "-no_clean_removed_inputs"},
		},
	};

	CmdLineContent cmd_content;
	if (!ParseCmdLine({args + 1, args + narg}, cmd_format, cmd_content)) {
		OutputUsage(cmd_format);
		return -1;
	}

	// toolchain
	const auto toolchain_path = GetCmdLineSingleValue(cmd_content, "-toolchain", PathJoin({exe_path, default_toolchain_path}));

	assetc::SetToolchain(toolchain_path);
	assetc::ToolchainExists();

	/*
	if (!assetc::ToolchainExists()) {
		std::cout << "Error: Toolchain not found " << toolchain_path << std::endl;
		OutputUsage(cmd_format);
		return -4;
	}
	*/

	assetc::api = GetCmdLineSingleValue(cmd_content, "-api", assetc::api);
	assetc::platform = GetCmdLineSingleValue(cmd_content, "-platform", assetc::platform);
	assetc::global_shader_defines = GetCmdLineSingleValue(cmd_content, "-defines", "");

	if (!assetc::global_shader_defines.empty() && !ends_with(assetc::global_shader_defines, ";"))
		assetc::global_shader_defines += ";";

	assetc::max_async_jobs = GetCmdLineSingleValue(cmd_content, "-job", 0);
	if (assetc::max_async_jobs < 1)
		assetc::max_async_jobs = std::thread::hardware_concurrency();

	assetc::progress = GetCmdLineFlagValue(cmd_content, "-progress");
	assetc::log_errors_to_stderr = GetCmdLineFlagValue(cmd_content, "-log_errors_to_stderr");
	assetc::compile_with_debug_info = GetCmdLineFlagValue(cmd_content, "-debug");

	assetc::fast_check = GetCmdLineFlagValue(cmd_content, "-fast_check");

	assetc::poll_process_id = GetCmdLineSingleValue(cmd_content, "-poll_pid", 0);
	assetc::clean_outputs_for_removed_inputs = !GetCmdLineFlagValue(cmd_content, "-no_clean_removed_inputs");

	assetc::SetRenderPipeline(GetForwardPipelineInfo());

	//
	int log_level = LL_Normal | LL_Warning | LL_Error;

	if (GetCmdLineFlagValue(cmd_content, "-quiet"))
		log_level = LL_Warning | LL_Error;

	if (GetCmdLineFlagValue(cmd_content, "-verbose"))
		log_level = LL_All; // verbose logs all

	set_log_level(log_level);

	// input folder compilation mode
	if (cmd_content.positionals.empty()) {
		std::cout << "Error: No input folder specified" << std::endl;
		OutputUsage(cmd_format);
		return -2;
	}

	assetc::SetInputDir(cmd_content.positionals[0]);

	if (!IsDir(assetc::input_dir.c_str())) {
		std::cout << "Error: Input dir '" << assetc::input_dir << "' not found" << std::endl;
		OutputUsage(cmd_format);
		return -3;
	}

	if (cmd_content.positionals.size() > 1)
		assetc::SetOutputDir(cmd_content.positionals[1]);
	else
		assetc::SetOutputDir(assetc::input_dir + "_compiled");

	//
	auto exe_path_or_nothing = [](const std::string &path) { return path.empty() ? "-" : path; };

	log(format("> Input dir: %1").arg(assetc::input_dir));
	log(format("> Output dir: %1").arg(assetc::output_dir));
	log("");
	log(format("> Target platform: %1").arg(assetc::platform));
	log(format("> Target graphics API: %1").arg(assetc::api));
	log(format("> Target pipeline: %1").arg(assetc::pipeline.name));
	log("");
	log(format("> Using %1 parallel job").arg(assetc::max_async_jobs));
	log(format("> Toolchain compilers (%1):").arg(toolchain_path));
	log(format("  - Shader      %1").arg(exe_path_or_nothing(assetc::toolchain.shaderc)));
	log(format("  - Texture     %1").arg(exe_path_or_nothing(assetc::toolchain.texturec)));
	log(format("  - Probe       %1").arg(exe_path_or_nothing(assetc::toolchain.cmft)));
	log(format("  - Lua         %1").arg(exe_path_or_nothing(assetc::toolchain.luac)));
	log(format("  - Pathfinding %1").arg(exe_path_or_nothing(assetc::toolchain.recastc)));
	log("");

	// initial run over input directory (process all files)
	{
		ProfilerPerfSection perf("Total");

		assetc::LoadCompilationDB();

		if (assetc::clean_outputs_for_removed_inputs)
			assetc::CleanOutputsForRemovedInputs();

		if (assetc::CompileClassifiedInputs(assetc::ClassifyInputDir())) {
			// enter daemon mode, process input dir files as they change
			if (GetCmdLineFlagValue(cmd_content, "-daemon"))
				assetc::DaemonMode();
		}
	}

	//
	assetc::OutputPerfReport();

	std::cout << "Exit" << std::endl;
	return 0;
}
