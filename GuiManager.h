#pragma once
#include "framework.h"

#include "Window.h"
#include "DirectX11Handler.h"


class GuiManager {
public:
    GuiManager() = default;

    ~GuiManager();

    void build(Window* window, DirectX11Handler* handler);

    void createNewFrame(std::function<void(void*)> function, void* param);
    void render();
};