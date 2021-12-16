# Generates the Harfang API XML description used to generate the documentation.

add_custom_command(
 	OUTPUT
 		${CMAKE_CURRENT_BINARY_DIR}/harfang/api.xml
 	COMMAND
 		${Python3_EXECUTABLE} bind.py ${CMAKE_SOURCE_DIR}/binding/bind_harfang.py --xml --out ${CMAKE_CURRENT_BINARY_DIR}/harfang ${HG_BINDING_DEFINES}
 	MAIN_DEPENDENCY
 		${CMAKE_SOURCE_DIR}/binding/bind_harfang.py
 	WORKING_DIRECTORY
 		${HG_FABGEN_PATH}
 	COMMENT
 		"Generating Harfang API description file")

add_custom_target(gen_api_xml ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/harfang/api.xml)

# add_custom_command(
# 	OUTPUT
# 		${CMAKE_CURRENT_BINARY_DIR}/harfang/api.xml
# 	COMMAND
# 		${Python3_EXECUTABLE} bind.py ${CMAKE_CURRENT_SOURCE_DIR}/../binding/bind_harfang.py --xml --out ${CMAKE_CURRENT_BINARY_DIR}/harfang ${HG_BINDING_DEFINES}
# 	MAIN_DEPENDENCY
# 		${CMAKE_SOURCE_DIR}/binding/bind_harfang.py
# 	WORKING_DIRECTORY
# 		${HG_FABGEN_PATH}
# 	COMMENT
# 		"Generating Harfang API description file")

# offline docs
#configure_file(doc/index.html.in ${CMAKE_INSTALL_PREFIX}/offline_docs/index.html @ONLY IMMEDIATE)
#add_custom_target(offline_docs ALL
#		${Python3_EXECUTABLE} doc_to_html.py --project_name Harfang --doc_path doc --api_path ${CMAKE_CURRENT_BINARY_DIR}/harfang/api.xml --out_path ${CMAKE_INSTALL_PREFIX}/offline_docs --version ${HG_VERSION}
#	WORKING_DIRECTORY
#		${CMAKE_CURRENT_SOURCE_DIR}
#	DEPENDS 
#		${CMAKE_CURRENT_BINARY_DIR}/harfang/api.xml)
#install(DIRECTORY img DESTINATION offline_docs/content COMPONENT offline_docs)
#set_target_properties(offline_docs PROPERTIES FOLDER "harfang/doc")
