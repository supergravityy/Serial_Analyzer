// 원래 windows gui 프로그램은 wina main으로 시작해야 함 -> 표준 main으로 시작하게 해주는 설정
#pragma comment(linker, "/entry:mainCRTStartup") 
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "implot.h"
#include <d3d11.h>
#include <vector>
#include <string>
#include <math.h>

// 데이터 설정
struct ScrollingBuffer {
    int MaxSize;
    int Offset;
    std::vector<ImVec2> Data;

    // functions 001 : 
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

static ScrollingBuffer sdata;           // 실시간 데이터 저장용 링버퍼
static std::vector<std::string> logs;   // 시스템로그 저장소 (시리얼로 받은 문자열이나 시스템 상태 메시지를 화면에 리스트 형태로 보여주기 위해 필요)
static char tx_buffer[256] = "";
static int x_axis_range = 5;
static float elapsed_time = 0;

// DirectX 관련 설정 (이전과 동일)
static ID3D11Device* g_pd3dDevice = NULL;                       // 텍스처나 버퍼 같은 그래픽 자원을 '생성'할 때 쓰임
static ID3D11DeviceContext* g_pd3dDeviceContext = NULL;         // Dx11에게 화면 그리기/지우기 명령을 내릴때 쓰임
static IDXGISwapChain* g_pSwapChain = NULL;                     // 뒤에서 그린 화면(예비화면)을 교체해서 보여주는 화면전환기
static ID3D11RenderTargetView* g_mainRenderTargetView = NULL;   // 실제 화면이 그려지는 객체

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// [Main 함수 시작]
int main(int, char**)
{
    // functions 002 : 
    // 윈도우에서 프로그램을 띄우기 위해선 창의 '속성'을 정의해야 한다.
    WNDCLASSEXW wc = { sizeof(WNDCLASSEXW), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"SerialAnalyzer", NULL };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Serial Analyzer", WS_OVERLAPPEDWINDOW, 100, 100, 736, 519, NULL, NULL, wc.hInstance, NULL);

    // functions 003 : 
    // Dx11 그래픽 엔진 가동
    if (!CreateDeviceD3D(hwnd)) { CleanupDeviceD3D(); return 1; }
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // functions 004 : 
    // imgui가 윈도우 창 wc와 dx11을 위에서 동작하게 서로 연결해주는 과정
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();     // 그래픽을 위한 메모리공간 확보
    ImPlot::CreateContext();    // 그래프를 위한 메모리공간 확보

    // functions 005 : 
    // 백엔드와 핸드쉐이크 하는과정
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    bool done = false;
    while (!done)
    {
        MSG msg;
        // functions 006 : 
        // 윈도우는 사용자가 X를 누르거나 키보드를 치는 행동을 '메시지'형태로 전달함 -> 이거 처리 X시 응답없음이 뜨는 것
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg); ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        // functions 007 : 
        // 가짜 데이터 생성: imgui가 보장해주는 fps로 실행됨
        elapsed_time += ImGui::GetIO().DeltaTime; // 프레임 간격의 시간 계산
        float fake_val = sinf(elapsed_time) * 10.0f + 50.0f;
        sdata.AddPoint(elapsed_time, fake_val);

        // functions 008 : 
        // 프레임 시작
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        
        // functions 009 : 
        // GUI 레이아웃 시작 -> GUI 창 정보 설정 후 메인대쉬보드(창) 설정 확정
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(720, 480));
        ImGui::Begin("MainDash", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        // functions 010 : 
        // 왼쪽: U1(로그) + U2(입력)
        ImGui::BeginChild("Left", ImVec2(ImGui::GetContentRegionAvail().x * 0.4f, 0), true);
        // 차일드 : 크기 컨테이너 -> 스크롤이 독립적으로 주어짐
        // 파라미터 :  ID, ImVec2(끝위치), 테두리 출력 여부
        // GetContentRegionAvail -> 부모 창에서 쓸 수 있는 크기 정보, 0 -> 다쓰겠다
        // 크기인데 시작 이 없는 이유? -> ImGui는 "자동 배치(Auto-layout)" 시스템 
        // -> 시작점 : BeginChild를 호출하는 순간, 이전에 그려진 객체 바로 다음 위치(커서 위치)가 자동으로 시작점이 됨
        // , 끝점 : 우리가 준 **가로/세로 길이(Size)**만큼 시작점에서 더해진 지점이 자동으로 끝점
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "U1: Rx Log");

        // functions 011 : 
        ImGui::BeginChild("Log", ImVec2(0, -70), true);
        // Log는 Left의 차일드임, ImVec2(부모의 가로 다 쓰겠다, -70 바닥에서 70픽셀만큼은 비우고 나머지 다씀)
        for (auto& l : logs) ImGui::TextUnformatted(l.c_str());
        // 로그는 아래에서 위로 올라가는 형태
        // 사용자가 현재 마우스로 가장 최신 줄을 보고 있는 중인가?(가장 최신 -> 바닥보고 있음) 
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f); // SetScrollHereY -> 비율(100%)로 강제이동
        ImGui::EndChild();

        // functions 012 : 
        // ##Tx는 Left의 위젯(부품), ## 붙이는 건 id는 유지하되 글자는 출력이 안되게, 
        // ImGuiInputTextFlags_EnterReturnsTrue -> 엔터를 쳤을때만 참으로 만들게끔 해줌
        if (ImGui::InputText("##Tx", tx_buffer, 256, ImGuiInputTextFlags_EnterReturnsTrue)) {
            logs.push_back("[TX]: " + std::string(tx_buffer));
            tx_buffer[0] = '\0'; // memset같은 기능, 꼼수
            ImGui::SetKeyboardFocusHere(-1);  // 커서 깜빡
        }
        ImGui::EndChild(); 

        // functions 013 : 
        // 지금 Left를 만들어서 다음 줄은 480 이후(짤려서 안보임)이기에 Left와 똑같은 y좌표에 쓰겠다는 뜻
        ImGui::SameLine(); 
        // 오른쪽: U3(그래프) + U4/U5
        // 여러 개의 위젯을 하나의 커다란 부품(덩어리)처럼 묶어라
        ImGui::BeginGroup();

        // functions 014 : 
        ImGui::BeginChild("U3", ImVec2(0, 280), true);
        if (ImPlot::BeginPlot("##Plot", ImVec2(-1, -1))) // 그래프 위젯 생성 및 크기(-1,-1)-> 다쓰겠다
        {
            ImPlot::SetupAxes("Time", "Value", 0, 0); // x축 이름, y축 이름 -> 전부 기본설정 사용
            // X축이 보여줄 데이터의 범위를 강제로 지정
            // ImAxis_X1(기본 X축 제어), 재 시간에서 사용자가 설정한 범위(예: 5초)를 뺀 지점부터 보여줌, 끝점, 매프레임마다 강제적용 설정
            ImPlot::SetupAxisLimits(ImAxis_X1, elapsed_time - (float)x_axis_range, elapsed_time, ImGuiCond_Always);
            // 범위 고정
            ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 100);

            // functions 015 : 
            if (sdata.Data.size() > 0) {
                // 1. x값과 y값을 따로 담을 바구니 두 개를 만듭니다.
                std::vector<float> x_data, y_data;
                x_data.reserve(sdata.Data.size());
                y_data.reserve(sdata.Data.size());

                // 2. 시간 순서대로(Offset부터 끝까지, 그다음 처음부터 Offset까지) 분리해서 담습니다.
                // 링큐이기에. offset이 가장 최신데이터, 그 다음 인덱스가 가장 오래된 데이터
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

        // functions 016 : 
        ImGui::BeginChild("U4U5", ImVec2(0, 0), true);
        ImGui::Text("U5: Graph Config");
        ImGui::SliderInt("X Range", &x_axis_range, 1, 20);
        ImGui::EndChild();
        ImGui::EndGroup();

        ImGui::End();

        // functions 017 : 
        // 렌더링
        ImGui::Render();
        const float clear_color[4] = { 0.1f, 0.1f, 0.12f, 1.0f };

        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }

    // functions 018 : 
    ImPlot::DestroyContext();
    ImGui_ImplDX11_Shutdown(); ImGui_ImplWin32_Shutdown(); ImGui::DestroyContext();
    CleanupDeviceD3D(); ::DestroyWindow(hwnd); ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}

// functions 019 :
/*----------------------------Visualization start----------------------------------------*/
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
/*----------------------------Visualization end----------------------------------------*/