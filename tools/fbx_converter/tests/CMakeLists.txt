add_executable(fbx_converter_tests main.cpp)
target_link_libraries(fbx_converter_tests PUBLIC engine PRIVATE tiny-process-library)
set_target_properties(fbx_converter_tests PROPERTIES FOLDER "harfang/tools" OUTPUT_NAME "tests")

if(WIN32)
	set_target_properties(fbx_converter_tests PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/fbx_converter)
	set_target_properties(fbx_converter_tests PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "-data_path ${CMAKE_CURRENT_SOURCE_DIR}/data")
endif()

install(TARGETS fbx_converter_tests RUNTIME DESTINATION fbx_converter COMPONENT fbx_converter)
install(DIRECTORY data DESTINATION fbx_converter)