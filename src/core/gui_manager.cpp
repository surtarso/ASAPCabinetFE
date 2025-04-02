#include "core/gui_manager.h"
#include "core/iwindow_manager.h"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "utils/logging.h"

GuiManager::GuiManager(IWindowManager* windowManager) 
    : windowManager_(windowManager), configWindow_(nullptr), configRenderer_(nullptr), context_(nullptr) {}

GuiManager::GuiManager(SDL_Window* window, SDL_Renderer* renderer) 
    : windowManager_(nullptr), configWindow_(window), configRenderer_(renderer), context_(nullptr) {}

GuiManager::~GuiManager() {
    if (context_) {
        ImGui_ImplSDLRenderer2_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext(context_);
        LOG_DEBUG("GuiManager destroyed");
    }
}

void GuiManager::initialize() {
    IMGUI_CHECKVERSION();
    context_ = ImGui::CreateContext();
    ImGui::StyleColorsDark();
    if (windowManager_) {
        ImGui_ImplSDL2_InitForSDLRenderer(windowManager_->getPrimaryWindow(), windowManager_->getPrimaryRenderer());
        ImGui_ImplSDLRenderer2_Init(windowManager_->getPrimaryRenderer());
    } else {
        ImGui_ImplSDL2_InitForSDLRenderer(configWindow_, configRenderer_);
        ImGui_ImplSDLRenderer2_Init(configRenderer_);
    }
    LOG_DEBUG("GuiManager initialized");
}

void GuiManager::newFrame() {
    ImGui_ImplSDLRenderer2_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void GuiManager::render(SDL_Renderer* renderer) {
    ImGui::Render();
    if (ImGui::GetDrawData()) {
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
    }
}

void GuiManager::processEvent(const SDL_Event& event) {
    ImGui_ImplSDL2_ProcessEvent(&event);
}