#pragma once

#include <Windows.h>
#include <vector>
#include <string>
#include "imgui.h"
#include "implot.h"

#define VIEW_COLOR_BUFF_SIZE		(4UL)
#define VIEW_WINDOW_NAME_SIZE		(32UL)

enum VIEW_errCode
{
	VIEW_RUN_ERR_NONE,
	VIEW_RUN_ERR_MAIN_WINDOW_INVALID_DATA,
	VIEW_RUN_ERR_MAIN_WINDOW_NO_CALLBACK,
	VIEW_RUN_ERR_CHILD_WINDOW_NO_CALLBACK,
	VIEW_RUN_ERR_CHILD_WINDOW_INVALID_DATA
};

class analyzerView
{
public:
	// 반드시 호출되어야 하는 메서드
	analyzerView();
	~analyzerView();
	void set_mainDash_window(ImVec2 mainWindowPos, ImVec2 mainWindowSize, const char* mainWindow_name, ImGuiWindowFlags mainWindowOpt);
	void layout(void (*CBFunc)(void));

	// 사용자가 시스템을 구성하기 위한 API
	void childWindow(void (*CBFunc)(void), const char* childID, ImVec2 windowSize, ImGuiChildFlags windowFlg);
	int get_childCnt(void);
	ImVec2 get_windowCursor_pos(void);
	ImVec2 get_mainWindow_Size(void);
	VIEW_errCode get_errCode(void);
private:
	VIEW_errCode errCode;
	bool mainWindow_validData_flg;

	ImVec2 mainWindowPos; ImVec2 mainWindowSize;
	char mainWindowID[VIEW_WINDOW_NAME_SIZE];
	ImGuiWindowFlags mainWindowOpt;
	ImVec2 windowCursor_pos; // 다음 텍스트나 위젯이 그려질 시작 좌표

	std::vector<const char*> childWindowID;
	int childCnt;
};