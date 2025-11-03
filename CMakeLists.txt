cmake_minimum_required(VERSION 3.20)
project(PSB LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(dependencies/tinyLog)

# GLFW static
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS   OFF CACHE BOOL "" FORCE)
add_subdirectory(dependencies/glfw)  # target: glfw

# WebGPU native import
set(WGPU_DIR "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/wgpu" CACHE PATH "Path to wgpu-native package (include/lib/bin)")
add_library(wgpu_native SHARED IMPORTED GLOBAL)
if (WIN32)
  set_target_properties(wgpu_native PROPERTIES
    IMPORTED_LOCATION "${WGPU_DIR}/bin/wgpu_native.dll"
    IMPORTED_IMPLIB   "${WGPU_DIR}/lib/wgpu_native.lib")
elseif(APPLE)
  set_target_properties(wgpu_native PROPERTIES
    IMPORTED_LOCATION "${WGPU_DIR}/lib/libwgpu_native.dylib")
else()
  # set_target_properties(wgpu_native PROPERTIES IMPORTED_LOCATION "${WGPU_DIR}/lib/libwgpu_native.so")
endif()
target_include_directories(wgpu_native INTERFACE "${WGPU_DIR}/include")

# Sources
file(GLOB_RECURSE PSB_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")

add_library(PSB STATIC ${PSB_SOURCES})

target_include_directories(PSB PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/dependencies/glm)   # glm is header-only

# Core deps for consumers (Sample inherits these)
target_link_libraries(PSB PUBLIC tinyLog wgpu_native glfw)

# Platform defines
if (WIN32)
  target_compile_definitions(PSB PUBLIC PSB_WIN32)
elseif(APPLE)
  target_compile_definitions(PSB PUBLIC PSB_MACOS)
elseif(UNIX)
  target_compile_definitions(PSB PUBLIC PSB_LINUX)
endif()

# Windows system libs needed by wgpu_native
if (WIN32)
  target_link_libraries(PSB PUBLIC
    user32 gdi32 shell32 ole32 oleaut32 uuid advapi32
    userenv        # GetUserProfileDirectoryW
    ntdll          # Nt* + RtlNtStatusToDosError
    ws2_32         # Winsock
    propsys        # VariantToPropVariant, PropVariantToBSTR
    runtimeobject  # RoOriginateErrorW
    opengl32       # wgl* referenced by GLES path
  )
endif()

# Runtime copy helper (call this from the executable's CMake)
function(PSB_copy_wgpu_runtime target)
  if (WIN32 OR APPLE)
    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:wgpu_native>" "$<TARGET_FILE_DIR:${target}>"
      COMMENT "[PSB] Copied wgpu_native runtime to $<TARGET_FILE_DIR:${target}>")
  endif()
endfunction()
