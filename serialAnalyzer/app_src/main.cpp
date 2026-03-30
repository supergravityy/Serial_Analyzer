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

extern analyzerSys appSystem;

int main(void)
{
	
	if (appSystem.init() == true)
	{
		appSystem.run();
	}

	return 0;
}