#pragma comment(linker, "/entry:mainCRTStartup")
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "implot.h"
#include <d3d11.h>
#include <vector>
#include <string>
#include <math.h>

// [1. 데이터 및 실시간 버퍼 설정]
struct ScrollingBuffer {
    int MaxSize;
    int Offset;
    std::vector<ImVec2> Data;
    ScrollingBuffer(int max_size = 2000) {
        MaxSize = max_size;
        Offset = 0;
        Data.reserve(MaxSize);
    }
    void AddPoint(float x, float y) {
        if (Data.size() < MaxSize)
            Data.push_back(ImVec2(x, y));
        else {
            Data[Offset] = ImVec2(x, y);
            Offset = (Offset + 1) % MaxSize;
        }
    }
};

static ScrollingBuffer sdata;
static std::vector<std::string> logs;
static char tx_buffer[256] = "";
static int x_axis_range = 5;
static float t = 0;

// DirectX 관련 설정 (이전과 동일)
static ID3D11Device* g_pd3dDevice = NULL;
static ID3D11DeviceContext* g_pd3dDeviceContext = NULL;
static IDXGISwapChain* g_pSwapChain = NULL;
static ID3D11RenderTargetView* g_mainRenderTargetView = NULL;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// [Main 함수 시작]
int main(int, char**)
{
    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"SerialAnalyzer", NULL };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Serial Analyzer", WS_OVERLAPPEDWINDOW, 100, 100, 736, 519, NULL, NULL, wc.hInstance, NULL);

    if (!CreateDeviceD3D(hwnd)) { CleanupDeviceD3D(); return 1; }
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    bool done = false;
    while (!done)
    {
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg); ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        // [2. 가짜 데이터 생성: 1초에 60번 실행됨]
        t += ImGui::GetIO().DeltaTime;
        float fake_val = sinf(t) * 10.0f + 50.0f;
        sdata.AddPoint(t, fake_val);

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // [3. 레이아웃 시작]
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(720, 480));
        ImGui::Begin("MainDash", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        // 왼쪽: U1(로그) + U2(입력)
        ImGui::BeginChild("Left", ImVec2(ImGui::GetContentRegionAvail().x * 0.4f, 0), true);
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "U1: Rx Log");
        ImGui::BeginChild("Log", ImVec2(0, -70), true);
        for (auto& l : logs) ImGui::TextUnformatted(l.c_str());
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
        ImGui::EndChild();
        if (ImGui::InputText("##Tx", tx_buffer, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
            logs.push_back("[TX]: " + std::string(tx_buffer));
            tx_buffer[0] = '\0';
            ImGui::SetKeyboardFocusHere(-1);
        }
        ImGui::EndChild();

        ImGui::SameLine();

        // 오른쪽: U3(그래프) + U4/U5
        ImGui::BeginGroup();

        ImGui::BeginChild("U3", ImVec2(0, 280), true);
        if (ImPlot::BeginPlot("##Plot", ImVec2(-1, -1))) {
            ImPlot::SetupAxes("Time", "Value", 0, 0);
            ImPlot::SetupAxisLimits(ImAxis_X1, t - (float)x_axis_range, t, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 100);

            if (sdata.Data.size() > 0) {
                // 1. x값과 y값을 따로 담을 바구니 두 개를 만듭니다.
                std::vector<float> x_data, y_data;
                x_data.reserve(sdata.Data.size());
                y_data.reserve(sdata.Data.size());

                // 2. 시간 순서대로(Offset부터 끝까지, 그다음 처음부터 Offset까지) 분리해서 담습니다.
                for (int i = sdata.Offset; i < (int)sdata.Data.size(); ++i) {
                    x_data.push_back(sdata.Data[i].x);
                    y_data.push_back(sdata.Data[i].y);
                }
                for (int i = 0; i < sdata.Offset; ++i) {
                    x_data.push_back(sdata.Data[i].x);
                    y_data.push_back(sdata.Data[i].y);
                }

                // 3. 이제 ImPlot이 원하는 대로 x배열, y배열을 각각 던져줍니다. (인자 4개!)
                ImPlot::PlotLine("Temperature", x_data.data(), y_data.data(), (int)x_data.size());
            }
            ImPlot::EndPlot();
        }
        ImGui::EndChild();
        //ImGui::EndChild();

        ImGui::BeginChild("U4U5", ImVec2(0, 0), true);
        ImGui::Text("U5: Graph Config");
        ImGui::SliderInt("X Range", &x_axis_range, 1, 20);
        ImGui::EndChild();
        ImGui::EndGroup();

        ImGui::End();

        // [4. 렌더링]
        ImGui::Render();
        const float clear_color[4] = { 0.1f, 0.1f, 0.12f, 1.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }

    ImPlot::DestroyContext();
    ImGui_ImplDX11_Shutdown(); ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext();
    CleanupDeviceD3D(); ::DestroyWindow(hwnd); ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}

// --- 보조 함수 (필수 복사) ---
bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd; ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2; sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1; sd.Windowed = TRUE; sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, NULL, &g_pd3dDeviceContext) != S_OK) return false;
    CreateRenderTarget(); return true;
}
void CreateRenderTarget() { ID3D11Texture2D* pBackBuffer; g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer)); g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView); pBackBuffer->Release(); }
void CleanupDeviceD3D() { CleanupRenderTarget(); if (g_pSwapChain) g_pSwapChain->Release(); if (g_pd3dDevice) g_pd3dDevice->Release(); }
void CleanupRenderTarget() { if (g_mainRenderTargetView) g_mainRenderTargetView->Release(); }
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    if (msg == WM_DESTROY) { ::PostQuitMessage(0); return 0; }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}