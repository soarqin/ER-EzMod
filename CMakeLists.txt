cmake_minimum_required(VERSION 3.13)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
project(ER_EzMod)

include(CustomCompilerOptions)
fix_compile_flags()
fix_release_flags()
add_static_runtime_option()
add_subdirectory(src)
