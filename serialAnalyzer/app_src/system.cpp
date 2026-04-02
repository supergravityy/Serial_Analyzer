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
#define SYSTEM_SERI_ERR_OFFSET		(G_ERR_CHILD_WINDOW_INVALID_DATA)
#define SYSTEM_MDOEL_ERR_OFFSET		(G_ERR_SERIAL_RUN_RX_TIMEOUT)

analyzerSys appSystem;

analyzerSys::analyzerSys()
	: ctrl(CS_CLASSDC, L"Serial Analyzer", { 100, 100, 736, 519 })
{
	this->g_errCode = G_ERR_NONE;
	this->err_startTime = 0.0f;
}

analyzerSys::~analyzerSys()
{
	this->ctrl.end(); // 자원 해제
}

bool analyzerSys::init(void)
{
	CTRL_errcode tempCode;
	ImGuiWindowFlags dashFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;

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
	this->view.set_mainDash_window(ImVec2(0, 0), ImVec2(720, 480), "MainDash", dashFlags);

	return true;
}

void analyzerSys::update_errcode(void)
{
	VIEW_errCode tempCode;
	SERIAL_ErrCode tempCode2;
	MODEL_errCode tempCode3;
	float dt;

	tempCode = this->view.get_errCode();
	tempCode2 = this->serial.get_errCode();
	tempCode3 = this->model.get_errCode();

	// 1. 에러코드 확인 (우선순위: View > Serial > Model)
	if (tempCode != VIEW_RUN_ERR_NONE) {
		this->g_errCode = (SysErrCode)(tempCode + SYSTEM_VIEW_ERR_OFFSET);
		this->err_startTime = (float)ImGui::GetTime();
	}
	else if (tempCode2 != SERIAL_ERR_NONE) {
		this->g_errCode = (SysErrCode)(tempCode2 + SYSTEM_SERI_ERR_OFFSET);
		this->err_startTime = (float)ImGui::GetTime();
	}
	else if (tempCode3 != MODL_ERR_NONE) {
		this->g_errCode = (SysErrCode)(tempCode3 + SYSTEM_MDOEL_ERR_OFFSET);
		this->err_startTime = (float)ImGui::GetTime();
	}
	else;

	// 2. 에러가 떠있는 상태이고, 발생한 지 5.0초가 지났다면 글로벌 에러 해제
	if (this->g_errCode != G_ERR_NONE)
	{
		dt = (float)ImGui::GetTime() - this->err_startTime;

		if (dt > 10.0f) 
			this->g_errCode = G_ERR_NONE;
	}
}

void analyzerSys::run(void)
{
	std::string rxData;

	// 1. 메인 루프 시작 (윈도우 메시지 처리( X표시 및 최소화) 및 화면 업데이트)
	while (this->ctrl.stillRunning() == true)
	{
		float dt = this->ctrl.get_deltaTime();
		this->model.add_elapsedTime(dt); 

		// 2. 데이터 업데이트
#if(ANL_RUN_MODE == ANL_DEBUG_MODE)
		rxData = this->model.update_fakeData(this->ctrl.get_deltaTime());
		this->model.parse_teleplot_data(rxData);
		this->model.add_log("RX", rxData.c_str());
		this->model.add_log_with_time(this->model.get_elapsedTime(), rxData);
#else
		rxData = this->serial.readPendings();

		// 3. 수신된 데이터가 있다면 기록으로 추가 -> U1 로깅, CSV 기록용 로깅(시간 데이터), 그래프용 파싱후 도메인/값 로깅
		if (rxData.empty() == false) 
		{
			this->model.add_log("RX", rxData.c_str());

			this->model.parse_teleplot_data(rxData);
		}
#endif
		// 4. 화면 그리기 시작
		this->ctrl.BeginFrame();

		// 5. 레이아웃 렌더링
		this->view.layout(MainLayout);

		// 6. 에러코드 업데이트
		this->update_errcode();

		// 7. 화면 출력
		this->ctrl.EndFrame();
	}
}

SysErrCode analyzerSys::get_global_errCode(void)
{
	return this->g_errCode;
}
