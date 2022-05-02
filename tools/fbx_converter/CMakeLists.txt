list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

set(FBXSDK_ROOT_DIR ${HG_FBX_SDK})

if(UNIX AND NOT APPLE)
	find_package(LibXml2 REQUIRED)
endif()

find_package(FBXSDK)

add_executable(fbx_converter fbx_converter.cpp)

target_compile_definitions(fbx_converter PUBLIC -DFBXSDK_SHARED)
target_include_directories(fbx_converter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} PRIVATE ${CMAKE_SOURCE_DIR}/extern/lua/src)
target_link_libraries(fbx_converter engine foundation platform fbxsdk)
set_target_properties(fbx_converter PROPERTIES FOLDER "harfang/tools")

add_dependencies(fbx_converter bind_hg_lua)

install(FILES $<TARGET_FILE:fbxsdk> DESTINATION fbx_converter COMPONENT fbx_converter)
install(TARGETS fbx_converter LIBRARY DESTINATION fbx_converter RUNTIME DESTINATION fbx_converter COMPONENT fbx_converter)

install_cppsdk_dependencies(fbx_converter)

if(HG_BUILD_TESTS)
	add_subdirectory(tests)
endif()
