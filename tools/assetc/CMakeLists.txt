add_executable(assetc assetc.cpp)

add_dependencies(assetc texturec shaderc luac cmft)
set_target_properties(assetc PROPERTIES FOLDER "harfang/tools")
target_link_libraries(assetc engine tiny-process-library)

if(HG_ENABLE_RECAST_DETOUR_API)
	add_subdirectory(recastc)
endif()

if(UNIX)
	target_link_libraries(assetc pthread)
endif()

# assetc installation
install(TARGETS assetc RUNTIME DESTINATION assetc LIBRARY DESTINATION assetc COMPONENT assetc)

install_cppsdk_dependencies(assetc)

if(NOT ASSETC_TARGET_PREFIX)
	set(ASSETC_TARGET_PREFIX ${HG_TARGET_PREFIX})
endif()

# toolchain installation

# [EJ22032021] BEWARE! The host for assetc is NOT the host for CMake!
# cmake_host (HG_HOST_PREFIX) builds the executables for cmake_target (HG_TARGET_PREFIX)
# assetc_host runs the executables (so HG_TARGET_PREFIX) and outputs assets for assetc_target (ASSETC_TARGET_PREFIX)
set(TOOLCHAIN_PATH assetc/toolchains/host-${HG_TARGET_PREFIX}-target-${ASSETC_TARGET_PREFIX})

install(TARGETS texturec shaderc luac cmft RUNTIME DESTINATION ${TOOLCHAIN_PATH} COMPONENT assetc)

if(WIN32)
	install(PROGRAMS bin/win/texconv.exe DESTINATION ${TOOLCHAIN_PATH} COMPONENT assetc)
endif()

if(HG_ENABLE_RECAST_DETOUR_API)
	install(TARGETS recastc RUNTIME DESTINATION ${TOOLCHAIN_PATH} COMPONENT assetc)
endif()
