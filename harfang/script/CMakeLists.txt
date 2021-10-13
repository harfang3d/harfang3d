set(HDRS
	lua_vm.h)

set(SRCS
	lua_vm.cpp)

add_library(script STATIC ${SRCS} ${HDRS})
target_link_libraries(script PUBLIC foundation libluadll)
set_property(TARGET script PROPERTY PUBLIC_HEADER ${HDRS})
set_target_properties(script PROPERTIES FOLDER "harfang")

if(WIN32)
#target_compile_options(script PRIVATE /Od)  # fix sonovision POC throw to interrupt a scenario
endif()

install_cppsdk_target(script)
