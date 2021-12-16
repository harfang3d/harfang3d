include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(platform)
add_subdirectory(foundation)
add_subdirectory(script)
add_subdirectory(engine)
if(HG_BUILD_TESTS)
	add_subdirectory(tests)
endif()

if(HG_BUILD_CPP_SDK)
	install(FILES version.txt DESTINATION cppsdk COMPONENT cppsdk)
endif(HG_BUILD_CPP_SDK)

if(HG_BUILD_DOCS)
	configure_file(doxyfile.tpl ${CMAKE_CURRENT_BINARY_DIR}/doxyfile @ONLY)
	add_custom_target(doc_cppsdk ALL
		${CMAKE_COMMAND} -E make_directory ${CMAKE_INSTALL_PREFIX}/cppsdk_docs
		COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doxyfile
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Generating CPPSDK documentation"
		VERBATIM
	)
endif()
