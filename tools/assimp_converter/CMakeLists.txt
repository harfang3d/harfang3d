list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Assimp
add_definitions(-DASSIMP_BUILD_NO_PBRT_EXPORTER=1) # TODO: if this exported is needed, find a way to share the stb_image library
add_definitions(-DASSIMP_BUILD_NO_3MF_EXPORTER=1) # TODO: if this exported is needed, find a way to resolve conflicts with the miniz library
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(assimp EXCLUDE_FROM_ALL)
set_target_properties(assimp PROPERTIES FOLDER "harfang/tools/3rdparty")

if (TARGET zlibstatic)
	# So it seems assimp is building zlib static library.
	set_target_properties(zlibstatic PROPERTIES FOLDER "harfang/tools/3rdparty")
endif()

add_executable(assimp_converter assimp_converter.cpp fbx_optimizegraph.cpp fbx_optimizegraph.h)

target_include_directories(assimp_converter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} PRIVATE assimp/code PRIVATE assimp/include PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../extern/lua/src)
target_link_libraries(assimp_converter assimp engine)
set_target_properties(assimp_converter PROPERTIES FOLDER "harfang/tools")

# add_dependencies(assimp_converter bind_hg_lua)

install(FILES $<TARGET_FILE:assimp_converter> DESTINATION assimp_converter COMPONENT assimp_converter)
install(TARGETS assimp_converter
	RUNTIME DESTINATION assimp_converter
	LIBRARY DESTINATION assimp_converter
	COMPONENT assimp_converter
)

install_cppsdk_dependencies(assimp_converter)
install(TARGETS assimp_converter RUNTIME DESTINATION assimp_converter COMPONENT assimp_converter)
