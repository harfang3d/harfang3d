
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

add_custom_command(
	OUTPUT
		${CMAKE_CURRENT_BINARY_DIR}/binding/bind.go
		${CMAKE_CURRENT_BINARY_DIR}/binding/wrapper.cpp
		${CMAKE_CURRENT_BINARY_DIR}/binding/wrapper.h
		${CMAKE_CURRENT_BINARY_DIR}/binding/fabgen.h
		${CMAKE_CURRENT_BINARY_DIR}/binding/go.mod
	COMMAND
		${Python3_EXECUTABLE} bind.py ${CMAKE_CURRENT_SOURCE_DIR}/../../binding/bind_harfang.py --go --out ${CMAKE_CURRENT_BINARY_DIR}/binding --doc_md_folder ${CMAKE_CURRENT_SOURCE_DIR}/../../doc/doc ${HG_BINDING_DEFINES}
	MAIN_DEPENDENCY
		${CMAKE_CURRENT_SOURCE_DIR}/../../binding/bind_harfang.py
	WORKING_DIRECTORY
		${HG_FABGEN_PATH}
	COMMENT
		"Generating Harfang binding for Go: go fabgen=${HG_FABGEN_PATH} srcdir=${CMAKE_SOURCE_DIR} dstdir=${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS ${static_libs})
    

add_library(hg_go STATIC 
	${CMAKE_CURRENT_BINARY_DIR}/binding/wrapper.cpp
	${CMAKE_CURRENT_BINARY_DIR}/binding/wrapper.h
	${CMAKE_CURRENT_BINARY_DIR}/binding/fabgen.h)

target_link_libraries(hg_go PUBLIC script engine foundation platform openxr)
set_target_properties(hg_go PROPERTIES FOLDER "harfang/languages")


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

add_dependencies(hg_go bind_hg_lua)
install_cppsdk_dependencies(hg_go go)

function(bundle_static_library tgt_name bundled_tgt_name)
  list(APPEND static_libs ${tgt_name})

  function(_recursively_collect_dependencies input_target)
    set(_input_link_libraries LINK_LIBRARIES)
    get_target_property(_input_type ${input_target} TYPE)
    if (${_input_type} STREQUAL "INTERFACE_LIBRARY")
      set(_input_link_libraries INTERFACE_LINK_LIBRARIES)
    endif()
    get_target_property(public_dependencies ${input_target} ${_input_link_libraries})
    foreach(dependency IN LISTS public_dependencies)
      if(TARGET ${dependency})
        get_target_property(alias ${dependency} ALIASED_TARGET)
        if (TARGET ${alias})
          set(dependency ${alias})
        endif()
        get_target_property(_type ${dependency} TYPE)
        if (${_type} STREQUAL "STATIC_LIBRARY")
          list(APPEND static_libs ${dependency})
        endif()

        get_property(library_already_added
          GLOBAL PROPERTY _${tgt_name}_static_bundle_${dependency})
        if (NOT library_already_added)
          set_property(GLOBAL PROPERTY _${tgt_name}_static_bundle_${dependency} ON)
          _recursively_collect_dependencies(${dependency})
        endif()
      endif()
    endforeach()
    set(static_libs ${static_libs} PARENT_SCOPE)
  endfunction()

  _recursively_collect_dependencies(${tgt_name})

  list(REMOVE_DUPLICATES static_libs)

  set(bundled_tgt_full_name ${CMAKE_BINARY_DIR}/${CMAKE_STATIC_LIBRARY_PREFIX}${bundled_tgt_name}${CMAKE_STATIC_LIBRARY_SUFFIX})
  set(bundled_tgt_full_name ${bundled_tgt_full_name} PARENT_SCOPE)

  if (CMAKE_CXX_COMPILER_ID MATCHES "^(Clang|GNU)$")
    file(WRITE ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in
      "CREATE ${bundled_tgt_full_name}\n" )
        
    foreach(tgt IN LISTS static_libs)
      file(APPEND ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in
        "ADDLIB $<TARGET_FILE:${tgt}>\n")
    endforeach()
    
    file(APPEND ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in "SAVE\n")
    file(APPEND ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in "END\n")

    file(GENERATE
      OUTPUT ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar
      INPUT ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar.in)

    set(ar_tool ${CMAKE_AR})
    if (CMAKE_INTERPROCEDURAL_OPTIMIZATION)
      set(ar_tool ${CMAKE_CXX_COMPILER_AR})
    endif()

    add_custom_command(
      COMMAND ${ar_tool} -M < ${CMAKE_BINARY_DIR}/${bundled_tgt_name}.ar
      OUTPUT ${bundled_tgt_full_name}
      COMMENT "Bundling ${bundled_tgt_name}"
      VERBATIM)
  elseif(MSVC)
    find_program(lib_tool lib)

    foreach(tgt IN LISTS static_libs)
      list(APPEND static_libs_full_names $<TARGET_FILE:${tgt}>)
    endforeach()

    add_custom_command(
      COMMAND ${lib_tool} /NOLOGO /OUT:${bundled_tgt_full_name} ${static_libs_full_names}
      OUTPUT ${bundled_tgt_full_name}
      COMMENT "Bundling ${bundled_tgt_name}"
      VERBATIM)
  else()
    message(FATAL_ERROR "Unknown bundle scenario!")
  endif()

  add_custom_target(bundling_target ALL DEPENDS ${bundled_tgt_full_name})
  add_dependencies(bundling_target ${tgt_name})

  add_library(${bundled_tgt_name} STATIC IMPORTED)
  set_target_properties(${bundled_tgt_name} 
    PROPERTIES 
      IMPORTED_LOCATION ${bundled_tgt_full_name}
      INTERFACE_INCLUDE_DIRECTORIES $<TARGET_PROPERTY:${tgt_name},INTERFACE_INCLUDE_DIRECTORIES>)
  add_dependencies(${bundled_tgt_name} bundling_target)
  set_target_properties(bundling_target PROPERTIES FOLDER "harfang/languages")

endfunction()

bundle_static_library(hg_go harfang)
install(FILES ${bundled_tgt_full_name} DESTINATION hg_go COMPONENT go)


set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS OFF)