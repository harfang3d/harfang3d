cmake_minimum_required(VERSION 3.1)

set( BGFX_SRCS
	bgfx/src/bgfx.cpp             bgfx/src/renderer_gl.cpp
	bgfx/src/debug_renderdoc.cpp  bgfx/src/renderer_gnm.cpp
	bgfx/src/dxgi.cpp             bgfx/src/renderer_noop.cpp
	bgfx/src/glcontext_egl.cpp    bgfx/src/renderer_vk.cpp
	bgfx/src/glcontext_glx.cpp    bgfx/src/shader.cpp
	bgfx/src/glcontext_wgl.cpp    bgfx/src/shader_dx9bc.cpp
	bgfx/src/nvapi.cpp            bgfx/src/shader_dxbc.cpp
	bgfx/src/renderer_d3d11.cpp   bgfx/src/shader_spirv.cpp
	bgfx/src/renderer_d3d12.cpp   bgfx/src/topology.cpp
	bgfx/src/renderer_d3d9.cpp    bgfx/src/vertexlayout.cpp
	bgfx/src/renderer_nvn.cpp     bgfx/src/renderer_webgpu.cpp
	bgfx/src/renderer_agc.cpp
)

set( BGFX_HDRS
	bgfx/src/bgfx_p.h            bgfx/src/glimports.h
	bgfx/src/charset.h           bgfx/src/nvapi.h
	bgfx/src/config.h            bgfx/src/renderer_d3d11.h
	bgfx/src/debug_renderdoc.h   bgfx/src/renderer_d3d12.h
	bgfx/src/dxgi.h              bgfx/src/renderer_d3d9.h
	bgfx/src/fs_clear0.bin.h     bgfx/src/renderer_d3d.h
	bgfx/src/fs_clear1.bin.h     bgfx/src/renderer_gl.h
	bgfx/src/fs_clear2.bin.h     bgfx/src/renderer.h
	bgfx/src/fs_clear3.bin.h     bgfx/src/renderer_mtl.h
	bgfx/src/fs_clear4.bin.h     bgfx/src/renderer_vk.h
	bgfx/src/fs_clear5.bin.h     bgfx/src/shader_dx9bc.h
	bgfx/src/fs_clear6.bin.h     bgfx/src/shader_dxbc.h
	bgfx/src/fs_clear7.bin.h     bgfx/src/shader.h
	bgfx/src/fs_debugfont.bin.h  bgfx/src/shader_spirv.h
	bgfx/src/glcontext_eagl.h    bgfx/src/topology.h
	bgfx/src/glcontext_egl.h     bgfx/src/vertexlayout.h
	bgfx/src/glcontext_glx.h     bgfx/src/vs_clear.bin.h
	bgfx/src/glcontext_nsgl.h    bgfx/src/vs_debugfont.bin.h
	bgfx/src/glcontext_wgl.h     bgfx/src/renderer_webgpu.h

	bgfx/include/bgfx/bgfx.h     bgfx/include/bgfx/embedded_shader.h
	bgfx/include/bgfx/defines.h  bgfx/include/bgfx/platform.h

	bgfx/include/bgfx/c99/bgfx.h 
)

add_library( bgfx ${BGFX_SRCS} ${BGFX_HDRS} )
set_property( TARGET bgfx PROPERTY PUBLIC_HEADER
	${CMAKE_CURRENT_SOURCE_DIR}/bgfx/include/bgfx/bgfx.h
	${CMAKE_CURRENT_SOURCE_DIR}/bgfx/include/bgfx/defines.h
)
target_include_directories( bgfx 
	PRIVATE
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bgfx/3rdparty>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bgfx/3rdparty/dxsdk/include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bgfx/3rdparty/khronos>
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bgfx/include>
)

if( MSVC )
	target_compile_definitions( bgfx PRIVATE "_CRT_SECURE_NO_WARNINGS" )
endif()

target_compile_definitions( bgfx PUBLIC BGFX_CONFIG_MULTITHREADED=0 )

target_link_libraries( bgfx PUBLIC bx bimg )

if( UNIX AND NOT APPLE AND NOT EMSCRIPTEN )
	if( NOT HG_USE_GLFW_WAYLAND )
		find_package( X11 )
		if(X11_FOUND)
			target_link_libraries( bgfx PUBLIC ${X11_LIBRARIES} )
		endif()
	endif()

	if( (NOT X11_FOUND) OR HG_USE_GLFW_WAYLAND )
		find_package(ECM REQUIRED NO_MODULE)
		list(APPEND CMAKE_MODULE_PATH "${ECM_MODULE_PATH}")
		find_package(Wayland REQUIRED)
		if(Wayland_FOUND)
			target_compile_definitions( bgfx PUBLIC WL_EGL_PLATFORM )
			target_link_libraries( bgfx PUBLIC wayland-egl )
		endif()
	endif()

	find_package( OpenGL )
	if( OPENGL_FOUND )
		target_link_libraries( bgfx PUBLIC ${OPENGL_LIBRARIES})
		set(OPENGL_VERSION 33)
	else()
		find_package(OpenGLES3)
		if(NOT OPENGLES3_FOUND)
			find_package(OpenGLES2 REQUIRED)
			if(OPENGLES2_FOUND)
					set(OPENGL_ES 2)
					set(OPENGLES_LIBRARIES ${OPENGLES2_LIBRARIES})
			endif()
		else()
			set(OPENGL_ES 3)
			set(OPENGLES_LIBRARIES ${OPENGLES3_LIBRARIES})
		endif()
		find_package(EGL REQUIRED)
		target_link_libraries( bgfx PUBLIC ${OPENGLES_LIBRARY} ${EGL_LIBRARIES} )
	endif()
endif()

if( NOT ${OPENGL_VERSION} STREQUAL "" )
	target_compile_definitions( bgfx PRIVATE BGFX_CONFIG_RENDERER_OPENGL=${OPENGL_VERSION})
	message(STATUS "OpenGL version: ${OPENGL_VERSION}")
elseif( NOT ${OPENGLES_VERSION} STREQUAL "")
	target_compile_definitions( bgfx PRIVATE BGFX_CONFIG_RENDERER_OPENGLES=${OPENGLES_VERSION})
	message(STATUS "OpenGL ES version: ${OPENGLES_VERSION}")
endif()

set_target_properties( bgfx PROPERTIES FOLDER "bgfx" )
