#include "Window.h"

#include "DirectX11Handler.h"

LRESULT CALLBACK Window::DefWEWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) return true;

    switch (uMsg) {
    case WM_SIZE: {
        Window::WndInfoPtr* wip = reinterpret_cast<Window::WndInfoPtr*> (GetWindowLongPtr(hWnd, GWLP_USERDATA));
        if (!wip) break;

        auto dx11h = reinterpret_cast<DirectX11Handler*>(wip->dx11h);
        int width  = LOWORD(lParam);
        int height = HIWORD(lParam);

        *wip->w = width;
        *wip->h = height;

        auto swapChain = dx11h->getSwapChain();
        DXGI_SWAP_CHAIN_DESC scd = {};
        swapChain->GetDesc(&scd);

        swapChain->ResizeBuffers(scd.BufferCount, width, height, scd.BufferDesc.Format, NULL);
        dx11h->createRenderTargetView();
        dx11h->createDepthStencil(DXGI_FORMAT_D24_UNORM_S8_UINT);

        dx11h->prepare();
        
        dx11h->setViewport(width, height);
        break;
    }
    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
}

Window::Window(const wchar_t* windowName,
               const int      x,
               const int      y,
               const int      width,
               const int      height) :
    msg({}), width(width), height(height)
{
    this->wip.w = &this->width;
    this->wip.h = &this->height;

    HINSTANCE hInstance = GetModuleHandle(NULL);

    WNDCLASSEXW wc   = {};
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.hInstance     = hInstance;
    wc.lpszClassName = L"WorldEngineWC";
    wc.lpfnWndProc   = DefWEWindowProc;
    RegisterClassEx(&wc);

    this->hWnd = CreateWindowEx(NULL,
                                L"WorldEngineWC", windowName,
                                WS_OVERLAPPEDWINDOW,
                                x, y, width, height,
                                nullptr, nullptr, hInstance, nullptr);

    ShowWindow(this->hWnd, 10);
    UpdateWindow(this->hWnd);
}
Window::Window(const wchar_t* className,
               const wchar_t* windowName,
               const int      x,
               const int      y,
               const int      width,
               const int      height) :
    msg({}), width(width), height(height)
{
    HINSTANCE hInstance = GetModuleHandle(NULL);

    WNDCLASSEXW wc   = {};
    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.hInstance     = hInstance;
    wc.lpszClassName = className;
    wc.lpfnWndProc   = DefWEWindowProc;
    RegisterClassEx(&wc);

    this->hWnd = CreateWindowEx(NULL,
                                className, windowName,
                                WS_OVERLAPPEDWINDOW,
                                x, y, width, height,
                                nullptr, nullptr, hInstance, nullptr);

    ShowWindow(this->hWnd, SW_SHOWNORMAL);
    UpdateWindow(this->hWnd);
}

void Window::setWIP(DirectX11Handler* dx11h) {
    this->wip.dx11h = dx11h;
    SetWindowLongPtrA(this->hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&this->wip));
}

BOOL Window::poolEvents() {
    BOOL returnValue;
    while (returnValue = PeekMessage(&this->msg, NULL, 0, 0, PM_REMOVE)) {
        if (this->msg.message == WM_QUIT) {
            this->squit = TRUE;
            return FALSE;
        }

        TranslateMessage(&this->msg);
        DispatchMessage(&this->msg);
    }
    return returnValue;
}