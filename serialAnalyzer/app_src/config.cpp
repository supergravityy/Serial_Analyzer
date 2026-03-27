#include "config.h"
#include "imgui.h"
#include "implot.h"
#include <vector>
#include <string>
#include <math.h>
#include "ctrl.h"
#include "view.h"
#include "model.h"
#include "system.h"
#include "serial.h"

static void printErrMsg(void)
{
	SysErrCode temp = appSystem.get_global_errCode();
	if (temp == G_ERR_NONE) return;

	ImVec4 errCol = ImVec4_COLOR_RED;

	// 텍스트 래핑 시작 (현재 창 너비 기준)
	ImGui::PushTextWrapPos(0.0f);

	switch (temp)
	{
	case G_ERR_INIT_INVALID_SPEC:
		ImGui::TextColored(errCol, "Err : [Control] Invalid Window Specification (Width/Height)");
		break;
	case G_ERR_INIT_DX11_ON:
		ImGui::TextColored(errCol, "Err : [Control] DirectX 11 Device Creation Failed");
		break;
	case G_ERR_INIT_HANDSHAKE_WIN_DX:
		ImGui::TextColored(errCol, "Err : [Control] Win32-DX11 Backend Handshake Failed");
		break;
	case G_ERR_MAIN_WINDOW_INVALID_DATA:
		ImGui::TextColored(errCol, "Err : [View] Main Dashboard Data is Invalid");
		break;
	case G_ERR_MAIN_WINDOW_NO_CALLBACK:
		ImGui::TextColored(errCol, "Err : [View] Main Layout Callback is Missing");
		break;
	case G_ERR_CHILD_WINDOW_NO_CALLBACK:
		ImGui::TextColored(errCol, "Err : [View] Child Window Callback is Missing");
		break;
	case G_ERR_CHILD_WINDOW_INVALID_DATA:
		ImGui::TextColored(errCol, "Err : [View] Child Window ID or Size is Invalid");
		break;
	case G_ERR_SERIAL_INIT_INVALID_SPEC:
		ImGui::TextColored(errCol, "Err : [Serial] Invalid Configuration (Baud/Parity/Stop)");
		break;
	case G_ERR_SERIAL_INIT_CANT_OPEN:
		ImGui::TextColored(errCol, "Err : [Serial] Port Open Failed (Check Cable/Port)");
		break;
	case G_ERR_SERIAL_RUN_COMM_FAIL:
		ImGui::TextColored(errCol, "Err : [Serial] Communication Parameter Setup Failed");
		break;
	case G_ERR_SERIAL_RUN_READ_FAIL:
		ImGui::TextColored(errCol, "Err : [Serial] Data Read Failed (Hardware Disconnected?)");
		break;
	default:
		ImGui::TextColored(errCol, "Err : Unknown System Error (Code: %d)", (int)temp);
		break;
	}

	ImGui::PopTextWrapPos(); // 래핑 종료
	ImGui::Separator();
}

void U1_Log(void)
{
	printErrMsg();

	for (auto& l : appSystem.model.get_logs())
	{
		ImGui::TextWrapped("%s", l.c_str()); // 자동 줄바꿈 적용
	}

	// 최신 로그 갱신 자동 스크롤
	if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
		ImGui::SetScrollHereY(1.0f);
}

void U2_InputText(void)
{
	ImGui::PushItemWidth(-1);
	if (ImGui::InputText("##U2_InputText", appSystem.model.get_txBuffer(), ANL_TX_BUFF_SIZE, ImGuiInputTextFlags_EnterReturnsTrue))
	{
		appSystem.model.add_log("TX", appSystem.model.get_txBuffer());	// 모델에 로그 추가
		appSystem.model.clear_txBuffer();								// 버퍼 비우기 (tx_buffer[0] = '\0' 역할)
		ImGui::SetKeyboardFocusHere(-1);								// 커서 유지
	}
	ImGui::PopItemWidth();
}

void DrawLeftPanel(void)
{
	ImGui::TextColored(ImVec4_COLOR_CYAN, "Log Window");

	// U1, 내부 로그 창
	appSystem.view.childWindow(U1_Log, "U1_Log", ImVec2(0, -50), true);

	// U2, 입력창
	U2_InputText();
}

void U3_Graph(void)
{
	float elapsed_T = appSystem.model.get_elapsedTime();
	int range = *(appSystem.model.get_xAxisRange_ptr());
	std::vector<float> x_data, y_data;

	// 1. 모델에서 현재 선택된 도메인의 이름과 버퍼 주소를 가져옴
	std::string targetDomain = appSystem.model.get_targetDomain();
	ScrollingBuffer* sdata = appSystem.model.get_targetBuffer();

	if (ImPlot::BeginPlot("##Graph", ImVec2(-1, -1)) == true)
	{
		// 1. 그래프 축 설정
		ImPlot::SetupAxes("Time", "Value", 0, 0);
		ImPlot::SetupAxisLimits(ImAxis_X1, elapsed_T - (float)range, elapsed_T, ImGuiCond_Always);

		// Y축은 필요에 따라 Auto-fit(자동 조절)으로 바꾸셔도 좋습니다. 
		// 일단 기존 코드대로 0~100으로 유지합니다.
		ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 100);

		// 2. 데이터 그리기 (선택된 도메인이 있고 데이터가 비어있지 않을 때만)
		if (sdata != nullptr && sdata->data.size() > 0)
		{
			x_data.reserve(sdata->data.size());
			y_data.reserve(sdata->data.size());

			// 링 버퍼이기에 cursor 부터 오름차순으로 저장
			for (int i = sdata->cursor; i < (int)sdata->data.size(); i++)
			{
				x_data.push_back(sdata->data[i].x);
				y_data.push_back(sdata->data[i].y);
			}
			for (int i = 0; i < sdata->cursor; i++)
			{
				x_data.push_back(sdata->data[i].x);
				y_data.push_back(sdata->data[i].y);
			}

			// 라벨 이름을 하드코딩("Temperature") 대신 실제 도메인 이름으로 출력
			ImPlot::PlotLine(targetDomain.c_str(), x_data.data(), y_data.data(), (int)x_data.size());
		}
		ImPlot::EndPlot();
	}
}

void U4_SelDomain(void)
{
	std::vector<std::string> domains;

	ImGui::TextColored(ImVec4_COLOR_YELLOW, "U4: Domain & Control");
	ImGui::Separator();

	ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 60.0f);
	ImGui::SliderInt("XRange", appSystem.model.get_xAxisRange_ptr(), 1, 10); 
	ImGui::PopItemWidth(); // 누락되었던 Pop 추가 (ImGui 에러 방지)
	ImGui::Spacing();

	ImGui::Text("Detected Domains:");

	// 높이를 -25 정도로 설정하여 하단 Reset 버튼 자리를 확보하고 나머지는 스크롤 가능하게 함
	ImGui::BeginChild("DomainList", ImVec2(0, -25), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

	// 1. 모델에서 파싱되어 저장된 모든 도메인 이름 리스트를 가져옴
	appSystem.model.get_domain_names(domains);
	std::string currentTarget = appSystem.model.get_targetDomain();

	// 2. 리스트를 순회하며 동적 버튼 생성
	for (const auto& domain_name : domains) 
	{
		// 현재 뷰에 선택된 도메인이면 색상을 다르게(파란색 계열) 칠해서 하이라이트
		bool is_selected = (domain_name == currentTarget);
		if (is_selected) 
		{
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4_COLOR_CYAN);
		}

		// 버튼을 누르면 해당 도메인을 Target으로 지정함
		if (ImGui::Button(domain_name.c_str(), ImVec2(-1, 0))) 
		{
			appSystem.model.set_targetDomain(domain_name);
		}

		if (is_selected) 
		{
			ImGui::PopStyleColor();
		}
	}

	ImGui::EndChild();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4_COLOR_GRAY);
	if (ImGui::Button("Reset All Data", ImVec2(-1, 0))) {
		appSystem.model.clear_graphData();   // 모든 도메인의 버퍼 내용을 싹 비움
		appSystem.model.get_logs().clear();  // 로그 비움
		appSystem.model.add_log("SYS", "All data has been reset.");
	}
	ImGui::PopStyleColor();
}

void U5_configSerial(void)
{
	// --- 1. UI 내부 상태 관리를 위한 정적 변수 ---
	static std::vector<std::string> portList;
	static int selPortIdx = 0;
	static int selDataIdx = 3;  // 기본 8-bit (리스트의 4번째)
	static int selStopIdx = 0;  // 기본 1-stop
	static int selParityIdx = 0; // 기본 None
	static char baudBuf[16] = "115200"; // 보드레이트 입력용 버퍼
	static float lastScanTime = 0;

	ImGui::TextColored(ImVec4_COLOR_YELLOW, "U5: Serial Config");
	ImGui::Separator();

	// 2초마다 포트 리스트 동적 스캔
	if (ImGui::GetTime() - lastScanTime > 2.0f || portList.empty()) {
		portList = analyzerSerial::get_ports();
		lastScanTime = (float)ImGui::GetTime();
	}

	// --- 2. 연결 상태에 따른 인터락(Interlock) 설정 ---
	bool isConnected = appSystem.serial.is_opened();

	if (isConnected) ImGui::BeginDisabled(); // 연결 중이면 모든 설정창 잠금

	// A. 포트 이름 (Combo)
	const char* portPreview = (portList.empty()) ? "N/A" : portList[selPortIdx].c_str();
	if (ImGui::BeginCombo("Port", portPreview)) {
		for (int i = 0; i < (int)portList.size(); i++) {
			if (ImGui::Selectable(portList[i].c_str(), selPortIdx == i)) selPortIdx = i;
		}
		ImGui::EndCombo();
	}

	// B. 보드레이트 (InputText) - 숫자만 입력받도록 설정
	ImGui::InputText("Baudrate", baudBuf, IM_ARRAYSIZE(baudBuf), ImGuiInputTextFlags_CharsDecimal);

	// C. 데이터 비트 (Combo: 5, 6, 7, 8)
	const char* dataBits[] = { "5", "6", "7", "8" };
	ImGui::Combo("DataBits", &selDataIdx, dataBits, IM_ARRAYSIZE(dataBits));

	// D. 스톱 비트 (Combo: 1, 2)
	const char* stopBits[] = { "1", "2" };
	ImGui::Combo("StopBits", &selStopIdx, stopBits, IM_ARRAYSIZE(stopBits));

	// E. 패리티 비트 (Combo: None, Odd, Even, Mark, Space)
	const char* parityItems[] = { "None", "Odd", "Even" };
	ImGui::Combo("Parity", &selParityIdx, parityItems, IM_ARRAYSIZE(parityItems));
	
	if (isConnected) ImGui::EndDisabled(); // 잠금 해제

	ImGui::Spacing();

	// --- 3. Connect / Disconnect 버튼 로직 ---
	if (!isConnected)
	{
		// [Connect 버튼]
		if (ImGui::Button("Connect", ImVec2(-1, 30))) {
			if (!portList.empty()) {
				unsigned int baud = (unsigned int)atoi(baudBuf);
				unsigned int data = (unsigned int)atoi(dataBits[selDataIdx]);
				unsigned int stop = (unsigned int)atoi(stopBits[selStopIdx]);
				unsigned int parity = (unsigned int)selParityIdx;

				// 실제 연결 시도
				if (appSystem.serial.open(portList[selPortIdx].c_str(), baud, data, stop, parity)) {
					appSystem.model.add_log("SYS", "Serial Port Connected.");
				}
				else {
					// 실패 시 에러는 이미 serial.errCode에 담겨있으므로 U1에서 출력됨
				}
			}
		}
	}
	else
	{
		// [Disconnect 버튼]
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4_COLOR_RED); // 빨간색 버튼
		if (ImGui::Button("Disconnect", ImVec2(-1, 30))) {
			appSystem.serial.close();
			appSystem.model.add_log("SYS", "Serial Port Disconnected.");
		}
		ImGui::PopStyleColor();
	}
}

void DrawRightPanel(void)
{
	// U3 그래프 높이를 280 -> 220으로 축소하여 U5의 스크롤을 방지함
	appSystem.view.childWindow(U3_Graph, "U3_Graph", ImVec2(0, 250), true);

	// 하단 영역 (U4, U5)
	appSystem.view.childWindow(U4_SelDomain, "U4_selDomain", ImVec2(ImGui::GetContentRegionAvail().x * 0.5f, 0), true);

	ImGui::SameLine();

	appSystem.view.childWindow(U5_configSerial, "U5_config_Serial", ImVec2(0, 0), true);
}

// --- 최상위 화면 조립 : 반드시 함수원형은 유지되어야 함---
void MainLayout(void)
{
	appSystem.view.childWindow(DrawLeftPanel, "Left", ImVec2(ImGui::GetContentRegionAvail().x * 0.4f, 0), true);

	// 좌우 분할
	ImGui::SameLine();

	// 오른쪽 그룹 (U3, U4, U5 묶음)
	ImGui::BeginGroup();
	appSystem.view.childWindow(DrawRightPanel, "Right", ImVec2(0, 0), true);
	ImGui::EndGroup();
}