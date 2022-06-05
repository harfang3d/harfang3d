// HARFANG(R) Copyright (C) 2021 Emmanuel Julien, NWNC HARFANG. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.

#pragma once

#include "foundation/time.h"

#include <thread>
#include <vector>
#include <string>

namespace hg {

struct ProfilerFrame {
	uint32_t frame;

	struct Section {
		Section() = default;
		Section(const Section &) = default;
		Section(Section &&s) : thread_id(s.thread_id), start(s.start), end(s.end), details(std::move(s.details)) {}

		std::thread::id thread_id;
		time_ns start{0}, end{0};
		std::string details;
	};

	struct Task {
		std::string name;
		time_ns duration;
		std::vector<size_t> section_indexes;
	};

	std::vector<Section> sections;
	std::vector<Task> tasks;

	time_ns start, end;
};

//
using ProfilerSectionIndex = size_t;

/// Begin a named profiler section. Call EndProfilerSection to end the section.
ProfilerSectionIndex BeginProfilerSection(const std::string &name, const std::string &section_details = {});
void EndProfilerSection(ProfilerSectionIndex section_index);

/// capture and end the current profiler frame
ProfilerFrame EndProfilerFrame();
/// capture current profiler frame without ending it
ProfilerFrame CaptureProfilerFrame();

void PrintProfilerFrame(const ProfilerFrame &frame);

//
class ProfilerPerfSection {
public:
	ProfilerPerfSection(const std::string &task_name, const std::string &section_details = {});
	~ProfilerPerfSection();
private:
	ProfilerSectionIndex section_index;
};

} // namespace hg
