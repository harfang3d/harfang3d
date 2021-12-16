// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#include "foundation/profiler.h"
#include "foundation/math.h"
#include <algorithm>
#include <mutex>

namespace hg {

static std::mutex lock;
static std::vector<size_t> task_bucket[256];

struct Task {
	std::string name;
	std::vector<size_t> section_indexes;
};

static std::vector<Task> tasks;

struct Section {
	std::thread::id thread_id;
	time_ns start{0}, end{0};
	std::string details;
};

static std::vector<Section> sections;

static uint32_t frame{0};
static ProfilerFrame last_frame_profile;

//
static size_t GetTaskBucket(const std::string &name) { return std::hash<std::string>{}(name)&255; }

static size_t GetTaskInBucket(size_t bucket_index, const std::string &name) {
	auto &bucket = task_bucket[bucket_index];

	// look into the bucket for the period
	for (auto &i : bucket)
		if (tasks[i].name == name)
			return i;

	// create missing event
	auto event = tasks.emplace(tasks.end());
	event->name = name;

	bucket.push_back(tasks.size() - 1);
	return tasks.size() - 1;
}

//
static bool _compare_tree_entry(const ProfilerFrame::Task &a, const ProfilerFrame::Task &b) {
	auto task_a_name_length = a.name.length(), task_b_name_length = b.name.length();
	auto shortest_length = Min(task_a_name_length, task_b_name_length);

	for (uint32_t i = 0; i < shortest_length; ++i) {
		auto dt = b.name[i] - a.name[i];
		if (dt)
			return dt > 0;
	}

	return task_a_name_length < task_b_name_length;
}

static void CaptureFrame(ProfilerFrame &f) {
	auto t_now = time_now();

	f.frame = frame;

	f.tasks.clear();
	f.sections.clear();

	auto task_count = tasks.size();

	if (!task_count) {
		f.start = f.end = 0;
	} else {
		f.start = f.end = sections[0].start;

		f.sections.resize(sections.size());
		for (uint32_t i = 0; i < sections.size(); ++i) {
			auto &frame_section = f.sections[i];
			auto &section = sections[i];

			auto thread_id = section.thread_id;

			frame_section.thread_id = thread_id;
			frame_section.start = section.start;
			if (section.end == 0)
				frame_section.end = t_now; // fix-up pending section
			else
				frame_section.end = section.end;
			frame_section.details = section.details;

			if (frame_section.start < f.start)
				f.start = frame_section.start;
			if (frame_section.end > f.end)
				f.end = frame_section.end;
		}

		f.tasks.clear();
		f.tasks.resize(task_count);

		for (uint32_t i = 0; i < task_count; ++i) {
			auto &task = tasks[i];
			auto &frame_task = f.tasks[i];

			frame_task.name = task.name;
			frame_task.duration = 0;

			for (auto &section_index : task.section_indexes) {
				auto &section = f.sections[section_index];
				frame_task.duration += section.end - section.start;
			}

			frame_task.section_indexes = task.section_indexes;
		}

		std::sort(f.tasks.begin(), f.tasks.end(), &_compare_tree_entry);
	}
}

//
void EndProfilerFrame() {
	std::lock_guard<std::mutex> guard(lock);

	CaptureFrame(last_frame_profile);

	for (auto &bucket : task_bucket)
		bucket.clear();

	tasks.clear();
	sections.clear();

	++frame;
}

const ProfilerFrame &GetLastFrameProfile() { return last_frame_profile; }

//
ProfilerSectionIndex BeginProfilerSection(const std::string &name, const std::string &section_details) {
	std::lock_guard<std::mutex> guard(lock);

	auto bucket_idx = GetTaskBucket(name);
	auto task_idx = GetTaskInBucket(bucket_idx, name);
	auto &task = tasks[task_idx];

	auto section = sections.emplace(sections.end());
	section->thread_id = std::this_thread::get_id();
	section->details = section_details;
	section->start = time_now();

	auto section_idx = sections.size() - 1;
	task.section_indexes.push_back(section_idx);
	return section_idx;
}

void EndProfilerSection(ProfilerSectionIndex section_index) {
	std::lock_guard<std::mutex> guard(lock);
	if (section_index < sections.size() && sections[section_index].start != 0)
		sections[section_index].end = time_now();
}

void LockProfiler() { lock.lock(); }
void UnlockProfiler() { lock.unlock(); }

//
ProfilerPerfSection::ProfilerPerfSection(const std::string &task_name, const std::string &section_details) : section_index(BeginProfilerSection(task_name, section_details)) {}
ProfilerPerfSection::~ProfilerPerfSection() { EndProfilerSection(section_index); }

} // namespace hg
