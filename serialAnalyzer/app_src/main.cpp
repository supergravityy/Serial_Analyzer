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

void U1_Log(void)
{
	for (auto& l : model.get_logs())
	{
		ImGui::TextWrapped("%s", l.c_str()); // 수정 후: 자동 줄바꿈 적용
	}
	// 최신 로그 갱신 자동 스크롤
	if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
		ImGui::SetScrollHereY(1.0f);
}

void U2_InputText(void)
{
	ImGui::PushItemWidth(-1);
	if (ImGui::InputText("##U2_InputText", model.get_txBuffer(), ANL_TX_BUFF_SIZE, ImGuiInputTextFlags_EnterReturnsTrue))
	{
		model.add_log("TX", model.get_txBuffer()); // 모델에 로그 추가
		model.clear_txBuffer();                    // 버퍼 비우기 (tx_buffer[0] = '\0' 역할)
		ImGui::SetKeyboardFocusHere(-1);           // 커서 유지
	}
	ImGui::PopItemWidth();
}

void DrawLeftPanel(void)
{
	ImGui::TextColored(ImVec4(0, 1, 1, 1), "U1: Log Window");

	// U1, 내부 로그 창
	view.childWindow(U1_Log, "U1_Log", ImVec2(0, -70), true);

	// U2, 입력창
	U2_InputText();
}

void U3_Graph(void)
{
	float elapsed_T = model.get_elapsedTime();
	int range = *(model.get_xAxisRange_ptr());
	std::vector<float> x_data, y_data;
	ScrollingBuffer& sdata = model.get_graphData();

	if (ImPlot::BeginPlot("##Graph", ImVec2(-1, -1)) == true)
	{
		// 1. 그래프 축 설정
		ImPlot::SetupAxes("Time", "Value", 0, 0);

		ImPlot::SetupAxisLimits(ImAxis_X1, elapsed_T - (float)range, elapsed_T, ImGuiCond_Always);
		ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 100);

		// 2. 데이터 그리기
		if (sdata.data.size() > 0)
		{
			// y축, x축 따로 저장
			x_data.reserve(sdata.data.size());
			y_data.reserve(sdata.data.size());

			// 링 버퍼이기에 cursor 부터 오름차순으로 저장
			for (int i = sdata.cursor; i < (int)sdata.data.size(); i++)
			{
				x_data.push_back(sdata.data[i].x);
				y_data.push_back(sdata.data[i].y);
			}
			for (int i = 0; i < sdata.cursor; i++)
			{
				x_data.push_back(sdata.data[i].x);
				y_data.push_back(sdata.data[i].y);
			}
			// todo : "Temperature" 대신 시리얼 데이터에서 받은 id를 활용할 것
			ImPlot::PlotLine("Temperature", x_data.data(), y_data.data(), (int)x_data.size());
		}
		ImPlot::EndPlot();
	}
}

void U4_SelDomain(void)
{
	ImGui::TextColored(ImVec4(1, 1, 0, 1), "U4: Domain List"); // todo : 나중에 색깔을 전부 imVec 디파인문으로 지정하면 더 쓰기 쉬울듯
	ImGui::Separator(); // 구분선 -> 노션의 ---

	// todo : 추후 모델에서 도메인 리스트를 받아와서 버튼으로 생성할 자리
	if (ImGui::Button("Domain: Motor_Speed")) { /* 그래프 데이터 교체 로직 */ }
	if (ImGui::Button("Domain: Phase_Current")) { /* 그래프 데이터 교체 로직 */ }
}

void U5_configSerial(void)
{
	ImGui::TextColored(ImVec4(1, 1, 0, 1), "U5: Serial Config");
	ImGui::Separator();

	// 그래프 X축 조절 (기존 기능)
	ImGui::SliderInt("X Range", model.get_xAxisRange_ptr(), 1, 20);

	// 보드레이트 설정
	ImGui::Text("Baudrate: 115200");
	
	// 시리얼 포트 연결 로직
	if (ImGui::Button("Connect")) { /* 시리얼 연결 로직 */ }
}

void DrawRightPanel(void)
{
	// U3, 그래프 
	view.childWindow(U3_Graph, "U3_Graph", ImVec2(0, 280), true);

	// U4, 도메인 선택 - 오른쪽 영역 절반 사용	
	view.childWindow(U4_SelDomain, "U4_selDomain", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0), true);

	ImGui::SameLine(); // U4와 U5를 나란히 배치

	// U5, 시리얼 설정 - 남은 공간 전부 사용
	view.childWindow(U5_configSerial, "U5_config_Serial", ImVec2(0, 0), true);

}

// --- 최상위 화면 조립 ---
void MainLayout(void)
{
	view.childWindow(DrawLeftPanel, "Left", ImVec2(ImGui::GetContentRegionAvail().x * 0.4f, 0), true);

	// 좌우 분할
	ImGui::SameLine();

	// 오른쪽 그룹 (U3, U4, U5 묶음)
	ImGui::BeginGroup();
	view.childWindow(DrawRightPanel, "Right", ImVec2(0, 0), true);
	ImGui::EndGroup();
}