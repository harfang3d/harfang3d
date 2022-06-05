add_custom_command(
	OUTPUT
		${CMAKE_CURRENT_BINARY_DIR}/binding/bind_CPython.cpp
		${CMAKE_CURRENT_BINARY_DIR}/binding/bind_CPython.h
		${CMAKE_CURRENT_BINARY_DIR}/binding/fabgen.h
	COMMAND
		${Python3_EXECUTABLE} bind.py ${CMAKE_CURRENT_SOURCE_DIR}/../../binding/bind_harfang.py --cpython --out ${CMAKE_CURRENT_BINARY_DIR}/binding --doc_md_folder ${CMAKE_CURRENT_SOURCE_DIR}/../../doc/doc ${HG_BINDING_DEFINES}
	MAIN_DEPENDENCY
		${CMAKE_CURRENT_SOURCE_DIR}/../../binding/bind_harfang.py
	WORKING_DIRECTORY
		${HG_FABGEN_PATH}
	COMMENT
		"Generating Harfang binding for CPython: python=${Python3_EXECUTABLE} fabgen=${HG_FABGEN_PATH} srcdir=${CMAKE_SOURCE_DIR} dstdir=${CMAKE_CURRENT_BINARY_DIR}")

add_library(hg_python SHARED
	${CMAKE_CURRENT_BINARY_DIR}/binding/bind_CPython.cpp
	${CMAKE_CURRENT_BINARY_DIR}/binding/bind_CPython.h
	${CMAKE_CURRENT_BINARY_DIR}/binding/fabgen.h)

if(WIN32)
	set_target_properties(hg_python PROPERTIES OUTPUT_NAME harfang SUFFIX .pyd)
	set_target_properties(hg_python PROPERTIES COMPILE_FLAGS /bigobj)
else()
	set_target_properties(hg_python PROPERTIES OUTPUT_NAME harfang PREFIX "")
endif()

message(STATUS "Python libs: " ${Python3_LIBRARIES})

if(WIN32)
	list(TRANSFORM Python3_LIBRARIES REPLACE "libs/python3[0-9]*" "libs/python3")  # since we use the limited API
	message(STATUS "Using limited API libs: " ${Python3_LIBRARIES})
endif()

target_compile_definitions(hg_python PUBLIC Py_LIMITED_API=0x03020000)
target_include_directories(hg_python PUBLIC ${Python3_INCLUDE_DIRS})
target_link_libraries(hg_python script engine foundation platform ${Python3_LIBRARIES})
set_target_properties(hg_python PROPERTIES FOLDER "harfang/languages")

if(HG_PYTHON_PIP)
	add_subdirectory(pip)
else()
	add_subdirectory(bdist_wheel)
endif()