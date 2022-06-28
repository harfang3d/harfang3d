cmake_minimum_required(VERSION 3.12)

project(lua LANGUAGES C CXX)

set(LUA_HDRS
	src/lapi.h
	src/lauxlib.h
	src/lcode.h
	src/lctype.h
	src/ldebug.h
	src/ldo.h
	src/lfunc.h
	src/lgc.h
	src/llex.h
	src/llimits.h
	src/lmem.h
	src/lobject.h
	src/lopcodes.h
	src/lparser.h
	src/lprefix.h
	src/lstate.h
	src/lstring.h
	src/ltable.h
	src/ltm.h
	src/lua.h
	src/luaconf.h
	src/lualib.h
	src/lundump.h
	src/lvm.h
	src/lzio.h
	src/ljumptab.h
	src/lopnames.h)

set(SRC_CORE
	src/lapi.c
	src/lcode.c
	src/lctype.c
	src/ldebug.c
	src/ldo.c
	src/ldump.c
	src/lfunc.c
	src/lgc.c
	src/llex.c
	src/lmem.c
	src/lobject.c
	src/lopcodes.c
	src/lparser.c
	src/lstate.c
	src/lstring.c
	src/ltable.c
	src/ltm.c
	src/lundump.c
	src/lvm.c
	src/lzio.c)

set(SRC_LIB
	src/lauxlib.c
	src/lbaselib.c
	src/lcorolib.c
	src/ldblib.c
	src/liolib.c
	src/lmathlib.c
	src/loadlib.c
	src/loslib.c
	src/lstrlib.c
	src/ltablib.c
	src/lutf8lib.c
	src/linit.c)

set(SRC_LUA src/lua.c)
set(SRC_LUAC src/luac.c)

add_library(liblua ${SRC_CORE} ${SRC_LIB})
target_include_directories(liblua 
	PUBLIC 
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
		$<INSTALL_INTERFACE:include/extern/lua>
)
set_target_properties(liblua PROPERTIES OUTPUT_NAME lua)
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
	target_compile_definitions(liblua PUBLIC LUA_USE_LINUX)
endif()
if (WIN32)
	target_compile_definitions(liblua PUBLIC _CRT_SECURE_NO_WARNINGS)
endif()

add_library(libluadll SHARED ${SRC_CORE} ${SRC_LIB})
target_include_directories(libluadll 
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
if(WIN32)
	target_compile_definitions(libluadll PRIVATE _CRT_SECURE_NO_WARNINGS LUA_BUILD_AS_DLL)
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
	target_compile_definitions(libluadll PUBLIC LUA_USE_LINUX)
endif()
set_target_properties(libluadll PROPERTIES OUTPUT_NAME lua53)

if(NOT WIN32)
	set(EXE_LIBS m readline dl)
endif()

add_executable(lua ${SRC_LUA})
target_link_libraries(lua libluadll ${EXE_LIBS})

add_executable(luac ${SRC_LUAC})
target_link_libraries(luac liblua ${EXE_LIBS})
