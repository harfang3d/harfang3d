add_library(DummyVideoStream SHARED dummy_video_stream.cpp)
target_include_directories(DummyVideoStream PRIVATE ${CMAKE_SOURCE_DIR}/harfang/engine ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(DummyVideoStream PROPERTIES
	PREFIX ""
	FOLDER "harfang"
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data
	LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/data
)
target_link_libraries(DummyVideoStream PRIVATE foundation platform)
install(TARGETS DummyVideoStream
	RUNTIME DESTINATION cppsdk/bin/$<CONFIG>/data 
	LIBRARY DESTINATION cppsdk/bin/$<CONFIG>/data 
	COMPONENT cppsdk
)

set(TEST_FOUNDATION_SRCS
	foundation/cext.cpp
	foundation/curve.cpp
	foundation/pack_float.cpp
	foundation/math.cpp
	foundation/rand.cpp
	foundation/units.cpp
	foundation/string.cpp
	foundation/path_tools.cpp
	foundation/log.cpp
	foundation/vec2.cpp
	foundation/vec3.cpp
	foundation/vec4.cpp
	foundation/mat3.cpp
	foundation/mat4.cpp
	foundation/mat44.cpp
	foundation/color.cpp
	foundation/minmax.cpp
	foundation/plane.cpp
	foundation/projection.cpp
	foundation/obb.cpp
	foundation/easing.cpp
	foundation/quaternion.cpp
	foundation/frustum.cpp
	foundation/vector_list.cpp
	foundation/generational_vector_list.cpp
	foundation/intrusive_shared_ptr_st.cpp
	foundation/file.cpp
	foundation/dir.cpp
	foundation/data.cpp
	foundation/rw_interface.cpp
	foundation/data_rw_interface.cpp
	foundation/file_rw_interface.cpp
	foundation/clock.cpp
	foundation/cmd_line.cpp
	foundation/guid.cpp
	foundation/time.cpp
	foundation/version.cpp
	foundation/rect.cpp
	foundation/timer.cpp
	foundation/signal.cpp
)

set(TEST_ENGINE_SRCS
	engine/assets.cpp
	engine/animation.cpp
	engine/audio.cpp
	engine/meta.cpp
	engine/picture.cpp
	engine/video_stream.cpp
	engine/scene.cpp
)

set(TEST_SCRIPT_SRCS
	script/lua_vm.cpp
)

set(TEST_PLATFORM_SRCS
	platform/window.cpp
)

add_executable(tests main.cpp utils.cpp utils.h ${TEST_FOUNDATION_SRCS} ${TEST_ENGINE_SRCS} ${TEST_PLATFORM_SRCS} ${TEST_SCRIPT_SRCS})
target_link_libraries(tests PUBLIC engine foundation platform)
target_include_directories(tests PRIVATE ../../extern/acutest)
add_dependencies(tests DummyVideoStream)
set_target_properties(tests PROPERTIES FOLDER "harfang")
if(UNIX)
	target_link_libraries(tests PRIVATE pthread)
endif()
if(WIN32)
	set_target_properties(tests PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/cppsdk/bin/$<CONFIG>)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT tests)
endif()

install(TARGETS tests 
	RUNTIME DESTINATION cppsdk/bin/$<CONFIG>
	LIBRARY DESTINATION cppsdk/bin/$<CONFIG>
	COMPONENT cppsdk
)

install(DIRECTORY data DESTINATION cppsdk/bin/$<CONFIG>/ COMPONENT cppsdk)

if(HG_ENABLE_COVERAGE)
	set(COVERAGE_EXCLUDES
		"build*"
		"doc/*"
		"extern/*"
		"languages/*"
		"tests/*" 
		"tools/*"
		"/usr/*"
		"/c/msys64/*"
	)

	setup_target_for_coverage_lcov(
		NAME coverage
		BASE_DIR "${PROJECT_SOURCE_DIR}"
		EXECUTABLE tests)
endif()
