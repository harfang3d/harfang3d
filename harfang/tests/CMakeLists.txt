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

target_include_directories(tests PRIVATE ${CMAKE_BINARY_DIR}/binding)
set_target_properties(tests PROPERTIES FOLDER "harfang")

install(TARGETS tests 
	RUNTIME DESTINATION cppsdk/bin/$<CONFIG>
	LIBRARY DESTINATION cppsdk/bin/$<CONFIG>
	COMPONENT cppsdk
)

if(CMAKE_CXX_LINKER_SUPPORTS_PDB)
	install(FILES $<TARGET_PDB_FILE:tests> DESTINATION cppsdk/pdb OPTIONAL COMPONENT cppsdk)
endif()