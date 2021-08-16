#pragma once

#include <ui/ui_components.h>
#include <loader/world_loader.h>
#include <renderer/renderer.h>
#include <util/opengl.h>
#include <glm/glm.hpp>

#include <cstdint>

#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace vx3d::ui
{
    class display
    {
    public:
        display(std::uint16_t width, std::uint16_t height);

        [[nodiscard]] bool should_close() const noexcept;

        void render(vx3d::world_loader &world_loader, vx3d::renderer &renderer);

    private:
        ImGui::FileBrowser _file_browser;

        std::uint16_t _width;
        std::uint16_t _height;

        GLFWwindow *_window;
    };
}    // namespace vx3d::ui