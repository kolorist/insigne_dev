# 1. required version
cmake_minimum_required(VERSION 3.20min)

# 2. initial setup
set (PROJECT_NAME "modelbaker")
project (${PROJECT_NAME})
include ("${PROJECT_SOURCE_DIR}/project_configs.cmake")
message (STATUS "Project: ${PROJECT_NAME}")
message (STATUS "Executable: ${EXECUTABLE_FILE_NAME}.exe")
message (STATUS "Project source directory: ${PROJECT_SOURCE_DIR}")
message (STATUS "Project binary directory: ${PROJECT_BINARY_DIR}")

# 3. target platform
if (${TARGET_PLATFORM} STREQUAL "arm64-v8a"
	OR ${TARGET_PLATFORM} STREQUAL "arm-v7a"
	OR ${TARGET_PLATFORM} STREQUAL "x86"
	OR ${TARGET_PLATFORM} STREQUAL "x64")
	message (STATUS "Target platform: ${TARGET_PLATFORM}")
else ()
	message (FATAL_ERROR "Target platform: no platform specified or you are using not supported target platform, please choose one of (arm64-v8a, arm-v7a, x86, x64)")
	return()
endif ()

# 4. build commands output
if (${USE_MSVC_PROJECT})
	message (STATUS "MSVC Solution / Project structure will be generated")
endif ()

# 5.1 file listing
if (${USE_MSVC_PROJECT})
	file (GLOB_RECURSE file_list
		LIST_DIRECTORIES false
		"${PROJECT_SOURCE_DIR}/src/*.cpp"
		"${PROJECT_SOURCE_DIR}/src/*.c"
		"${PROJECT_SOURCE_DIR}/src/*.h")
else ()
	file (GLOB_RECURSE file_list
		LIST_DIRECTORIES false
		"${PROJECT_SOURCE_DIR}/src/*.c"
		"${PROJECT_SOURCE_DIR}/src/*.cpp")
endif ()

# 5.2 exclude file according to platform
if (${TARGET_PLATFORM} STREQUAL "arm64-v8a" OR ${TARGET_PLATFORM} STREQUAL "arm-v7a")

else ()

endif ()

# 6. platform specific compiling
if (${TARGET_PLATFORM} STREQUAL "arm64-v8a")

elseif (${TARGET_PLATFORM} STREQUAL "arm-v7a")

elseif (${TARGET_PLATFORM} STREQUAL "x86")
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
elseif (${TARGET_PLATFORM} STREQUAL "x64")
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif ()
include_directories("${PROJECT_SOURCE_DIR}/src")

# 7. subdirectories
add_subdirectory("${PROJECT_SOURCE_DIR}/../../externals/floral" "floral")
add_subdirectory("${PROJECT_SOURCE_DIR}/../../externals/helich" "helich")
add_subdirectory("${PROJECT_SOURCE_DIR}/../../externals/clover" "clover")

# 8. platform specific linking
if (${TARGET_PLATFORM} STREQUAL "arm64-v8a" OR ${TARGET_PLATFORM} STREQUAL "arm-v7a")

else ()
	add_executable (${PROJECT_NAME} ${file_list})
	target_link_libraries (${PROJECT_NAME} 
		floral
		helich
		clover)
	set (CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -Xlinker /subsystem:console")
	set_target_properties (${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${EXECUTABLE_FILE_NAME})
endif ()

# 9. C and CXX compile options
if (${TARGET_PLATFORM} STREQUAL "arm64-v8a" OR ${TARGET_PLATFORM} STREQUAL "arm-v7a")
	target_compile_options(${PROJECT_NAME}
		PUBLIC	$<$<COMPILE_LANGUAGE:CXX>:-std=c++11 -Wall -fno-rtti -fno-exceptions>
		PUBLIC	$<$<COMPILE_LANGUAGE:C>:-std=c11 -Wall>)
else ()
	# we will c++14 on Windows build as c++11 yields compile errors
	target_compile_options(${PROJECT_NAME}
		PUBLIC	$<$<COMPILE_LANGUAGE:CXX>:-std=c++14 -Wall -fno-rtti -fno-exceptions>
		PUBLIC	$<$<COMPILE_LANGUAGE:C>:-std=c11 -Wall>)
endif ()

# 10. misc
if (${USE_MSVC_PROJECT})
	# organize filters
	foreach(_source IN ITEMS ${file_list})
		get_filename_component(_source_path "${_source}" PATH)
		file(RELATIVE_PATH _source_path_rel "${PROJECT_SOURCE_DIR}" "${_source_path}")
		string(REPLACE "/" "\\" _group_path "${_source_path_rel}")
		source_group("${_group_path}" FILES "${_source}")
	endforeach()
	# TODO: startup project and working directory
endif ()
