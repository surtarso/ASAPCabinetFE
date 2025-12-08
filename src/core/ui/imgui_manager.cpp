#include "imgui_manager.h"
#include "core/iwindow_manager.h"
#include "config/iconfig_service.h"
#include "imgui.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_sdlrenderer2.h"
#include "log/logging.h"

ImGuiManager::ImGuiManager(IWindowManager* windowManager, IConfigService* configService)
    : windowManager_(windowManager), configService_(configService), configWindow_(nullptr), configRenderer_(nullptr), context_(nullptr) {}

ImGuiManager::ImGuiManager(SDL_Window* window, SDL_Renderer* renderer, IConfigService* configService)
    : windowManager_(nullptr), configService_(configService), configWindow_(window), configRenderer_(renderer), context_(nullptr) {}

ImGuiManager::~ImGuiManager() {
    if (context_) {
        ImGui_ImplSDLRenderer2_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext(context_);
        LOG_DEBUG("ImGuiManager: ImGuiManager destroyed");
    }
}

void ImGuiManager::reinitialize() {
    if (!context_) {
        initialize();
        return;
    }

    // Shutdown existing backends (keep context)
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();

    // Re-init using the same logic as initialize, but without recreating context
    if (windowManager_) {
        SDL_Window* w = windowManager_->getPlayfieldWindow();
        SDL_Renderer* r = windowManager_->getPlayfieldRenderer();
        LOG_DEBUG(std::string("ImGuiManager: Reinitializing ImGui for playfield window=" ) + std::to_string(reinterpret_cast<uintptr_t>(w)) +
                  ", renderer=" + std::to_string(reinterpret_cast<uintptr_t>(r)));
        ImGui_ImplSDL2_InitForSDLRenderer(w, r);
        ImGui_ImplSDLRenderer2_Init(r);
    } else {
        LOG_DEBUG(std::string("ImGuiManager: Reinitializing ImGui for config window=" ) + std::to_string(reinterpret_cast<uintptr_t>(configWindow_)) +
                  ", renderer=" + std::to_string(reinterpret_cast<uintptr_t>(configRenderer_)));
        ImGui_ImplSDL2_InitForSDLRenderer(configWindow_, configRenderer_);
        ImGui_ImplSDLRenderer2_Init(configRenderer_);
    }
}

void ImGuiManager::initialize() {
    IMGUI_CHECKVERSION();
    context_ = ImGui::CreateContext();
    ImGui::StyleColorsDark();

    // Disable ini file loading and saving
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr; // Prevent loading/saving imgui.ini

    // Apply DPI scaling to ImGui
    const Settings& settings = configService_->getSettings();
#if defined(__APPLE__)
    // Fonts and style scaling is handled dynamically in newFrame() using DisplayFramebufferScale
#else
    if (settings.enableDpiScaling) {
        io.FontGlobalScale = settings.dpiScale;
        ImGui::GetStyle().ScaleAllSizes(settings.dpiScale);
    }
#endif

    if (windowManager_) {
        SDL_Window* w = windowManager_->getPlayfieldWindow();
        SDL_Renderer* r = windowManager_->getPlayfieldRenderer();
        LOG_DEBUG(std::string("ImGuiManager: Initializing ImGui for playfield window=" ) + std::to_string(reinterpret_cast<uintptr_t>(w)) +
                  ", renderer=" + std::to_string(reinterpret_cast<uintptr_t>(r)));
        ImGui_ImplSDL2_InitForSDLRenderer(w, r);
        ImGui_ImplSDLRenderer2_Init(r);
    } else {
        LOG_DEBUG(std::string("ImGuiManager: Initializing ImGui for config window=" ) + std::to_string(reinterpret_cast<uintptr_t>(configWindow_)) +
                  ", renderer=" + std::to_string(reinterpret_cast<uintptr_t>(configRenderer_)));
        ImGui_ImplSDL2_InitForSDLRenderer(configWindow_, configRenderer_);
        ImGui_ImplSDLRenderer2_Init(configRenderer_);
    }
    LOG_INFO("ImGui Initialized.");
}

void ImGuiManager::newFrame() {
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();

    ImGuiIO& io = ImGui::GetIO();

    SDL_Window* window = windowManager_ ? windowManager_->getPlayfieldWindow() : configWindow_;
    SDL_Renderer* renderer = windowManager_ ? windowManager_->getPlayfieldRenderer() : configRenderer_;

    if (!window || !renderer) return;

    int winW = 0, winH = 0;
    int fbW = 0, fbH = 0;
    SDL_GetWindowSize(window, &winW, &winH);                    // Logical size (points)
    SDL_GetRendererOutputSize(renderer, &fbW, &fbH);            // Actual framebuffer (pixels)

    io.DisplaySize = ImVec2(static_cast<float>(winW), static_cast<float>(winH));
    io.DisplayFramebufferScale = ImVec2(
        static_cast<float>(fbW) / static_cast<float>(winW),
        static_cast<float>(fbH) / static_cast<float>(winH)
    );

#if defined(__APPLE__)
    // Optional: you can scale fonts automatically on Retina
    io.FontGlobalScale = io.DisplayFramebufferScale.x;
    ImGui::GetStyle().ScaleAllSizes(io.DisplayFramebufferScale.x);
#endif

    // Skip frame if invalid
    if (winW <= 0 || winH <= 0 || fbW <= 0 || fbH <= 0)
        return;

    ImGui::NewFrame();
}

void ImGuiManager::render(SDL_Renderer* renderer) {
    ImGui::Render();

    // Ensure valid DisplaySize before drawing
    ImGuiIO& io = ImGui::GetIO();
    if (io.DisplaySize.x <= 0.0f || io.DisplaySize.y <= 0.0f)
        return;

    if (ImGui::GetDrawData()) {
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
    }
}

void ImGuiManager::processEvent(const SDL_Event& event) {
    if (event.type == SDL_TEXTINPUT) {
        LOG_DEBUG("ImGuiManager: SDL_TEXTINPUT event, text: " + std::string(event.text.text));
    } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
        LOG_DEBUG("ImGuiManager: SDL_KEYDOWN event, Escape key pressed");
    }
    ImGui_ImplSDL2_ProcessEvent(&event);
}
