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

void ImGuiManager::initialize() {
    IMGUI_CHECKVERSION();
    context_ = ImGui::CreateContext();
    ImGui::StyleColorsDark();

    // Disable ini file loading and saving
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr; // Prevent loading/saving imgui.ini

    // Apply DPI scaling to ImGui
    const Settings& settings = configService_->getSettings();
    if (settings.enableDpiScaling) {
        //LOG_DEBUG("ImGuiManager: Applying DPI scale: " << settings.dpiScale);
        io.FontGlobalScale = settings.dpiScale;
        // Scale all ImGui style sizes
        ImGui::GetStyle().ScaleAllSizes(settings.dpiScale);
    }

    if (windowManager_) {
        ImGui_ImplSDL2_InitForSDLRenderer(windowManager_->getPlayfieldWindow(), windowManager_->getPlayfieldRenderer());
        ImGui_ImplSDLRenderer2_Init(windowManager_->getPlayfieldRenderer());
    } else {
        ImGui_ImplSDL2_InitForSDLRenderer(configWindow_, configRenderer_);
        ImGui_ImplSDLRenderer2_Init(configRenderer_);
    }
    LOG_INFO("ImGui Initialized.");
}

void ImGuiManager::newFrame() {
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void ImGuiManager::render(SDL_Renderer* renderer) {
    ImGui::Render();
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
