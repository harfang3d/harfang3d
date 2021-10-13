add_executable(recastc main.cpp)

set_target_properties(recastc PROPERTIES FOLDER "harfang/tools")
target_link_libraries(recastc engine Recast Detour)
