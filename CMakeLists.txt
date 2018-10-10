cmake_minimum_required (VERSION 3.0)

include ("project_configs.cmake")

project (${PROJECT_NAME})

add_subdirectory (externals/floral)
add_subdirectory (externals/helich)
add_subdirectory (externals/clover)
add_subdirectory (externals/refrain2)
add_subdirectory (externals/calyx)
add_subdirectory (externals/insigne)
add_subdirectory (externals/imgui)
add_subdirectory (externals/lotus)

include_directories ("${PROJECT_SOURCE_DIR}/src")

file (GLOB_RECURSE file_list
	"${PROJECT_SOURCE_DIR}/src/main.cpp"
	"${PROJECT_SOURCE_DIR}/src/Application.cpp"
	"${PROJECT_SOURCE_DIR}/src/Memory/MemorySystem.cpp"
	"${PROJECT_SOURCE_DIR}/src/System/Controller.cpp"
	"${PROJECT_SOURCE_DIR}/src/Graphics/SurfaceDefinitions.cpp"
	"${PROJECT_SOURCE_DIR}/src/Graphics/DebugDrawer.cpp"

	"${PROJECT_SOURCE_DIR}/src/Graphics/Tests/PlainQuad.cpp"
	"${PROJECT_SOURCE_DIR}/src/Graphics/Tests/PlainTextureQuad.cpp"
	"${PROJECT_SOURCE_DIR}/src/Graphics/Tests/CubeMapTexture.cpp"
	"${PROJECT_SOURCE_DIR}/src/Graphics/Tests/VectorMath.cpp"
	)

add_definitions(
	-D_CRT_SECURE_NO_WARNINGS)
	
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
	refrain2
	calyx
	insigne
	imgui
	lotus
	comctl32)

set_target_properties (${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${EXECUTABLE_FILE_NAME})
