#include "renderer.h"

namespace
{
    [[nodiscard]] inline std::uint32_t hash_32b(int x, int z)
    {
        int hash = 17;
        hash     = 31 * hash + x;
        hash     = 31 * hash + z;
        return hash;
    }
}    // namespace

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
    ZoneScopedN("Renderer::render");
    const auto res = resolution - resolution % 2;

    glBindTexture(GL_TEXTURE_2D, _target_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, res.x, res.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
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
            delta *= current_zoom;

            current_translation.x += delta.x;
            current_translation.y += delta.y;
        }
    }

    const auto chunk_count = glm::ivec2(
      (std::int32_t((res.x + (16 - (res.x % 16)))) * current_zoom) / 16,
      (std::int32_t((res.y + (16 - (res.y % 16)))) * current_zoom) / 16);

    glUniform2i(glGetUniformLocation(_compute_program, "scene_size"), res.x, res.y);
    glUniform2i(glGetUniformLocation(_compute_program, "chunk_count"), chunk_count.x, chunk_count.y);
    glUniform1f(glGetUniformLocation(_compute_program, "zoom"), current_zoom);
    glUniform2f(
      glGetUniformLocation(_compute_program, "translation"),
      current_translation.x,
      current_translation.y);

    ZoneNamedN(a, "Renderer::render::load_chunks", true);

    // Request the chunks we're going to render
    auto chunks = std::vector<vx3d::loader::chunk_location>();
    chunks.reserve(chunk_count.x * chunk_count.y);

    const auto chunk_offset = glm::ivec2(current_translation) / 16;

    for (auto x = -chunk_count.x / 2; x < chunk_count.x / 2; x++)
        for (auto z = -chunk_count.y / 2; z < chunk_count.y / 2; z++)
            chunks.emplace_back(chunk_offset.x + x, chunk_offset.y + z);

    auto found = loader.get_locations(chunks);

    ZoneNamedN(b, "Renderer::render::create_indices", true);

    struct gpu_location_node
    {
        int x     = 0;
        int z     = 0;
        int empty = true;
        int pad = 0;
    };

    auto hash_map = std::vector<gpu_location_node>(found.size() * 1.7);
    for (const auto location : found)
    {
        auto gpu_location  = gpu_location_node();
        gpu_location.x     = location.x;
        gpu_location.z     = location.z;
        gpu_location.empty = false;

        auto hash = ::hash_32b(location.x, location.z);
        auto idx  = hash % hash_map.size();
        while (!hash_map[idx++].empty)
        {
            if (idx == hash_map.size())
                idx = 0;
        }
        hash_map[--idx] = gpu_location;
    }

    // Loaded chunks
    ZoneNamedN(d, "Renderer::render::upload_buffers", true)
      glBindBuffer(GL_SHADER_STORAGE_BUFFER, _chunk_buffer);
    glBufferData(
      GL_SHADER_STORAGE_BUFFER,
      hash_map.size() * sizeof(std::int32_t) * 4,
      hash_map.data(),
      GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _chunk_buffer);

    ZoneNamedN(c, "Renderer::render::compute", true) glDispatchCompute(
      static_cast<int>(glm::ceil(resolution.x / 8)),
      static_cast<int>(glm::ceil(resolution.y / 8)),
      1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

    return _target_texture;
}
