cmake_minimum_required(VERSION 3.19)

project(ArchimedesTemplate)

set(CMAKE_CXX_STANDARD 23)

#add_compile_options("/arch:AVX512")

# MSVC specific behavior
include("cmake/msvc.cmake")

# OS handling
include("cmake/os.cmake")

# Build mode handling
include("cmake/build_mode.cmake")

# Conan handling
include("cmake/conan.cmake")

# Ninja fixes
include("cmake/ninja_fix.cmake")

# Add library
include("cmake/library.cmake")

# #defines
include("cmake/defines.cmake")

# Launch functions
include("cmake/launch.cmake")

# Add targets
include("cmake/targets.cmake")

#target_include_directories(test archimedes_INCLUDE_DIRS)