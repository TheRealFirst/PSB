# PSB/CMakeLists.txt
cmake_minimum_required(VERSION 3.20)
project(PSB LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the logger submodule
add_subdirectory(dependencies/tinyLog)
# add_subdirectory(dependencies/dawn)

file(GLOB_RECURSE PSB_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

add_library(PSB STATIC ${PSB_SOURCES})

target_include_directories(PSB
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
	# ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/dawn/include
)

target_link_libraries(PSB
    PUBLIC
        tinyLog          # link with the logger
	# dawn
	# webgpu_dawn
)