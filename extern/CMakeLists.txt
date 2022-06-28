function(install_cppsdk_external_target TARGET)
	if(NOT HG_BUILD_CPP_SDK)
		return()
	endif()

	if (DEFINED ARGV1)
		set(TargetHeaderPath ${ARGV1})
	else()
		set(TargetHeaderPath ${TARGET})
	endif()

	get_target_property(TargetType ${TARGET} TYPE)
	if ((TargetType STREQUAL "STATIC_LIBRARY") OR MSVC)
		set(LibraryDestination ${CppSdkLibraryDestination})
	else()
		set(LibraryDestination ${CppSdkRuntimeDestination})
	endif()

	install(
		TARGETS ${TARGET}
		EXPORT cppsdkTargets
		COMPONENT cppsdk
		PUBLIC_HEADER DESTINATION ${CppSdkHeadersDestination}/extern/${TargetHeaderPath}
		ARCHIVE	DESTINATION ${CppSdkLibraryDestination}
		LIBRARY	DESTINATION ${LibraryDestination}
		RUNTIME DESTINATION ${CppSdkRuntimeDestination}
		INCLUDES DESTINATION ${CppSdkHeadersDestination}/extern
	)
endfunction(install_cppsdk_external_target)

# glfw
if(HG_USE_GLFW)
	# determine glfw backend (as we do not have access to glfw_config.h).
	if(GLFW_USE_WAYLAND)
		set(HG_GLFW_BACKEND "WAYLAND")
	elseif (WIN32)
		set(HG_GLFW_BACKEND "WIN32")
	elseif (HG_APPLE)
		set(HG_GLFW_BACKEND "COCOA")
	elseif (UNIX)
		set(HG_GLFW_BACKEND "X11")
	else ()
		message(ERROR "GLFW: unknown or unsupported backed")
	endif()
	set(HG_GLFW_BACKEND ${HG_GLFW_BACKEND} PARENT_SCOPE)

	if(NOT DEFINED HG_REBUILD_GLFW)
		set(HG_REBUILD_GLFW ON)
	endif()

	if(NOT HG_REBUILD_GLFW)
		# use system package
		find_package(glfw3 3.2)

		if(glfw3_FOUND)
			# check that we are using the shared library version
			get_target_property(glfw_target_type glfw TYPE)
			if(glfw_target_type STREQUAL "SHARED_LIBRARY")
				set_target_properties(glfw PROPERTIES IMPORTED_GLOBAL TRUE)
				# we set the corresponding compile definition. This define will be exported in the CMake configuration files.
				target_compile_definitions(glfw INTERFACE GLFW_${HG_GLFW_BACKEND}=1)
			else()
				message(WARNING "Only the dynamic library version of GLFW is allowed")
				set(HG_REBUILD_GLFW ON)
			endif()
		else()
			message(WARNING "GLFW was not found")
			set(HG_REBUILD_GLFW ON)
		endif()
	endif()

	if(HG_REBUILD_GLFW)
		# build it from sources
		set(BUILD_SHARED_LIBS ON CACHE BOOL "Build shared libraries" FORCE)
		set(GLFW_INSTALL OFF CACHE BOOL "")
		set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
		set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
		set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
		add_subdirectory(glfw EXCLUDE_FROM_ALL)
		# here we completly hijack glfw shared library output name.
		# on *nix we don't want all the so names with versions (libgflw.so.3 libglfw.so.3.x etc...)
		set_property(TARGET glfw PROPERTY VERSION)
		set_property(TARGET glfw PROPERTY SOVERSION)
		set_property(TARGET glfw PROPERTY OUTPUT_NAME glfw3)
		set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
		install_cppsdk_external_target(glfw)
		set_target_properties(glfw PROPERTIES FOLDER "harfang/3rdparty")
		set_target_properties(glfw PROPERTIES
			INCLUDE_DIRECTORIES
		 		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/glfw/include>;${CMAKE_CURRENT_SOURCE_DIR}/glfw/src;${CMAKE_CURRENT_BINARY_DIR}/glfw/src"
			INTERFACE_INCLUDE_DIRECTORIES
				"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/glfw/include>"
		)
		# we set the corresponding compile definition. This define will be exported in the CMake configuration files.
		target_compile_definitions(glfw PUBLIC GLFW_${HG_GLFW_BACKEND}=1)
	endif()
else()
	set(HG_GLFW_BACKEND "NONE" PARENT_SCOPE)
endif()

# miniz
add_library(miniz STATIC miniz/miniz.c miniz/miniz.h)
set_target_properties(miniz PROPERTIES FOLDER "harfang/3rdparty")
target_include_directories(miniz PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/miniz> )
install_cppsdk_external_target(miniz)

# bgfx
add_subdirectory(bgfx EXCLUDE_FROM_ALL)

install_cppsdk_external_target(bx)
install_cppsdk_external_target(bimg)
install_cppsdk_external_target(bgfx)
install_cppsdk_external_target(astc-codec)
install_cppsdk_external_target(astc)
install_cppsdk_external_target(edtaa3)
install_cppsdk_external_target(etc1)
install_cppsdk_external_target(etc2)
install_cppsdk_external_target(iqa)
install_cppsdk_external_target(squish)
install_cppsdk_external_target(nvtt)
install_cppsdk_external_target(pvrtc)

# Lua
if(UNIX AND NOT APPLE)
	# lua links against libreadline but does not check for its availability.
	find_package(readline REQUIRED)
endif()
add_subdirectory(lua EXCLUDE_FROM_ALL)
set_property(TARGET lua luac liblua libluadll PROPERTY FOLDER "harfang/3rdparty/lua")
if(HG_BUILD_CPP_SDK)
	set_target_properties(libluadll PROPERTIES PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/lua/src/lua.h;${CMAKE_CURRENT_SOURCE_DIR}/lua/src/luaconf.h;${CMAKE_CURRENT_SOURCE_DIR}/lua/src/lualib.h;${CMAKE_CURRENT_SOURCE_DIR}/lua/src/lauxlib.h;${CMAKE_CURRENT_SOURCE_DIR}/lua/src/lua.hpp")
endif()
target_include_directories(libluadll PUBLIC $<INSTALL_INTERFACE:${CppSdkHeadersDestination}/extern/lua>)

install_cppsdk_external_target(libluadll lua)

# gtest
if(HG_BUILD_TESTS)
	set(gtest_force_shared_crt ON CACHE BOOL "googletest: Force shared CRT")

	add_subdirectory(googletest EXCLUDE_FROM_ALL)
	set_target_properties(gtest PROPERTIES FOLDER "harfang/3rdparty")
endif(HG_BUILD_TESTS)

# STB vorbis (customized to handle raw VORBIS stream)
add_library(stb_vorbis STATIC stb_vorbis/stb_vorbis.c)
set_target_properties(stb_vorbis PROPERTIES FOLDER "harfang/3rdparty")
target_compile_definitions(stb_vorbis PUBLIC STB_VORBIS_NO_STDIO=1)
target_include_directories(stb_vorbis PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/stb_vorbis>)

install_cppsdk_external_target(stb_vorbis)

# STB truetype
add_library(stb_truetype STATIC stb_truetype/stb_truetype.c stb_truetype/stb_truetype.h)
target_include_directories(stb_truetype PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/stb_truetype>)
set_target_properties(stb_truetype PROPERTIES FOLDER "harfang/3rdparty")

install_cppsdk_external_target(stb_truetype)

# STB image write
add_library(stb_image_write STATIC stb_image/stb_image_write.c stb_image/stb_image_write.h)
target_include_directories(stb_image_write PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/stb_image>)
set_target_properties(stb_image_write PROPERTIES FOLDER "harfang/3rdparty")

install_cppsdk_external_target(stb_image_write)

# Bullet 3
if(HG_ENABLE_BULLET3_SCENE_PHYSICS)
	add_subdirectory(bullet3 EXCLUDE_FROM_ALL)

	set_target_properties(BulletDynamics PROPERTIES FOLDER "harfang/3rdparty/bullet3")
	set_target_properties(BulletCollision PROPERTIES FOLDER "harfang/3rdparty/bullet3")
	set_target_properties(BulletCommon PROPERTIES FOLDER "harfang/3rdparty/bullet3")
	set_target_properties(LinearMath PROPERTIES FOLDER "harfang/3rdparty/bullet3")

	install_cppsdk_external_target(BulletDynamics)
	install_cppsdk_external_target(BulletCollision)
	install_cppsdk_external_target(BulletCommon)
	install_cppsdk_external_target(LinearMath)
endif()

# imgui
add_library(imgui STATIC
	imgui/misc/cpp/imgui_stdlib.cpp
	imgui/imgui.cpp
	imgui/imgui_draw.cpp
	imgui/imgui_tables.cpp
	imgui/imgui_widgets.cpp
	imgui/imgui.h
	imgui/imgui_internal.h
	imgui/imconfig.h)
target_include_directories(imgui
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/imgui>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../harfang>
		$<INSTALL_INTERFACE:${CppSdkHeadersDestination}/extern/imgui>
)
set_target_properties(imgui PROPERTIES FOLDER "harfang/3rdparty")

install_cppsdk_external_target(imgui)
if(HG_BUILD_CPP_SDK)
	install(DIRECTORY imgui DESTINATION cppsdk/include/extern FILES_MATCHING PATTERN "*.h" PATTERN "fonts" EXCLUDE)
endif()

# OpenAL soft
message("Configuring OpenAL soft")

set(ALSOFT_UTILS OFF CACHE BOOL "OpenAL soft: Build and install utility programs")
set(ALSOFT_EXAMPLES OFF CACHE BOOL "OpenAL soft: Build and install example programs")
set(ALSOFT_TESTS OFF CACHE BOOL "OpenAL soft: Build and install test programs")
set(ALSOFT_CONFIG OFF CACHE BOOL "OpenAL soft: Install alsoft.conf sample configuration file")
set(ALSOFT_INSTALL OFF CACHE BOOL "OpenAL soft: Install headers and libraries")
set(LIBTYPE STATIC)  # build as static library
add_subdirectory(openal-soft EXCLUDE_FROM_ALL)
target_include_directories(OpenAL PRIVATE openal-soft-hrtf)
target_include_directories(OpenAL PUBLIC $<BUILD_INTERFACE:${OpenAL_SOURCE_DIR}/include>)
target_compile_definitions(OpenAL PUBLIC ALSOFT_EMBED_HRTF_DATA=1 AL_LIBTYPE_STATIC=1) # make sure to correctly propagate this flag to consuming librairies
set_target_properties(OpenAL PROPERTIES FOLDER "harfang/3rdparty")

# ... try to fix include directories
get_target_property(OpenAL_include_dirs OpenAL INTERFACE_INCLUDE_DIRECTORIES)
list(REMOVE_ITEM OpenAL_include_dirs "$<INSTALL_INTERFACE:include>")
set_property(TARGET OpenAL PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${OpenAL_include_dirs})

install_cppsdk_external_target(OpenAL)

# json.hpp
add_library(jsonhpp INTERFACE)
target_include_directories(jsonhpp
	INTERFACE
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/json>
		$<INSTALL_INTERFACE:${CppSdkHeadersDestination}/extern/json>
)
set_target_properties(jsonhpp PROPERTIES PUBLIC_HEADER "json/json.hpp;json/json_fwd.hpp")

install_cppsdk_external_target(jsonhpp json)

# tiny-process-library
add_subdirectory(tiny-process-library EXCLUDE_FROM_ALL)
target_include_directories(tiny-process-library INTERFACE tiny-process-library)
target_compile_definitions(tiny-process-library PUBLIC UNICODE=1)
set_target_properties(tiny-process-library PROPERTIES FOLDER "harfang/3rdparty")

# OpenVR
if(HG_ENABLE_OPENVR_API)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(_OPENVR_BIT 64)
	else()
		set(_OPENVR_BIT 32)
	endif()

	if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
		set(_OPENVR_ARCH "linux${_OPENVR_BIT}")
	elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
		set(_OPENVR_ARCH "win${_OPENVR_BIT}")
	elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")					# OpenVR only supports 32 bits osx...
		set(_OPENVR_ARCH "osx32")
	else(HG_ENABLE_OPENVR_API)
		message(WARNING "OpenVR: unsupported platform ${CMAKE_SYSTEM_NAME}")
		set(HG_ENABLE_OPENVR_API OFF)
	endif()

	if(HG_ENABLE_OPENVR_API)
		set(OPENVR_INCLUDE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/openvr/headers)
		find_library(OPENVR_LIBRARY name openvr_api PATHS ${CMAKE_CURRENT_SOURCE_DIR}/openvr/lib/${_OPENVR_ARCH} NO_DEFAULT_PATH)

		add_library(OpenVR SHARED IMPORTED GLOBAL)
		if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
			set(OPENVR_DLL ${CMAKE_CURRENT_SOURCE_DIR}/openvr/bin/${_OPENVR_ARCH}/openvr_api.dll)
			set(OPENVR_DLL ${OPENVR_DLL} PARENT_SCOPE)
			set_target_properties(OpenVR PROPERTIES
				IMPORTED_IMPLIB ${OPENVR_LIBRARY}
				IMPORTED_LOCATION ${OPENVR_DLL}
				INTERFACE_INCLUDE_DIRECTORIES ${OPENVR_INCLUDE_PATH}
			)
			if(HG_BUILD_CPP_SDK)
				install(FILES ${OPENVR_DLL} DESTINATION ${CppSdkRuntimeDestination} COMPONENT cppsdk)
			endif()
		else()
			set_target_properties(OpenVR PROPERTIES 
				IMPORTED_LOCATION ${OPENVR_LIBRARY}
				IMPORTED_NO_SONAME TRUE 
				INTERFACE_INCLUDE_DIRECTORIES ${OPENVR_INCLUDE_PATH}
			)
		endif()

		if(HG_BUILD_CPP_SDK)
			install(FILES ${OPENVR_LIBRARY} DESTINATION ${CppSdkLibraryDestination} COMPONENT cppsdk)
		endif()
	endif(HG_ENABLE_OPENVR_API)

	unset(_OPENVR_BIT)
	unset(_OPENVR_ARCH)
endif()

# SRanipal
if(HG_ENABLE_SRANIPAL_API)
	# [todo] linux
	find_library(SRANIPAL_LIBRARY name SRanipal PATHS ${CMAKE_CURRENT_SOURCE_DIR}/sranipal/lib NO_DEFAULT_PATH)
	set(SRANIPAL_DLL 
			${CMAKE_CURRENT_SOURCE_DIR}/sranipal/bin/libHTC_License.dll 
			${CMAKE_CURRENT_SOURCE_DIR}/sranipal/bin/nanomsg.dll
			${CMAKE_CURRENT_SOURCE_DIR}/sranipal/bin/SRanipal.dll
			${CMAKE_CURRENT_SOURCE_DIR}/sranipal/bin/SRWorks_Log.dll
			${CMAKE_CURRENT_SOURCE_DIR}/sranipal/bin/ViveSR_Client.dll)

	set(SRANIPAL_DLL ${SRANIPAL_DLL} PARENT_SCOPE)

	add_library(SRanipal SHARED IMPORTED GLOBAL)
	set_target_properties(SRanipal PROPERTIES
		IMPORTED_IMPLIB ${CMAKE_CURRENT_SOURCE_DIR}/sranipal/lib/SRanipal.lib
		IMPORTED_LOCATION "${SRANIPAL_DLL}"
		INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/sranipal/include
	)

	if(HG_BUILD_CPP_SDK)
		install(FILES ${SRANIPAL_DLL} DESTINATION ${CppSdkRuntimeDestination} COMPONENT cppsdk)
		install(FILES ${SRANIPAL_LIBRARY} DESTINATION ${CppSdkLibraryDestination} COMPONENT cppsdk)
	endif()
endif()

# RecastDetour
if(HG_ENABLE_RECAST_DETOUR_API)
	set(RECASTNAVIGATION_DEMO OFF CACHE BOOL "Build demo")
	set(RECASTNAVIGATION_TESTS OFF CACHE BOOL "Build tests")
	set(RECASTNAVIGATION_EXAMPLES OFF CACHE BOOL "Build examples")
	add_subdirectory(recastnavigation EXCLUDE_FROM_ALL)
	set_property(TARGET Recast Detour DetourCrowd PROPERTY FOLDER "harfang/3rdparty/Detour")

	install_cppsdk_external_target(Recast)
	install_cppsdk_external_target(Detour)
	install_cppsdk_external_target(DetourCrowd)
endif()

# CMFT
add_executable(cmft
	cmft/src/cmft/common/stb_image.cpp
	cmft/src/cmft/common/print.cpp
	cmft/src/cmft/allocator.cpp
	cmft/src/cmft/clcontext.cpp
	cmft/src/cmft/cubemapfilter.cpp
	cmft/src/cmft/image.cpp
	cmft/src/main.cpp)
target_include_directories(cmft PRIVATE cmft/include cmft/dependency cmft/src/cmft)
target_compile_definitions(cmft PRIVATE CMFT_ENABLE_INFO_MESSAGES=1)
set_target_properties(cmft PROPERTIES FOLDER "harfang/tools")

if(UNIX)
	target_compile_definitions(cmft PRIVATE linux)
	target_link_libraries(cmft pthread dl)
endif()

# meshoptimizer
set(CMAKE_INSTALL_INCLUDEDIR ${CppSdkHeadersDestination}/extern)
add_subdirectory(meshoptimizer EXCLUDE_FROM_ALL)
set_target_properties(meshoptimizer PROPERTIES FOLDER "harfang/3rdparty")
install_cppsdk_external_target(meshoptimizer)

# mikktspace
add_library(mikktspace STATIC mikktspace/mikktspace.c mikktspace/mikktspace.h)
target_include_directories(mikktspace PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/mikktspace>)
set_target_properties(mikktspace PROPERTIES FOLDER "harfang/3rdparty")
install_cppsdk_external_target(mikktspace)