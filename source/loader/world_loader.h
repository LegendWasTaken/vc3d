#pragma once

#include <filesystem>
#include <thread_pool.h>
#include <tsl/robin_map.h>
#include <loader/minecraft_loader.h>
#include <tracy/Tracy.hpp>

namespace vx3d
{
    class world_loader
    {
    private:

        void _chunk_load(std::int32_t x, std::int32_t z);

        void _load_chunk_headers();

    public:
        [[nodiscard]] static std::uint64_t hash_pos(std::int32_t x, std::int32_t z);

        world_loader();

//        [[nodiscard]] bool get_chunk(std::int32_t x, std::int32_t z);

        [[nodiscard]] std::vector<loader::chunk_location>
          get_locations(const std::vector<loader::chunk_location> &locations);

        void set_world(const std::filesystem::path &world_folder);

    private:
        std::filesystem::path _world_folder;

        vx3d::thread_pool _thread_pool;

        std::mutex                          _loaded_chunks_mutex;
        tsl::robin_map<std::uint64_t, vx3d::loader::chunk_location> _loaded_chunk_headers;
    };
}    // namespace vx3d
