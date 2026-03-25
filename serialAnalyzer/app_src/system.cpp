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

#define SYSTEM_VIEW_ERR_OFFSET		(G_ERR_MAIN_WINDOW_INVALID_DATA)
#define SYSTEM_SYS_ERR_OFFSET		(G_ERR_SERIAL_CANT_CONNECT)

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

		if (tempCode == CTRL_INIT_ERR_INVALID_SPEC)
			this->g_errCode = G_ERR_INIT_INVALID_SPEC;
		else if (tempCode == CTRL_INIT_ERR_DX11_ON)
			this->g_errCode = G_ERR_INIT_DX11_ON;
		else

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
	while (this->ctrl.stillRunning() == true)
	{
		// 1. 데이터 업데이트
		this->model.update_data(this->ctrl.get_deltaTime());

		// 2. 화면 그리기 시작
		this->ctrl.BeginFrame();

		// 3. 레이아웃 렌더링
		this->view.layout(MainLayout);

		// 4. 화면 출력
		this->ctrl.EndFrame();
	}
}

SysErrCode analyzerSys::get_global_errCode(void)
{
	return this->g_errCode;
}