#include "editor/editor.h"
#include "editor/editor_ui.h"
#include "log/logger.h"
#include "core/ui/imgui_manager.h"
#include "tables/table_loader.h"
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_sdlrenderer2.h>
#include <filesystem>
#include <iostream>

Editor::Editor(const std::string& configPath)
    : configPath_(configPath) {
    initializeSDL();
    initializeImGui();

    // Load ASAPCabinetFE config through shared interface
    auto keybindProvider = DependencyFactory::createKeybindProvider();
    config_ = DependencyFactory::createConfigService(configPath_, keybindProvider.get());
    keybindProvider_ = std::move(keybindProvider);
    tableLoader_ = std::make_unique<TableLoader>();

    editorUI_ = std::make_unique<EditorUI>(config_.get(), tableLoader_.get());
    LOG_INFO("Editor initialized using shared configuration");
}

Editor::~Editor() {
    cleanup();
}

void Editor::initializeSDL() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        LOG_ERROR("SDL_Init Error: " + std::string(SDL_GetError()));
        throw std::runtime_error("Editor SDL initialization failed");
    }

    window_ = SDL_CreateWindow("ASAPCabinetFE Editor",
                               SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED,
                               1280, 720,
                               SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window_) throw std::runtime_error("Failed to create SDL window");

    renderer_ = SDL_CreateRenderer(window_, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_) throw std::runtime_error("Failed to create SDL renderer");

    LOG_INFO("SDL initialized for Editor");
}

void Editor::initializeImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr; // Prevent loading/saving imgui.ini
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplSDL2_InitForSDLRenderer(window_, renderer_);
    ImGui_ImplSDLRenderer2_Init(renderer_);
    LOG_DEBUG("ImGui initialized for Editor");
}

void Editor::mainLoop() {
    while (!editorUI_->shouldExit()) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                return;
        }

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        editorUI_->draw();

        ImGui::Render();
        SDL_SetRenderDrawColor(renderer_, 30, 30, 30, 255);
        SDL_RenderClear(renderer_);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer_);
        SDL_RenderPresent(renderer_);
    }
}

void Editor::cleanup() {
    if (loadingThread_.joinable())
        loadingThread_.join();

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_) SDL_DestroyWindow(window_);
    SDL_Quit();

    LOG_INFO("Editor cleaned up");
}

void Editor::run() {
    mainLoop();
}
