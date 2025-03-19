#pragma once
#include "framework.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

class DirectX11Handler;
class GuiManager;
class InputManager;

class Window {
private:
    static LRESULT CALLBACK DefWEWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HWND hWnd;

    BOOL squit;

public:
    friend class DirectX11Handler;
    friend class GuiManager;
	friend class InputManager;

    MSG msg;

    int width;
    int height;

    struct WndInfoPtr {
        int* w;
        int* h;

        void* dx11h;
    } wip;
    
    Window(const wchar_t* windowName,
           const int      x,
           const int      y,
           const int      width,
           const int      height);
    Window(const wchar_t* className,
           const wchar_t* windowName,
           const int      x,
           const int      y,
           const int      width,
           const int      height);

    BOOL poolEvents();

    void setWIP(DirectX11Handler* dx11h);

    bool shouldQuit() noexcept { return squit; }
};
