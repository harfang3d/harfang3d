
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

add_custom_command(
	OUTPUT
		${CMAKE_CURRENT_BINARY_DIR}/binding/bind.go
		${CMAKE_CURRENT_BINARY_DIR}/binding/wrapper.cpp
		${CMAKE_CURRENT_BINARY_DIR}/binding/wrapper.h
		${CMAKE_CURRENT_BINARY_DIR}/binding/fabgen.h
		${CMAKE_CURRENT_BINARY_DIR}/binding/go.mod
	COMMAND
		${Python3_EXECUTABLE} bind.py ${CMAKE_CURRENT_SOURCE_DIR}/../../binding/bind_harfang.py --go --out ${CMAKE_CURRENT_BINARY_DIR}/binding ${HG_BINDING_DEFINES}
	MAIN_DEPENDENCY
		${CMAKE_CURRENT_SOURCE_DIR}/../../binding/bind_harfang.py
	WORKING_DIRECTORY
		${HG_FABGEN_PATH}
	COMMENT
		"Generating Harfang binding for Go: go fabgen=${HG_FABGEN_PATH} srcdir=${CMAKE_SOURCE_DIR} dstdir=${CMAKE_CURRENT_BINARY_DIR}")

add_library(hg_go SHARED
	${CMAKE_CURRENT_BINARY_DIR}/binding/wrapper.cpp
	${CMAKE_CURRENT_BINARY_DIR}/binding/wrapper.h
	${CMAKE_CURRENT_BINARY_DIR}/binding/fabgen.h)
	
target_link_libraries(hg_go script engine foundation platform)
set_target_properties(hg_go PROPERTIES FOLDER "harfang/languages")
set_target_properties(hg_go PROPERTIES OUTPUT_NAME "harfang" ) 

install(TARGETS hg_go 
	RUNTIME DESTINATION hg_go
	LIBRARY DESTINATION hg_go
	ARCHIVE DESTINATION hg_go
	COMPONENT go
)

install(FILES 
		${CMAKE_CURRENT_BINARY_DIR}/binding/go.mod
		${CMAKE_CURRENT_BINARY_DIR}/binding/wrapper.h
		${CMAKE_CURRENT_BINARY_DIR}/binding/fabgen.h
		${CMAKE_CURRENT_BINARY_DIR}/binding/bind.go
	DESTINATION hg_go
	COMPONENT go
)

install_cppsdk_dependencies(hg_go go)

set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS OFF)