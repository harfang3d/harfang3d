# Try to find OpenGLES3. Once done this will define:
#     OPENGLES3_FOUND
#     OPENGLES_VERSION
#     OPENGLES3_INCLUDE_DIRS
#     OPENGLES3_LIBRARIES

if(OPENGLES_VERSION)
    find_path(OPENGLES3_INCLUDE_DIRS NAMES GLES3/gl${OPENGLES_VERSION}.h
        HINTS ${PC_OPENGLES3_INCLUDEDIR} ${PC_OPENGLES3_INCLUDE_DIRS}
    )
else()
    foreach(minor_version "2" "1" "")
        find_path(OPENGLES3_INCLUDE_DIRS NAMES GLES3/gl3${minor_version}.h
            HINTS ${PC_OPENGLES3_INCLUDEDIR} ${PC_OPENGLES3_INCLUDE_DIRS}
        )
        if(OPENGLES3_INCLUDE_DIRS)
            set(OPENGLES_VERSION 3${minor_version})
            break()
        endif()
    endforeach(minor_version)
endif()

set(OPENGLES3_NAMES ${OPENGLES3_NAMES} GLESv3 GLESv2 libGLESv2)
find_library(OPENGLES3_LIBRARIES NAMES ${OPENGLES3_NAMES}
    HINTS ${PC_OPENGLES3_LIBDIR} ${PC_OPENGLES3_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenGLES3 REQUIRED_VARS OPENGLES3_INCLUDE_DIRS OPENGLES3_LIBRARIES
                                  FOUND_VAR OPENGLES3_FOUND)

mark_as_advanced(OPENGLES_VERSION OPENGLES3_INCLUDE_DIRS OPENGLES3_LIBRARIES)
