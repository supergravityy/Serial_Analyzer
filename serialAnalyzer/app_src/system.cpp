#pragma once

#include <vector>
#include <string>
#include <math.h>
#include "config.h"
#include "imgui.h"
#include "implot.h"
#include "ctrl.h"
#include "model.h"
#include "view.h"
#include "system.h"

#define SYSTEM_VIEW_ERR_OFFSET		(G_ERR_INIT_HANDSHAKE_WIN_DX)
#define SYSTEM_SYS_ERR_OFFSET		(G_ERR_CHILD_WINDOW_INVALID_DATA)
#define SYSTEM_MDOEL_ERR_OFFSET		(G_ERR_SERIAL_RUN_READ_FAIL)

analyzerSys appSystem;

analyzerSys::analyzerSys()
	: ctrl(CS_CLASSDC, L"Serial Analyzer", { 100, 100, 736, 519 })
{
	this->g_errCode = G_ERR_NONE;
}

analyzerSys::~analyzerSys()
{
	this->ctrl.end(); // 자원 해제
}

bool analyzerSys::init(void)
{
	CTRL_errcode tempCode;

	// 1. 백엔드 및 DX11 초기화
	if (this->ctrl.begin(nullptr) == false)
	{
		tempCode = ctrl.get_errCode();

		this->g_errCode = (SysErrCode)tempCode;

		return false;
	}

	// 2. 배경색 설정
	this->ctrl.set_WindowClr(MAIN_WND_BACK_CLR_RGB);

	// 3. 메인 대쉬보드 창 설정
	ImGuiWindowFlags dashFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	this->view.set_mainDash_window(ImVec2(0, 0), ImVec2(720, 480), "MainDash", dashFlags);

	return true;
}

void analyzerSys::update_errcode(void)
{
	VIEW_errCode tempCode;
	SERIAL_ErrCode tempCode2;
	MODEL_errCode tempCode3;

	tempCode = this->view.get_errCode();
	tempCode2 = this->serial.get_errCode();
	tempCode3 = this->model.get_errCode();
	if (tempCode != VIEW_RUN_ERR_NONE)
		this->g_errCode = (SysErrCode)(tempCode + SYSTEM_VIEW_ERR_OFFSET);
	else if (tempCode2 != SERIAL_ERR_NONE)
		this->g_errCode = (SysErrCode)(tempCode2 + SYSTEM_SYS_ERR_OFFSET);
	else if (tempCode3 != MODL_ERR_NONE)
		this->g_errCode = (SysErrCode)(tempCode3 + SYSTEM_MDOEL_ERR_OFFSET);
	else;
}

void analyzerSys::run(void)
{
	std::string rxData;
	while (this->ctrl.stillRunning() == true)
	{
		float dt = this->ctrl.get_deltaTime();
		this->model.add_elapsedTime(dt); // 시간 누적

		// 1. 데이터 업데이트
#if(ANL_RUN_MODE == ANL_DEBUG_MODE)
		rxData = this->model.update_fakeData(this->ctrl.get_deltaTime());
		this->model.parse_teleplot_data(rxData);
		this->model.add_log("RX", rxData.c_str());
		this->model.add_log_with_time(this->model.get_elapsedTime(), rxData);
#else
		rxData = this->serial.readPending();
		if (!rxData.empty()) 
		{
			// 1. 로그창 출력 (선택 사항)
			this->model.add_log("RX", rxData.c_str());

			// 2. 수신 로그 기록
			this->model.add_log_with_time(this->model.get_elapsedTime(), rxData);

			// 3. 모델 내부에 파싱 지시 -> 자동으로 맵에 쌓임
			this->model.parse_teleplot_data(rxData);
		}
#endif
		// 2. 화면 그리기 시작
		this->ctrl.BeginFrame();

		// 3. 레이아웃 렌더링
		this->view.layout(MainLayout);

		// 4. 에러코드 업데이트
		this->update_errcode();

		// 5. 화면 출력
		this->ctrl.EndFrame();
	}
}

SysErrCode analyzerSys::get_global_errCode(void)
{
	return this->g_errCode;
}
