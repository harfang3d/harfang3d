if(NOT DEFINED HG_COMMIT_ID)
	if(NOT DEFINED GIT_EXECUTABLE)
		find_package(Git QUIET REQUIRED)
	endif()

	if(NOT DEFINED GIT_EXECUTABLE)
		message(FATAL_ERROR "Unable to determine GIT filepath.")
	endif()

	get_filename_component(GIT_EXECUTABLE ${GIT_EXECUTABLE} ABSOLUTE)

	if(DEFINED GIT_EXECUTABLE)
		execute_process(COMMAND
			"${GIT_EXECUTABLE}" rev-parse HEAD
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
		RESULT_VARIABLE exit_code
		OUTPUT_VARIABLE HG_COMMIT_ID
		ERROR_VARIABLE stderr
		OUTPUT_STRIP_TRAILING_WHITESPACE)

		if(NOT exit_code EQUAL 0)
			set(HG_COMMIT_ID "unknown")
		endif()
	else()
		set(HG_COMMIT_ID "unknown")
	endif()
endif()

configure_file(build_info.cpp.in ${CMAKE_CURRENT_SOURCE_DIR}/build_info.cpp)

set(HDRS
	ascii_encoder.h
	assert.h
	axis.h
	bit.h
	build_info.h
	byte_sort.h
	cext.h
	clock.h
	cmd_line.h
	color.h
	country.h
	curve.h
	data.h
	data_rw_interface.h
	dir.h
	easing.h
	endian.h
	file.h
	file_rw_interface.h
	format.h
	frustum.h
	generational_vector_list.h
	guid.h
	half_float.h
	intersection.h
	intrusive_shared_ptr_st.h
	kv_store.h
	log.h
	log_file.h
	math.h
	matrix3.h
	matrix4.h
	matrix44.h
	md5.h
	minmax.h
	murmur3.h
	named_parm_string.h
	obb.h
	pack_float.h
	parser.h
	path_tools.h
	plane.h
	profiler.h
	projection.h
	qmc.h
	quaternion.h
	rand.h
	file_rw_interface.h
	rect.h
	rect_packer.h
	ref_counted_vector_list.h
	rotation_order.h
	rw_interface.h
	seek_mode.h
	sha1.h
	signal.h
	smart_median_average.h
	string.h
	sysinfo.h
	thread.h
	time.h
	timer.h
	time_chrono.h
	time_to_string.h
	unit.h
	utf8.h
	vector2.h
	vector3.h
	vector4.h
	vector_list.h
	version.h
	xxhash.h)

set(SRCS
	ascii_encoder.cpp
	assert.cpp
	bit.cpp
	build_info.cpp
	clock.cpp
	cmd_line.cpp
	color.cpp
	country.cpp
	data.cpp
	data_rw_interface.cpp
	dir.cpp
	easing.cpp
	endian.cpp
	file.cpp
	file_rw_interface.cpp
	format.cpp
	frustum.cpp
	guid.cpp
	half_float.cpp
	intersection.cpp
	kv_store.cpp
	log.cpp
	log_file.cpp
	math.cpp
	matrix3.cpp
	matrix4.cpp
	matrix44.cpp
	md5.cpp
	minmax.cpp
	murmur3.cpp
	named_parm_string.cpp
	obb.cpp
	parser.cpp
	path_tools.cpp
	plane.cpp
	profiler.cpp
	projection.cpp
	qmc.cpp
	quaternion.cpp
	rand.cpp
	rw_interface.cpp
	sha1.cpp
	string.cpp
	sysinfo.cpp
	time.cpp
	timer.cpp
	time_chrono.cpp
	time_to_string.cpp
	utf8.cpp
	unit.cpp
	vector2.cpp
	vector3.cpp
	vector4.cpp
	version.cpp
	xxhash.c)

add_library(foundation STATIC ${SRCS} ${HDRS})
set_property(TARGET foundation PROPERTY PUBLIC_HEADER "${HDRS}")
set_target_properties(foundation PROPERTIES FOLDER "harfang")

if(NOT WIN32)
	target_link_libraries(foundation PUBLIC uuid)
else()
	target_link_libraries(foundation PUBLIC Iphlpapi)
	if(MSVC)
		target_compile_options(foundation PRIVATE "$<$<CONFIG:Release>:/Zi>")
		target_link_options(foundation PRIVATE "$<$<CONFIG:Release>:/DEBUG>")
	endif()
endif()

install_cppsdk_target(foundation)
