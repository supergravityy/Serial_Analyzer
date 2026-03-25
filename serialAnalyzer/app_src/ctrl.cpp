#include "ctrl.h"
#include <Windows.h>
#include <vector>
#include <string>
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "imgui.h"
#include "implot.h"

analyzerCtrl::analyzerCtrl(unsigned int classStyle, const wchar_t* windowName, const windowSize_Data windowSpec)
{
	this->classStyle = classStyle;
	wcscpy_s(this->windowName, CTRL_WINDOW_NAME_SIZE, windowName);
	this->xPos = windowSpec.xPos;
	this->yPos = windowSpec.yPos;
	this->width = windowSpec.width;
	this->height = windowSpec.height;

	this->set_WindowClr(ImVec4(0,0,0,0));
	this->errCode = CTRL_INIT_ERR_NONE;
	this->CBproc = nullptr;
	this->WindowFeatures = { 0 };
	this->WindowHandler = { 0 };
}

bool analyzerCtrl::begin(LRESULT(*CBproc)(HWND, UINT, WPARAM, LPARAM))
{
	bool result = false;
	DWORD fixedStyle = WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME;

	this->CBproc = (CBproc != nullptr) ? CBproc : defaultCBproc;

	this->WindowFeatures = { sizeof(WNDCLASSEXW), this->classStyle, this->CBproc, 0L, 0L,
		GetModuleHandle(NULL), NULL, NULL, NULL, NULL, this->classId, NULL };

	::RegisterClassExW(&this->WindowFeatures);

	if (this->width <= 0 || this->height <= 0)
	{
		this->errCode = CTRL_INIT_ERR_INVALID_SPEC;
		return result;
	}

	this->WindowHandler = ::CreateWindowW(this->WindowFeatures.lpszClassName, this->windowName,
		fixedStyle, this->xPos, this->yPos, this->width, this->height, NULL, NULL,
		this->WindowFeatures.hInstance, NULL);

	if (!this->CreateDeviceD3D(this->WindowHandler)) 
	{ 
		this->CleanupDeviceD3D(); 
		this->errCode = CTRL_INIT_ERR_DX11_ON;
	}
	else
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImPlot::CreateContext();

		ImGui_ImplWin32_Init(this->WindowHandler);
		ImGui_ImplDX11_Init(this->pd3dDevice, this->pd3dDeviceContext);

		::ShowWindow(this->WindowHandler, SW_SHOWDEFAULT);
		::UpdateWindow(this->WindowHandler);

		result = true;
	}
	return result;
}

void analyzerCtrl::end(void)
{
	ImPlot::DestroyContext();
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	CleanupDeviceD3D();
	::DestroyWindow(this->WindowHandler);
	::UnregisterClassW(this->WindowFeatures.lpszClassName, this->WindowFeatures.hInstance);
}

void analyzerCtrl::BeginFrame(void)
{
	// 백엔드(Win32, DX11)에게 새 프레임임을 알림
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void analyzerCtrl::EndFrame(void)
{
	ImGui::Render(); // 지금까지 작성된 도면을 최종 확정

	this->pd3dDeviceContext->OMSetRenderTargets(1, &this->mainRenderTargetView, NULL);
	this->pd3dDeviceContext->ClearRenderTargetView(this->mainRenderTargetView, windowClr);

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	this->pSwapChain->Present(1, 0);
}

bool analyzerCtrl::stillRunning(void)
{
	MSG msg;
	// PeekMessage: 메시지가 있으면 처리하고, 없으면 기다리지 않고 즉시 false 반환 (Non-blocking)
	while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
	{
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);

		if (msg.message == WM_QUIT)
			return false; // 프로그램 종료 신호 발생
	}
	return true; // 계속 실행 중
}

CTRL_errcode analyzerCtrl::get_errCode(void)
{
	return this->errCode;
}

float analyzerCtrl::get_deltaTime(void)
{
	return ImGui::GetIO().DeltaTime;
}

void analyzerCtrl::set_WindowClr(ImVec4 color)
{
	this->windowClr[0] = color.x;
	this->windowClr[1] = color.y;
	this->windowClr[2] = color.z;
	this->windowClr[3] = color.w;
}

bool analyzerCtrl::CreateDeviceD3D(HWND hWnd)
{
	DXGI_SWAP_CHAIN_DESC sd; ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2; sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1; sd.Windowed = TRUE; sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &sd,
		&this->pSwapChain, &this->pd3dDevice, NULL, &this->pd3dDeviceContext) != S_OK)
	{
		return false;
	}
	else
	{
		CreateRenderTarget(); 
		return true;
	}
}

void analyzerCtrl::CreateRenderTarget(void)
{
	ID3D11Texture2D* pBackBuffer = nullptr;

	if (SUCCEEDED(this->pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer))) && pBackBuffer != nullptr)
	{
		this->pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &this->mainRenderTargetView);
		pBackBuffer->Release();
	}
	else
	{
		this->errCode = CTRL_RUN_ERR_RENDER_FAIL;
	}
}

void analyzerCtrl::CleanupDeviceD3D(void)
{
	this->CleanupRenderTarget(); 
	if (this->pSwapChain) this->pSwapChain->Release();
	if (this->pd3dDevice) this->pd3dDevice->Release();
}

void analyzerCtrl::CleanupRenderTarget(void)
{
	if (this->mainRenderTargetView)
		this->mainRenderTargetView->Release();
}

// ** 멤버함수로 만들 수 없는 이유 **
// 실제 멤버 함수는 this 포인터를 무조건 인자로 갖는다 -> 윈도우는 this 포인터가 쓸모 없음 
// => 그래서 static을 붙여준다

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static LRESULT defaultCBproc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_CLOSE:
		::DestroyWindow(hWnd);
		return 0;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	default:
		return ::DefWindowProc(hWnd, msg, wParam, lParam);
	}
}