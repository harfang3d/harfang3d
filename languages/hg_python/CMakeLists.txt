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

# package wheel on install
install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/bdist_wheel)" COMPONENT python)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bdist_wheel DESTINATION ${CMAKE_CURRENT_BINARY_DIR} COMPONENT python)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../harfang/version.txt DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/bdist_wheel COMPONENT python)
install(FILES __init__.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/bdist_wheel/harfang COMPONENT python)

install(
	TARGETS
		hg_python
	RUNTIME DESTINATION
		${CMAKE_CURRENT_BINARY_DIR}/bdist_wheel/harfang
	LIBRARY DESTINATION
		${CMAKE_CURRENT_BINARY_DIR}/bdist_wheel/harfang
	COMPONENT
		python)

install_cppsdk_dependencies(${CMAKE_CURRENT_BINARY_DIR}/bdist_wheel/harfang python)

if(WIN32)
	set(BDIST_WHEEL_LIMITED_API_FLAGS "") # broken feature on Windows
else()
	set(BDIST_WHEEL_LIMITED_API_FLAGS "--py-limited-api=cp32")
endif()

install(CODE "execute_process(
		COMMAND ${Python3_EXECUTABLE} setup.py bdist_wheel ${BDIST_WHEEL_LIMITED_API_FLAGS}
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bdist_wheel)"
	COMPONENT python)

if(WIN32)
	install(CODE "execute_process(
			COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/rename.py -src bdist_wheel/dist
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/)"
		 COMPONENT python)
endif()

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bdist_wheel/dist/ DESTINATION hg_python COMPONENT python)

install(
	CODE "file(GLOB HG_PYTHON_WHEEL \"${CMAKE_INSTALL_PREFIX}/hg_python/*.whl\")"
	CODE "execute_process(COMMAND ${Python3_EXECUTABLE} -m pip install \${HG_PYTHON_WHEEL} --upgrade --force-reinstall)"
	COMPONENT python)
