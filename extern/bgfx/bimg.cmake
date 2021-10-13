cmake_minimum_required(VERSION 3.1)

set( ASTC_CODEC_SRCS
	bimg/3rdparty/astc-codec/src/decoder/astc_file.cc
	bimg/3rdparty/astc-codec/src/decoder/codec.cc
	bimg/3rdparty/astc-codec/src/decoder/endpoint_codec.cc
	bimg/3rdparty/astc-codec/src/decoder/footprint.cc
	bimg/3rdparty/astc-codec/src/decoder/integer_sequence_codec.cc
	bimg/3rdparty/astc-codec/src/decoder/intermediate_astc_block.cc
	bimg/3rdparty/astc-codec/src/decoder/logical_astc_block.cc
	bimg/3rdparty/astc-codec/src/decoder/partition.cc
	bimg/3rdparty/astc-codec/src/decoder/physical_astc_block.cc
	bimg/3rdparty/astc-codec/src/decoder/quantization.cc
	bimg/3rdparty/astc-codec/src/decoder/weight_infill.cc
	bimg/3rdparty/astc-codec/include/astc-codec/astc-codec.h
)
add_library( astc-codec STATIC ${ASTC_CODEC_SRCS} )
target_include_directories( astc-codec
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bimg/3rdparty/astc-codec>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bimg/3rdparty/astc-codec/include>
)
set_property( TARGET astc-codec PROPERTY FOLDER "bgfx/3rdparty" )

set( ASTC_SRCS 
	bimg/3rdparty/astc/astc_averages_and_directions.cpp
	bimg/3rdparty/astc/astc_block_sizes2.cpp
	bimg/3rdparty/astc/astc_color_quantize.cpp
	bimg/3rdparty/astc/astc_color_unquantize.cpp
	bimg/3rdparty/astc/astc_compress_symbolic.cpp
	bimg/3rdparty/astc/astc_compute_variance.cpp
	bimg/3rdparty/astc/astc_decompress_symbolic.cpp
	bimg/3rdparty/astc/astc_encoding_choice_error.cpp
	bimg/3rdparty/astc/astc_find_best_partitioning.cpp
	bimg/3rdparty/astc/astc_ideal_endpoints_and_weights.cpp
	bimg/3rdparty/astc/astc_imageblock.cpp
	bimg/3rdparty/astc/astc_integer_sequence.cpp
	bimg/3rdparty/astc/astc_kmeans_partitioning.cpp
	bimg/3rdparty/astc/astc_lib.cpp
	bimg/3rdparty/astc/astc_partition_tables.cpp
	bimg/3rdparty/astc/astc_percentile_tables.cpp
	bimg/3rdparty/astc/astc_pick_best_endpoint_format.cpp
	bimg/3rdparty/astc/astc_quantization.cpp
	bimg/3rdparty/astc/astc_symbolic_physical.cpp
	bimg/3rdparty/astc/astc_weight_align.cpp
	bimg/3rdparty/astc/astc_weight_quant_xfer_tables.cpp
	bimg/3rdparty/astc/mathlib.cpp
	bimg/3rdparty/astc/softfloat.cpp
)
set( ASTC_HDRS 
	bimg/3rdparty/astc/astc_codec_internals.h
	bimg/3rdparty/astc/softfloat.h
	bimg/3rdparty/astc/astc_lib.h
	bimg/3rdparty/astc/vectypes.h
)
add_library( astc STATIC ${ASTC_SRCS} ${ASTC_HDRS})
target_include_directories( astc
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bimg/3rdparty>
)
if( MSVC )
	target_compile_options( astc PRIVATE /wd4244 )
endif()
set_property( TARGET astc PROPERTY FOLDER "bgfx/3rdparty" )

add_library( edtaa3 STATIC bimg/3rdparty/edtaa3/edtaa3func.cpp bimg/3rdparty/edtaa3/edtaa3func.h )
target_include_directories( edtaa3
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bimg/3rdparty>
)
set_property( TARGET edtaa3 PROPERTY FOLDER "bgfx/3rdparty" )

add_library( etc1 STATIC bimg/3rdparty/etc1/etc1.cpp bimg/3rdparty/etc1/etc1.h )
target_include_directories( etc1
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bimg/3rdparty>
)
set_property( TARGET etc1 PROPERTY FOLDER "bgfx/3rdparty" )

set( ETC2_SRCS bimg/3rdparty/etc2/ProcessRGB.cpp  bimg/3rdparty/etc2/Tables.cpp )
set( ETC2_HDRS 
	bimg/3rdparty/etc2/Math.hpp           bimg/3rdparty/etc2/Tables.hpp
	bimg/3rdparty/etc2/ProcessCommon.hpp  bimg/3rdparty/etc2/Types.hpp
	bimg/3rdparty/etc2/ProcessRGB.hpp     bimg/3rdparty/etc2/Vector.hpp
)
add_library( etc2 STATIC ${ETC2_SRCS} ${ETC2_HRDS} )
target_include_directories( etc2
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bimg/3rdparty>
)
target_link_libraries( etc2 bx )
set_target_properties( etc2 PROPERTIES FOLDER "bgfx/3rdparty" )

set( IQA_SRCS
	bimg/3rdparty/iqa/source/convolve.c    bimg/3rdparty/iqa/source/ms_ssim.c
	bimg/3rdparty/iqa/source/decimate.c    bimg/3rdparty/iqa/source/psnr.c
	bimg/3rdparty/iqa/source/math_utils.c  bimg/3rdparty/iqa/source/ssim.c
	bimg/3rdparty/iqa/source/mse.c
)
set( IQA_HDRS
	bimg/3rdparty/iqa/include/convolve.h  bimg/3rdparty/iqa/include/iqa_os.h
	bimg/3rdparty/iqa/include/decimate.h  bimg/3rdparty/iqa/include/math_utils.h
	bimg/3rdparty/iqa/include/iqa.h       bimg/3rdparty/iqa/include/ssim.h
)
add_library( iqa STATIC ${IQA_SRCS} ${IQA_HDRS} )
target_include_directories( iqa
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bimg/3rdparty/iqa/include>

)
set_property( TARGET iqa PROPERTY FOLDER "bgfx/3rdparty" )

set( SQUISH_SRCS
	bimg/3rdparty/libsquish/alpha.cpp
	bimg/3rdparty/libsquish/clusterfit.cpp
	bimg/3rdparty/libsquish/colourblock.cpp
	bimg/3rdparty/libsquish/colourfit.cpp
	bimg/3rdparty/libsquish/colourset.cpp
	bimg/3rdparty/libsquish/maths.cpp
	bimg/3rdparty/libsquish/rangefit.cpp
	bimg/3rdparty/libsquish/singlecolourfit.cpp
	bimg/3rdparty/libsquish/squish.cpp
)
set( SQUISH_HDRS
	bimg/3rdparty/libsquish/alpha.h
	bimg/3rdparty/libsquish/clusterfit.h
	bimg/3rdparty/libsquish/colourblock.h
	bimg/3rdparty/libsquish/colourfit.h
	bimg/3rdparty/libsquish/colourset.h
	bimg/3rdparty/libsquish/config.h
	bimg/3rdparty/libsquish/maths.h
	bimg/3rdparty/libsquish/rangefit.h
	bimg/3rdparty/libsquish/simd_float.h
	bimg/3rdparty/libsquish/simd.h
	bimg/3rdparty/libsquish/singlecolourfit.h
	bimg/3rdparty/libsquish/squish.h
	bimg/3rdparty/libsquish/singlecolourlookup.inl
)
add_library( squish STATIC ${SQUISH_SRCS} ${SQUISH_HDRS} )
target_include_directories( squish
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bimg/3rdparty>
)
set_property( TARGET squish PROPERTY FOLDER "bgfx/3rdparty" )

set( NVTT_SRCS
	bimg/3rdparty/nvtt/bc6h/zoh.cpp     bimg/3rdparty/nvtt/bc6h/zohtwo.cpp
	bimg/3rdparty/nvtt/bc6h/zohone.cpp  bimg/3rdparty/nvtt/bc6h/zoh_utils.cpp
	bimg/3rdparty/nvtt/bc7/avpcl.cpp        bimg/3rdparty/nvtt/bc7/avpcl_mode4.cpp
	bimg/3rdparty/nvtt/bc7/avpcl_mode0.cpp  bimg/3rdparty/nvtt/bc7/avpcl_mode5.cpp
	bimg/3rdparty/nvtt/bc7/avpcl_mode1.cpp  bimg/3rdparty/nvtt/bc7/avpcl_mode6.cpp
	bimg/3rdparty/nvtt/bc7/avpcl_mode2.cpp  bimg/3rdparty/nvtt/bc7/avpcl_mode7.cpp
	bimg/3rdparty/nvtt/bc7/avpcl_mode3.cpp  bimg/3rdparty/nvtt/bc7/avpcl_utils.cpp
	bimg/3rdparty/nvtt/nvmath/fitting.cpp
	bimg/3rdparty/nvtt/nvtt.cpp
)
set( NVTT_HDRS
	bimg/3rdparty/nvtt/bc6h/bits.h        bimg/3rdparty/nvtt/bc6h/zoh.h
	bimg/3rdparty/nvtt/bc6h/shapes_two.h  bimg/3rdparty/nvtt/bc6h/zoh_utils.h
	bimg/3rdparty/nvtt/bc6h/tile.h
	bimg/3rdparty/nvtt/bc7/avpcl.h        bimg/3rdparty/nvtt/bc7/shapes_three.h
	bimg/3rdparty/nvtt/bc7/avpcl_utils.h  bimg/3rdparty/nvtt/bc7/shapes_two.h
	bimg/3rdparty/nvtt/bc7/bits.h         bimg/3rdparty/nvtt/bc7/tile.h
	bimg/3rdparty/nvtt/bc7/endpts.h
	bimg/3rdparty/nvtt/nvmath/fitting.h  bimg/3rdparty/nvtt/nvmath/plane.h
	bimg/3rdparty/nvtt/nvmath/matrix.h   bimg/3rdparty/nvtt/nvmath/vector.h
	bimg/3rdparty/nvtt/nvmath/nvmath.h   bimg/3rdparty/nvtt/nvmath/matrix.inl
	bimg/3rdparty/nvtt/nvmath/vector.inl bimg/3rdparty/nvtt/nvmath/plane.inl
	bimg/3rdparty/nvtt/nvcore/array.h
	bimg/3rdparty/nvtt/nvcore/array.inl
	bimg/3rdparty/nvtt/nvcore/debug.h
	bimg/3rdparty/nvtt/nvcore/defsgnucdarwin.h
	bimg/3rdparty/nvtt/nvcore/defsgnuclinux.h
	bimg/3rdparty/nvtt/nvcore/defsgnucwin32.h
	bimg/3rdparty/nvtt/nvcore/defsvcwin32.h
	bimg/3rdparty/nvtt/nvcore/foreach.h
	bimg/3rdparty/nvtt/nvcore/hash.h
	bimg/3rdparty/nvtt/nvcore/memory.h
	bimg/3rdparty/nvtt/nvcore/nvcore.h
	bimg/3rdparty/nvtt/nvcore/posh.h
	bimg/3rdparty/nvtt/nvcore/stdstream.h
	bimg/3rdparty/nvtt/nvcore/stream.h
	bimg/3rdparty/nvtt/nvcore/strlib.h
	bimg/3rdparty/nvtt/nvcore/utils.h
	bimg/3rdparty/nvtt/nvtt.h
)
add_library( nvtt STATIC ${NVTT_SRCS} ${NVTT_HDRS} )
target_include_directories( nvtt
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bimg/3rdparty>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bimg/3rdparty/nvtt>
)
target_link_libraries( nvtt bx )
set_property( TARGET nvtt PROPERTY FOLDER "bgfx/3rdparty" )

set( PVRTC_SRCS 
	bimg/3rdparty/pvrtc/BitScale.cpp      bimg/3rdparty/pvrtc/PvrTcEncoder.cpp
	bimg/3rdparty/pvrtc/MortonTable.cpp   bimg/3rdparty/pvrtc/PvrTcPacket.cpp
	bimg/3rdparty/pvrtc/PvrTcDecoder.cpp
)
set( PVRTC_HDRS
	bimg/3rdparty/pvrtc/AlphaBitmap.h  bimg/3rdparty/pvrtc/Point2.h
	bimg/3rdparty/pvrtc/Bitmap.h       bimg/3rdparty/pvrtc/PvrTcDecoder.h
	bimg/3rdparty/pvrtc/BitScale.h     bimg/3rdparty/pvrtc/PvrTcEncoder.h
	bimg/3rdparty/pvrtc/BitUtility.h   bimg/3rdparty/pvrtc/PvrTcPacket.h
	bimg/3rdparty/pvrtc/ColorRgba.h    bimg/3rdparty/pvrtc/RgbaBitmap.h
	bimg/3rdparty/pvrtc/Interval.h     bimg/3rdparty/pvrtc/RgbBitmap.h
	bimg/3rdparty/pvrtc/MortonTable.h
)
add_library( pvrtc STATIC ${PVRTC_SRCS} ${PVRTC_HDRS} )
target_include_directories( pvrtc
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bimg/3rdparty>
)
set_property( TARGET pvrtc PROPERTY FOLDER "bgfx/3rdparty" )

set( BIMG_SRCS 
	bimg/src/image.cpp                 bimg/src/image_encode.cpp
	bimg/src/image_cubemap_filter.cpp  bimg/src/image_gnf.cpp
	bimg/src/image_decode.cpp
)
set( BIMG_HDRS
	bimg/include/bimg/bimg.h    bimg/include/bimg/encode.h
	bimg/include/bimg/decode.h
)
add_library( bimg STATIC ${BIMG_SRCS} ${BIMG_HDRS} )
target_include_directories( bimg
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../miniz>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bimg/include>
)
target_link_libraries( bimg bx astc-codec astc edtaa3 etc1 etc2 iqa squish nvtt pvrtc miniz )
set_property( TARGET bimg PROPERTY FOLDER "bgfx")
set_property( TARGET bimg PROPERTY PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/bimg/include/bimg/bimg.h )
