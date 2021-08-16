#pragma once

#include <util/opengl.h>
#include <loader/world_loader.h>

#include <glm/glm.hpp>
#include <imgui/imgui.h>

namespace vx3d
{
    class renderer
    {
    public:
        renderer();

        [[nodiscard]] GLuint render(const glm::ivec2 &resolution, vx3d::world_loader &loader);

    private:
        GLuint _compute_program;
        GLuint _compute_shader;
        GLuint _target_texture;
        GLuint _chunk_buffer;
    };
}
