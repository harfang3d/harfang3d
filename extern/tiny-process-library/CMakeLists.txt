cmake_minimum_required(VERSION 3.12)

project(tiny-process-library LANGUAGES CXX)

find_package(Threads REQUIRED)

add_library(tiny-process-library process.cpp)
target_compile_definitions(tiny-process-library PUBLIC UNICODE=1)
set_target_properties(tiny-process-library PROPERTIES CXX_STANDARD 11)
if(MSVC)
	target_compile_definitions(tiny-process-library PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
	target_compile_options(tiny-process-library PRIVATE -Wall -Wextra)
endif()


if(WIN32)
	target_sources(tiny-process-library PRIVATE process_win.cpp)
	#If compiled using MSYS2, use sh to run commands
	if(MSYS)
		target_compile_definitions(tiny-process-library PUBLIC MSYS_PROCESS_USE_SH)
	endif()
else()
	target_sources(tiny-process-library PRIVATE process_unix.cpp)
endif()

target_link_libraries(tiny-process-library ${CMAKE_THREAD_LIBS_INIT})
target_include_directories(tiny-process-library PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
