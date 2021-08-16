#include "renderer.h"

vx3d::renderer::renderer()
{
    glGenTextures(1, &_target_texture);
    glBindTexture(GL_TEXTURE_2D, _target_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenBuffers(1, &_chunk_buffer);

    _compute_shader = vx3d::opengl::create_shader(
      std::string(VX3D_ASSET_PATH) + "shaders/texture_display.comp",
      GL_COMPUTE_SHADER);
    _compute_program = vx3d::opengl::create_program(_compute_shader);
}

GLuint vx3d::renderer::render(const glm::ivec2 &resolution, vx3d::world_loader &loader)
{
    glBindTexture(GL_TEXTURE_2D, _target_texture);
    glTexImage2D(
      GL_TEXTURE_2D,
      0,
      GL_RGBA8,
      resolution.x,
      resolution.y,
      0,
      GL_RGBA,
      GL_UNSIGNED_BYTE,
      nullptr);
    // Texture resized, now we need to render to it

    glUseProgram(_compute_program);
    glBindTexture(GL_TEXTURE_2D, _target_texture);
    glClearTexImage(_target_texture, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindImageTexture(0, _target_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);

    static auto current_zoom        = float(1.0f);
    static auto current_translation = glm::vec2(0.0f, 0.0f);
    if (ImGui::IsWindowHovered())
    {
        auto &io = ImGui::GetIO();
        current_zoom += io.MouseWheel * -0.05;

        if (ImGui::IsMouseDown(0))
        {
            auto delta = glm::vec2(io.MouseDelta.x, io.MouseDelta.y) * glm::vec2(-1, -1);
            delta.x /= resolution.x / 2;
            delta.y /= resolution.y / 2;
            delta *= current_zoom;

            current_translation.x += delta.x;
            current_translation.y += delta.y;
        }
    }

    glUniform2i(glGetUniformLocation(_compute_program, "scene_size"), resolution.x, resolution.y);
    glUniform1f(glGetUniformLocation(_compute_program, "zoom"), current_zoom);
    glUniform2f(
      glGetUniformLocation(_compute_program, "translation"),
      current_translation.x,
      current_translation.y);

    // A single chunk is 16x16 pixels, 1 pixel per block (theoretically)
    // This means, with a zoom of 1, the amount of chunks will be
    // ceil(pixel_size.x / 16) * ceil(pixel_size.y / 16)

    // With the origin of them being at 0,0 + translation
    // the start of the chunks will be that block, the chunk will be
    // chunk_x = translation.x >> 4
    // chunk_z = translation.y >> 4


    const auto &loaded = loader.loaded_chunks();
    auto indices = std::vector<std::int32_t>(resolution.x * resolution.y, -1);

    // When the zoom is under 16, there will be at least 1 block per chunk

    if (current_zoom < 16)
    {
        for (auto i = 0; i < loaded.size(); i++)
        {
            const auto location = loaded[i];

            const auto blocks_per_chunk = std::int8_t(16.0f / current_zoom);
            for (auto y = location.z * 16; y < location.z * 16 + blocks_per_chunk; y++)
                for (auto x = location.x * 16; x < location.x * 16 + blocks_per_chunk; x++)
                    indices[x + y * resolution.x] = i;
        }
    }
    else
    {
        // Send chunks per pixels instead of blocks

    }

    // Loaded chunks
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _chunk_buffer);
    glBufferData(
      GL_SHADER_STORAGE_BUFFER,
      loaded.size() * sizeof(std::int32_t) * 2,
      loaded.data(),
      GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _chunk_buffer);

    auto index_buffer = GLuint();
    glGenBuffers(1, &index_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, index_buffer);
    glBufferData(
      GL_SHADER_STORAGE_BUFFER,
      indices.size() * sizeof(std::int32_t),
      indices.data(),
      GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, index_buffer);

    glDispatchCompute(
      static_cast<int>(glm::ceil(resolution.x / 8)),
      static_cast<int>(glm::ceil(resolution.y / 8)),
      1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

    glDeleteBuffers(1, &index_buffer);

    return _target_texture;
}
