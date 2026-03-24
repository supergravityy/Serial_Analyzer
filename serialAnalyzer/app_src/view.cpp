#include <vector>
#include <string>
#include <cstring>
#include "imgui.h"
#include "implot.h"
#include "ctrl.h"
#include "view.h"

analyzerView::analyzerView()
{
	// 1. 에러 및 플래그 초기화
	this->errCode = VIEW_RUN_ERR_NONE;
	this->mainWindow_validData_flg = false;

	// 2. 메인 윈도우 속성 초기화
	this->mainWindowPos = ImVec2(0.0f, 0.0f);
	this->mainWindowSize = ImVec2(0.0f, 0.0f);
	memset(this->mainWindowID, 0, VIEW_WINDOW_NAME_SIZE); // 문자열 버퍼를 0으로(NULL) 채움
	this->mainWindowOpt = 0; // ImGuiWindowFlags_None 과 동일

	// 3. 커서 및 차일드 스택 초기화
	this->windowCursor_pos = ImVec2(0.0f, 0.0f);
	this->childCnt = 0;
	this->childWindowID.clear();
}

analyzerView::~analyzerView()
{
	this->childWindowID.clear();
}

void analyzerView::set_mainDash_window(ImVec2 mainWindowPos, ImVec2 mainWindowSize, const char* mainWindow_name, ImGuiWindowFlags mainWindowOpt)
{
	if (mainWindow_name == nullptr || mainWindowSize.x <= 0 || mainWindowSize.y <= 0)
	{
		this->errCode = VIEW_RUN_ERR_MAIN_WINDOW_INVALID_DATA;
		return;
	}

	this->mainWindowOpt = mainWindowOpt;
	this->mainWindowPos = mainWindowPos;
	this->mainWindowSize = mainWindowSize;
	strcpy_s(this->mainWindowID, (size_t)VIEW_WINDOW_NAME_SIZE, mainWindow_name);
	this->mainWindow_validData_flg = true;
}

void analyzerView::layout(void (*CBFunc)(void)) // ctrl의 배경색 설정하기
{
	if (CBFunc == nullptr)
	{
		this->errCode = VIEW_RUN_ERR_MAIN_WINDOW_NO_CALLBACK;
		return;
	}
	else if (this->mainWindow_validData_flg == false)
	{
		this->errCode = VIEW_RUN_ERR_MAIN_WINDOW_NO_DATA;
		return;
	}

	ImGui::SetNextWindowPos(this->mainWindowPos);
	ImGui::SetNextWindowSize(this->mainWindowSize);
	ImGui::Begin((const char*)this->mainWindowID, nullptr, this->mainWindowOpt);
	this->windowCursor_pos = ImGui::GetCursorPos();

	CBFunc();

	ImGui::End();
}

void analyzerView::childWindow(void (*CBFunc)(void), const char* childID, ImVec2 windowSize, ImGuiChildFlags windowFlg)
{
	if (CBFunc == nullptr)
	{
		this->errCode = VIEW_RUN_ERR_CHILD_WINDOW_NO_CALLBACK;
		return;
	}
	else if (childID == nullptr)
	{
		this->errCode = VIEW_RUN_ERR_CHILD_WINDOW_INVALID_DATA;
		return;
	}
	else if (this->errCode != VIEW_RUN_ERR_NONE)
	{
		return;
	}
	else
	{
		ImGui::BeginChild(childID, windowSize, windowFlg);
		this->windowCursor_pos = ImGui::GetCursorPos();
		this->childCnt++;
		this->childWindowID.push_back(childID);
	}

	CBFunc();

	ImGui::EndChild();
	this->childCnt--;
	this->childWindowID.pop_back();
}

int analyzerView::get_childCnt(void)
{
	return this->childCnt;
}

VIEW_errCode analyzerView::get_errCode(void)
{
	return this->errCode;
}

ImVec2 analyzerView::get_mainWindow_Size(void)
{
	return this->mainWindowSize;
}

ImVec2 analyzerView::get_windowCursor_pos(void)
{
	return this->windowCursor_pos;
}