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

void analyzerSys::run(void)
{
	VIEW_errCode tempCode;
	while (this->ctrl.stillRunning() == true)
	{
		// 1. 데이터 업데이트
		this->model.update_data(this->ctrl.get_deltaTime());

		// 2. 화면 그리기 시작
		this->ctrl.BeginFrame();

		// 3. 레이아웃 렌더링
		this->view.layout(MainLayout);

		// 4. 에러코드 업데이트
		tempCode = this->view.get_errCode();
		this->g_errCode = (tempCode != VIEW_RUN_ERR_NONE)
			? (SysErrCode)(tempCode + SYSTEM_VIEW_ERR_OFFSET) : this->g_errCode;

		// 5. 화면 출력
		this->ctrl.EndFrame();
	}
}

SysErrCode analyzerSys::get_global_errCode(void)
{
	return this->g_errCode;
}