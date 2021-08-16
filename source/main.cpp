#include <ui/display.h>

int main() {
    auto world_loader = vx3d::world_loader();
    auto display = vx3d::ui::display(1920, 1080);
    auto renderer = vx3d::renderer();

    while (!display.should_close())
        display.render(world_loader, renderer);

    return 0;
}
