file(WRITE ${CMAKE_BINARY_DIR}/tiny_gltf.cpp "#define TINYGLTF_IMPLEMENTATION\n#include \"${CMAKE_CURRENT_SOURCE_DIR}/tiny_gltf.h\"")
add_library(tiny_gltf OBJECT ${CMAKE_BINARY_DIR}/tiny_gltf.cpp tiny_gltf.h)
target_include_directories(tiny_gltf 
	PUBLIC 
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../extern/stb_image>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../extern/json>
)
set_target_properties(tiny_gltf PROPERTIES FOLDER "harfang/3rdparty")

function(gltf_build_tool name)
	add_executable(${name} ${name}.cpp)
	target_include_directories(${name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../extern/lua/src ${CMAKE_CURRENT_SOURCE_DIR}/../../extern/stb_image)
	target_link_libraries(${name} PUBLIC engine tiny_gltf)
	set_target_properties(${name} PROPERTIES FOLDER "harfang/tools")

	add_dependencies(${name} bind_hg_lua)

	install(TARGETS ${name} RUNTIME DESTINATION ${name} LIBRARY DESTINATION ${name} COMPONENT ${name})
	
	install_cppsdk_dependencies(${name})
endfunction()


if(HG_BUILD_GLTF_EXPORTER)
	gltf_build_tool(gltf_exporter)
endif()

if(HG_BUILD_GLTF_IMPORTER)
	gltf_build_tool(gltf_importer)
endif()
