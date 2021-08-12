#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <optional>
#include <vector>
#include <string_view>

#include <byte_buffer.h>

namespace vx3d::nbt {

    enum class TagType : std::uint8_t {
        END,
        BYTE,
        SHORT,
        INT,
        LONG,
        FLOAT,
        DOUBLE,
        BYTE_ARRAY,
        STRING,
        LIST,
        COMPOUND,
        INT_ARRAY,
        LONG_ARRAY
    };

    class node {
    public:
        using byte_buffer = vx3d::byte_buffer<vx3d::bit_endianness::big>;

        [[nodiscard]] const node *get_node(std::string_view value) const;

        [[nodiscard]] static node read(byte_buffer &buffer);

    private:
        static node _read_node(
                byte_buffer &buffer,
                TagType parent = TagType::END,
                TagType list_type = TagType::END);

        static void _read_value(byte_buffer &buffer, node &node);

        TagType _type;
        std::variant<
                std::int8_t,
                std::int16_t,
                std::int32_t,
                std::int64_t,
                float,
                double,
                std::vector<std::int8_t>,
                std::string,
                std::vector<node>,
                std::vector<std::int32_t>,
                std::vector<std::int64_t>>
                _value;

        std::optional<std::string> _name;
    public:

        template<typename T>
        [[nodiscard]] std::optional<std::reference_wrapper<T>> get() {
            if (T *ptr = std::get_if<T>(_value); ptr) {
                return *ptr;
            }
            return {};
        }

    };

}