#include "app.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <fmt/format.h>

#include "mainframe.h"

#define WIDTH 800
#define HEIGHT 600

namespace {
    struct app_data
    {
        app_data()
        {

        }

        void set_size(int w, int h)
        {
            width = w;
            height = h;
        }

        int width{0};
        int height{0};
        GLFWwindow* window{nullptr};
    };

    //guarantees initialization
    app_data& instance() {
        static app_data data;
        return data;
    }

    const char* glsl_version = "#version 330";
}

void imc::gui::run_app()
{
    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "ImCommander", NULL, NULL);
    glfwMakeContextCurrent(window);
    instance().window = window;

    int version = gladLoadGL(glfwGetProcAddress);
    fmt::print(stderr, "GL {}.{}\n", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    glfwSetCharCallback(window, ImGui_ImplGlfw_CharCallback);
    bool show_demo_window = false;
    //bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

         // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        instance().set_size(display_w, display_h);

        bool should_close = draw_mainframe(display_w, display_h);

        // Rendering
        ImGui::Render();

        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);

        if (should_close)
            break;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    instance().window = nullptr;

    glfwTerminate();
}


