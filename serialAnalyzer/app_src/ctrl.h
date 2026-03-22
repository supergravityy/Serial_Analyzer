#pragma once

#include <Windows.h>
#include <vector>
#include <string>
#include <d3d11.h>
#include "imgui.h"

#define CTRL_WINDOW_NAME_SIZE			(32UL)
#define CTRL_WINDOW_CLR_BUFF_SIZE		(4UL)

// 시스템 제어 및 로직
// 생성자: 사용자 정의 변수 입력
// 시작 : 
// 소멸자: 모든 그래픽 자원 해제 및 윈도우 파괴.

enum errcode
{
	CTRL_INIT_ERR_NONE,
	CTRL_INIT_ERR_INVALID_SPEC,
	CTRL_INIT_ERR_DX11_ON,
	CTRL_INIT_ERR_HANDSHAKE_WIN_DX,
	CTRL_RUN_ERR_RENDER_FAIL
};

typedef struct
{
	int xPos;
	int yPos;
	int width;
	int height;
}windowSize_Data;

class analyzerCtrl
{
public:
	analyzerCtrl(unsigned int classStyle, const wchar_t* windowName, const windowSize_Data windowSpec);
	bool begin(LRESULT(*CBproc)(HWND, UINT, WPARAM, LPARAM ));
	void end(void);
	~analyzerCtrl() {};

	void BeginFrame(void);
	void EndFrame(void);
	bool stillRunning(void); // 내부에서 PeekMessage 처리 및 종료 확인
	float get_deltaTime(void);
	void set_WindowClr(float r, float g, float b, float alpha);

protected:
	bool CreateDeviceD3D(HWND hWnd);
	void CreateRenderTarget(void);
	void CleanupDeviceD3D(void);
	void CleanupRenderTarget(void);

private: // 핵심변수 WNDCLASSEXW wc, HWND hwnd
	errcode errCode;
	float windowClr[CTRL_WINDOW_CLR_BUFF_SIZE];

	// WNDCLASSEXW 생성용
	LRESULT (*CBproc)(HWND, UINT, WPARAM, LPARAM);
	unsigned int classStyle;
	const wchar_t* classId = L"SerialAnalyzer";
	
	// WindowHandler 생성용
	int xPos, yPos;
	int width, height;
	wchar_t windowName[CTRL_WINDOW_NAME_SIZE];

	ID3D11Device* pd3dDevice = NULL;                       // 텍스처나 버퍼 같은 그래픽 자원을 '생성'할 때 쓰임
	ID3D11DeviceContext* pd3dDeviceContext = NULL;         // Dx11에게 화면 그리기/지우기 명령을 내릴때 쓰임
	IDXGISwapChain* pSwapChain = NULL;                     // 뒤에서 그린 화면(예비화면)을 교체해서 보여주는 화면전환기
	ID3D11RenderTargetView* mainRenderTargetView = NULL;   // 실제 화면이 그려지는 객체

	WNDCLASSEXW WindowFeatures; // 윈도우 os 제출용
	HWND WindowHandler; // 윈도우 os 수취용
};

static LRESULT defaultCBproc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);