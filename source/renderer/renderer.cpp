#include "renderer.h"

namespace
{
    [[nodiscard]] inline std::uint32_t wang_hash(int seed)
    {
        seed = (seed ^ 61) ^ (seed >> 16);
        seed *= 9;
        seed = seed ^ (seed >> 4);
        seed *= 0x27d4eb2d;
        seed = seed ^ (seed >> 15);
        return seed;
    }

    [[nodiscard]] inline std::uint32_t hash_32b(int x, int z)
    {
        return wang_hash(x) ^ wang_hash(z);
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
    glGenBuffers(1, &_index_buffer);

    _compute_shader = vx3d::opengl::create_shader(
      std::string(VX3D_ASSET_PATH) + "shaders/texture_display.comp",
      GL_COMPUTE_SHADER);
    _compute_program = vx3d::opengl::create_program(_compute_shader);

    _uniform_scene_size  = glGetUniformLocation(_compute_program, "scene_size");
    _uniform_chunk_count = glGetUniformLocation(_compute_program, "chunk_count");
    _uniform_zoom        = glGetUniformLocation(_compute_program, "zoom");
    _uniform_translation = glGetUniformLocation(_compute_program, "translation");
}

GLuint vx3d::renderer::render(const glm::ivec2 &resolution, vx3d::world_loader &loader)
{
    ZoneScopedN("Renderer::render");
    const auto res = resolution - resolution % 2;

    static auto previous_res = res;
    static auto first        = true;
    if (previous_res != res || first)
    {
        first        = false;
        previous_res = res;
        // Texture resized, now we need to render to it
        glBindTexture(GL_TEXTURE_2D, _target_texture);
        glTexImage2D(
          GL_TEXTURE_2D,
          0,
          GL_RGBA8,
          res.x,
          res.y,
          0,
          GL_RGBA,
          GL_UNSIGNED_BYTE,
          nullptr);
        glClearTexImage(_target_texture, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    }

    glUseProgram(_compute_program);
    glBindImageTexture(0, _target_texture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
    glClearTexImage(_target_texture, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

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

    ZoneNamedN(z, "Renderer::render::update_uniforms", true);
    glUniform2i(_uniform_scene_size, res.x, res.y);
    glUniform2i(_uniform_chunk_count, chunk_count.x, chunk_count.y);
    glUniform1f(_uniform_zoom, current_zoom);
    glUniform2f(_uniform_translation, current_translation.x, current_translation.y);

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
        int pad   = 0;
    };

    struct hash_node_entry
    {
        int begin = -1;
        int end   = -1;
    };

    auto hash_map = std::vector<std::vector<gpu_location_node>>(found.size() * 1.7);
    for (const auto location : found)
    {
        auto gpu_location  = gpu_location_node();
        gpu_location.x     = location.x;
        gpu_location.z     = location.z;
        gpu_location.empty = false;
        gpu_location.pad   = rand();

        const auto hash = ::hash_32b(location.x, location.z);
        hash_map[hash % hash_map.size()].push_back(gpu_location);
    }

    auto flattened_hash_map = std::vector<gpu_location_node>();
    flattened_hash_map.reserve(found.size());
    auto hash_map_hash_indices = std::vector<hash_node_entry>(found.size());
    for (auto i = 0; i < hash_map_hash_indices.size(); i++)
    {
        const auto &current_map = hash_map[i];
        if (!current_map.empty())
        {
            const auto begin_index         = flattened_hash_map.size();
            const auto end_index           = begin_index + current_map.size();
            hash_map_hash_indices[i].begin = begin_index;
            hash_map_hash_indices[i].end   = end_index;

            for (const auto &location : current_map) flattened_hash_map.push_back(location);
        }
    }

    // Loaded chunks
    ZoneNamedN(d, "Renderer::render::upload_buffers", true);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _chunk_buffer);
    glBufferData(
      GL_SHADER_STORAGE_BUFFER,
      flattened_hash_map.size() * sizeof(std::int32_t) * 4,
      flattened_hash_map.data(),
      GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _chunk_buffer);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _index_buffer);
    glBufferData(
      GL_SHADER_STORAGE_BUFFER,
      hash_map_hash_indices.size() * sizeof(std::int32_t) * 2,
      hash_map_hash_indices.data(),
      GL_DYNAMIC_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _index_buffer);

    ZoneNamedN(c, "Renderer::render::compute", true);
    glDispatchCompute(
      static_cast<int>(glm::ceil(resolution.x / 32)),
      static_cast<int>(glm::ceil(resolution.y / 32)),
      1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

    return _target_texture;
}
