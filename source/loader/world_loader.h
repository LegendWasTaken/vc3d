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
        world_loader();

        [[nodiscard]] bool get_chunk(std::int32_t x, std::int32_t z);

        void set_world(const std::filesystem::path &world_folder);

    private:
        std::filesystem::path _world_folder;

        vx3d::thread_pool _thread_pool;

        std::mutex _loaded_chunks_mutex;
        std::unordered_set<std::uint64_t> _loaded_chunks;

        std::unordered_map<std::uint64_t, vx3d::loader::chunk_location> _loaded_chunk_headers;
    };
}
