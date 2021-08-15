cmake_minimum_required(VERSION 3.19)
project(vx3d)

option(VX3D_USE_TRACY "" OFF)
option(VX3D_USE_RELATIVE_PATH "" OFF)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(external)

if (CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC" AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(STATUS "Using clang-cl options (MSVC interface)")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GR- /arch:AVX /MT /W4 /EHa /EHs")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(STATUS "Using Clang/GCC compiler options")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
    if ((CMAKE_BUILD_TYPE MATCHES Release) OR (CMAKE_BUILD_TYPE MATCHES RelWithDbgInfo))
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
    endif ()
endif ()

add_executable(vx3d
        source/main.cpp

        source/glad/glad.c

        source/ui/display.cpp

        source/imgui/imgui.cpp
        source/imgui/imgui_draw.cpp
        source/imgui/imgui_impl_glfw.cpp
        source/imgui/imgui_impl_opengl3.cpp
        source/imgui/imgui_tables.cpp
        source/imgui/imgui_widgets.cpp

        source/loader/minecraft_loader.h
        source/nbt/nbt.cpp
        source/cursor.h source/byte_buffer.h

        source/tracy/TracyClient.cpp
        source/thread_pool.cpp source/thread_pool.h source/loader/world_loader.cpp source/loader/world_loader.h source/util/opengl.h)

target_include_directories(vx3d PUBLIC source external)
target_link_libraries(vx3d PUBLIC glfw zlib glm)

if (VX3D_USE_TRACY)
    message("Tracy has been enabled")
    set(VX3D_TRACY_MACRO -DTRACY_ENABLE)
else ()
    set(VX3D_TRACY_MACRO "")
endif ()

if (VX3D_USE_RELATIVE_PATH)
    message("Using relative asset path")
    set(VX3D_RELATIVE_PATH "./assets/")
else()
    message("Using none-relative asset path")
    set(VX3D_RELATIVE_PATH "${CMAKE_SOURCE_DIR}/assets/")
endif()

target_compile_definitions(vx3d PUBLIC -D__STDC_CONSTANT_MACROS -DGLFW_INCLUDE_NONE ${VX3D_TRACY_MACRO} -DNOMINMAX VX3D_ASSET_PATH="${VX3D_RELATIVE_PATH}")