// HARFANG(R) Copyright (C) 2022 NWNC. Released under GPL/LGPL/Commercial Licence, see licence.txt for details.
#include "acutest.h"

// foundation tests
extern void test_cext();
extern void test_pack_float();
extern void test_math();
extern void test_rand();
extern void test_units();
extern void test_string();
extern void test_path_tools();
extern void test_log();
extern void test_vec2();
extern void test_vec3();
extern void test_vec4();
extern void test_mat3();
extern void test_mat4();
extern void test_mat44();
extern void test_color();
extern void test_minmax();
extern void test_plane();
extern void test_projection();
extern void test_obb();
extern void test_easing();
extern void test_quaternion();
extern void test_frustum();
extern void test_vector_list();
extern void test_generational_vector_list();
extern void test_intrusive_shared_ptr_st();
extern void test_file();
extern void test_dir();
extern void test_data();
extern void test_rw_interface();
extern void test_data_rw_interface();
extern void test_file_rw_interface();
extern void test_clock();
extern void test_cmd_line();
extern void test_curve();
extern void test_guid();
extern void test_time();
extern void test_version();
extern void test_rect();
extern void test_timer();
extern void test_signal();

// platform tests
extern void test_window();

// engine tests
extern void test_assets();
extern void test_animation();
extern void test_audio();
extern void test_meta();
extern void test_picture();
extern void test_video_stream();
extern void test_scene();

// script tests
extern void test_lua_vm();

TEST_LIST = {
	// foundation
	{"foundation.cext", test_cext},
	{"foundation.pack_float", test_pack_float},
	{"foundation.math", test_math},
	{"foundation.rand", test_rand},
	{"foundation.units", test_units},
	{"foundation.string", test_string},
	{"foundation.path_tools", test_path_tools},
	{"foundation.log", test_log},
	{"foundation.vec2", test_vec2},
	{"foundation.vec3", test_vec3},
	{"foundation.vec4", test_vec4},
	{"foundation.mat3", test_mat3},
	{"foundation.mat4", test_mat4},
	{"foundation.mat44", test_mat44},
	{"foundation.color", test_color},
	{"foundation.minMax", test_minmax},
	{"foundation.plane", test_plane},
	{"foundation.projection", test_projection},
	{"foundation.obb", test_obb},
	{"foundation.easing", test_easing},
	{"foundation.quaternion", test_quaternion},
	{"foundation.frustum", test_frustum},
	{"foundation.vector_list", test_vector_list},
	{"foundation.generational_vector_list", test_generational_vector_list},
	{"foundation.intrusive_shared_ptr_st", test_intrusive_shared_ptr_st},
	{"foundation.file", test_file},
	{"foundation.dir", test_dir},
	{"foundation.data", test_data},
	{"foundation.rw_interface", test_rw_interface},
	{"foundation.data_rw_interface", test_data_rw_interface},
	{"foundation.file_rw_interface", test_file_rw_interface},
	{"foundation.clock", test_clock},
	{"foundation.cmd_line", test_cmd_line},
	{"foundation.curve", test_curve},
	{"foundation.guid", test_guid},
	{"foundation.time", test_time},
	{"foundation.version", test_version},
	{"foundation.rect", test_rect},
	{"foundation.timer", test_timer},
	{"foundation.signal", test_signal},

	// platform
	{"platform.window", test_window},

	// engine
	{"engine.assets", test_assets},
	{"engine.animation", test_animation},
	{"engine.audio", test_audio},
	{"engine.meta", test_meta},
	{"engine.picture", test_picture},
	{"engine.video_stream", test_video_stream},
	{"engine.scene", test_scene},

	// script
	{"script.lua_vm", test_lua_vm},

	{NULL, NULL},
};
