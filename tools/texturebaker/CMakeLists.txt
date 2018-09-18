cmake_minimum_required (VERSION 3.0)

include ("project_configs.cmake")

project (${PROJECT_NAME})

add_subdirectory("${PROJECT_SOURCE_DIR}/../../externals/floral" "floral")
add_subdirectory("${PROJECT_SOURCE_DIR}/../../externals/helich" "helich")
add_subdirectory("${PROJECT_SOURCE_DIR}/../../externals/clover" "clover")

include_directories ("${PROJECT_SOURCE_DIR}/src")

file (GLOB_RECURSE file_list
	"${PROJECT_SOURCE_DIR}/src/*.cpp")

add_definitions(
	-D_CRT_SECURE_NO_WARNINGS
	-DSTB_IMAGE_STATIC)
	
if (${ANDROID_BUILD})
	message(STATUS ${PROJECT_NAME} " will be built using Android configs")
	add_definitions (
		-DPLATFORM_POSIX)
		
	# platform abi
	if (${ANDROID_ABI} STREQUAL "arm64-v8a")
		message(STATUS ${PROJECT_NAME} " Android ABI: arm64")
		add_definitions (
			-DPOSIX64)
	else ()
		message(STATUS ${PROJECT_NAME} " Android ABI: arm")
		add_definitions (
			-DPOSIX32)
	endif (${ANDROID_ABI} STREQUAL "arm64-v8a")	
	
else ()
	message(STATUS ${PROJECT_NAME} " will be built using Windows configs")
	add_definitions (
		-DPLATFORM_WINDOWS)
endif (${ANDROID_BUILD})

add_executable (${PROJECT_NAME} ${file_list})

target_link_libraries (${PROJECT_NAME} 
	floral
	helich
	clover
	comctl32)

set_target_properties (${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${EXECUTABLE_FILE_NAME})
