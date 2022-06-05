install(
	TARGETS
		hg_python
	RUNTIME DESTINATION
		${CMAKE_INSTALL_PREFIX}
	LIBRARY DESTINATION
		${CMAKE_INSTALL_PREFIX}
	COMPONENT
		python)

install_cppsdk_dependencies(${CMAKE_INSTALL_PREFIX} python)

if(WIN32)
	set(BDIST_WHEEL_LIMITED_API_FLAGS "") # broken feature on Windows
else()
	set(BDIST_WHEEL_LIMITED_API_FLAGS "--py-limited-api=cp32")
endif()
