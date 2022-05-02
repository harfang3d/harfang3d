# Copyright (C) Movida Production 2020
#[=======================================================================[.rst:
FindFBXSDK
-------

Locate the Autodesk FBX SDK.

First set ``FBXSDK_ROOT_DIR`` to the FBX SDK installation root directory.
If for some reasons, the FBX SDK library could not be found, try setting ``FBXSDK_LIBRARY_PREFIX```
containing the library files (for example ${FBXSDK_ROOT_DIR}\FBX\2020.1.1\lib\vs2017\arm64)

Imported targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` target:

``fbxsdk``
  The FBX SDK library, if found

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables in your project:

``FBXSDK_INCLUDE_DIRECTORY``
  where to find fbxsdk.h
``FBXSDK_LIBRARY``
  the name of the library to link against
``FBXSDK_FOUND``
  if false, do not try to link to FBX SDK
``FBXSDK_VERSION``
  the human-readable string containing the version of FBX_SDK if found
``FBXSDK_VERSION_MAJOR``
  the major version of the FBX SDK
``FBXSDK_VERSION_MINOR``
  the minor version of the FBX SDK
``FBXSDK_VERSION_POINT``
    the patch version of the FBX SDK

#]=======================================================================]

if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
    set(FBXSDK_PLATFORM "x64")
else()
    set(FBXSDK_PLATFORM "x86")
endif()

if(NOT FBXSDK_ROOT_DIR)
	message(FATAL_ERROR "Please set FBXSDK_ROOT_DIR to the path where the Autodesk FBX SDK is installed.")
endif()

if(NOT FBXSDK_LIBRARY_PREFIX)
    # If this fails try setting FBXSDK_LIBRARY_PREFIX beforehand.
    file(GLOB FBXSDK_LIBRARY_PREFIX LIST_DIRECTORIES true "${FBXSDK_ROOT_DIR}/lib/*/${FBXSDK_PLATFORM}")
    list(SORT FBXSDK_LIBRARY_PREFIX CASE INSENSITIVE ORDER DESCENDING)
    list(GET FBXSDK_LIBRARY_PREFIX 0 FBXSDK_LIBRARY_PREFIX)
endif()

find_path(FBXSDK_INCLUDE_DIRECTORY NAMES "fbxsdk.h" PATH_SUFFIXES "include" HINTS "${FBXSDK_ROOT_DIR}" NO_DEFAULT_PATH)

mark_as_advanced(FBXSDK_INCLUDE_DIRECTORY)

if(FBXSDK_INCLUDE_DIRECTORY AND EXISTS "${FBXSDK_INCLUDE_DIRECTORY}/fbxsdk/fbxsdk_version.h")
    file(READ "${FBXSDK_INCLUDE_DIRECTORY}/fbxsdk/fbxsdk_version.h" FBXSDK_VERSION_HEADER)

    string(REGEX REPLACE "^.*#define[ |\t]+FBXSDK_VERSION_MAJOR[ |\t]+([0-9]+).*$" "\\1" FBXSDK_VERSION_MAJOR ${FBXSDK_VERSION_HEADER})
    string(REGEX REPLACE "^.*#define[ |\t]+FBXSDK_VERSION_MINOR[ |\t]+([0-9]+).*$" "\\1" FBXSDK_VERSION_MINOR ${FBXSDK_VERSION_HEADER})
    string(REGEX REPLACE "^.*#define[ |\t]+FBXSDK_VERSION_POINT[ |\t]+([0-9]+).*$" "\\1" FBXSDK_VERSION_POINT ${FBXSDK_VERSION_HEADER})

    set(FBXSDK_VERSION "${FBXSDK_VERSION_MAJOR}.${FBXSDK_VERSION_MINOR}.${FBXSDK_VERSION_POINT}")
    set(FBXSDK_VERSION_STRING "${FBXSDK_VERSION}")

    unset(FBXSDK_VERSION_HEADER)
endif()

find_library(FBXSDK_LIBRARY_DEBUG NAMES fbxsdk libfbxsdk HINTS "${FBXSDK_LIBRARY_PREFIX}/debug" NO_DEFAULT_PATH)
find_library(FBXSDK_LIBRARY_RELEASE NAMES fbxsdk libfbxsdk HINTS "${FBXSDK_LIBRARY_PREFIX}/release" NO_DEFAULT_PATH)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    find_file(FBXSDK_SHARED_LIBRARY_DEBUG NAME "libfbxsdk${CMAKE_SHARED_LIBRARY_SUFFIX}" HINTS "${FBXSDK_LIBRARY_PREFIX}/debug" NO_DEFAULT_PATH)
    find_file(FBXSDK_SHARED_LIBRARY_RELEASE NAME "libfbxsdk${CMAKE_SHARED_LIBRARY_SUFFIX}" HINTS "${FBXSDK_LIBRARY_PREFIX}/release" NO_DEFAULT_PATH)
endif()

include(SelectLibraryConfigurations)
select_library_configurations(FBXSDK)
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(FBXDSK_SHARED_LIBRARY "")
    list(APPEND FBXDSK_SHARED_LIBRARY optimized ${FBXSDK_LIBRARY_RELEASE})
    list(APPEND FBXDSK_SHARED_LIBRARY debug ${FBXSDK_SHARED_LIBRARY_DEBUG})
    mark_as_advanced(FBXSDK_SHARED_LIBRARY_DEBUG FBXSDK_SHARED_LIBRARY_RELEASE FBXDSK_SHARED_LIBRARY)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FBXSDK 
    REQUIRED_VARS FBXSDK_LIBRARY FBXSDK_INCLUDE_DIRECTORY
    VERSION_VAR FBXSDK_VERSION_STRING
)

set(FBX_SDK_LIBRARIES ${FBX_SDK_LIBRARY})

if(FBXSDK_FOUND AND NOT TARGET fbxsdk)
    add_library(fbxsdk SHARED IMPORTED)
    set_target_properties(fbxsdk PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${FBXSDK_INCLUDE_DIRECTORY}")
        
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        if(FBXSDK_LIBRARY_RELEASE)
            set_property(TARGET fbxsdk APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
            set_target_properties(fbxsdk PROPERTIES
                IMPORTED_LOCATION_RELEASE ${FBXSDK_SHARED_LIBRARY_RELEASE}
                IMPORTED_IMPLIB_RELEASE ${FBXSDK_LIBRARY_RELEASE}
            )
        endif()

        if(FBXSDK_LIBRARY_DEBUG)
            set_property(TARGET fbxsdk APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
            set_target_properties(fbxsdk PROPERTIES
                IMPORTED_LOCATION_DEBUG ${FBXSDK_SHARED_LIBRARY_DEBUG}
                IMPORTED_IMPLIB_DEBUG ${FBXSDK_LIBRARY_DEBUG}
            )
        endif()

        if(NOT FBXSDK_LIBRARY_RELEASE AND NOT FBXSDK_LIBRARY_DEBUG)
            set_property(TARGET fbxsdk APPEND PROPERTY IMPORTED_LOCATION "${FBXSDK_SHARED_LIBRARY}")
            set_property(TARGET fbxsdk APPEND PROPERTY IMPORTED_IMPLIB "${FBXSDK_LIBRARY}")
        endif()
    else()
        if(FBXSDK_LIBRARY_RELEASE)
            set_property(TARGET fbxsdk APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
            set_target_properties(fbxsdk PROPERTIES IMPORTED_LOCATION_RELEASE "${FBXSDK_LIBRARY_RELEASE}")
        endif()

        if(FBXSDK_LIBRARY_DEBUG)
            set_property(TARGET fbxsdk APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
            set_target_properties(fbxsdk PROPERTIES IMPORTED_LOCATION_DEBUG "${FBXSDK_LIBRARY_DEBUG}")
        endif()

        if(NOT FBXSDK_LIBRARY_RELEASE AND NOT FBXSDK_LIBRARY_DEBUG)
            set_property(TARGET fbxsdk APPEND PROPERTY IMPORTED_LOCATION "${FBXSDK_LIBRARY}")
        endif()

	if(UNIX AND NOT APPLE)
		set_target_properties(fbxsdk PROPERTIES IMPORTED_LINK_INTERFACE_LIBRARIES LibXml2::LibXml2)
	endif()
    endif()
endif()
