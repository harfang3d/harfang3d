add_executable(bulletc main.cpp)

set_target_properties(bulletc PROPERTIES FOLDER "harfang/tools")
target_link_libraries(bulletc engine BulletDynamics BulletWorldImporter)
