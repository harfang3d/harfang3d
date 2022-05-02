set(HDRS
	shared.h)

set(SRCS
	main.cpp
	shared.cpp
	t_audio.cpp
	t_assets.cpp
	t_animation.cpp
	t_clock.cpp
	t_filters.cpp
	t_container.cpp
	t_curve.cpp
	t_dir.cpp
	t_font.cpp
	t_guid.cpp
	t_http.cpp
	t_imgui.cpp
	t_iso.cpp
	t_lua_vm.cpp
	t_meta.cpp
	t_mixer.cpp
	t_path_tools.cpp
	t_picture.cpp
	t_rect.cpp
	t_scene.cpp
	t_signal.cpp
	t_sort.cpp
	t_string.cpp
	t_time.cpp
	t_timer.cpp
	t_types.cpp
	t_vector.cpp
	t_version.cpp
	t_window.cpp
	t_video_stream.cpp
)

add_executable(tests ${SRCS} ${HDRS})
target_link_libraries(tests PUBLIC engine foundation platform PRIVATE gtest)

if(UNIX)
	target_link_libraries(tests PRIVATE pthread)
endif()

if(WIN32)
	set_target_properties(tests PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX})
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT tests)
endif()

add_library(DummyVideoStream SHARED dummy_video_stream.cpp)
target_include_directories(DummyVideoStream PRIVATE ${CMAKE_SOURCE_DIR}/harfang/engine ${CMAKE_CURRENT_BINARY_DIR})
set_property(TARGET DummyVideoStream PROPERTY PREFIX "")
target_link_libraries(DummyVideoStream PRIVATE foundation platform)
install(TARGETS DummyVideoStream
	RUNTIME DESTINATION cppsdk/bin/$<CONFIG>/data 
	LIBRARY DESTINATION cppsdk/bin/$<CONFIG>/data 
	COMPONENT cppsdk
)
set_target_properties(DummyVideoStream PROPERTIES FOLDER "harfang")

target_include_directories(tests PRIVATE ${CMAKE_BINARY_DIR}/binding)
set_target_properties(tests PROPERTIES FOLDER "harfang")

install(TARGETS tests 
	RUNTIME DESTINATION cppsdk/bin/$<CONFIG>
	LIBRARY DESTINATION cppsdk/bin/$<CONFIG>
	COMPONENT cppsdk
)
