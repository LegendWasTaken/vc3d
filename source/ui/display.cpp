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

    glGenTextures(1, &_display_texture);
    glBindTexture(GL_TEXTURE_2D, _display_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(
      GL_TEXTURE_2D,
      0,
      GL_RGBA8,
      1920,
      1040,
      0,
      GL_RGBA,
      GL_UNSIGNED_BYTE,
      nullptr);

    glGenTextures(1, &_temp);
    glBindTexture(GL_TEXTURE_2D, _temp);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    auto dimensions = glm::ivec3();
    auto data = stbi_load((std::string(VX3D_ASSET_PATH) + "new_reference.png").c_str(), &dimensions.x, &dimensions.y, &dimensions.z, 4);
    glTexImage2D(
      GL_TEXTURE_2D,
      0,
      GL_RGBA8,
      dimensions.x,
      dimensions.y,
      0,
      GL_RGBA,
      GL_UNSIGNED_BYTE,
      data);

    _compute_shader = vx3d::opengl::create_shader(std::string(VX3D_ASSET_PATH) + "shaders/chunk_display.comp", GL_COMPUTE_SHADER);
    _compute_program = vx3d::opengl::create_program(_compute_shader);

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

void vx3d::ui::display::render(vx3d::world_loader &world_loader)
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

    glUseProgram(_compute_program);
    glBindTexture(GL_TEXTURE_2D, _display_texture);
    glClearTexImage(_display_texture, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindImageTexture(0, _display_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _temp);

    static auto current_zoom = float(1.0f);
    static auto current_translation = glm::vec2(0.0f, 0.0f);
    if (ImGui::IsWindowHovered())
    {
        auto &io = ImGui::GetIO();
        current_zoom += io.MouseWheel * -0.05;
//        current_zoom = glm::max(1.0f, current_zoom);

        if (ImGui::IsMouseDown(0))
        {
            auto delta =
              glm::vec2(io.MouseDelta.x, io.MouseDelta.y) * glm::vec2(-1, -1);
            delta.x /= 1920;
            delta.y /= 1040;
            delta *= 2;
            delta *= current_zoom;

            current_translation.x += delta.x;
            current_translation.y += delta.y;
        }
    }

    glUniform2i(glGetUniformLocation(_compute_program, "scene_size"), 1920, 1040);
    glUniform1f(glGetUniformLocation(_compute_program, "zoom"), current_zoom);
    glUniform2f(glGetUniformLocation(_compute_program, "translation"), current_translation.x, current_translation.y);


    glDispatchCompute(
      static_cast<int>(glm::ceil(1920 / 8)),
      static_cast<int>(glm::ceil(1040 / 8)),
      1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);

    ImGui::Image(reinterpret_cast<void *>(_display_texture), ImVec2(1920, 1040));

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(_window);
    glfwPollEvents();
}
