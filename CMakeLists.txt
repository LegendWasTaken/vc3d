cmake_minimum_required(VERSION 3.19)
project(vx3d)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(external)

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
        source/cursor.h source/byte_buffer.h)

target_include_directories(vx3d PUBLIC source external)
target_link_libraries(vx3d PUBLIC glfw zlib glm)
target_compile_definitions(vx3d PUBLIC -DGLFW_INCLUDE_NONE)