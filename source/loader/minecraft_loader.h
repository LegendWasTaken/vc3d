#pragma once

#include <filesystem>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <cmath>

#include <glm/common.hpp>

#include <tracy/Tracy.hpp>
#include <zlib-ng.h>

#include <nbt/nbt.h>
#include <thread_pool.h>
#include <Windows.h>

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
        [[nodiscard]] inline std::vector<std::uint8_t> read_binary_fopen(const std::string &path)
        {
            ZoneScopedN("Loader::read_binary_fopen");
            std::vector<uint8_t> result;

            FILE * file = fopen(path.c_str(), "rb");
            if (file == nullptr)
                return {};

            fseek(file, 0, SEEK_END);
            size_t const file_size = _ftelli64(file);
            fseek(file, 0, SEEK_SET);

            result.resize(file_size);
            fread(result.data(), sizeof(char), file_size, file);

            fclose(file);
            return result;
        }

        [[nodiscard]] inline std::vector<std::uint8_t> read_binary_windows_read(const std::string &path)
        {
            ZoneScopedN("Loader::read_binary_window_read");
            std::vector<uint8_t> result;
            HANDLE const file = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            assert(file != INVALID_HANDLE_VALUE);

            LARGE_INTEGER file_size;
            GetFileSizeEx(file, &file_size);

            result.resize(file_size.QuadPart);
            DWORD bytes_read = 0;
            ReadFile(file, result.data(), DWORD(file_size.QuadPart), &bytes_read, nullptr);
            CloseHandle(file);
            return result;
        }

        inline auto load_whole_filer(const std::string &path) -> std::vector<uint8_t>
        {
            ZoneScopedN("Loader::fstream_read");
            std::vector<uint8_t> result;
            auto file = std::ifstream(path, std::ios::binary);
            assert(file.is_open());
            file.seekg(0, std::ios::end);
            auto const file_size = file.tellg();
            file.seekg(0, std::ios::beg);
            result.resize(file_size);
            file.read(reinterpret_cast<char *>(result.data()), file_size);
            return result;
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

        inline void read_region_file(const std::filesystem::path &file, vx3d::thread_pool *thread_pool)
        {
            ZoneScopedN("Loader::read_region_file");

            const auto file_data_a = std::make_shared<std::vector<std::uint8_t>>(read_binary(file.string()));
            const auto file_data_b = std::make_shared<std::vector<std::uint8_t>>(load_whole_filer(file.string()));
            const auto file_data_c = std::make_shared<std::vector<std::uint8_t>>(read_binary_fopen(file.string()));
            const auto file_data_d = std::make_shared<std::vector<std::uint8_t>>(read_binary_windows_read(file.string()));
//            if (!file_data->empty())
//            {
//                const auto location_table = read_data_table(*file_data);
//                for (auto i = 0; i < location_table.size(); i++)
//                {
//                    const auto location = location_table[i];
//                    if (location.valid())
//                        thread_pool->submit_task([location, file_data]{
//                          read_chunk(location, *file_data);
//                        });
//                }
//            }
        }

    }    // namespace detail

    inline void load_world(const std::filesystem::path &directory)
    {
        ZoneScopedN("Loader::load_world");
        auto thread_pool = vx3d::thread_pool(16);
        const auto region_directory = directory / "region";

        auto region_files = std::vector<std::filesystem::path>();

        for (const auto &file : std::filesystem::directory_iterator(region_directory))
            detail::read_region_file(file, &thread_pool);

        //        for (const auto &file : region_files)
        //            detail::read_region_file(file);

        const auto b = 5;
    }

}    // namespace vx3d::loader
