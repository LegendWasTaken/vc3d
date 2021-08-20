#pragma once

#include <util/opengl.h>
#include <loader/world_loader.h>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>
#include <imgui/imgui.h>

#include <iostream>

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
