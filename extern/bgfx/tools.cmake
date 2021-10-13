cmake_minimum_required(VERSION 3.1)

add_executable( texturec ${CMAKE_CURRENT_LIST_DIR}/bimg/tools/texturec/texturec.cpp )
target_link_libraries( texturec bimg )
set_target_properties( texturec PROPERTIES FOLDER "bgfx/tools" )

file( GLOB SPIRV_OPT_SRCS
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/opt/*.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/reduce/*.cpp
)
list( APPEND SPIRV_OPT_SRCS 
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/assembly_grammar.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/binary.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/diagnostic.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/disassemble.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/enum_string_mapping.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/ext_inst.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/extensions.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/libspirv.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/name_mapper.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/opcode.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/operand.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/parsed_operand.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/print.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/software_version.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/spirv_endian.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/spirv_optimizer_options.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/spirv_reducer_options.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/spirv_target_env.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/spirv_validator_options.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/table.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/text.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/text_handler.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/util/bit_vector.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/util/parse_number.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/util/string_utils.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/basic_block.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/construct.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/function.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/instruction.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validate.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validate_adjacency.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validate_annotation.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validate_arithmetics.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validate_atomics.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validate_barriers.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validate_bitwise.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validate_builtins.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validate_capability.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validate_cfg.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validate_composites.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validate_constants.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validate_conversion.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validate_debug.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validate_decorations.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validate_derivatives.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validate_execution_limitations.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validate_extensions.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validate_function.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validate_id.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validate_image.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validate_instruction.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validate_interfaces.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validate_layout.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validate_literals.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validate_logicals.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validate_memory.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validate_memory_semantics.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validate_misc.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validate_mode_setting.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validate_non_uniform.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validate_primitives.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validate_scopes.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validate_small_type_uses.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validate_type.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validation_state.cpp
)
file( GLOB SPIRV_OPT_HDRS
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/opt/*.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/reduce/*.h
)
list( APPEND SPIRV_OPT_SRCS 
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/assembly_grammar.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/binary.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/cfa.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/diagnostic.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/disassemble.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/enum_set.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/enum_string_mapping.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/ext_inst.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/extensions.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/instruction.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/latest_version_glsl_std_450_header.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/latest_version_opencl_std_header.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/latest_version_spirv_header.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/macro.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/name_mapper.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/opcode.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/operand.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/parsed_operand.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/print.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/spirv_constant.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/spirv_definition.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/spirv_endian.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/spirv_target_env.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/spirv_validator_options.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/table.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/text.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/text_handler.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/util/bit_vector.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/util/bitutils.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/util/hex_float.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/util/parse_number.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/util/string_utils.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/util/timer.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/decoration.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source/val/validate.h
)
add_library( spirv-opt STATIC ${SPIRV_OPT_SRCS} ${SPIRV_OPT_HDRS} )
target_include_directories( spirv-opt PUBLIC ${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/include ${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/include/generated ${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-headers/include )
target_include_directories( spirv-opt PRIVATE ${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools )
target_compile_definitions( spirv-opt PRIVATE SPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS )
if(MSVC)
	target_compile_options( spirv-opt PRIVATE /wd4127 /wd4389 /wd4702 /wd4706 )
elseif( UNIX AND (MINGW OR MSYS) )
	target_compile_options( spirv-opt PRIVATE -Wno-switch )
	if( NOT APPLE )
		target_compile_options( spirv-opt PRIVATE -Wno-misleading-indentation )
	endif()
endif()
set_target_properties( spirv-opt PROPERTIES FOLDER "bgfx/3rdparty" )

set( SPIRV_CROSS_SRCS
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-cross/spirv_cfg.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-cross/spirv_cpp.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-cross/spirv_cross.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-cross/spirv_cross_parsed_ir.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-cross/spirv_cross_util.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-cross/spirv_glsl.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-cross/spirv_hlsl.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-cross/spirv_msl.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-cross/spirv_parser.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-cross/spirv_reflect.cpp
)
add_library( spirv-cross STATIC ${SPIRV_CROSS_SRCS} )
target_include_directories( spirv-cross PUBLIC ${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-cross/ ${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-cross/include )
target_compile_definitions( spirv-cross PRIVATE SPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS )
if(MSVC)
	target_compile_options( spirv-cross PRIVATE /wd4018 /wd4245 /wd4706 /wd4715 )
endif()
set_target_properties( spirv-cross PROPERTIES FOLDER "bgfx/3rdparty" )

set( FCPP_SRCS
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/fcpp/cpp1.c  ${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/fcpp/cpp5.c
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/fcpp/cpp2.c  ${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/fcpp/cpp6.c
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/fcpp/cpp3.c  ${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/fcpp/cpp4.c
)
set( FCPP_HDRS
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/fcpp/cppadd.h   ${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/fcpp/fpp.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/fcpp/cppdef.h   ${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/fcpp/fpp_pragmas.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/fcpp/cpp.h      ${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/fcpp/FPP_protos.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/fcpp/FPPBase.h
)
add_library( fcpp STATIC ${FCPP_SRCS} ${FCPP_HDRS} )
target_include_directories( fcpp PUBLIC ${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/fcpp )
if(MSVC)
	target_compile_options( fcpp PRIVATE /wd4055 /wd4244 /wd4701 /wd4706 )
else()
	target_compile_options( fcpp PRIVATE -Wno-implicit-fallthrough -Wno-incompatible-pointer-types -Wno-parentheses-equality )
endif()
target_compile_definitions( fcpp PUBLIC NINCLUDE=64 NWORK=65536 NBUFF=65536 OLD_PREPROCESSOR=0 )
set_target_properties( fcpp PROPERTIES FOLDER "bgfx/3rdparty" )

file( GLOB GLSLANG_SRCS
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glslang/glslang/MachineIndependent/*.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glslang/glslang/MachineIndependent/preprocessor/*.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glslang/glslang/GenericCodeGen/*.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glslang/glslang/HLSL/*.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glslang/hlsl/*.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glslang/SPIRV/*.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glslang/OGLCompilersDLL/*.cpp
)
file( GLOB GLSLANG_HDRS
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glslang/glslang/MachineIndependent/*.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glslang/glslang/HLSL/*.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glslang/hlsl/*.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glslang/SPIRV/*.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glslang/OGLCompilersDLL/*.h
)
if(MSVC)
	list( APPEND GLSLANG_SRCS ${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glslang/glslang/OSDependent/Windows/ossource.cpp )
else()
	list( APPEND GLSLANG_SRCS ${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glslang/glslang/OSDependent/Unix/ossource.cpp )
endif()
add_library( glslang STATIC ${GLSLANG_SRCS} ${GLSLANG_HDRS} )
target_include_directories( glslang PUBLIC ${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glslang ${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/include ${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/spirv-tools/source ${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glslang/glslang/Public ${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glslang/glslang/Include PRIVATE ${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty )
target_compile_definitions( glslang PUBLIC ENABLE_OPT=1 ENABLE_HLSL=1 )
if(MSVC)
	target_compile_options( glslang PRIVATE /wd4005 /wd4065 /wd4100 /wd4127 /wd4189 /wd4244 /wd4310 /wd4389 /wd4456 /wd4457 /wd4458 /wd4702 /wd4715 /wd4838 )
elseif( UNIX AND (MINGW OR MSYS) )
	target_compile_options( glslang PRIVATE -Wno-ignored-qualifiers -Wno-implicit-fallthrough -Wno-missing-field-initializers -Wno-reorder -Wno-return-type -Wno-shadow -Wno-sign-compare -Wno-switch -Wno-undef -Wno-unknown-pragmas -Wno-unused-function -Wno-unused-parameter -Wno-unused-variable )
	target_compile_options( glslang PRIVATE -fno-strict-aliasing )
	if( APPLE )
		target_compile_options( glslang PRIVATE -Wno-c++11-extensions -Wno-unused-const-variable -Wno-deprecated-register )
	else()
		target_compile_options( glslang PRIVATE -Wno-unused-but-set-variable )
	endif()
endif()
set_target_properties( glslang PROPERTIES FOLDER "bgfx/3rdparty" )

set( GLSL_OPTIMIZER_SRCS 
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/glcpp/glcpp-parse.c
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/glcpp/pp.c
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/glcpp/glcpp-lex.c

	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/glsl_parser.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/lower_discard_flow.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/opt_dead_code.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/lower_ubo_reference.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/link_uniform_blocks.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/glsl_types.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir_function_detect_recursion.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/loop_analysis.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/opt_algebraic.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/opt_if_simplification.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/hir_field_selection.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir_hv_accept.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/linker.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir_basic_block.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/lower_discard.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ast_to_hir.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/builtin_functions.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/strtod.c
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/opt_constant_variable.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/glsl_lexer.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/opt_tree_grafting.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/lower_named_interface_blocks.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/lower_vec_index_to_swizzle.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/opt_dead_builtin_varyings.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/lower_vector.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/opt_flip_matrices.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/lower_vec_index_to_cond_assign.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/opt_rebalance_tree.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/opt_function_inlining.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/lower_if_to_cond_assign.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/opt_constant_folding.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir_expression_flattening.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/lower_vertex_id.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/opt_minmax.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/link_uniform_block_active_visitor.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/lower_mat_op_to_vec.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir_hierarchical_visitor.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/lower_packing_builtins.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/lower_instructions.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/opt_dead_code_local.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/opt_array_splitting.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/lower_noise.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir_equals.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/lower_jumps.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir_unused_structs.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/opt_noop_swizzle.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/builtin_variables.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/link_interface_blocks.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/link_uniforms.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir_print_glsl_visitor.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/opt_flatten_nested_if_blocks.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir_print_visitor.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/lower_variable_index_to_cond_assign.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/standalone_scaffolding.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir_function.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ast_type.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/builtin_types.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/opt_dead_functions.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ast_function.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/link_functions.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir_builder.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/glsl_optimizer.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/link_atomics.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/lower_packed_varyings.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/s_expression.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir_clone.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/glsl_parser_extras.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/opt_structure_splitting.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/glcpp/glcpp-parse.c
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/glcpp/pp.c
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/glcpp/glcpp-lex.c
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/link_varyings.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir_stats.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir_validate.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir_rvalue_visitor.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir_function_can_inline.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/opt_copy_propagation_elements.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/loop_controls.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/lower_vector_insert.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/lower_output_reads.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir_variable_refcount.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/opt_constant_propagation.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/lower_clip_distance.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/opt_vectorize.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir_print_metal_visitor.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/opt_dead_builtin_variables.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir_constant_expression.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/opt_swizzle_swizzle.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/opt_cse.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/lower_offset_array.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir_import_prototypes.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/glsl_symbol_table.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ast_expr.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/link_uniform_initializers.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/loop_unroll.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/opt_redundant_jumps.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ast_array_index.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/opt_copy_propagation.cpp
	
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/mesa/program/symbol_table.c
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/mesa/program/prog_hash_table.c
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/mesa/main/imports.c

	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/util/hash_table.c
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/util/ralloc.c
)

set( GLSL_OPTIMIZER_HDRS 
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/glcpp/glcpp-parse.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/glcpp/glcpp.h

	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/glsl_optimizer.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir_hierarchical_visitor.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir_function_inlining.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/strtod.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir_print_metal_visitor.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/glsl_symbol_table.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir_print_visitor.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/linker.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir_stats.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir_optimization.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir_basic_block.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir_builder.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir_uniform.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/glsl_parser_extras.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/link_uniform_block_active_visitor.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/loop_analysis.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/list.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir_print_glsl_visitor.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir_expression_flattening.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/program.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/standalone_scaffolding.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir_visitor.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/glcpp/glcpp-parse.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/glcpp/glcpp.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir_unused_structs.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/link_varyings.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/glsl_types.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/glsl_parser.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir_rvalue_visitor.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/builtin_type_macros.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/s_expression.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ir_variable_refcount.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl/ast.h

	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/mesa/program/prog_instruction.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/mesa/program/hash_table.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/mesa/program/symbol_table.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/mesa/program/prog_parameter.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/mesa/program/prog_statevars.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/mesa/main/dd.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/mesa/main/compiler.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/mesa/main/core.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/mesa/main/context.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/mesa/main/errors.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/mesa/main/simple_list.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/mesa/main/imports.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/mesa/main/config.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/mesa/main/glheader.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/mesa/main/macros.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/mesa/main/glminimal.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/mesa/main/mtypes.h

	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/util/ralloc.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/util/hash_table.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/util/macros.h
)

add_library( glsl-optimizer STATIC ${GLSL_OPTIMIZER_SRCS} ${GLSL_OPTIMIZER_HDRS} )
target_include_directories( glsl-optimizer PUBLIC ${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src ${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/include ${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/mesa ${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/mapi ${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/glsl-optimizer/src/glsl )
if(MSVC)
	target_compile_definitions( glsl-optimizer PUBLIC __STDC__ __STDC_VERSION__=199901L strdup=_strdup alloca=_alloca isascii=__isascii )
	target_compile_options( glsl-optimizer PRIVATE /W0 )
elseif( UNIX AND (MINGW OR MSYS) )
	target_compile_options( glsl-optimizer PRIVATE -Wno-implicit-fallthrough -Wno-parentheses -Wno-sign-compare -Wno-unused-function -Wno-unused-parameter -Wno-shadow )
	target_compile_options( glsl-optimizer PRIVATE -fno-strict-aliasing )
	if( APPLE )
		target_compile_options( glsl-optimizer PRIVATE -Wno-deprecated-register )
	else()
		target_compile_options( glsl-optimizer PRIVATE -Wno-misleading-indentation )
	endif()
endif()
set_target_properties( glsl-optimizer PROPERTIES FOLDER "bgfx/3rdparty" )

set( SHADERC_SRCS
	${CMAKE_CURRENT_LIST_DIR}/bgfx/tools/shaderc/shaderc.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/tools/shaderc/shaderc_pssl.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/tools/shaderc/shaderc_glsl.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/tools/shaderc/shaderc_spirv.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/tools/shaderc/shaderc_hlsl.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/tools/shaderc/shaderc_metal.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/src/vertexlayout.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/src/shader_spirv.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/src/shader.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/src/shader_dx9bc.cpp
	${CMAKE_CURRENT_LIST_DIR}/bgfx/src/shader_dxbc.cpp  
)
set( SHADERC_HDRS
	${CMAKE_CURRENT_LIST_DIR}/bgfx/tools/shaderc/shaderc.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/src/vertexlayout.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/src/shader_spirv.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/src/shader_dx9bc.h
	${CMAKE_CURRENT_LIST_DIR}/bgfx/src/shader_dxbc.h 
	${CMAKE_CURRENT_LIST_DIR}/bgfx/src/shader.h
)

add_executable( shaderc ${SHADERC_SRCS} ${SHADERC_HDRS} )
target_compile_definitions( shaderc PRIVATE "-D_CRT_SECURE_NO_WARNINGS" )
target_include_directories( shaderc PRIVATE ${CMAKE_CURRENT_LIST_DIR}/bgfx/include ${CMAKE_CURRENT_LIST_DIR}/bgfx/3rdparty/webgpu/include )
target_link_libraries( shaderc bx bimg fcpp glsl-optimizer glslang spirv-cross spirv-opt )
set_target_properties( shaderc PROPERTIES FOLDER "bgfx/tools" )

if( ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" ) 
	if( CMAKE_CXX_COMPILER_VERSION VERSION_EQUAL 7.5.0 ) 
		target_compile_options( glsl-optimizer PUBLIC $<$<CONFIG:RELEASE>:-O2 -DNDEBUG> ) 
		target_compile_options( shaderc PUBLIC $<$<CONFIG:RELEASE>:-O2 -DNDEBUG> ) 
	endif() 
endif()