#[[
 Try to find libuuid
 Provides the following target and variables:
 	* uuid : library target
	* UUID_FOUND : set if libuuid was found
	* UUID_INCLUDE_DIR : libuuid include diretory
	* UUID_LIBRARY : libuuid library file
#]]
find_path(UUID_INCLUDE_DIR
	NAMES uuid/uuid.h
	HINTS ${UUID_ROOT_DIR}
)

find_library(UUID_LIBRARY
	NAMES uuid
	HINTS ${UUID_ROOT_DIR}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(uuid REQUIRED_VARS UUID_LIBRARY UUID_INCLUDE_DIR)

mark_as_advanced(UUID_FOUND UUID_LIBRARY UUID_INCLUDE_DIR)

if(UUID_FOUND AND NOT TARGET uuid)
	add_library(uuid UNKNOWN IMPORTED)
	set_target_properties(uuid PROPERTIES 
		IMPORTED_LOCATION "${UUID_LIBRARY}"
		INTERFACE_INCLUDE_DIRECTORIES "${UUID_INCLUDE_DIR}"
	)
endif()
