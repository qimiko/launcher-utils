cmake_minimum_required(VERSION 3.1)

project(launcher-utils-test)

add_library(launcher-utils-test SHARED
	vibration.cpp
	controller.cpp
)

target_compile_features(launcher-utils-test PUBLIC cxx_std_20)
set_target_properties(launcher-utils-test PROPERTIES CXX_EXTENSIONS OFF)

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

target_link_libraries(launcher-utils-test launcher-utils)

setup_geode_mod(launcher-utils-test)
