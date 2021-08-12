#include "nbt.h"

namespace {
    [[nodiscard]] vx3d::nbt::TagType read_type(vx3d::byte_buffer<vx3d::bit_endianness::big> &buffer) {
        return static_cast<vx3d::nbt::TagType>(buffer.read_u8());
    }

    [[nodiscard]] std::string read_string(vx3d::byte_buffer<vx3d::bit_endianness::big> &buffer) {
        const auto string_size = buffer.read_u16();
        auto data = std::vector<char>(string_size);
        for (auto i = 0; i < string_size; i++)
            data[i] = static_cast<char>(buffer.read_u8());

        auto string = std::string(string_size, '\0');
        for (auto i = 0; i < string_size; i++) string[i] = data[i];

        return string;
    }
}    // namespace

vx3d::nbt::node vx3d::nbt::node::read(byte_buffer &buffer) {
    return _read_node(buffer);
}

vx3d::nbt::node
vx3d::nbt::node::_read_node(byte_buffer &buffer, TagType parent, TagType list_type) {
    // We use end to signify that we're on the first node, it should only get called with `end`
    // in the first call, and never under (only COMPOUND, or LIST)
    auto value = node();
    if (parent == TagType::END) {
        value._type = ::read_type(buffer);
        value._name = ::read_string(buffer);
    } else if (parent == TagType::LIST) {
        value._type = list_type;
    } else {
        value._type = ::read_type(buffer);
        if (value._type == TagType::END) return value;

        value._name = ::read_string(buffer);
    }
    _read_value(buffer, value);

    return value;
}

void vx3d::nbt::node::_read_value(byte_buffer &buffer, vx3d::nbt::node &node) {
    switch (node._type) {
        case TagType::END:
            break;
        case TagType::BYTE:
            node._value = buffer.read_i8();
            break;
        case TagType::SHORT:
            node._value = buffer.read_i16();
            break;
        case TagType::INT:
            node._value = buffer.read_i32();
            break;
        case TagType::LONG:
            node._value = buffer.read_i64();
            break;
        case TagType::FLOAT:
            node._value = buffer.read_f32();
            break;
        case TagType::DOUBLE:
            node._value = buffer.read_f64();
            break;
        case TagType::BYTE_ARRAY: {
            const auto array_length = buffer.read_u16();
            auto array_data = std::vector<std::int8_t>(array_length);
            std::generate(array_data.begin(), array_data.end(), [&buffer](){ return buffer.read_u8(); });

            // Note - This CTOR doesn't only reserve, but also creates. Be careful...
            auto byte_vector = std::vector<std::int8_t>(array_length);
            for (auto i = 0; i < array_length; i++) byte_vector[i] = array_data[i];

            node._value = byte_vector;
            break;
        }
        case TagType::STRING:
            node._value = ::read_string(buffer);
            break;
        case TagType::LIST: {
            const auto child_type = static_cast<vx3d::nbt::TagType>(buffer.read_u8());
            const auto children_count = buffer.read_i32();
            auto children = std::vector<nbt::node>(children_count);

            for (auto i = 0; i < children_count; i++)
                children[i] = _read_node(buffer, TagType::LIST, child_type);

            node._value = std::move(children);
            break;
        }
        case TagType::COMPOUND: {
            auto children = std::vector<nbt::node>();
            auto current_node = nbt::node();

            while ((current_node = _read_node(buffer, TagType::COMPOUND))._type != TagType::END)
                children.push_back(current_node);

            node._value = std::move(children);
            break;
        }
        case TagType::INT_ARRAY: {
            const auto array_length = buffer.read_u16();
            auto array_data = std::vector<std::int32_t>(array_length);
            std::generate(array_data.begin(), array_data.end(), [&buffer](){ return buffer.read_i32(); });

            // Note - This CTOR doesn't only reserve, but also creates. Be careful...
            auto byte_vector = std::vector<std::int32_t>(array_length);
            for (auto i = 0; i < array_length; i++) byte_vector[i] = array_data[i];

            node._value = byte_vector;
            break;
        }
        case TagType::LONG_ARRAY: {
            const auto array_length = buffer.read_u16();
            auto array_data = std::vector<std::int64_t>(array_length);
            std::generate(array_data.begin(), array_data.end(), [&buffer](){ return buffer.read_i64(); });

            // Note - This CTOR doesn't only reserve, but also creates. Be careful...
            auto byte_vector = std::vector<std::int64_t>(array_length);
            for (auto i = 0; i < array_length; i++) byte_vector[i] = array_data[i];

            node._value = byte_vector;
            break;
        }
    }
}

const vx3d::nbt::node *vx3d::nbt::node::get_node(std::string_view value) const {
    // I do not like this function one bit... Too much C++ bullshit going on
    const node *node_ptr = nullptr;

    if (_type != TagType::COMPOUND) return node_ptr;    // Can't search by name

    const auto &nodes = std::get<std::vector<node>>(_value);

    for (const auto & node : nodes)
        if (std::string_view(node._name.value()) == value) node_ptr = &node;

    return node_ptr;
}