list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

set(FBXSDK_ROOT_DIR ${HG_FBX_SDK})
find_package(FBXSDK)

add_executable(fbx_converter fbx_converter.cpp)

target_compile_definitions(fbx_converter PUBLIC -DFBXSDK_SHARED)
target_include_directories(fbx_converter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} PRIVATE ${CMAKE_SOURCE_DIR}/extern/lua/src)
target_link_libraries(fbx_converter engine foundation platform fbxsdk)
set_target_properties(fbx_converter PROPERTIES FOLDER "harfang/tools")

add_dependencies(fbx_converter bind_hg_lua)

install(FILES $<TARGET_FILE:fbxsdk> DESTINATION fbx_converter COMPONENT fbx_converter)
install(TARGETS libluadll fbx_converter LIBRARY DESTINATION fbx_converter RUNTIME DESTINATION fbx_converter COMPONENT fbx_converter)

install(FILES $<TARGET_FILE:glfw> DESTINATION fbx_converter COMPONENT fbx_converter)

if(HG_ENABLE_OPENVR_API)
	install(FILES ${OPENVR_DLL} DESTINATION fbx_converter COMPONENT fbx_converter)
endif()

if(HG_ENABLE_SRANIPAL_API)
	install(FILES ${SRANIPAL_DLL} DESTINATION fbx_converter COMPONENT fbx_converter)
endif()

if(HG_BUILD_TESTS)
	add_subdirectory(tests)
endif()
