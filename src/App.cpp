#include "../include/MultiLauncher/App.hpp"
#include "../include/MultiLauncher/GameManager.hpp"
#include "../include/MultiLauncher/EpicScanner.hpp"
#include "../include/MultiLauncher/SteamScanner.hpp"
#include "../include/MultiLauncher/PlaytimeManager.hpp"
#ifdef _WIN32
#include <windows.h>
#include <d3d11.h>
#include "../include/external/imgui/imgui_impl_win32.h"
#include "../include/external/imgui/imgui_impl_dx11.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#else
#include "../include/external/imgui/imgui.h"
#include <GLFW/glfw3.h>
#include "../include/external/imgui/imgui_impl_glfw.h"
#include "../include/external/imgui/imgui_impl_opengl3.h"
#include <stdio.h>
#endif

namespace MultiLauncher{

#ifdef _WIN32
    void CreateRenderTarget() {
        ID3D11Texture2D* pBackBuffer;
        g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
        pBackBuffer->Release();
    }

    void CleanupRenderTarget() {
        if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
    }

    void CleanupDeviceD3D() {
        CleanupRenderTarget();
        if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
        if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
        if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
    }

    bool CreateDeviceD3D(HWND hWnd) {
        DXGI_SWAP_CHAIN_DESC sd{};
        sd.BufferCount = 1;
        sd.BufferDesc.Width = 0;
        sd.BufferDesc.Height = 0;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hWnd;
        sd.SampleDesc.Count = 1;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
            0, nullptr, 0, D3D11_SDK_VERSION, &sd, &g_pSwapChain,
            &g_pd3dDevice, nullptr, &g_pd3dDeviceContext) != S_OK)
            return false;

        CreateRenderTarget();
        return true;
    }

    LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
            return true;

        switch (msg) {
        case WM_SIZE:
            if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
                CleanupRenderTarget();
                g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
                CreateRenderTarget();
            }
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    void App::run() {
        WNDCLASSEXA wc{ sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L,0L,
                        (HINSTANCE)GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr,
                        "MultiLauncher", nullptr };
        if (!RegisterClassExA(&wc)) {
            MessageBoxA(NULL, "Failed to register window class (RegisterClassExA returned 0).", "MultiLauncher Error", MB_ICONERROR | MB_OK);
            return;
        }

        HWND hwnd = CreateWindowA(wc.lpszClassName, "MultiLauncher", WS_OVERLAPPEDWINDOW, 100,100,800,600,
                                nullptr,nullptr,wc.hInstance,nullptr);

        if (!hwnd) {
            MessageBoxA(NULL, "Failed to create main window (CreateWindowA returned NULL).", "MultiLauncher Error", MB_ICONERROR | MB_OK);
            return;
        }

        if (!CreateDeviceD3D(hwnd)) {
            MessageBoxA(NULL, "Failed to create D3D11 device and swap chain.", "MultiLauncher Error", MB_ICONERROR | MB_OK);
            return;
        }

        ShowWindow(hwnd, SW_SHOWDEFAULT);
        UpdateWindow(hwnd);

        // Init GUI
        gui.init(hwnd, g_pd3dDevice, g_pd3dDeviceContext, g_mainRenderTargetView);

        // Init playtime tracking
        PlaytimeManager::instance().init();

        MultiLauncher::GameManager manager;
        manager.addScanner(std::make_unique<SteamScanner>());
        manager.addScanner(std::make_unique<MultiLauncher::EpicScanner>());
        
        // Scan in background to avoid UI lag
        std::thread([&manager](){
            manager.scanAll();
        }).detach();

        // Main loop
        MSG msg;
        ZeroMemory(&msg, sizeof(msg));
        auto lastUpdate = std::chrono::steady_clock::now();

        while (msg.message != WM_QUIT) {
            if (PeekMessage(&msg, nullptr, 0,0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                continue;
            }

            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdate).count() >= 2) {
                manager.update();
                lastUpdate = now;
            }

            g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            gui.render(manager);

            ImGui::Render();
            float clear_color[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
            g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
            g_pSwapChain->Present(1,0);
        }

        gui.shutdown();
        CleanupDeviceD3D();
        UnregisterClassA(wc.lpszClassName, wc.hInstance);
    }
#else
    static void glfw_error_callback(int error, const char* description)
    {
        fprintf(stderr, "Glfw Error %d: %s\n", error, description);
    }

    void App::run() {
        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit())
            return;

        // GL 3.0 + GLSL 130
        const char* glsl_version = "#version 130";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

        GLFWwindow* window = glfwCreateWindow(1280, 720, "MultiLauncher", NULL, NULL);
        if (window == NULL)
            return;
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // Enable vsync

        // Init GUI
        gui.init(window); 

        // Init playtime tracking
        PlaytimeManager::instance().init();

        MultiLauncher::GameManager manager;
        manager.addScanner(std::make_unique<SteamScanner>());
        manager.addScanner(std::make_unique<MultiLauncher::EpicScanner>());
        
        // Scan in background
        std::thread([&manager](){
            manager.scanAll();
        }).detach();

        // Main loop
        auto lastUpdate = std::chrono::steady_clock::now();
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();

             auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdate).count() >= 2) {
                manager.update();
                lastUpdate = now;
            }

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            gui.render(manager);

            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window);
        }

        gui.shutdown();
        glfwDestroyWindow(window);
        glfwTerminate();
    }
#endif
}