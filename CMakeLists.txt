cmake_minimum_required(VERSION 3.16.2)

include(CMakePackageConfigHelpers) 
include(CMakeDependentOption)

file(READ harfang/version.txt HG_VERSION)
string(STRIP ${HG_VERSION} HG_VERSION)

project(harfang 
	VERSION ${HG_VERSION}
	LANGUAGES CXX C
	HOMEPAGE_URL "harfang3D.com"
)

set(CMAKE_CXX_STANDARD 14)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/harfang/cmake")

# -----------------------------------------------------------------------------
# Debug/tooling options
# -----------------------------------------------------------------------------
option(HG_ENABLE_TSA "Harfang: Enable TSA" OFF)
option(HG_ENABLE_TSAN "Harfang: Enable TSAN" OFF)
option(HG_ENABLE_ASAN "Harfang: Enable ASAN" OFF)

# -----------------------------------------------------------------------------
# Build options
# -----------------------------------------------------------------------------
option(HG_BUILD_ASSIMP_CONVERTER "Harfang: Build Assimp converter" ON)
option(HG_BUILD_FBX_CONVERTER "Harfang: Build FBX converter" ON)
option(HG_BUILD_GLTF_EXPORTER "Harfang: Build GLTF exporter" ON)
option(HG_BUILD_GLTF_IMPORTER "Harfang: Build GLTF importer" ON)
option(HG_BUILD_ASSETC "Harfang: Build AssetC" ON)

option(HG_BUILD_CPP_SDK "Harfang: Build C++ SDK" OFF)
option(HG_BUILD_TESTS "Harfang: Build Unit tests" OFF)

option(HG_BUILD_DOCS "Harfang: Build documentation" OFF)

option(HG_BUILD_HG_LUA "Harfang: Build Harfang Lua extension" OFF)
option(HG_BUILD_HG_PYTHON "Harfang: Build Harfang CPython 3.2+ extension" OFF)
option(HG_BUILD_HG_GO "Harfang: Build Harfang Go extension" OFF)

option(HG_USE_GLFW "Harfang: Use GLFW backend" ON)

option(HG_ENABLE_BULLET3_SCENE_PHYSICS "Harfang: Scene physics using Bullet Dynamics" ON)
option(HG_ENABLE_RECAST_DETOUR_API "Harfang: Enable Recast/Detour API" ON)

option(HG_ENABLE_OPENVR_API "Harfang: Enable OpenVR API" OFF)
cmake_dependent_option(HG_ENABLE_SRANIPAL_API "Harfang: Enable VIVE Eye and Facial Tracking SDK (SRanipal) API" OFF "HG_ENABLE_OPENVR_API;MSVC" OFF)

set(HG_FABGEN_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../fabgen" CACHE PATH "Harfang: Path to FabGen script installation")
get_filename_component(HG_FABGEN_PATH ${HG_FABGEN_PATH} REALPATH)
message(CHECK_START "Checking FabGen path")
if(EXISTS ${HG_FABGEN_PATH})
	message(CHECK_PASS "ok")
else()
	message(FATAL_ERROR "unable to access ${HG_FABGEN_PATH}")
endif()

# -----------------------------------------------------------------------------
# Configuration constraints
# -----------------------------------------------------------------------------
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
	message(STATUS "! Forcing build type to RelWithDebInfo since CMAKE_BUILD_TYPE is not set")
	SET(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm") 
	message(STATUS "! Forcing embedded build since system processor is 'arm'")
	set(HG_EMBEDDED ON FORCE)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
	message(STATUS "! Disabling GLFW since building for Emscripten")
	set(HG_USE_GLFW OFF FORCE)
endif()

if(HG_ENABLE_SRANIPAL_API AND MSVC)
	if (NOT HG_ENABLE_OPENVR_API)
		message(STATUS "! Enabling OpenVR API")
		set(HG_ENABLE_OPENVR_API ON FORCE)
	endif()
endif()

# -----------------------------------------------------------------------------
# Configuration flags
# -----------------------------------------------------------------------------
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	message(STATUS "Building for 64-bit")
	add_definitions(-DMARCH_WORD_SIZE=64)
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
	message(STATUS "Building for 32-bit")
	add_definitions(-DMARCH_WORD_SIZE=32)
endif()

if(MSVC)
	add_compile_options(/MP /fp:fast /permissive-) # use /GL for LTGC
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS _ENABLE_EXTENDED_ALIGNED_STORAGE)
	#add_compile_options(/Od /Zi /Zo)
	add_link_options(/DEBUG)
	#add_link_options(/LTCG)
elseif(ANDROID)
	add_compile_definitions(ANDROID)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
	message(STATUS "Emscripten sets")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --bind -fPIC -DEMSCRIPTEN -s USE_SDL=2 -s USE_WEBGL2=1 -s FULL_ES3=1 -s ALLOW_MEMORY_GROWTH=1 -s OFFSCREENCANVAS_SUPPORT=1 -s LLD_REPORT_UNDEFINED -s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=1 -s FORCE_FILESYSTEM=1 -DBGFX_CONFIG_RENDERER_OPENGLES_MIN_VERSION=30")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --bind -fPIC -DEMSCRIPTEN -s USE_SDL=2 -s USE_WEBGL2=1 -s FULL_ES3=1 -s ALLOW_MEMORY_GROWTH=1 -s OFFSCREENCANVAS_SUPPORT=1 -s LLD_REPORT_UNDEFINED -s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=1 -s FORCE_FILESYSTEM=1 -DBGFX_CONFIG_RENDERER_OPENGLES_MIN_VERSION=30")

	# without pthread and with wasm
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -s WASM=1 -Oz -DNDEBUG -s DISABLE_EXCEPTION_CATCHING=0 -s FORCE_FILESYSTEM=1")
	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS} -s WASM=1 -Oz -DNDEBUG -s DISABLE_EXCEPTION_CATCHING=0 -s FORCE_FILESYSTEM=1")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -s WASM=1 -O0 -DNDEBUG -g4 -s ASSERTIONS=2 -s DEMANGLE_SUPPORT=1 -s FORCE_FILESYSTEM=1")
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -s WASM=1 -O0 -DNDEBUG -g4 -s ASSERTIONS=2 -s DEMANGLE_SUPPORT=1 -s FORCE_FILESYSTEM=1")	
else()
	set(CMAKE_POSITION_INDEPENDENT_CODE ON)
	if(NOT APPLE)
		set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
		set(CMAKE_SKIP_BUILD_RPATH  FALSE)
		set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
		set(CMAKE_INSTALL_RPATH $ORIGIN)
	endif()
	# gcc 7.4.0 + shaderc workaround ...
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		if(CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL "7.4.0")
			set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "" FORCE)
			set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "" FORCE)
		endif()
	endif()
endif()

if(HG_ENABLE_TSA)
	message(STATUS "Building with thread safety analysis")
	add_compile_options(-Wthread-safety)
endif()

if(HG_ENABLE_ASAN)
	message(STATUS "Building with address-sanitizer support")
	add_compile_options(-fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls -fno-limit-debug-info)
	add_link_options(-fsanitize=address)
endif()

if(HG_ENABLE_TSAN)
	message(STATUS "Building with thread-sanitizer support")
	add_compile_options(-fsanitize=thread -fno-omit-frame-pointer -fno-optimize-sibling-calls -fno-limit-debug-info)
	add_link_options(-fsanitize=thread)
endif()

if(MSVC)
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /Od /Ob0 /Zi")
	set(CMAKE_RELWITHDEBINFO_POSTFIX "")
else()
	set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O0")
endif()

if(HG_EMBEDDED) 
	add_compile_definitions(HG_EMBEDDED)
endif()

if(HG_ENABLE_OPENVR_API)
	add_compile_definitions(HG_ENABLE_OPENVR_API)
endif()

if(HG_ENABLE_BULLET3_SCENE_PHYSICS)
	add_compile_definitions(HG_ENABLE_BULLET3_SCENE_PHYSICS)
endif()

if(HG_ENABLE_SRANIPAL_API)
	add_compile_definitions(HG_ENABLE_SRANIPAL_API)
endif()

if(HG_ENABLE_RECAST_DETOUR_API)
	add_compile_definitions(HG_ENABLE_RECAST_DETOUR_API)
endif()

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------
if(NOT Python3_FOUND)
	if(HG_BUILD_HG_PYTHON)
		find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
	else()
		find_package(Python3 COMPONENTS Interpreter REQUIRED)
	endif()
endif()

if(HG_BUILD_DOCS)
	find_package(Doxygen REQUIRED)
endif()

# -----------------------------------------------------------------------------
# System configuration
# -----------------------------------------------------------------------------
if(NOT HG_HOST_PREFIX)
	string(TOLOWER ${CMAKE_HOST_SYSTEM_NAME} HG_HOST_PREFIX)
	if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "amd64.*|x86_64.*|AMD64.*")
		string(APPEND HG_HOST_PREFIX "-x64")
	elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "i686.*|i386.*|x86.*")
		string(APPEND HG_HOST_PREFIX "-x86")
	elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(aarch64.*|AARCH64.*)")
		string(APPEND HG_HOST_PREFIX "-aarch64")
	elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(arm.*|ARM.*)")
		string(APPEND HG_HOST_PREFIX "-arm")
	endif()
endif(NOT HG_HOST_PREFIX)

if(NOT HG_TARGET_PREFIX)
	string(TOLOWER ${CMAKE_SYSTEM_NAME} HG_TARGET_PREFIX)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		if(CMAKE_SYSTEM_PROCESSOR MATCHES "amd64.*|x86_64.*|AMD64.*")
			string(APPEND HG_TARGET_PREFIX "-x64")
		elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(aarch64.*|AARCH64.*)")
			string(APPEND HG_TARGET_PREFIX "-aarch64")
		endif()
	elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
		if(CMAKE_SYSTEM_PROCESSOR MATCHES "i686.*|i386.*|x86.*|amd64.*|AMD64.*")
			string(APPEND HG_TARGET_PREFIX "-x86")
		elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm.*|ARM.*)")
			string(APPEND HG_TARGET_PREFIX "-arm")
		endif()
	endif()
endif(NOT HG_TARGET_PREFIX)

get_directory_property(_hasParent PARENT_DIRECTORY)
if(_hasParent)	# after 3.21 use if(NOT PROJECT_IS_TOP_LEVEL)
	set(HG_HOST_PREFIX ${HG_TARGET_PREFIX} PARENT_SCOPE)
	set(HG_TARGET_PREFIX ${HG_TARGET_PREFIX} PARENT_SCOPE)
	set(HG_GRAPHIC_API ${HG_GRAPHIC_API} PARENT_SCOPE)
endif()
unset(_hasParent)

if(NOT HG_GRAPHIC_API)
	if(MSVC)
		set(HG_GRAPHIC_API "DX11")
	elseif(NOT HG_GRAPHC_API)
		set(HG_GRAPHIC_API "GL")
	endif()
endif(NOT HG_GRAPHIC_API)

add_compile_definitions(HG_GRAPHIC_API="${HG_GRAPHIC_API}")

# -----------------------------------------------------------------------------
# Install helper
# -----------------------------------------------------------------------------
set(CppSdkLibraryDestination cppsdk/lib/$<CONFIG>)
set(CppSdkRuntimeDestination cppsdk/bin/$<CONFIG>)
set(CppSdkHeadersDestination cppsdk/include)

# helper function for cppsdk library install.
function(install_cppsdk_target TARGET)
	if(NOT HG_BUILD_CPP_SDK)
		return()
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
		PUBLIC_HEADER DESTINATION ${CppSdkHeadersDestination}/${TARGET}
		ARCHIVE	DESTINATION ${CppSdkLibraryDestination}
		LIBRARY	DESTINATION ${LibraryDestination}
		RUNTIME DESTINATION ${CppSdkRuntimeDestination}
		INCLUDES DESTINATION ${CppSdkHeadersDestination}
	)
	if(CMAKE_CXX_LINKER_SUPPORTS_PDB)
		if(TargetType STREQUAL "STATIC_LIBRARY")
			install(DIRECTORY $<TARGET_PROPERTY:${TARGET},BINARY_DIR>/$<CONFIG>/ DESTINATION cppsdk/pdb OPTIONAL COMPONENT cppsdk FILES_MATCHING PATTERN "*.pdb")
		else()
			install(FILES $<TARGET_PDB_FILE:${TARGET}> DESTINATION cppsdk/pdb OPTIONAL COMPONENT cppsdk)
		endif()
	endif()
endfunction(install_cppsdk_target)

# Helper function to deploy shared libraries of CPPSDK external dependencies.
# This function is used only when building CPPSDK.
# Its "SDK" counterpart is defined in harfangConfig.cmake.
function(install_cppsdk_dependencies Destination)
	if (DEFINED ARGV1)
		set(ComponentName ${ARGV1})
	else()
		set(ComponentName ${Destination})
	endif()

	if(HG_USE_GLFW)
		install(FILES $<TARGET_FILE:glfw> DESTINATION ${Destination} COMPONENT ${ComponentName})
	endif()

	install(FILES $<TARGET_FILE:libluadll> DESTINATION ${Destination} COMPONENT ${ComponentName})
	if(HG_ENABLE_OPENVR_API)
		get_target_property(OpenVRTargetFiles OpenVR IMPORTED_LOCATION)
		install(FILES ${OpenVRTargetFiles} DESTINATION ${Destination} COMPONENT ${ComponentName})
	endif()

	if(HG_ENABLE_SRANIPAL_API)
		get_target_property(SRanipalTargetFiles SRanipal IMPORTED_LOCATION)
		install(FILES ${SRanipalTargetFiles} DESTINATION ${Destination} COMPONENT ${ComponentName})
	endif()
endfunction()

# -----------------------------------------------------------------------------
# Build targets
# -----------------------------------------------------------------------------
SET(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)

if(HG_ENABLE_BULLET3_SCENE_PHYSICS)
	list(APPEND HG_BINDING_DEFINES_LIST "HG_ENABLE_BULLET3_SCENE_PHYSICS")
endif()

if(HG_BINDING_DEFINES_LIST)
	list(JOIN HG_BINDING_DEFINES_LIST "," HG_BINDING_DEFINES_LIST)
	set(HG_BINDING_DEFINES "--defines" ${HG_BINDING_DEFINES_LIST})
endif()

add_subdirectory(extern)
add_subdirectory(binding)

add_subdirectory(harfang)

if(NOT HG_BUILD_CPP_SDK)
	set_target_properties(engine script foundation platform PROPERTIES EXCLUDE_FROM_ALL TRUE)  # [EJ] EXCLUDE_FROM_ALL is set to enable minimal build for docs only configuration
endif()

add_subdirectory(tools)

add_subdirectory(languages)

if(HG_BUILD_DOCS)
	add_subdirectory(doc)
endif()

# -----------------------------------------------------------------------------
# Setup/recap dump
# -----------------------------------------------------------------------------
message(STATUS [[
=====================
Configuration report:
=====================]])

message(STATUS "Harfang version: " ${HG_VERSION})
message(STATUS "Build type: " ${CMAKE_BUILD_TYPE})
message(STATUS "System processor: " ${CMAKE_SYSTEM_PROCESSOR})
message(STATUS "Graphic API: " ${HG_GRAPHIC_API})

message(STATUS "Host prefix: " ${HG_HOST_PREFIX})
message(STATUS "Target prefix: " ${HG_TARGET_PREFIX})

message(STATUS "Enable TSA: " ${HG_ENABLE_TSA})
message(STATUS "Enable TSAN: " ${HG_ENABLE_TSAN})
message(STATUS "Enable ASAN: " ${HG_ENABLE_ASAN})

message(STATUS "Build Assimp converter: " ${HG_BUILD_ASSIMP_CONVERTER})
message(STATUS "Build FBX converter: " ${HG_BUILD_FBX_CONVERTER})
message(STATUS "Build GLTF exporter: " ${HG_BUILD_GLTF_EXPORTER})
message(STATUS "Build GLTF importer: " ${HG_BUILD_GLTF_IMPORTER})
message(STATUS "Build AssetC: " ${HG_BUILD_ASSETC})
message(STATUS "Build C++ SDK: " ${HG_BUILD_CPP_SDK})
message(STATUS "Build documentation: " ${HG_BUILD_DOCS})

message(STATUS "Build Lua extension: " ${HG_BUILD_HG_LUA})

message(STATUS "Build CPython module: " ${HG_BUILD_HG_PYTHON})
if(HG_BUILD_HG_PYTHON)
	message(STATUS "    Using CPython libraries: " ${Python3_LIBRARIES})
	message(STATUS "    Install CPython module to " ${Python3_EXECUTABLE})
endif()

message(STATUS "Build Go module: " ${HG_BUILD_HG_GO})

message(STATUS "Embedded system build: " ${HG_EMBEDDED})
message(STATUS "Binding defines: " ${HG_BINDING_DEFINES})

message(STATUS "Available APIs:")
message(STATUS " - Core")
if(HG_ENABLE_BULLET3_SCENE_PHYSICS)
	message(STATUS " - Bullet Dynamics")
endif()
if(HG_ENABLE_OPENVR_API)
	message(STATUS " - OpenVR")
endif()
if(HG_ENABLE_SRANIPAL_API)
	message(STATUS " - Sranipal")
endif()
if(HG_ENABLE_RECAST_DETOUR_API)
	message(STATUS " - RecastDetour")
endif()

message(STATUS "Use GLFW backend: " ${HG_USE_GLFW})
message(STATUS "Fabgen dir: " ${HG_FABGEN_PATH})
message(STATUS "Installation dir: " ${CMAKE_INSTALL_PREFIX})

message(STATUS "")

# -----------------------------------------------------------------------------
# Package configuration
# -----------------------------------------------------------------------------

# [WARNING] Don't forget to add any new shared library target to this list!
set(HG_SHARED_LIBRARY_TARGETS "hg::glfw;hg::libluadll")
if(HG_ENABLE_OPENVR_API)
    list(APPEND HG_SHARED_LIBRARY_TARGETS OpenVR)
	if(HG_ENABLE_SRANIPAL_API)
		list(APPEND HG_SHARED_LIBRARY_TARGETS SRanipal)
	endif()
endif()

if(HG_BUILD_CPP_SDK)
	write_basic_package_version_file( harfangConfigVersion.cmake VERSION ${HG_VERSION} COMPATIBILITY SameMajorVersion )
	configure_package_config_file( harfang/cmake/harfangConfig.cmake.in harfangConfig.cmake INSTALL_DESTINATION cmake INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} )
	install(
		FILES 
			${CMAKE_CURRENT_BINARY_DIR}/harfangConfigVersion.cmake
			${CMAKE_CURRENT_BINARY_DIR}/harfangConfig.cmake
		DESTINATION cppsdk/cmake
	)
	install(
		EXPORT cppsdkTargets
		DESTINATION cppsdk/cmake
		NAMESPACE hg::
		COMPONENT cppsdk
	)
endif()
