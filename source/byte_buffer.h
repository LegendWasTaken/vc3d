#pragma once

#include <cstddef>
#include <memory>
#include <array>
#include <algorithm>
#include <zlib-ng.h>
#include <tracy/Tracy.hpp>

namespace vx3d
{
    enum class bit_endianness
    {
        little,
        big
    };

    template<bit_endianness endian>
    class byte_buffer
    {
    private:
        [[nodiscard]] inline std::uint16_t _bswapu16(std::uint16_t val) const noexcept;

        [[nodiscard]] inline std::uint32_t _bswapu32(std::uint32_t val) const noexcept;

        [[nodiscard]] inline std::uint64_t _bswapu64(std::uint64_t val) const noexcept;

        // This method will decompress the internal buffer
                void _decompress()
                {
                    ZoneScopedN("ByteBuffer::decompress")
                    auto decompressed_data = std::vector<std::byte>();

                    const std::uint32_t CHUNK = 16384 * 16;
                    int                 ret;
                    zng_stream          stream;
                    stream.zalloc                    = Z_NULL;
                    stream.zfree                     = Z_NULL;
                    stream.opaque                    = Z_NULL;
                    stream.avail_in                  = Z_NULL;
                    stream.next_in                   = Z_NULL;
                    std::uint32_t compressed_index   = 0;
                    std::uint32_t decompressed_index = 0;

                    ret = zng_inflateInit(&stream);
                    if (ret != Z_OK) throw std::logic_error("Failed to init inflate stream");

                    do {
                        stream.avail_in = glm::max(std::uint32_t(_size - compressed_index),
                        CHUNK); if (stream.avail_in == 0) break;

                        stream.next_in = reinterpret_cast<const uint8_t
                        *>(&_data[compressed_index]);

                        decompressed_data.resize(decompressed_index + stream.avail_in);
                        do {
                            stream.avail_out = CHUNK;
                            stream.next_out  =
                              reinterpret_cast<uint8_t *>(&decompressed_data[decompressed_index]);
                            ret              = zng_inflate(&stream, Z_NO_FLUSH);
                            if (ret == Z_STREAM_ERROR) throw std::logic_error("State Clobbered");
                            switch (ret)
                            {
                            case Z_NEED_DICT:
                            case Z_DATA_ERROR:
                            {
                                zng_inflateEnd(&stream);
                                throw std::logic_error("Invalid or incomplete deflate data");
                            }
                            case Z_MEM_ERROR:
                            {
                                zng_inflateEnd(&stream);
                                throw std::logic_error("Out of memory!");
                            }
                            }
                        } while (stream.avail_out == 0);
                    } while (ret != Z_STREAM_END);

                    zng_inflateEnd(&stream);
                    if (ret != Z_STREAM_END) throw std::logic_error("Invalid or incomplete deflate data");

                    _data = std::make_unique<std::byte[]>(decompressed_data.size());
                    _size = decompressed_data.size();
                    std::memcpy(_data.get(), decompressed_data.data(), decompressed_data.size());
                    decompressed_data.clear();
                }

    public:
        explicit byte_buffer(void *data, size_t size, bool compressed = false)
            : _compressed(compressed), _cursor(0), _size(size),
              _data(std::make_unique<std::byte[]>(size))
        {
            std::memcpy(_data.get(), data, _size);
                        if (_compressed)
                            _decompress();
        }

        [[nodiscard]] std::byte *at_and_increment(size_t size)
        {
            _cursor += size;
            return _data.get() + (_cursor - size);
        }

        void step_back(size_t count) { _cursor -= count; }

        [[nodiscard]] inline size_t size() const noexcept { return _size; }

        [[nodiscard]] inline std::uint8_t read_u8()
        {
            return static_cast<std::uint8_t>(_data[_cursor++]);
        }

        [[nodiscard]] inline std::int8_t read_i8() { return static_cast<std::int8_t>(read_u8()); }

        [[nodiscard]] inline std::uint16_t read_u16()
        {
            auto value = std::uint16_t(0);
            std::memcpy(&value, _data.get() + _cursor, 2);
            _cursor += 2;
            return _bswapu16(value);
        }

        [[nodiscard]] inline std::int16_t read_i16()
        {
            return static_cast<std::int16_t>(read_u16());
        }

        [[nodiscard]] inline std::uint32_t read_u32()
        {
            auto value = std::uint32_t(0);
            std::memcpy(&value, _data.get() + _cursor, 4);
            _cursor += 4;
            return _bswapu32(value);
        }

        [[nodiscard]] inline std::int32_t read_i32()
        {
            return static_cast<std::int32_t>(read_u32());
        }

        [[nodiscard]] inline std::uint64_t read_u64()
        {
            auto value = std::uint64_t(0);
            std::memcpy(&value, _data.get() + _cursor, 8);
            _cursor += 8;
            return _bswapu64(value);
        }

        [[nodiscard]] inline std::int64_t read_i64()
        {
            return static_cast<std::int64_t>(read_u64());
        }

        [[nodiscard]] inline float read_f32()
        {
            const auto as_int = read_u32();
            float      ret;
            std::memcpy(&ret, &as_int, 4);
            return ret;
        }

        [[nodiscard]] inline double read_f64()
        {
            const auto as_long = read_u64();
            double     ret;
            std::memcpy(&ret, &as_long, 8);
            return ret;
        }

        [[nodiscard]] inline std::int32_t read_var_int()
        {
            auto num_read = std::int32_t(0);
            auto result   = std::int32_t(0);
            auto read     = std::int8_t(0);
            do {
                read      = read_u8();
                int value = (read & 0b01111111);
                result |= (value << (7 * num_read));

                num_read++;
                if (num_read > 5) { std::terminate(); }
            } while ((read & 0b10000000) != 0);

            return result;
        }

        [[nodiscard]] inline std::int64_t read_var_long()
        {
            auto num_read = std::int32_t(0);
            auto result   = std::int64_t(0);
            auto read     = std::int8_t(0);
            do {
                read      = read_u8();
                int value = (read & 0b01111111);
                result |= (value << (7 * num_read));

                num_read++;
                if (num_read > 10) { std::terminate(); }
            } while ((read & 0b10000000) != 0);

            return result;
        }

        void reset() { _cursor = 0; }

    private:
        bool                         _compressed;
        size_t                       _cursor;
        size_t                       _size;
        std::unique_ptr<std::byte[]> _data;
    };

    template<>
    [[nodiscard]] inline std::uint16_t
      byte_buffer<bit_endianness::little>::_bswapu16(std::uint16_t val) const noexcept
    {
        return val;
    }

    template<>
    [[nodiscard]] inline std::uint16_t
      byte_buffer<bit_endianness::big>::_bswapu16(std::uint16_t val) const noexcept
    {
        return (val & 0x00'FF) << 8 | (val & 0xFF'00) >> 8;
    }

    template<>
    [[nodiscard]] inline std::uint32_t
      byte_buffer<bit_endianness::little>::_bswapu32(std::uint32_t val) const noexcept
    {
        return val;
    }

    template<>
    [[nodiscard]] inline std::uint32_t
      byte_buffer<bit_endianness::big>::_bswapu32(std::uint32_t val) const noexcept
    {
        return (val & 0x00'00'00'FF) << 24 | (val & 0x00'00'FF'00) << 8 |
          (val & 0x00'FF'00'00) >> 8 | (val & 0xFF'00'00'00) >> 24;
    }

    template<>
    [[nodiscard]] inline std::uint64_t
      byte_buffer<bit_endianness::little>::_bswapu64(std::uint64_t val) const noexcept
    {
        return val;
    }

    template<>
    [[nodiscard]] inline std::uint64_t
      byte_buffer<bit_endianness::big>::_bswapu64(std::uint64_t val) const noexcept
    {
        return (val & 0x00'00'00'00'00'00'00'FF) << 56 | (val & 0x00'00'00'00'00'00'FF'00) << 40 |
          (val & 0x00'00'00'00'00'FF'00'00) << 24 | (val & 0x00'00'00'00'FF'00'00'00) << 8 |
          (val & 0x00'00'00'FF'00'00'00'00) >> 8 | (val & 0x00'00'FF'00'00'00'00'00) >> 24 |
          (val & 0x00'FF'00'00'00'00'00'00) >> 40 | (val & 0xFF'00'00'00'00'00'00'00) >> 56;
    }

}    // namespace vx3d
