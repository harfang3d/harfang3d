cmake_minimum_required(VERSION 3.1)

set( BX_SRCS 
	bx/src/allocator.cpp    bx/src/debug.cpp     bx/src/math.cpp       bx/src/sort.cpp
	bx/src/dtoa.cpp         bx/src/mutex.cpp     bx/src/string.cpp     bx/src/settings.cpp
	bx/src/bounds.cpp       bx/src/easing.cpp    bx/src/os.cpp         bx/src/thread.cpp
	bx/src/bx.cpp           bx/src/file.cpp      bx/src/process.cpp    bx/src/timer.cpp
	bx/src/commandline.cpp  bx/src/filepath.cpp  bx/src/semaphore.cpp  bx/src/url.cpp
	bx/src/crtnone.cpp      bx/src/hash.cpp
)

set( BX_HDRS
	bx/include/bx/allocator.h    bx/include/bx/handlealloc.h   bx/include/bx/rng.h
	bx/include/bx/bounds.h       bx/include/bx/hash.h          bx/include/bx/semaphore.h
	bx/include/bx/bx.h           bx/include/bx/macros.h        bx/include/bx/settings.h
	bx/include/bx/commandline.h  bx/include/bx/maputil.h       bx/include/bx/simd_t.h
	bx/include/bx/config.h       bx/include/bx/math.h          bx/include/bx/sort.h
	bx/include/bx/cpu.h          bx/include/bx/mpscqueue.h     bx/include/bx/spscqueue.h
	bx/include/bx/debug.h        bx/include/bx/mutex.h         bx/include/bx/string.h
	bx/include/bx/easing.h       bx/include/bx/os.h            bx/include/bx/thread.h
	bx/include/bx/endian.h       bx/include/bx/pixelformat.h   bx/include/bx/timer.h
	bx/include/bx/error.h        bx/include/bx/platform.h      bx/include/bx/uint32_t.h
	bx/include/bx/file.h         bx/include/bx/process.h       bx/include/bx/url.h
	bx/include/bx/filepath.h     bx/include/bx/readerwriter.h
	bx/include/bx/float4x4_t.h   bx/include/bx/ringbuffer.h
)

set( BX_INLINE_HDRS
	bx/include/bx/inline/allocator.inl    bx/include/bx/inline/readerwriter.inl
	bx/include/bx/inline/bounds.inl       bx/include/bx/inline/ringbuffer.inl
	bx/include/bx/inline/bx.inl           bx/include/bx/inline/rng.inl
	bx/include/bx/inline/cpu.inl          bx/include/bx/inline/simd128_langext.inl
	bx/include/bx/inline/easing.inl       bx/include/bx/inline/simd128_neon.inl
	bx/include/bx/inline/endian.inl       bx/include/bx/inline/simd128_ref.inl
	bx/include/bx/inline/error.inl        bx/include/bx/inline/simd128_sse.inl
	bx/include/bx/inline/float4x4_t.inl   bx/include/bx/inline/simd128_swizzle.inl
	bx/include/bx/inline/handlealloc.inl  bx/include/bx/inline/simd256_avx.inl
	bx/include/bx/inline/hash.inl         bx/include/bx/inline/simd256_ref.inl
	bx/include/bx/inline/math.inl         bx/include/bx/inline/simd_ni.inl
	bx/include/bx/inline/mpscqueue.inl    bx/include/bx/inline/sort.inl
	bx/include/bx/inline/mutex.inl        bx/include/bx/inline/spscqueue.inl
	bx/include/bx/inline/os.inl           bx/include/bx/inline/string.inl
	bx/include/bx/inline/pixelformat.inl  bx/include/bx/inline/uint32_t.inl
) 

add_library( bx STATIC ${BX_SRCS} ${BX_HDRS} ${BX_INLINE_HDRS} )
set_property( TARGET bx PROPERTY FOLDER "bgfx" )
set_property( TARGET bx PROPERTY PUBLIC_HEADER
	${CMAKE_CURRENT_SOURCE_DIR}/bx/include/bx/bx.h
	${CMAKE_CURRENT_SOURCE_DIR}/bx/include/bx/platform.h
	${CMAKE_CURRENT_SOURCE_DIR}/bx/include/bx/config.h
	${CMAKE_CURRENT_SOURCE_DIR}/bx/include/bx/macros.h
)
target_include_directories( bx
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bx/include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bx/3rdparty>
)

if( MSVC )
	target_include_directories( bx PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bx/include/compat/msvc>	)
elseif( MINGW )
	target_include_directories( bx PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bx/include/compat/mingw> )
elseif( APPLE )
	target_include_directories( bx PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bx/include/compat/osx> )
endif()

target_compile_definitions( bx PUBLIC "__STDC_LIMIT_MACROS" )
target_compile_definitions( bx PUBLIC "__STDC_FORMAT_MACROS" )
target_compile_definitions( bx PUBLIC "__STDC_CONSTANT_MACROS" )

target_compile_definitions( bx 
	PUBLIC 
		$<$<CONFIG:Debug>:BX_CONFIG_DEBUG=1>
		$<$<CONFIG:Release>:BX_CONFIG_DEBUG=0>
)

if( UNIX AND NOT APPLE )
	find_package( Threads )
	target_link_libraries( bx ${CMAKE_THREAD_LIBS_INIT} dl rt )
elseif(APPLE)
	find_library( FOUNDATION_LIBRARY Foundation)
	mark_as_advanced( FOUNDATION_LIBRARY )
	target_link_libraries( bx PUBLIC ${FOUNDATION_LIBRARY} )
elseif(MSVC)
	target_link_libraries( bx PUBLIC psapi )
endif()
