// Created by Modar Nasser on 12/03/2022.

#include "App.hpp"
#include "TextureManager.hpp"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include <LDtkLoader/World.hpp>

#include <filesystem>

constexpr float PANEL_WIDTH = 200.f;
constexpr float BAR_HEIGHT = 30.f;

App::App() : m_window(1280, 720, "LDtk World Viewer") {
    m_projects_data.insert({"", {}});
    m_shader.load(vert_shader, frag_shader);
    initImGui();
}

bool App::loadLDtkFile(const char* path) {
    m_projects.insert({path, {}});
    m_projects[path].load(path);

    m_projects_data.insert({path, {}});
    m_projects_data[path].camera.setSize(m_window.getSize());
    return true;
}

void App::run() {
    while (m_window.isOpen()) {
        while (auto event = m_window.nextEvent()) {
            processEvent(event.value());
        }

        if (m_selected_project.empty())
            m_window.clear({54.f/255.f, 60.f/255.f, 69.f/255.f});
        else
            m_window.clear(getActiveProject().bg_color);

        m_shader.bind();
        m_shader.setUniform("window_size", glm::vec2(m_window.getSize()));
        m_shader.setUniform("offset", glm::vec2(PANEL_WIDTH, BAR_HEIGHT));

        if (!m_selected_project.empty()) {
            const int active_depth = getActiveDepth();
            m_shader.setUniform("transform", getActiveCamera().getTransform());
            for (const auto& world : getActiveProject().worlds) {
                for (const auto& [depth, levels] : world.levels) {
                    if (depth > active_depth)
                        continue;
                    float opacity = 0.5f - std::abs(active_depth - depth)/8.f;
                    m_shader.setUniform("opacity", depth == active_depth ? 1.f : opacity);
                    for (const auto& level : levels) {
                        for (const auto& layer : level.layers)
                            layer.render(m_shader);
                    }
                }
            }
        }

        renderImGui();

        m_window.display();
    }
}

void App::processEvent(sogl::Event& event) {
    static bool camera_grabbed = false;
    static glm::vec<2, int> grab_pos;

    if (auto resize = event.as<sogl::Event::Resize>()) {
        for (auto& [_, data] : m_projects_data)
            data.camera.setSize({resize->width, resize->height});
    }
    else if (auto drop = event.as<sogl::Event::Drop>()) {
        for (auto& file : drop->files)
            loadLDtkFile(file.c_str());
    }
    else if (auto key = event.as<sogl::Event::Key>()) {
        if (!ImGui::GetIO().WantCaptureKeyboard) {
            if (key->action == GLFW_PRESS) {
                switch (key->key) {
                    case GLFW_KEY_ESCAPE:
                        m_window.close();
                        break;
                    default:
                        break;
                }
            }
        }
    }
    else if (auto btn = event.as<sogl::Event::MouseButton>()) {
        if (!ImGui::GetIO().WantCaptureMouse) {
            if (btn->button == GLFW_MOUSE_BUTTON_LEFT) {
                if (btn->action == GLFW_PRESS) {
                    camera_grabbed = true;
                    grab_pos = m_window.getMousePosition();
                } else if (btn->action == GLFW_RELEASE) {
                    camera_grabbed = false;
                }
            } else if (btn->button == GLFW_MOUSE_BUTTON_RIGHT) {
                if (btn-> action == GLFW_PRESS) {
                    auto& active_project_levels = getActiveProject().worlds[0].levels;
                    auto depth_offset = active_project_levels.begin()->first;
                    auto depth = getActiveDepth()+1 - depth_offset;
                    m_projects_data[m_selected_project].depth = (depth % active_project_levels.size()) + depth_offset;
                }
            }
        }
    }
    else if (auto move = event.as<sogl::Event::MouseMove>()) {
        if (camera_grabbed) {
            auto& camera = getActiveCamera();
            auto dx = (grab_pos.x - move->x) / camera.getZoom();
            auto dy = (grab_pos.y - move->y) / camera.getZoom();
            grab_pos = {move->x, move->y};
            camera.move(dx, dy);
        }
    }
    else if (auto scroll = event.as<sogl::Event::Scroll>()) {
        if (!ImGui::GetIO().WantCaptureMouse) {
            auto& camera = getActiveCamera();
            if (scroll->dy < 0) {
                camera.zoom(0.9f);
            } else if (scroll->dy > 0) {
                camera.zoom(1.1f);
            }
        }
    }
}

LDtkProject& App::getActiveProject() {
    if (m_selected_project.empty())
        return m_dummy_project;
    return m_projects.at(m_selected_project);
}

Camera2D& App::getActiveCamera() {
    return m_projects_data.at(m_selected_project).camera;
}

int App::getActiveDepth() {
    return m_projects_data.at(m_selected_project).depth;
}

void App::initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(&m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    ImGui::GetStyle().WindowBorderSize = 0.f;
    ImGui::GetStyle().SelectableTextAlign = {0.5f, 0.5f};
    // ImGui::GetStyle().ScaleAllSizes(1.f);
}

void App::renderImGui() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    static bool show_demo_window = false;
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);
    {
        static std::map<std::string, bool> worlds_tabs;
        ImGui::GetStyle().WindowPadding = {0.f, 10.f};
        ImGui::SetNextWindowSize({(float)m_window.getSize().x-PANEL_WIDTH, BAR_HEIGHT});
        ImGui::SetNextWindowPos({PANEL_WIDTH, 0});
        ImGui::Begin("Full", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDecoration
                                    | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar);
        ImGui::BeginTabBar("WorldsSelector");
        worlds_tabs.clear();
        for (auto& [name, _] : m_projects) {
            worlds_tabs[name] = true;
            auto filename = std::filesystem::path(name).filename().string();
            if (ImGui::BeginTabItem(filename.c_str(), &worlds_tabs[name])) {
                m_selected_project = name;
                ImGui::EndTabItem();
            }
        }
        for (auto& [name, open] : worlds_tabs) {
            if (!open) {
                m_projects.erase(name);
                m_projects_data.erase(name);
                m_selected_project.clear();
            }
        }
        ImGui::EndTabBar();
        ImGui::End();
    }
    {
        ImGui::GetStyle().WindowPadding = {0.f, 0.f};
        ImGui::SetNextWindowSize({PANEL_WIDTH, (float)m_window.getSize().y});
        ImGui::SetNextWindowPos({0, 0});
        ImGui::Begin("Main", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDecoration);

        // Software Title + version
        ImGui::Pad(0, 20);
        ImGui::TextCentered("LDtk Viewer v0.1");

        // Current world levels
        if (!m_selected_project.empty()) {
            ImGui::Pad(15, 30);
            ImGui::Text("Levels");
            ImGui::BeginListBox("Levels", {PANEL_WIDTH, 0});
            for (const auto& level : getActiveProject().worlds[0].levels.at(getActiveDepth())) {
                if (ImGui::Selectable(level.name.c_str(), false)) {
                    auto level_center = level.bounds.pos + level.bounds.size / 2.f;
                    getActiveCamera().centerOn(level_center.x, level_center.y);
                }
            }
            ImGui::EndListBox();
        }

        // demo window
        ImGui::Pad(15, 30);
        ImGui::Checkbox("Demo Window", &show_demo_window);
        ImGui::End();
    }
    ImGui::Render();

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}