#include "GuiManager.h"

#include "Window.h"
#include "DirectX11Handler.h"

GuiManager::~GuiManager() {
    // Cleanup ImGui
    if (ImGui::GetCurrentContext()) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }
}

void GuiManager::build(Window* window, DirectX11Handler* handler) {
    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    if (ImGui::GetCurrentContext() == nullptr) {
        std::cout << "Failed to create ImGui context\n";
    }

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    if (ImGui_ImplWin32_Init(window->hWnd) == false) {
        std::cout << "Failed to initialize ImGuiImplWin32\n";
    }

    if (ImGui_ImplDX11_Init(handler->getDevice().Get(), handler->getDeviceContext().Get()) == false) {
        std::cout << "Failed to initialize ImGuiImplDX11\n";
        return;
    }

    // Optional: Load fonts
    ImGui::GetIO().Fonts->AddFontDefault();
}

void GuiManager::createNewFrame(std::function<void(void*)> function, void* param) {
    // Start the ImGui frame
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    function(param);

    ImGui::Render();
}

void GuiManager::render() {
    auto drawData = ImGui::GetDrawData();
    if (drawData) {
        ImGui_ImplDX11_RenderDrawData(drawData);
    }
}