cmake_minimum_required(VERSION 3.1)

project(launcher_utils_test)

add_library(launcher_utils_test SHARED
	vibration.cpp
	controller.cpp
)

target_compile_features(launcher_utils_test PUBLIC cxx_std_20)
set_target_properties(launcher_utils_test PROPERTIES CXX_EXTENSIONS OFF)

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

target_link_libraries(launcher_utils_test launcher_utils)

setup_geode_mod(launcher_utils_test)
