if(HG_BUILD_ASSETC)
	add_subdirectory(assetc)
endif()

if(HG_BUILD_ASSIMP_CONVERTER)
	add_subdirectory(assimp_converter)
endif()

if(HG_BUILD_FBX_CONVERTER)
	add_subdirectory(fbx_converter)
endif()

if(HG_BUILD_GLTF_EXPORTER OR HG_BUILD_GLTF_IMPORTER)
	add_subdirectory(gltf_converter)
endif()
