#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <optional>
#include <vector>
#include <string_view>

#include <byte_buffer.h>

namespace vx3d::nbt
{
    enum class TagType : std::uint8_t
    {
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

    class node
    {
    public:
        using byte_buffer = vx3d::byte_buffer<vx3d::bit_endianness::big>;

        [[nodiscard]] const node *get_node(std::string_view value) const;

        struct node_list
        {
            size_t count;
            std::unique_ptr<vx3d::nbt::node[]> nodes;
        };
        [[nodiscard]] static node_list read(byte_buffer &buffer);

    private:
        static bool _parse_nbt(
          byte_buffer &          buffer,
          std::unique_ptr<node[]> &nodes,
          size_t &count,
          TagType                parent    = TagType::END,
          TagType                list_type = TagType::END);

        static void _read_value(byte_buffer &buffer, std::unique_ptr<vx3d::nbt::node[]> &nodes, size_t &count);

        TagType          _type;
        std::byte *      _value;
        std::string_view _name;

    public:
        template<typename T>
        [[nodiscard]] std::optional<std::reference_wrapper<T>> get()
        {
            //            if (T *ptr = std::get_if<T>(_value); ptr) { return *ptr; }
            return {};
        }
    };

}    // namespace vx3d::nbt