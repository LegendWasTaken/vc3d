#pragma once

#include <cstddef>
#include <cstdint>

namespace vx3d {
    typedef std::byte *&cursor;

    namespace cursor_read {
        [[nodiscard]] inline std::uint8_t read_u8(vx3d::cursor at) {
            auto val = at[0];
            at++;
            return val;
        }

        [[nodiscard]] inline std::uint16_t read_u16(vx3d::cursor at) {
            auto val =
        }

    }

}
