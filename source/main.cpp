#include <ui/display.h>

int main() {
    auto world_loader = vx3d::world_loader();
    auto display = vx3d::ui::display(1920, 1080);
    auto renderer = vx3d::renderer();

    while (!display.should_close())
        display.render(world_loader, renderer);

//    auto file = daw::filesystem::memory_mapped_file_t<uint8_t>("./raw_chunk (1).nbt");
//    auto file = daw::filesystem::memory_mapped_file_t<uint8_t>("./test.nbt");
//    auto buffer = vx3d::byte_buffer<vx3d::bit_endianness::big>(file.data(), file.size());

//    for (auto i = 0; i < 1000000; i++)
//    {
//        auto node = vx3d::nbt::node::read(buffer);
//
//        buffer.reset();
//    }

    return 0;
}
