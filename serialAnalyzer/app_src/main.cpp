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

// 1. 전역 객체 생성
analyzerCtrl  ctrl(CS_CLASSDC, L"Serial Analyzer", { 100, 100, 736, 519 });
analyzerModel model;
analyzerView  view;

// 2. 콜백 함수 선언
void MainLayout(void);
void DrawLeftPanel(void);
void DrawRightPanel(void);

// [Main 함수 시작]
int main(void)
{
	// 백엔드 및 DX11 초기화 (원본 functions 002 ~ 005 역할)
	if (!ctrl.begin(nullptr))
		return -1;

	// 배경색 설정 (원본 functions 017의 clear_color)
	ctrl.set_WindowClr(0.1f, 0.1f, 0.12f, 1.0f);

	// 메인 대쉬보드 창 설정 (원본 functions 009 역할)
	ImGuiWindowFlags dashFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	view.set_mainDash_window(ImVec2(0, 0), ImVec2(720, 480), "MainDash", dashFlags);

	// 메인 루프 (원본 functions 006 ~ 017 역할)
	while (ctrl.stillRunning() == true)
	{
		// 가짜 데이터 업데이트 (원본 functions 007)
		model.update_data(ctrl.get_deltaTime());

		ctrl.BeginFrame();

		// 뷰 레이아웃 렌더링 시작 (내부에서 MainLayout 호출)
		view.layout(MainLayout);

		ctrl.EndFrame();
	}

	// 자원 해제 (원본 functions 018)
	ctrl.end();
	return 0;
}

// --- 왼쪽 패널: U1(로그) + U2(입력) ---
void DrawLeftPanel(void)
{
	ImGui::TextColored(ImVec4(0, 1, 1, 1), "U1: Rx Log");

	// functions 011: 내부 로그창
	ImGui::BeginChild("Log", ImVec2(0, -70), true);
	for (auto& l : model.get_logs()) {
		// ImGui::TextUnformatted(l.c_str());
		ImGui::TextWrapped("%s", l.c_str()); // 수정 후: 자동 줄바꿈 적용
	}
	// 자동 스크롤
	if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
		ImGui::SetScrollHereY(1.0f);
	ImGui::EndChild();

	// functions 012: 입력창
	ImGui::PushItemWidth(-1);
	if (ImGui::InputText("##Tx", model.get_txBuffer(), ANL_TX_BUFF_SIZE, ImGuiInputTextFlags_EnterReturnsTrue)) {
		model.add_log("TX", model.get_txBuffer()); // 모델에 로그 추가
		model.clear_txBuffer();                    // 버퍼 비우기 (tx_buffer[0] = '\0' 역할)
		ImGui::SetKeyboardFocusHere(-1);           // 커서 유지
	}
	ImGui::PopItemWidth();
}

// --- 오른쪽 패널: U3(그래프) + U4/U5(설정) ---
void DrawRightPanel(void)
{
	// functions 014: U3 그래프 영역
	ImGui::BeginChild("U3", ImVec2(0, 280), true);
	if (ImPlot::BeginPlot("##Plot", ImVec2(-1, -1)))
	{
		ImPlot::SetupAxes("Time", "Value", 0, 0);

		float elapsed = model.get_elapsedTime();
		int range = *model.get_xAxisRange_ptr();

		ImPlot::SetupAxisLimits(ImAxis_X1, elapsed - (float)range, elapsed, ImGuiCond_Always);
		ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 100);

		// functions 015: 링버퍼 데이터 그리기
		ScrollingBuffer& sdata = model.get_graphData();
		if (sdata.data.size() > 0) {
			std::vector<float> x_data, y_data;
			x_data.reserve(sdata.data.size());
			y_data.reserve(sdata.data.size());

			// Offset(cursor) 기준으로 순서 정렬
			for (int i = sdata.cursor; i < (int)sdata.data.size(); ++i) {
				x_data.push_back(sdata.data[i].x);
				y_data.push_back(sdata.data[i].y);
			}
			for (int i = 0; i < sdata.cursor; ++i) {
				x_data.push_back(sdata.data[i].x);
				y_data.push_back(sdata.data[i].y);
			}

			ImPlot::PlotLine("Temperature", x_data.data(), y_data.data(), (int)x_data.size());
		}
		ImPlot::EndPlot();
	}
	ImGui::EndChild();

	// functions 016: U5 설정 영역
	ImGui::BeginChild("U4U5", ImVec2(0, 0), true);
	ImGui::Text("U5: Graph Config");
	ImGui::SliderInt("X Range", model.get_xAxisRange_ptr(), 1, 20);
	ImGui::EndChild();
}

// --- 최상위 화면 조립 ---
void MainLayout(void)
{
	// functions 010: Left 차일드
	// 뷰 객체의 childWindow 콜백을 통해 영역 생성
	view.childWindow(DrawLeftPanel, "Left", ImVec2(ImGui::GetContentRegionAvail().x * 0.4f, 0), true);

	// functions 013: 좌우 분할
	ImGui::SameLine();

	// 오른쪽 그룹 (U3, U4U5 묶음)
	ImGui::BeginGroup();
	DrawRightPanel(); // 내부에서 BeginChild로 알아서 분할됨
	ImGui::EndGroup();
}