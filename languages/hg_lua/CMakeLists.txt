add_custom_command(
	OUTPUT
		${CMAKE_CURRENT_BINARY_DIR}/binding/bind_Lua.cpp
		${CMAKE_CURRENT_BINARY_DIR}/binding/bind_Lua.h
		${CMAKE_CURRENT_BINARY_DIR}/binding/fabgen.h
	COMMAND
		${Python3_EXECUTABLE} bind.py ${CMAKE_CURRENT_SOURCE_DIR}/../../binding/bind_harfang.py --lua --out ${CMAKE_CURRENT_BINARY_DIR}/binding ${HG_BINDING_DEFINES}
	MAIN_DEPENDENCY
		${CMAKE_CURRENT_SOURCE_DIR}/../../binding/bind_harfang.py
	WORKING_DIRECTORY
		${HG_FABGEN_PATH}
	COMMENT
		"Generating Harfang binding for Lua extension")

add_library(hg_lua SHARED
	${CMAKE_CURRENT_BINARY_DIR}/binding/bind_Lua.cpp
	${CMAKE_CURRENT_BINARY_DIR}/binding/bind_Lua.h
	${CMAKE_CURRENT_BINARY_DIR}/binding/fabgen.h)
target_include_directories(hg_lua PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../extern/lua/src)
target_link_libraries(hg_lua engine foundation platform)
set_target_properties(hg_lua PROPERTIES OUTPUT_NAME "harfang" PREFIX "" DEBUG_POSTFIX "")
set_target_properties(hg_lua PROPERTIES FOLDER "harfang/languages")
add_dependencies(hg_lua lua)

if(WIN32)
	set_target_properties(hg_lua PROPERTIES COMPILE_FLAGS /bigobj)
elseif(UNIX)
	target_compile_definitions(hg_lua PUBLIC LUA_USE_LINUX)
endif()

install(TARGETS hg_lua lua RUNTIME DESTINATION hg_lua LIBRARY DESTINATION hg_lua COMPONENT lua)

install_cppsdk_dependencies(hg_lua lua)

foreach(_target assetc fbx_converter assimp_converter gltf_importer gltf_exporter)
	if(TARGET ${_target})
		install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}/hg_lua/harfang -DBUILD_TYPE=$<CONFIG> -P $<TARGET_PROPERTY:${_target},BINARY_DIR>/cmake_install.cmake)"
			COMPONENT lua)
	endif()
endforeach()

install(FILES bin.lua DESTINATION hg_lua/harfang COMPONENT lua)
