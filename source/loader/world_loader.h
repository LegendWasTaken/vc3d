#pragma once

#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <thread_pool.h>
#include <loader/minecraft_loader.h>

namespace vx3d
{
    class world_loader
    {
    private:
        [[nodiscard]] static std::uint64_t hash_pos(std::int32_t x, std::int32_t z);

        void _chunk_load(std::int32_t x, std::int32_t z);

        void _load_chunk_headers();

    public:
        struct chunk_location
        {
            chunk_location() = default;
            chunk_location(std::int32_t x, std::int32_t z) : x(x), z(z) { }
            std::int32_t x = 0;
            std::int32_t z = 0;
        };

        world_loader();

        [[nodiscard]] bool get_chunk(std::int32_t x, std::int32_t z);

        void set_world(const std::filesystem::path &world_folder);

        [[nodiscard]] std::vector<chunk_location> loaded_chunks() const noexcept
        {
            auto value = std::vector<chunk_location>();
            value.emplace_back(0, 0);
            value.emplace_back(1, 1);
            value.emplace_back(2, 2);
            value.emplace_back(3, 3);
            return value;
        }

    private:
        std::filesystem::path _world_folder;

        vx3d::thread_pool _thread_pool;

        std::mutex                        _loaded_chunks_mutex;
        std::vector<chunk_location> _loaded_chunks;

        std::unordered_map<std::uint64_t, vx3d::loader::chunk_location> _loaded_chunk_headers;
    };
}    // namespace vx3d
