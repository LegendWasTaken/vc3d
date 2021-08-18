#include "nbt.h"

namespace
{
    [[nodiscard]] vx3d::nbt::TagType read_type(vx3d::byte_buffer<vx3d::bit_endianness::big> &buffer)
    {
        return static_cast<vx3d::nbt::TagType>(buffer.read_u8());
    }

    [[nodiscard]] std::string_view read_string(vx3d::byte_buffer<vx3d::bit_endianness::big> &buffer)
    {
        const auto string_size = buffer.read_u16();
        return std::string_view(
          reinterpret_cast<char *>(buffer.at_and_increment(string_size)),
          string_size);
    }
}    // namespace

vx3d::nbt::node::node_list vx3d::nbt::node::read(byte_buffer &buffer)
{
    ZoneScopedN("nbt::node::read");
    auto nodes = std::make_unique<node[]>(buffer.size() / sizeof(vx3d::nbt::node));
    auto count = size_t(0);
    _parse_nbt(buffer, nodes, count);
    auto result = node_list();
    result.nodes = std::move(nodes);
    result.count = count;
    return std::move(result);
}

bool vx3d::nbt::node::_parse_nbt(
  byte_buffer &                       buffer,
  std::unique_ptr<vx3d::nbt::node[]> &nodes,
  size_t &                            count,
  TagType                             parent,
  TagType                             list_type)
{
    // We use end to signify that we're on the first node, it should only get called with `end`
    // in the first call, and never under (only COMPOUND, or LIST)
    auto value = node();
    if (parent == TagType::END)
    {
        value._type = ::read_type(buffer);
        value._name = ::read_string(buffer);
    }
    else if (parent == TagType::LIST)
    {
        value._type = list_type;
    }
    else
    {
        value._type = ::read_type(buffer);
        if (value._type == TagType::END)
        {
            nodes[count++] = value;
            return false;
        }

        value._name = ::read_string(buffer);
    }
    nodes[count++] = value;
    _read_value(buffer, nodes, count);
    return true;
}

void vx3d::nbt::node::_read_value(byte_buffer &buffer, std::unique_ptr<vx3d::nbt::node[]> &nodes, size_t &count)
{
    auto &node = nodes[count - 1];

    switch (node._type)
    {
    case TagType::END: break;
    case TagType::BYTE: node._value = buffer.at_and_increment(1); break;
    case TagType::SHORT: node._value = buffer.at_and_increment(2); break;
    case TagType::INT:
    case TagType::FLOAT: node._value = buffer.at_and_increment(4); break;
    case TagType::LONG:
    case TagType::DOUBLE: node._value = buffer.at_and_increment(8); break;
    case TagType::BYTE_ARRAY:
    {
        const auto array_length = buffer.read_i32();
        node._value             = buffer.at_and_increment(array_length * sizeof(std::uint8_t));
        ;
        break;
    }
    case TagType::STRING:
    {
        // Strings are a bit more complicated due to their nature.
        const auto size = buffer.read_u16();
        buffer.step_back(sizeof(std::uint16_t));
        node._value = buffer.at_and_increment(2 + size);
        break;
    }
    case TagType::LIST:
    {
        const auto child_type     = static_cast<vx3d::nbt::TagType>(buffer.read_u8());
        const auto children_count = buffer.read_i32();
        node._value               = nullptr;

        for (auto i = 0; i < children_count; i++)
            _parse_nbt(buffer, nodes, count, TagType::LIST, child_type);

        auto value  = vx3d::nbt::node();
        value._type = TagType::END;
        nodes[count++] = value;
        break;
    }
    case TagType::COMPOUND:
    {
        node._value = nullptr;

        while (_parse_nbt(buffer, nodes, count, TagType::COMPOUND))
            ;

        break;
    }
    case TagType::INT_ARRAY:
    {
        const auto array_length = buffer.read_i32();
        node._value             = buffer.at_and_increment(array_length * sizeof(std::int32_t));
        break;
    }
    case TagType::LONG_ARRAY:
    {
        const auto array_length = buffer.read_i32();
        node._value             = buffer.at_and_increment(array_length * sizeof(std::int64_t));
        break;
    }
    }
}

const vx3d::nbt::node *vx3d::nbt::node::get_node(std::string_view value) const
{
    // I do not like this function one bit... Too much C++ bullshit going on
    const node *node_ptr = nullptr;

    if (_type != TagType::COMPOUND) return node_ptr;    // Can't search by name

    //    const auto &nodes = std::get<std::vector<node>>(_value);

    //    for (const auto &node : nodes)
    //        if (std::string_view(node._name.value()) == value) node_ptr = &node;
    //
    return node_ptr;
}