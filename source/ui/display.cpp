#include "display.h"

vx3d::ui::display::display(std::uint16_t width, std::uint16_t height) :
        _width(width), _height(height) {
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
    _file_browser.SetTypeFilters({".schematic", ".schem"});
}

bool vx3d::ui::display::should_close() const noexcept {
    return glfwWindowShouldClose(_window);
}

void vx3d::ui::display::render() const noexcept {
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

    _file_browser.Display();
    if (ImGui::BeginMenuBar()) {

        if (ImGui::BeginMenu("Load")) {

            if (ImGui::BeginMenu("Minecraft World")) {
                static auto current_directory = std::filesystem::temp_directory_path()
                        .parent_path()
                        .parent_path()
                        .parent_path() / "Roaming";
                if (std::filesystem::exists(current_directory / ".minecraft" / "saves")) {
                    static auto minecraft_directory = current_directory / ".minecraft" / "saves";

                    for (const auto &directory : std::filesystem::directory_iterator(minecraft_directory)) {
                        if (ImGui::MenuItem(directory.path().stem().string().c_str())) {
                            loader::load_world(directory);
                        }
                    }
                } else
                    ImGui::MenuItem("Couldn't find your minecraft installation");
                // List the minecraft worlds here


                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("Load Schematic"))
                _file_browser.Open();

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Export")) {

            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(_window);
    glfwPollEvents();
}
