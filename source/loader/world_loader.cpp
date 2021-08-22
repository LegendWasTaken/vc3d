#include "world_loader.h"

std::uint64_t vx3d::world_loader::hash_pos(std::int32_t x, std::int32_t z)
{
    return static_cast<std::uint64_t>(x) << 32 | (static_cast<std::uint64_t>(z) & 0xFFFFFFFF);
}

vx3d::world_loader::world_loader() : _thread_pool(0)
{
}

// bool vx3d::world_loader::get_chunk(std::int32_t x, std::int32_t z)
//{
//     auto guard = std::lock_guard(_loaded_chunks_mutex);
//     auto found = _loaded_chunk_headers.find(hash_pos(x, z)) != _loaded_chunk_headers.end();

//    if (!found) _thread_pool.submit_task([this, x, z] { _chunk_load(x, z); });

//    return found;
//}

void vx3d::world_loader::set_world(const std::filesystem::path &world_folder)
{
    _world_folder = world_folder;
    _load_chunk_headers();
}

void vx3d::world_loader::_chunk_load(std::int32_t x, std::int32_t z)
{
}

void vx3d::world_loader::_load_chunk_headers()
{
    ZoneScopedN("WorldLoader::load_chunk_headers");
    for (const auto &file : std::filesystem::directory_iterator(_world_folder / "region"))
    {
        const auto file_name = file.path().stem().string();
        auto       region_x  = std::int32_t(0);
        auto       region_z  = std::int32_t(0);
        std::sscanf(file_name.data(), "r.%d.%d.mcr", &region_x, &region_z);

        const auto mapped =
          daw::filesystem::memory_mapped_file_t<std::uint8_t>(file.path().string());

        if (mapped.size())
        {
            const auto chunk_locations = vx3d::loader::read_data_table(mapped);
            for (auto location : chunk_locations)
            {
                location.x += region_x;
                location.z += region_z;
                if (location.valid())
                    _loaded_chunk_headers.insert({ hash_pos(location.x, location.z), location });
            }
        }
    }
}

std::vector<vx3d::loader::chunk_location>
  vx3d::world_loader::get_locations(const std::vector<vx3d::loader::chunk_location> &locations)
{
    ZoneScopedN("WorldLoader::get_locations");
    auto found = std::vector<vx3d::loader::chunk_location>();
    found.reserve(locations.size());

    for (auto location : locations)
        if (const auto &at = _loaded_chunk_headers.find(hash_pos(location.x, location.z));
            at != _loaded_chunk_headers.end())
            found.push_back(location);

    return found;
}
