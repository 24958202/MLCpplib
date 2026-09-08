#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <SDL3/SDL.h>
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <SDL3/SDL_opengles2.h>
#else
#include <SDL3/SDL_opengl.h>
#endif

// Main code
int main(int, char**)
{
    // 1. Initialize SDL3
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return -1;
    }

    // Decide GL+GLSL versions
#if defined(__APPLE__)
    // GL 3.2 Core + GLSL 150
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    Uint32 window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN;
    
    SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL3+OpenGL3 Example", 1280, 720, window_flags);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }
    
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync
    SDL_ShowWindow(window);

    // 2. Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Load Arial font dynamically based on the OS
#if defined(_WIN32)
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", 16.0f);
#elif defined(__APPLE__)
    io.Fonts->AddFontFromFileTTF("/System/Library/Fonts/Supplemental/Arial.ttf", 16.0f);
#else
    io.Fonts->AddFontFromFileTTF("/usr/share/fonts/truetype/msttcorefonts/arial.ttf", 16.0f);
#endif

    // --------------------------------------------------------
    // APPLY WINDOWS STYLE UI
    // --------------------------------------------------------
    ImGui::StyleColorsLight(); // Base it on light mode for black text
    
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Windows 11 style rounded corners and distinct borders
    style.WindowRounding    = 8.0f;
    style.FrameRounding     = 4.0f;
    style.PopupRounding     = 8.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding      = 4.0f;
    style.TabRounding       = 4.0f;
    style.WindowBorderSize  = 1.0f;
    style.FrameBorderSize   = 1.0f;

    ImVec4* colors = style.Colors;
    
    // Clean whites and greys for backgrounds
    colors[ImGuiCol_WindowBg]           = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
    colors[ImGuiCol_PopupBg]            = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
    colors[ImGuiCol_Border]             = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    
    // Title bars blend seamlessly into the window like native apps
    colors[ImGuiCol_TitleBg]            = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
    colors[ImGuiCol_TitleBgActive]      = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]   = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
    
    // Interactive elements (Inputs, Dropdowns, etc.)
    colors[ImGuiCol_FrameBg]            = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]     = ImVec4(0.92f, 0.92f, 0.92f, 1.00f);
    colors[ImGuiCol_FrameBgActive]      = ImVec4(0.88f, 0.88f, 0.88f, 1.00f);
    
    // Buttons
    colors[ImGuiCol_Button]             = ImVec4(0.98f, 0.98f, 0.98f, 1.00f);
    colors[ImGuiCol_ButtonHovered]      = ImVec4(0.92f, 0.92f, 0.92f, 1.00f);
    colors[ImGuiCol_ButtonActive]       = ImVec4(0.88f, 0.88f, 0.88f, 1.00f);
    
    // Windows Accent Blue for active components (Sliders, Checkboxes, Tabs)
    colors[ImGuiCol_CheckMark]          = ImVec4(0.00f, 0.45f, 0.85f, 1.00f); 
    colors[ImGuiCol_SliderGrab]         = ImVec4(0.00f, 0.45f, 0.85f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]   = ImVec4(0.00f, 0.35f, 0.75f, 1.00f);
    colors[ImGuiCol_Header]             = ImVec4(0.92f, 0.92f, 0.92f, 1.00f);
    colors[ImGuiCol_HeaderHovered]      = ImVec4(0.88f, 0.88f, 0.88f, 1.00f);
    colors[ImGuiCol_HeaderActive]       = ImVec4(0.00f, 0.45f, 0.85f, 0.20f); 
    
    // Text
    colors[ImGuiCol_Text]               = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
    // --------------------------------------------------------

    // 3. Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // 4. Main loop
    bool done = false;
    ImVec4 clear_color = ImVec4(0.85f, 0.85f, 0.85f, 1.00f); // Lightened desktop background

    while (!done)
    {
        // Poll and handle events (inputs, window resize, etc.)
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // --- BUILD YOUR UI HERE ---
        
        ImGui::Begin("System Settings", NULL, ImGuiWindowFlags_AlwaysAutoResize);
        
        ImGui::Text("Windows Style UI Properties");
        ImGui::Separator();
        ImGui::Spacing();
        
        static bool is_enabled = true;
        ImGui::Checkbox("Enable background services", &is_enabled);
        
        static float volume = 50.0f;
        ImGui::SliderFloat("Master Volume", &volume, 0.0f, 100.0f, "%.0f%%");
        
        static char text_buf[64] = "/System/Library/CoreServices";
        ImGui::InputText("Target Path", text_buf, 64);
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        if (ImGui::Button("Apply & Close", ImVec2(120, 0))) {
            done = true;
        }
        
        ImGui::End();

        // --------------------------

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        SDL_GL_SwapWindow(window);
    }

    // 5. Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DestroyContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}