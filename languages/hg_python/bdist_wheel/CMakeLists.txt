# package wheel on install
install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR})" COMPONENT python)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR} COMPONENT python)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../../harfang/version.txt DESTINATION ${CMAKE_CURRENT_BINARY_DIR} COMPONENT python)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../DESCRIPTION.rst DESTINATION ${CMAKE_CURRENT_BINARY_DIR} COMPONENT python)
install(FILES __init__.py DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/harfang COMPONENT python)

install(
	TARGETS
		hg_python
	RUNTIME DESTINATION
		${CMAKE_CURRENT_BINARY_DIR}/harfang
	LIBRARY DESTINATION
		${CMAKE_CURRENT_BINARY_DIR}/harfang
	COMPONENT
		python)

install_cppsdk_dependencies(${CMAKE_CURRENT_BINARY_DIR}/harfang python)

if(WIN32)
	set(BDIST_WHEEL_LIMITED_API_FLAGS "") # broken feature on Windows
else()
	set(BDIST_WHEEL_LIMITED_API_FLAGS "--py-limited-api=cp32")
endif()

install(CODE "execute_process(
		COMMAND ${Python3_EXECUTABLE} setup.py bdist_wheel ${BDIST_WHEEL_LIMITED_API_FLAGS}
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})"
	COMPONENT python)

if(WIN32)
	install(CODE "execute_process(
			COMMAND ${Python3_EXECUTABLE} rename.py -src dist
			WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})"
		 COMPONENT python)
endif()

install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/dist/ DESTINATION hg_python COMPONENT python)

install(
	CODE "file(GLOB HG_PYTHON_WHEEL \"${CMAKE_INSTALL_PREFIX}/hg_python/*.whl\")"
	CODE "execute_process(COMMAND ${Python3_EXECUTABLE} -m pip install \${HG_PYTHON_WHEEL} --upgrade --force-reinstall)"
	COMPONENT python)
