cmake_minimum_required(VERSION 3.4.1)

# building platform
if (${ANDROID_BUILD})
	message(STATUS "Building for Android Platform")
	message(STATUS "Android API level: ${ANDROID_PLATFORM_LEVEL}")
	
	if (${ANDROID_PLATFORM_LEVEL} LESS 12)
		message(FATAL_ERROR "GLES3 does not present with Android API level <= 11.")
		return()
	elseif (${ANDROID_PLATFORM_LEVEL} LESS 18)
		message(STATUS "GLES3 will be dynamic initialized with Android API level 12 -> 17")
		add_definitions("-DDYNAMIC_ES3")
		set(OPENGL_LIB GLESv2)
	else ()
		message(STATUS "Building using API level >= 18")
		set(OPENGL_LIB GLESv3)
	endif (${ANDROID_PLATFORM_LEVEL} LESS 12)
	
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 -Wall")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti -fno-exceptions -Wall")
endif (${ANDROID_BUILD})


include ("${PROJECT_SOURCE_DIR}/project_configs.cmake")

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
	"${PROJECT_SOURCE_DIR}/src/Graphics/DebugDrawer.cpp"
	"${PROJECT_SOURCE_DIR}/src/Graphics/TrackballCamera.cpp"
	"${PROJECT_SOURCE_DIR}/src/Graphics/GeometryBuilder.cpp"
	"${PROJECT_SOURCE_DIR}/src/Graphics/SurfaceDefinitions.cpp"

	"${PROJECT_SOURCE_DIR}/src/Graphics/Tests/PlainQuad.cpp"
	"${PROJECT_SOURCE_DIR}/src/Graphics/Tests/SHBaking.cpp"
	"${PROJECT_SOURCE_DIR}/src/Graphics/Tests/SHBakingUtils.cpp"
	"${PROJECT_SOURCE_DIR}/src/Graphics/Tests/FormFactorsBaking.cpp"
	"${PROJECT_SOURCE_DIR}/src/Graphics/Tests/FormFactorsValidating.cpp"
	# "${PROJECT_SOURCE_DIR}/src/Graphics/Tests/PlainTextureQuad.cpp"
	# "${PROJECT_SOURCE_DIR}/src/Graphics/Tests/CubeMapTexture.cpp"
	# "${PROJECT_SOURCE_DIR}/src/Graphics/Tests/VectorMath.cpp"
	# "${PROJECT_SOURCE_DIR}/src/Graphics/Tests/GPUVectorMath.cpp"
	# "${PROJECT_SOURCE_DIR}/src/Graphics/Tests/CornelBox.cpp"
	# "${PROJECT_SOURCE_DIR}/src/Graphics/Tests/OmniShadow.cpp"
	# "${PROJECT_SOURCE_DIR}/src/Graphics/Tests/SHMath.cpp"

	# "${PROJECT_SOURCE_DIR}/src/Graphics/Tests/GlobalIllumination.cpp"
	# "${PROJECT_SOURCE_DIR}/src/Graphics/Tests/GIShader.cpp"
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
	
	set (file_list
		${file_list}
		"externals/android/app/src/main/cpp/jnibridge.cpp")
		
	add_library(${PROJECT_NAME} SHARED ${file_list})

	# Include libraries needed for gles3jni lib
	target_link_libraries(${PROJECT_NAME}
		floral
		helich
		clover
		refrain2
		calyx
		insigne
		imgui
		lotus
		${OPENGL_LIB}
		android
		log
		m)
else ()
	# project (${PROJECT_NAME})
	message(STATUS ${PROJECT_NAME} " will be built using Windows configs")
	add_definitions (
		-DPLATFORM_WINDOWS)

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
endif (${ANDROID_BUILD})
