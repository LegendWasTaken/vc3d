#pragma once

#include <filesystem>
#include <algorithm>
#include <fstream>
#include <cmath>

#include <glm/common.hpp>

#include <nbt/nbt.h>
#include <zlib-ng.h>

namespace vx3d::loader
{
    namespace detail
    {
        [[nodiscard]] inline std::vector<std::uint8_t> read_binary(const std::string &path)
        {
            std::ifstream file(path, std::ios::binary);

            // Stop eating new lines in binary mode!!!
            file.unsetf(std::ios::skipws);

            const auto empty = file.peek() == std::ifstream::traits_type::eof();

            // get its size:
            std::streampos fileSize;

            file.seekg(0, std::ios::end);
            fileSize = file.tellg();
            file.seekg(0, std::ios::beg);

            // reserve capacity
            std::vector<std::uint8_t> vec;
            vec.reserve(fileSize);

            // read the data:
            vec.insert(
              vec.begin(),
              std::istream_iterator<BYTE>(file),
              std::istream_iterator<BYTE>());

            return vec;
        }

        struct region_location
        {
            struct entry
            {
                int offset_0 : 3;
                int size_0 : 1;
                int offset_1 : 3;
                int size_1 : 1;
            };
        };

        struct chunk_location
        {
            std::uint8_t  size;
            std::uint32_t x;
            std::uint32_t z;
            std::uint32_t offset;
            std::uint32_t time_stamp;
        };

        inline std::array<chunk_location, 1024>
          read_data_table(const std::vector<std::uint8_t> &file_data)
        {
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

                const auto offset = ((bytes[0] << 16) | (bytes[1] << 8) | (bytes[2])) / 4;

                locations[i].time_stamp = (time_stamp_bytes[0] << 24) |
                  (time_stamp_bytes[1]) << 16 | (time_stamp_bytes[2] << 8) | time_stamp_bytes[3];
                locations[i].size   = bytes[3];
                locations[i].x      = offset & 31;
                locations[i].z      = offset >> 5;
                locations[i].offset = offset * 4;

                if (offset == 0 && locations[i].size == 0)
                {
                    locations[i].size       = std::numeric_limits<std::uint8_t>::max();
                    locations[i].x          = std::numeric_limits<std::uint32_t>::max();
                    locations[i].z          = std::numeric_limits<std::uint32_t>::max();
                    locations[i].time_stamp = std::numeric_limits<std::uint32_t>::max();
                    locations[i].offset     = std::numeric_limits<std::uint32_t>::max();
                }
            }

            return locations;
        }

        inline std::vector<std::array<std::uint32_t, 4096>>
          read_chunks(const std::vector<std::uint8_t> &chunks)
        {
            auto current_index = size_t(0);
//            while (true)
            {
                // Keep running
                const auto header = std::array<std::uint32_t, 5>({
                  chunks[current_index + 0],
                  chunks[current_index + 1],
                  chunks[current_index + 2],
                  chunks[current_index + 3],
                  chunks[current_index + 4]});

                const auto length =
                  (header[0] << 24) | (header[1]) << 16 | (header[2] << 8) | header[3];
                const auto compression_scheme = header[4];
                assert(compression_scheme == 2); // If it's not zlib... oh man

                auto chunk_data = std::vector<std::byte>(length);
                std::memcpy(chunk_data.data(), &chunks[current_index + 5], length);

                auto buffer = vx3d::byte_buffer<bit_endianness::big>(chunk_data.data(), length, true);
                auto node = nbt::node::read(buffer);

                const auto root = node.get<std::vector<nbt::node>>();


                const auto five = 5;

            }

            return {};
        }

        inline void read_region_file(const std::filesystem::path &file)
        {
            auto file_data      = read_binary(file.string());
            auto location_table = read_data_table(file_data);

            const auto chunk_data =
              std::vector<std::uint8_t>(file_data.begin() + 8192, file_data.end());
            const auto chunks = read_chunks(chunk_data);

            auto entry_regions = std::array<region_location::entry, 512>();
            auto after         = 0;
        }

    }    // namespace detail

    inline void load_world(const std::filesystem::path &directory)
    {
        const auto region_directory = directory / "region";

        auto region_files = std::vector<std::filesystem::path>();

        for (const auto &file : std::filesystem::directory_iterator(region_directory))
        {
            detail::read_region_file(file);
        }

        //        for (const auto &file : region_files)
        //            detail::read_region_file(file);

        const auto b = 5;
    }

}    // namespace vx3d::loader
