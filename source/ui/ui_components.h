#pragma once

#include <string>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imfilebrowser.h>

namespace vx3d::ui {

    struct menu_tab_input
    {
        std::filesystem::path directory;
    };
    [[nodiscard]] inline menu_tab_input menu_tab_component(ImGui::FileBrowser &browser)
    {
        auto input = menu_tab_input();

        browser.Display();
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
                            if (ImGui::MenuItem(directory.path().stem().string().c_str()))
                                input.directory = directory;
                        }
                    } else
                        ImGui::MenuItem("Couldn't find your minecraft installation");
                    // List the minecraft worlds here


                    ImGui::EndMenu();
                }

                if (ImGui::MenuItem("Load Schematic"))
                    browser.Open();

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Export")) {

                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        return input;
    }

}