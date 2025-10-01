cmake_minimum_required(VERSION 3.1)

project(launcher_utils)

add_library(launcher_utils INTERFACE)

set_target_properties(launcher_utils PROPERTIES CXX_VISIBILITY_PRESET hidden)

target_sources(launcher_utils INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
)

if (PROJECT_IS_TOP_LEVEL)
	add_subdirectory(test)
endif()

target_include_directories(launcher_utils INTERFACE include)
