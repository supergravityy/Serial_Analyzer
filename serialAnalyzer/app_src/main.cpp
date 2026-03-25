// 원래 windows gui 프로그램은 win main으로 시작해야 함 -> 표준 main으로 시작하게 해주는 설정
#pragma comment(linker, "/entry:mainCRTStartup") 
#include "imgui.h"
#include "implot.h"
#include <vector>
#include <string>
#include <math.h>
#include "ctrl.h"
#include "view.h"
#include "model.h"
#include "system.h"

//// 1. 전역 객체 생성
//analyzerCtrl  ctrl(CS_CLASSDC, L"Serial Analyzer", { 100, 100, 736, 519 });
//analyzerModel model;
//analyzerView  view;

extern analyzerSys appSystem;

// [Main 함수 시작]
int main(void)
{
	// use the global appSystem (defined in system.cpp)
	if (appSystem.init() == true)
	{
		// 3. 무한 루프 가동
		appSystem.run();
	}

	return 0;
}