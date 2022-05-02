#[[
 Try to find libreadline
 Provides the following target and variables:
 	* readline : library target
	* READLINE_FOUND : set if libreadline was found
	* READLINE_INCLUDE_DIR : libreadline include diretory
	* READLINE_LIBRARY : libreadline library file
#]]
find_path(READLINE_INCLUDE_DIR
	NAMES readline/readline.h
	HINTS ${READLINE_ROOT_DIR}
)

find_library(READLINE_LIBRARY
	NAMES readline
	HINTS ${READLINE_ROOT_DIR}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(readline REQUIRED_VARS READLINE_LIBRARY READLINE_INCLUDE_DIR)

mark_as_advanced(READLINE_FOUND READLINE_LIBRARY READLINE_INCLUDE_DIR)

if(READLINE_FOUND AND NOT TARGET readline)
	add_library(readline UNKNOWN IMPORTED)
	set_target_properties(readline PROPERTIES 
		IMPORTED_LOCATION "${READLINE_LIBRARY}"
		INTERFACE_INCLUDE_DIRECTORIES "${READLINE_INCLUDE_DIR}"
	)
endif()
