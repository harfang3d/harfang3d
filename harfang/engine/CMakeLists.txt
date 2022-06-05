set(HDRS
	animation.h
	assets.h
	assets_rw_interface.h
	audio.h
	audio_stream_interface.h
	bloom.h
	create_geometry.h
	cubemap.h
	dear_imgui.h
	debugger.h
	file_format.h
	font.h
	forward_pipeline.h
	fps_controller.h
	vertex.h
	geometry.h
	geometry_builder.h
	iso_surface.h
	lua_object.h
	load_save_scene_flags.h
	meta.h
	model_builder.h
	motion_blur.h
	node.h
	openvr_api.h
	physics.h
	picking_helper.h
	picture.h
	render_pipeline.h
	resource_cache.h
	sao.h
	scene_forward_pipeline.h
	scene.h
	scene_lua_vm.h
	scene_systems.h
	scene_to_obj.h
	script_param.h
	sranipal_api.h
	ssgi.h
	hiz.h
	ssr.h
	taa.h
	downsample.h
	upsample.h
	temporal_accumulation.h
	aaa_blur.h
	to_json.h
	video_stream.h
	video_stream_interface.h
	wav_audio_stream.h
	ogg_audio_stream.h)

set(SRCS
	anim_load_binary.cpp
	anim_load_json.cpp
	animation.cpp
	assets.cpp
	assets_rw_interface.cpp
	audio.cpp
	bloom.cpp
	component.cpp
	create_geometry.cpp
	cubemap.cpp
	dear_imgui.cpp
	debugger.cpp
	font.cpp
	forward_pipeline.cpp
	fps_controller.cpp
	geometry.cpp
	geometry_builder.cpp
	iso_surface.cpp
	lua_object.cpp
	scene_lua_vm.cpp
	meta.cpp
	model_builder.cpp
	motion_blur.cpp
	node.cpp
	openvr_api.cpp
	physics.cpp
	picking_helper.cpp
	picture.cpp
	render_pipeline.cpp
	sao.cpp
	scene.cpp
	scene_systems.cpp
	scene_forward_pipeline.cpp
	scene_load_binary.cpp
	scene_load_json.cpp
	scene_to_obj.cpp
	sranipal_api.cpp
	hiz.cpp
	ssgi.cpp
	ssr.cpp
	taa.cpp
	downsample.cpp
	upsample.cpp
	aaa_blur.cpp
	temporal_accumulation.cpp
	wav_audio_stream.cpp
	ogg_audio_stream.cpp
	vertex.cpp
	video_stream.cpp)

if(HG_ENABLE_RECAST_DETOUR_API)
	list(APPEND SRCS recast_detour.cpp)
	list(APPEND HDRS recast_detour.h)
endif()

if(HG_ENABLE_BULLET3_SCENE_PHYSICS)
	list(APPEND SRCS scene_bullet3_physics.cpp)
	list(APPEND HDRS scene_bullet3_physics.h)
endif()

add_library(engine STATIC ${SRCS} ${HDRS})

target_link_libraries(engine PUBLIC bind_hg_lua script foundation platform bgfx imgui jsonhpp meshoptimizer PRIVATE stb_truetype stb_vorbis stb_image_write)

if(NOT (CMAKE_SYSTEM_NAME STREQUAL "Emscripten"))
	target_link_libraries(engine PRIVATE OpenAL)
endif()

target_link_libraries(engine PRIVATE miniz mikktspace)

if(HG_ENABLE_OPENVR_API)
	target_link_libraries(engine PRIVATE OpenVR)
endif()

if(HG_ENABLE_SRANIPAL_API)
	target_link_libraries(engine PRIVATE SRanipal)
endif()

if(HG_ENABLE_RECAST_DETOUR_API)
	target_link_libraries(engine PRIVATE Recast Detour DetourCrowd)
endif()

if(HG_ENABLE_BULLET3_SCENE_PHYSICS)
	target_link_libraries(engine PRIVATE BulletDynamics)
endif()

if(MSVC)
	target_compile_options(engine PRIVATE "$<$<CONFIG:Release>:/Zi>")
	target_link_options(engine PRIVATE "$<$<CONFIG:Release>:/DEBUG>")
endif()

set_property(TARGET engine PROPERTY PUBLIC_HEADER ${HDRS})
set_target_properties(engine PROPERTIES FOLDER "harfang")

install_cppsdk_target(engine)
