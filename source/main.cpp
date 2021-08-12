#include <ui/display.h>

#include <fstream>

int main() {
    auto display = vx3d::ui::display(1920, 1080);

    while (!display.should_close())
        display.render();

    return 0;
}
