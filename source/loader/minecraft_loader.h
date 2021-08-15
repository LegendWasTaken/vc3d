#pragma once

#include <filesystem>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <cmath>

#include <zlib-ng.h>

#include <glm/common.hpp>
#include <daw/daw_memory_mapped_file.h>
#include <tracy/Tracy.hpp>

#include <thread_pool.h>
#include <nbt/nbt.h>

namespace vx3d::loader
{
    struct chunk_location
    {
        std::uint8_t  size;
        std::uint32_t offset;
        std::uint32_t time_stamp;
        std::int32_t x;
        std::int32_t z;

        [[nodiscard]] bool valid() const noexcept
        {
            return size != std::numeric_limits<std::uint8_t>::max() ||
              offset != std::numeric_limits<std::uint32_t>::max();
        }
    };

    inline std::array<chunk_location, 1024>
      read_data_table(const daw::filesystem::memory_mapped_file_t<std::uint8_t> &file_data)
    {
        ZoneScopedN("Loader::read_data_table");
        auto locations = std::array<chunk_location, 1024>();

        for (auto i = 0; i < 1024; i++)
        {
            const auto bytes = std::array<std::uint32_t, 4>({
              file_data[i * 4 + 0],
              file_data[i * 4 + 1],
              file_data[i * 4 + 2],
              file_data[i * 4 + 3],
            });

            const auto time_stamp_bytes = std::array<std::int32_t, 4>({
              file_data[i * 4 + 0 + 4096],
              file_data[i * 4 + 1 + 4096],
              file_data[i * 4 + 2 + 4096],
              file_data[i * 4 + 3 + 4096],
            });

            const auto offset = ((bytes[0] << 16) | (bytes[1] << 8) | (bytes[2]));

            locations[i].time_stamp = (time_stamp_bytes[0] << 24) | (time_stamp_bytes[1]) << 16 |
              (time_stamp_bytes[2] << 8) | time_stamp_bytes[3];
            locations[i].size   = bytes[3];
            locations[i].offset = offset;
            locations[i].x      = offset & 31;
            locations[i].z      = offset >> 5;

            if (offset == 0 || locations[i].size == 0)
            {
                locations[i].size       = std::numeric_limits<std::uint8_t>::max();
                locations[i].time_stamp = std::numeric_limits<std::uint32_t>::max();
                locations[i].offset     = std::numeric_limits<std::uint32_t>::max();
            }
        }

        return locations;
    }

    inline void read_chunk(
      const chunk_location &                                     location,
      const daw::filesystem::memory_mapped_file_t<std::uint8_t> &file)
    {
        ZoneScopedN("Loader::read_chunk");

        const auto index = location.offset * 4096;

        // Keep running
        const auto header = std::array<std::uint32_t, 5>(
          { file[index + 0], file[index + 1], file[index + 2], file[index + 3], file[index + 4] });

        const auto length = ((header[0] << 24) | (header[1]) << 16 | (header[2] << 8) | header[3]);
        const auto compression_scheme = header[4];
        if (compression_scheme != 2)    // If it's not zlib... oh man
            throw std::exception("Invalid compress scheme");

        auto chunk_data = std::vector<std::byte>(length - 1);
        std::memcpy(chunk_data.data(), &file[index + 5], length - 1);

        auto buffer = vx3d::byte_buffer<bit_endianness::big>(chunk_data.data(), length - 1, true);
        auto node   = nbt::node::read(buffer);
    }

    inline int read_region_file(const std::filesystem::path &file, vx3d::thread_pool *thread_pool)
    {
        ZoneScopedN("Loader::read_region_file");

        const auto file_handle =
          std::make_shared<daw::filesystem::memory_mapped_file_t<std::uint8_t>>(file.string());
        auto chunks_read = 0;
        if (file_handle->size() != 0)
        {
            const auto location_table = read_data_table(*file_handle);
            for (auto i = 0; i < location_table.size(); i++)
            {
                const auto location = location_table[i];
                if (location.valid())
                {
                    chunks_read++;
                    thread_pool->submit_task([location, file_handle]
                                             { read_chunk(location, *file_handle); });
                }
            }
        }
        return chunks_read;
    }

    inline void load_world(const std::filesystem::path &directory)
    {
        ZoneScopedN("Loader::load_world");
//        auto       thread_pool      = vx3d::thread_pool(16);
        const auto region_directory = directory / "region";

        auto region_files = std::vector<std::filesystem::path>();

        auto chunks_read = 0;
        for (const auto &file : std::filesystem::directory_iterator(region_directory))
            chunks_read += read_region_file(file, &thread_pool);

        //        for (const auto &file : region_files)
        //            detail::read_region_file(file);
        std::cout << chunks_read << std::endl;
    }

}    // namespace vx3d::loader
