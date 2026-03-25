#pragma once

#include <vector>
#include <string>
#include <math.h>
#include "config.h"
#include "ctrl.h"
#include "model.h"
#include "view.h"

enum SysErrCode {
	// ctrl 에러
	G_ERR_NONE,
	G_ERR_INIT_INVALID_SPEC,
	G_ERR_INIT_DX11_ON,
	G_ERR_INIT_HANDSHAKE_WIN_DX,
	G_ERR_RUN_RENDER_FAIL,
	// view 에러
	G_ERR_MAIN_WINDOW_INVALID_DATA,
	G_ERR_MAIN_WINDOW_NO_DATA,
	G_ERR_MAIN_WINDOW_NO_CALLBACK,
	G_ERR_CHILD_WINDOW_NO_CALLBACK,
	G_ERR_CHILD_WINDOW_INVALID_DATA,
	// system 에러
	G_ERR_SERIAL_CANT_CONNECT
};

class analyzerSys
{
public:
	analyzerSys();
	~analyzerSys();

	bool init(void); // 시스템 초기화 (DX11, ImGui 세팅)
	void run(void);  // 무한 루프 실행 (메인 스케줄러)
	SysErrCode get_global_errCode(void);

	analyzerCtrl  ctrl;
	analyzerModel model;
	analyzerView  view;
private:
	SysErrCode g_errCode;
};

extern analyzerSys appSystem;