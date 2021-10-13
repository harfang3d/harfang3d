set(HDRS
	call_stack.h
	crash_dump.h
	filesystem_watcher.h
	input_system.h
	platform.h
	process.h
	shared_library.h
	window_system.h)

set(SRCS
	input_system.cpp
	filesystem_watcher.cpp
	window_system_base.cpp)

if(HG_USE_GLFW)
	list(APPEND HDRS
		glfw/input_system_glfw.h
	)

	list(APPEND SRCS
		glfw/input_system_glfw.cpp
		glfw/window_system.cpp
	)
endif()

if(WIN32)
	list(APPEND HDRS
		win32/assert.h
		win32/input_dinput.h
		win32/platform.h
	)

	list(APPEND SRCS
		win32/assert.cpp
		win32/call_stack.cpp
		win32/crash_dump.cpp
		win32/platform.cpp
		win32/process.cpp
		win32/shared_library.cpp
		win32/thread.cpp
		win32/input_dinput.cpp
	)
elseif(APPLE)
	list(APPEND SRCS
		osx/platform.cpp
	)
else()
	list(APPEND SRCS
		posix/call_stack.cpp
		posix/crash_dump.cpp
		posix/process.cpp
		posix/shared_library.cpp
		posix/thread.cpp
	)
	if(ANDROID)
		list(APPEND HDRS
			android/android_jni.h
			android/io_aasset.h
			android/window_system.h
			android/logcat_immediate_output.h
		)
		list(APPEND SRCS
			android/android_jni.cpp
			android/window_system.cpp
			android/io_aasset.cpp
			android/input_system.cpp
			android/logcat_immediate_output.cpp
			android/platform.cpp
		)
	elseif(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
		list(APPEND SRCS
			sdl/input_system.cpp
			sdl/window_system.cpp
		)
	else()
		list(APPEND SRCS
			posix/platform.cpp
		)
		endif()
endif()

add_library(platform STATIC ${SRCS} ${HDRS})
set_target_properties(platform PROPERTIES FOLDER "harfang")

if(WIN32)
	target_link_libraries(platform foundation User32 Gdi32 Ws2_32 Wldap32 Winmm DbgHelp Shlwapi)
	target_include_directories(platform PUBLIC ${GLFW_INCLUDE_DIRS})
	target_link_libraries(platform foundation glfw)
elseif(ANDROID)
	target_include_directories(platform PRIVATE ${ANDROID_NDK}/sources/android/native_app_glue)
	target_link_libraries(platform foundation android log)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
	target_link_libraries(platform foundation GL drm dl)
else()
	target_include_directories(platform PUBLIC ${GLFW_INCLUDE_DIRS})
	target_link_libraries(platform foundation glfw)
	if(UNIX AND NOT APPLE)
		if(NOT HG_EMBEDDED)
			find_package(PkgConfig REQUIRED)
			pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
			target_include_directories(platform PRIVATE ${GTK3_INCLUDE_DIRS})
			target_link_libraries(platform ${GTK3_LIBRARIES})
		endif()
		target_link_libraries(platform dl)
	endif()
endif()

set_property(TARGET platform PROPERTY PUBLIC_HEADER call_stack.h crash_dump.h filesystem_watcher.h input_system.h platform.h process.h shared_library.h window_system.h)

install_cppsdk_target(platform)
