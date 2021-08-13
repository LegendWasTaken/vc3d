#pragma once

#include <filesystem>
#include <algorithm>
#include <fstream>
#include <cmath>

#include <tracy/Tracy.hpp>
#include <glm/common.hpp>

#include <nbt/nbt.h>
#include <zlib-ng.h>
#include <iostream>

namespace vx3d::loader
{
    namespace detail
    {
        [[nodiscard]] inline std::vector<std::uint8_t> read_binary(const std::string &path)
        {
            ZoneScopedN("Loader::read_binary");
            std::ifstream file(path, std::ios::binary);

            file.unsetf(std::ios::skipws);

            const auto empty = file.peek() == std::ifstream::traits_type::eof();
            if (empty) return {};

            std::streampos fileSize;

            file.seekg(0, std::ios::end);
            fileSize = file.tellg();
            file.seekg(0, std::ios::beg);

            std::vector<std::uint8_t> vec;
            vec.reserve(fileSize);

            vec.insert(
              vec.begin(),
              std::istream_iterator<std::uint8_t>(file),
              std::istream_iterator<std::uint8_t>());

            return vec;
        }

        struct chunk_location
        {
            std::uint8_t  size;
            std::uint32_t offset;
            std::uint32_t time_stamp;

            [[nodiscard]] bool valid() const noexcept
            {
                return size != std::numeric_limits<std::uint8_t>::max() ||
                  offset != std::numeric_limits<std::uint32_t>::max();
            }
        };

        inline std::array<chunk_location, 1024>
          read_data_table(const std::vector<std::uint8_t> &file_data)
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

                locations[i].time_stamp = (time_stamp_bytes[0] << 24) |
                  (time_stamp_bytes[1]) << 16 | (time_stamp_bytes[2] << 8) | time_stamp_bytes[3];
                locations[i].size   = bytes[3];
                locations[i].offset = offset;

                if (offset == 0 || locations[i].size == 0)
                {
                    locations[i].size       = std::numeric_limits<std::uint8_t>::max();
                    locations[i].time_stamp = std::numeric_limits<std::uint32_t>::max();
                    locations[i].offset     = std::numeric_limits<std::uint32_t>::max();
                }
            }

            return locations;
        }

        inline void
          read_chunk(const chunk_location &location, const std::vector<std::uint8_t> &file)
        {
            ZoneScopedN("Loader::read_chunk");

            const auto index = location.offset * 4096;

            // Keep running
            const auto header = std::array<std::uint32_t, 5>({ file[index + 0],
                                                               file[index + 1],
                                                               file[index + 2],
                                                               file[index + 3],
                                                               file[index + 4] });

            const auto length =
              ((header[0] << 24) | (header[1]) << 16 | (header[2] << 8) | header[3]);
            const auto compression_scheme = header[4];
            if (compression_scheme != 2)    // If it's not zlib... oh man
                throw std::exception("Invalid compress scheme");

            auto chunk_data = std::vector<std::byte>(length - 1);
            std::memcpy(chunk_data.data(), &file[index + 5], length - 1);

            auto buffer =
              vx3d::byte_buffer<bit_endianness::big>(chunk_data.data(), length - 1, true);
            auto node = nbt::node::read(buffer);
        }

        inline std::vector<std::array<std::uint32_t, 4096>>
          read_chunks(const std::vector<std::uint8_t> &chunks)
        {
            ZoneScopedN("Loader::read_chunks");

            auto previous_length = 0;
            auto previous_index  = 0;

            auto current_index = size_t(0);
            while (current_index < chunks.size())
            {
                // Keep running
                const auto header = std::array<std::uint32_t, 5>({ chunks[current_index + 0],
                                                                   chunks[current_index + 1],
                                                                   chunks[current_index + 2],
                                                                   chunks[current_index + 3],
                                                                   chunks[current_index + 4] });

                const auto length =
                  ((header[0] << 24) | (header[1]) << 16 | (header[2] << 8) | header[3]);
                const auto compression_scheme = header[4];
                assert(compression_scheme == 2);    // If it's not zlib... oh man

                auto chunk_data = std::vector<std::byte>(length - 1);
                std::memcpy(chunk_data.data(), &chunks[current_index + 5], length - 1);

                auto buffer =
                  vx3d::byte_buffer<bit_endianness::big>(chunk_data.data(), length - 1, true);
                auto node = nbt::node::read(buffer);

                previous_length = length;
                previous_index  = current_index;

                current_index += 4096 * ((length / 4096) + 1);
            }

            return {};
        }

        inline void read_region_file(const std::filesystem::path &file)
        {
            ZoneScopedN("Loader::read_region_file");
            auto file_data = read_binary(file.string());
            auto chunks_read = 0;
            if (!file_data.empty())
            {
                const auto location_table = read_data_table(file_data);
                for (auto i = 0; i < location_table.size(); i++)
                {
                    const auto location = location_table[i];
                    if (location.valid())
                    {
                        read_chunk(location, file_data);
                        chunks_read++;
                    }
                }
            }
            std::cout << "Chunks read: " << chunks_read << std::endl;
            auto after = 0;
        }

    }    // namespace detail

    inline void load_world(const std::filesystem::path &directory)
    {
        ZoneScopedN("Loader::load_world");
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
