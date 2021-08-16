#include "display.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
vx3d::ui::display::display(std::uint16_t width, std::uint16_t height)
    : _width(width), _height(height)
{
    ZoneScopedN("Display Creation");
    glfwInit();

    glfwInitHint(GLFW_VERSION_MAJOR, 4);
    glfwInitHint(GLFW_VERSION_MINOR, 5);
    glfwInitHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    _window = glfwCreateWindow(_width, _height, "vx3d", nullptr, nullptr);
    glfwMakeContextCurrent(_window);

    gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForOpenGL(_window, true);
    ImGui_ImplOpenGL3_Init("#version 450");

    _file_browser.SetTitle("Select Schematic");
    _file_browser.SetTypeFilters({ ".schematic", ".schem" });
}

bool vx3d::ui::display::should_close() const noexcept
{
    return glfwWindowShouldClose(_window);
}

void vx3d::ui::display::render(vx3d::world_loader &world_loader, vx3d::renderer &renderer)
{
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    auto viewport = ImGui::GetMainViewport();
    auto flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_MenuBar;

    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);

    ImGui::Begin("Hidden", nullptr, flags);

    const auto tab_input = ui::menu_tab_component(_file_browser);

    if (!tab_input.directory.empty()) world_loader.set_world(tab_input.directory);

    auto window_size = ImGui::GetContentRegionAvail();

//    auto loaded = world_loader.loaded_chunks();
//    loaded.emplace_back(0, 0);
//    loaded.emplace_back(1, 3);
//    loaded.emplace_back(1, 4);
    const auto texture = renderer.render(glm::ivec2(window_size.x, window_size.y), world_loader);

    ImGui::Image(reinterpret_cast<void *>(texture), ImVec2(1920, 1040));

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(_window);
    glfwPollEvents();
}
